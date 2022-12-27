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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <log/log.h>
#include <hardware/exynos/ion.h>

#include <hardware/hwcomposer2.h>

#include <exynos_format.h> // hardware/smasung_slsi/exynos/include

#include "acrylic_g2d.h"
#include "acrylic_internal.h"

#ifndef V4L2_PIX_FMT_NV12M_S10B
#define V4L2_PIX_FMT_NV12M_S10B         v4l2_fourcc('B', 'M', '1', '2')
#endif

static void debug_show_m2m1shot2_image(const char __unused *title, int __unused idx,
                                       const struct m2m1shot2_image __unused &img)
{
    ALOGD_TEST("%s%d: flags %#x, fence %d, memory %d, plane count %d", title, idx,
               img.flags, img.fence, img.memory, img.num_planes);
    ALOGD_TEST("         rsvd[0] %#x, rsvd[1] %#x, rsvd[2] %#x",
               img.reserved[0], img.reserved[1], img.reserved[2]);
    ALOGD_TEST("         %dx%d of format %#x, %ux%u@%dx%d -> %ux%u@%dx%d",
               img.fmt.width, img.fmt.height, img.fmt.pixelformat,
               img.fmt.crop.width, img.fmt.crop.height, img.fmt.crop.left, img.fmt.crop.top,
               img.fmt.window.width, img.fmt.window.height, img.fmt.window.left, img.fmt.window.top);
    ALOGD_TEST("         scale factor %dx%d, fillcolor %#x, transform %d, composit mode %d",
               img.ext.horizontal_factor, img.ext.vertical_factor, img.ext.fillcolor,
               img.ext.transform, img.ext.composit_mode);
    ALOGD_TEST("         alpha %d, color alpha(r%d, g%d, b%d), xrepeat %d, yrepeat %d, filter %d",
               img.ext.galpha, img.ext.galpha_red, img.ext.galpha_green, img.ext.galpha_blue,
               img.ext.xrepeat, img.ext.yrepeat, img.ext.scaler_filter);
    for (int i = 0; i < img.num_planes; i++) {
        ALOGD_TEST("         buf[%d] ptr %#lx, fd %d, offset %u, length %u, payload %u, rsvd %#lx",
                   i, img.plane[i].userptr, img.plane[i].fd, img.plane[i].offset,
                   img.plane[i].length, img.plane[i].payload, img.plane[i].reserved);
    }
}

static void debug_show_m2m1shot2(const struct m2m1shot2 __unused &desc)
{
    ALOGD_TEST("Showing the content of m2m1shot2 descriptor");
    ALOGD_TEST("source count %d, flags %#x, rsvd1 %#x, rsvd2 %#x, rsvd4 %#lx",
               desc.num_sources, desc.flags, desc.reserved1, desc.reserved2, desc.reserved4);
    debug_show_m2m1shot2_image("Target", 0, desc.target);
    for (unsigned int i = 0; i < desc.num_sources; i++)
        debug_show_m2m1shot2_image("Source", i, desc.sources[i]);
}

AcrylicCompositorM2M1SHOT2_G2D::AcrylicCompositorM2M1SHOT2_G2D(const HW2DCapability &capability)
    : Acrylic(capability), mDev("/dev/fimg2d"), mMaxSourceCount(0), mClientION(-1), mPriority(-1)
{
    memset(&mDesc, 0, sizeof(mDesc));

    ALOGD_TEST("Created a new Acrylic for G2D by m2m1shot2 on %p", this);
}

AcrylicCompositorM2M1SHOT2_G2D::~AcrylicCompositorM2M1SHOT2_G2D()
{
    delete [] mDesc.sources;

    if (mClientION >= 0)
        close(mClientION);

    ALOGD_TEST("Deleting Acrylic for G2D by m2m1shot2 on %p", this);
}

int AcrylicCompositorM2M1SHOT2_G2D::preparePrescaleBuffer(size_t len, bool drm_protected)
{
    if (mClientION < 0) {
        mClientION = open("/dev/ion", O_RDWR);
        if (mClientION < 0) {
            ALOGERR("Failed to open /dev/ion");
            return -1;
        }
    }

    unsigned int heapmask = drm_protected ? EXYNOS_ION_HEAP_VIDEO_SCALER_MASK : EXYNOS_ION_HEAP_SYSTEM_MASK;
    unsigned int flags = drm_protected ? ION_FLAG_PROTECTED : ION_FLAG_NOZEROED;
    int buf_fd;

    buf_fd = exynos_ion_alloc(mClientION, len, heapmask, flags);
    if (buf_fd < 0) {
        ALOGERR("Failed to allocate %zu bytes from ION heap mask %#x with flag %#x",
                len, heapmask, flags);
        // no need to close mClientION because it is closed on the destruction of this instance
        return -1;
    }

    return buf_fd;
}

