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
  @file     osutil-darwin.c
  @brief    Collection of utilities for darwin
 */

#include "emulator_common.h"
#include "osutil.h"
#include "emulator.h"
#include "emul_state.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#ifndef CONFIG_DARWIN
#error
#endif

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sysctl.h>
#include <SystemConfiguration/SystemConfiguration.h>

MULTI_DEBUG_CHANNEL(qemu, osutil);


static qemu_timeval tv = { 0, 0 };
static time_t ti;
static char buf_time[64];
static int g_shmid;
static CFDictionaryRef proxySettings;

extern char tizen_target_img_path[];

static char *cfstring_to_cstring(CFStringRef str) {
    if (str == NULL) {
        return NULL;
    }

    CFIndex length = CFStringGetLength(str);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
    char *buffer = (char *)malloc(maxSize);
    if (CFStringGetCString(str, buffer, maxSize, kCFStringEncodingUTF8))
        return buffer;
    return NULL;
}

static int cfnumber_to_int(CFNumberRef num) {
    if (!num)
        return 0;

    int value;
    CFNumberGetValue(num, kCFNumberIntType, &value);
    return value;
}

void check_vm_lock_os(void)
{
    /* TODO: */
}

void make_vm_lock_os(void)
{
    char *shared_memory;
    int base_port;
    base_port = get_emul_vm_base_port();
    g_shmid = shmget((key_t)base_port, getpagesize(), 0666|IPC_CREAT);
    if (g_shmid == -1) {
        ERR("shmget failed\n");
        perror("osutil-darwin: ");
        return;
    }

    shared_memory = shmat(g_shmid, (char *)0x00, 0);
    if (shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("osutil-darwin: ");
        return;
    }
    g_sprintf(shared_memory, "%s", tizen_target_img_path);
    INFO("shared memory key: %d, value: %s\n", base_port, (char *)shared_memory);

    if (shmdt(shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("osutil-darwin: ");
    }

}

void remove_vm_lock_os(void)
{
    if (shmctl(g_shmid, IPC_RMID, 0) == -1) {
        ERR("shmctl failed\n");
        perror("osutil-linux: ");
    }
}

void set_bin_path_os(char const *const exec_argv)
{
    gchar *file_name = NULL;

    if (!exec_argv) {
        return;
    }

    char *data = g_strdup(exec_argv);
    if (!data) {
        ERR("Fail to strdup for paring a binary directory.\n");
        return;
    }

    file_name = g_strrstr(data, "/");
    if (!file_name) {
        free(data);
        return;
    }

    g_strlcpy(bin_path, data, strlen(data) - strlen(file_name) + 1);

    g_strlcat(bin_path, "/", PATH_MAX);
    free(data);
}

int get_number_of_processors(void)
{
    int mib[2], sys_num = 0;
    size_t len;

    mib[0] = CTL_HW;
    mib[1] = HW_AVAILCPU;

    sysctl(mib, 2, &sys_num, &len, NULL, 0);
    if (sys_num < 1) {
        mib[1] = HW_NCPU;
        sysctl(mib, 2, &sys_num, &len, NULL, 0);

        if (sys_num < 1) {
            sys_num = 1;
        }
    }
    INFO("* Number of processors : %d\n", sys_num);

    return sys_num;
}

void print_system_info_os(void)
{
    INFO("* Mac\n");

    /* uname */
    INFO("* Host machine uname :\n");
    char const *const uname_cmd = "uname -a";
    if(system(uname_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", uname_cmd);
    }

    /* hw information */
    int mib[2];
    size_t len;
    char *sys_info;
    int sys_num = 0;

    mib[0] = CTL_HW;
    mib[1] = HW_MODEL;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine model : %s\n", sys_info);
    }
    free(sys_info);

    mib[0] = CTL_HW;
    mib[1] = HW_MACHINE;
    sysctl(mib, 2, NULL, &len, NULL, 0);
    sys_info = malloc(len * sizeof(char));
    if (sysctl(mib, 2, sys_info, &len, NULL, 0) >= 0) {
        INFO("* Machine class : %s\n", sys_info);
    }
    free(sys_info);

#if 0
    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Number of processors : %d\n", sys_num);
    }
#endif
    get_number_of_processors();

    mib[0] = CTL_HW;
    mib[1] = HW_PHYSMEM;
    len = sizeof(sys_num);
    if (sysctl(mib, 2, &sys_num, &len, NULL, 0) >= 0) {
        INFO("* Total memory : %llu bytes\n", sys_num);
    }

    /* java version */
    INFO("* Java version :\n");
    char const *const lspci_cmd = "java -version";

    fflush(stdout);
    if(system(lspci_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lspci_cmd);
    }
}

char *get_timeofday(void)
{
    qemu_gettimeofday(&tv);
    ti = tv.tv_sec;

    struct tm tm;
    localtime_r(&ti, &tm);
    strftime(buf_time, sizeof(buf_time),
             "%H:%M:%S", &tm);

    return buf_time;
}

