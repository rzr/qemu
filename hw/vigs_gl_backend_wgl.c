#include "vigs_gl_backend.h"
#include "vigs_log.h"
#include <windows.h>
#include <wingdi.h>
#include <GL/gl.h>
#include <GL/wglext.h>

#define VIGS_WGL_WIN_CLASS "VIGSWinClass"

#define VIGS_WGL_GET_PROC(proc_type, proc_name, label) \
    do { \
        gl_backend_wgl->proc_name = \
            (proc_type)GetProcAddress(gl_backend_wgl->handle, #proc_name); \
        if (!gl_backend_wgl->proc_name) { \
            VIGS_LOG_CRITICAL("Unable to load " #proc_name " symbol"); \
            goto label; \
        } \
    } while (0)

#define VIGS_WGL_GET_EXT_PROC(ext_name, proc_type, proc_name) \
    do { \
        if ((strstr(ext_str, #ext_name " ") == NULL)) { \
            VIGS_LOG_CRITICAL("%s extension not supported", #ext_name); \
            goto fail; \
        } \
        gl_backend_wgl->proc_name = \
            (proc_type)gl_backend_wgl->wglGetProcAddress((LPCSTR)#proc_name); \
        if (!gl_backend_wgl->proc_name) { \
            VIGS_LOG_CRITICAL("Unable to load " #proc_name " symbol"); \
            goto fail; \
        } \
    } while (0)

#define VIGS_GL_GET_PROC(func, proc_name) \
    do { \
        *(void**)(&gl_backend_wgl->base.func) = gl_backend_wgl->wglGetProcAddress((LPCSTR)#proc_name); \
        if (!gl_backend_wgl->base.func) { \
            *(void**)(&gl_backend_wgl->base.func) = GetProcAddress(gl_backend_wgl->handle, #proc_name); \
            if (!gl_backend_wgl->base.func) { \
                VIGS_LOG_CRITICAL("Unable to load " #proc_name " symbol"); \
                goto fail; \
            } \
        } \
    } while (0)

#define VIGS_GL_GET_PROC_OPTIONAL(func, proc_name) \
    do { \
        *(void**)(&gl_backend_wgl->base.func) = gl_backend_wgl->wglGetProcAddress((LPCSTR)#proc_name); \
        if (!gl_backend_wgl->base.func) { \
            *(void**)(&gl_backend_wgl->base.func) = GetProcAddress(gl_backend_wgl->handle, #proc_name); \
        } \
    } while (0)

typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTPROC)(HDC hdl);
typedef BOOL (WINAPI *PFNWGLDELETECONTEXTPROC)(HGLRC hdl);
typedef PROC (WINAPI *PFNWGLGETPROCADDRESSPROC)(LPCSTR sym);
typedef BOOL (WINAPI *PFNWGLMAKECURRENTPROC)(HDC dev_ctx, HGLRC rend_ctx);
typedef BOOL (WINAPI *PFNWGLSHARELISTSPROC)(HGLRC ctx1, HGLRC ctx2);
typedef HGLRC (WINAPI *PFNWGLGETCURRENTCONTEXTPROC)(void);

struct vigs_gl_backend_wgl
{
    struct vigs_gl_backend base;

    HINSTANCE handle;

    PFNWGLCREATECONTEXTPROC wglCreateContext;
    PFNWGLDELETECONTEXTPROC wglDeleteContext;
    PFNWGLGETPROCADDRESSPROC wglGetProcAddress;
    PFNWGLMAKECURRENTPROC wglMakeCurrent;
    PFNWGLGETCURRENTCONTEXTPROC wglGetCurrentContext;

    /* WGL extensions */
    PFNWGLGETEXTENSIONSSTRINGEXTPROC wglGetExtensionsStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
    PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
    PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
    PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
    PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
    PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
    PFNWGLMAKECONTEXTCURRENTARBPROC wglMakeContextCurrentARB;

    HWND win;
    HDC dc;
    HPBUFFERARB sfc;
    HGLRC ctx;
};

static int vigs_gl_backend_wgl_choose_config(struct vigs_gl_backend_wgl *gl_backend_wgl)
{
    const int config_attribs[] = {
        WGL_SUPPORT_OPENGL_ARB, TRUE,
        WGL_DOUBLE_BUFFER_ARB, TRUE,
        WGL_DRAW_TO_PBUFFER_ARB, TRUE,
        WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
        WGL_RED_BITS_ARB, 8,
        WGL_GREEN_BITS_ARB, 8,
        WGL_BLUE_BITS_ARB, 8,
        WGL_ALPHA_BITS_ARB, 8,
        WGL_COLOR_BITS_ARB, 32,
        WGL_DEPTH_BITS_ARB, 24,
        WGL_STENCIL_BITS_ARB, 8,
        0,
    };
    int config_id = 0;
    UINT n = 0;
    PIXELFORMATDESCRIPTOR pix_fmt;

    if (!gl_backend_wgl->wglChoosePixelFormatARB(gl_backend_wgl->dc,
                                                 config_attribs,
                                                 NULL,
                                                 1,
                                                 &config_id,
                                                 &n) || (n == 0)) {
        return 0;
    }

    if (!DescribePixelFormat(gl_backend_wgl->dc,
                             config_id,
                             sizeof(PIXELFORMATDESCRIPTOR),
                             &pix_fmt)) {
        return 0;
    }

    if (!SetPixelFormat(gl_backend_wgl->dc,
                        config_id,
                        &pix_fmt)) {
        return 0;
    }

    return config_id;
}

static bool vigs_gl_backend_wgl_create_surface(struct vigs_gl_backend_wgl *gl_backend_wgl,
                                               int config_id)
{
    int surface_attribs[] = {
        WGL_PBUFFER_LARGEST_ARB, FALSE,
        WGL_TEXTURE_TARGET_ARB, WGL_NO_TEXTURE_ARB,
        WGL_TEXTURE_FORMAT_ARB, WGL_NO_TEXTURE_ARB,
        0
    };

    gl_backend_wgl->sfc =
        gl_backend_wgl->wglCreatePbufferARB(gl_backend_wgl->dc, config_id,
                                            1, 1, surface_attribs);

    if (!gl_backend_wgl->sfc) {
        VIGS_LOG_CRITICAL("wglCreatePbufferARB failed");
        return false;
    }

    return true;
}

static bool vigs_gl_backend_wgl_create_context(struct vigs_gl_backend_wgl *gl_backend_wgl)
{
    gl_backend_wgl->ctx = gl_backend_wgl->wglCreateContext(gl_backend_wgl->dc);

    if (!gl_backend_wgl->ctx) {
        VIGS_LOG_CRITICAL("wglCreateContext failed");
        return false;
    }

    return true;
}

static bool vigs_gl_backend_wgl_has_current(struct vigs_gl_backend *gl_backend)
{
    struct vigs_gl_backend_wgl *gl_backend_wgl =
        (struct vigs_gl_backend_wgl*)gl_backend;

    return gl_backend_wgl->wglGetCurrentContext() != NULL;
}

static bool vigs_gl_backend_wgl_make_current(struct vigs_gl_backend *gl_backend,
                                             bool enable)
{
    struct vigs_gl_backend_wgl *gl_backend_wgl =
        (struct vigs_gl_backend_wgl*)gl_backend;
    HDC dc;

    if (enable) {
        dc = gl_backend_wgl->wglGetPbufferDCARB(gl_backend_wgl->sfc);

        if (!dc) {
            VIGS_LOG_CRITICAL("wglGetPbufferDCARB failed");
            return false;
        }

        if (!gl_backend_wgl->wglMakeContextCurrentARB(dc, dc, gl_backend_wgl->ctx)) {
            VIGS_LOG_CRITICAL("wglMakeContextCurrentARB failed");
            gl_backend_wgl->wglReleasePbufferDCARB(gl_backend_wgl->sfc, dc);
            return false;
        }
    } else {
        if (!gl_backend_wgl->wglMakeContextCurrentARB(NULL, NULL, NULL)) {
             VIGS_LOG_CRITICAL("wglMakeContextCurrentARB failed");
             return false;
         }
    }

    return true;
}

static void vigs_gl_backend_wgl_destroy(struct vigs_backend *backend)
{
    struct vigs_gl_backend_wgl *gl_backend_wgl = (struct vigs_gl_backend_wgl*)backend;
    HDC dc;

    vigs_gl_backend_cleanup(&gl_backend_wgl->base);

    gl_backend_wgl->wglDeleteContext(gl_backend_wgl->ctx);

    dc = gl_backend_wgl->wglGetPbufferDCARB(gl_backend_wgl->sfc);
    if (dc) {
        gl_backend_wgl->wglReleasePbufferDCARB(gl_backend_wgl->sfc, dc);
    }
    gl_backend_wgl->wglDestroyPbufferARB(gl_backend_wgl->sfc);

    ReleaseDC(gl_backend_wgl->win, gl_backend_wgl->dc);
    DestroyWindow(gl_backend_wgl->win);

    FreeLibrary(gl_backend_wgl->handle);

    vigs_backend_cleanup(&gl_backend_wgl->base.base);

    g_free(gl_backend_wgl);

    UnregisterClassA((LPCTSTR)VIGS_WGL_WIN_CLASS, NULL);

    VIGS_LOG_DEBUG("destroyed");
}

struct vigs_backend *vigs_gl_backend_create(void *display)
{
    WNDCLASSEXA vigs_win_class;
    HWND tmp_win = NULL;
    HDC tmp_dc = NULL;
    HGLRC tmp_ctx = NULL;
    int config_id = 0;
    PIXELFORMATDESCRIPTOR tmp_pixfmt = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE,
    };
    const char *ext_str = NULL;
    struct vigs_gl_backend_wgl *gl_backend_wgl = NULL;

    vigs_win_class.cbSize = sizeof(WNDCLASSEXA);
    vigs_win_class.style = 0;
    vigs_win_class.lpfnWndProc = &DefWindowProcA;
    vigs_win_class.cbClsExtra = 0;
    vigs_win_class.cbWndExtra = 0;
    vigs_win_class.hInstance = NULL;
    vigs_win_class.hIcon = NULL;
    vigs_win_class.hCursor = NULL;
    vigs_win_class.hbrBackground = NULL;
    vigs_win_class.lpszMenuName =  NULL;
    vigs_win_class.lpszClassName = VIGS_WGL_WIN_CLASS;
    vigs_win_class.hIconSm = NULL;

    if (!RegisterClassExA(&vigs_win_class)) {
        VIGS_LOG_CRITICAL("Unable to register window class");
        return NULL;
    }

    gl_backend_wgl = g_malloc0(sizeof(*gl_backend_wgl));

    vigs_backend_init(&gl_backend_wgl->base.base,
                      &gl_backend_wgl->base.ws_info.base);

    gl_backend_wgl->handle = LoadLibraryA("opengl32");

    if (!gl_backend_wgl->handle) {
        VIGS_LOG_CRITICAL("Unable to load opengl32.dll");
        goto fail;
    }

    VIGS_WGL_GET_PROC(PFNWGLCREATECONTEXTPROC, wglCreateContext, fail);
    VIGS_WGL_GET_PROC(PFNWGLDELETECONTEXTPROC, wglDeleteContext, fail);
    VIGS_WGL_GET_PROC(PFNWGLGETPROCADDRESSPROC, wglGetProcAddress, fail);
    VIGS_WGL_GET_PROC(PFNWGLMAKECURRENTPROC, wglMakeCurrent, fail);
    VIGS_WGL_GET_PROC(PFNWGLGETCURRENTCONTEXTPROC, wglGetCurrentContext, fail);

    tmp_win = CreateWindow(VIGS_WGL_WIN_CLASS, "VIGSWin",
                           WS_DISABLED | WS_POPUP,
                           0, 0, 1, 1, NULL, NULL, 0, 0);

    if (!tmp_win) {
        VIGS_LOG_CRITICAL("Unable to create window");
        goto fail;
    }

    tmp_dc = GetDC(tmp_win);

    if (!tmp_dc) {
        VIGS_LOG_CRITICAL("Unable to get window DC");
        goto fail;
    }

    config_id = ChoosePixelFormat(tmp_dc, &tmp_pixfmt);

    if (!config_id) {
        VIGS_LOG_CRITICAL("ChoosePixelFormat failed");
        goto fail;
    }

    if (!SetPixelFormat(tmp_dc, config_id, &tmp_pixfmt)) {
        VIGS_LOG_CRITICAL("SetPixelFormat failed");
        goto fail;
    }

    tmp_ctx = gl_backend_wgl->wglCreateContext(tmp_dc);
    if (!tmp_ctx) {
        VIGS_LOG_CRITICAL("wglCreateContext failed");
        goto fail;
    }

    if (!gl_backend_wgl->wglMakeCurrent(tmp_dc, tmp_ctx)) {
        VIGS_LOG_CRITICAL("wglMakeCurrent failed");
        goto fail;
    }

    /*
     * WGL extensions couldn't be queried by glGetString(), we need to use
     * wglGetExtensionsStringARB or wglGetExtensionsStringEXT for this, which
     * themselves are extensions
     */
    gl_backend_wgl->wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
        gl_backend_wgl->wglGetProcAddress((LPCSTR)"wglGetExtensionsStringARB");
    gl_backend_wgl->wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
        gl_backend_wgl->wglGetProcAddress((LPCSTR)"wglGetExtensionsStringEXT");

    if (gl_backend_wgl->wglGetExtensionsStringARB) {
        ext_str = gl_backend_wgl->wglGetExtensionsStringARB(tmp_dc);
    } else if (gl_backend_wgl->wglGetExtensionsStringEXT) {
        ext_str = gl_backend_wgl->wglGetExtensionsStringEXT();
    }

    if (!ext_str) {
        VIGS_LOG_CRITICAL("Unable to obtain WGL extension string");
        goto fail;
    }

    VIGS_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, PFNWGLCREATEPBUFFERARBPROC, wglCreatePbufferARB);
    VIGS_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, PFNWGLGETPBUFFERDCARBPROC, wglGetPbufferDCARB);
    VIGS_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, PFNWGLRELEASEPBUFFERDCARBPROC, wglReleasePbufferDCARB);
    VIGS_WGL_GET_EXT_PROC(WGL_ARB_pbuffer, PFNWGLDESTROYPBUFFERARBPROC, wglDestroyPbufferARB);
    VIGS_WGL_GET_EXT_PROC(WGL_ARB_pixel_format, PFNWGLCHOOSEPIXELFORMATARBPROC, wglChoosePixelFormatARB);
    VIGS_WGL_GET_EXT_PROC(WGL_ARB_make_current_read, PFNWGLMAKECONTEXTCURRENTARBPROC, wglMakeContextCurrentARB);

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

    gl_backend_wgl->wglMakeCurrent(NULL, NULL);
    gl_backend_wgl->wglDeleteContext(tmp_ctx);
    tmp_ctx = NULL;
    ReleaseDC(tmp_win, tmp_dc);
    tmp_dc = NULL;
    DestroyWindow(tmp_win);
    tmp_win = NULL;

    gl_backend_wgl->win = CreateWindow(VIGS_WGL_WIN_CLASS, "VIGSWin",
                                       WS_DISABLED | WS_POPUP,
                                       0, 0, 1, 1, NULL, NULL, 0, 0);

    if (!gl_backend_wgl->win) {
        VIGS_LOG_CRITICAL("Unable to create window");
        goto fail;
    }

    gl_backend_wgl->dc = GetDC(gl_backend_wgl->win);

    if (!gl_backend_wgl->dc) {
        VIGS_LOG_CRITICAL("Unable to get window DC");
        goto fail;
    }

    config_id = vigs_gl_backend_wgl_choose_config(gl_backend_wgl);

    if (!config_id) {
        goto fail;
    }

    if (!vigs_gl_backend_wgl_create_surface(gl_backend_wgl, config_id)) {
        goto fail;
    }

    if (!vigs_gl_backend_wgl_create_context(gl_backend_wgl)) {
        goto fail;
    }

    gl_backend_wgl->base.base.destroy = &vigs_gl_backend_wgl_destroy;
    gl_backend_wgl->base.has_current = &vigs_gl_backend_wgl_has_current;
    gl_backend_wgl->base.make_current = &vigs_gl_backend_wgl_make_current;
    gl_backend_wgl->base.ws_info.context = gl_backend_wgl->ctx;

    if (!vigs_gl_backend_init(&gl_backend_wgl->base)) {
        goto fail;
    }

    VIGS_LOG_DEBUG("created");

    return &gl_backend_wgl->base.base;

fail:
    if (gl_backend_wgl->ctx) {
        gl_backend_wgl->wglDeleteContext(gl_backend_wgl->ctx);
    }
    if (gl_backend_wgl->sfc) {
        HDC dc = gl_backend_wgl->wglGetPbufferDCARB(gl_backend_wgl->sfc);
        if (dc) {
            gl_backend_wgl->wglReleasePbufferDCARB(gl_backend_wgl->sfc, dc);
        }
        gl_backend_wgl->wglDestroyPbufferARB(gl_backend_wgl->sfc);
    }
    if (gl_backend_wgl->dc) {
        ReleaseDC(gl_backend_wgl->win, gl_backend_wgl->dc);
    }
    if (gl_backend_wgl->win) {
        DestroyWindow(gl_backend_wgl->win);
    }
    if (gl_backend_wgl->wglMakeCurrent && tmp_ctx) {
        gl_backend_wgl->wglMakeCurrent(NULL, NULL);
    }
    if (gl_backend_wgl->wglDeleteContext && tmp_ctx) {
        gl_backend_wgl->wglDeleteContext(tmp_ctx);
    }
    if (tmp_dc) {
        ReleaseDC(tmp_win, tmp_dc);
    }
    if (tmp_win) {
        DestroyWindow(tmp_win);
    }
    if (gl_backend_wgl->handle) {
        FreeLibrary(gl_backend_wgl->handle);
    }

    vigs_backend_cleanup(&gl_backend_wgl->base.base);

    g_free(gl_backend_wgl);

    UnregisterClassA((LPCTSTR)VIGS_WGL_WIN_CLASS, NULL);

    return NULL;
}
