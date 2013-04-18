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


#define COMMAND_TYPE_INJECTOR	"injector"
#define COMMAND_TYPE_CONTROL	"control"
#define COMMAND_TYPE_MONITOR	"monitor"

#define TIMER_ALIVE_S			60	
#define TYPE_DATA_SELF			"self"


typedef unsigned short	type_length;
typedef unsigned char	type_group;
typedef unsigned char	type_action;


int start_ecs(void);
int stop_ecs(void);

void ecs_vprintf(const char *type, const char *fmt, va_list ap);
void ecs_printf(const char *type, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

bool ntf_to_injector(const char* data, const int len);
bool ntf_to_control(const char* data, const int len);
bool ntf_to_monitor(const char* data, const int len);

bool send_to_all_client(const char* data, const int len);
void send_to_client(int fd, const char *str);


void read_val_short(const char* data, unsigned short* ret_val);
void read_val_char(const char* data, unsigned char* ret_val);
void read_val_str(const char* data, char* ret_val, int len);

#endif /* __ECS_H__ */
