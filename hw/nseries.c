/*
 * Nokia N-series internet tablets.
 *
 * Copyright (C) 2007 Nokia Corporation
 * Written by Andrzej Zaborowski <andrew@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "qemu-common.h"
#include "sysemu.h"
#include "omap.h"
#include "arm-misc.h"
#include "irq.h"
#include "console.h"
#include "boards.h"
#include "i2c.h"
#include "devices.h"
#include "flash.h"
#include "hw.h"

/* Nokia N8x0 support */
struct n800_s {
    struct omap_mpu_state_s *cpu;

    struct rfbi_chip_s blizzard;
    struct uwire_slave_s *ts;
    i2c_bus *i2c;

    int keymap[0x80];

    struct tusb_s *usb;
    void *retu;
    void *tahvo;
};

/* GPIO pins */
#define N800_TUSB_ENABLE_GPIO		0
#define N800_MMC2_WP_GPIO		8
#define N800_UNKNOWN_GPIO0		9	/* out */
#define N800_UNKNOWN_GPIO1		10	/* out */
#define N800_CAM_TURN_GPIO		12
#define N800_BLIZZARD_POWERDOWN_GPIO	15
#define N800_MMC1_WP_GPIO		23
#define N8X0_ONENAND_GPIO		26
#define N800_UNKNOWN_GPIO2		53	/* out */
#define N8X0_TUSB_INT_GPIO		58
#define N800_BT_WKUP_GPIO		61
#define N800_STI_GPIO			62
#define N8X0_CBUS_SEL_GPIO		64
#define N8X0_CBUS_CLK_GPIO		65	/* sure? */
#define N8X0_CBUS_DAT_GPIO		66
#define N800_WLAN_IRQ_GPIO		87
#define N800_BT_RESET_GPIO		92
#define N800_TEA5761_CS_GPIO		93
#define N800_UNKNOWN_GPIO		94
#define N800_CAM_ACT_GPIO		95
#define N800_MMC_CS_GPIO		96
#define N800_WLAN_PWR_GPIO		97
#define N8X0_BT_HOST_WKUP_GPIO		98
#define N800_UNKNOWN_GPIO3		101	/* out */
#define N810_KB_LOCK_GPIO		102
#define N800_TSC_TS_GPIO		103
#define N810_TSC2005_GPIO		106
#define N800_HEADPHONE_GPIO		107
#define N8X0_RETU_GPIO			108
#define N800_TSC_KP_IRQ_GPIO		109
#define N810_KEYBOARD_GPIO		109
#define N800_BAT_COVER_GPIO		110
#define N810_SLIDE_GPIO			110
#define N8X0_TAHVO_GPIO			111
#define N800_UNKNOWN_GPIO4		112	/* out */
#define N810_TSC_RESET_GPIO		118
#define N800_TSC_RESET_GPIO		119	/* ? */
#define N8X0_TMP105_GPIO		125

/* Config */
#define XLDR_LL_UART			1

/* Addresses on the I2C bus */
#define N8X0_TMP105_ADDR		0x48
#define N8X0_MENELAUS_ADDR		0x72

/* Chipselects on GPMC NOR interface */
#define N8X0_ONENAND_CS			0
#define N8X0_USB_ASYNC_CS		1
#define N8X0_USB_SYNC_CS		4

static void n800_mmc_cs_cb(void *opaque, int line, int level)
{
    /* TODO: this seems to actually be connected to the menelaus, to
     * which also both MMC slots connect.  */
    omap_mmc_enable((struct omap_mmc_s *) opaque, !level);

    printf("%s: MMC slot %i active\n", __FUNCTION__, level + 1);
}

static void n800_gpio_setup(struct n800_s *s)
{
    qemu_irq *mmc_cs = qemu_allocate_irqs(n800_mmc_cs_cb, s->cpu->mmc, 1);
    omap2_gpio_out_set(s->cpu->gpif, N800_MMC_CS_GPIO, mmc_cs[0]);

    qemu_irq_lower(omap2_gpio_in_get(s->cpu->gpif, N800_BAT_COVER_GPIO)[0]);
}

static void n8x0_nand_setup(struct n800_s *s)
{
    /* Either ec40xx or ec48xx are OK for the ID */
    omap_gpmc_attach(s->cpu->gpmc, N8X0_ONENAND_CS, 0, onenand_base_update,
                    onenand_base_unmap,
                    onenand_init(0xec4800, 1,
                            omap2_gpio_in_get(s->cpu->gpif,
                                    N8X0_ONENAND_GPIO)[0]));
}

