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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Contributors:
 * - S-Core Co., Ltd
 *
 */

/**
  @file option.c
  @brief    collection of dialog function
 */

#include "option.h"
#include "emulator.h"
#include "maru_common.h"
#if defined (CONFIG_LINUX)
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#elif defined (CONFIG_WIN32)
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <winreg.h>
#elif defined (CONFIG_DARWIN)
#include <SystemConfiguration/SystemConfiguration.h>
CFDictionaryRef proxySettings;
#endif
#include <curl/curl.h>

#include "debug_ch.h"

#define HTTP_PROTOCOL "http="
#define HTTP_PREFIX "http://"
#define HTTPS_PROTOCOL "https="
#define FTP_PROTOCOL "ftp="
#define SOCKS_PROTOCOL "socks="
#define DIRECT "DIRECT"
#define PROXY "PROXY"
#define MAXPORTLEN 6
MULTI_DEBUG_CHANNEL(tizen, option);
#if defined(CONFIG_WIN32)
BYTE *url;
#endif
const char *pactempfile = ".autoproxy"; 

#if defined (CONFIG_DARWIN)
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
#endif

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{     
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}  

static void download_url(char *url) 
{     
    CURL *curl;     
    FILE *fp;     
    CURLcode res;     

    curl = curl_easy_init();
    if (curl) { 
        fp = fopen(pactempfile,"wb");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        //just in case network does not work.
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 3000);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        res = curl_easy_perform(curl);
        if(res != 0) {
            ERR("Fail to download pac file: %s\n", url);
        }
        curl_easy_cleanup(curl); 
        fclose(fp);
    }     

    return; 
} 

#if defined (CONFIG_DARWIN)
static void getmacproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
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
        snprintf(http_proxy, MAXLEN, "%s:%d", hostname, port);

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
        snprintf(https_proxy, MAXLEN, "%s:%d", hostname, port);

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
        snprintf(ftp_proxy, MAXLEN, "%s:%d", hostname, port);

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
        snprintf(socks_proxy, MAXLEN, "%s:%d", hostname, port);

        free(hostname);
    } else {
        INFO("socks proxy is null\n");
    }
    CFRelease(proxySettings);
}
#endif

static void remove_string(char *src, char *dst, const char *toremove)
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

