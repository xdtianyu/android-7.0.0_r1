/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#define LOG_TAG "hwcomposer-drm"

#include "drmhwcomposer.h"
#include "drmeventlistener.h"
#include "drmresources.h"
#include "platform.h"
#include "virtualcompositorworker.h"
#include "vsyncworker.h"

#include <stdlib.h>

#include <cinttypes>
#include <map>
#include <vector>
#include <sstream>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <sw_sync.h>
#include <sync/sync.h>
#include <utils/Trace.h>

#define UM_PER_INCH 25400

namespace android {

class DummySwSyncTimeline {
 public:
  int Init() {
    int ret = timeline_fd_.Set(sw_sync_timeline_create());
    if (ret < 0)
      return ret;
    return 0;
  }

  UniqueFd CreateDummyFence() {
    int ret = sw_sync_fence_create(timeline_fd_.get(), "dummy fence",
                                   timeline_pt_ + 1);
    if (ret < 0) {
      ALOGE("Failed to create dummy fence %d", ret);
      return ret;
    }

    UniqueFd ret_fd(ret);

    ret = sw_sync_timeline_inc(timeline_fd_.get(), 1);
    if (ret) {
      ALOGE("Failed to increment dummy sync timeline %d", ret);
      return ret;
    }

    ++timeline_pt_;
    return ret_fd;
  }

 private:
  UniqueFd timeline_fd_;
  int timeline_pt_ = 0;
};

struct CheckedOutputFd {
  CheckedOutputFd(int *fd, const char *description,
                  DummySwSyncTimeline &timeline)
      : fd_(fd), description_(description), timeline_(timeline) {
  }
  CheckedOutputFd(CheckedOutputFd &&rhs)
      : description_(rhs.description_), timeline_(rhs.timeline_) {
    std::swap(fd_, rhs.fd_);
  }

  CheckedOutputFd &operator=(const CheckedOutputFd &rhs) = delete;

  ~CheckedOutputFd() {
    if (fd_ == NULL)
      return;

    if (*fd_ >= 0)
      return;

    *fd_ = timeline_.CreateDummyFence().Release();

    if (*fd_ < 0)
      ALOGE("Failed to fill %s (%p == %d) before destruction",
            description_.c_str(), fd_, *fd_);
  }

 private:
  int *fd_ = NULL;
  std::string description_;
  DummySwSyncTimeline &timeline_;
};

typedef struct hwc_drm_display {
  struct hwc_context_t *ctx;
  int display;

  std::vector<uint32_t> config_ids;

  VSyncWorker vsync_worker;
} hwc_drm_display_t;

class DrmHotplugHandler : public DrmEventHandler {
 public:
  void Init(DrmResources *drm, const struct hwc_procs *procs) {
    drm_ = drm;
    procs_ = procs;
  }

  void HandleEvent(uint64_t timestamp_us) {
    for (auto &conn : drm_->connectors()) {
      drmModeConnection old_state = conn->state();

      conn->UpdateModes();

      drmModeConnection cur_state = conn->state();

      if (cur_state == old_state)
        continue;

      ALOGI("%s event @%" PRIu64 " for connector %u\n",
            cur_state == DRM_MODE_CONNECTED ? "Plug" : "Unplug", timestamp_us,
            conn->id());

      if (cur_state == DRM_MODE_CONNECTED) {
        // Take the first one, then look for the preferred
        DrmMode mode = *(conn->modes().begin());
        for (auto &m : conn->modes()) {
          if (m.type() & DRM_MODE_TYPE_PREFERRED) {
            mode = m;
            break;
          }
        }
        ALOGI("Setting mode %dx%d for connector %d\n", mode.h_display(),
              mode.v_display(), conn->id());
        int ret = drm_->SetDisplayActiveMode(conn->display(), mode);
        if (ret) {
          ALOGE("Failed to set active config %d", ret);
          return;
        }
      } else {
        int ret = drm_->SetDpmsMode(conn->display(), DRM_MODE_DPMS_OFF);
        if (ret) {
          ALOGE("Failed to set dpms mode off %d", ret);
          return;
        }
      }

      procs_->hotplug(procs_, conn->display(),
                      cur_state == DRM_MODE_CONNECTED ? 1 : 0);
    }
  }

