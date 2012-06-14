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

#include <stdlib.h>
#ifdef _WIN32
#	include <windows.h>
#else
#	include <dlfcn.h>
#endif // !def _WIN32
#include <string.h>
#include <pthread.h>

#include "degl.h"
#include "context.h"
#if(CONFIG_OSMESA == 1)
#	include "hosmesa.h"
#endif // (CONFIG_OSMESA == 1)
#if(CONFIG_GLX == 1)
#	include "hglx.h"
#endif // (CONFIG_GLX == 1)
#if(CONFIG_WGL == 1)
#	include "hwgl.h"
#endif // (CONFIG_WGL == 1)
#if(CONFIG_COCOA == 1)
#   include "hcocoa.h"
#endif // (CONFIG_COCOA == 1)

#if !(defined __APPLE__) && defined(__STRICT_ANSI__) && !defined(__MINGW64__)
#ifdef _WIN32
_CRTIMP __cdecl int putenv(const char *);
#else
extern int putenv(char *);
#endif
#endif

static int degl_initialized = 0;
static char* degl_hostlib;
static pthread_key_t degl_tls_key;
static pthread_once_t degl_tls_init_control = PTHREAD_ONCE_INIT;

DGLES2_EXPORT DEGLBackend degl_backend = DEGLBackend_invalid;
DGLES2_EXPORT DEGLFrontend degl_frontend = DEGLFrontend_invalid;
DGLES2_EXPORT int degl_no_alpha;
#if(CONFIG_GLX == 1)
DGLES2_EXPORT int degl_glx_direct;
#endif

// The dynamic library handle.
#if(CONFIG_STATIC == 1)
DGLES2_EXPORT
#else
static
#endif // CONFIG_STATIC == 1
void *degl_handle = 0;

#ifdef _WIN32
#define RTLD_LAZY  0x01
#define RTLD_LOCAL 0x02

#if(CONFIG_OSMESA == 1)
static void *degl_handle2 = 0;
static void *(GL_APIENTRY *degl_osmesa_getprocaddress)(const char *) = 0;
static void *(GL_APIENTRY *degl_wgl_getprocaddress)(const char *) = 0;
#if defined(__STRICT_ANSI__) && !defined(__MINGW64__)
_CRTIMP int __cdecl _stricmp (const char*, const char*);
#endif
#endif

void* dlopen(char const* name, unsigned flags)
{
#	if (CONFIG_OSMESA==1)
	if(degl_backend == DEGLBackend_osmesa && !_stricmp(name, "osmesa/osmesa.dll"))
	{
		void *lib = NULL;

		if((degl_handle2 = LoadLibrary("osmesa/opengl32.dll")))
		{
			Dprintf("opengl32.dll loaded at %p.\n", degl_handle2);
			lib = LoadLibrary(name);
		}
		else
		{
			fprintf(stderr, "%s: cannot load opengl32.dll\n", __FUNCTION__);
		}

		if(!lib)
		{
			char *msg = NULL;
			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL, GetLastError(), 0,
				(LPTSTR)&msg, 1, NULL);
			fprintf(stderr, "%s: system error message: %s\n", __FUNCTION__, msg);
			LocalFree(msg);
		}
		else
		{
			degl_osmesa_getprocaddress = (void *(GL_APIENTRY *)(const char *))
				GetProcAddress(lib, "OSMesaGetProcAddress");
			if(!degl_osmesa_getprocaddress)
			{
				fprintf(stderr, "%s: cannot locate OSMesaGetProcAddress function!\n",
					__FUNCTION__);
			}

			degl_wgl_getprocaddress = (void *(GL_APIENTRY *)(const char *))
				GetProcAddress(degl_handle2, "wglGetProcAddress");
			if(!degl_wgl_getprocaddress)
			{
				fprintf(stderr, "%s: cannot locate wglGetProcAddress function!\n",
					__FUNCTION__);
			}
		}
		return lib;
	}
#	endif
	return LoadLibrary(name);
}

