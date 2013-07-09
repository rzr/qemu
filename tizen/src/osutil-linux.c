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
  @file     osutil-linux.c
  @brief    Collection of utilities for linux
 */

#include "maru_common.h"
#include "osutil.h"
#include "emulator.h"
#include "debug_ch.h"
#include "maru_err_table.h"
#include "sdb.h"

#ifndef CONFIG_LINUX
#error
#endif

#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

MULTI_DEBUG_CHANNEL(emulator, osutil);

extern char tizen_target_img_path[];
extern int tizen_base_port;
int g_shmid;
char *g_shared_memory;
int gproxytool = GCONFTOOL;

/* Getting proxy commands */
static const char* gproxycmds[][2] = {
	{ "gconftool-2 -g /system/proxy/mode" , "gsettings get org.gnome.system.proxy mode" },
	{ "gconftool-2 -g /system/proxy/autoconfig_url", "gsettings get org.gnome.system.proxy autoconfig-url" },
	{ "gconftool-2 -g /system/http_proxy/host", "gsettings get org.gnome.system.proxy.http host" },
	{ "gconftool-2 -g /system/http_proxy/port", "gsettings get org.gnome.system.proxy.http port"},
	{ "gconftool-2 -g /system/proxy/secure_host", "gsettings get org.gnome.system.proxy.https host" },
	{ "gconftool-2 -g /system/proxy/secure_port", "gsettings get org.gnome.system.proxy.https port" },
	{ "gconftool-2 -g /system/proxy/ftp_host", "gsettings get org.gnome.system.proxy.ftp host" },
	{ "gconftool-2 -g /system/proxy/ftp_port", "gsettings get org.gnome.system.proxy.ftp port" },
	{ "gconftool-2 -g /system/proxy/socks_host", "gsettings get org.gnome.system.proxy.socks host" },
	{ "gconftool-2 -g /system/proxy/socks_port", "gsettings get org.gnome.system.proxy.socks port" },
};

void check_vm_lock_os(void)
{
    int shm_id;
    void *shm_addr;
    uint32_t port;
    int val;
    struct shmid_ds shm_info;

    for (port = 26100; port < 26200; port += 10) {
        shm_id = shmget((key_t)port, 0, 0);
        if (shm_id != -1) {
            shm_addr = shmat(shm_id, (void *)0, 0);
            if ((void *)-1 == shm_addr) {
                ERR("error occured at shmat()\n");
                break;
            }

            val = shmctl(shm_id, IPC_STAT, &shm_info);
            if (val != -1) {
                INFO("count of process that use shared memory : %d\n",
                    shm_info.shm_nattch);
                if ((shm_info.shm_nattch > 0) &&
                    g_strcmp0(tizen_target_img_path, (char *)shm_addr) == 0) {
                    if (check_port_bind_listen(port + 1) > 0) {
                        shmdt(shm_addr);
                        continue;
                    }
                    shmdt(shm_addr);
                    maru_register_exit_msg(MARU_EXIT_UNKNOWN,
                                        "Can not execute this VM.\n"
                                        "The same name is running now.");
                    exit(0);
                } else {
                    shmdt(shm_addr);
                }
            }
        }
    }
}

void make_vm_lock_os(void)
{
    g_shmid = shmget((key_t)tizen_base_port, MAXLEN, 0666|IPC_CREAT);
    if (g_shmid == -1) {
        ERR("shmget failed\n");
        perror("osutil-linux: ");
        return;
    }

    g_shared_memory = shmat(g_shmid, (char *)0x00, 0);
    if (g_shared_memory == (void *)-1) {
        ERR("shmat failed\n");
        perror("osutil-linux: ");
        return;
    }

    g_sprintf(g_shared_memory, "%s", tizen_target_img_path);
    INFO("shared memory key: %d value: %s\n",
        tizen_base_port, (char *)g_shared_memory);

    if (shmdt(g_shared_memory) == -1) {
        ERR("shmdt failed\n");
        perror("osutil-linux: ");
    }
}

void set_bin_path_os(gchar * exec_argv)
{
    gchar link_path[PATH_MAX] = { 0, };
    char *file_name = NULL;

    ssize_t len = readlink("/proc/self/exe", link_path, sizeof(link_path) - 1);

    if (len < 0 || len > sizeof(link_path)) {
        perror("set_bin_path error : ");
        return;
    }

    link_path[len] = '\0';

    file_name = g_strrstr(link_path, "/");
    g_strlcpy(bin_path, link_path, strlen(link_path) - strlen(file_name) + 1);

    g_strlcat(bin_path, "/", PATH_MAX);
}