 private:
  DrmResources *drm_ = NULL;
  const struct hwc_procs *procs_ = NULL;
};

struct hwc_context_t {
  // map of display:hwc_drm_display_t
  typedef std::map<int, hwc_drm_display_t> DisplayMap;

  ~hwc_context_t() {
    virtual_compositor_worker.Exit();
  }

  hwc_composer_device_1_t device;
  hwc_procs_t const *procs = NULL;

  DisplayMap displays;
  DrmResources drm;
  std::unique_ptr<Importer> importer;
  const gralloc_module_t *gralloc;
  DummySwSyncTimeline dummy_timeline;
  VirtualCompositorWorker virtual_compositor_worker;
  DrmHotplugHandler hotplug_handler;
};

static native_handle_t *dup_buffer_handle(buffer_handle_t handle) {
  native_handle_t *new_handle =
      native_handle_create(handle->numFds, handle->numInts);
  if (new_handle == NULL)
    return NULL;

  const int *old_data = handle->data;
  int *new_data = new_handle->data;
  for (int i = 0; i < handle->numFds; i++) {
    *new_data = dup(*old_data);
    old_data++;
    new_data++;
  }
  memcpy(new_data, old_data, sizeof(int) * handle->numInts);

  return new_handle;
}

static void free_buffer_handle(native_handle_t *handle) {
  int ret = native_handle_close(handle);
  if (ret)
    ALOGE("Failed to close native handle %d", ret);
  ret = native_handle_delete(handle);
  if (ret)
    ALOGE("Failed to delete native handle %d", ret);
}

const hwc_drm_bo *DrmHwcBuffer::operator->() const {
  if (importer_ == NULL) {
    ALOGE("Access of non-existent BO");
    exit(1);
    return NULL;
  }
  return &bo_;
}

void DrmHwcBuffer::Clear() {
  if (importer_ != NULL) {
    importer_->ReleaseBuffer(&bo_);
    importer_ = NULL;
  }
}

int DrmHwcBuffer::ImportBuffer(buffer_handle_t handle, Importer *importer) {
  hwc_drm_bo tmp_bo;

  int ret = importer->ImportBuffer(handle, &tmp_bo);
  if (ret)
    return ret;

  if (importer_ != NULL) {
    importer_->ReleaseBuffer(&bo_);
  }

  importer_ = importer;

  bo_ = tmp_bo;

  return 0;
}

int DrmHwcNativeHandle::CopyBufferHandle(buffer_handle_t handle,
                                         const gralloc_module_t *gralloc) {
  native_handle_t *handle_copy = dup_buffer_handle(handle);
  if (handle_copy == NULL) {
    ALOGE("Failed to duplicate handle");
    return -ENOMEM;
  }

  int ret = gralloc->registerBuffer(gralloc, handle_copy);
  if (ret) {
    ALOGE("Failed to register buffer handle %d", ret);
    free_buffer_handle(handle_copy);
    return ret;
  }

  Clear();

  gralloc_ = gralloc;
  handle_ = handle_copy;

  return 0;
}

DrmHwcNativeHandle::~DrmHwcNativeHandle() {
  Clear();
}

void DrmHwcNativeHandle::Clear() {
  if (gralloc_ != NULL && handle_ != NULL) {
    gralloc_->unregisterBuffer(gralloc_, handle_);
    free_buffer_handle(handle_);
    gralloc_ = NULL;
    handle_ = NULL;
  }
}

int DrmHwcLayer::InitFromHwcLayer(hwc_layer_1_t *sf_layer, Importer *importer,
                                  const gralloc_module_t *gralloc) {
  sf_handle = sf_layer->handle;
  alpha = sf_layer->planeAlpha;

  source_crop = DrmHwcRect<float>(
      sf_layer->sourceCropf.left, sf_layer->sourceCropf.top,
      sf_layer->sourceCropf.right, sf_layer->sourceCropf.bottom);
  display_frame = DrmHwcRect<int>(
      sf_layer->displayFrame.left, sf_layer->displayFrame.top,
      sf_layer->displayFrame.right, sf_layer->displayFrame.bottom);

  transform = 0;
  // 270* and 180* cannot be combined with flips. More specifically, they
  // already contain both horizontal and vertical flips, so those fields are
  // redundant in this case. 90* rotation can be combined with either horizontal
  // flip or vertical flip, so treat it differently
  if (sf_layer->transform == HWC_TRANSFORM_ROT_270) {
    transform = DrmHwcTransform::kRotate270;
  } else if (sf_layer->transform == HWC_TRANSFORM_ROT_180) {
    transform = DrmHwcTransform::kRotate180;
  } else {
    if (sf_layer->transform & HWC_TRANSFORM_FLIP_H)
      transform |= DrmHwcTransform::kFlipH;
    if (sf_layer->transform & HWC_TRANSFORM_FLIP_V)
      transform |= DrmHwcTransform::kFlipV;
    if (sf_layer->transform & HWC_TRANSFORM_ROT_90)
      transform |= DrmHwcTransform::kRotate90;
  }

  switch (sf_layer->blending) {
    case HWC_BLENDING_NONE:
      blending = DrmHwcBlending::kNone;
      break;
    case HWC_BLENDING_PREMULT:
      blending = DrmHwcBlending::kPreMult;
      break;
    case HWC_BLENDING_COVERAGE:
      blending = DrmHwcBlending::kCoverage;
      break;
    default:
      ALOGE("Invalid blending in hwc_layer_1_t %d", sf_layer->blending);
      return -EINVAL;
  }

  int ret = buffer.ImportBuffer(sf_layer->handle, importer);
  if (ret)
    return ret;

  ret = handle.CopyBufferHandle(sf_layer->handle, gralloc);
  if (ret)
    return ret;

  ret = gralloc->perform(gralloc, GRALLOC_MODULE_PERFORM_GET_USAGE,
                         handle.get(), &gralloc_buffer_usage);
  if (ret) {
    ALOGE("Failed to get usage for buffer %p (%d)", handle.get(), ret);
    return ret;
  }

  return 0;
}

static void hwc_dump(struct hwc_composer_device_1 *dev, char *buff,
                     int buff_len) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  std::ostringstream out;

