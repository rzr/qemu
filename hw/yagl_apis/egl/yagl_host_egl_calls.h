#ifndef _QEMU_YAGL_HOST_EGL_CALLS_H
#define _QEMU_YAGL_HOST_EGL_CALLS_H

#include "yagl_api.h"
#include <EGL/egl.h>

struct yagl_api_ps
    *yagl_host_egl_process_init(struct yagl_api *api,
                                struct yagl_process_state *ps);

EGLint yagl_host_eglGetError(void);
yagl_host_handle yagl_host_eglGetDisplay(target_ulong /* Display* */ display_id);
EGLBoolean yagl_host_eglInitialize(yagl_host_handle dpy_,
    target_ulong /* EGLint* */ major,
    target_ulong /* EGLint* */ minor);
EGLBoolean yagl_host_eglTerminate(yagl_host_handle dpy_);
EGLBoolean yagl_host_eglGetConfigs(yagl_host_handle dpy_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_);
EGLBoolean yagl_host_eglChooseConfig(yagl_host_handle dpy_,
    target_ulong /* const EGLint* */ attrib_list_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_);
EGLBoolean yagl_host_eglGetConfigAttrib(yagl_host_handle dpy_,
    yagl_host_handle config_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
EGLBoolean yagl_host_eglDestroySurface(yagl_host_handle dpy_,
    yagl_host_handle surface_);
EGLBoolean yagl_host_eglQuerySurface(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
EGLBoolean yagl_host_eglBindAPI(EGLenum api);
EGLenum yagl_host_eglQueryAPI(void);
EGLBoolean yagl_host_eglWaitClient(void);
EGLBoolean yagl_host_eglReleaseThread(void);
yagl_host_handle yagl_host_eglCreatePbufferFromClientBuffer(yagl_host_handle dpy,
    EGLenum buftype,
    yagl_host_handle buffer,
    yagl_host_handle config,
    target_ulong /* const EGLint* */ attrib_list);
EGLBoolean yagl_host_eglSurfaceAttrib(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    EGLint value);
EGLBoolean yagl_host_eglBindTexImage(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer);
EGLBoolean yagl_host_eglReleaseTexImage(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer);
EGLBoolean yagl_host_eglSwapInterval(yagl_host_handle dpy,
    EGLint interval);
yagl_host_handle yagl_host_eglCreateContext(yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_host_handle share_context_,
    target_ulong /* const EGLint* */ attrib_list_);
EGLBoolean yagl_host_eglDestroyContext(yagl_host_handle dpy_,
    yagl_host_handle ctx_);
EGLBoolean yagl_host_eglMakeCurrent(yagl_host_handle dpy_,
    yagl_host_handle draw_,
    yagl_host_handle read_,
    yagl_host_handle ctx_);
yagl_host_handle yagl_host_eglGetCurrentContext(void);
yagl_host_handle yagl_host_eglGetCurrentSurface(EGLint readdraw);
yagl_host_handle yagl_host_eglGetCurrentDisplay(void);
EGLBoolean yagl_host_eglQueryContext(yagl_host_handle dpy_,
    yagl_host_handle ctx_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
EGLBoolean yagl_host_eglWaitGL(void);
EGLBoolean yagl_host_eglWaitNative(EGLint engine);
EGLBoolean yagl_host_eglSwapBuffers(yagl_host_handle dpy_,
    yagl_host_handle surface_);
EGLBoolean yagl_host_eglCopyBuffers(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLNativePixmapType target);
yagl_host_handle yagl_host_eglCreateWindowSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
yagl_host_handle yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
yagl_host_handle yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
EGLBoolean yagl_host_eglResizeOffscreenSurfaceYAGL(yagl_host_handle dpy_,
    yagl_host_handle surface_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_);

#endif
