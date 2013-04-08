#ifndef __ECS_H__
#define __ECS_H__

#include "qemu-common.h"

#define ECS_DEBUG	1

#ifdef ECS_DEBUG
#define LOG(fmt, arg...)	\
	do {	\
		fprintf(stdout,"[%s-%s:%d] "fmt"\n", __TIME__, __FUNCTION__, __LINE__, ##arg);	\
	} while (0)
#else
#define LOG(fmt, arg...)
#endif

#ifndef _WIN32
#define LOG_HOME				"HOME"
#define LOG_PATH 				"/tizen-sdk-data/emulator-vms/vms/ecs.log"
#endif

#define HOST_LISTEN_ADDR		"localhost"
#define HOST_LISTEN_PORT		27000
#define EMULATOR_SERVER_NUM		3
#define WELCOME_MESSAGE			"### Welcome to ECS service. ###\n"

#define COMMANDS_TYPE			"type"
#define COMMANDS_DATA			"data"

#define TIMER_ALIVE_S			60	
#define TYPE_DATA_SELF			"self"

int start_ecs(void);
int stop_ecs(void);

void ecs_vprintf(const char *type, const char *fmt, va_list ap);
void ecs_printf(const char *type, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

void send_to_client(int fd, const char *str);

#endif /* __ECS_H__ */
