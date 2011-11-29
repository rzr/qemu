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

#ifndef GPI_H
#define GPI_H

#define CAll_HANDLE_MAX 128
#define FTN_SYNC_SDB_PORT 1

/* operations valid on all objects */
typedef struct object_ops object_ops_t;
struct object_ops {
	int (*ftn)(char *args_in, int args_len, char *r_buffer, int r_len);
};

int call_gpi(int pid, int ftn_num, char *in_args, int args_len, char *r_buffer, int r_len);

#endif 
