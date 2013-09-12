
#include "qemu-common.h"
#include "sysemu/sysemu.h"
#include "qmp-commands.h"

AccelInfo *qmp_query_accel(Error **);

AccelInfo *qmp_query_accel(Error **errp)
{
    AccelInfo *info = g_malloc0(sizeof(*info));
    info->Xaxis = 1;
    info->Yaxis = 2;
    info->Zaxis = 3;
	return info;
}

