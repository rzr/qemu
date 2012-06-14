/* Copyright (C) 2010  Nokia Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "common.h"
#include "degl.h"
#include "hwgl.h"

HWGL hwgl;

static void hwglPrintError(const char *fmt, ...)
{
	va_list args;
	char *err = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), 0, (LPTSTR)&err, 1, NULL);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, ": %s\n", err);
	LocalFree(err);
}

static LRESULT CALLBACK Dgles2HaxWinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

static void Dgles2HaxFlush(HWND hwnd)
{
	MSG msg;

	while(PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
	{
		if(msg.message == WM_QUIT)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

static BOOL (WINAPI *realMakeCurrent)(HDC hDC, HGLRC hRC);
static BOOL WINAPI forceMakeCurrent(HDC hDC, HGLRC hRC)
{
	Dprintf("wglMakeCurrent(%p, %p)\n", hDC, hRC);
	for(unsigned i = 0; i < 60; ++i)
	{
		if(!realMakeCurrent(hDC, hRC))
		{
			hwglPrintError("Attempt %d failed", i + 1);
			Sleep(1);
			continue;
		}
		return TRUE;
	}
	return FALSE;
}

static int hwglLoadExt()
{
#define HWGL_FUNC(ret, name, attr) \
	if((hwgl.name = (ret(WINAPI *)attr)hwgl.GetProcAddress("wgl"#name)) == NULL) \
	{ \
		fprintf(stderr, "ERROR: Function wgl" #name " not found!\n"); \
		return 0; \
	}
	HWGL_MANDATORY_EXT_FUNCS
#undef HWGL_FUNC
#define HWGL_FUNC(ret, name, attr) \
	if((hwgl.name = (ret(WINAPI *)attr)hwgl.GetProcAddress("wgl"#name)) == NULL) \
	{ \
		fprintf(stderr, "WARNING: Function wgl" #name " not found!\n"); \
	}
	HWGL_OPTIONAL_EXT_FUNCS
#undef HWGL_FUNC
	return 1;
}

static void hwglReleaseTemporaryPbufferContext(void)
{
	if(hwgl.pbuffer.haxctx)
	{
		hwgl.DeleteContext(hwgl.pbuffer.haxctx);
		hwgl.pbuffer.haxctx = 0;
	}
	if(hwgl.pbuffer.haxdc)
	{
		hwgl.ReleasePbufferDCARB(hwgl.pbuffer.haxbuf, hwgl.pbuffer.haxdc);
		hwgl.pbuffer.haxdc = 0;
	}
	if(hwgl.pbuffer.haxbuf)
	{
		hwgl.DestroyPbufferARB(hwgl.pbuffer.haxbuf);
		hwgl.pbuffer.haxbuf = 0;
	}
	hwgl.pbuffer.haxpf = 0;
}

static int hwglCreateTemporaryPbufferContext(void)
{
	hwglReleaseTemporaryPbufferContext();

	hwgl.pbuffer.haxpf = hwgl.window.haxpf;
	const int pattribs[] = { 0 };
	if(!(hwgl.pbuffer.haxbuf = hwgl.CreatePbufferARB(hwgl.window.haxdc, hwgl.pbuffer.haxpf, 1, 1, pattribs)))
	{
		hwglPrintError("wglCreatePbufferARB failed for temporary pbuffer context");
	}
	else
	{
		if (!(hwgl.pbuffer.haxdc = hwgl.GetPbufferDCARB(hwgl.pbuffer.haxbuf)))
		{
			hwglPrintError("wglGetPbufferDCARB failed for temporary pbuffer context");
		}
		else
		{
			if (!(hwgl.pbuffer.haxctx = hwgl.CreateContext(hwgl.pbuffer.haxdc)))
			{
				hwglPrintError("wglCreateContext failed for temporary pbuffer context");
			}
			else
			{
				return 1;
			}
		}
	}

	hwglReleaseTemporaryPbufferContext();
	return 0;
}

static int hwglCreateTemporaryContexts(int pfid)
{
	HINSTANCE hModuleInstance;
	if(!(hModuleInstance = GetModuleHandle(NULL)))
	{
		hwglPrintError("Failed to acquire module handle");
		goto failure;
	}

	// Create the hax window.
	if(!(hwgl.window.haxwnd = CreateWindowEx(0, hwgl.window.haxclass.lpszClassName, "Dgles2HaxWindow",
		WS_POPUP | WS_DISABLED, 0, 0, 10, 10, NULL, NULL, hModuleInstance, NULL)))
	{
		hwglPrintError("Failed to create hidden window");
		goto failure;
	}
	ShowWindow(hwgl.window.haxwnd, SW_HIDE);
	Dgles2HaxFlush(hwgl.window.haxwnd);

	if(!(hwgl.window.haxdc = GetDC(hwgl.window.haxwnd)))
	{
		hwglPrintError("Failed to get hidden window device context");
		goto window_failure;
	}

	if(pfid)
	{
		hwgl.window.haxpf = pfid;
		Dprintf("Using requested pixel format %d for temporary window context\n", hwgl.window.haxpf);
	}
	else
	{
		// Choose basic pixel format.
		static PIXELFORMATDESCRIPTOR const basicpfd =
		{
			.nSize      = sizeof(PIXELFORMATDESCRIPTOR),
			.nVersion   = 1,
			.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER_DONTCARE,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 32,
			.iLayerType = PFD_MAIN_PLANE,
		};

		if(!(hwgl.window.haxpf = ChoosePixelFormat(hwgl.window.haxdc, &basicpfd)))
		{
			hwglPrintError("No basic pixel format found for temporary window context");
			goto pf_failure;
		}
		Dprintf("Chose basic pixel format %d for use with window context.\n", hwgl.window.haxpf);
	}

	if(!DescribePixelFormat(hwgl.window.haxdc, hwgl.window.haxpf, sizeof(PIXELFORMATDESCRIPTOR), &hwgl.window.haxpfd))
	{
		hwglPrintError("Failed to describe pixel format %d", hwgl.window.haxpf);
		goto pf_failure;
	}
	Dprintf("Described pixel format %d.\n", hwgl.window.haxpf);

	if(!SetPixelFormat(hwgl.window.haxdc, hwgl.window.haxpf, &hwgl.window.haxpfd))
	{
		hwglPrintError("Cannot set pixel format %d for temporary window context", hwgl.window.haxpf);
		goto pf_failure;
	}
	Dprintf("Pixel format %d set for temporary window context\n", hwgl.window.haxpf);

	if(!(hwgl.window.haxctx = hwgl.CreateContext(hwgl.window.haxdc)))
	{
		hwglPrintError("Temporary context creation failed");
		goto pf_failure;
	}
	Dprintf("Temporary window context successfully created\n");

	if(!hwgl.MakeCurrent(hwgl.window.haxdc, hwgl.window.haxctx))
	{
		hwglPrintError("Failed to make temporary context current");
		goto context_failure;
	}
	Dprintf("Temporary window context successfully activated\n");

	if(!hwglLoadExt())
	{
		hwglPrintError("Cannot load WGL extension functions");
		goto function_failure;
	}

	if (!hwglCreateTemporaryPbufferContext())
	{
		goto function_failure;
	}

	return hwgl.window.haxpf;
function_failure:
	hwgl.MakeCurrent(0, 0);
context_failure:
	hwgl.DeleteContext(hwgl.window.haxctx);
	hwgl.window.haxctx = 0;
pf_failure:
	hwgl.window.haxpf = 0;
	ReleaseDC(hwgl.window.haxwnd, hwgl.window.haxdc);
	hwgl.window.haxdc = 0;
window_failure:
	DestroyWindow(hwgl.window.haxwnd);
failure:
	return 0;
}

int hwglLoad()
{
	hwgl.pbuffer.haxbuf = 0;
	hwgl.pbuffer.haxdc = 0;
	hwgl.pbuffer.haxctx = 0;
	hwgl.pbuffer.haxpf = 0;
	Dprintf("Loading WGL functions...\n");

#define HWGL_FUNC(ret, name, attr) \
	if((hwgl.name = deglGetHostProcAddress("wgl"#name)) == NULL) \
	{ \
		fprintf(stderr, "Function wgl" #name " not found!\n"); \
		return 0; \
	}
	HWGL_FUNCS
#undef HWGL_FUNC

	// Needed to overcome NVidia bug existing for many years
	realMakeCurrent = hwgl.MakeCurrent;
	hwgl.MakeCurrent = forceMakeCurrent;

	HINSTANCE hModuleInstance;
	if(!(hModuleInstance = GetModuleHandle(NULL)))
	{
		hwglPrintError("Failed to acquire module handle");
		goto failure;
	}
	
	// Register hax window class.
	hwgl.window.haxclass.cbSize = sizeof(WNDCLASSEX);
	hwgl.window.haxclass.style = CS_OWNDC;
	hwgl.window.haxclass.lpfnWndProc = Dgles2HaxWinProc;
	hwgl.window.haxclass.cbClsExtra = 0;
	hwgl.window.haxclass.cbWndExtra = 0;
	hwgl.window.haxclass.hInstance = hModuleInstance;
	hwgl.window.haxclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	hwgl.window.haxclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	hwgl.window.haxclass.hbrBackground = NULL;
	hwgl.window.haxclass.lpszMenuName = NULL;
	hwgl.window.haxclass.lpszClassName = "Dgles2HaxClass";
	hwgl.window.haxclass.hIconSm = NULL;

	if(!RegisterClassEx(&hwgl.window.haxclass))
	{
		hwglPrintError("Failed to register hidden window class");
		goto failure;
	}

	int haxpf;
	if(!(haxpf = hwglCreateTemporaryContexts(0)))
	{
		goto context_failure;
	}

	Dprintf("Loading WGL extension functions...\n");
	if(!hwglLoadExt())
	{
		goto context_failure;
	}

	// Choose more detailed pixel format if extension is available.
	if(!hwgl.ChoosePixelFormatARB)
	{
		goto function_success;
	}

	int const haxiattribs[] =
	{
		WGL_PIXEL_TYPE_ARB,      WGL_TYPE_RGBA_ARB,
		WGL_DRAW_TO_WINDOW_ARB,  GL_TRUE,
		WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
		WGL_COLOR_BITS_ARB,      32,
		WGL_RED_BITS_ARB,        8,
		WGL_GREEN_BITS_ARB,      8,
		WGL_BLUE_BITS_ARB,       8,
		WGL_ALPHA_BITS_ARB,      8,

		WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
		WGL_ACCELERATION_ARB,    WGL_FULL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB,   GL_FALSE,
		0,
	};

	float const haxfattribs[] = { 0 };
	int newhaxpf = 0;
	UINT num_formats;

	if(!(hwgl.ChoosePixelFormatARB(hwgl.window.haxdc, haxiattribs,
		haxfattribs, 1, &newhaxpf, &num_formats) && newhaxpf && num_formats))
	{
		hwglPrintError("WARNING: Couldn't use pixel format wglChoosePixelFormatARB");
		hwgl.ChoosePixelFormatARB = 0;
		goto function_success;
	}

	Dprintf("Chose new pixel format %d for temporary context.\n", newhaxpf);
	
	hwgl.MakeCurrent(NULL, NULL);
	hwgl.DeleteContext(hwgl.window.haxctx);
	hwgl.window.haxctx = 0;
	ReleaseDC(hwgl.window.haxwnd, hwgl.window.haxdc);
	hwgl.window.haxdc = 0;
	DestroyWindow(hwgl.window.haxwnd);

	if(!hwglCreateTemporaryContexts(newhaxpf))
	{
		hwglPrintError("WARNING: Couldn't use pixel format from wglChoosePixelFormatARB");
		hwgl.ChoosePixelFormatARB = 0;

		Dprintf("Recreating basic context as fallback.\n");
		if(!hwglCreateTemporaryContexts(haxpf))
		{
			hwglPrintError("Cannot reactivate basic temporary context");
			goto context_failure;
		}
	}
function_success:
	hwgl.MakeCurrent(NULL, NULL);
	pthread_mutex_init(&hwgl.haxlock, 0);
	return 1;

function_failure:
	hwgl.MakeCurrent(NULL, NULL);
	hwgl.DeleteContext(hwgl.window.haxctx);
	ReleaseDC(hwgl.window.haxwnd, hwgl.window.haxdc);
context_failure:
	UnregisterClass(hwgl.window.haxclass.lpszClassName, hModuleInstance);
	hwglReleaseTemporaryPbufferContext();
failure:
	return 0;
}