void* dlsym(void* handle, char const* proc)
{
#	if(CONFIG_OSMESA == 1)
	if(degl_backend == DEGLBackend_osmesa && handle == degl_handle)
	{
		void *f = NULL;

		if(degl_osmesa_getprocaddress)
			f = degl_osmesa_getprocaddress(proc);

		if(!f && degl_wgl_getprocaddress)
			f = degl_wgl_getprocaddress(proc);

		if(!f)
			f = GetProcAddress(degl_handle2, proc);
		if(!f)
			f = GetProcAddress(handle, proc);
		return f;
	}
#	endif
	return GetProcAddress(handle, proc); 
}

void dlclose(void* handle)
{
#	if (CONFIG_OSMESA==1)
	if(degl_backend == DEGLBackend_osmesa && handle == degl_handle)
	{
		FreeLibrary(degl_handle2);
	}
#	endif
	FreeLibrary(handle);
}

#endif // def _WIN32

static char* mystrdup(char const* str)
{
	if(!str) return NULL;
	unsigned nbytes = strlen(str) + 1;
	char* copy = malloc(nbytes);
	memcpy(copy, str, nbytes);
	return copy;
}

// Open the backend renderer library.
static int deglOpenHostLibrary(char const* hostlib)
{
	if(!strlen(hostlib) || !strcmp(hostlib, "self"))
		degl_hostlib = NULL;
	else
		degl_hostlib = mystrdup(hostlib);

	if(!degl_hostlib)
#	if(CONFIG_STATIC == 1)
	{
		degl_handle = (void*)-1;
	}
	else
#	else
	{
		fprintf(stderr, "ERROR: No library to open specified (and no static library)!\n");
		return 0;
	}
#	endif
	if(!(degl_handle = dlopen(degl_hostlib, RTLD_LAZY | RTLD_LOCAL)))
	{
		fprintf(stderr, "ERROR: couldn't open library \'%s\'!\n",
			degl_hostlib ? degl_hostlib : "self");
		return 0;
	}

	Dprintf("Library \'%s\' opened at %p.\n", degl_hostlib ? degl_hostlib : "self", degl_handle);
	return 1;
}

// Get address of a host renderer function.
DGLES2_EXPORT void* EGLAPIENTRY deglGetHostProcAddress(char const* proc)
{
#if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		Dprintf("dummy lookup for \'%s\'\n", proc);
		return NULL;
	}
#endif // CONFIG_COCOA == 1
	void *result = dlsym(degl_handle, proc);
#if(CONFIG_GLX == 1)
	if(result == NULL && strstr(proc, "glX"))
	{
		typedef void * (*glXGetProcAddressARBPtr)(const char *);
		glXGetProcAddressARBPtr glXGetProcAddressARB = (glXGetProcAddressARBPtr)dlsym(degl_handle, "glXGetProcAddressARB");
		if (glXGetProcAddressARB != NULL)
		{
			result = glXGetProcAddressARB(proc);
		}
	}
#endif // CONFIG_GLX == 1
	return result;
}

DGLES2_EXPORT void* EGLAPIENTRY dglGetHostProcAddress(char const* proc)
{
#if(CONFIG_WGL == 1)
	if(degl_backend == DEGLBackend_wgl)
	{
		void* addr = hwgl.GetProcAddress(proc);
		if(!addr)
			addr = dlsym(degl_handle, proc);
		return addr;
	}
#endif // CONFIG_WGL == 1
#if(CONFIG_COCOA == 1)
	if(degl_backend == DEGLBackend_cocoa)
	{
		return dlsym(RTLD_NEXT, proc);
	}
#endif // CONFIG_COCOA == 1
	return dlsym(degl_handle, proc);
}

