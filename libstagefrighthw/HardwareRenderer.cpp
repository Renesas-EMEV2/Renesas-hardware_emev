/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "HardwareRenderer"
#include <utils/Log.h>

#include "HardwareRenderer.h"

#include <media/stagefright/MediaDebug.h>

namespace android {

////////////////////////////////////////////////////////////////////////////////

EMEVHardwareRenderer::EMEVHardwareRenderer(
        const sp<ISurface> &surface,
        size_t displayWidth, size_t displayHeight,
        size_t decodedWidth, size_t decodedHeight,
        OMX_COLOR_FORMATTYPE colorFormat)
    : mISurface(surface),
      mDisplayWidth(displayWidth),
      mDisplayHeight(displayHeight),
      mDecodedWidth(decodedWidth),
      mDecodedHeight(decodedHeight),
      mColorFormat(colorFormat),
      mInitCheck(NO_INIT),
      mIndex(0)
{
    int overlay_format;

    if (colorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        overlay_format = OVERLAY_FORMAT_YCbCr_420_SP;
    } else if (colorFormat == OMX_COLOR_FormatYUV420Planar) {
        overlay_format = OVERLAY_FORMAT_YCbCr_420_P;
    } else {
        return;
    }

    sp<OverlayRef> ref = mISurface->createOverlay(
            mDisplayWidth, mDisplayHeight, overlay_format, 0);
    if (ref.get() == NULL) {
        LOGE("Unable to create the overlay!");
        return;
    }

    mOverlay = new Overlay(ref);

    overlay_buffer_t buf;
    for (int i = 0; i < USE_BUF_COUNT; i++) {
        buf = (overlay_buffer_t)&mOverlayBuffer[i];
        if (mOverlay->dequeueBuffer(&buf)) {
            return;
        }
        mOverlayBuffer[i].w_src = mDecodedWidth;
        mOverlayBuffer[i].h_src = mDecodedHeight;
    }

    mInitCheck = OK;
}

EMEVHardwareRenderer::~EMEVHardwareRenderer()
{
    if (mOverlay.get() != NULL) {
        mOverlay->destroy();
        mOverlay.clear();

        // XXX apparently destroying an overlay is an asynchronous process...
        sleep(1);
    }
}

void EMEVHardwareRenderer::render(
        const void *data, size_t size, void *platformPrivate)
{

    if (mOverlay.get() == NULL) {
        return;
    }

    overlay_buffer_t buf = (overlay_buffer_t)&mOverlayBuffer[mIndex];

    mOverlayBuffer[mIndex].vaddr = (void *)data;
    mOverlay->queueBuffer(buf);
    mOverlay->dequeueBuffer(&buf);
    if (++mIndex == USE_BUF_COUNT) mIndex = 0;
}

}  // namespace android

