#ifndef _QEMU_YAGL_HOST_EGL_CALLS_H
#define _QEMU_YAGL_HOST_EGL_CALLS_H

#include "yagl_api.h"
#include <EGL/egl.h>

struct yagl_api_ps *yagl_host_egl_process_init(struct yagl_api *api);

yagl_host_handle yagl_host_eglGetDisplay(uint32_t display_id,
    EGLint *error);
EGLBoolean yagl_host_eglInitialize(yagl_host_handle dpy,
    EGLint *major,
    EGLint *minor,
    EGLint *error);
EGLBoolean yagl_host_eglTerminate(yagl_host_handle dpy,
    EGLint *error);
EGLBoolean yagl_host_eglGetConfigs(yagl_host_handle dpy,
    yagl_host_handle *configs, int32_t configs_maxcount, int32_t *configs_count,
    EGLint *error);
EGLBoolean yagl_host_eglChooseConfig(yagl_host_handle dpy,
    const EGLint *attrib_list, int32_t attrib_list_count,
    yagl_host_handle *configs, int32_t configs_maxcount, int32_t *configs_count,
    EGLint *error);
EGLBoolean yagl_host_eglGetConfigAttrib(yagl_host_handle dpy,
    yagl_host_handle config,
    EGLint attribute,
    EGLint *value,
    EGLint *error);
EGLBoolean yagl_host_eglDestroySurface(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint *error);
EGLBoolean yagl_host_eglQuerySurface(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint attribute,
    EGLint *value,
    EGLint *error);
void yagl_host_eglBindAPI(EGLenum api);
void yagl_host_eglWaitClient(void);
EGLBoolean yagl_host_eglReleaseThread(EGLint *error);
EGLBoolean yagl_host_eglSurfaceAttrib(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint attribute,
    EGLint value,
    EGLint *error);
yagl_host_handle yagl_host_eglCreateContext(yagl_host_handle dpy,
    yagl_host_handle config,
    yagl_host_handle share_context,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
EGLBoolean yagl_host_eglDestroyContext(yagl_host_handle dpy,
    yagl_host_handle ctx,
    EGLint *error);
void yagl_host_eglMakeCurrent(yagl_host_handle dpy,
    yagl_host_handle draw,
    yagl_host_handle read,
    yagl_host_handle ctx);
EGLBoolean yagl_host_eglQueryContext(yagl_host_handle dpy,
    yagl_host_handle ctx,
    EGLint attribute,
    EGLint *value,
    EGLint *error);
EGLBoolean yagl_host_eglSwapBuffers(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint *error);
EGLBoolean yagl_host_eglCopyBuffers(yagl_host_handle dpy,
    yagl_host_handle surface,
    EGLint *error);
yagl_host_handle yagl_host_eglCreateWindowSurfaceOffscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
yagl_host_handle yagl_host_eglCreatePbufferSurfaceOffscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
yagl_host_handle yagl_host_eglCreatePixmapSurfaceOffscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
EGLBoolean yagl_host_eglResizeOffscreenSurfaceYAGL(yagl_host_handle dpy,
    yagl_host_handle surface,
    uint32_t width,
    uint32_t height,
    uint32_t bpp,
    target_ulong pixels,
    EGLint *error);
yagl_host_handle yagl_host_eglCreateWindowSurfaceOnscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    yagl_winsys_id win,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
yagl_host_handle yagl_host_eglCreatePbufferSurfaceOnscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    yagl_winsys_id buffer,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
yagl_host_handle yagl_host_eglCreatePixmapSurfaceOnscreenYAGL(yagl_host_handle dpy,
    yagl_host_handle config,
    yagl_winsys_id pixmap,
    const EGLint *attrib_list, int32_t attrib_list_count,
    EGLint *error);
void yagl_host_eglInvalidateOnscreenSurfaceYAGL(yagl_host_handle dpy,
    yagl_host_handle surface,
    yagl_winsys_id buffer);
EGLBoolean yagl_host_eglCreateImageYAGL(uint32_t texture,
    yagl_host_handle dpy,
    yagl_winsys_id buffer,
    EGLint *error);

#endif