// Load our own GLES 2.0 or 1.0 library dynamically,to communicate with.
void deglActivateClientAPI(DEGLContext *context)
{
	char const* libname;
	
	if(!context->clientlib)
	{
		if(context->version == 2)
		{
			libname =
#			if(defined _WIN32)
				"GLESv2.dll";
#				elif(defined __APPLE__)
				"libGLESv2.dylib";
#				else
				"libGLESv2.so";
#				endif // !def _WIN32
		}
		else
		{
			libname =
#				if(defined _WIN32)
				"GLES_CM.dll";
#				elif(defined __APPLE__)
				"libGLES_CM.dylib";
#				else
				"libGLES_CM.so";
#				endif // !def _WIN32
		}

#		if(CONFIG_STATIC == 1)
		context->clientlib = (void*)-1;
#		else
		if(!(context->clientlib = dlopen(libname, RTLD_LAZY | RTLD_LOCAL)))
		{
			fprintf(stderr, "ERROR: Couldn't load GLES%d library (%s)!\n",
				(context->version == 2) ? 2 : 1, libname);
			exit(0);
		}
#		endif // CONFIG_STATIC != 1
	}

	void* (*dglActivateContextFunc)(int version, void* clientdata);

#	if(CONFIG_STATIC == 1)
	extern void* dglActivateContext(int version, void* clientdata);
	dglActivateContextFunc = &dglActivateContext;
	context->glesfuncs->dglReadPixels = &dglReadPixels;
	context->glesfuncs->glFlush = &glFlush;
	context->glesfuncs->glFinish = &glFinish;
	context->glesfuncs->dglBindTexImage = &dglBindTexImage;
#	if(CONFIG_OFFSCREEN == 1)
	context->glesfuncs->dglDeletePBO = &dglDeletePBO;
#	endif // CONFIG_OFFSCREEN == 1
	deglGetTLS()->vertex_free = &dglVertexFinish;
#	else
	dglActivateContextFunc = dlsym(context->clientlib, "dglActivateContext");
	context->glesfuncs->dglReadPixels = dlsym(context->clientlib,"dglReadPixels");
	context->glesfuncs->glFlush = dlsym(context->clientlib,"glFlush");
	context->glesfuncs->glFinish = dlsym(context->clientlib,"glFinish");
	context->glesfuncs->dglBindTexImage =  dlsym(context->clientlib,"dglBindTexImage");
#	if(CONFIG_OFFSCREEN == 1)
	context->glesfuncs->dglDeletePBO = dlsym(context->clientlib, "dglDeletePBO");
#	endif // CONFIG_OFFSCREEN == 1
	deglGetTLS()->vertex_free = dlsym(context->clientlib, "dglVertexFinish");
#	endif // CONFIG_STATIC == 1
	context->clientdata = dglActivateContextFunc(context->version, context->clientdata);
	if(!context->clientdata)
	{
		fprintf(stderr, "ERROR: Not a valid DGLES2 Client API library!\n");
		exit(0);
	}
}

void deglDeactivateClientAPI(DEGLContext *context)
{
	if(context)
	{
		if(context->clientdata)
		{
			free(context->clientdata);
			context->clientdata = NULL;
		}
#		if(CONFIG_STATIC != 1)
		if(context->clientlib)
		{
			dlclose(context->clientlib);
			context->clientlib = NULL;
		}
#		endif // CONFIG_STATIC != 1
	}
}

static void degl_tls_free(void *tls_ptr)
{
	degl_tls *tls = tls_ptr;
	if(tls->shader_strtok.buffer)
	{
		free(tls->shader_strtok.buffer);
	}
	if(tls->vertex_arrays && tls->vertex_free)
	{
		tls->vertex_free();
	}
#	if(CACHE_ELEMENT_BUFFERS == 1)
	degl_ebo *e = tls->ebo.list;
	while(e)
	{
		if(e->cache)
		{
			free(e->cache);
		}
		degl_ebo *n = e->next;
		free(e);
		e = n;
	}
#	endif // CACHE_ELEMENT_BUFFERS == 1
	if(tls->hgl)
	{
		free(tls->hgl);
	}
	if(tls->extensions_string)
	{
		free(tls->extensions_string);
	}
	deglFreeVBOList(tls);
#	if(CONFIG_OFFSCREEN == 1)
	if(tls->reserved_buffers)
	{
		degl_internal_bo *b = tls->reserved_buffers;
		while(b)
		{
			degl_internal_bo *n = b->next;
			free(b);
			b = n;
		}
	}
#	endif // CONFIG_OFFSCREEN
	memset(tls, 0, sizeof(*tls));
	free(tls);
}

