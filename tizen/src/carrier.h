#ifndef __CARRIER_H__
#define __CARRIER_H__

#include "qemu-common.h"

#define LOG(fmt, arg...)	\
	do {	\
		fprintf(stderr,"[carrier:%s:%d] "fmt"\n", __FUNCTION__, __LINE__, ##arg);	\
	} while (0)


#define HOST_LISTEN_ADDR	"localhost"

#define COMMANDS_TARGET		"target"
#define COMMANDS_DATA		"data"


#define TARGET_ALL			"all"
#define TARGET_ECP			"ecp"
#define TARGET_SELF			"self"
#define TARGET_QMP			"qmp"
#define TARGET_VIRTUAL		"virtual"

int start_carrier(int server_port);
int stop_carrier(void);

void carrier_vprintf(const char *target, const char *fmt, va_list ap);
void carrier_printf(const char *target, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

#endif /* __CARRIER_H__ */
