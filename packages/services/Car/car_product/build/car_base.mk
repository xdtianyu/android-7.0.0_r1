#
# Copyright (C) 2016 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Base platform for car builds
# car packages should be added to car.mk instead of here

PRODUCT_PACKAGE_OVERLAYS := packages/services/Car/car_product/overlay

PRODUCT_PACKAGES += \
    ContactsProvider \
    DefaultContainerService \
    Home \
    BasicDreams \
    CaptivePortalLogin \
    CertInstaller \
    DeskClock \
    DocumentsUI \
    DownloadProviderUi \
    FusedLocation \
    InputDevices \
    KeyChain \
    Keyguard \
    LatinIME \
    Launcher2 \
    ManagedProvisioning \
    PicoTts \
    PacProcessor \
    libpac \
    PrintSpooler \
    ProxyHandler \
    Settings \
    SharedStorageBackup \
    VpnDialogs \
    MmsService \
    ExternalStorageProvider \
    atrace \
    libandroidfw \
    libaudiopreprocessing \
    libaudioutils \
    libfilterpack_imageproc \
    libgabi++ \
    libmdnssd \
    libnfc_ndef \
    libpowermanager \
    libspeexresampler \
    libstagefright_soft_aacdec \
    libstagefright_soft_aacenc \
    libstagefright_soft_amrdec \
    libstagefright_soft_amrnbenc \
    libstagefright_soft_amrwbenc \
    libstagefright_soft_avcdec \
    libstagefright_soft_avcenc \
    libstagefright_soft_flacenc \
    libstagefright_soft_g711dec \
    libstagefright_soft_gsmdec \
    libstagefright_soft_hevcdec \
    libstagefright_soft_mp3dec \
    libstagefright_soft_mpeg2dec \
    libstagefright_soft_mpeg4dec \
    libstagefright_soft_mpeg4enc \
    libstagefright_soft_opusdec \
    libstagefright_soft_rawdec \
    libstagefright_soft_vorbisdec \
    libstagefright_soft_vpxdec \
    libstagefright_soft_vpxenc \
    libvariablespeed \
    libwebrtc_audio_preprocessing \
    mdnsd \
    requestsync \
    wifi-service \
    A2dpSinkService

$(call inherit-product, $(SRC_TARGET_DIR)/product/core_minimal.mk)