static void degl_tls_init(void)
{
	Dprintf("creating TLS key\n");
	pthread_key_create(&degl_tls_key, degl_tls_free);
}

DGLES2_EXPORT degl_tls * EGLAPIENTRY deglGetTLS(void)
{
	pthread_once(&degl_tls_init_control, degl_tls_init);
	degl_tls *tls = pthread_getspecific(degl_tls_key);
	if(!tls)
	{
		Dprintf("initializing TLS for current thread\n");
		tls = malloc(sizeof(*tls));
		memset(tls, 0, sizeof(*tls));
		tls->error = EGL_SUCCESS;
		tls->current_context = EGL_NO_CONTEXT;
		tls->extensions_string = 0;
		tls->vbo_list = malloc(sizeof(degl_vbo));
		tls->vbo_list->name = 0;
		tls->vbo_list->next = NULL;
		pthread_setspecific(degl_tls_key, tls);
	}
	return tls;
}

void deglFreeTLS(void)
{
	pthread_once(&degl_tls_init_control, degl_tls_init);
	degl_tls *tls = pthread_getspecific(degl_tls_key);
	if(tls)
	{
		Dprintf("freeing TLS for current thread\n");
		degl_tls_free(tls);
		pthread_setspecific(degl_tls_key, NULL);
	}
}

