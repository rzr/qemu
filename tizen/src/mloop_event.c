/*
 * mainloop_evhandle.c
 *
 * Copyright (C) 2011 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 * Kitae Kim <kt920.kim@samsung.com>
 * GiWoong Kim <giwoong.kim@samsung.com>
 * DoHyung Hong
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

//#include "qobject.h"
#include "qemu-common.h"
#include "hw/usb.h"
#include "hw/irq.h"
#include "mloop_event.h"
#include "ui/console.h"
#include "emul_state.h"
#include "debug_ch.h"
#include "monitor/monitor.h"
#include "hw/pci/pci.h"
#include "sysemu/sysemu.h"

#include "emulator.h"
#include "guest_debug.h"
#include "skin/maruskin_server.h"
#include "hw/maru_virtio_touchscreen.h"
#include "hw/maru_virtio_keyboard.h"

MULTI_DEBUG_CHANNEL(qemu, mloop_event);

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
#define MLOOP_EVTYPE_TOUCH      6
#define MLOOP_EVTYPE_KEYBOARD   7
#define MLOOP_EVTYPE_KBD_ADD    8
#define MLOOP_EVTYPE_KBD_DEL    9
#define MLOOP_EVTYPE_RAMDUMP    10
#define MLOOP_EVTYPE_SDCARD_ATTACH  11
#define MLOOP_EVTYPE_SDCARD_DETACH  12


static struct mloop_evsock mloop = {-1, 0, 0};

static int mloop_evsock_create(struct mloop_evsock *ev)
{
    struct sockaddr sa;
    socklen_t sa_size;
    int ret;
    unsigned long nonblock = 1;

    if (ev == NULL) {
        ERR("null pointer\n");
        return -1;
    }

    ev->sockno = socket(AF_INET, SOCK_DGRAM, 0);
    if ( ev->sockno == -1 ) {
        ERR("socket() failed\n");
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
        ERR("bind() failed\n");
#ifdef _WIN32
        closesocket(ev->sockno);
#else
        close(ev->sockno);
#endif
        ev->sockno = -1;
        ev->status = 0;
        return ret;
    }

    if (ev->portno == 0) {
        memset(&sa, '\0', sizeof(sa));
        getsockname(ev->sockno, (struct sockaddr *) &sa, &sa_size);
        ev->portno = ntohs(((struct sockaddr_in *) &sa)->sin_port);
    }

    ret = connect(ev->sockno, (struct sockaddr *) &sa, sa_size);
    if (ret) {
        ERR("connect() failed\n");
#ifdef _WIN32
        closesocket(ev->sockno);
#else
        close(ev->sockno);
#endif
        ev->sockno = -1;
        ev->status = 0;
        return ret;
    }

    ev->status = MLOOP_EVSOCK_CONNECTED;
    return 0;
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
        ERR("invalid mloop_evsock\n");
        return -1;
    }

    if (p == NULL || p->size <= 0) {
        ERR("invalid mloop_evpack\n");
        return -1;
    }

    do {
        ret = send(ev->sockno, p, p->size, 0);
#ifdef _WIN32
    } while (ret == -1 && (WSAGetLastError() == WSAEWOULDBLOCK));
#else
    } while (ret == -1 && (errno == EWOULDBLOCK || errno == EINTR));
#endif // _WIN32

    return ret;
}

static USBDevice *usbkbd = NULL;
static USBDevice *usbdisk = NULL;
#ifdef TARGET_I386
static PCIDevice *hostkbd = NULL;
static PCIDevice *virtio_sdcard = NULL;
#endif

static void mloop_evhandle_usb_add(char *name)
{
    if (name == NULL) {
        ERR("Packet data for usb device is NULL\n");
        return;
    }

    if (strcmp(name, "keyboard") == 0) {
        if (usbkbd == NULL) {
            usbkbd = usbdevice_create(name);
        } else if (usbkbd->attached == 0) {
            usb_device_attach(usbkbd);
        }
    } else if (strncmp(name, "disk:", 5) == 0) {
        if (usbdisk == NULL) {
            usbdisk = usbdevice_create(name);
        }
    } else {
        WARN("There is no usb-device for %s.\n", name);
     }
}

static void mloop_evhandle_usb_del(char *name)
{
    if (name == NULL) {
        ERR("Packet data for usb device is NULL\n");
        return;
    }

    if (strcmp(name, "keyboard") == 0) {
        if (usbkbd && usbkbd->attached != 0) {
            usb_device_detach(usbkbd);
        }
    } else if (strncmp(name, "disk:", 5) == 0) {
        if (usbdisk) {
            qdev_free(&usbdisk->qdev);
        }
    } else {
        WARN("There is no usb-device for %s.\n", name);
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

static void mloop_evhandle_touch(struct mloop_evpack* pack)
{
    maru_virtio_touchscreen_notify();
}

static void mloop_evhandle_keyboard(long data)
{
    virtio_keyboard_notify((void*)data);
}

#ifdef TARGET_I386
static void mloop_evhandle_kbd_add(char *name)
{
    QDict *qdict;

    TRACE("mloop_evhandle_kbd_add\n");

    if (name == NULL) {
        ERR("packet data is NULL.\n");
        return;
    }

    if (hostkbd) {
        INFO("virtio-keyboard has been already added.\n");
        return;
    }

    qdict = qdict_new();
    qdict_put(qdict, "pci_addr", qstring_from_str("auto"));
    qdict_put(qdict, "type", qstring_from_str(name));

    hostkbd = do_pci_device_hot_add(cur_mon, qdict);
    if (hostkbd) {
        INFO("hot_add keyboard device.\n");
        TRACE("virtio-keyboard device: domain %d, bus %d, slot %d, function %d\n",
                pci_find_domain(hostkbd->bus), pci_bus_num(hostkbd->bus),
                PCI_SLOT(hostkbd->devfn), PCI_FUNC(hostkbd->devfn));
    } else {
        ERR("failed to hot_add keyboard device.\n");
    }

    QDECREF(qdict);
}

static void mloop_evhandle_kbd_del(void)
{
    QDict *qdict;
    int slot = 0;
    char slotbuf[4] = {0,};

    TRACE("mloop_evhandle_kbd_del\n");

    if (!hostkbd) {
        ERR("Failed to remove a keyboard device "
            "because the device has not been created yet.\n");
        return;
    }

    slot = PCI_SLOT(hostkbd->devfn);
    snprintf(slotbuf, sizeof(slotbuf), "%x", slot);
    TRACE("virtio-keyboard slot %s.\n", slotbuf);

    qdict = qdict_new();
    qdict_put(qdict, "pci_addr", qstring_from_str(slotbuf));

    do_pci_device_hot_remove(cur_mon, qdict);
    INFO("hot_remove keyboard.\n");

    hostkbd = NULL;

    QDECREF(qdict);
}

static void mloop_evhandle_sdcard_attach(char *name)
{
    char opts[PATH_MAX];

    INFO("mloop_evhandle_sdcard_attach\n");

    if (name == NULL) {
        ERR("Packet data is NULL.\n");
        return;
    }

    if (virtio_sdcard) {
        ERR("sdcard is already attached.\n");
        return;
    }

    QDict *qdict = qdict_new();

    qdict_put(qdict, "pci_addr", qstring_from_str("auto"));
    qdict_put(qdict, "type", qstring_from_str("storage"));
    snprintf(opts, sizeof(opts), "file=%s,if=virtio", name);
    qdict_put(qdict, "opts", qstring_from_str(opts));

    virtio_sdcard = do_pci_device_hot_add(cur_mon, qdict);
    if (virtio_sdcard) {
        INFO("hot add virtio storage device with [%s]\n", opts);
        INFO("virtio-sdcard device: domain %d, bus %d, slot %d, function %d\n",
                pci_find_domain(virtio_sdcard->bus), pci_bus_num(virtio_sdcard->bus),
                PCI_SLOT(virtio_sdcard->devfn), PCI_FUNC(virtio_sdcard->devfn));
    } else {
        ERR("failed to hot_add sdcard device.\n");

    }
    QDECREF(qdict);
}

static void mloop_evhandle_sdcard_detach(void)
{
    INFO("mloop_evhandle_sdcard_detach\n");

    if (!virtio_sdcard) {
        ERR("sdcard is not attached yet.\n");
        return;
    }

    QDict *qdict = qdict_new();
    int slot = 0;
    char slotbuf[4] = {0,};

    slot = PCI_SLOT(virtio_sdcard->devfn);
    snprintf(slotbuf, sizeof(slotbuf), "%x", slot);
    INFO("virtio-sdcard slot [%d].\n", slot);
    qdict_put(qdict, "pci_addr", qstring_from_str(slotbuf));

    do_pci_device_hot_remove(cur_mon, qdict);

    virtio_sdcard = NULL;

    INFO("hot remove virtio storage device.\n");

    QDECREF(qdict);
}

int mloop_evcmd_get_hostkbd_status(void)
{
    return hostkbd ? 1 : 0;
}
#endif

static void mloop_evhandle_ramdump(struct mloop_evpack* pack)
{
#define MAX_PATH 256
    INFO("dumping...\n");

#if defined(CONFIG_LINUX) && !defined(TARGET_ARM) /* FIXME: Handle ARM ram as list */
    MemoryRegion* rm = get_ram_memory();
    unsigned int size = rm->size.lo;
    char dump_fullpath[MAX_PATH];
    char dump_filename[MAX_PATH];

    char* dump_path = g_path_get_dirname(get_log_path());

    sprintf(dump_filename, "0x%08x%s0x%08x%s", rm->ram_addr, "-",
        rm->ram_addr + size, "_RAM.dump");
    sprintf(dump_fullpath, "%s/%s", dump_path, dump_filename);
    free(dump_path);

    FILE *dump_file = fopen(dump_fullpath, "w+");
    if(!dump_file) {
        fprintf(stderr, "Dump file create failed [%s]\n", dump_fullpath);

        return;
    }

    size_t written;
    written = fwrite(qemu_get_ram_ptr(rm->ram_addr), sizeof(char), size, dump_file);
    fprintf(stdout, "Dump file written [%08x][%d bytes]\n", rm->ram_addr, written);
    if(written != size) {
        fprintf(stderr, "Dump file size error [%d, %d, %d]\n", written, size, errno);
    }

    fprintf(stdout, "Dump file create success [%s, %d bytes]\n", dump_fullpath, size);

    fclose(dump_file);