static void n800_i2c_setup(struct n800_s *s)
{
    qemu_irq tmp_irq = omap2_gpio_in_get(s->cpu->gpif, N8X0_TMP105_GPIO)[0];

    /* Attach the CPU on one end of our I2C bus.  */
    s->i2c = omap_i2c_bus(s->cpu->i2c[0]);

    /* Attach a menelaus PM chip */
    i2c_set_slave_address(
                    twl92230_init(s->i2c,
                            s->cpu->irq[0][OMAP_INT_24XX_SYS_NIRQ]),
                    N8X0_MENELAUS_ADDR);

    /* Attach a TMP105 PM chip (A0 wired to ground) */
    i2c_set_slave_address(tmp105_init(s->i2c, tmp_irq), N8X0_TMP105_ADDR);
}

/* Touchscreen and keypad controller */
#define RETU_KEYCODE	61	/* F3 */

static void n800_key_event(void *opaque, int keycode)
{
    struct n800_s *s = (struct n800_s *) opaque;
    int code = s->keymap[keycode & 0x7f];

    if (code == -1) {
        if ((keycode & 0x7f) == RETU_KEYCODE)
            retu_key_event(s->retu, !(keycode & 0x80));
        return;
    }

    tsc210x_key_event(s->ts, code, !(keycode & 0x80));
}

static const int n800_keys[16] = {
    -1,
    72,	/* Up */
    63,	/* Home (F5) */
    -1,
    75,	/* Left */
    28,	/* Enter */
    77,	/* Right */
    -1,
    1,	/* Cycle (ESC) */
    80,	/* Down */
    62,	/* Menu (F4) */
    -1,
    66,	/* Zoom- (F8) */
    64,	/* FS (F6) */
    65,	/* Zoom+ (F7) */
    -1,
};

static struct mouse_transform_info_s n800_pointercal = {
    .x = 800,
    .y = 480,
    .a = { 14560, -68, -3455208, -39, -9621, 35152972, 65536 },
};

static void n800_tsc_setup(struct n800_s *s)
{
    int i;

    /* XXX: are the three pins inverted inside the chip between the
     * tsc and the cpu (N4111)?  */
    qemu_irq penirq = 0;	/* NC */
    qemu_irq kbirq = omap2_gpio_in_get(s->cpu->gpif, N800_TSC_KP_IRQ_GPIO)[0];
    qemu_irq dav = omap2_gpio_in_get(s->cpu->gpif, N800_TSC_TS_GPIO)[0];

    s->ts = tsc2301_init(penirq, kbirq, dav, 0);

    for (i = 0; i < 0x80; i ++)
        s->keymap[i] = -1;
    for (i = 0; i < 0x10; i ++)
        if (n800_keys[i] >= 0)
            s->keymap[n800_keys[i]] = i;

    qemu_add_kbd_event_handler(n800_key_event, s);

    tsc210x_set_transform(s->ts, &n800_pointercal);
}

/* LCD MIPI DBI-C controller (URAL) */
struct mipid_s {
    int resp[4];
    int param[4];
    int p;
    int pm;
    int cmd;

    int sleep;
    int booster;
    int te;
    int selfcheck;
    int partial;
    int normal;
    int vscr;
    int invert;
    int onoff;
    int gamma;
    uint32_t id;
};

static void mipid_reset(struct mipid_s *s)
{
    if (!s->sleep)
        fprintf(stderr, "%s: Display off\n", __FUNCTION__);

    s->pm = 0;
    s->cmd = 0;

    s->sleep = 1;
    s->booster = 0;
    s->selfcheck =
            (1 << 7) |	/* Register loading OK.  */
            (1 << 5) |	/* The chip is attached.  */
            (1 << 4);	/* Display glass still in one piece.  */
    s->te = 0;
    s->partial = 0;
    s->normal = 1;
    s->vscr = 0;
    s->invert = 0;
    s->onoff = 1;
    s->gamma = 0;
}