  ctx->drm.compositor()->Dump(&out);
  std::string out_str = out.str();
  strncpy(buff, out_str.c_str(),
          std::min((size_t)buff_len, out_str.length() + 1));
  buff[buff_len - 1] = '\0';
}

static bool hwc_skip_layer(const std::pair<int, int> &indices, int i) {
  return indices.first >= 0 && i >= indices.first && i <= indices.second;
}

static int hwc_prepare(hwc_composer_device_1_t *dev, size_t num_displays,
                       hwc_display_contents_1_t **display_contents) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;

  for (int i = 0; i < (int)num_displays; ++i) {
    if (!display_contents[i])
      continue;

    bool use_framebuffer_target = false;
    DrmMode mode;
    if (i == HWC_DISPLAY_VIRTUAL) {
      use_framebuffer_target = true;
    } else {
      DrmConnector *c = ctx->drm.GetConnectorForDisplay(i);
      if (!c) {
        ALOGE("Failed to get DrmConnector for display %d", i);
        return -ENODEV;
      }
      mode = c->active_mode();
    }

    // Since we can't composite HWC_SKIP_LAYERs by ourselves, we'll let SF
    // handle all layers in between the first and last skip layers. So find the
    // outer indices and mark everything in between as HWC_FRAMEBUFFER
    std::pair<int, int> skip_layer_indices(-1, -1);
    int num_layers = display_contents[i]->numHwLayers;
    for (int j = 0; !use_framebuffer_target && j < num_layers; ++j) {
      hwc_layer_1_t *layer = &display_contents[i]->hwLayers[j];

      if (!(layer->flags & HWC_SKIP_LAYER))
        continue;

      if (skip_layer_indices.first == -1)
        skip_layer_indices.first = j;
      skip_layer_indices.second = j;
    }

    for (int j = 0; j < num_layers; ++j) {
      hwc_layer_1_t *layer = &display_contents[i]->hwLayers[j];

      if (!use_framebuffer_target && !hwc_skip_layer(skip_layer_indices, j)) {
        // If the layer is off the screen, don't earmark it for an overlay.
        // We'll leave it as-is, which effectively just drops it from the frame
        const hwc_rect_t *frame = &layer->displayFrame;
        if ((frame->right - frame->left) <= 0 ||
            (frame->bottom - frame->top) <= 0 ||
            frame->right <= 0 || frame->bottom <= 0 ||
            frame->left >= (int)mode.h_display() ||
            frame->top >= (int)mode.v_display())
            continue;

        if (layer->compositionType == HWC_FRAMEBUFFER)
          layer->compositionType = HWC_OVERLAY;
      } else {
        switch (layer->compositionType) {
          case HWC_OVERLAY:
          case HWC_BACKGROUND:
          case HWC_SIDEBAND:
          case HWC_CURSOR_OVERLAY:
            layer->compositionType = HWC_FRAMEBUFFER;
            break;
        }
      }
    }
  }

  return 0;
}