bool AcrylicCompositorM2M1SHOT2_G2D::prepareImage(m2m1shot2_image &image, AcrylicCanvas &layer)
{
    memset(&image, 0, sizeof(image));

    hw2d_coord_t xy;

    if (layer.getFence() >= 0) {
        image.flags |= M2M1SHOT2_IMGFLAG_ACQUIRE_FENCE;
        image.fence = layer.getFence();
    }

    if (layer.isCompressed())
        image.flags |= M2M1SHOT2_IMGFLAG_COMPRESSED;

    if (layer.isProtected())
        image.flags |= M2M1SHOT2_IMGFLAG_SECURE;

    if (layer.isUOrder())
        image.flags |= M2M1SHOT2_IMGFLAG_UORDER_ADDR;

    xy = layer.getImageDimension();
    image.fmt.width = xy.hori;
    image.fmt.height = xy.vert;

    image.fmt.crop.width = xy.hori;
    image.fmt.crop.height = xy.vert;

    image.fmt.window.width = xy.hori;
    image.fmt.window.height = xy.vert;

    image.fmt.pixelformat = halfmt_to_v4l2(layer.getFormat());
    if (image.fmt.pixelformat == V4L2_PIX_FMT_NV12M_S10B)
        image.fmt.pixelformat = V4L2_PIX_FMT_NV12M;

    LOGASSERT(image.fmt.pixelformat != 0, "unknown HAL format %#x", layer.getFormat());

    image.num_planes = halfmt_plane_count(layer.getFormat());
    if (layer.getBufferCount() < image.num_planes) {
        ALOGE("HAL format %#x requires %u buffers but %u buffers are configured",
              layer.getFormat(), image.num_planes, layer.getBufferCount());
        return false;
    }

    if (layer.getBufferType() == AcrylicCanvas::MT_DMABUF) {
        image.memory = M2M1SHOT2_BUFTYPE_DMABUF;
        for (unsigned int i = 0; i < layer.getBufferCount(); i++) {
            image.plane[i].fd = layer.getDmabuf(i);
            image.plane[i].offset = layer.getOffset(i);
            image.plane[i].length = layer.getBufferLength(i);
        }
    } else {
        LOGASSERT(layer.getBufferType() == AcrylicCanvas::MT_USERPTR,
                  "Unknown buffer type %d", layer.getBufferType());
        image.memory = M2M1SHOT2_BUFTYPE_USERPTR;
        for (unsigned int i = 0; i < layer.getBufferCount(); i++) {
            image.plane[i].userptr = reinterpret_cast<unsigned long>(layer.getUserptr(i));
            image.plane[i].length = layer.getBufferLength(i);
        }
    }

    image.colorspace = haldataspace_to_v4l2(layer.getDataspace(), xy.hori, xy.vert);

    return true;
}

// @opr: dividend
// @div: divider
// @align: alignment requirement which is power of 2
template<typename T>
static inline T div_align(T opr, T div, T align)
{
    opr = (opr + div - 1) / div;
    return (opr + align - 1) & ~(align - 1);
}

