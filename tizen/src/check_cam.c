/*
 * check the availability of a host webcam.
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Jinhyung Jo <jinhyung.jo@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

#ifdef _WIN32
/* Windows implement */
#include <stdio.h>
#include <windows.h>
#define CINTERFACE
#define COBJMACROS
#include "ocidl.h"
#include "errors.h"      /* for VFW_E_XXXX */
#include "mmsystem.h"    /* for MAKEFOURCC macro */
#include "hw/maru_camera_win32_interface.h"

/*
 * COM Interface implementations
 *
 */

#define SAFE_RELEASE(x) \
    do { \
        if (x) { \
            (x)->lpVtbl->Release(x); \
            x = NULL; \
        } \
    } while (0)

static int check_cam(void)
{
    int ret = 0;
    char *device_name = NULL;
    HRESULT hr = E_FAIL;
    ICreateDevEnum *pCreateDevEnum = NULL;
    IGraphBuilder *pGB = NULL;
    ICaptureGraphBuilder2 *pCGB = NULL;
    IEnumMoniker *pEnumMK = NULL;
    IMoniker *pMoniKer = NULL;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        fprintf(stdout, "[Webcam] failed to CoInitailizeEx\n");
        return ret;
    }

    hr = CoCreateInstance(&CLSID_FilterGraph, NULL,
                          CLSCTX_INPROC,
                          &IID_IGraphBuilder,
                          (void **)&pGB);
    if (FAILED(hr)) {
        fprintf(stdout, "[Webcam] Failed to create GraphBuilder, 0x%x\n", hr);
        CoUninitialize();
        return ret;
    }

    hr = CoCreateInstance(&CLSID_CaptureGraphBuilder2, NULL,
                          CLSCTX_INPROC,
                          &IID_ICaptureGraphBuilder2,
                          (void **)&pCGB);
    if (FAILED(hr)) {
        fprintf(stdout,
        "[Webcam] Failed to create CaptureGraphBuilder2, 0x%x\n", hr);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    hr = pCGB->lpVtbl->SetFiltergraph(pCGB, pGB);
    if (FAILED(hr)) {
        fprintf(stdout, "[Webcam] Failed to SetFiltergraph, 0x%x\n", hr);
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL,
                          CLSCTX_INPROC,
                          &IID_ICreateDevEnum,
                          (void **)&pCreateDevEnum);
    if (FAILED(hr)) {
        fprintf(stdout,
            "[Webcam] failed to create instance of CLSID_SystemDeviceEnum\n");
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    hr = pCreateDevEnum->lpVtbl->CreateClassEnumerator(pCreateDevEnum,
                                  &CLSID_VideoInputDeviceCategory, &pEnumMK, 0);
    if (FAILED(hr)) {
        fprintf(stdout, "[Webcam] failed to create class enumerator\n");
        SAFE_RELEASE(pCreateDevEnum);
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    if (!pEnumMK) {
        fprintf(stdout, "[Webcam] class enumerator is NULL!!\n");
        SAFE_RELEASE(pCreateDevEnum);
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    pEnumMK->lpVtbl->Reset(pEnumMK);
    hr = pEnumMK->lpVtbl->Next(pEnumMK, 1, &pMoniKer, NULL);
    if (FAILED(hr) || (hr == S_FALSE)) {
        fprintf(stdout, "[Webcam] enum moniker returns a invalid value.\n");
        SAFE_RELEASE(pEnumMK);
        SAFE_RELEASE(pCreateDevEnum);
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    }

    IPropertyBag *pBag = NULL;
    hr = pMoniKer->lpVtbl->BindToStorage(pMoniKer, 0, 0,
                                         &IID_IPropertyBag,
                                         (void **)&pBag);
    if (FAILED(hr)) {
        fprintf(stdout, "[Webcam] failed to bind to storage.\n");
        SAFE_RELEASE(pEnumMK);
        SAFE_RELEASE(pCreateDevEnum);
        SAFE_RELEASE(pCGB);
        SAFE_RELEASE(pGB);
        CoUninitialize();
        return ret;
    } else {
        VARIANT var;
        var.vt = VT_BSTR;
        hr = pBag->lpVtbl->Read(pBag, L"FriendlyName", &var, NULL);
        if (hr == S_OK) {
            ret = 1;
            fprintf(stdout, "[Webcam] Check success\n");
        } else {
            fprintf(stdout, "[Webcam] failed to find to webcam device.\n");
        }
        SysFreeString(var.bstrVal);
        SAFE_RELEASE(pBag);
    }
    SAFE_RELEASE(pMoniKer);
    SAFE_RELEASE(pCGB);
    SAFE_RELEASE(pGB);
    SAFE_RELEASE(pEnumMK);
    SAFE_RELEASE(pCreateDevEnum);
    CoUninitialize();

    return ret;
}


#elif __linux

/* Linux implement */
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/videodev2.h>

static int check_cam(void)
{
    int tmp_fd;
    struct stat st;
    struct v4l2_fmtdesc format;
    struct v4l2_frmsizeenum size;
    struct v4l2_capability cap;
    char dev_name[] = "/dev/video0";
    int ret = 0;

    if (stat(dev_name, &st) < 0) {
        fprintf(stdout, "[Webcam] <WARNING> Cannot identify '%s': %d\n",
                dev_name, errno);
    } else {
        if (!S_ISCHR(st.st_mode)) {
            fprintf(stdout, "[Webcam] <WARNING>%s is no character device\n",
                    dev_name);
        }
    }

    tmp_fd = open(dev_name, O_RDWR | O_NONBLOCK, 0);
    if (tmp_fd < 0) {
        fprintf(stdout, "[Webcam] Camera device open failed: %s\n", dev_name);
        return ret;
    }
    if (ioctl(tmp_fd, VIDIOC_QUERYCAP, &cap) < 0) {
        fprintf(stdout, "[Webcam] Could not qeury video capabilities\n");
        close(tmp_fd);
        return ret;
    }
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
            !(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stdout, "[Webcam] Not supported video driver\n");
        close(tmp_fd);
        return ret;
    }
    fprintf(stdout, "[Webcam] Check success\n");
    close(tmp_fd);

    return 1;
}

#elif __APPLE__
/* MacOS, Now, not implemented. */
static int check_cam(void)
{
    return 1;
}

#else
/* Unsupported OS */
static int check_cam(void)
{
    fprintf(stdout, "[Webcam] Unsupported OS. Not available.\n");
    return 0;
}
#endif    /* end of the if statement */

int main(int argc, char** argv)
{
    return check_cam();
}