static uint32_t mipid_txrx(void *opaque, uint32_t cmd)
{
    struct mipid_s *s = (struct mipid_s *) opaque;
    uint8_t ret;

    if (s->p >= sizeof(s->resp) / sizeof(*s->resp))
        ret = 0;
    else
        ret = s->resp[s->p ++];
    if (s->pm --> 0)
        s->param[s->pm] = cmd;
    else
        s->cmd = cmd;

    switch (s->cmd) {
    case 0x00:	/* NOP */
        break;

    case 0x01:	/* SWRESET */
        mipid_reset(s);
        break;

    case 0x02:	/* BSTROFF */
        s->booster = 0;
        break;
    case 0x03:	/* BSTRON */
        s->booster = 1;
        break;

    case 0x04:	/* RDDID */
        s->p = 0;
        s->resp[0] = (s->id >> 16) & 0xff;
        s->resp[1] = (s->id >>  8) & 0xff;
        s->resp[2] = (s->id >>  0) & 0xff;
        break;

    case 0x06:	/* RD_RED */
    case 0x07:	/* RD_GREEN */
        /* XXX the bootloader sometimes issues RD_BLUE meaning RDDID so
         * for the bootloader one needs to change this.  */
    case 0x08:	/* RD_BLUE */
        s->p = 0;
        /* TODO: return first pixel components */
        s->resp[0] = 0x01;
        break;

    case 0x09:	/* RDDST */
        s->p = 0;
        s->resp[0] = s->booster << 7;
        s->resp[1] = (5 << 4) | (s->partial << 2) |
                (s->sleep << 1) | s->normal;
        s->resp[2] = (s->vscr << 7) | (s->invert << 5) |
                (s->onoff << 2) | (s->te << 1) | (s->gamma >> 2);
        s->resp[3] = s->gamma << 6;
        break;

    case 0x0a:	/* RDDPM */
        s->p = 0;
        s->resp[0] = (s->onoff << 2) | (s->normal << 3) | (s->sleep << 4) |
                (s->partial << 5) | (s->sleep << 6) | (s->booster << 7);
        break;
    case 0x0b:	/* RDDMADCTR */
        s->p = 0;
        s->resp[0] = 0;
        break;
    case 0x0c:	/* RDDCOLMOD */
        s->p = 0;
        s->resp[0] = 5;	/* 65K colours */
        break;
    case 0x0d:	/* RDDIM */
        s->p = 0;
        s->resp[0] = (s->invert << 5) | (s->vscr << 7) | s->gamma;
        break;
    case 0x0e:	/* RDDSM */
        s->p = 0;
        s->resp[0] = s->te << 7;
        break;
    case 0x0f:	/* RDDSDR */
        s->p = 0;
        s->resp[0] = s->selfcheck;
        break;

    case 0x10:	/* SLPIN */
        s->sleep = 1;
        break;
    case 0x11:	/* SLPOUT */
        s->sleep = 0;
        s->selfcheck ^= 1 << 6;	/* POFF self-diagnosis Ok */
        break;

    case 0x12:	/* PTLON */
        s->partial = 1;
        s->normal = 0;
        s->vscr = 0;
        break;
    case 0x13:	/* NORON */
        s->partial = 0;
        s->normal = 1;
        s->vscr = 0;
        break;

    case 0x20:	/* INVOFF */
        s->invert = 0;
        break;
    case 0x21:	/* INVON */
        s->invert = 1;
        break;

    case 0x22:	/* APOFF */
    case 0x23:	/* APON */
        goto bad_cmd;

    case 0x25:	/* WRCNTR */
        if (s->pm < 0)
            s->pm = 1;
        goto bad_cmd;

    case 0x26:	/* GAMSET */
        if (!s->pm)
            s->gamma = ffs(s->param[0] & 0xf) - 1;
        else if (s->pm < 0)
            s->pm = 1;
        break;

    case 0x28:	/* DISPOFF */
        s->onoff = 0;
        fprintf(stderr, "%s: Display off\n", __FUNCTION__);
        break;
    case 0x29:	/* DISPON */
        s->onoff = 1;
        fprintf(stderr, "%s: Display on\n", __FUNCTION__);
        break;

    case 0x2a:	/* CASET */
    case 0x2b:	/* RASET */
    case 0x2c:	/* RAMWR */
    case 0x2d:	/* RGBSET */
    case 0x2e:	/* RAMRD */
    case 0x30:	/* PTLAR */
    case 0x33:	/* SCRLAR */
        goto bad_cmd;

    case 0x34:	/* TEOFF */
        s->te = 0;
        break;
    case 0x35:	/* TEON */
        if (!s->pm)
            s->te = 1;
        else if (s->pm < 0)
            s->pm = 1;
        break;

    case 0x36:	/* MADCTR */
        goto bad_cmd;

    case 0x37:	/* VSCSAD */
        s->partial = 0;
        s->normal = 0;
        s->vscr = 1;
        break;

    case 0x38:	/* IDMOFF */
    case 0x39:	/* IDMON */
    case 0x3a:	/* COLMOD */
        goto bad_cmd;

    case 0xb0:	/* CLKINT / DISCTL */
    case 0xb1:	/* CLKEXT */
        if (s->pm < 0)
            s->pm = 2;
        break;

    case 0xb4:	/* FRMSEL */
        break;

    case 0xb5:	/* FRM8SEL */
    case 0xb6:	/* TMPRNG / INIESC */
    case 0xb7:	/* TMPHIS / NOP2 */
    case 0xb8:	/* TMPREAD / MADCTL */
    case 0xba:	/* DISTCTR */
    case 0xbb:	/* EPVOL */
        goto bad_cmd;

    case 0xbd:	/* Unknown */
        s->p = 0;
        s->resp[0] = 0;
        s->resp[1] = 1;
        break;

    case 0xc2:	/* IFMOD */
        if (s->pm < 0)
            s->pm = 2;
        break;

    case 0xc6:	/* PWRCTL */
    case 0xc7:	/* PPWRCTL */
    case 0xd0:	/* EPWROUT */
    case 0xd1:	/* EPWRIN */
    case 0xd4:	/* RDEV */
    case 0xd5:	/* RDRR */
        goto bad_cmd;

    case 0xda:	/* RDID1 */
        s->p = 0;
        s->resp[0] = (s->id >> 16) & 0xff;
        break;
    case 0xdb:	/* RDID2 */
        s->p = 0;
        s->resp[0] = (s->id >>  8) & 0xff;
        break;
    case 0xdc:	/* RDID3 */
        s->p = 0;
        s->resp[0] = (s->id >>  0) & 0xff;
        break;

    default:
    bad_cmd:
        fprintf(stderr, "%s: unknown command %02x\n", __FUNCTION__, s->cmd);
        break;
    }

    return ret;
}

