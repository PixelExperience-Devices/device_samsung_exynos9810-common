#
# Copyright (C) 2021 The LineageOS Project
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

COMMON_PATH := device/samsung/exynos9810-common

BUILD_BROKEN_DUP_RULES := true

## Include path
TARGET_SPECIFIC_HEADER_PATH := $(COMMON_PATH)/include

## Inherit proprietary vendor configuration
include vendor/samsung/exynos9810-common/BoardConfigVendor.mk

# APEX image
DEXPREOPT_GENERATE_APEX_IMAGE := true
OVERRIDE_TARGET_FLATTEN_APEX := true

## Architecture
TARGET_ARCH := arm64
TARGET_ARCH_VARIANT := armv8-a
TARGET_CPU_ABI := arm64-v8a
TARGET_CPU_ABI2 :=
TARGET_CPU_VARIANT := cortex-a53

## Architecture (secondary)
TARGET_2ND_ARCH := arm
TARGET_2ND_ARCH_VARIANT := armv8-a
TARGET_2ND_CPU_ABI := armeabi-v7a
TARGET_2ND_CPU_ABI2 := armeabi
TARGET_2ND_CPU_VARIANT := cortex-a53

# Boot Image
BOARD_BOOT_HEADER_VERSION := 1
BOARD_CUSTOM_BOOTIMG := true
BOARD_CUSTOM_BOOTIMG_MK := $(COMMON_PATH)/mkbootimg/mkbootimg.mk
BOARD_KERNEL_BASE := 0x10000000
BOARD_KERNEL_OFFSET := 0x00008000
BOARD_KERNEL_PAGESIZE := 2048
BOARD_KERNEL_RAMDISK_OFFSET := 0x01000000
BOARD_KERNEL_TAGS_OFFSET     := 0x00000100

BOARD_MKBOOTIMG_ARGS += --ramdisk_offset $(BOARD_KERNEL_RAMDISK_OFFSET)
BOARD_MKBOOTIMG_ARGS += --tags_offset $(BOARD_KERNEL_TAGS_OFFSET)
BOARD_MKBOOTIMG_ARGS := --kernel_offset $(BOARD_KERNEL_OFFSET)

# Dexpreopt
ifeq ($(HOST_OS),linux)
  ifneq ($(TARGET_BUILD_VARIANT),eng)
    WITH_DEXPREOPT_BOOT_IMG_AND_SYSTEM_SERVER_ONLY ?= false
    WITH_DEXPREOPT := true
  endif
endif

## Exclude AudioFX
TARGET_EXCLUDES_AUDIOFX := true

## Filesystem
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_VENDORIMAGE_FILE_SYSTEM_TYPE := erofs
BOARD_SYSTEMIMAGE_FILE_SYSTEM_TYPE := erofs
TARGET_COPY_OUT_VENDOR := vendor
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_COPY_OUT_PRODUCT := system/product

## Fingerprint
TARGET_SEC_FP_HAS_FINGERPRINT_GESTURES := true

# Graphics
USE_OPENGL_RENDERER := true

# HWUI
HWUI_COMPILE_FOR_PERF := true

# Kernel
BOARD_KERNEL_IMAGE_NAME := Image
BOARD_KERNEL_SEPARATED_DT := true
TARGET_CUSTOM_DTBTOOL := dtbhtoolExynos
TARGET_KERNEL_CLANG_COMPILE := true
TARGET_KERNEL_SOURCE := kernel/samsung/exynos9810

# Manifest
DEVICE_MANIFEST_FILE += $(COMMON_PATH)/configs/vintf/manifest.xml
DEVICE_MATRIX_FILE := $(COMMON_PATH)/configs/vintf/compatibility_matrix.xml

# NGA
TARGET_SUPPORTS_NEXT_GEN_ASSISTANT := true

# Partitions
BOARD_BOOTIMAGE_PARTITION_SIZE := 57671680
BOARD_CACHEIMAGE_PARTITION_SIZE := 209715200
BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 68149248
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 4561305600
BOARD_VENDORIMAGE_PARTITION_SIZE := 681574400

BOARD_ROOT_EXTRA_FOLDERS := efs

## Platform
BOARD_VENDOR := samsung
TARGET_BOARD_PLATFORM := universal9810
TARGET_SLSI_VARIANT := bsp
TARGET_SOC := exynos9810
TARGET_BOOTLOADER_BOARD_NAME := exynos9810

## Properties
TARGET_SYSTEM_PROP += $(COMMON_PATH)/configs/props/system.prop
TARGET_PRODUCT_PROP += $(COMMON_PATH)/configs/props/product.prop
TARGET_VENDOR_PROP += $(COMMON_PATH)/configs/props/vendor.prop

# Quick Tap
TARGET_SUPPORTS_QUICK_TAP := false

# Recovery
TARGET_USES_AOSP_RECOVERY := true
BOARD_HAS_DOWNLOAD_MODE := true
TARGET_RECOVERY_PIXEL_FORMAT := "ABGR_8888"
TARGET_RECOVERY_FSTAB := $(COMMON_PATH)/rootdir/etc/fstab.samsungexynos9810

## Releasetools
TARGET_RELEASETOOLS_EXTENSIONS := $(COMMON_PATH)/releasetools

# RIL
ENABLE_VENDOR_RIL_SERVICE := true

# Sepolicy
BOARD_SEPOLICY_TEE_FLAVOR := mobicore
include device/samsung_slsi/sepolicy/sepolicy.mk

BOARD_VENDOR_SEPOLICY_DIRS += $(COMMON_PATH)/sepolicy/vendor

## VNDK
BOARD_VNDK_VERSION := current

## Wi-Fi
BOARD_WLAN_DEVICE                := bcmdhd
BOARD_WPA_SUPPLICANT_DRIVER      := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_DRIVER             := NL80211
BOARD_HOSTAPD_PRIVATE_LIB        := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
WIFI_AVOID_IFACE_RESET_MAC_CHANGE := true
WIFI_HIDL_FEATURE_DUAL_INTERFACE := true
WPA_SUPPLICANT_VERSION           := VER_0_8_X
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
WIFI_DRIVER_FW_PATH_PARAM        := "/sys/module/dhd/parameters/firmware_path"