#if defined (CONFIG_LINUX)    
static void getlinuxproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char buf[MAXLEN] = {0,};
    char buf_port[MAXPORTLEN] = {0,};
    char buf_proxy[MAXLEN] = {0,};
    char *buf_proxy_bak;
    char *proxy;
    FILE *output;
    int MAXPROXYLEN = MAXLEN + MAXPORTLEN;

    output = popen("gconftool-2 --get /system/http_proxy/host", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);
    
    output = popen("gconftool-2 --get /system/http_proxy/port", "r");
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

    output = popen("gconftool-2 --get /system/proxy/secure_host", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen("gconftool-2 --get /system/proxy/secure_port", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(https_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, MAXLEN);
    memset(buf_proxy, 0, MAXLEN);
    INFO("https_proxy : %s\n", https_proxy);

    output = popen("gconftool-2 --get /system/proxy/ftp_host", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen("gconftool-2 --get /system/proxy/ftp_port", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(ftp_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    memset(buf, 0, MAXLEN);
    memset(buf_proxy, 0, MAXLEN);
    INFO("ftp_proxy : %s\n", ftp_proxy);

    output = popen("gconftool-2 --get /system/proxy/socks_host", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(buf_proxy, MAXLEN, "%s", buf);
    }
    pclose(output);

    output = popen("gconftool-2 --get /system/proxy/socks_port", "r");
    if(fscanf(output, "%s", buf) > 0) {
        snprintf(socks_proxy, MAXPROXYLEN, "%s:%s", buf_proxy, buf);
    }
    pclose(output);
    INFO("socks_proxy : %s\n", socks_proxy);
}
#endif

static int getautoproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
    char type[MAXLEN];
    char proxy[MAXLEN];
    char line[MAXLEN];
    FILE *fp_pacfile;
    char *p = NULL;
#if defined(CONFIG_LINUX)
    FILE *output;
    char buf[MAXLEN];

    output = popen("gconftool-2 --get /system/proxy/autoconfig_url", "r");
    if(fscanf(output, "%s", buf) > 0) {
        INFO("pac address: %s\n", buf);
        download_url(buf);
    }
    pclose(output);
#elif defined(CONFIG_WIN32)    
    INFO("pac address: %s\n", (char*)url);
    download_url((char*)url);
#elif defined(CONFIG_DARWIN)
    CFStringRef pacURL = (CFStringRef)CFDictionaryGetValue(proxySettings,
			        kSCPropNetProxiesProxyAutoConfigURLString);
	if (pacURL) {
		char url[MAXLEN] = {};
		CFStringGetCString(pacURL, url, sizeof url, kCFStringEncodingASCII);
                INFO("pac address: %s\n", (char*)url);
		download_url(url);
        }
#endif
    fp_pacfile = fopen(pactempfile, "r");
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

    remove(pactempfile);
    return 0;
}


/**
  @brief    get host proxy server address
  @param    proxy: return value (proxy server address)
  @return always 0
 */
int gethostproxy(char *http_proxy, char *https_proxy, char *ftp_proxy, char *socks_proxy)
{
#if defined(CONFIG_LINUX) 
    char buf[MAXLEN];
    FILE *output;

    output = popen("gconftool-2 --get /system/proxy/mode", "r");
    if(fscanf(output, "%s", buf) > 0) {
        //priority : auto > manual > none       
        if (strcmp(buf, "auto") == 0) {
            INFO("AUTO PROXY MODE\n");
            getautoproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "manual") == 0) {
            INFO("MANUAL PROXY MODE\n");
            getlinuxproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
        }
        else if (strcmp(buf, "none") == 0) {
            INFO("DIRECT PROXY MODE\n");
        }
    }
    pclose(output);

#elif defined(CONFIG_WIN32)
    HKEY hKey;
    int nRet;
    LONG lRet;
    BYTE *proxyenable, *proxyserver;
    char *p;
    char *real_proxy;

    DWORD dwLength = 0;
    nRet = RegOpenKeyEx(HKEY_CURRENT_USER,
            "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings",
            0, KEY_QUERY_VALUE, &hKey);
    if (nRet != ERROR_SUCCESS) {
        ERR("Failed to open registry from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
        return 0;
    }
    //check auto proxy key exists
    lRet = RegQueryValueEx(hKey, "AutoConfigURL", 0, NULL, NULL, &dwLength);
    if (lRet != ERROR_SUCCESS && dwLength == 0) {
        ERR("Failed to query value from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\AutoConfigURL");
    }
    else {
        //if exists
        url = (char*)malloc(dwLength);
        if (url == NULL) {
            ERR( "Failed to allocate a buffer\n");
        }
        else {
            memset(url, 0x00, dwLength);
            lRet = RegQueryValueEx(hKey, "AutoConfigURL", 0, NULL, url, &dwLength);
            if (lRet == ERROR_SUCCESS && dwLength != 0) {
                getautoproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy);
                RegCloseKey(hKey);      
                return 0;
            }
        }
    }
    //check manual proxy key exists
    lRet = RegQueryValueEx(hKey, "ProxyEnable", 0, NULL, NULL, &dwLength);
    if (lRet != ERROR_SUCCESS && dwLength == 0) {
        ERR(stderr, "Failed to query value from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyEnable");
        RegCloseKey(hKey);
        return 0;
    }
    proxyenable = (BYTE*)malloc(dwLength);
    if (proxyenable == NULL) {
        ERR( "Failed to allocate a buffer\n");
        RegCloseKey(hKey);
        return 0;
    }

    lRet = RegQueryValueEx(hKey, "ProxyEnable", 0, NULL, proxyenable, &dwLength);
    if (lRet != ERROR_SUCCESS) {
        free(proxyenable);
        ERR("Failed to query value from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ProxyEnable");
        RegCloseKey(hKey);
        return 0;
    }
    if (*(char*)proxyenable == 0) {
        free(proxyenable);
        RegCloseKey(hKey);      
        return 0;
    }

    dwLength = 0;
    lRet = RegQueryValueEx(hKey, "ProxyServer", 0, NULL, NULL, &dwLength);
    if (lRet != ERROR_SUCCESS && dwLength == 0) {
        ERR("Failed to query value from from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
        RegCloseKey(hKey);      
        return 0;
    }

    proxyserver = (BYTE*)malloc(dwLength);
    if (proxyserver == NULL) {
        ERR( "Failed to allocate a buffer\n");
        RegCloseKey(hKey);      
        return 0;
    }

    memset(proxyserver, 0x00, dwLength);
    lRet = RegQueryValueEx(hKey, "ProxyServer", 0, NULL, proxyserver, &dwLength);
    if (lRet != ERROR_SUCCESS) {
        free(proxyserver);
        ERR( "Failed to query value from from %s\n",
                "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
        RegCloseKey(hKey);
        return 0;
    }
    
    if((char*)proxyserver != NULL) {
        INFO("proxy value: %s\n", (char*)proxyserver);
        real_proxy = malloc(MAXLEN);
        
        for(p = strtok((char*)proxyserver, ";"); p; p = strtok(NULL, ";")){
            if(strstr(p, HTTP_PROTOCOL)) {
                remove_string(p, real_proxy, HTTP_PROTOCOL);
                strcpy(http_proxy, real_proxy);
            }
            else if(strstr(p, HTTPS_PROTOCOL)) {
                remove_string(p, real_proxy, HTTPS_PROTOCOL);
                strcpy(https_proxy, real_proxy);
            }
            else if(strstr(p, FTP_PROTOCOL)) {
                remove_string(p, real_proxy, FTP_PROTOCOL);
                strcpy(ftp_proxy, real_proxy);
            }
            else if(strstr(p, SOCKS_PROTOCOL)) {
                remove_string(p, real_proxy, SOCKS_PROTOCOL);
                strcpy(socks_proxy, real_proxy);
            }
            else {
                INFO("all protocol uses the same proxy server: %s\n", p);
                strcpy(http_proxy, p);
                strcpy(https_proxy, p);
                strcpy(ftp_proxy, p);
                strcpy(socks_proxy, p);
            }
        }
        free(real_proxy);
    }
    else {
        INFO("proxy is null\n");
        return 0;
    }
    RegCloseKey(hKey);
#elif defined (CONFIG_DARWIN)
    int ret;
    proxySettings = SCDynamicStoreCopyProxies(NULL);
    if(proxySettings) {
        INFO("AUTO PROXY MODE\n");
        ret = getautoproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy); 
        if(strlen(http_proxy) == 0 && ret < 0) {
            INFO("MANUAL PROXY MODE\n");
	        getmacproxy(http_proxy, https_proxy, ftp_proxy, socks_proxy); 
	    }
    }
#endif
    return 0;
}
