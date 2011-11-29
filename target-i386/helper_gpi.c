/* 
 * virtio general purpose 
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * Authors:
 *  Dongkyun Yun  dk77.yun@samsung.com
 *
 * PROPRIETARY/CONFIDENTIAL
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").      
 * You shall not disclose such Confidential Information and shall use it only in accordance with the terms of the lice    nse agreement 
 * you entered into with SAMSUNG ELECTRONICS.  SAMSUNG make no representations or warranties about the suitability 
 * of the software, either express or implied, including but not limited to the implied warranties of merchantability,     fitness for 
 * a particular purpose, or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee as 
 * a result of using, modifying or distributing this software or its derivatives.
 */

#define _XOPEN_SOURCE 600

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "qemu-common.h"
#include "qemu-log.h"
#include "helper_gpi.h"

#include "sdb.h"
#include "debug_ch.h"
MULTI_DEBUG_CHANNEL(qemu, gpi);

/**
 *  Global variables
 *
 */
static struct object_ops *call_ops = NULL;

/* do_call_gpi()
 *
 */
static int sync_sdb_port(char *args_in, int args_len, char *r_buffer, int r_len)
{
	char tmp[32] = {0};

	snprintf(tmp, sizeof(tmp)-1, "%d", get_sdb_base_port());

	if( r_len < strlen(tmp) ){
		ERR("short return buffer length [%d < %d(%s)] \n", r_len, strlen(tmp), tmp);
		return 0;
	}

	memcpy(r_buffer, tmp, strlen(tmp));
	TRACE("sdb-port: => return [%s]\n", r_buffer);

	return 1;
}

static int dummy_function(char *args_in, int args_len, char *r_buffer, int r_len)
{
	ERR("Need the specific operation \n");

	return 1;
}

static void do_call_init(void)
{
	int i;

	call_ops = qemu_mallocz(CAll_HANDLE_MAX*sizeof(object_ops_t) );

	/* set dummy function */
	for( i = 0; i < CAll_HANDLE_MAX ; i++ ) {
		call_ops[i].ftn = &dummy_function;
	}

	/* init function */
	call_ops[FTN_SYNC_SDB_PORT].ftn = &sync_sdb_port;

	return;
}


int call_gpi(int pid, int ftn_num, char *in_args, int args_len, char *r_buffer, int r_len)
{
	static int init = 0;
	int ret;

	TRACE("ftn_num(%d) args_len(%d) r_len(%d) \n", ftn_num, args_len, r_len);

	if( init == 0 ){
		do_call_init();
		init = 1;
	}

	ret = call_ops[ftn_num].ftn(in_args, args_len, r_buffer, r_len);
	TRACE("return [%s]\n", r_buffer);

	if(!ret){
		/* error */
	}

	return ret;
}
