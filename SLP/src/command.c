/*
 * Emulator
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * DoHyung Hong <don.hong@samsung.com>
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * JinKyu Kim <fredrick.kim@samsung.com>
 * KiTae Kim <kt920.kim@samsung.com>
 * JinHyung Jo <jinhyung.jo@samsung.com>
 * YuYeon Oh <yuyeon.oh@samsung.com>
 * WooJin Jung <woojin2.jung@samsung.com>
 * SungMin Ha <sungmin82.ha@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * DongKyun Yun <dk77.yun@samsung.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */


/**
	@file 	command.c
	@brief	functions to manage terminal window
*/

#include "command.h"
#include "dialog.h"
#include "configuration.h"

#ifdef _WIN32
void* system_telnet(void)
{
	gchar cmd[256] = "";
	//sprintf (cmd, "start cmd /C telnet %s",configuration.qemu_configuration.root_file_system_ip); 
	//sprintf (cmd, "start cmd /C ssh root@%s",configuration.qemu_configuration.root_file_system_ip); 
	system(cmd);
}
#endif


/* execute a terminal connected to the target */
void create_cmdwindow(void)
{

	gchar cmd[256];
	const char *terminal = getenv("EMULATOR_TERMINAL");

#ifdef _WIN32
	sprintf (cmd, "start cmd /C telnet localhost %d", startup_option.telnet_port);
	system(cmd);
	fflush(stdout);
#else

	/* gnome-terminal */

	if (!terminal)
		terminal = "/usr/bin/xterm -l -e";

	sprintf(cmd, "%s ssh -o StrictHostKeyChecking=no root@localhost -p %d", terminal, startup_option.ssh_port);

	if(emul_create_process(cmd) == TRUE)
		log_msg(MSGL_INFO, "start command window\n");
	else
		log_msg(MSGL_ERROR, "falied to start command window\n");

#endif
}