static int get_auto_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char type[DEFAULTBUFLEN];
    char proxy[DEFAULTBUFLEN];
    char line[DEFAULTBUFLEN];
    FILE *fp_pacfile;
    char *p = NULL;

    CFStringRef pacURL = (CFStringRef)CFDictionaryGetValue(proxySettings,
                    kSCPropNetProxiesProxyAutoConfigURLString);
    if (pacURL) {
        char url[DEFAULTBUFLEN] = {};
        CFStringGetCString(pacURL, url, sizeof url, kCFStringEncodingASCII);
                INFO("pac address: %s\n", (char*)url);
        download_url(url);
        }

    fp_pacfile = fopen(pac_tempfile, "r");
    if(fp_pacfile != NULL) {
        while(fgets(line, DEFAULTBUFLEN, fp_pacfile) != NULL) {
            if( (strstr(line, "return") != NULL) && (strstr(line, "if") == NULL)) {
                INFO("line found %s", line);
                sscanf(line, "%*[^\"]\"%s %s", type, proxy);
            }
        }

        if(g_str_has_prefix(type, DIRECT)) {
            INFO("auto proxy is set to direct mode\n");
            fclose(fp_pacfile);
        }
        else if(g_str_has_prefix(type, PROXY)) {
            INFO("auto proxy is set to proxy mode\n");
            INFO("type: %s, proxy: %s\n", type, proxy);
            p = strtok(proxy, "\";");
            if(p != NULL) {
                INFO("auto proxy to set: %s\n",p);
                strcpy(http_proxy, p);
                strcpy(https_proxy, p);
                strcpy(ftp_proxy, p);
                strcpy(socks_proxy, p);
            }
            fclose(fp_pacfile);
        }
        else
        {
            ERR("pac file is not wrong! It could be the wrong pac address or pac file format\n");
            fclose(fp_pacfile);
        }
    }
    else {
        ERR("fail to get pacfile fp\n");
    return -1;
    }

    remove(pac_tempfile);
    return 0;
}

static void get_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char *hostname;
    int port;
    CFNumberRef isEnable;
    CFStringRef proxyHostname;
    CFNumberRef proxyPort;
    CFDictionaryRef proxySettings;
    proxySettings = SCDynamicStoreCopyProxies(NULL);

    isEnable  = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPEnable);
    if (cfnumber_to_int(isEnable)) {
        // Get proxy hostname
        proxyHostname = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPProxy);
        hostname = cfstring_to_cstring(proxyHostname);
        // Get proxy port
        proxyPort = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPPort);
        port = cfnumber_to_int(proxyPort);
        // Save hostname & port
        snprintf(http_proxy, DEFAULTBUFLEN, "%s:%d", hostname, port);

        free(hostname);
    } else {
        INFO("http proxy is null\n");
    }

    isEnable  = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPSEnable);
    if (cfnumber_to_int(isEnable)) {
        // Get proxy hostname
        proxyHostname = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPSProxy);
        hostname = cfstring_to_cstring(proxyHostname);
        // Get proxy port
        proxyPort = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesHTTPSPort);
        port = cfnumber_to_int(proxyPort);
        // Save hostname & port
        snprintf(https_proxy, DEFAULTBUFLEN, "%s:%d", hostname, port);

        free(hostname);
    } else {
        INFO("https proxy is null\n");
    }

    isEnable  = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesFTPEnable);
    if (cfnumber_to_int(isEnable)) {
        // Get proxy hostname
        proxyHostname = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesFTPProxy);
        hostname = cfstring_to_cstring(proxyHostname);
        // Get proxy port
        proxyPort = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesFTPPort);
        port = cfnumber_to_int(proxyPort);
        // Save hostname & port
        snprintf(ftp_proxy, DEFAULTBUFLEN, "%s:%d", hostname, port);

        free(hostname);
    } else {
        INFO("ftp proxy is null\n");
    }

    isEnable  = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesSOCKSEnable);
    if (cfnumber_to_int(isEnable)) {
        // Get proxy hostname
        proxyHostname = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesSOCKSProxy);
        hostname = cfstring_to_cstring(proxyHostname);
        // Get proxy port
        proxyPort = CFDictionaryGetValue(proxySettings, kSCPropNetProxiesSOCKSPort);
        port = cfnumber_to_int(proxyPort);
        // Save hostname & port
        snprintf(socks_proxy, DEFAULTBUFLEN, "%s:%d", hostname, port);

        free(hostname);
    } else {
        INFO("socks proxy is null\n");
    }
    CFRelease(proxySettings);
}

void get_host_proxy_os(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    int ret;
    proxySettings = SCDynamicStoreCopyProxies(NULL);
    if(proxySettings) {
        INFO("AUTO PROXY MODE\n");
        ret = get_auto_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        if(strlen(http_proxy) == 0 && ret < 0) {
            INFO("MANUAL PROXY MODE\n");
            get_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
    }
}
