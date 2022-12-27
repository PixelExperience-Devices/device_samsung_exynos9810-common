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

#ifndef __HARDWARE_EXYNOS_HW2DCOMPOSITOR_G2D_H__
#define __HARDWARE_EXYNOS_HW2DCOMPOSITOR_G2D_H__

#include <uapi/m2m1shot2.h>

#include <hardware/exynos/acryl.h>

#include "acrylic_device.h"

class AcrylicCompositorM2M1SHOT2_G2D: public Acrylic {
public:
    AcrylicCompositorM2M1SHOT2_G2D(const HW2DCapability &capability);
    virtual ~AcrylicCompositorM2M1SHOT2_G2D();
    virtual bool execute(int fence[], unsigned int num_fences);
    virtual bool execute(int *handle = NULL);
    virtual bool waitExecution(int handle);
    /*
     * Return -1 on failure in configuring the give priority or the priority is invalid.
     * Return 0 when the priority is configured successfully without any side effect.
     * Return 1 when the priority is configured successfully but the priority may not
     * be reflected promptly due to other pending tasks with lower priorities.
     */
    virtual int prioritize(int priority = -1);
    virtual bool requestPerformanceQoS(AcrylicPerformanceRequest *request);
protected:
    virtual void removeTransitData(AcrylicLayer *layer);
private:
    bool executeG2D(int fence[], unsigned int num_fences, bool nonblocking);
    bool prepareImage(m2m1shot2_image &image, AcrylicCanvas &layer);
    bool prepareSource(m2m1shot2_image &image, AcrylicLayer &layer,
                       uint32_t target_width, uint32_t target_height);
    AcrylicCompositorM2M1SHOT2_G2D *prescaleSource(
                            AcrylicLayer &layer, hw2d_coord_t target_size);
    int preparePrescaleBuffer(size_t len, bool drm_protected);

    AcrylicRedundantDevice mDev;
    struct m2m1shot2 mDesc;
    unsigned int mMaxSourceCount;
    int mClientION;
    int mPriority;
};

class AcrylicTransitM2M1SHOT2_G2D {
    AcrylicCompositorM2M1SHOT2_G2D *mCompositor;
    AcrylicLayer *mLayer;
    int mBufferFD;
    bool mIsProtected;
    size_t mBufferLen;
public:
    AcrylicTransitM2M1SHOT2_G2D()
        : mCompositor(NULL), mLayer(NULL), mBufferFD(-1),
          mIsProtected(false), mBufferLen(0)
    {
    }

    AcrylicTransitM2M1SHOT2_G2D(AcrylicTransitM2M1SHOT2_G2D &transit)
        : mCompositor(transit.mCompositor), mLayer(transit.mLayer),
          mBufferFD(transit.mBufferFD), mIsProtected(transit.mIsProtected),
          mBufferLen(transit.mBufferLen)
    {
    }

    ~AcrylicTransitM2M1SHOT2_G2D() {
        if (mBufferFD >= 0)
            close(mBufferFD);
        if (mLayer)
            delete mLayer;
        if (mCompositor)
            delete mCompositor;
    }

    void clear() {
            mBufferFD = -1;
            mLayer = NULL;
            mCompositor = NULL;
    }

    AcrylicLayer *getLayer() { return mLayer; }
    AcrylicCompositorM2M1SHOT2_G2D *getCompositor() { return mCompositor; }
    int getDmabuf() { return mBufferFD; }
    size_t getBufferLen() { return mBufferLen; }
    bool isProtected() { return mIsProtected; }

    bool execute() { return mCompositor->execute(NULL, 0); }

    void setLayer(AcrylicLayer *layer) { mLayer = layer; }
    void setCompositor(AcrylicCompositorM2M1SHOT2_G2D *compositor) { mCompositor = compositor; }
    void setBuffer(int fd, size_t len, bool is_protected) {
        if (mBufferFD >= 0)
            close(mBufferFD);
        mBufferFD = fd;
        mBufferLen = len;
        mIsProtected = is_protected;
    }
};

#endif /* __HARDWARE_EXYNOS_HW2DCOMPOSITOR_G2D_H__ */