static void hwc_add_layer_to_retire_fence(
    hwc_layer_1_t *layer, hwc_display_contents_1_t *display_contents) {
  if (layer->releaseFenceFd < 0)
    return;

  if (display_contents->retireFenceFd >= 0) {
    int old_retire_fence = display_contents->retireFenceFd;
    display_contents->retireFenceFd =
        sync_merge("dc_retire", old_retire_fence, layer->releaseFenceFd);
    close(old_retire_fence);
  } else {
    display_contents->retireFenceFd = dup(layer->releaseFenceFd);
  }
}

static int hwc_set(hwc_composer_device_1_t *dev, size_t num_displays,
                   hwc_display_contents_1_t **sf_display_contents) {
  ATRACE_CALL();
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  int ret = 0;

  std::vector<CheckedOutputFd> checked_output_fences;
  std::vector<DrmHwcDisplayContents> displays_contents;
  std::vector<DrmCompositionDisplayLayersMap> layers_map;
  std::vector<std::vector<size_t>> layers_indices;
  displays_contents.reserve(num_displays);
  // layers_map.reserve(num_displays);
  layers_indices.reserve(num_displays);

  // Phase one does nothing that would cause errors. Only take ownership of FDs.
  for (size_t i = 0; i < num_displays; ++i) {
    hwc_display_contents_1_t *dc = sf_display_contents[i];
    displays_contents.emplace_back();
    DrmHwcDisplayContents &display_contents = displays_contents.back();
    layers_indices.emplace_back();
    std::vector<size_t> &indices_to_composite = layers_indices.back();

    if (!sf_display_contents[i])
      continue;

    if (i == HWC_DISPLAY_VIRTUAL) {
      ctx->virtual_compositor_worker.QueueComposite(dc);
      continue;
    }

    std::ostringstream display_index_formatter;
    display_index_formatter << "retire fence for display " << i;
    std::string display_fence_description(display_index_formatter.str());
    checked_output_fences.emplace_back(&dc->retireFenceFd,
                                       display_fence_description.c_str(),
                                       ctx->dummy_timeline);
    display_contents.retire_fence = OutputFd(&dc->retireFenceFd);

    size_t num_dc_layers = dc->numHwLayers;
    int framebuffer_target_index = -1;
    for (size_t j = 0; j < num_dc_layers; ++j) {
      hwc_layer_1_t *sf_layer = &dc->hwLayers[j];
      if (sf_layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
        framebuffer_target_index = j;
        break;
      }
    }

    for (size_t j = 0; j < num_dc_layers; ++j) {
      hwc_layer_1_t *sf_layer = &dc->hwLayers[j];

      display_contents.layers.emplace_back();
      DrmHwcLayer &layer = display_contents.layers.back();

      // In prepare() we marked all layers FRAMEBUFFER between SKIP_LAYER's.
      // This means we should insert the FB_TARGET layer in the composition
      // stack at the location of the first skip layer, and ignore the rest.
      if (sf_layer->flags & HWC_SKIP_LAYER) {
        if (framebuffer_target_index < 0)
          continue;
        int idx = framebuffer_target_index;
        framebuffer_target_index = -1;
        hwc_layer_1_t *fbt_layer = &dc->hwLayers[idx];
        if (!fbt_layer->handle || (fbt_layer->flags & HWC_SKIP_LAYER)) {
          ALOGE("Invalid HWC_FRAMEBUFFER_TARGET with HWC_SKIP_LAYER present");
          continue;
        }
        indices_to_composite.push_back(idx);
        continue;
      }

      if (sf_layer->compositionType == HWC_OVERLAY)
        indices_to_composite.push_back(j);

      layer.acquire_fence.Set(sf_layer->acquireFenceFd);
      sf_layer->acquireFenceFd = -1;

      std::ostringstream layer_fence_formatter;
      layer_fence_formatter << "release fence for layer " << j << " of display "
                            << i;
      std::string layer_fence_description(layer_fence_formatter.str());
      checked_output_fences.emplace_back(&sf_layer->releaseFenceFd,
                                         layer_fence_description.c_str(),
                                         ctx->dummy_timeline);
      layer.release_fence = OutputFd(&sf_layer->releaseFenceFd);
    }

    // This is a catch-all in case we get a frame without any overlay layers, or
    // skip layers, but with a value fb_target layer. This _shouldn't_ happen,
    // but it's not ruled out by the hwc specification
    if (indices_to_composite.empty() && framebuffer_target_index >= 0) {
      hwc_layer_1_t *sf_layer = &dc->hwLayers[framebuffer_target_index];
      if (!sf_layer->handle || (sf_layer->flags & HWC_SKIP_LAYER)) {
        ALOGE(
            "Expected valid layer with HWC_FRAMEBUFFER_TARGET when all "
            "HWC_OVERLAY layers are skipped.");
        ret = -EINVAL;
      }
      indices_to_composite.push_back(framebuffer_target_index);
    }
  }

  if (ret)
    return ret;

  for (size_t i = 0; i < num_displays; ++i) {
    hwc_display_contents_1_t *dc = sf_display_contents[i];
    DrmHwcDisplayContents &display_contents = displays_contents[i];
    if (!sf_display_contents[i] || i == HWC_DISPLAY_VIRTUAL)
      continue;

    layers_map.emplace_back();
    DrmCompositionDisplayLayersMap &map = layers_map.back();
    map.display = i;
    map.geometry_changed =
        (dc->flags & HWC_GEOMETRY_CHANGED) == HWC_GEOMETRY_CHANGED;
    std::vector<size_t> &indices_to_composite = layers_indices[i];
    for (size_t j : indices_to_composite) {
      hwc_layer_1_t *sf_layer = &dc->hwLayers[j];

      DrmHwcLayer &layer = display_contents.layers[j];

      ret = layer.InitFromHwcLayer(sf_layer, ctx->importer.get(), ctx->gralloc);
      if (ret) {
        ALOGE("Failed to init composition from layer %d", ret);
        return ret;
      }
      map.layers.emplace_back(std::move(layer));
    }
  }

  std::unique_ptr<DrmComposition> composition(
      ctx->drm.compositor()->CreateComposition(ctx->importer.get()));
  if (!composition) {
    ALOGE("Drm composition init failed");
    return -EINVAL;
  }

  ret = composition->SetLayers(layers_map.size(), layers_map.data());
  if (ret) {
    return -EINVAL;
  }

  ret = ctx->drm.compositor()->QueueComposition(std::move(composition));
  if (ret) {
    return -EINVAL;
  }

  for (size_t i = 0; i < num_displays; ++i) {
    hwc_display_contents_1_t *dc = sf_display_contents[i];
    if (!dc)
      continue;

    size_t num_dc_layers = dc->numHwLayers;
    for (size_t j = 0; j < num_dc_layers; ++j) {
      hwc_layer_1_t *layer = &dc->hwLayers[j];
      if (layer->flags & HWC_SKIP_LAYER)
        continue;
      hwc_add_layer_to_retire_fence(layer, dc);
    }
  }

  composition.reset(NULL);

  return ret;
}