static void *mipid_init(void)
{
    struct mipid_s *s = (struct mipid_s *) qemu_mallocz(sizeof(*s));

    s->id = 0x838f03;
    mipid_reset(s);

    return s;
}

static void n800_spi_setup(struct n800_s *s)
{
    void *tsc2301 = s->ts->opaque;
    void *mipid = mipid_init();

    omap_mcspi_attach(s->cpu->mcspi[0], tsc210x_txrx, tsc2301, 0);
    omap_mcspi_attach(s->cpu->mcspi[0], mipid_txrx, mipid, 1);
}

/* This task is normally performed by the bootloader.  If we're loading
 * a kernel directly, we need to enable the Blizzard ourselves.  */
static void n800_dss_init(struct rfbi_chip_s *chip)
{
    uint8_t *fb_blank;

    chip->write(chip->opaque, 0, 0x2a);		/* LCD Width register */
    chip->write(chip->opaque, 1, 0x64);
    chip->write(chip->opaque, 0, 0x2c);		/* LCD HNDP register */
    chip->write(chip->opaque, 1, 0x1e);
    chip->write(chip->opaque, 0, 0x2e);		/* LCD Height 0 register */
    chip->write(chip->opaque, 1, 0xe0);
    chip->write(chip->opaque, 0, 0x30);		/* LCD Height 1 register */
    chip->write(chip->opaque, 1, 0x01);
    chip->write(chip->opaque, 0, 0x32);		/* LCD VNDP register */
    chip->write(chip->opaque, 1, 0x06);
    chip->write(chip->opaque, 0, 0x68);		/* Display Mode register */
    chip->write(chip->opaque, 1, 1);		/* Enable bit */

    chip->write(chip->opaque, 0, 0x6c);	
    chip->write(chip->opaque, 1, 0x00);		/* Input X Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Input X Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Input Y Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Input Y Start Position */
    chip->write(chip->opaque, 1, 0x1f);		/* Input X End Position */
    chip->write(chip->opaque, 1, 0x03);		/* Input X End Position */
    chip->write(chip->opaque, 1, 0xdf);		/* Input Y End Position */
    chip->write(chip->opaque, 1, 0x01);		/* Input Y End Position */
    chip->write(chip->opaque, 1, 0x00);		/* Output X Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Output X Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Output Y Start Position */
    chip->write(chip->opaque, 1, 0x00);		/* Output Y Start Position */
    chip->write(chip->opaque, 1, 0x1f);		/* Output X End Position */
    chip->write(chip->opaque, 1, 0x03);		/* Output X End Position */
    chip->write(chip->opaque, 1, 0xdf);		/* Output Y End Position */
    chip->write(chip->opaque, 1, 0x01);		/* Output Y End Position */
    chip->write(chip->opaque, 1, 0x01);		/* Input Data Format */
    chip->write(chip->opaque, 1, 0x01);		/* Data Source Select */

    fb_blank = memset(qemu_malloc(800 * 480 * 2), 0xff, 800 * 480 * 2);
    /* Display Memory Data Port */
    chip->block(chip->opaque, 1, fb_blank, 800 * 480 * 2, 800);
    free(fb_blank);
}

static void n800_dss_setup(struct n800_s *s, DisplayState *ds)
{
    s->blizzard.opaque = s1d13745_init(0, ds);
    s->blizzard.block = s1d13745_write_block;
    s->blizzard.write = s1d13745_write;
    s->blizzard.read = s1d13745_read;

    omap_rfbi_attach(s->cpu->dss, 0, &s->blizzard);
}