AcrylicCompositorM2M1SHOT2_G2D *AcrylicCompositorM2M1SHOT2_G2D::prescaleSource(
                                        AcrylicLayer &layer, hw2d_coord_t target_size)
{
    HW2DCapability cap = getCapabilities();
    bool upscaling = false;
    int16_t hfactor, vfactor;
    uint32_t fmt;
    int dataspace;

    // CSC is performed during prescaling due to the consideration
    // about chroma subsampling restriction. H/W tends to require
    // the number of horizontal and vertical pixels to be aligned by
    // the horizontal and vertical chroma subsampling factor, respectively.
    fmt = layer.getFormat();
    dataspace = layer.getDataspace();
    hfactor = halfmt_chroma_subsampling(fmt);
    vfactor = halfmt_chroma_subsampling(getCanvas().getFormat());
    if (hfactor > vfactor) {
        hfactor = vfactor;
        fmt = getCanvas().getFormat();
        dataspace = getCanvas().getDataspace();
    }
    vfactor = hfactor & 0xF;
    hfactor = (hfactor >> 4) & 0xF;

    hw2d_coord_t transit_image_size;
    hw2d_coord_t image_size = layer.getImageRect().size;
    // swapping between hfactor and vfactor is not required
    // because they are only involved in downscaling
    if (!!(layer.getTransform() & HAL_TRANSFORM_ROT_90))
        target_size.swap();

    hw2d_coord_t max_xy = cap.supportedMaxMagnification();
    hw2d_coord_t min_xy = cap.supportedMinMinification();

    if ((image_size.hori * max_xy.hori) < target_size.hori) {
        upscaling = true;
        transit_image_size.hori = image_size.hori * max_xy.hori;
    } else if (image_size.hori > (target_size.hori * min_xy.hori)) {
        transit_image_size.hori = div_align(image_size.hori, min_xy.hori, hfactor);
    } else {
        transit_image_size.hori = target_size.hori;
    }

    if ((image_size.vert * max_xy.vert) < target_size.vert) {
        upscaling = true;
        transit_image_size.vert = image_size.vert * max_xy.vert;
    } else if (image_size.vert > (target_size.vert * min_xy.vert)) {
        transit_image_size.vert = div_align(image_size.vert, min_xy.vert, vfactor);
    } else {
        transit_image_size.vert = target_size.vert;
    }

    if (upscaling && !!(layer.getTransform() & HAL_TRANSFORM_ROT_90))
        transit_image_size.swap();

    // NOTE: No AFBC is applied to the transit image due to the simplicity
    // The transit image is likely a continous-tone image because it is required
    // to resampling video frames AFBC does not show a good performance to that
    // sort of images.
    fmt = find_format_equivalent(fmt);
    size_t len = halfmt_plane_length(fmt, 0, transit_image_size.hori, transit_image_size.vert);

    AcrylicCompositorM2M1SHOT2_G2D *prescaler;
    AcrylicLayer *transit_layer;
    int transit_buffer_fd = -1;

    AcrylicTransitM2M1SHOT2_G2D *transit = reinterpret_cast<AcrylicTransitM2M1SHOT2_G2D *>(layer.getTransit());
    if (!transit) {
        transit = new AcrylicTransitM2M1SHOT2_G2D();
        if (!transit) {
            ALOGE("Failed to allocate transit data for rescaling");
            return NULL;
        }

        prescaler = new AcrylicCompositorM2M1SHOT2_G2D(getCapabilities());
        if (!prescaler) {
            ALOGE("Failed to create prescaler instance");
            delete transit;
            return NULL;
        }

        transit->setCompositor(prescaler);

        transit_layer = prescaler->createLayer();
        if (!transit_layer) {
            ALOGE("Failed to create prescaler source layer instance");
            delete transit;
            return NULL;
        }

        transit->setLayer(transit_layer);

        layer.storeTransit(transit);

        if (prescaler->prioritize(mPriority) < 0) {
            ALOGE("Failed to configure priority %d to the new prescaler", mPriority);
            return NULL;
        }

    } else {
        prescaler = transit->getCompositor();
        transit_layer = transit->getLayer();
        // free the existing buffer if it is too small or has different property
        if (transit->getDmabuf() >= 0) {
            if ((transit->getBufferLen() < len) || (transit->isProtected() != layer.isProtected())) {
                transit->setBuffer(-1, 0, false); // free the exiting buffer
            } else {
                transit_buffer_fd = transit->getDmabuf();
            }
        }
    }

    if (transit->getDmabuf() < 0) {
        transit_buffer_fd = preparePrescaleBuffer(len, layer.isProtected());
        if (transit_buffer_fd < 0) {
            ALOGE("Failed to allocate transit buffer of %dx%d (fmt: %#x)",
                  target_size.hori, target_size.vert, fmt);
            return NULL;
        }

        transit->setBuffer(transit_buffer_fd, len, layer.isProtected());
    }

    if (!prescaler->setCanvasDimension(transit_image_size.hori, transit_image_size.vert))
        return NULL;

    if (!prescaler->setCanvasBuffer(&transit_buffer_fd, &len, 1, -1,
                layer.isProtected() ? AcrylicCanvas::ATTR_PROTECTED : AcrylicCanvas::ATTR_NONE))
        return NULL;

    if (!prescaler->setCanvasImageType(fmt, dataspace))
        return NULL;

    transit_layer->importLayer(layer, upscaling);

    // non blocking execution
    if (!transit->execute()) {
        ALOGE("Failed prescaling from %dx%d to %dx%d",
              image_size.hori, image_size.vert, transit_image_size.hori, transit_image_size.vert);
        return NULL;
    }

    ALOGD("Executed prescaling from %dx%d to %dx%d",
          image_size.hori, image_size.vert, transit_image_size.hori, transit_image_size.vert);

    return prescaler;
}

