/*
 * Emulator
 *
 * Copyright (C) 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * SeokYeon Hwang <syeon.hwang@samsung.com>
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

#ifndef __EMULATOR_OPTIONS_H__
#define __EMULATOR_OPTIONS_H__

void set_variable(const char * const arg1, const char * const arg2, bool override);
char *get_variable(const char * const name);
void reset_variables(void);
bool load_profile_default(const char * const conf, const char * const profile);
bool assemble_profile_args(int *qemu_argc, char **qemu_argv,
        int *skin_argc, char **skin_argv);

#endif /* __EMULATOR_OPTIONS_H__ */
