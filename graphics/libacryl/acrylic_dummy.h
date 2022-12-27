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

#ifndef __HARDWARE_EXYNOS_HW2DCOMPOSITOR_DUMMY_H__
#define __HARDWARE_EXYNOS_HW2DCOMPOSITOR_DUMMY_H__

#include <hardware/exynos/acryl.h>

class AcrylicCompositorDummy: public Acrylic {
public:
    AcrylicCompositorDummy(const HW2DCapability &capability);
    virtual ~AcrylicCompositorDummy();
    virtual bool execute(int fence[], unsigned int num_fences);
    virtual bool execute(int *handle = NULL);
    virtual bool waitExecution(int handle);

    AcrylicLayer *getLayerForTest(unsigned int index) { return getLayer(index); }
};

#endif /* __HARDWARE_EXYNOS_HW2DCOMPOSITOR_DUMMY_H__ */