bool AcrylicCompositorM2M1SHOT2_G2D::prepareSource(m2m1shot2_image &image, AcrylicLayer &layer,
                                      uint32_t target_width, uint32_t target_height)
{
    // Discover the target image size first than the other tasks because we need
    // to determine if the prescaling is required. Note that the target rect
    // configuratio nto @image is not changed even though the prescaling is involved.
    hw2d_rect_t target_rect = layer.getTargetRect();
    if (area_is_zero(target_rect)) {
        target_rect.size.hori = target_width;
        target_rect.size.vert = target_height;
    }

    uint32_t trf = layer.getTransform();

    if (!(layer.getCompositAttr() & AcrylicLayer::ATTR_NORESAMPLING) &&
            !getCapabilities().supportedHWResampling(
                    layer.getImageRect().size, target_rect.size, layer.getTransform())) {
        Acrylic *prescaler = prescaleSource(layer, target_rect.size);
        if (!prescaler)
            return false;

        if (!prepareImage(image, prescaler->getCanvas()))
            return false;

        // if geometric transform is performed during prescaling, skip transform here.
        AcrylicTransitM2M1SHOT2_G2D *transit = reinterpret_cast<AcrylicTransitM2M1SHOT2_G2D *>(layer.getTransit());
        if (transit->getLayer()->getTransform() != 0)
            trf = 0;
    } else {
        if (!prepareImage(image, layer))
            return false;

        hw2d_rect_t image_rect = layer.getImageRect();

        image.fmt.crop.left = image_rect.pos.hori;
        image.fmt.crop.top = image_rect.pos.vert;
        image.fmt.crop.width = image_rect.size.hori;
        image.fmt.crop.height = image_rect.size.vert;

        for (unsigned int i = 0; i < image.num_planes; i++)
            image.plane[i].payload = halfmt_plane_length(layer.getFormat(), i,
                    image.fmt.width,
                    image.fmt.crop.height + image.fmt.crop.top);
    }

    if (trf == HAL_TRANSFORM_ROT_270) {
        image.ext.transform = M2M1SHOT2_IMGTRFORM_ROT270;
    } else if (trf == HAL_TRANSFORM_ROT_180) {
        image.ext.transform = M2M1SHOT2_IMGTRFORM_ROT180;
    } else {
        if (!!(trf & HAL_TRANSFORM_FLIP_H))
            image.ext.transform |= M2M1SHOT2_IMGTRFORM_YFLIP;
        if (!!(trf & HAL_TRANSFORM_FLIP_V))
            image.ext.transform |= M2M1SHOT2_IMGTRFORM_XFLIP;
        if (!!(trf & HAL_TRANSFORM_ROT_90))
            image.ext.transform |= M2M1SHOT2_IMGTRFORM_ROT90;
    }


    image.fmt.window.left = target_rect.pos.hori;
    image.fmt.window.top = target_rect.pos.vert;
    image.fmt.window.width = target_rect.size.hori;
    image.fmt.window.height = target_rect.size.vert;

    // Pretend if a global alpha value is always configured because its default value is 255(1.0)
    // If a user does not configure the global alpha value, it is always 255.
    image.flags |= M2M1SHOT2_IMGFLAG_GLOBAL_ALPHA;
    image.ext.galpha = layer.getPlaneAlpha();

    // Premultiplied alpha is not important to the compositor. It just means
    // that the compositor should not multiply its source color brightnesses
    // with the alpha values of the same pixel because it is already multiplied
    // to the brightness values. It is actually SRC_OVER with alpha.
    image.flags |= M2M1SHOT2_IMGFLAG_PREMUL_ALPHA;
    if ((layer.getCompositingMode() == HWC_BLENDING_PREMULT) ||
            (layer.getCompositingMode() == HWC2_BLEND_MODE_PREMULTIPLIED)) {
        image.ext.composit_mode = M2M1SHOT2_BLEND_SRCOVER;
    } else if ((layer.getCompositingMode() == HWC_BLENDING_COVERAGE) ||
               (layer.getCompositingMode() == HWC2_BLEND_MODE_COVERAGE)) {
        image.ext.composit_mode = M2M1SHOT2_BLEND_NONE;
    } else {
        image.ext.composit_mode = M2M1SHOT2_BLEND_SRC;
    }

    image.ext.scaler_filter = M2M1SHOT2_SCFILTER_BILINEAR;

    return true;
}

