/* 
 * Emulator
 *
 * Copyright (C) 2011, 2012 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: 
 * SeokYeon Hwang <syeon.hwang@samsung.com>
 * HyunJun Son <hj79.son@samsung.com>
 * MunKyu Im <munkyu.im@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * YeongKyoon Lee <yeongkyoon.lee@samsung.com>
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


#include "maru_common.h"
#include "emulator.h"
#include "sdb.h"
#include "skin/skin_server.h"

MULTI_DEBUG_CHANNEL(tizen, main);


int tizen_base_port = 0;

int _emulator_condition = 0; //TODO:

int get_emulator_condition(void)
{
	return _emulator_condition;
}

void set_emulator_condition(int state)
{
	_emulator_condition = state;
}

void exit_emulator(void)
{

}

static void construct_main_window(void)
{
    start_skin_server(11111, 0, 0);
    if (start_skin_client() == false) {
        //TODO:
    }
}

int qemu_main(int argc, char **argv, char **envp);

int main(int argc, char** argv)
{
	tizen_base_port = get_sdb_base_port();
	construct_main_window();
	qemu_main(argc, argv, NULL);

	return 0;
}

