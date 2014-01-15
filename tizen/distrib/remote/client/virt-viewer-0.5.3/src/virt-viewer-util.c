/*
 * Virt Viewer: A virtual machine console viewer
 *
 * Copyright (C) 2007-2012 Red Hat, Inc.
 * Copyright (C) 2009-2012 Daniel P. Berrange
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Daniel P. Berrange <berrange@redhat.com>
 */

#ifdef CONFIG_MARU
#ifdef WIN32
#define off_t long
#define off64_t long long
//#undef __STRICT_ANSI__
#endif
#endif

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include "virt-viewer-util.h"

#ifdef CONFIG_MARU
#define SHARE "/../share"
#ifdef WIN32
#define stat _stat32
#endif
#endif

GtkBuilder *virt_viewer_util_load_ui(const char *name)
{
    struct stat sb;
    GtkBuilder *builder;
    GError *error = NULL;

#ifdef CONFIG_MARU
    gchar *ui_xml = NULL;
    const gchar *pwd = g_getenv("PWD");
    size_t bufsize = strlen(pwd) + strlen(SHARE) + 1;
    gchar *remote_share = malloc(bufsize);
    memset(remote_share, '\0', bufsize);

    g_stpcpy(remote_share, pwd);
    g_strlcat(remote_share, SHARE, bufsize);

    ui_xml = g_build_filename(remote_share, PACKAGE, "ui", name, NULL);

    if (remote_share) {
        g_free(remote_share);
    }
#endif

    builder = gtk_builder_new();
    if (stat(name, &sb) >= 0) {
        gtk_builder_add_from_file(builder, name, &error);
    } else {
#ifdef CONFIG_MARU
        if (gtk_builder_add_from_file(builder, ui_xml, NULL) != 0) {
                g_free(ui_xml);
                return builder;
        }
#endif
        const gchar * const * dirs = g_get_system_data_dirs();
        g_return_val_if_fail(dirs != NULL, NULL);

        while (dirs[0] != NULL) {
            gchar *path = g_build_filename(dirs[0], PACKAGE, "ui", name, NULL);
            if (gtk_builder_add_from_file(builder, path, NULL) != 0) {
                g_free(path);
                break;
            }
            g_free(path);
            dirs++;
        }
        if (dirs[0] == NULL)
            goto failed;
    }

    if (error) {
        g_error("Cannot load UI description %s: %s", name,
                error->message);
        g_clear_error(&error);
        goto failed;
    }

    return builder;
 failed:
    g_error("failed to find UI description file");
    g_object_unref(builder);
    return NULL;
}

int
virt_viewer_util_extract_host(const char *uristr,
                              char **scheme,
                              char **host,
                              char **transport,
                              char **user,
                              int *port)
{
    xmlURIPtr uri;
    char *offset = NULL;

    if (uristr == NULL ||
        !g_ascii_strcasecmp(uristr, "xen"))
        uristr = "xen:///";

    uri = xmlParseURI(uristr);
    g_return_val_if_fail(uri != NULL, 1);

    if (host) {
        if (!uri || !uri->server) {
            *host = g_strdup("localhost");
        } else {
            if (uri->server[0] == '[') {
                gchar *tmp;
                *host = g_strdup(uri->server + 1);
                if ((tmp = strchr(*host, ']')))
                    *tmp = '\0';
            } else {
                *host = g_strdup(uri->server);
            }
        }
    }

    if (user) {
        if (uri->user)
            *user = g_strdup(uri->user);
        else
            *user = NULL;
    }

    if (port)
        *port = uri->port;

    if (uri->scheme)
        offset = strchr(uri->scheme, '+');

    if (transport) {
        if (offset)
            *transport = g_strdup(offset + 1);
        else
            *transport = NULL;
    }

    if (scheme && uri->scheme) {
        if (offset)
            *scheme = g_strndup(uri->scheme, offset - uri->scheme);
        else
            *scheme = g_strdup(uri->scheme);
    }

    xmlFreeURI(uri);
    return 0;
}

/*
 * Local variables:
 *  c-indent-level: 4
 *  c-basic-offset: 4
 *  indent-tabs-mode: nil
 * End:
 */
