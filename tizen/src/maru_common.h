/*
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
 * HyunJun Son
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
 * MA 02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

/**
 * @file maru_common.h
 * @brief - header file that covers maru common features
 */

#ifndef __MARU_COMMON_H__
#define __MARU_COMMON_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "config-host.h"

#if !defined(PATH_MAX)
#if defined(MAX_PATH)
#define PATH_MAX    MAX_PATH
#else
#define PATH_MAX    256
#endif
#endif

#define JAVA_MAX_COMMAND_LENGTH 1024

#define JAR_SKINFILE "emulator-skin.jar"
#define JAVA_LIBRARY_PATH "-Djava.library.path"

#ifndef CONFIG_DARWIN
#define JAVA_EXEOPTION "-jar"
#else
#define JAVA_EXEOPTION "-XstartOnFirstThread -jar" // Must start the Java window on the first thread on Mac
#endif
#define JAVA_SIMPLEMODE_OPTION "simple.msg"

#ifdef CONFIG_WIN32
#define MY_KEY_WOW64_64KEY 0x0100
#else
#define JAVA_EXEFILE_PATH "java"
#endif


// W/A for preserve larger continuous heap for RAM.
extern void *preallocated_ptr;

#endif /* __MARU_COMMON_H__ */