// Library initalizer.
void deglInit(void)
{
	if(degl_initialized)
		return;

	Dprintf("DEGL initialization!\n");

	degl_no_alpha = 0;
#	if(CONFIG_GLX == 1)
	degl_glx_direct = 1;
#	endif

	char* backend = mystrdup(getenv("DGLES2_BACKEND"));
	char* frontend = mystrdup(getenv("DGLES2_FRONTEND"));
	char* hostlib = mystrdup(getenv("DGLES2_LIB"));
	char *env_value = getenv("DGLES2_NO_ALPHA");
	if(env_value) degl_no_alpha = atoi(env_value);
#	if(CONFIG_GLX == 1)
	if((env_value = getenv("DGLES2_GLX_DIRECT"))) degl_glx_direct = atoi(env_value);
#	endif

	static struct
	{
		char const* name;
		DEGLFrontend value;
	}
	const frontends[] =
	{
		{
			"x11",
#			if(CONFIG_GLX == 1)
			DEGLFrontend_x11
#			else // (CONFIG_GLX == 1)
			(DEGLFrontend)-1
#			endif // !(CONFIG_GLX == 1)
		},

		{
			"cocoa",
#			if(CONFIG_COCOA==1)
			DEGLFrontend_cocoa,
#			else
			(DEGLFrontend)-1
#			endif
		},

		{
			"offscreen",
#			if(CONFIG_OFFSCREEN == 1)
			DEGLFrontend_offscreen
#			else // (CONFIG_OFFSCREEN == 1)
			(DEGLBackend)-1
#			endif // !(CONFIG_OFFSCREEN == 1)
		},
	};

	if(frontend)
	{
		unsigned f;
		for(f = sizeof(frontends)/sizeof(frontends[0]); f--;)
			if(!strcmp(frontends[f].name, frontend))
				break;

		if(f == (unsigned)-1)
		{
			fprintf(stderr, "ERROR: unknown frontend \'%s\'!\n",
				frontend);
			exit(1);
		}

		if(frontends[f].value == (DEGLFrontend)-1)
		{
			fprintf(stderr, "ERROR: unsupported frontend \'%s\'!\n",
				frontend);
			exit(1);
		}

		degl_frontend = frontends[f].value;

		Dprintf("Initializing frontend \'%s\' (%d).\n",
			frontends[f].name, degl_frontend);
	}
	else
	{
#		if(CONFIG_GLX == 1)
		degl_frontend = DEGLFrontend_x11;
		Dprintf("Initializing default frontend: x11\n");
#		else
#		if(CONFIG_COCOA == 1)
		degl_frontend = DEGLFrontend_cocoa;
		Dprintf("Initializing default frontend: cocoa\n");
#		else
#		if(CONFIG_OFFSCREEN == 1)
		degl_frontend = DEGLFrontend_offscreen;
		Dprintf("Initializing default frontend: offscreen\n");
#		else
#		error No frontends enabled!
#		endif // CONFIG_OFFSCREEN == 1
#		endif // CONFIG_COCOA == 1
#		endif // CONFIG_GLX == 1
	}

	static struct
	{
		char const* name;
		DEGLBackend value;
	}
	const backends[] =
	{
		{
			"osmesa",
#			if(CONFIG_OSMESA == 1)
			DEGLBackend_osmesa
#			else // (CONFIG_OSMESA == 1)
			(DEGLBackend)-1
#			endif // !(CONFIG_OSMESA == 1)
		},

		{
			"glx",
#			if(CONFIG_GLX == 1)
			DEGLBackend_glx
#			else // (CONFIG_GLX == 1)
			(DEGLBackend)-1
#			endif // !(CONFIG_GLX == 1)
		},
		{
			"wgl",
#			if(CONFIG_WGL == 1)
			DEGLBackend_wgl
#			else // (CONFIG_WGL == 1)
			(DEGLBackend)-1
#			endif // !(CONFIG_WGL == 1)
		},
		{
			"cocoa",
#			if(CONFIG_COCOA == 1)
			DEGLBackend_cocoa
#			else // (CONFIG_COCOA == 1)
			(DEGLBackend)-1
#			endif // !(CONFIG_COCOA == 1)
		},
	};

#	if(CONFIG_GLX == 1)
	if(degl_frontend == DEGLFrontend_x11)
	{
		degl_backend = DEGLBackend_glx;
		Dprintf("x11 frontend selected, initializing glx backend\n");
	}
	else
#	endif // CONFIG_GLX == 1
#	if(CONFIG_COCOA == 1)
	if(degl_frontend == DEGLFrontend_cocoa)
	{
		degl_backend = DEGLBackend_cocoa;
		Dprintf("cocoa frontend selected, initializing cocoa backend\n");
	}
	else
#	endif // CONFIG_COCOA == 1
	if(backend)
	{
		unsigned b;
		for(b = sizeof(backends)/sizeof(backends[0]); b--;)
			if(!strcmp(backends[b].name, backend))
				break;

		if(b == (unsigned)-1)
		{
			fprintf(stderr, "ERROR: unknown backend \'%s\'!\n",
				backend);
			exit(1);
		}

		if(backends[b].value == (DEGLBackend)-1)
		{
			fprintf(stderr, "ERROR: unsupported backend \'%s\'!\n",
				backend);
			exit(1);
		}

		degl_backend = backends[b].value;

		Dprintf("Initializing backend \'%s\' (%d).\n",
			backends[b].name, degl_backend);
	}
	else
	{
#		if (CONFIG_GLX == 1)
		degl_backend = DEGLBackend_glx;
		Dprintf("Initializing default backend: glx\n");
#		elif (CONFIG_WGL == 1)
		degl_backend = DEGLBackend_wgl;
		Dprintf("Initializing default backend: wgl\n");
#		elif (CONFIG_COCOA == 1)
		degl_backend = DEGLBackend_cocoa;
		Dprintf("Initializing default backend: cocoa\n");
#		endif
//		temporary: move this inside the above ifdef once autodetection suffices
#		if(CONFIG_OSMESA == 1)
		Dprintf("Initializing default backend: osmesa\n");
		degl_backend = DEGLBackend_osmesa;
#		endif
		unsigned b;
		for(b = sizeof(backends)/sizeof(backends[0]); b--;)
			if(backends[b].value == degl_backend)
			{
				backend = mystrdup(backends[b].name);
				break;
			}
	}

#	if(CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1)
	int retry = 0;
	do
	{
		if(retry && degl_frontend == DEGLFrontend_offscreen)
		{
			if(degl_backend == DEGLBackend_osmesa)
			{
				fprintf(stderr, "ERROR: invalid osmesa backend found\n");
				exit(1);
			}
			fprintf(stderr, "ERROR: primary backend failed, trying osmesa...\n");
			if(degl_handle && degl_handle != (void *)-1)
			{
				dlclose(degl_handle);
				degl_handle = (void *)-1;
			}
			free(degl_hostlib);
			degl_hostlib = NULL;
			free(hostlib);
			hostlib = NULL;
			free(backend);
			degl_backend = DEGLBackend_osmesa;
			unsigned b;
			for(b = sizeof(backends)/sizeof(backends[0]); b--;)
				if(backends[b].value == degl_backend)
				{
					backend = mystrdup(backends[b].name);
					break;
				}
		}
#	endif // CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1
		switch(degl_backend)
		{
#			if(CONFIG_OSMESA == 1)
			case DEGLBackend_osmesa:
			{
				static char const* libname =
#				if(CONFIG_STATIC == 1)
				"self";
#				elif defined _WIN32
				"osmesa/osmesa.dll";
#				elif defined __APPLE__
				"libOSMesa.dylib";			
#				else
				"libOSMesa.so";
#				endif // !def _WIN32
				if(!deglOpenHostLibrary(hostlib ? hostlib : libname) ||
					!hOSMesaLoad())
				{
					exit(1);
				}
#				if(CONFIG_OFFSCREEN == 1)
				retry = 0;
#				endif
				break;
			}
#			endif // (CONFIG_OSMESA == 1)
#			if(CONFIG_GLX == 1)
			case DEGLBackend_glx:
			{
				static char const* libname = "libGL.so.1";

				if(!deglOpenHostLibrary(hostlib ? hostlib : libname) ||
					!hglXLoad())
				{
#					if(CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1)
					retry = 1;
#					else
					exit(1);
#					endif
				}
				break;
			}
#			endif // (CONFIG_GLX == 1)
#			if(CONFIG_WGL == 1)
			case DEGLBackend_wgl:
			{
				static char const* libname = "opengl32.dll";

				if(!deglOpenHostLibrary(hostlib ? hostlib : libname) ||
					!hwglLoad())
				{
#					if (CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1)
					retry = 1;
#					else
					exit(1);
#					endif
				}
				break;
			}
#			endif // (CONFIG_GLX == 1)
#			if(CONFIG_COCOA == 1)
			case DEGLBackend_cocoa:
			{
				if(!hcocoaLoad())
				{
#					if(CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1)
					retry = 1;
#					else
					exit(1);
#					endif
				}
			}
#			endif // (CONFIG_COCOA == 1)
			default:
				break;
		}
#	if (CONFIG_OSMESA == 1 && CONFIG_OFFSCREEN == 1)
	} while(retry);
#	endif

	// publish chosen backend. use putenv instead of setenv as the latter
	// is missing in mingw.
	if(backend)
	{
		char *e = malloc(40);
		if(e && sprintf(e, "DGLES2_BACKEND=%.16s", backend) > 0)
			putenv(e);
	}

	free(backend);
	free(frontend);
	free(hostlib);

	degl_initialized = 1;
	Dprintf("Successfully initialized!\n");
}

