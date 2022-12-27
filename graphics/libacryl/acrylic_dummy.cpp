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

#include <log/log.h>

#include <exynos_format.h> // hardware/smasung_slsi/exynos/include

#include "acrylic_internal.h"
#include "acrylic_dummy.h"


AcrylicCompositorDummy::AcrylicCompositorDummy(const HW2DCapability &capability)
    : Acrylic(capability)
{
    ALOGD("dummy compositor created!");
}

AcrylicCompositorDummy::~AcrylicCompositorDummy()
{
    ALOGD("dummy compositor deleted!");
}

bool AcrylicCompositorDummy::execute(int fence[], unsigned int num_fences)
{
    if (!execute(NULL))
        return false;

    for (unsigned int i = 0; i < num_fences; i++)
        fence[i] = i + 1;

    getCanvas().clearSettingModified();
    for (unsigned int i = 0; i < layerCount(); i++)
        getLayer(i)->clearSettingModified();

    return true;
}

bool AcrylicCompositorDummy::execute(int *handle)
{
    ALOGD("dummy compositor -> execute()");

    if (!validateAllLayers())
        return false;

    sortLayers();

    if (handle)
        *handle = 0;

    unsigned index = 0;
    AcrylicLayer *layer;

    while ((layer = getLayer(index++)))
        layer->setFence(-1);

    getCanvas().setFence(-1);

    getCanvas().clearSettingModified();
    for (unsigned int i = 0; i < layerCount(); i++)
        getLayer(i)->clearSettingModified();

    return true;
}

bool AcrylicCompositorDummy::waitExecution(int __unused handle)
{
    LOG_FATAL(handle == 0);

    return true;
}
