#ifndef _QEMU_YAGL_GLES_OGL_MACROS_H
#define _QEMU_YAGL_GLES_OGL_MACROS_H

/*
 * GLES OpenGL prototyping macros.
 * @{
 */

#define YAGL_GLES_OGL_PROC0(func) \
void (*func)(void);

#define YAGL_GLES_OGL_PROC_RET0(ret_type, func) \
ret_type (*func)(void);

#define YAGL_GLES_OGL_PROC1(func, arg0_type, arg0) \
void (*func)(arg0_type arg0);

#define YAGL_GLES_OGL_PROC_RET1(ret_type, func, arg0_type, arg0) \
ret_type (*func)(arg0_type arg0);

#define YAGL_GLES_OGL_PROC2(func, arg0_type, arg1_type, arg0, arg1) \
void (*func)(arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_OGL_PROC_RET2(ret_type, func, arg0_type, arg1_type, arg0, arg1) \
ret_type (*func)(arg0_type arg0, arg1_type arg1);

#define YAGL_GLES_OGL_PROC3(func, arg0_type, arg1_type, arg2_type, arg0, arg1, arg2) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2);

#define YAGL_GLES_OGL_PROC4(func, arg0_type, arg1_type, arg2_type, arg3_type, arg0, arg1, arg2, arg3) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3);

#define YAGL_GLES_OGL_PROC5(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg0, arg1, arg2, arg3, arg4) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4);

#define YAGL_GLES_OGL_PROC6(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg0, arg1, arg2, arg3, arg4, arg5) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5);

#define YAGL_GLES_OGL_PROC7(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6);

#define YAGL_GLES_OGL_PROC8(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7);

#define YAGL_GLES_OGL_PROC9(func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
void (*func)(arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, \
     arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7, \
     arg8_type arg8);

/*
 * @}
 */

/*
 * GLES OpenGL basic implementation macros.
 * @{
 */

#define YAGL_GLES_OGL_PROC_IMPL0(obj_type, ps_type, driver_type, func) \
static void driver_type##_##func(struct obj_type *driver_ps) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(); \
}

#define YAGL_GLES_OGL_PROC_IMPL_RET0(obj_type, ps_type, driver_type, ret_type, func) \
static ret_type driver_type##_##func(struct obj_type *driver_ps) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    return d->gl##func(); \
}

#define YAGL_GLES_OGL_PROC_IMPL1(obj_type, ps_type, driver_type, func, arg0_type, arg0) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0); \
}

#define YAGL_GLES_OGL_PROC_IMPL_RET1(obj_type, ps_type, driver_type, ret_type, func, arg0_type, arg0) \
static ret_type driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    return d->gl##func(arg0); \
}

#define YAGL_GLES_OGL_PROC_IMPL2(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg0, arg1) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1); \
}

#define YAGL_GLES_OGL_PROC_IMPL_RET2(obj_type, ps_type, driver_type, ret_type, func, arg0_type, arg1_type, arg0, arg1) \
static ret_type driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    return d->gl##func(arg0, arg1); \
}

#define YAGL_GLES_OGL_PROC_IMPL3(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg0, arg1, arg2) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2); \
}

#define YAGL_GLES_OGL_PROC_IMPL4(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg0, arg1, arg2, arg3) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3); \
}

#define YAGL_GLES_OGL_PROC_IMPL5(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg0, arg1, arg2, arg3, arg4) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3, arg4); \
}

#define YAGL_GLES_OGL_PROC_IMPL6(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg0, arg1, arg2, arg3, arg4, arg5) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4, arg5_type arg5) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3, arg4, arg5); \
}

#define YAGL_GLES_OGL_PROC_IMPL7(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4, arg5_type arg5, arg6_type arg6) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3, arg4, arg5, arg6); \
}

#define YAGL_GLES_OGL_PROC_IMPL8(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7); \
}

#define YAGL_GLES_OGL_PROC_IMPL9(obj_type, ps_type, driver_type, func, arg0_type, arg1_type, arg2_type, arg3_type, arg4_type, arg5_type, arg6_type, arg7_type, arg8_type, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) \
static void driver_type##_##func(struct obj_type *driver_ps, arg0_type arg0, arg1_type arg1, arg2_type arg2, arg3_type arg3, arg4_type arg4, arg5_type arg5, arg6_type arg6, arg7_type arg7, arg8_type arg8) \
{ \
    struct driver_type *d = (struct driver_type*)(((struct ps_type*)driver_ps)->driver); \
    d->gl##func(arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8); \
}

/*
 * @}
 */

/*
 * GLES OpenGL function loading/assigning macros.
 * @{
 */

#define YAGL_GLES_OGL_GET_PROC(driver, func) \
    do { \
        if (get_address) { \
            *(void**)(&driver->func) = get_address((const GLubyte*)#func); \
        } \
        if (!driver->func) { \
            *(void**)(&driver->func) = yagl_dyn_lib_get_sym(dyn_lib, #func); \
            if (!driver->func) { \
                YAGL_LOG_ERROR("Unable to get symbol: %s", \
                               yagl_dyn_lib_get_error(dyn_lib)); \
                goto fail; \
            } \
        } \
    } while (0)

#define YAGL_GLES_OGL_ASSIGN_PROC(driver_type, driver_ps, func) \
    driver_ps->base.func = &driver_type##_##func

/*
 * @}
 */

#endif
