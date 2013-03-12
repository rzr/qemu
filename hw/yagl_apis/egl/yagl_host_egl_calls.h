#ifndef _QEMU_YAGL_HOST_EGL_CALLS_H
#define _QEMU_YAGL_HOST_EGL_CALLS_H

#include "yagl_api.h"
#include <EGL/egl.h>

struct yagl_api_ps
    *yagl_host_egl_process_init(struct yagl_api *api,
                                struct yagl_process_state *ps);

bool yagl_host_eglGetError(EGLint* retval);
bool yagl_host_eglGetDisplay(yagl_host_handle* retval,
    target_ulong /* Display* */ display_id);
bool yagl_host_eglInitialize(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* EGLint* */ major,
    target_ulong /* EGLint* */ minor);
bool yagl_host_eglTerminate(EGLBoolean* retval,
    yagl_host_handle dpy_);
bool yagl_host_eglGetConfigs(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_);
bool yagl_host_eglChooseConfig(EGLBoolean* retval,
    yagl_host_handle dpy_,
    target_ulong /* const EGLint* */ attrib_list_,
    target_ulong /* EGLConfig^* */ configs_,
    EGLint config_size,
    target_ulong /* EGLint* */ num_config_);
bool yagl_host_eglGetConfigAttrib(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
bool yagl_host_eglDestroySurface(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_);
bool yagl_host_eglQuerySurface(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
bool yagl_host_eglBindAPI(EGLBoolean* retval,
    EGLenum api);
bool yagl_host_eglWaitClient(EGLBoolean* retval);
bool yagl_host_eglReleaseThread(EGLBoolean* retval);
bool yagl_host_eglCreatePbufferFromClientBuffer(yagl_host_handle* retval,
    yagl_host_handle dpy,
    EGLenum buftype,
    yagl_host_handle buffer,
    yagl_host_handle config,
    target_ulong /* const EGLint* */ attrib_list);
bool yagl_host_eglSurfaceAttrib(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLint attribute,
    EGLint value);
bool yagl_host_eglBindTexImage(EGLBoolean* retval,
    yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer);
bool yagl_host_eglReleaseTexImage(EGLBoolean* retval,
    yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint buffer);
bool yagl_host_eglCreateContext(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    yagl_host_handle share_context_,
    target_ulong /* const EGLint* */ attrib_list_);
bool yagl_host_eglDestroyContext(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle ctx_);
bool yagl_host_eglMakeCurrent(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle draw_,
    yagl_host_handle read_,
    yagl_host_handle ctx_);
bool yagl_host_eglQueryContext(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle ctx_,
    EGLint attribute,
    target_ulong /* EGLint* */ value_);
bool yagl_host_eglWaitGL(EGLBoolean* retval);
bool yagl_host_eglWaitNative(EGLBoolean* retval,
    EGLint engine);
bool yagl_host_eglSwapBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_);
bool yagl_host_eglCopyBuffers(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    EGLNativePixmapType target);
bool yagl_host_eglCreateWindowSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
bool yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
bool yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(yagl_host_handle* retval,
    yagl_host_handle dpy_,
    yagl_host_handle config_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_,
    target_ulong /* const EGLint* */ attrib_list_);
bool yagl_host_eglResizeOffscreenSurfaceYAGL(EGLBoolean* retval,
    yagl_host_handle dpy_,
    yagl_host_handle surface_,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong /* void* */ pixels_);

#endif