void print_system_info_os(void)
{
    INFO("* Linux\n");

    /* depends on building */
    INFO("* QEMU build machine linux kernel version : (%d, %d, %d)\n",
        LINUX_VERSION_CODE >> 16,
        (LINUX_VERSION_CODE >> 8) & 0xff,
        LINUX_VERSION_CODE & 0xff);

     /* depends on launching */
    struct utsname host_uname_buf;
    if (uname(&host_uname_buf) == 0) {
        INFO("* Host machine uname : %s %s %s %s %s\n",
            host_uname_buf.sysname, host_uname_buf.nodename,
            host_uname_buf.release, host_uname_buf.version,
            host_uname_buf.machine);
    }

    struct sysinfo sys_info;
    if (sysinfo(&sys_info) == 0) {
        INFO("* Total Ram : %llu kB, Free: %llu kB\n",
            sys_info.totalram * (unsigned long long)sys_info.mem_unit / 1024,
            sys_info.freeram * (unsigned long long)sys_info.mem_unit / 1024);
    }

    /* get linux distribution information */
    INFO("* Linux distribution infomation :\n");
    char lsb_release_cmd[MAXLEN] = "lsb_release -d -r -c >> ";
    strcat(lsb_release_cmd, log_path);
    if(system(lsb_release_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lsb_release_cmd);
    }

    /* pci device description */
    INFO("* Host PCI devices :\n");
    char lspci_cmd[MAXLEN] = "lspci >> ";
    strcat(lspci_cmd, log_path);

    fflush(stdout);
    if(system(lspci_cmd) < 0) {
        INFO("system function command '%s' \
            returns error !", lspci_cmd);
    }
}

static void process_string(char *buf)
{
    char tmp_buf[MAXLEN];

    /* remove single quotes of strings gotten by gsettings */
    if (gproxytool == GSETTINGS) {
        remove_string(buf, tmp_buf, "\'");
        memset(buf, 0, MAXLEN);
        strncpy(buf, tmp_buf, strlen(tmp_buf)-1);
    }
}

static int get_auto_proxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char type[MAXLEN];
    char proxy[MAXLEN];
    char line[MAXLEN];
    FILE *fp_pacfile;
    char *p = NULL;
    FILE *output;
    char buf[MAXLEN];

    output = popen(gproxycmds[GNOME_PROXY_AUTOCONFIG_URL][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        INFO("pac address: %s\n", buf);
        download_url(buf);
    }
    pclose(output);
    fp_pacfile = fopen(pac_tempfile, "r");
    if(fp_pacfile != NULL) {
        while(fgets(line, MAXLEN, fp_pacfile) != NULL) {
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
    char buf[MAXLEN] = {0,};
    char buf_port[MAXPORTLEN] = {0,};
    char buf_proxy[MAXLEN] = {0,};
    char *buf_proxy_bak;
    char *proxy;
    FILE *output;
    int MAXPROXYLEN = MAXLEN + MAXPORTLEN;

    output = popen(gproxycmds[GNOME_PROXY_HTTP_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);
    
    output = popen(gproxycmds[GNOME_PROXY_HTTP_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf_port) <= 0) {
        //for abnormal case: if can't find the key of http port, get from environment value.
        buf_proxy_bak = getenv("http_proxy");
        INFO("http_proxy from env: %s\n", buf_proxy_bak);
        if(buf_proxy_bak != NULL) {
            proxy = malloc(MAXLEN);
            remove_string(buf_proxy_bak, proxy, HTTP_PREFIX);
            strncpy(http_proxy, proxy, strlen(proxy)-1);
            INFO("final http_proxy value: %s\n", http_proxy);
            free(proxy);
        }
        else {
            INFO("http_proxy is not set on env.\n");
            pclose(output);
            return;
        }

    }
    else {
        snprintf(http_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf_port);
        memset(buf_proxy, 0, MAXLEN);
        INFO("http_proxy: %s\n", http_proxy);
    }
    pclose(output);

    memset(buf, 0, MAXLEN);

    output = popen(gproxycmds[GNOME_PROXY_HTTPS_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_HTTPS_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(https_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, MAXLEN);
    memset(buf_proxy, 0, MAXLEN);
    INFO("https_proxy : %s\n", https_proxy);

    output = popen(gproxycmds[GNOME_PROXY_FTP_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_FTP_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(ftp_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, MAXLEN);
    memset(buf_proxy, 0, MAXLEN);
    INFO("ftp_proxy : %s\n", ftp_proxy);

    output = popen(gproxycmds[GNOME_PROXY_SOCKS_HOST][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        process_string(buf);
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen(gproxycmds[GNOME_PROXY_SOCKS_PORT][gproxytool], "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(socks_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    INFO("socks_proxy : %s\n", socks_proxy);
}


void get_host_proxy_os(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char buf[MAXLEN];
    FILE *output;
    int ret;

    output = popen(gproxycmds[GNOME_PROXY_MODE][gproxytool], "r");
    ret = fscanf(output, "%s", buf);
    if (ret <= 0) {
        pclose(output);
        INFO("Try to use gsettings to get proxy\n");
        gproxytool = GSETTINGS;
        output = popen(gproxycmds[GNOME_PROXY_MODE][gproxytool], "r");
        ret = fscanf(output, "%s", buf);
    }
    if (ret > 0) {
        process_string(buf);
        //priority : auto > manual > none       
        if (strcmp(buf, "auto") == 0) {
            INFO("AUTO PROXY MODE\n");
            get_auto_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "manual") == 0) {
            INFO("MANUAL PROXY MODE\n");
            get_proxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "none") == 0) {
            INFO("DIRECT PROXY MODE\n");
        }
    }
    pclose(output);
}
