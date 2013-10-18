#include "vigs_gl_backend.h"
#include "vigs_log.h"
#include <GL/glx.h>
#include <dlfcn.h>

#ifndef GLX_VERSION_1_4
#error GL/glx.h must be equal to or greater than GLX 1.4
#endif

#define VIGS_GLX_GET_PROC(proc_type, proc_name) \
    do { \
        gl_backend_glx->proc_name = \
            (proc_type)gl_backend_glx->glXGetProcAddress((const GLubyte*)#proc_name); \
        if (!gl_backend_glx->proc_name) { \
            VIGS_LOG_CRITICAL("Unable to load symbol: %s", dlerror()); \
            goto fail2; \
        } \
    } while (0)

#define VIGS_GL_GET_PROC(func, proc_name) \
    do { \
        *(void**)(&gl_backend_glx->base.func) = gl_backend_glx->glXGetProcAddress((const GLubyte*)#proc_name); \
        if (!gl_backend_glx->base.func) { \
            *(void**)(&gl_backend_glx->base.func) = dlsym(gl_backend_glx->handle, #proc_name); \
            if (!gl_backend_glx->base.func) { \
                VIGS_LOG_CRITICAL("Unable to load symbol: %s", dlerror()); \
                goto fail2; \
            } \
        } \
    } while (0)

#define VIGS_GL_GET_PROC_OPTIONAL(func, proc_name) \
    do { \
        *(void**)(&gl_backend_glx->base.func) = gl_backend_glx->glXGetProcAddress((const GLubyte*)#proc_name); \
        if (!gl_backend_glx->base.func) { \
            *(void**)(&gl_backend_glx->base.func) = dlsym(gl_backend_glx->handle, #proc_name); \
        } \
    } while (0)

/* GLX 1.0 */
typedef void (*PFNGLXDESTROYCONTEXTPROC)(Display *dpy, GLXContext ctx);
typedef GLXContext (*PFNGLXGETCURRENTCONTEXTPROC)(void);

struct vigs_gl_backend_glx
{
    struct vigs_gl_backend base;

    void *handle;

    PFNGLXGETPROCADDRESSPROC glXGetProcAddress;
    PFNGLXCHOOSEFBCONFIGPROC glXChooseFBConfig;
    PFNGLXGETFBCONFIGATTRIBPROC glXGetFBConfigAttrib;
    PFNGLXCREATEPBUFFERPROC glXCreatePbuffer;
    PFNGLXDESTROYPBUFFERPROC glXDestroyPbuffer;
    PFNGLXDESTROYCONTEXTPROC glXDestroyContext;
    PFNGLXMAKECONTEXTCURRENTPROC glXMakeContextCurrent;
    PFNGLXGETCURRENTCONTEXTPROC glXGetCurrentContext;

    /* GLX_ARB_create_context */
    PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB;

    Display *dpy;
    GLXPbuffer sfc;
    GLXContext ctx;
};

static GLXFBConfig vigs_gl_backend_glx_get_config(struct vigs_gl_backend_glx *gl_backend_glx)
{
    int config_attribs[] =
    {
        GLX_DOUBLEBUFFER, True,
        GLX_RED_SIZE, 8,
        GLX_GREEN_SIZE, 8,
        GLX_BLUE_SIZE, 8,
        GLX_ALPHA_SIZE, 8,
        GLX_BUFFER_SIZE, 32,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        None
    };
    int n = 0;
    GLXFBConfig *glx_configs;
    GLXFBConfig best_config = NULL;

    glx_configs = gl_backend_glx->glXChooseFBConfig(gl_backend_glx->dpy,
                                                    DefaultScreen(gl_backend_glx->dpy),
                                                    config_attribs,
                                                    &n);

    if (n > 0) {
        int tmp;
        best_config = glx_configs[0];

        if (gl_backend_glx->glXGetFBConfigAttrib(gl_backend_glx->dpy,
                                                 best_config,
                                                 GLX_FBCONFIG_ID,
                                                 &tmp) == Success) {
            VIGS_LOG_INFO("GLX config ID: 0x%X", tmp);
        }

        if (gl_backend_glx->glXGetFBConfigAttrib(gl_backend_glx->dpy,
                                                 best_config,
                                                 GLX_VISUAL_ID,
                                                 &tmp) == Success) {
            VIGS_LOG_INFO("GLX visual ID: 0x%X", tmp);
        }
    }

    XFree(glx_configs);

    if (!best_config) {
        VIGS_LOG_CRITICAL("Suitable GLX config not found");
    }

    return best_config;
}

static bool vigs_gl_backend_glx_create_surface(struct vigs_gl_backend_glx *gl_backend_glx,
                                               GLXFBConfig config)
{
    int surface_attribs[] =
    {
        GLX_PBUFFER_WIDTH, 1,
        GLX_PBUFFER_HEIGHT, 1,
        GLX_LARGEST_PBUFFER, False,
        None
    };

    gl_backend_glx->sfc = gl_backend_glx->glXCreatePbuffer(gl_backend_glx->dpy,
                                                           config,
                                                           surface_attribs);

    if (!gl_backend_glx->sfc) {
        VIGS_LOG_CRITICAL("glXCreatePbuffer failed");
        return false;
    }

    return true;
}

static bool vigs_gl_backend_glx_create_context(struct vigs_gl_backend_glx *gl_backend_glx,
                                               GLXFBConfig config)
{
    int attribs[] =
    {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        GLX_RENDER_TYPE, GLX_RGBA_TYPE,
        None
    };

    gl_backend_glx->ctx = gl_backend_glx->glXCreateContextAttribsARB(gl_backend_glx->dpy,
                                                                     config,
                                                                     NULL,
                                                                     True,
                                                                     attribs);

    if (!gl_backend_glx->ctx) {
        VIGS_LOG_CRITICAL("glXCreateContextAttribsARB failed");
        return false;
    }

    return true;
}

static bool vigs_gl_backend_glx_has_current(struct vigs_gl_backend *gl_backend)
{
    struct vigs_gl_backend_glx *gl_backend_glx =
        (struct vigs_gl_backend_glx*)gl_backend;

    return gl_backend_glx->glXGetCurrentContext() != NULL;
}

static bool vigs_gl_backend_glx_make_current(struct vigs_gl_backend *gl_backend,
                                             bool enable)
{
    struct vigs_gl_backend_glx *gl_backend_glx =
        (struct vigs_gl_backend_glx*)gl_backend;
    Bool ret;

    ret = gl_backend_glx->glXMakeContextCurrent(gl_backend_glx->dpy,
                                                (enable ? gl_backend_glx->sfc : None),
                                                (enable ? gl_backend_glx->sfc : None),
                                                (enable ? gl_backend_glx->ctx : NULL));

    if (!ret) {
        VIGS_LOG_CRITICAL("glXMakeContextCurrent failed");
        return false;
    }

    return true;
}

static void vigs_gl_backend_glx_destroy(struct vigs_backend *backend)
{
    struct vigs_gl_backend_glx *gl_backend_glx = (struct vigs_gl_backend_glx*)backend;

    vigs_gl_backend_cleanup(&gl_backend_glx->base);

    gl_backend_glx->glXDestroyContext(gl_backend_glx->dpy,
                                      gl_backend_glx->ctx);

    gl_backend_glx->glXDestroyPbuffer(gl_backend_glx->dpy,
                                      gl_backend_glx->sfc);

    dlclose(gl_backend_glx->handle);

    vigs_backend_cleanup(&gl_backend_glx->base.base);

    g_free(gl_backend_glx);

    VIGS_LOG_DEBUG("destroyed");
}

struct vigs_backend *vigs_gl_backend_create(void *display)
{
    struct vigs_gl_backend_glx *gl_backend_glx;
    GLXFBConfig config;
    Display *x_display = display;

    gl_backend_glx = g_malloc0(sizeof(*gl_backend_glx));

    vigs_backend_init(&gl_backend_glx->base.base,
                      &gl_backend_glx->base.ws_info.base);

    gl_backend_glx->handle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);

    if (!gl_backend_glx->handle) {
        VIGS_LOG_CRITICAL("Unable to load libGL.so.1: %s", dlerror());
        goto fail1;
    }

    gl_backend_glx->glXGetProcAddress =
        dlsym(gl_backend_glx->handle, "glXGetProcAddress");

    if (!gl_backend_glx->glXGetProcAddress) {
        gl_backend_glx->glXGetProcAddress =
            dlsym(gl_backend_glx->handle, "glXGetProcAddressARB");
    }

    if (!gl_backend_glx->glXGetProcAddress) {
        VIGS_LOG_CRITICAL("Unable to load symbol: %s", dlerror());
        goto fail2;
    }

    VIGS_GLX_GET_PROC(PFNGLXCHOOSEFBCONFIGPROC, glXChooseFBConfig);
    VIGS_GLX_GET_PROC(PFNGLXGETFBCONFIGATTRIBPROC, glXGetFBConfigAttrib);
    VIGS_GLX_GET_PROC(PFNGLXCREATEPBUFFERPROC, glXCreatePbuffer);
    VIGS_GLX_GET_PROC(PFNGLXDESTROYPBUFFERPROC, glXDestroyPbuffer);
    VIGS_GLX_GET_PROC(PFNGLXDESTROYCONTEXTPROC, glXDestroyContext);
    VIGS_GLX_GET_PROC(PFNGLXMAKECONTEXTCURRENTPROC, glXMakeContextCurrent);
    VIGS_GLX_GET_PROC(PFNGLXGETCURRENTCONTEXTPROC, glXGetCurrentContext);
    VIGS_GLX_GET_PROC(PFNGLXCREATECONTEXTATTRIBSARBPROC, glXCreateContextAttribsARB);

    VIGS_GL_GET_PROC(GenTextures, glGenTextures);
    VIGS_GL_GET_PROC(DeleteTextures, glDeleteTextures);
    VIGS_GL_GET_PROC(BindTexture, glBindTexture);
    VIGS_GL_GET_PROC(Begin, glBegin);
    VIGS_GL_GET_PROC(End, glEnd);
    VIGS_GL_GET_PROC(CullFace, glCullFace);
    VIGS_GL_GET_PROC(TexParameterf, glTexParameterf);
    VIGS_GL_GET_PROC(TexParameterfv, glTexParameterfv);
    VIGS_GL_GET_PROC(TexParameteri, glTexParameteri);
    VIGS_GL_GET_PROC(TexParameteriv, glTexParameteriv);
    VIGS_GL_GET_PROC(TexImage2D, glTexImage2D);
    VIGS_GL_GET_PROC(TexSubImage2D, glTexSubImage2D);
    VIGS_GL_GET_PROC(TexEnvf, glTexEnvf);
    VIGS_GL_GET_PROC(TexEnvfv, glTexEnvfv);
    VIGS_GL_GET_PROC(TexEnvi, glTexEnvi);
    VIGS_GL_GET_PROC(TexEnviv, glTexEnviv);
    VIGS_GL_GET_PROC(Clear, glClear);
    VIGS_GL_GET_PROC(ClearColor, glClearColor);
    VIGS_GL_GET_PROC(Disable, glDisable);
    VIGS_GL_GET_PROC(Enable, glEnable);
    VIGS_GL_GET_PROC(Finish, glFinish);
    VIGS_GL_GET_PROC(Flush, glFlush);
    VIGS_GL_GET_PROC(PixelStorei, glPixelStorei);
    VIGS_GL_GET_PROC(ReadPixels, glReadPixels);
    VIGS_GL_GET_PROC(Viewport, glViewport);
    VIGS_GL_GET_PROC(GenFramebuffers, glGenFramebuffersEXT);
    VIGS_GL_GET_PROC(GenRenderbuffers, glGenRenderbuffersEXT);
    VIGS_GL_GET_PROC(DeleteFramebuffers, glDeleteFramebuffersEXT);
    VIGS_GL_GET_PROC(DeleteRenderbuffers, glDeleteRenderbuffersEXT);
    VIGS_GL_GET_PROC(BindFramebuffer, glBindFramebufferEXT);
    VIGS_GL_GET_PROC(BindRenderbuffer, glBindRenderbufferEXT);
    VIGS_GL_GET_PROC(RenderbufferStorage, glRenderbufferStorageEXT);
    VIGS_GL_GET_PROC(FramebufferRenderbuffer, glFramebufferRenderbufferEXT);
    VIGS_GL_GET_PROC(FramebufferTexture2D, glFramebufferTexture2DEXT);
    VIGS_GL_GET_PROC(GetIntegerv, glGetIntegerv);
    VIGS_GL_GET_PROC(GetString, glGetString);
    VIGS_GL_GET_PROC(LoadIdentity, glLoadIdentity);
    VIGS_GL_GET_PROC(MatrixMode, glMatrixMode);
    VIGS_GL_GET_PROC(Ortho, glOrtho);
    VIGS_GL_GET_PROC(EnableClientState, glEnableClientState);
    VIGS_GL_GET_PROC(DisableClientState, glDisableClientState);
    VIGS_GL_GET_PROC(Color4f, glColor4f);
    VIGS_GL_GET_PROC(TexCoordPointer, glTexCoordPointer);
    VIGS_GL_GET_PROC(VertexPointer, glVertexPointer);
    VIGS_GL_GET_PROC(DrawArrays, glDrawArrays);
    VIGS_GL_GET_PROC(Color4ub, glColor4ub);
    VIGS_GL_GET_PROC(WindowPos2f, glWindowPos2f);
    VIGS_GL_GET_PROC(PixelZoom, glPixelZoom);
    VIGS_GL_GET_PROC(DrawPixels, glDrawPixels);
    VIGS_GL_GET_PROC(BlendFunc, glBlendFunc);
    VIGS_GL_GET_PROC(CopyTexImage2D, glCopyTexImage2D);
    VIGS_GL_GET_PROC(BlitFramebuffer, glBlitFramebufferEXT);

    gl_backend_glx->dpy = x_display;

    config = vigs_gl_backend_glx_get_config(gl_backend_glx);

    if (!config) {
        goto fail2;
    }

    if (!vigs_gl_backend_glx_create_surface(gl_backend_glx, config)) {
        goto fail2;
    }

    if (!vigs_gl_backend_glx_create_context(gl_backend_glx, config)) {
        goto fail3;
    }

    gl_backend_glx->base.base.destroy = &vigs_gl_backend_glx_destroy;
    gl_backend_glx->base.has_current = &vigs_gl_backend_glx_has_current;
    gl_backend_glx->base.make_current = &vigs_gl_backend_glx_make_current;
    gl_backend_glx->base.ws_info.context = gl_backend_glx->ctx;

    if (!vigs_gl_backend_init(&gl_backend_glx->base)) {
        goto fail4;
    }

    VIGS_LOG_DEBUG("created");

    return &gl_backend_glx->base.base;

fail4:
    gl_backend_glx->glXDestroyContext(gl_backend_glx->dpy,
                                      gl_backend_glx->ctx);
fail3:
    gl_backend_glx->glXDestroyPbuffer(gl_backend_glx->dpy,
                                      gl_backend_glx->sfc);
fail2:
    dlclose(gl_backend_glx->handle);
fail1:
    vigs_backend_cleanup(&gl_backend_glx->base.base);

    g_free(gl_backend_glx);

    return NULL;
}
