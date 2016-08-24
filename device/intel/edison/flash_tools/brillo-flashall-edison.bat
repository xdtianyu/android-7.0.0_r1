@ECHO OFF
:: Copyright 2012 The Android Open Source Project
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

PATH=%PATH%;"%SYSTEMROOT%\System32"

:: Only execute this script on a Brillo provisioned Edison.
:: See your Brillo-Edison online information for initial provisioning and recovery.

fastboot flash gpt      gpt.bin
fastboot flash u-boot   u-boot-edison.bin
fastboot flash boot_a   boot.img
fastboot flash system_a system.img
fastboot flash boot_b   boot.img
fastboot flash system_b system.img
fastboot flash userdata userdata.img
fastboot oem set_active 0