static void n800_cbus_setup(struct n800_s *s)
{
    qemu_irq dat_out = omap2_gpio_in_get(s->cpu->gpif, N8X0_CBUS_DAT_GPIO)[0];
    qemu_irq retu_irq = omap2_gpio_in_get(s->cpu->gpif, N8X0_RETU_GPIO)[0];
    qemu_irq tahvo_irq = omap2_gpio_in_get(s->cpu->gpif, N8X0_TAHVO_GPIO)[0];

    struct cbus_s *cbus = cbus_init(dat_out);

    omap2_gpio_out_set(s->cpu->gpif, N8X0_CBUS_CLK_GPIO, cbus->clk);
    omap2_gpio_out_set(s->cpu->gpif, N8X0_CBUS_DAT_GPIO, cbus->dat);
    omap2_gpio_out_set(s->cpu->gpif, N8X0_CBUS_SEL_GPIO, cbus->sel);

    cbus_attach(cbus, s->retu = retu_init(retu_irq, 1));
    cbus_attach(cbus, s->tahvo = tahvo_init(tahvo_irq, 1));
}

static void n800_usb_power_cb(void *opaque, int line, int level)
{
    struct n800_s *s = opaque;

    tusb6010_power(s->usb, level);
}

static void n800_usb_setup(struct n800_s *s)
{
    qemu_irq tusb_irq = omap2_gpio_in_get(s->cpu->gpif, N8X0_TUSB_INT_GPIO)[0];
    qemu_irq tusb_pwr = qemu_allocate_irqs(n800_usb_power_cb, s, 1)[0];
    struct tusb_s *tusb = tusb6010_init(tusb_irq);

    /* Using the NOR interface */
    omap_gpmc_attach(s->cpu->gpmc, N8X0_USB_ASYNC_CS,
                    tusb6010_async_io(tusb), 0, 0, tusb);
    omap_gpmc_attach(s->cpu->gpmc, N8X0_USB_SYNC_CS,
                    tusb6010_sync_io(tusb), 0, 0, tusb);

    s->usb = tusb;
    omap2_gpio_out_set(s->cpu->gpif, N800_TUSB_ENABLE_GPIO, tusb_pwr);
}

/* This task is normally performed by the bootloader.  If we're loading
 * a kernel directly, we need to set up GPMC mappings ourselves.  */
static void n800_gpmc_init(struct n800_s *s)
{
    uint32_t config7 =
            (0xf << 8) |	/* MASKADDRESS */
            (1 << 6) |		/* CSVALID */
            (4 << 0);		/* BASEADDRESS */

    cpu_physical_memory_write(0x6800a078,		/* GPMC_CONFIG7_0 */
                    (void *) &config7, sizeof(config7));
}

/* Setup sequence done by the bootloader */
static void n800_boot_init(void *opaque)
{
    struct n800_s *s = (struct n800_s *) opaque;
    uint32_t buf;

    /* PRCM setup */
#define omap_writel(addr, val)	\
    buf = (val);			\
    cpu_physical_memory_write(addr, (void *) &buf, sizeof(buf))

    omap_writel(0x48008060, 0x41);		/* PRCM_CLKSRC_CTRL */
    omap_writel(0x48008070, 1);			/* PRCM_CLKOUT_CTRL */
    omap_writel(0x48008078, 0);			/* PRCM_CLKEMUL_CTRL */
    omap_writel(0x48008090, 0);			/* PRCM_VOLTSETUP */
    omap_writel(0x48008094, 0);			/* PRCM_CLKSSETUP */
    omap_writel(0x48008098, 0);			/* PRCM_POLCTRL */
    omap_writel(0x48008140, 2);			/* CM_CLKSEL_MPU */
    omap_writel(0x48008148, 0);			/* CM_CLKSTCTRL_MPU */
    omap_writel(0x48008158, 1);			/* RM_RSTST_MPU */
    omap_writel(0x480081c8, 0x15);		/* PM_WKDEP_MPU */
    omap_writel(0x480081d4, 0x1d4);		/* PM_EVGENCTRL_MPU */
    omap_writel(0x480081d8, 0);			/* PM_EVEGENONTIM_MPU */
    omap_writel(0x480081dc, 0);			/* PM_EVEGENOFFTIM_MPU */
    omap_writel(0x480081e0, 0xc);		/* PM_PWSTCTRL_MPU */
    omap_writel(0x48008200, 0x047e7ff7);	/* CM_FCLKEN1_CORE */
    omap_writel(0x48008204, 0x00000004);	/* CM_FCLKEN2_CORE */
    omap_writel(0x48008210, 0x047e7ff1);	/* CM_ICLKEN1_CORE */
    omap_writel(0x48008214, 0x00000004);	/* CM_ICLKEN2_CORE */
    omap_writel(0x4800821c, 0x00000000);	/* CM_ICLKEN4_CORE */
    omap_writel(0x48008230, 0);			/* CM_AUTOIDLE1_CORE */
    omap_writel(0x48008234, 0);			/* CM_AUTOIDLE2_CORE */
    omap_writel(0x48008238, 7);			/* CM_AUTOIDLE3_CORE */
    omap_writel(0x4800823c, 0);			/* CM_AUTOIDLE4_CORE */
    omap_writel(0x48008240, 0x04360626);	/* CM_CLKSEL1_CORE */
    omap_writel(0x48008244, 0x00000014);	/* CM_CLKSEL2_CORE */
    omap_writel(0x48008248, 0);			/* CM_CLKSTCTRL_CORE */
    omap_writel(0x48008300, 0x00000000);	/* CM_FCLKEN_GFX */
    omap_writel(0x48008310, 0x00000000);	/* CM_ICLKEN_GFX */
    omap_writel(0x48008340, 0x00000001);	/* CM_CLKSEL_GFX */
    omap_writel(0x48008400, 0x00000004);	/* CM_FCLKEN_WKUP */
    omap_writel(0x48008410, 0x00000004);	/* CM_ICLKEN_WKUP */
    omap_writel(0x48008440, 0x00000000);	/* CM_CLKSEL_WKUP */
    omap_writel(0x48008500, 0x000000cf);	/* CM_CLKEN_PLL */
    omap_writel(0x48008530, 0x0000000c);	/* CM_AUTOIDLE_PLL */
    omap_writel(0x48008540,			/* CM_CLKSEL1_PLL */
                    (0x78 << 12) | (6 << 8));
    omap_writel(0x48008544, 2);			/* CM_CLKSEL2_PLL */

    /* GPMC setup */
    n800_gpmc_init(s);

    /* Video setup */
    n800_dss_init(&s->blizzard);

    /* CPU setup */
    s->cpu->env->regs[15] = s->cpu->env->boot_info->loader_start;
    s->cpu->env->GE = 0x5;
}

