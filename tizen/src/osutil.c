/* 
 * Emulator
 *
 * Copyright (C) 2012, 2013 Samsung Electronics Co., Ltd. All rights reserved.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

/**
  @file     osutil.c
  @brief    Common functions for osutil
 */

#include "osutil.h"
#include "debug_ch.h"

#include <curl/curl.h>
#include <string.h>

MULTI_DEBUG_CHANNEL(emulator, osutil);


const char *pac_tempfile = ".autoproxy";

inline size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{     
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}  

inline void download_url(char *url) 
{     
    CURL *curl;
    FILE *fp;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(pac_tempfile, "wb");
        if(fp == NULL) {
            ERR("failed to fopen(): %s\n", pac_tempfile);
            return;
        }
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        /* just in case network does not work */
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 3000);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        res = curl_easy_perform(curl);
        if (res != 0) {
            ERR("Fail to download pac file: %s\n", url);
        }

        curl_easy_cleanup(curl);
        fclose(fp);
    }

    return;
} 

inline void remove_string(char *src, char *dst, const char *toremove)
{
    int len = strlen(toremove);
    int i, j;
    int max_len = strlen(src);

    for(i = len, j = 0; i < max_len; i++)
    {
        dst[j++] = src[i];
    }

    dst[j] = '\0';
}