bool AcrylicCompositorM2M1SHOT2_G2D::executeG2D(int fence[], unsigned int num_fences, bool nonblocking)
{
    if (!validateAllLayers())
        return false;

    unsigned int layercount = layerCount();

    if (hasBackgroundColor()) {
        layercount++;

        if (layercount > getCapabilities().maxLayerCount()) {
            ALOGE("Too many layers %d with the default background color configured", layerCount());
            return false;
        }
    }

    if (mMaxSourceCount < layercount) {
        delete [] mDesc.sources;

        mDesc.sources = new m2m1shot2_image[layercount];
        if (!mDesc.sources) {
            ALOGE("Failed to allocate %u source image descriptors", layercount);
            return false;
        }

        mMaxSourceCount = layercount;
    }

    sortLayers();

    if (!prepareImage(mDesc.target, getCanvas())) {
        ALOGE("Failed to configure the target image");
        return false;
    }

    // The output brightness values of the G2D should be multiplied
    // with its associated alpha value because libacryl requires it.
    mDesc.target.flags |= M2M1SHOT2_IMGFLAG_PREMUL_ALPHA;

    unsigned int baseidx = 0;

    if (hasBackgroundColor()) {
        uint16_t a, r, g, b;

        getBackgroundColor(&r, &g, &b, &a);

        memset(&mDesc.sources[0], 0, sizeof(mDesc.sources[0]));

        mDesc.sources[0].flags = M2M1SHOT2_IMGFLAG_COLORFILL;
        mDesc.sources[0].memory = M2M1SHOT2_BUFTYPE_EMPTY;
        mDesc.sources[0].fmt.width = mDesc.target.fmt.width;
        mDesc.sources[0].fmt.height = mDesc.target.fmt.height;
        mDesc.sources[0].fmt.crop.width = mDesc.target.fmt.width;
        mDesc.sources[0].fmt.crop.height = mDesc.target.fmt.height;
        mDesc.sources[0].fmt.window.width = mDesc.target.fmt.width;
        mDesc.sources[0].fmt.window.height = mDesc.target.fmt.height;
        mDesc.sources[0].fmt.pixelformat = V4L2_PIX_FMT_ARGB32;
        mDesc.sources[0].ext.fillcolor = (a & 0xFF00) << 16;
        mDesc.sources[0].ext.fillcolor |= (r & 0xFF00) << 8;
        mDesc.sources[0].ext.fillcolor |= (g & 0xFF00) << 0;
        mDesc.sources[0].ext.fillcolor |= (b & 0xFF00) >> 8;

        baseidx++;
    }

    for (unsigned int i = baseidx; i < layercount; i++) {
        if (!prepareSource(mDesc.sources[i], *getLayer(i - baseidx),
                mDesc.target.fmt.width, mDesc.target.fmt.height)) {
            ALOGE("Failed to configure source layer %u", i - baseidx);
            return false;
        }
    }

    mDesc.num_sources = static_cast<uint8_t>(layercount);

    if (nonblocking)
        mDesc.flags = M2M1SHOT2_FLAG_NONBLOCK;

    unsigned int fence_idx = 0;
    if ((fence != NULL) && (num_fences > 0)) {
        if (!!(mDesc.target.flags & M2M1SHOT2_IMGFLAG_ACQUIRE_FENCE)) {
            mDesc.target.flags |= M2M1SHOT2_IMGFLAG_RELEASE_FENCE;
            fence_idx++;
        }

        for (unsigned int i = baseidx; (fence_idx < num_fences) && (i < mDesc.num_sources); i++) {
            if (!!(mDesc.sources[i].flags & M2M1SHOT2_IMGFLAG_ACQUIRE_FENCE)) {
                mDesc.sources[i].flags |= M2M1SHOT2_IMGFLAG_RELEASE_FENCE;
                fence_idx++;
            }
        }

        // Force generation of release fences from the layers without an acquire fence
        // if the client requests to generate more release fences than the acquire fences
        // it provided.
        if (fence_idx < num_fences) {
            if (!(mDesc.target.flags & M2M1SHOT2_IMGFLAG_RELEASE_FENCE)) {
                mDesc.target.flags |= M2M1SHOT2_IMGFLAG_RELEASE_FENCE;
                fence_idx++;
            }

            for (unsigned int i = baseidx; (fence_idx < num_fences) && (i < mDesc.num_sources); i++) {
                if (!(mDesc.sources[i].flags & M2M1SHOT2_IMGFLAG_RELEASE_FENCE)) {
                    mDesc.sources[i].flags |= M2M1SHOT2_IMGFLAG_RELEASE_FENCE;
                    fence_idx++;
                }
            }
        }
    }

    // inform an invalid fence fd to the users if they required more release fences than the acqiure fences.
    while ((fence != NULL) && (fence_idx < num_fences))
        fence[fence_idx++] = -1;

    debug_show_m2m1shot2(mDesc);

    if (mDev.ioctl_single(M2M1SHOT2_IOC_PROCESS, &mDesc) < 0) {
        if (errno != EBUSY)
            ALOGERR("Failed to process a m2m1shot2 task to G2D");

        return false;
    }

    if (!!(mDesc.flags & M2M1SHOT2_FLAG_ERROR)) {
        ALOGE("Error occurred during processing a m2m1shot2 task to G2D");
        return false;
    }

    getCanvas().clearSettingModified();
    getCanvas().setFence(-1);

    for (unsigned int i = 0; i < (layercount - baseidx); i++) {
        AcrylicLayer *layer = getLayer(i);

        layer->clearSettingModified();
        layer->setFence(-1);
    }

    if ((fence == NULL) || (num_fences == 0))
        return true;

    fence_idx = 0;
    if (!!(mDesc.target.flags & M2M1SHOT2_IMGFLAG_RELEASE_FENCE))
        fence[fence_idx++] = mDesc.target.fence;

    for (unsigned int i = 0; i < mDesc.num_sources; i++) {
        if (!!(mDesc.sources[i].flags & M2M1SHOT2_IMGFLAG_RELEASE_FENCE))
            fence[fence_idx++] = mDesc.sources[i].fence;
    }

    return true;
}

