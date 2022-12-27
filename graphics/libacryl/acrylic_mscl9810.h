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

#ifndef __HARDWARE_EXYNOS_HW2DCOMPOSITOR_MSCL8895_LEGACY_H__
#define __HARDWARE_EXYNOS_HW2DCOMPOSITOR_MSCL8895_LEGACY_H__

#include <linux/videodev2.h>
#include <hardware/exynos/acryl.h>

#include "acrylic_device.h"

class AcrylicCompositorMSCL9810: public Acrylic {
    friend class AcrylicFactory;
public:
    enum BUFDIRECTION { SOURCE = 0, TARGET = 1, NUM_IMAGES = 2 };

    AcrylicCompositorMSCL9810(const HW2DCapability &capability);
    virtual ~AcrylicCompositorMSCL9810();
    virtual bool execute(int fence[], unsigned int num_fences);
    virtual bool execute(int *handle = NULL);
    virtual bool waitExecution(int handle);
    virtual bool requestPerformanceQoS(AcrylicPerformanceRequest *request);
private:
    enum { STATE_REQBUFS = 1, STATE_QBUF = 2, STATE_PROCESSING = STATE_REQBUFS | STATE_QBUF };

    bool resetMode(AcrylicCanvas &canvas, BUFDIRECTION dir);
    bool resetMode();
    bool changeMode(AcrylicCanvas &canvas, BUFDIRECTION dir);
    bool setFormat(AcrylicCanvas &canvas, v4l2_buf_type buftype);
    bool setTransform();
    bool setCrop(hw2d_rect_t rect, v4l2_buf_type buftype, hw2d_rect_t &save_rect);
    bool prepareExecute();
    bool prepareExecute(AcrylicCanvas &canvas, BUFDIRECTION dir);
    bool configureCSC();
    bool queueBuffer(AcrylicCanvas &canvas, v4l2_buf_type buftype, int *fence, bool needReleaseFence);
    bool queueBuffer(int fence[], unsigned int num_fences);
    bool dequeueBuffer();
    bool dequeueBuffer(BUFDIRECTION dir, v4l2_buffer *buffer);

    void setDeviceState(BUFDIRECTION dir, int state) { mDeviceState[dir] |= state; }
    void clearDeviceState(BUFDIRECTION dir, int state) { mDeviceState[dir] &= ~state; }
    bool testDeviceState(BUFDIRECTION dir, int state) { return (mDeviceState[dir] & state) == state; }

    AcrylicDevice   mDev;
    uint32_t        mCurrentTransform;
    bool            mProtectedContent;
    bool            mTransformChanged;
    uint32_t        mCurrentPixFmt[NUM_IMAGES];
    hw2d_rect_t     mCurrentCrop[NUM_IMAGES];
    v4l2_buf_type   mCurrentTypeBuf[NUM_IMAGES];
    int             mCurrentTypeMem[NUM_IMAGES]; // AcrylicCanvas::memory_type
    int             mDeviceState[NUM_IMAGES];
    uint32_t        mUseFenceFlag;
};
#endif //__HARDWARE_EXYNOS_HW2DCOMPOSITOR_MSCL8895_LEGACY_H__