DGLES2_EXPORT degl_vbo* deglFindVBO(GLuint name)
{
	if(!name) return 0;
	degl_tls* tls = deglGetTLS();
	degl_vbo* curr = tls->vbo_list->next;

	while(curr)
	{
		if(curr->name == name)
		{
			Dprintf("vbo %d was found in the list\n",name);
			return curr;
		}
		curr = curr->next;
	}
	Dprintf("vbo %d was NOT found in the list\n",name);
	return NULL;
}

DGLES2_EXPORT int deglAddVBOToList(GLuint name, GLenum from)
{
	degl_tls* tls = deglGetTLS();
	degl_vbo* vbo = malloc(sizeof(degl_vbo));
	vbo->name = name;
	vbo->from = from;
	vbo->to = GL_FLOAT;//hardcoded is enough for now
	vbo->next = NULL;
	degl_vbo* curr = tls->vbo_list;
	while(curr->next)
	{
		curr = curr->next;
	}
	curr->next = vbo;
	Dprintf("vbo %d was added to list\n", name);
	return 1;
}

DGLES2_EXPORT int deglRemoveVBOFromList(GLuint name)
{
	if(!name) return 0;
	degl_tls* tls = deglGetTLS();
	degl_vbo* prev, *curr;
	prev = tls->vbo_list;
	curr = prev->next;
	while(curr)
	{
		if(curr->name == name)
		{
			prev->next = curr->next;
			free(curr);
			Dprintf("vbo %d was deleted from list\n", name);
			return 1;
		}
		prev = curr;
		curr = curr->next;
	}
	//Dprintf("vbo %d was NOT found in list\n", name);
	return 0;
}