bool AcrylicCompositorM2M1SHOT2_G2D::execute(int fence[], unsigned int num_fences)
{
    if (!executeG2D(fence, num_fences, true)) {
        // Clearing all acquire fences because their buffers are expired.
        // The clients should configure everything again to start new execution
        for (unsigned int i = 0; i < layerCount(); i++)
            getLayer(i)->setFence(-1);
        getCanvas().setFence(-1);

        return false;
    }

    return true;
}

bool AcrylicCompositorM2M1SHOT2_G2D::execute(int *handle)
{
    if (!executeG2D(NULL, 0, handle ? true : false)) {
        // Clearing all acquire fences because their buffers are expired.
        // The clients should configure everything again to start new execution
        for (unsigned int i = 0; i < layerCount(); i++)
            getLayer(i)->setFence(-1);
        getCanvas().setFence(-1);

        return false;
    }

    if (handle != NULL)
        *handle = 1; /* dummy handle */

    return true;
}

bool AcrylicCompositorM2M1SHOT2_G2D::waitExecution(int __unused handle)
{
    LOG_FATAL(handle == 1);

    if ((mDev.ioctl_current(M2M1SHOT2_IOC_WAIT_PROCESS, &mDesc) < 0) && (errno != EAGAIN)) {
        ALOGERR("Failed to wait the previous execution");
        return false;
    }

    // m2m1shot2 fills target image payload and the state of the previous execution
    // but forget about the payload filled by m2m1shot2 because the user does not want it
    if (!!(mDesc.flags & M2M1SHOT2_FLAG_ERROR)) {
        ALOGE("An error occurred on the previous execution");
        return false;
    }

    ALOGD_TEST("Waiting for execution of m2m1shot2 G2D completed by handle %d", handle);

    return true;
}

void AcrylicCompositorM2M1SHOT2_G2D::removeTransitData(AcrylicLayer *layer)
{
    AcrylicTransitM2M1SHOT2_G2D *transit = reinterpret_cast<AcrylicTransitM2M1SHOT2_G2D *>(layer->getTransit());
    if (transit)
        delete transit;
}

