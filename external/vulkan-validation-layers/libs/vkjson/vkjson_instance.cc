///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and/or associated documentation files (the "Materials"), to
// deal in the Materials without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Materials, and to permit persons to whom the Materials are
// furnished to do so, subject to the following conditions:
//
// The above copyright notice(s) and this permission notice shall be included in
// all copies or substantial portions of the Materials.
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
// USE OR OTHER DEALINGS IN THE MATERIALS.
///////////////////////////////////////////////////////////////////////////////

#define VK_PROTOTYPES
#include "vkjson.h"

#include <utility>

namespace {
bool EnumerateExtensions(const char* layer_name,
                         std::vector<VkExtensionProperties>* extensions) {
  VkResult result;
  uint32_t count = 0;
  result = vkEnumerateInstanceExtensionProperties(layer_name, &count, nullptr);
  if (result != VK_SUCCESS)
    return false;
  extensions->resize(count);
  result = vkEnumerateInstanceExtensionProperties(layer_name, &count,
                                                  extensions->data());
  if (result != VK_SUCCESS)
    return false;
  return true;
}

}  // anonymous namespace

VkJsonDevice VkJsonGetDevice(VkPhysicalDevice physical_device) {
  VkJsonDevice device;
  vkGetPhysicalDeviceProperties(physical_device, &device.properties);
  vkGetPhysicalDeviceFeatures(physical_device, &device.features);
  vkGetPhysicalDeviceMemoryProperties(physical_device, &device.memory);

  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);
  if (queue_family_count > 0) {
    device.queues.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(
        physical_device, &queue_family_count, device.queues.data());
  }

  // Only device extensions.
  // TODO(piman): do we want to show layer extensions?
  uint32_t extension_count = 0;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                       &extension_count, nullptr);
  if (extension_count > 0) {
    device.extensions.resize(extension_count);
    vkEnumerateDeviceExtensionProperties(
        physical_device, nullptr, &extension_count, device.extensions.data());
  }

  uint32_t layer_count = 0;
  vkEnumerateDeviceLayerProperties(physical_device, &layer_count, nullptr);
  if (layer_count > 0) {
    device.layers.resize(layer_count);
    vkEnumerateDeviceLayerProperties(physical_device, &layer_count,
                                     device.layers.data());
  }

  VkFormatProperties format_properties = {};
  for (VkFormat format = VK_FORMAT_R4G4_UNORM_PACK8;
       format <= VK_FORMAT_END_RANGE;
       format = static_cast<VkFormat>(format + 1)) {
    vkGetPhysicalDeviceFormatProperties(physical_device, format,
                                        &format_properties);
    if (format_properties.linearTilingFeatures ||
        format_properties.optimalTilingFeatures ||
        format_properties.bufferFeatures) {
      device.formats.insert(std::make_pair(format, format_properties));
    }
  }
  return device;
}

VkJsonInstance VkJsonGetInstance() {
  VkJsonInstance instance;
  VkResult result;
  uint32_t count;

  count = 0;
  result = vkEnumerateInstanceLayerProperties(&count, nullptr);
  if (result != VK_SUCCESS)
    return VkJsonInstance();
  if (count > 0) {
    std::vector<VkLayerProperties> layers(count);
    result = vkEnumerateInstanceLayerProperties(&count, layers.data());
    if (result != VK_SUCCESS)
      return VkJsonInstance();
    instance.layers.reserve(count);
    for (auto& layer : layers) {
      instance.layers.push_back(VkJsonLayer{layer, std::vector<VkExtensionProperties>()});
      if (!EnumerateExtensions(layer.layerName,
                               &instance.layers.back().extensions))
        return VkJsonInstance();
    }
  }

  if (!EnumerateExtensions(nullptr, &instance.extensions))
    return VkJsonInstance();

  std::vector<const char*> layer_names;
  layer_names.reserve(instance.layers.size());
  for (auto& layer : instance.layers)
    layer_names.push_back(layer.properties.layerName);

  const VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                      nullptr,
                                      "vkjson_info",
                                      1,
                                      "",
                                      0,
                                      VK_API_VERSION_1_0};
  VkInstanceCreateInfo instance_info = {
      VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      nullptr,
      0,
      &app_info,
      static_cast<uint32_t>(layer_names.size()),
      layer_names.data(),
      0,
      nullptr};
  VkInstance vkinstance;
  result = vkCreateInstance(&instance_info, nullptr, &vkinstance);
  if (result != VK_SUCCESS)
    return VkJsonInstance();

  count = 0;
  result = vkEnumeratePhysicalDevices(vkinstance, &count, nullptr);
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }
  std::vector<VkPhysicalDevice> devices(count, VK_NULL_HANDLE);
  result = vkEnumeratePhysicalDevices(vkinstance, &count, devices.data());
  if (result != VK_SUCCESS) {
    vkDestroyInstance(vkinstance, nullptr);
    return VkJsonInstance();
  }

  instance.devices.reserve(devices.size());
  for (auto device : devices)
    instance.devices.emplace_back(VkJsonGetDevice(device));

  vkDestroyInstance(vkinstance, nullptr);
  return instance;
}