#define OMAP_TAG_NOKIA_BT	0x4e01
#define OMAP_TAG_WLAN_CX3110X	0x4e02
#define OMAP_TAG_CBUS		0x4e03
#define OMAP_TAG_EM_ASIC_BB5	0x4e04

static int n800_atag_setup(struct arm_boot_info *info, void *p)
{
    uint8_t *b;
    uint16_t *w;
    uint32_t *l;

    w = p;

    stw_raw(w ++, OMAP_TAG_UART);		/* u16 tag */
    stw_raw(w ++, 4);				/* u16 len */
    stw_raw(w ++, (1 << 2) | (1 << 1) | (1 << 0)); /* uint enabled_uarts */
    w ++;

    stw_raw(w ++, OMAP_TAG_EM_ASIC_BB5);	/* u16 tag */
    stw_raw(w ++, 4);				/* u16 len */
    stw_raw(w ++, N8X0_RETU_GPIO);		/* s16 retu_irq_gpio */
    stw_raw(w ++, N8X0_TAHVO_GPIO);		/* s16 tahvo_irq_gpio */

    stw_raw(w ++, OMAP_TAG_CBUS);		/* u16 tag */
    stw_raw(w ++, 8);				/* u16 len */
    stw_raw(w ++, N8X0_CBUS_CLK_GPIO);		/* s16 clk_gpio */
    stw_raw(w ++, N8X0_CBUS_DAT_GPIO);		/* s16 dat_gpio */
    stw_raw(w ++, N8X0_CBUS_SEL_GPIO);		/* s16 sel_gpio */
    w ++;

    stw_raw(w ++, OMAP_TAG_GPIO_SWITCH);	/* u16 tag */
    stw_raw(w ++, 20);				/* u16 len */
    strcpy((void *) w, "bat_cover");		/* char name[12] */
    w += 6;
    stw_raw(w ++, N800_BAT_COVER_GPIO);		/* u16 gpio */
    stw_raw(w ++, 0x01);
    stw_raw(w ++, 0);
    stw_raw(w ++, 0);

    stw_raw(w ++, OMAP_TAG_GPIO_SWITCH);	/* u16 tag */
    stw_raw(w ++, 20);				/* u16 len */
    strcpy((void *) w, "cam_act");		/* char name[12] */
    w += 6;
    stw_raw(w ++, N800_CAM_ACT_GPIO);		/* u16 gpio */
    stw_raw(w ++, 0x20);
    stw_raw(w ++, 0);
    stw_raw(w ++, 0);

    stw_raw(w ++, OMAP_TAG_GPIO_SWITCH);	/* u16 tag */
    stw_raw(w ++, 20);				/* u16 len */
    strcpy((void *) w, "cam_turn");		/* char name[12] */
    w += 6;
    stw_raw(w ++, N800_CAM_TURN_GPIO);		/* u16 gpio */
    stw_raw(w ++, 0x21);
    stw_raw(w ++, 0);
    stw_raw(w ++, 0);

    stw_raw(w ++, OMAP_TAG_GPIO_SWITCH);	/* u16 tag */
    stw_raw(w ++, 20);				/* u16 len */
    strcpy((void *) w, "headphone");		/* char name[12] */
    w += 6;
    stw_raw(w ++, N800_HEADPHONE_GPIO);		/* u16 gpio */
    stw_raw(w ++, 0x11);
    stw_raw(w ++, 0);
    stw_raw(w ++, 0);

    stw_raw(w ++, OMAP_TAG_NOKIA_BT);		/* u16 tag */
    stw_raw(w ++, 12);				/* u16 len */
    b = (void *) w;
    stb_raw(b ++, 0x01);			/* u8 chip_type	(CSR) */
    stb_raw(b ++, N800_BT_WKUP_GPIO);		/* u8 bt_wakeup_gpio */
    stb_raw(b ++, N8X0_BT_HOST_WKUP_GPIO);	/* u8 host_wakeup_gpio */
    stb_raw(b ++, N800_BT_RESET_GPIO);		/* u8 reset_gpio */
    stb_raw(b ++, 1);				/* u8 bt_uart */
    memset(b, 0, 6);				/* u8 bd_addr[6] */
    b += 6;
    stb_raw(b ++, 0x02);			/* u8 bt_sysclk (38.4) */
    w = (void *) b;

    stw_raw(w ++, OMAP_TAG_WLAN_CX3110X);	/* u16 tag */
    stw_raw(w ++, 8);				/* u16 len */
    stw_raw(w ++, 0x25);			/* u8 chip_type */
    stw_raw(w ++, N800_WLAN_PWR_GPIO);		/* s16 power_gpio */
    stw_raw(w ++, N800_WLAN_IRQ_GPIO);		/* s16 irq_gpio */
    stw_raw(w ++, -1);				/* s16 spi_cs_gpio */

    stw_raw(w ++, OMAP_TAG_MMC);		/* u16 tag */
    stw_raw(w ++, 16);				/* u16 len */
    stw_raw(w ++, 0xf);				/* unsigned flags */
    stw_raw(w ++, -1);				/* s16 power_pin */
    stw_raw(w ++, -1);				/* s16 switch_pin */
    stw_raw(w ++, -1);				/* s16 wp_pin */
    stw_raw(w ++, 0);				/* unsigned flags */
    stw_raw(w ++, 0);				/* s16 power_pin */
    stw_raw(w ++, 0);				/* s16 switch_pin */
    stw_raw(w ++, 0);				/* s16 wp_pin */

    stw_raw(w ++, OMAP_TAG_TEA5761);		/* u16 tag */
    stw_raw(w ++, 4);				/* u16 len */
    stw_raw(w ++, N800_TEA5761_CS_GPIO);	/* u16 enable_gpio */
    w ++;

    stw_raw(w ++, OMAP_TAG_PARTITION);		/* u16 tag */
    stw_raw(w ++, 28);				/* u16 len */
    strcpy((void *) w, "bootloader");		/* char name[16] */
    l = (void *) (w + 8);
    stl_raw(l ++, 0x00020000);			/* unsigned int size */
    stl_raw(l ++, 0x00000000);			/* unsigned int offset */
    stl_raw(l ++, 0x3);				/* unsigned int mask_flags */
    w = (void *) l;

    stw_raw(w ++, OMAP_TAG_PARTITION);		/* u16 tag */
    stw_raw(w ++, 28);				/* u16 len */
    strcpy((void *) w, "config");		/* char name[16] */
    l = (void *) (w + 8);
    stl_raw(l ++, 0x00060000);			/* unsigned int size */
    stl_raw(l ++, 0x00020000);			/* unsigned int offset */
    stl_raw(l ++, 0x0);				/* unsigned int mask_flags */
    w = (void *) l;

    stw_raw(w ++, OMAP_TAG_PARTITION);		/* u16 tag */
    stw_raw(w ++, 28);				/* u16 len */
    strcpy((void *) w, "kernel");		/* char name[16] */
    l = (void *) (w + 8);
    stl_raw(l ++, 0x00200000);			/* unsigned int size */
    stl_raw(l ++, 0x00080000);			/* unsigned int offset */
    stl_raw(l ++, 0x0);				/* unsigned int mask_flags */
    w = (void *) l;

    stw_raw(w ++, OMAP_TAG_PARTITION);		/* u16 tag */
    stw_raw(w ++, 28);				/* u16 len */
    strcpy((void *) w, "initfs");		/* char name[16] */
    l = (void *) (w + 8);
    stl_raw(l ++, 0x00200000);			/* unsigned int size */
    stl_raw(l ++, 0x00280000);			/* unsigned int offset */
    stl_raw(l ++, 0x3);				/* unsigned int mask_flags */
    w = (void *) l;

    stw_raw(w ++, OMAP_TAG_PARTITION);		/* u16 tag */
    stw_raw(w ++, 28);				/* u16 len */
    strcpy((void *) w, "rootfs");		/* char name[16] */
    l = (void *) (w + 8);
    stl_raw(l ++, 0x0fb80000);			/* unsigned int size */
    stl_raw(l ++, 0x00480000);			/* unsigned int offset */
    stl_raw(l ++, 0x3);				/* unsigned int mask_flags */
    w = (void *) l;

    stw_raw(w ++, OMAP_TAG_BOOT_REASON);	/* u16 tag */
    stw_raw(w ++, 12);				/* u16 len */
#if 0
    strcpy((void *) w, "por");			/* char reason_str[12] */
    strcpy((void *) w, "charger");		/* char reason_str[12] */
    strcpy((void *) w, "32wd_to");		/* char reason_str[12] */
    strcpy((void *) w, "sw_rst");		/* char reason_str[12] */
    strcpy((void *) w, "mbus");			/* char reason_str[12] */
    strcpy((void *) w, "unknown");		/* char reason_str[12] */
    strcpy((void *) w, "swdg_to");		/* char reason_str[12] */
    strcpy((void *) w, "sec_vio");		/* char reason_str[12] */
    strcpy((void *) w, "pwr_key");		/* char reason_str[12] */
    strcpy((void *) w, "rtc_alarm");		/* char reason_str[12] */
#else
    strcpy((void *) w, "pwr_key");		/* char reason_str[12] */
#endif
    w += 6;

#if 0	/* N810 */
    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "product");		/* char component[12] */
    w += 6;
    strcpy((void *) w, "RX-44");		/* char version[12] */
    w += 6;

    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "hw-build");		/* char component[12] */
    w += 6;
    strcpy((void *) w, "QEMU");			/* char version[12] */
    w += 6;

    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "nolo");			/* char component[12] */
    w += 6;
    strcpy((void *) w, "1.1.10-qemu");		/* char version[12] */
    w += 6;