static int hwc_event_control(struct hwc_composer_device_1 *dev, int display,
                             int event, int enabled) {
  if (event != HWC_EVENT_VSYNC || (enabled != 0 && enabled != 1))
    return -EINVAL;

  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  hwc_drm_display_t *hd = &ctx->displays[display];
  return hd->vsync_worker.VSyncControl(enabled);
}

static int hwc_set_power_mode(struct hwc_composer_device_1 *dev, int display,
                              int mode) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;

  uint64_t dpmsValue = 0;
  switch (mode) {
    case HWC_POWER_MODE_OFF:
      dpmsValue = DRM_MODE_DPMS_OFF;
      break;

    /* We can't support dozing right now, so go full on */
    case HWC_POWER_MODE_DOZE:
    case HWC_POWER_MODE_DOZE_SUSPEND:
    case HWC_POWER_MODE_NORMAL:
      dpmsValue = DRM_MODE_DPMS_ON;
      break;
  };
  return ctx->drm.SetDpmsMode(display, dpmsValue);
}

static int hwc_query(struct hwc_composer_device_1 * /* dev */, int what,
                     int *value) {
  switch (what) {
    case HWC_BACKGROUND_LAYER_SUPPORTED:
      *value = 0; /* TODO: We should do this */
      break;
    case HWC_VSYNC_PERIOD:
      ALOGW("Query for deprecated vsync value, returning 60Hz");
      *value = 1000 * 1000 * 1000 / 60;
      break;
    case HWC_DISPLAY_TYPES_SUPPORTED:
      *value = HWC_DISPLAY_PRIMARY_BIT | HWC_DISPLAY_EXTERNAL_BIT |
               HWC_DISPLAY_VIRTUAL_BIT;
      break;
  }
  return 0;
}

