#include "yagl_log.h"
#include "qemu-thread.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

static const char* g_log_level_to_str[yagl_log_level_max + 1] =
{
    "OFF",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

static struct
{
    const char* datatype;
    const char* format;
} g_datatype_to_format[] =
{
    { "EGLboolean", "%u" },
    { "EGLenum", "%u" },
    { "EGLint", "%d" },
    { "EGLConfig", "%p" },
    { "EGLContext", "%p" },
    { "EGLDisplay", "%p" },
    { "EGLSurface", "%p" },
    { "EGLClientBuffer", "%p" },
    { "yagl_host_handle", "%u" },
    { "uint32_t", "%u" },
    { "int", "%d" },
    { "GLenum", "%u" },
    { "GLuint", "%u" },
    { "GLbitfield", "%u" },
    { "GLclampf", "%f" },
    { "GLfloat", "%f" },
    { "GLint", "%d" },
    { "GLsizei", "%d" },
    { "GLclampx", "%d" },
    { "GLfixed", "%d" },
    { "GLboolean", "%" PRIu8 },
    { "GLchar", "%" PRIi8 },
    { "GLintptr", "%ld" },
    { "GLsizeiptr", "%ld" },
    { "target_ulong", "0x%lX" }
};

static yagl_log_level g_log_level = yagl_log_level_off;
static char** g_log_facilities_match = NULL;
static char** g_log_facilities_no_match = NULL;
static bool g_log_func_trace = false;
static QemuMutex g_log_mutex;

static const char* yagl_log_datatype_to_format(const char* type)
{
    unsigned int i;

    if (strchr(type, '*'))
    {
        return "%p";
    }

    for (i = 0; i < sizeof(g_datatype_to_format)/sizeof(g_datatype_to_format[0]); ++i)
    {
        if (strcmp(g_datatype_to_format[i].datatype, type) == 0)
        {
            return g_datatype_to_format[i].format;
        }
    }

    return NULL;
}

static void yagl_log_print_current_time(void)
{
    char buff[128];
#ifdef _WIN32
    struct tm *ptm;
#else
    struct tm tm;
#endif
    qemu_timeval tv = { 0, 0 };
    time_t ti;

    qemu_gettimeofday(&tv);

    ti = tv.tv_sec;

#ifdef _WIN32
    ptm = localtime(&ti);
    strftime(buff, sizeof(buff),
             "%H:%M:%S", ptm);
#else
    localtime_r(&ti, &tm);
    strftime(buff, sizeof(buff),
             "%H:%M:%S", &tm);
#endif
    fprintf(stderr, "%s", buff);
}

/*
 * Simple asterisk pattern matcher.
 */
static bool yagl_log_match(const char* str, const char* expr)
{
    while (*str && *expr)
    {
        /*
         * Swallow '**'.
         */

        while ((*expr == '*') &&
               (*(expr + 1) == '*'))
        {
            ++expr;
        }

        if (*expr == '*')
        {
            if (!*(expr + 1))
            {
                /*
                 * Last '*' always matches.
                 */

                return true;
            }

            /*
             * Search for symbol after the asterisk.
             */

            while (*str && (*str != *(expr + 1)))
            {
                ++str;
            }

            if (!*str)
            {
                /*
                 * Reached the end, didn't find symbol following asterisk,
                 * no match.
                 */

                return false;
            }

            /*
             * Jump to the symbol after the one that's after asterisk.
             */

            ++str;
            expr += 2;
        }
        else
        {
            /*
             * No asterisk, exact match.
             */

            if (*str != *expr)
            {
                return false;
            }

            ++str;
            ++expr;
        }
    }

    /*
     * Remaining '*' always match.
     */

    while (*expr == '*')
    {
        ++expr;
    }

    return (*str == 0) && (*expr == 0);
}

void yagl_log_init(void)
{
    char* level_str = getenv("YAGL_DEBUG");
    int level = level_str ? atoi(level_str) : yagl_log_level_off;
    char* facilities;
    char* func_trace;

    qemu_mutex_init(&g_log_mutex);

    if (level < 0)
    {
        g_log_level = yagl_log_level_off;
    }
    else if (level > yagl_log_level_max)
    {
        g_log_level = (yagl_log_level)yagl_log_level_max;
    }
    else
    {
        g_log_level = (yagl_log_level)level;
    }

    facilities = getenv("YAGL_DEBUG_FACILITIES");

    if (facilities)
    {
        char* tmp_facilities = facilities;
        int i = 0, num_match = 0, num_no_match = 0;

        while (1)
        {
            char* tmp = strchr(tmp_facilities, ',');

            if (!tmp)
            {
                break;
            }

            if (tmp - tmp_facilities > 0)
            {
                ++i;
            }

            tmp_facilities = tmp + 1;
        }

        if (strlen(tmp_facilities) > 0)
        {
            ++i;
        }

        g_log_facilities_match = g_malloc0((i + 1) * sizeof(char*));
        g_log_facilities_no_match = g_malloc0((i + 1) * sizeof(char*));

        tmp_facilities = facilities;

        while (1)
        {
            char* tmp = strchr(tmp_facilities, ',');

            if (!tmp)
            {
                break;
            }

            if (tmp - tmp_facilities > 0)
            {
                if (*tmp_facilities == '^')
                {
                    if ((tmp - tmp_facilities - 1) > 0)
                    {
                        g_log_facilities_no_match[num_no_match] = strndup(tmp_facilities + 1, tmp - tmp_facilities - 1);
                        ++num_no_match;
                    }
                }
                else
                {
                    g_log_facilities_match[num_match] = strndup(tmp_facilities, tmp - tmp_facilities);
                    ++num_match;
                }
            }

            tmp_facilities = tmp + 1;
        }

        if (strlen(tmp_facilities) > 0)
        {
            if (*tmp_facilities == '^')
            {
                if ((strlen(tmp_facilities) - 1) > 0)
                {
                    g_log_facilities_no_match[num_no_match] = strdup(tmp_facilities + 1);
                    ++num_no_match;
                }
            }
            else
            {
                g_log_facilities_match[num_match] = strdup(tmp_facilities);
                ++num_match;
            }
        }

        g_log_facilities_no_match[num_no_match] = NULL;
        g_log_facilities_match[num_match] = NULL;

        if (!num_no_match)
        {
            g_free(g_log_facilities_no_match);
            g_log_facilities_no_match = NULL;
        }

        if (!num_match)
        {
            g_free(g_log_facilities_match);
            g_log_facilities_match = NULL;
        }
    }

    func_trace = getenv("YAGL_DEBUG_FUNC_TRACE");

    g_log_func_trace = func_trace ? (atoi(func_trace) > 0) : false;
}

void yagl_log_cleanup(void)
{
    int i;
    g_log_level = yagl_log_level_off;
    if (g_log_facilities_no_match)
    {
        for (i = 0; g_log_facilities_no_match[i]; ++i)
        {
            free(g_log_facilities_no_match[i]);
            g_log_facilities_no_match[i] = NULL;
        }
        g_free(g_log_facilities_no_match);
        g_log_facilities_no_match = NULL;
    }
    if (g_log_facilities_match)
    {
        for (i = 0; g_log_facilities_match[i]; ++i)
        {
            free(g_log_facilities_match[i]);
            g_log_facilities_match[i] = NULL;
        }
        g_free(g_log_facilities_match);
        g_log_facilities_match = NULL;
    }
    g_log_func_trace = 0;
    qemu_mutex_destroy(&g_log_mutex);
}

void yagl_log_event(yagl_log_level log_level,
                    yagl_pid process_id,
                    yagl_tid thread_id,
                    const char* facility,
                    int line,
                    const char* format, ...)
{
    va_list args;

    qemu_mutex_lock(&g_log_mutex);

    yagl_log_print_current_time();
    fprintf(stderr,
            " %-5s [%u/%u] %s:%d",
            g_log_level_to_str[log_level],
            process_id,
            thread_id,
            facility,
            line);
    if (format)
    {
        va_start(args, format);
        fprintf(stderr, " - ");
        vfprintf(stderr, format, args);
        va_end(args);
    }
    fprintf(stderr, "\n");

    qemu_mutex_unlock(&g_log_mutex);
}

void yagl_log_func_enter(yagl_pid process_id,
                         yagl_tid thread_id,
                         const char* func,
                         int line,
                         const char* format, ...)
{
    va_list args;

    qemu_mutex_lock(&g_log_mutex);

    yagl_log_print_current_time();
    fprintf(stderr,
            " %-5s [%u/%u] {{{ %s(",
            g_log_level_to_str[yagl_log_level_trace],
            process_id,
            thread_id,
            func);
    if (format)
    {
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
    fprintf(stderr, "):%d\n", line);

    qemu_mutex_unlock(&g_log_mutex);
}

void yagl_log_func_exit(yagl_pid process_id,
                        yagl_tid thread_id,
                        const char* func,
                        int line,
                        const char* format, ...)
{
    va_list args;

    qemu_mutex_lock(&g_log_mutex);

    yagl_log_print_current_time();
    fprintf(stderr,
            " %-5s [%u/%u] }}} %s:%d",
            g_log_level_to_str[yagl_log_level_trace],
            process_id,
            thread_id,
            func,
            line);
    if (format)
    {
        va_start(args, format);
        fprintf(stderr, " = ");
        vfprintf(stderr, format, args);
        va_end(args);
    }
    fprintf(stderr, "\n");

    qemu_mutex_unlock(&g_log_mutex);
}

void yagl_log_func_enter_split(yagl_pid process_id,
                               yagl_tid thread_id,
                               const char* func,
                               int line,
                               int num_args, ...)
{
    char format[1025] = { '\0' };
    va_list args;

    qemu_mutex_lock(&g_log_mutex);

    yagl_log_print_current_time();
    fprintf(stderr,
            " %-5s [%u/%u] {{{ %s(",
            g_log_level_to_str[yagl_log_level_trace],
            process_id,
            thread_id,
            func);

    if (num_args > 0)
    {
        int i;
        bool skip = false;

        va_start(args, num_args);

        for (i = 0; i < num_args;)
        {
            const char* arg_format = yagl_log_datatype_to_format(va_arg(args, const char*));
            const char* arg_name;

            if (!arg_format)
            {
                skip = true;
                break;
            }

            arg_name = va_arg(args, const char*);

            strcat(format, arg_name);
            strcat(format, " = ");
            strcat(format, arg_format);

            ++i;

            if (i < num_args)
            {
                strcat(format, ", ");
            }
        }

        if (skip)
        {
            fprintf(stderr, "...");
        }
        else
        {
            vfprintf(stderr, format, args);
        }

        va_end(args);
    }

    fprintf(stderr, "):%d\n", line);

    qemu_mutex_unlock(&g_log_mutex);
}

void yagl_log_func_exit_split(yagl_pid process_id,
                              yagl_tid thread_id,
                              const char* func,
                              int line,
                              const char* datatype, ...)
{
    va_list args;

    qemu_mutex_lock(&g_log_mutex);

    yagl_log_print_current_time();
    fprintf(stderr,
            " %-5s [%u/%u] }}} %s:%d",
            g_log_level_to_str[yagl_log_level_trace],
            process_id,
            thread_id,
            func,
            line);

    if (datatype)
    {
        const char* format = yagl_log_datatype_to_format(datatype);
        if (format)
        {
            fprintf(stderr, " = ");
            va_start(args, datatype);
            vfprintf(stderr, format, args);
            va_end(args);
        }
    }

    fprintf(stderr, "\n");

    qemu_mutex_unlock(&g_log_mutex);
}

bool yagl_log_is_enabled_for_level(yagl_log_level log_level)
{
    return log_level <= g_log_level;
}

bool yagl_log_is_enabled_for_facility(const char* facility)
{
    int i;
    bool res = false;

    if (g_log_facilities_match)
    {
        for (i = 0; g_log_facilities_match[i]; ++i)
        {
            if (yagl_log_match(facility, g_log_facilities_match[i]))
            {
                res = true;
                break;
            }
        }
    }
    else
    {
        res = true;
    }

    if (res && g_log_facilities_no_match)
    {
        for (i = 0; g_log_facilities_no_match[i]; ++i)
        {
            if (yagl_log_match(facility, g_log_facilities_no_match[i]))
            {
                res = false;
                break;
            }
        }
    }

    return res;
}

bool yagl_log_is_enabled_for_func_tracing(void)
{
    return g_log_func_trace;
}