#else
    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "product");		/* char component[12] */
    w += 6;
    strcpy((void *) w, "RX-34");		/* char version[12] */
    w += 6;

    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "hw-build");		/* char component[12] */
    w += 6;
    strcpy((void *) w, "QEMU");			/* char version[12] */
    w += 6;

    stw_raw(w ++, OMAP_TAG_VERSION_STR);	/* u16 tag */
    stw_raw(w ++, 24);				/* u16 len */
    strcpy((void *) w, "nolo");			/* char component[12] */
    w += 6;
    strcpy((void *) w, "1.1.6-qemu");		/* char version[12] */
    w += 6;
#endif

    stw_raw(w ++, OMAP_TAG_LCD);		/* u16 tag */
    stw_raw(w ++, 36);				/* u16 len */
    strcpy((void *) w, "QEMU LCD panel");	/* char panel_name[16] */
    w += 8;
    strcpy((void *) w, "blizzard");		/* char ctrl_name[16] */
    w += 8;
    stw_raw(w ++, 5);				/* TODO s16 nreset_gpio */
    stw_raw(w ++, 16);				/* u8 data_lines */

    return (void *) w - p;
}

static struct arm_boot_info n800_binfo = {
    .loader_start = OMAP2_Q2_BASE,
    /* Actually two chips of 0x4000000 bytes each */
    .ram_size = 0x08000000,
    .board_id = 0x4f7,
    .atag_board = n800_atag_setup,
};