static void hwc_register_procs(struct hwc_composer_device_1 *dev,
                               hwc_procs_t const *procs) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;

  ctx->procs = procs;

  for (std::pair<const int, hwc_drm_display> &display_entry : ctx->displays)
    display_entry.second.vsync_worker.SetProcs(procs);

  ctx->hotplug_handler.Init(&ctx->drm, procs);
  ctx->drm.event_listener()->RegisterHotplugHandler(&ctx->hotplug_handler);
}

static int hwc_get_display_configs(struct hwc_composer_device_1 *dev,
                                   int display, uint32_t *configs,
                                   size_t *num_configs) {
  if (!*num_configs)
    return 0;

  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  hwc_drm_display_t *hd = &ctx->displays[display];
  hd->config_ids.clear();

  DrmConnector *connector = ctx->drm.GetConnectorForDisplay(display);
  if (!connector) {
    ALOGE("Failed to get connector for display %d", display);
    return -ENODEV;
  }

  int ret = connector->UpdateModes();
  if (ret) {
    ALOGE("Failed to update display modes %d", ret);
    return ret;
  }

  for (const DrmMode &mode : connector->modes()) {
    size_t idx = hd->config_ids.size();
    if (idx == *num_configs)
      break;
    hd->config_ids.push_back(mode.id());
    configs[idx] = mode.id();
  }
  *num_configs = hd->config_ids.size();
  return *num_configs == 0 ? -1 : 0;
}