static int32_t convertAcrylPriorityToM2M1SHOT2(int priority)
{
    static int32_t m2m1s2_priorities[] = {
            M2M1SHOT2_LOW_PRIORITY,     // 0
            M2M1SHOT2_MEDIUM_PRIORITY,  // 1
            M2M1SHOT2_HIGH_PRIORITY,    // 2
    };

    if (priority > 2)
        return M2M1SHOT2_HIGHEST_PRIORITY;
    else if (priority < 0)
        return M2M1SHOT2_DEFAULT_PRIORITY;
    else
        return m2m1s2_priorities[priority];
}

int AcrylicCompositorM2M1SHOT2_G2D::prioritize(int priority)
{
    if (priority == mPriority)
        return 0;

    if (Acrylic::prioritize(priority) < 0)
        return -1;

    int32_t arg = convertAcrylPriorityToM2M1SHOT2(priority);

    if (mDev.ioctl_broadcast(M2M1SHOT2_IOC_SET_PRIORITY, &arg) < 0) {
        if (errno != EBUSY) {
            ALOGERR("Failed to set priority on a context of G2D");
            return -1;
        }

        ALOGD("G2D Driver returned EBUSY but the priority of %d(%d) is successfully applied", priority, arg);
        return 1;
    }

    int ret = 0;
    unsigned int i;

    for (i = 0; i < layerCount(); i++) {
        AcrylicTransitM2M1SHOT2_G2D *transit;
        AcrylicCompositorM2M1SHOT2_G2D *prescaler;

        transit = reinterpret_cast<AcrylicTransitM2M1SHOT2_G2D *>(getLayer(i)->getTransit());
        if (!transit)
            continue;

        prescaler = transit->getCompositor();

        ret = prescaler->prioritize(priority);
        if (ret < 0) {
            ALOGE("Failed to configure priority %d to the prescaler of layer %u", priority, i);
            break;
        }
    }

    if (ret < 0) {
        while (i-- > 0) {
            AcrylicTransitM2M1SHOT2_G2D *transit;
            AcrylicCompositorM2M1SHOT2_G2D *prescaler;

            transit = reinterpret_cast<AcrylicTransitM2M1SHOT2_G2D *>(getLayer(i)->getTransit());
            if (!transit)
                continue;

            prescaler = transit->getCompositor();

            if (prescaler->prioritize(mPriority) < 0)
                ALOGE("Failed to restore priority of the prescaler of layer %u to %d", i, mPriority);
        }

        arg = convertAcrylPriorityToM2M1SHOT2(mPriority);

        if (mDev.ioctl_broadcast(M2M1SHOT2_IOC_SET_PRIORITY, &arg) < 0) {
            if (errno != EBUSY)
                ALOGERR("Failed to revert priority to %d on a context of G2D", mPriority);

            ALOGE("Successfully reverted priority to %d on a context of G2D", mPriority);
            return ret;
        }
    }

    ALOGD_TEST("Applied the priority of %d(%d) successfully", priority, arg);

    mPriority = priority;

    return 0;
}

