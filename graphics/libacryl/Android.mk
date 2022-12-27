# Copyright (C) 2016 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DLOG_TAG=\"libacryl\"

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libion_exynos libacryl_plugin_exynos9810_hdr10

LOCAL_HEADER_LIBRARIES += libacryl_hdrplugin_headers
LOCAL_HEADER_LIBRARIES += libexynos_headers

LOCAL_C_INCLUDES := $(LOCAL_PATH)/local_include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
    acrylic.cpp \
    acrylic_dummy.cpp \
    acrylic_g2d.cpp \
    acrylic_mscl9810.cpp \
    acrylic_g2d9810.cpp \
    acrylic_factory.cpp \
    acrylic_layer.cpp \
    acrylic_formats.cpp \
    acrylic_performance.cpp \
    acrylic_device.cpp

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libacryl_exynos9810
LOCAL_PROPRIETARY_MODULE := true

include $(BUILD_SHARED_LIBRARY)