static int hwc_get_display_attributes(struct hwc_composer_device_1 *dev,
                                      int display, uint32_t config,
                                      const uint32_t *attributes,
                                      int32_t *values) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  DrmConnector *c = ctx->drm.GetConnectorForDisplay(display);
  if (!c) {
    ALOGE("Failed to get DrmConnector for display %d", display);
    return -ENODEV;
  }
  DrmMode mode;
  for (const DrmMode &conn_mode : c->modes()) {
    if (conn_mode.id() == config) {
      mode = conn_mode;
      break;
    }
  }
  if (mode.id() == 0) {
    ALOGE("Failed to find active mode for display %d", display);
    return -ENOENT;
  }

  uint32_t mm_width = c->mm_width();
  uint32_t mm_height = c->mm_height();
  for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; ++i) {
    switch (attributes[i]) {
      case HWC_DISPLAY_VSYNC_PERIOD:
        values[i] = 1000 * 1000 * 1000 / mode.v_refresh();
        break;
      case HWC_DISPLAY_WIDTH:
        values[i] = mode.h_display();
        break;
      case HWC_DISPLAY_HEIGHT:
        values[i] = mode.v_display();
        break;
      case HWC_DISPLAY_DPI_X:
        /* Dots per 1000 inches */
        values[i] = mm_width ? (mode.h_display() * UM_PER_INCH) / mm_width : 0;
        break;
      case HWC_DISPLAY_DPI_Y:
        /* Dots per 1000 inches */
        values[i] =
            mm_height ? (mode.v_display() * UM_PER_INCH) / mm_height : 0;
        break;
    }
  }
  return 0;
}

static int hwc_get_active_config(struct hwc_composer_device_1 *dev,
                                 int display) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  DrmConnector *c = ctx->drm.GetConnectorForDisplay(display);
  if (!c) {
    ALOGE("Failed to get DrmConnector for display %d", display);
    return -ENODEV;
  }

  DrmMode mode = c->active_mode();
  hwc_drm_display_t *hd = &ctx->displays[display];
  for (size_t i = 0; i < hd->config_ids.size(); ++i) {
    if (hd->config_ids[i] == mode.id())
      return i;
  }
  return -1;
}

static int hwc_set_active_config(struct hwc_composer_device_1 *dev, int display,
                                 int index) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)&dev->common;
  hwc_drm_display_t *hd = &ctx->displays[display];
  if (index >= (int)hd->config_ids.size()) {
    ALOGE("Invalid config index %d passed in", index);
    return -EINVAL;
  }

  DrmConnector *c = ctx->drm.GetConnectorForDisplay(display);
  if (!c) {
    ALOGE("Failed to get connector for display %d", display);
    return -ENODEV;
  }

  if (c->state() != DRM_MODE_CONNECTED)
    return -ENODEV;

  DrmMode mode;
  for (const DrmMode &conn_mode : c->modes()) {
    if (conn_mode.id() == hd->config_ids[index]) {
      mode = conn_mode;
      break;
    }
  }
  if (mode.id() != hd->config_ids[index]) {
    ALOGE("Could not find active mode for %d/%d", index, hd->config_ids[index]);
    return -ENOENT;
  }
  int ret = ctx->drm.SetDisplayActiveMode(display, mode);
  if (ret) {
    ALOGE("Failed to set active config %d", ret);
    return ret;
  }
  ret = ctx->drm.SetDpmsMode(display, DRM_MODE_DPMS_ON);
  if (ret) {
    ALOGE("Failed to set dpms mode on %d", ret);
    return ret;
  }
  return ret;
}

static int hwc_device_close(struct hw_device_t *dev) {
  struct hwc_context_t *ctx = (struct hwc_context_t *)dev;
  delete ctx;
  return 0;
}

/*
 * TODO: This function sets the active config to the first one in the list. This
 * should be fixed such that it selects the preferred mode for the display, or
 * some other, saner, method of choosing the config.
 */
static int hwc_set_initial_config(hwc_drm_display_t *hd) {
  uint32_t config;
  size_t num_configs = 1;
  int ret = hwc_get_display_configs(&hd->ctx->device, hd->display, &config,
                                    &num_configs);
  if (ret || !num_configs)
    return 0;

  ret = hwc_set_active_config(&hd->ctx->device, hd->display, 0);
  if (ret) {
    ALOGE("Failed to set active config d=%d ret=%d", hd->display, ret);
    return ret;
  }

  return ret;
}