DGLES2_EXPORT int deglFreeVBOList(degl_tls* tls)
{
	degl_vbo * curr, * next;
	curr = tls->vbo_list->next;

	while(curr)
	{
		next = curr->next;
		Dprintf("vbo %d was deleted from list\n", curr->name);
		free(curr);
		curr = next;
	}
	free(tls->vbo_list);
	return 1;
}

DGLES2_EXPORT int deglUnbindVBOFromArrays(GLuint name)
{
	if(!name) return 0;
	int count = 0;
	degl_tls* tls = deglGetTLS();
	for(int i = 0; i < tls->max_vertex_arrays; ++i)
	{
		DGLVertexArray*const va = tls->vertex_arrays + i;
		if(va->buffer == name)
		{
			va->buffer = 0;
			count++;
			Dprintf("vbo %d unbinded from array %d\n", name, i);
		}
	}
	return count;
}

#if(CONFIG_OFFSCREEN == 1)
DGLES2_EXPORT void deglAddReservedBuffer(GLuint name)
{
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		degl_tls *tls = deglGetTLS();
		degl_internal_bo *b = malloc(sizeof(*b));
		if(!b)
		{
			Dprintf("out of memory!\n");
		}
		else
		{
			memset(b, 0, sizeof(*b));
			b->next = tls->reserved_buffers;
			b->name = name;
			b->surface = eglGetCurrentSurface(EGL_READ);
			if(!b->surface)
			{
				fprintf(stderr, "%s: ERROR: unable to set surface for "
					"reserved buffer %u!\n", __FUNCTION__, name);
			}    
			tls->reserved_buffers = b;
		}
	}
	else
	{
		Dprintf("unexpectedly created reserved buffer object %u!\n", name);
	}
}

DGLES2_EXPORT void deglRemoveReservedBuffer(GLuint name)
{
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		degl_tls *tls = deglGetTLS();
		degl_internal_bo *b = tls->reserved_buffers, *pb = NULL;
		while(b)
		{
			degl_internal_bo *n = b->next;
			if(b->name == name)
			{
				free(b);
				if(pb)
				{
					pb->next = n;
				}
				else
				{
					tls->reserved_buffers = n;
				}
			}
			else
			{
				pb = b;
			}
			b = n;
		}
	}
	else
	{
		Dprintf("unexpectedly removed reserved buffer object %u!\n", name);
	}
}

DGLES2_EXPORT void deglReplaceReservedBuffer(GLuint old_name, GLuint new_name)
{
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		degl_tls *tls = deglGetTLS();
		degl_internal_bo *b = tls->reserved_buffers;
		for(; b; b = b->next)
		{
			if(b->name == old_name)
			{
				b->name = new_name;
				if(!b->surface)
				{
					fprintf(stderr, "%s: ERROR: reserved buffer %u has no surface!\n",
						__FUNCTION__, b->name);
				}
				else
				{
					((DEGLSurface *)(b->surface))->pbo = new_name;
				}
			}
		}
	}
	else
	{
		Dprintf("unexpectedly replacing reserved buffer %u with %u!\n",
			old_name, new_name);
	}
}

DGLES2_EXPORT int deglIsReservedBuffer(GLuint name)
{
	if(degl_frontend == DEGLFrontend_offscreen)
	{
		degl_internal_bo *b = deglGetTLS()->reserved_buffers;
		for(; b; b = b->next)
		{
			if(b->name == name)
			{
				return 1;
			}
		}
	}
	return 0;
}
#endif // CONFIG_OFFSCREEN == 1
