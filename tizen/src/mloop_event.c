/*
 * mainloop_evhandle.c
 *
 * Copyright (C) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * DoHyung Hong <don.hong@samsung.com>
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


#ifdef _WIN32
#include <winsock.h>
#define socklen_t     int
#else
#include <netinet/in.h>
#include <sys/ioctl.h>
#endif

#include "qobject.h"
#include "qemu-common.h"
#include "hw/usb.h"
#include "hw/irq.h"
#include "mloop_event.h"
#include "console.h"
#include "emul_state.h"

#define error_report(x, ...)

struct mloop_evsock {
    int sockno;
    unsigned short portno;
    unsigned char status;
};

#define MLOOP_EVSOCK_NULL        0
#define MLOOP_EVSOCK_CREATED    1
#define MLOOP_EVSOCK_NOTBOUND    2
#define MLOOP_EVSOCK_BOUND        3
#define MLOOP_EVSOCK_CONNECTED    4

#define PACKET_LEN 512
struct mloop_evpack {
    short type;
    short size;
    char data[PACKET_LEN-4];
};

#define MLOOP_EVTYPE_USB_ADD    1
#define MLOOP_EVTYPE_USB_DEL    2
#define MLOOP_EVTYPE_INTR_UP    3
#define MLOOP_EVTYPE_INTR_DOWN  4
#define MLOOP_EVTYPE_HWKEY      5
#define MLOOP_EVTYPE_TOUCH      6


static struct mloop_evsock mloop = {-1, 0, 0};

static int mloop_evsock_create(struct mloop_evsock *ev)
{
    struct sockaddr sa;
    socklen_t sa_size;
    int ret;
    unsigned long nonblock = 1;

    if (ev == NULL) {
        error_report("mloop_evsock: null point");
        return -1;
    }

    ev->sockno = socket(AF_INET, SOCK_DGRAM, 0);
    if( ev->sockno == -1 ) {
        error_report("mloop_evsock: socket() failed");
        return -1;
    }

#ifdef _WIN32
    ioctlsocket(ev->sockno, FIONBIO, &nonblock );
#else
    ioctl(ev->sockno, FIONBIO, &nonblock);
#endif // _WIN32

    nonblock = 1 ;
    setsockopt( ev->sockno, SOL_SOCKET, SO_REUSEADDR, (char *)&nonblock, sizeof(nonblock) ) ;

    memset(&sa, '\0', sizeof(sa));
    ((struct sockaddr_in *) &sa)->sin_family = AF_INET;
    memcpy(&((struct sockaddr_in *) &sa)->sin_addr, "\177\000\000\001", 4); // 127.0.0.1
    ((struct sockaddr_in *) &sa)->sin_port = htons(ev->portno);
    sa_size = sizeof(struct sockaddr_in);

    ret = bind(ev->sockno, &sa, sa_size);
    if (ret) {
        error_report("mloop_evsock: bind() failed");
        goto mloop_evsock_init_cleanup;
    }

    if (ev->portno == 0) {
        memset(&sa, '\0', sizeof(sa));
        getsockname(ev->sockno, (struct sockaddr *) &sa, &sa_size);
        ev->portno = ntohs(((struct sockaddr_in *) &sa)->sin_port);
    }

    ret = connect(ev->sockno, (struct sockaddr *) &sa, sa_size);
    if (ret) {
        error_report("mloop_evsock: connect() failed");
        goto mloop_evsock_init_cleanup;
    }

    ev->status = MLOOP_EVSOCK_CONNECTED;
    return 0;

mloop_evsock_init_cleanup:
#ifdef _WIN32
    closesocket(ev->sockno);
#else
    close(ev->sockno);
#endif
    ev->sockno = -1;
    ev->status = 0;
    return ret;
}

static void mloop_evsock_remove(struct mloop_evsock *ev)
{
    if (!ev) {
        return ;
    }

    if (ev->sockno > 0) {
#ifdef _WIN32
        shutdown(ev->sockno, SD_BOTH);
        closesocket(ev->sockno);
#else
        shutdown(ev->sockno, SHUT_RDWR);
        close(ev->sockno);
#endif
        ev->sockno = -1;
        ev->status = 0;
    }
}

static int mloop_evsock_send(struct mloop_evsock *ev, struct mloop_evpack *p)
{
    int ret;

    if (ev == NULL || ev->sockno == -1) {
        error_report("invalid mloop_evsock");
        return -1;
    }

    if (p == NULL || p->size <= 0) {
        error_report("invalid mloop_evpack");
        return -1;
    }

    do {
        ret = send(ev->sockno, p, ntohs(p->size), 0);
#ifdef _WIN32
    } while (ret == -1 && (WSAGetLastError() == WSAEWOULDBLOCK));
#else
    } while (ret == -1 && (errno == EWOULDBLOCK || errno == EINTR));
#endif // _WIN32

    return ret;
}

static USBDevice *usbkbd = NULL;
static USBDevice *usbdisk = NULL;
static void mloop_evhandle_usb_add(char *name)
{
    if (name == NULL) {
        return;
    }

    if (strcmp(name, "keyboard") == 0) {
        if (usbkbd == NULL) {
            usbkbd = usbdevice_create(name);
        }
        else if (usbkbd->attached == 0) {
            usb_device_attach(usbkbd);
        }
    }
    else if (strncmp(name, "disk:", 5) == 0) {
        if (usbdisk == NULL) {
            usbdisk = usbdevice_create(name);
        }
    }
}

static void mloop_evhandle_usb_del(char *name)
{
    if (name == NULL) {
        return;
    }

    if (strcmp(name, "keyboard") == 0) {
        if (usbkbd && usbkbd->attached != 0) {
            usb_device_detach(usbkbd);
        }
    }
    else if (strncmp(name, "disk:", 5) == 0) {
        if (usbdisk) {
            qdev_free(&usbdisk->qdev);
        }
    }
}

static void mloop_evhandle_intr_up(long data)
{
    if (data == 0) {
        return;
    }

    qemu_irq_raise((qemu_irq)data);
}

static void mloop_evhandle_intr_down(long data)
{
    if (data == 0) {
        return;
    }

    qemu_irq_lower((qemu_irq)data);
}

static void mloop_evhandle_hwkey(struct mloop_evpack* pack)
{
    int event_type = 0;
    int keycode = 0;

    memcpy(&event_type, pack->data, sizeof(int));
    memcpy(&keycode, pack->data + sizeof(int), sizeof(int));

    if (KEY_PRESSED == event_type) {
        if (kbd_mouse_is_absolute()) {
            ps2kbd_put_keycode(keycode & 0x7f);
        }
    } else if ( KEY_RELEASED == event_type ) {
        if (kbd_mouse_is_absolute()) {
            ps2kbd_put_keycode(keycode | 0x80);
        }
    } else {
        fprintf(stderr, "Unknown hardkey event type.[event_type:%d, keycode:%d]", event_type, keycode);
    }
}

static void mloop_evhandle_touch(struct mloop_evpack* pack)
{
    maru_virtio_touchscreen_notify();
}

static void mloop_evcb_recv(struct mloop_evsock *ev)
{
    struct mloop_evpack pack;
    int ret;

    do {
        ret = recv(ev->sockno, (void *)&pack, sizeof(pack), 0);
#ifdef _WIN32
    } while (ret == -1 && WSAGetLastError() == WSAEINTR);
#else
    } while (ret == -1 && errno == EINTR);
#endif // _WIN32

    if (ret == -1 ) {
        return;
    }

    if (ret == 0 ) {
        return;
    }

    pack.type = ntohs(pack.type);
    pack.size = ntohs(pack.size);

    switch (pack.type) {
    case MLOOP_EVTYPE_USB_ADD:
        mloop_evhandle_usb_add(pack.data);
        break;
    case MLOOP_EVTYPE_USB_DEL:
        mloop_evhandle_usb_del(pack.data);
        break;
    case MLOOP_EVTYPE_INTR_UP:
        mloop_evhandle_intr_up(ntohl(*(long*)&pack.data[0]));
        break;
    case MLOOP_EVTYPE_INTR_DOWN:
        mloop_evhandle_intr_down(ntohl(*(long*)&pack.data[0]));
        break;
    case MLOOP_EVTYPE_HWKEY:
        mloop_evhandle_hwkey(&pack);
        break;
    case MLOOP_EVTYPE_TOUCH:
        mloop_evhandle_touch(&pack);
        break;
    default:
        break;
    }
}

void mloop_ev_init(void)
{
    int ret = mloop_evsock_create(&mloop);
    if (ret == 0) {
        qemu_set_fd_handler(mloop.sockno, (IOHandler *)mloop_evcb_recv, NULL, &mloop);
    }
}

void mloop_ev_stop(void)
{
    qemu_set_fd_handler(mloop.sockno, NULL, NULL, NULL);
    mloop_evsock_remove(&mloop);
}

void mloop_evcmd_usbkbd(int on)
{
    struct mloop_evpack pack = { htons(MLOOP_EVTYPE_USB_ADD), htons(13), "keyboard" };
    if (on == 0)
        pack.type = htons(MLOOP_EVTYPE_USB_DEL);
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_usbdisk(char *img)
{
    struct mloop_evpack pack;

    if (img) {
        if (strlen(img) > PACKET_LEN-5) {
            // Need log
            return;
        }

        pack.type = htons(MLOOP_EVTYPE_USB_ADD);
        pack.size = htons(5 + sprintf(pack.data, "disk:%s", img));
    }
    else {
        pack.type = htons(MLOOP_EVTYPE_USB_DEL);
        pack.size = htons(5 + sprintf(pack.data, "disk:"));
    }

    mloop_evsock_send(&mloop, &pack);
}

int mloop_evcmd_get_usbkbd_status(void)
{
    return (usbkbd && usbkbd->attached ? 1 : 0);
}

void mloop_evcmd_set_usbkbd(void *dev)
{
    usbkbd = (USBDevice *)dev;
}

void mloop_evcmd_set_usbdisk(void *dev)
{
    usbdisk = (USBDevice *)dev;
}

void mloop_evcmd_raise_intr(void *irq)
{
    struct mloop_evpack pack;
    memset((void*)&pack, 0, sizeof(struct mloop_evpack));
    pack.type = htons(MLOOP_EVTYPE_INTR_UP);
    pack.size = htons(8);
    *(long*)&pack.data[0] = htonl((long)irq);
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_lower_intr(void *irq)
{
    struct mloop_evpack pack;
    memset((void*)&pack, 0, sizeof(struct mloop_evpack));
    pack.type = htons(MLOOP_EVTYPE_INTR_DOWN);
    pack.size = htons(8);
    *(long*)&pack.data[0] = htonl((long)irq);
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_hwkey(int event_type, int keycode)
{
    struct mloop_evpack pack;

    pack.type = htons(MLOOP_EVTYPE_HWKEY);
    pack.size = htons(5 + 8); //TODO: ?

    memcpy(pack.data, &event_type, sizeof(int));
    memcpy(pack.data + sizeof(int), &keycode, sizeof(int));
    //pack.data = htons(pack.data);

    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_touch(void)
{
    struct mloop_evpack pack;
    memset(&pack, 0, sizeof(struct mloop_evpack));

    pack.type = htons(MLOOP_EVTYPE_TOUCH);
    pack.size = htons(5);
    mloop_evsock_send(&mloop, &pack);
}