static void n800_init(ram_addr_t ram_size, int vga_ram_size,
                const char *boot_device, DisplayState *ds,
                const char *kernel_filename, const char *kernel_cmdline,
                const char *initrd_filename, const char *cpu_model)
{
    struct n800_s *s = (struct n800_s *) qemu_mallocz(sizeof(*s));
    int sdram_size = n800_binfo.ram_size;
    int onenandram_size = 0x00010000;

    if (ram_size < sdram_size + onenandram_size + OMAP242X_SRAM_SIZE) {
        fprintf(stderr, "This architecture uses %i bytes of memory\n",
                        sdram_size + onenandram_size + OMAP242X_SRAM_SIZE);
        exit(1);
    }

    s->cpu = omap2420_mpu_init(sdram_size, NULL, cpu_model);

    n800_gpio_setup(s);
    n8x0_nand_setup(s);
    n800_i2c_setup(s);
    n800_tsc_setup(s);
    n800_spi_setup(s);
    n800_dss_setup(s, ds);
    n800_cbus_setup(s);
    if (usb_enabled)
        n800_usb_setup(s);

    /* Setup initial (reset) machine state */

    /* Start at the OneNAND bootloader.  */
    s->cpu->env->regs[15] = 0;

    if (kernel_filename) {
        /* Or at the linux loader.  */
        n800_binfo.kernel_filename = kernel_filename;
        n800_binfo.kernel_cmdline = kernel_cmdline;
        n800_binfo.initrd_filename = initrd_filename;
        arm_load_kernel(s->cpu->env, &n800_binfo);

        qemu_register_reset(n800_boot_init, s);
        n800_boot_init(s);
    }

    dpy_resize(ds, 800, 480);
}

QEMUMachine n800_machine = {
    "n800",
    "Nokia N800 aka. RX-34 tablet (OMAP2420)",
    n800_init,
};