static int hwc_initialize_display(struct hwc_context_t *ctx, int display) {
  hwc_drm_display_t *hd = &ctx->displays[display];
  hd->ctx = ctx;
  hd->display = display;

  int ret = hwc_set_initial_config(hd);
  if (ret) {
    ALOGE("Failed to set initial config for d=%d ret=%d", display, ret);
    return ret;
  }

  ret = hd->vsync_worker.Init(&ctx->drm, display);
  if (ret) {
    ALOGE("Failed to create event worker for display %d %d\n", display, ret);
    return ret;
  }

  return 0;
}

static int hwc_enumerate_displays(struct hwc_context_t *ctx) {
  int ret;
  for (auto &conn : ctx->drm.connectors()) {
    ret = hwc_initialize_display(ctx, conn->display());
    if (ret) {
      ALOGE("Failed to initialize display %d", conn->display());
      return ret;
    }
  }

  ret = ctx->virtual_compositor_worker.Init();
  if (ret) {
    ALOGE("Failed to initialize virtual compositor worker");
    return ret;
  }
  return 0;
}

static int hwc_device_open(const struct hw_module_t *module, const char *name,
                           struct hw_device_t **dev) {
  if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
    ALOGE("Invalid module name- %s", name);
    return -EINVAL;
  }

  std::unique_ptr<hwc_context_t> ctx(new hwc_context_t());
  if (!ctx) {
    ALOGE("Failed to allocate hwc context");
    return -ENOMEM;
  }

  int ret = ctx->drm.Init();
  if (ret) {
    ALOGE("Can't initialize Drm object %d", ret);
    return ret;
  }

  ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
                      (const hw_module_t **)&ctx->gralloc);
  if (ret) {
    ALOGE("Failed to open gralloc module %d", ret);
    return ret;
  }

  ret = ctx->dummy_timeline.Init();
  if (ret) {
    ALOGE("Failed to create dummy sw sync timeline %d", ret);
    return ret;
  }

  ctx->importer.reset(Importer::CreateInstance(&ctx->drm));
  if (!ctx->importer) {
    ALOGE("Failed to create importer instance");
    return ret;
  }

  ret = hwc_enumerate_displays(ctx.get());
  if (ret) {
    ALOGE("Failed to enumerate displays: %s", strerror(ret));
    return ret;
  }

  ctx->device.common.tag = HARDWARE_DEVICE_TAG;
  ctx->device.common.version = HWC_DEVICE_API_VERSION_1_4;
  ctx->device.common.module = const_cast<hw_module_t *>(module);
  ctx->device.common.close = hwc_device_close;

  ctx->device.dump = hwc_dump;
  ctx->device.prepare = hwc_prepare;
  ctx->device.set = hwc_set;
  ctx->device.eventControl = hwc_event_control;
  ctx->device.setPowerMode = hwc_set_power_mode;
  ctx->device.query = hwc_query;
  ctx->device.registerProcs = hwc_register_procs;
  ctx->device.getDisplayConfigs = hwc_get_display_configs;
  ctx->device.getDisplayAttributes = hwc_get_display_attributes;
  ctx->device.getActiveConfig = hwc_get_active_config;
  ctx->device.setActiveConfig = hwc_set_active_config;
  ctx->device.setCursorPositionAsync = NULL; /* TODO: Add cursor */

  *dev = &ctx->device.common;
  ctx.release();

  return 0;
}
}

static struct hw_module_methods_t hwc_module_methods = {
  .open = android::hwc_device_open
};

hwc_module_t HAL_MODULE_INFO_SYM = {
  .common = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = HWC_HARDWARE_MODULE_ID,
    .name = "DRM hwcomposer module",
    .author = "The Android Open Source Project",
    .methods = &hwc_module_methods,
    .dso = NULL,
    .reserved = {0},
  }
};
