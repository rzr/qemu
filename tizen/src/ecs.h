#ifndef __ECS_H__
#define __ECS_H__

#ifndef _WIN32
#include <sys/epoll.h>
#endif

#include "qapi/qmp/qerror.h"
#include "qemu-common.h"
#include "ecs-json-streamer.h"
#include "genmsg/ecs.pb-c.h"

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
#else
#define LOG_HOME				"LOCALAPPDATA"
#define LOG_PATH 				"\\tizen-sdk-data\\emulator-vms\\vms\\ecs.log"
#endif

#define ECS_OPTS_NAME			"ecs"
#define HOST_LISTEN_ADDR		"127.0.0.1"
#define HOST_LISTEN_PORT		27000
#define EMULATOR_SERVER_NUM		3
#define WELCOME_MESSAGE			"### Welcome to ECS service. ###\n"

#define COMMANDS_TYPE			"type"
#define COMMANDS_DATA			"data"


#define COMMAND_TYPE_INJECTOR	"injector"
#define COMMAND_TYPE_CONTROL	"control"
#define COMMAND_TYPE_MONITOR	"monitor"
#define COMMAND_TYPE_DEVICE		"device"

#define ECS_MSG_STARTINFO_REQ 	"startinfo_req"
#define ECS_MSG_STARTINFO_ANS 	"startinfo_ans"

#define MSG_TYPE_SENSOR			"sensor"
#define MSG_TYPE_NFC			"nfc"

#define MSG_GROUP_STATUS		15

#define MSG_ACTION_ACCEL		110
#define MSG_ACTION_GYRO			111
#define MSG_ACTION_MAG			112
#define MSG_ACTION_LIGHT		113
#define MSG_ACTION_PROXI		114

#define TIMER_ALIVE_S			60	
#define TYPE_DATA_SELF			"self"

enum sensor_level {
	level_accel = 1,
	level_proxi = 2,
	level_light = 3,
	level_gyro = 4,
	level_geo = 5,
	level_tilt = 12,
	level_magnetic = 13
};

typedef unsigned short	type_length;
typedef unsigned char	type_group;
typedef unsigned char	type_action;

#define OUT_BUF_SIZE	4096
#define READ_BUF_LEN 	4096



typedef struct sbuf
{
	int _netlen;
	int _use;
	char _buf[4096];
}sbuf;


struct Monitor {
    int suspend_cnt;
    uint8_t outbuf[OUT_BUF_SIZE];
    int outbuf_index;
    CPUArchState *mon_cpu;
    void *password_opaque;
    QError *error;
    QLIST_HEAD(,mon_fd_t) fds;
    QLIST_ENTRY(Monitor) entry;
};

#define MAX_EVENTS	1000
typedef struct ECS_State {
	int listen_fd;
#ifndef _WIN32
	int epoll_fd;
	struct epoll_event events[MAX_EVENTS];
#else
	fd_set reads;
#endif
	int is_unix;
	int ecs_running;
	QEMUTimer *alive_timer;
	Monitor *mon;
} ECS_State;

typedef struct ECS_Client {
	int client_fd;
	int client_id;
	int keep_alive;
	const char* type;

	sbuf sbuf;

	ECS_State *cs;
	JSONMessageParser parser;
    QTAILQ_ENTRY(ECS_Client) next;
} ECS_Client;


int start_ecs(void);
int stop_ecs(void);

void ecs_vprintf(const char *type, const char *fmt, va_list ap);
void ecs_printf(const char *type, const char *fmt, ...) GCC_FMT_ATTR(2, 3);

int get_ecs_port(void);

bool handle_protobuf_msg(ECS_Client* cli, char* data, const int len);

bool ntf_to_injector(const char* data, const int len);
bool ntf_to_control(const char* data, const int len);
bool ntf_to_monitor(const char* data, const int len);


bool send_to_ecp(ECS__Master* master);

bool send_start_ans(int host_keyboard_onff);
bool send_injector_ntf(const char* data, const int len);
bool send_control_ntf(const char* data, const int len);
bool send_monitor_ntf(const char* data, const int len);
bool send_hostkeyboard_ntf(int is_on);
bool send_device_ntf(const char* data, const int len);

bool send_to_all_client(const char* data, const int len);
void send_to_client(int fd, const char* data, const int len) ;


void make_header(QDict* obj, type_length length, type_group group, type_action action);

void read_val_short(const char* data, unsigned short* ret_val);
void read_val_char(const char* data, unsigned char* ret_val);
void read_val_str(const char* data, char* ret_val, int len);


bool msgproc_start_req(ECS_Client* ccli, ECS__StartReq* msg);
bool msgproc_injector_req(ECS_Client* ccli, ECS__InjectorReq* msg);
bool msgproc_control_msg(ECS_Client *cli, ECS__ControlMsg* msg);
bool msgproc_monitor_req(ECS_Client *ccli, ECS__MonitorReq* msg);
bool msgproc_device_req(ECS_Client* ccli, ECS__DeviceReq* msg);
bool msgproc_screen_dump_req(ECS_Client *ccli, ECS__ScreenDumpReq* msg);


enum{
	CONTROL_COMMAND_HOST_KEYBOARD_ONOFF_REQ = 1,
	CONTROL_COMMAND_SCREENSHOT_REQ = 2
};

// control sub messages
void msgproc_control_hostkeyboard_req(ECS_Client *cli, ECS__HostKeyboardReq* req);

void set_sensor_data(int length, const char* data);

static QemuOptsList qemu_ecs_opts = {
    .name = ECS_OPTS_NAME,
    .implied_opt_name = ECS_OPTS_NAME,
    .head = QTAILQ_HEAD_INITIALIZER(qemu_ecs_opts.head),
    .desc = {
		{
            .name = "host",
            .type = QEMU_OPT_STRING,
        },{
            .name = "port",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localaddr",
            .type = QEMU_OPT_STRING,
        },{
            .name = "localport",
            .type = QEMU_OPT_STRING,
        },{
            .name = "to",
            .type = QEMU_OPT_NUMBER,
        },{
            .name = "ipv4",
            .type = QEMU_OPT_BOOL,
        },{
            .name = "ipv6",
            .type = QEMU_OPT_BOOL,
        },
        { /* End of list */ }
    },
};

#endif /* __ECS_H__ */