#endif

    /* notify to skin process */
    notify_ramdump_completed();
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

    if (ret == -1) {
        return;
    }

    if (ret == 0) {
        return;
    }

    switch (pack.type) {
    case MLOOP_EVTYPE_USB_ADD:
        mloop_evhandle_usb_add(pack.data);
        break;
    case MLOOP_EVTYPE_USB_DEL:
        mloop_evhandle_usb_del(pack.data);
        break;
    case MLOOP_EVTYPE_INTR_UP:
        mloop_evhandle_intr_up(*(long*)&pack.data[0]);
        break;
    case MLOOP_EVTYPE_INTR_DOWN:
        mloop_evhandle_intr_down(*(long*)&pack.data[0]);
        break;
    case MLOOP_EVTYPE_TOUCH:
        mloop_evhandle_touch(&pack);
        break;
    case MLOOP_EVTYPE_KEYBOARD:
        mloop_evhandle_keyboard(*(uint64_t*)&pack.data[0]);
        break;
#ifdef TARGET_I386
    case MLOOP_EVTYPE_KBD_ADD:
        mloop_evhandle_kbd_add(pack.data);
        break;
    case MLOOP_EVTYPE_KBD_DEL:
        mloop_evhandle_kbd_del();
        break;
#endif
    case MLOOP_EVTYPE_RAMDUMP:
        mloop_evhandle_ramdump(&pack);
        break;
#ifdef TARGET_I386
    case MLOOP_EVTYPE_SDCARD_ATTACH:
        mloop_evhandle_sdcard_attach(pack.data);
        break;
    case MLOOP_EVTYPE_SDCARD_DETACH:
        mloop_evhandle_sdcard_detach();
        break;
#endif
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

void mloop_evcmd_raise_intr(void *irq)
{
    struct mloop_evpack pack;
    memset((void*)&pack, 0, sizeof(struct mloop_evpack));
    pack.type = MLOOP_EVTYPE_INTR_UP;
    pack.size = 8;
    *(long*)&pack.data[0] = (long)irq;
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_lower_intr(void *irq)
{
    struct mloop_evpack pack;
    memset((void*)&pack, 0, sizeof(struct mloop_evpack));
    pack.type = MLOOP_EVTYPE_INTR_DOWN;
    pack.size = 8;
    *(long*)&pack.data[0] = (long)irq;
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_usbkbd(int on)
{
    struct mloop_evpack pack = { MLOOP_EVTYPE_USB_ADD, 13, "keyboard" };
    if (on == 0) {
        pack.type = MLOOP_EVTYPE_USB_DEL;
    }
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_hostkbd(int on)
{
    struct mloop_evpack pack
        = {MLOOP_EVTYPE_KBD_ADD, 13, "keyboard"};
    if (on == 0) {
        pack.type = MLOOP_EVTYPE_KBD_DEL;
    }
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_usbdisk(char *img)
{
    struct mloop_evpack pack;

    if (img) {
        if (strlen(img) > PACKET_LEN-5) {
            // Need log
            ERR("The length of disk image path is greater than "
                "lenth of maximum packet.\n");
            return;
        }

        pack.type = MLOOP_EVTYPE_USB_ADD;
        pack.size = 5 + sprintf(pack.data, "disk:%s", img);
    } else {
        pack.type = MLOOP_EVTYPE_USB_DEL;
        pack.size = 5 + sprintf(pack.data, "disk:");
    }

    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_sdcard(char *img)
{
    struct mloop_evpack pack;

    if (img) {
        if (strlen(img) > PACKET_LEN-5) {
            // Need log
            ERR("The length of disk image path is greater than "
                "lenth of maximum packet.\n");
            return;
        }

        pack.type = MLOOP_EVTYPE_SDCARD_ATTACH;
        pack.size = 5 + sprintf(pack.data, "%s", img);
    } else {
        pack.type = MLOOP_EVTYPE_SDCARD_DETACH;
        pack.size = 5;
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

void mloop_evcmd_touch(void)
{
    struct mloop_evpack pack;
    memset(&pack, 0, sizeof(struct mloop_evpack));

    pack.type = MLOOP_EVTYPE_TOUCH;
    pack.size = 5;
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_keyboard(void *data)
{
    struct mloop_evpack pack;
    memset(&pack, 0, sizeof(struct mloop_evpack));

    pack.type = MLOOP_EVTYPE_KEYBOARD;
    pack.size = 4 + 8;
    *((VirtIOKeyboard **)pack.data) = (VirtIOKeyboard *)data;
    mloop_evsock_send(&mloop, &pack);
}

void mloop_evcmd_ramdump(void)
{
    struct mloop_evpack pack;
    memset(&pack, 0, sizeof(struct mloop_evpack));

    pack.type = MLOOP_EVTYPE_RAMDUMP;
    pack.size = 5;
    mloop_evsock_send(&mloop, &pack);
}