bool AcrylicCompositorM2M1SHOT2_G2D::requestPerformanceQoS(AcrylicPerformanceRequest *request)
{
    m2m1shot2_performance_data data;

    memset(&data, 0, sizeof(data));

    if (!request || (request->getFrameCount() == 0)) {
        if (mDev.ioctl_unique(M2M1SHOT2_IOC_REQUEST_PERF, &data) < 0) {
            ALOGERR("Failed to cancel performance request");
            return false;
        }

        ALOGD_TEST("Canceled performance request");
        return true;
    }

    ALOGD_TEST("Requesting performance: frame count %d:", request->getFrameCount());
    for (int i = 0; i < request->getFrameCount(); i++) {
        AcrylicPerformanceRequestFrame *frame = request->getFrame(i);
        uint64_t bandwidth = 0;
        bool src_yuv420;
        bool src_rotate;

        src_rotate = false;
        src_yuv420 = false;

        unsigned int bpp;
        for (int idx = 0; idx < frame->getLayerCount(); idx++) {
            AcrylicPerformanceRequestLayer *layer = &(frame->mLayers[idx]);
            uint64_t layer_bw;
            int32_t weight;
            uint32_t pixelcount, dst_pixcount;

            pixelcount = layer->mSourceRect.size.hori * layer->mSourceRect.size.vert;
            dst_pixcount = layer->mTargetRect.size.hori * layer->mTargetRect.size.vert;
            if (pixelcount < dst_pixcount)
                pixelcount = dst_pixcount;

            data.frame[i].layer[idx].pixelcount = pixelcount;

            bpp = halfmt_bpp(layer->mPixFormat);
            if (bpp == 12)
                src_yuv420 = true;


            layer_bw = pixelcount * bpp;
            // Below is checking if scaling is involved.
            // Comparisons are replaced by additions to avoid branches.
            if (!!(layer->mTransform & HAL_TRANSFORM_ROT_90)) {
                weight = layer->mSourceRect.size.hori - layer->mTargetRect.size.vert;
                weight += layer->mSourceRect.size.vert - layer->mTargetRect.size.hori;
                src_rotate = true;
                data.frame[i].layer[idx].layer_attr |= M2M1SHOT2_PERF_LAYER_ROTATE;
            } else {
                weight = layer->mSourceRect.size.hori - layer->mTargetRect.size.hori;
                weight += layer->mSourceRect.size.vert - layer->mTargetRect.size.vert;
            }
            // Weight to the bandwidth when scaling is involved is 1.125.
            // It is multiplied by 16 to avoid multiplication with a real number.
            // We also get benefit from shift instead of multiplication.
            if (weight == 0) {
                layer_bw <<= 4; // layer_bw * 16
                weight = 1000;
            } else {
                layer_bw = (layer_bw << 4) + (layer_bw << 1); // layer_bw * 18
                weight = 1125;
            }

            bandwidth += layer_bw;
            ALOGD_TEST("        LAYER[%d]: BW %llu WGT %u FMT %#x(%u) (%dx%d)@(%dx%d)on(%dx%d) --> (%dx%d)@(%dx%d) TRFM %#x",
                    idx, static_cast<unsigned long long>(layer_bw), weight, layer->mPixFormat, bpp,
                    layer->mSourceRect.size.hori, layer->mSourceRect.size.vert,
                    layer->mSourceRect.pos.hori, layer->mSourceRect.pos.vert,
                    layer->mSourceDimension.hori, layer->mSourceDimension.vert,
                    layer->mTargetRect.size.hori, layer->mTargetRect.size.vert,
                    layer->mTargetRect.pos.hori, layer->mTargetRect.pos.vert, layer->mTransform);
        }

        bandwidth *= frame->mFrameRate;
        bandwidth >>= 17; // divide by 16(weight), 8(bpp) and 1024(kilobyte)

        data.frame[i].bandwidth_read = static_cast<uint32_t>(bandwidth);

        bpp = halfmt_bpp(frame->mTargetPixFormat);

        bandwidth = frame->mTargetDimension.hori * frame->mTargetDimension.vert;
        bandwidth *= frame->mFrameRate * bpp;

        // RSH 12 : bw * 2 / (bits_per_byte * kilobyte)
        // RHS 13 : bw * 1 / (bits_per_byte * kilobyte)
        bandwidth >>= ((bpp == 12) && src_yuv420 && src_rotate) ? 12 : 13;
        data.frame[i].bandwidth_write = static_cast<uint32_t>(bandwidth);

        if (frame->mHasBackgroundLayer)
            data.frame[i].frame_attr |= M2M1SHOT2_PERF_FRAME_SOLIDCOLORFILL;

        data.frame[i].num_layers = frame->getLayerCount();
        data.frame[i].target_pixelcount = frame->mTargetDimension.vert * frame->mTargetDimension.hori;
        data.frame[i].frame_rate = frame->mFrameRate;

        ALOGD_TEST("    FRAME[%d]: BW:(%u, %u) Layercount %d, Framerate %d, Target %dx%d, FMT %#x Background? %d",
            i, data.frame[i].bandwidth_read, data.frame[i].bandwidth_write, data.frame[i].num_layers, frame->mFrameRate,
            frame->mTargetDimension.hori, frame->mTargetDimension.vert, frame->mTargetPixFormat,
            frame->mHasBackgroundLayer);
    }

    data.num_frames = request->getFrameCount();

    if (mDev.ioctl_unique(M2M1SHOT2_IOC_REQUEST_PERF, &data) < 0) {
        ALOGERR("Failed to request performance");
        return false;
    }

    return true;
}
