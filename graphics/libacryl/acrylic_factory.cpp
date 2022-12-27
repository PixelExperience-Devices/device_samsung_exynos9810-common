/*
 * Copyright Samsung Electronics Co.,LTD.
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>

#include <log/log.h>

#include <exynos_format.h> // hardware/smasung_slsi/exynos/include

#include "acrylic_internal.h"
#include "acrylic_g2d.h"
#include "acrylic_g2d9810.h"
#include "acrylic_mscl9810.h"

static uint32_t all_fimg2d_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_888,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
};

static uint32_t all_fimg2d_hdr_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBA_1010102,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_888,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
};

static uint32_t all_fimg2d_sbwc_lossy_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBA_1010102,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_888,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,      // SBWC Lossy 64 block
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40,  // SBWC Lossy 64 block with 10 bit
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80,  // SBWC Lossy 128 block with 10 bit
};

static uint32_t all_fimg2d_sbwc_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBA_1010102,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_888,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC,
};

static uint32_t all_mscl_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YV12,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M,
    HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
};

static uint32_t all_mscl_sbwc_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGBA_1010102,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YV12,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M,
    HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
};

static uint32_t all_mscl_sbwc_lossy_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGBA_1010102,
    HAL_PIXEL_FORMAT_RGB_565,
    HAL_PIXEL_FORMAT_YV12,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M,
    HAL_PIXEL_FORMAT_EXYNOS_YV12_M,
    HAL_PIXEL_FORMAT_YCrCb_420_SP,                  // NV21 (YVU420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M,         // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL,    // NV21 on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP,           // NV12 (YUV420 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN,          // NV12 with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M,         // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV,    // NV12M with MFC alignment constraints on multi-buffer
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B,     // NV12 10-bit with MFC alignment constraints
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B,    // NV12 10-bit multi-buffer
    HAL_PIXEL_FORMAT_YCBCR_P010,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_P010_M,
    HAL_PIXEL_FORMAT_YCbCr_422_I,                   // YUYV
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_422_I,            // YVYU
    HAL_PIXEL_FORMAT_YCbCr_422_SP,                  // YUV422 2P (YUV422 semi-planar)
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_10B_SBWC,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L50,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L40,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_SBWC_L75,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L60,
    HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_10B_SBWC_L80,
};

static uint32_t rgb_formats[] = {
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_BGRA_8888,
    HAL_PIXEL_FORMAT_RGBX_8888,
    HAL_PIXEL_FORMAT_RGB_888,
    HAL_PIXEL_FORMAT_RGB_565,
};

// The presence of the dataspace definitions are in the order
// of application's preference to reduce comparations.
static int all_hwc_dataspaces[] = {
    HAL_DATASPACE_STANDARD_BT709,
    HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_BT2020,
    HAL_DATASPACE_STANDARD_BT2020 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT2020 | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_BT601_625,
    HAL_DATASPACE_STANDARD_BT601_625 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT601_625 | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_BT601_525,
    HAL_DATASPACE_STANDARD_BT601_525 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT601_525 | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED,
    HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT601_625_UNADJUSTED | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED,
    HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT601_525_UNADJUSTED | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_DCI_P3,
    HAL_DATASPACE_STANDARD_DCI_P3 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_DCI_P3 | HAL_DATASPACE_RANGE_LIMITED,
    HAL_DATASPACE_STANDARD_FILM,
    HAL_DATASPACE_STANDARD_FILM | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_FILM | HAL_DATASPACE_RANGE_LIMITED,
    // 0 should be treated as BT709 Limited range
    0,
    HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_RANGE_LIMITED,
    // Depricated legacy dataspace definitions
    HAL_DATASPACE_SRGB,
    HAL_DATASPACE_JFIF,
    HAL_DATASPACE_BT601_525,
    HAL_DATASPACE_BT601_625,
    HAL_DATASPACE_BT709,
};

static int srgb_dataspaces[] = {
    HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_STANDARD_BT709 | HAL_DATASPACE_RANGE_FULL,
    HAL_DATASPACE_SRGB
};

// NOTE: Initializer list of C99 style is not supported by C++x11.
// G++ supports the C99 style initializer list with a restriction that
// the list should specify all the member variables in the order that
// the variables are defined in the structure.
const static stHW2DCapability __capability_fimg2d_8895 = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {2, 2},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 1,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE
                         | HW2DCapability::FEATURE_AFBC_ENCODE | HW2DCapability::FEATURE_AFBC_DECODE,
    .num_formats = ARRSIZE(all_fimg2d_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 8,
    .pixformats = all_fimg2d_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_9610 = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {31767, 32767},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE
                         | HW2DCapability::FEATURE_AFBC_ENCODE | HW2DCapability::FEATURE_AFBC_DECODE
                         | HW2DCapability::FEATURE_OTF_WRITE,
    .num_formats = ARRSIZE(all_fimg2d_hdr_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 8,
    .pixformats = all_fimg2d_hdr_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_9810 = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {31767, 32767},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE
                         | HW2DCapability::FEATURE_AFBC_ENCODE | HW2DCapability::FEATURE_AFBC_DECODE
                         | HW2DCapability::FEATURE_OTF_WRITE,
    .num_formats = ARRSIZE(all_fimg2d_hdr_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 16,
    .pixformats = all_fimg2d_hdr_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_9810_blter = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {31767, 32767},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE,
    .num_formats = sizeof(rgb_formats) / sizeof(rgb_formats[0]),
    .num_dataspaces = sizeof(srgb_dataspaces) / sizeof(srgb_dataspaces[0]),
    .max_layers = 2,
    .pixformats = rgb_formats,
    .dataspaces = srgb_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_L16FSBWC = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {31767, 32767},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE
                         | HW2DCapability::FEATURE_AFBC_ENCODE | HW2DCapability::FEATURE_AFBC_DECODE
                         | HW2DCapability::FEATURE_OTF_WRITE | HW2DCapability::FEATURE_SOLIDCOLOR,
    .num_formats = ARRSIZE(all_fimg2d_sbwc_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 16,
    .pixformats = all_fimg2d_sbwc_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_L8FSBWCL = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {31767, 32767},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {1, 1},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {1, 1},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_UORDER_WRITE
                         | HW2DCapability::FEATURE_AFBC_ENCODE | HW2DCapability::FEATURE_AFBC_DECODE
                         | HW2DCapability::FEATURE_OTF_WRITE | HW2DCapability::FEATURE_SOLIDCOLOR,
    .num_formats = ARRSIZE(all_fimg2d_sbwc_lossy_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 8,
    .pixformats = all_fimg2d_sbwc_lossy_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_fimg2d_8890 = {
    .max_upsampling_num = {32767, 32767},
    .max_downsampling_factor = {2, 2},
    .max_upsizing_num = {32767, 32767},
    .max_downsizing_factor = {32767, 32767},
    .min_src_dimension = {4, 4},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {4, 4},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 1,
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = HW2DCapability::FEATURE_PLANE_ALPHA | HW2DCapability::FEATURE_AFBC_DECODE,
    .num_formats = sizeof(rgb_formats) / sizeof(rgb_formats[0]),
    .num_dataspaces = sizeof(srgb_dataspaces) / sizeof(srgb_dataspaces[0]),
    .max_layers = 3,
    .pixformats = rgb_formats,
    .dataspaces = srgb_dataspaces,
    .base_align = 1,
};

const static stHW2DCapability __capability_mscl_9810 = {
    .max_upsampling_num = {64, 64},
    .max_downsampling_factor = {16, 16},
    .max_upsizing_num = {64, 64},
    .max_downsizing_factor = {16, 16},
    .min_src_dimension = {16, 16},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {4, 4},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    // MSCL does not perform alpha compositing but it is required to specify all to pass composit mode test
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = 0,
    .num_formats = ARRSIZE(all_mscl_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 1,
    .pixformats = all_mscl_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 4,
};

const static stHW2DCapability __capability_mscl_sbwc = {
    .max_upsampling_num = {64, 64},
    .max_downsampling_factor = {16, 16},
    .max_upsizing_num = {64, 64},
    .max_downsizing_factor = {16, 16},
    .min_src_dimension = {16, 16},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {4, 4},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    // MSCL does not perform alpha compositing but it is required to specify all to pass composit mode test
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = 0,
    .num_formats = ARRSIZE(all_mscl_sbwc_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 1,
    .pixformats = all_mscl_sbwc_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 4,
};

const static stHW2DCapability __capability_mscl_sbwcl = {
    .max_upsampling_num = {64, 64},
    .max_downsampling_factor = {16, 16},
    .max_upsizing_num = {64, 64},
    .max_downsizing_factor = {16, 16},
    .min_src_dimension = {16, 16},
    .max_src_dimension = {8192, 8192},
    .min_dst_dimension = {4, 4},
    .max_dst_dimension = {8192, 8192},
    .min_pix_align = {1, 1},
    .rescaling_count = 0,
    // MSCL does not perform alpha compositing but it is required to specify all to pass composit mode test
    .compositing_mode = HW2DCapability::BLEND_NONE | HW2DCapability::BLEND_SRC_COPY | HW2DCapability::BLEND_SRC_OVER,
    .transform_type = HW2DCapability::TRANSFORM_ALL,
    .auxiliary_feature = 0,
    .num_formats = ARRSIZE(all_mscl_sbwc_lossy_formats),
    .num_dataspaces = ARRSIZE(all_hwc_dataspaces),
    .max_layers = 1,
    .pixformats = all_mscl_sbwc_lossy_formats,
    .dataspaces = all_hwc_dataspaces,
    .base_align = 4,
};

static const HW2DCapability capability_fimg2d_8895(__capability_fimg2d_8895);
static const HW2DCapability capability_fimg2d_8890(__capability_fimg2d_8890);
static const HW2DCapability capability_fimg2d_9610(__capability_fimg2d_9610);
static const HW2DCapability capability_fimg2d_9810(__capability_fimg2d_9810);
static const HW2DCapability capability_fimg2d_L16FSBWC(__capability_fimg2d_L16FSBWC);
static const HW2DCapability capability_fimg2d_L8FSBWCL(__capability_fimg2d_L8FSBWCL);
static const HW2DCapability capability_fimg2d_9810_blter(__capability_fimg2d_9810_blter);
static const HW2DCapability capability_mscl_9810(__capability_mscl_9810);
static const HW2DCapability capability_mscl_sbwc(__capability_mscl_sbwc);
static const HW2DCapability capability_mscl_sbwcl(__capability_mscl_sbwcl);

Acrylic *Acrylic::createInstance(const char *spec)
{
    Acrylic *compositor = NULL;

    ALOGD_TEST("Creating a new Acrylic instance of '%s'", spec);

    if (strcmp(spec, "fimg2d_8890") == 0) {
        compositor = new AcrylicCompositorM2M1SHOT2_G2D(capability_fimg2d_8890);
    } else if (strcmp(spec, "fimg2d_8895") == 0) {
        compositor = new AcrylicCompositorM2M1SHOT2_G2D(capability_fimg2d_8895);
    } else if (strcmp(spec, "fimg2d_9610") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_9610, false);
    } else if (strcmp(spec, "fimg2d_9810") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_9810, false);
    } else if (strcmp(spec, "fimg2d_9810_blter") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_9810_blter, false);
    } else if (strcmp(spec, "fimg2d_9820") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_9810, true);
    } else if (strcmp(spec, "fimg2d_L8FSBWCL") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_L8FSBWCL, true);
    } else if (strcmp(spec, "fimg2d_L16FSBWC") == 0) {
        compositor = new AcrylicCompositorG2D9810(capability_fimg2d_L16FSBWC, true);
    } else if (strcmp(spec, "mscl_9810") == 0) {
        compositor = new AcrylicCompositorMSCL9810(capability_mscl_9810);
    } else if (strcmp(spec, "mscl_sbwc") == 0) {
        compositor = new AcrylicCompositorMSCL9810(capability_mscl_sbwc);
    } else if (strcmp(spec, "mscl_sbwcl") == 0) {
        compositor = new AcrylicCompositorMSCL9810(capability_mscl_sbwcl);
    } else {
        ALOGE("Unknown HW2D compositor spec., %s", spec);
        return NULL;
    }

    ALOGE_IF(!compositor, "Failed to create HW2D compositor of '%s'", spec);

    return compositor;
}

Acrylic *Acrylic::createCompositor()
{
    return Acrylic::createInstance(LIBACRYL_DEFAULT_COMPOSITOR);
}

Acrylic *Acrylic::createScaler()
{
    return Acrylic::createInstance(LIBACRYL_DEFAULT_SCALER);
}

Acrylic *Acrylic::createBlter()
{
    return Acrylic::createInstance(LIBACRYL_DEFAULT_BLTER);
}

Acrylic *AcrylicFactory::createAcrylic(const char *spec)
{
    if (strcmp(spec, "default_compositor") == 0) {
        spec = LIBACRYL_DEFAULT_COMPOSITOR;
    } else if (strcmp(spec, "default_scaler") == 0) {
        spec = LIBACRYL_DEFAULT_SCALER;
    } else if (strcmp(spec, "default_blter") == 0) {
        spec = LIBACRYL_DEFAULT_BLTER;
    }

    return Acrylic::createInstance(spec);
}
