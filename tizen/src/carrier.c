
#include "hw/qdev.h"
#include "net.h"
#include "console.h"
#include "migration.h"

#ifndef _WIN32
#include <sys/epoll.h>
#endif

#include "qemu-common.h"
#include "qemu_socket.h"
#include "qemu-queue.h"
#include "qemu-option.h"
#include "qemu-config.h"
#include "qemu-timer.h"
#include "main-loop.h"
#include "ui/qemu-spice.h"
#include "qemu-char.h"

#include "qjson.h"
#include "json-streamer.h"
#include "json-parser.h"
#include "monitor.h"
#include "qmp-commands.h"

#include "carrier.h"

#define OUT_BUF_SIZE	1024
#define READ_BUF_LEN 	4096

typedef struct mon_fd_t mon_fd_t;
struct mon_fd_t {
    char *name;
    int fd;
    QLIST_ENTRY(mon_fd_t) next;
};

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

#define MAX_CLIENT	10
#define MAX_EVENTS	1000
typedef struct CarrierState {
	int listen_fd;
	int epoll_fd;
	struct epoll_event events[MAX_EVENTS];
	int is_unix;
	Monitor *mon;
} CarrierState;

typedef struct CarrierClientInfo {
	int client_fd;
	const char* target;
	CarrierState *cs;
	JSONMessageParser parser;
    QTAILQ_ENTRY(CarrierClientInfo) next;
} CarrierClientInfo;

typedef struct mon_cmd_t {
    const char *name;
    const char *args_type;
    const char *params;
    const char *help;
    void (*user_print)(Monitor *mon, const QObject *data);
    union {
        void (*info)(Monitor *mon);
        void (*cmd)(Monitor *mon, const QDict *qdict);
        int  (*cmd_new)(Monitor *mon, const QDict *params, QObject **ret_data);
        int  (*cmd_async)(Monitor *mon, const QDict *params,
                          MonitorCompletion *cb, void *opaque);
    } mhandler;
    int flags;
} mon_cmd_t;

static QTAILQ_HEAD(CarrierClientInfoHead, CarrierClientInfo) clients =
    QTAILQ_HEAD_INITIALIZER(clients);

static int carrier_write(int fd, const uint8_t *buf, int len);

#define QMP_ACCEPT_UNKNOWNS 1
static void carrier_flush(CarrierClientInfo *clii)
{
    if (clii && clii->cs && clii->cs->mon && clii->cs->mon->outbuf_index != 0) {
        carrier_write(clii->client_fd, clii->cs->mon->outbuf, clii->cs->mon->outbuf_index);
        clii->cs->mon->outbuf_index = 0;
    }
}

/* flush at every end of line or if the buffer is full */
static void carrier_puts(CarrierClientInfo *clii, const char *str)
{
    char c;

    for(;;) {
        c = *str++;
        if (c == '\0')
            break;
        if (c == '\n')
            clii->cs->mon->outbuf[clii->cs->mon->outbuf_index++] = '\r';
        clii->cs->mon->outbuf[clii->cs->mon->outbuf_index++] = c;
        if (clii->cs->mon->outbuf_index >= (sizeof(clii->cs->mon->outbuf) - 1)
            || c == '\n')
            carrier_flush(clii);
    }
}

void carrier_vprintf(const char *target, const char *fmt, va_list ap)
{
    char buf[READ_BUF_LEN];
	CarrierClientInfo *clii;

    QTAILQ_FOREACH(clii, &clients, next) {
        if (!strcmp(target, TARGET_ALL) || !strcmp(clii->target, target)) {
			vsnprintf(buf, sizeof(buf), fmt, ap);
			carrier_puts(clii, buf);
		}
    }
}

void carrier_printf(const char* target, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    carrier_vprintf(target, fmt, ap);
    va_end(ap);
}

static inline int monitor_has_error(const Monitor *mon)
{
    return mon->error != NULL;
}

static void handler_audit(Monitor *mon, const mon_cmd_t *cmd, int ret)
{
    if (ret && !monitor_has_error(mon)) {
        qerror_report(QERR_UNDEFINED_ERROR);
    }
}

static QDict *build_qmp_error_dict(const QError *err)
{
    QObject *obj = qobject_from_jsonf("{ 'error': { 'class': %s, 'desc': %p } }",
                             ErrorClass_lookup[err->err_class],
                             qerror_human(err));

    return qobject_to_qdict(obj);
}

static void monitor_json_emitter(CarrierClientInfo *clii, const QObject *data)
{
    QString *json;

    json = qobject_to_json(data);

    assert(json != NULL);

    qstring_append_chr(json, '\n');
    carrier_puts(clii, qstring_get_str(json));

    QDECREF(json);
}

static void monitor_protocol_emitter(CarrierClientInfo *clii, QObject *data)
{
    QDict *qmp;

    trace_monitor_protocol_emitter(clii->cs->mon);

    if (!monitor_has_error(clii->cs->mon)) {
        /* success response */
        qmp = qdict_new();
        if (data) {
            qobject_incref(data);
            qdict_put_obj(qmp, "return", data);
        } else {
            /* return an empty QDict by default */
            qdict_put(qmp, "return", qdict_new());
        }
    } else {
        /* error response */
        qmp = build_qmp_error_dict(clii->cs->mon->error);
        QDECREF(clii->cs->mon->error);
        clii->cs->mon->error = NULL;
    }

    monitor_json_emitter(clii, QOBJECT(qmp));
    QDECREF(qmp);
}


static void qmp_monitor_complete(void *opaque, QObject *ret_data)
{
    monitor_protocol_emitter(opaque, ret_data);
}

static int qmp_async_cmd_handler(CarrierClientInfo *clii, const mon_cmd_t *cmd,
                                 const QDict *params)
{
    return cmd->mhandler.cmd_async(clii->cs->mon, params, qmp_monitor_complete, clii);
}

static void qmp_call_cmd(CarrierClientInfo *clii, const mon_cmd_t *cmd,
                         const QDict *params)
{
    int ret;
    QObject *data = NULL;

    ret = cmd->mhandler.cmd_new(clii->cs->mon, params, &data);
    handler_audit(clii->cs->mon, cmd, ret);
    monitor_protocol_emitter(clii, data);
    qobject_decref(data);
}

static inline bool handler_is_async(const mon_cmd_t *cmd)
{
    return cmd->flags & MONITOR_CMD_ASYNC;
}

static void monitor_user_noop(Monitor *mon, const QObject *data) 
{ 
}

static int do_screen_dump(Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    vga_hw_screen_dump(qdict_get_str(qdict, "filename"));
    return 0;
}

static int client_migrate_info(Monitor *mon, const QDict *qdict,
                               MonitorCompletion cb, void *opaque)
{
    return 0;
}

static int add_graphics_client(Monitor *mon, const QDict *qdict, QObject **ret_data)
{
    return 0;
}

static int do_qmp_capabilities(Monitor *mon, const QDict *params,
                               QObject **ret_data)
{
    return 0;
}

static const mon_cmd_t qmp_cmds[] = {
#include "qmp-commands-old.h"
    { /* NULL */ },
};

static int check_mandatory_args(const QDict *cmd_args,
                                const QDict *client_args, int *flags)
{
    const QDictEntry *ent;

    for (ent = qdict_first(cmd_args); ent; ent = qdict_next(cmd_args, ent)) {
        const char *cmd_arg_name = qdict_entry_key(ent);
        QString *type = qobject_to_qstring(qdict_entry_value(ent));
        assert(type != NULL);

        if (qstring_get_str(type)[0] == 'O') {
            assert((*flags & QMP_ACCEPT_UNKNOWNS) == 0);
            *flags |= QMP_ACCEPT_UNKNOWNS;
        } else if (qstring_get_str(type)[0] != '-' &&
                   qstring_get_str(type)[1] != '?' &&
                   !qdict_haskey(client_args, cmd_arg_name)) {
            qerror_report(QERR_MISSING_PARAMETER, cmd_arg_name);
            return -1;
        }
    }

    return 0;
}

static int check_client_args_type(const QDict *client_args,
                                  const QDict *cmd_args, int flags)
{
    const QDictEntry *ent;

    for (ent = qdict_first(client_args); ent;ent = qdict_next(client_args,ent)){
        QObject *obj;
        QString *arg_type;
        const QObject *client_arg = qdict_entry_value(ent);
        const char *client_arg_name = qdict_entry_key(ent);

        obj = qdict_get(cmd_args, client_arg_name);
        if (!obj) {
            if (flags & QMP_ACCEPT_UNKNOWNS) {
                continue;
            }
            qerror_report(QERR_INVALID_PARAMETER, client_arg_name);
            return -1;
        }

        arg_type = qobject_to_qstring(obj);
        assert(arg_type != NULL);

        switch (qstring_get_str(arg_type)[0]) {
        case 'F':
        case 'B':
        case 's':
            if (qobject_type(client_arg) != QTYPE_QSTRING) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                              "string");
                return -1;
            }
        break;
        case 'i':
        case 'l':
        case 'M':
        case 'o':
            if (qobject_type(client_arg) != QTYPE_QINT) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                              "int");
                return -1; 
            }
            break;
        case 'T':
            if (qobject_type(client_arg) != QTYPE_QINT &&
                qobject_type(client_arg) != QTYPE_QFLOAT) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                              "number");
               return -1; 
            }
            break;
        case 'b':
        case '-':
            if (qobject_type(client_arg) != QTYPE_QBOOL) {
                qerror_report(QERR_INVALID_PARAMETER_TYPE, client_arg_name,
                              "bool");
               return -1; 
            }
            break;
        case 'O':
            assert(flags & QMP_ACCEPT_UNKNOWNS);
            break;
        case 'q':
            break;
        case '/':
        case '.':
        default:
            abort();
        }
    }

    return 0;
}

static QDict *qdict_from_args_type(const char *args_type)
{
    int i;
    QDict *qdict;
    QString *key, *type, *cur_qs;

    assert(args_type != NULL);

    qdict = qdict_new();

    if (args_type == NULL || args_type[0] == '\0') {
        goto out;
    }

    key = qstring_new();
    type = qstring_new();

    cur_qs = key;

    for (i = 0;; i++) {
        switch (args_type[i]) {
            case ',':
            case '\0':
                qdict_put(qdict, qstring_get_str(key), type);
                QDECREF(key);
                if (args_type[i] == '\0') {
                    goto out;
                }
                type = qstring_new();
                cur_qs = key = qstring_new();
                break;
            case ':':
                cur_qs = type;
                break;
            default:
                qstring_append_chr(cur_qs, args_type[i]);
                break;
        }
    }

out:
    return qdict;
}

static int qmp_check_client_args(const mon_cmd_t *cmd, QDict *client_args)
{
    int flags, err;
    QDict *cmd_args;

    cmd_args = qdict_from_args_type(cmd->args_type);

    flags = 0;
    err = check_mandatory_args(cmd_args, client_args, &flags);
    if (err) {
        goto out;
    }

    err = check_client_args_type(client_args, cmd_args, flags);

out:
    QDECREF(cmd_args);
    return err;
}

static QDict *qmp_check_input_obj(QObject *input_obj)
{
    const QDictEntry *ent;
    int has_exec_key = 0;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return NULL;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
        const char *arg_name = qdict_entry_key(ent);
        const QObject *arg_obj = qdict_entry_value(ent);

        if (!strcmp(arg_name, "execute")) {
            if (qobject_type(arg_obj) != QTYPE_QSTRING) {
                qerror_report(QERR_QMP_BAD_INPUT_OBJECT_MEMBER, "execute",
                              "string");
                return NULL;
            }
            has_exec_key = 1;
        } else if (!strcmp(arg_name, "arguments")) {
            if (qobject_type(arg_obj) != QTYPE_QDICT) {
                qerror_report(QERR_QMP_BAD_INPUT_OBJECT_MEMBER, "arguments",
                              "object");
                return NULL;
            }
        } else if (!strcmp(arg_name, "id")) {
        } else {
            qerror_report(QERR_QMP_EXTRA_MEMBER, arg_name);
            return NULL;
        }
    }

    if (!has_exec_key) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "execute");
        return NULL;
    }

    return input_dict;
}

static int compare_cmd(const char *name, const char *list)
{
    const char *p, *pstart;
    int len;
    len = strlen(name);
    p = list;
    for(;;) {
        pstart = p;
        p = strchr(p, '|');
        if (!p)
            p = pstart + strlen(pstart);
        if ((p - pstart) == len && !memcmp(pstart, name, len))
            return 1;
        if (*p == '\0')
            break;
        p++;
    }
    return 0;
}

static const mon_cmd_t *search_dispatch_table(const mon_cmd_t *disp_table,
                                              const char *cmdname)
{
    const mon_cmd_t *cmd;

    for (cmd = disp_table; cmd->name != NULL; cmd++) {
        if (compare_cmd(cmdname, cmd->name)) {
            return cmd;
        }
    }

    return NULL;
}

static const mon_cmd_t *qmp_find_cmd(const char *cmdname)
{
    return search_dispatch_table(qmp_cmds, cmdname);
}

static void handle_qmp_command(CarrierClientInfo *clii, QObject *obj)
{
    int err;
    const mon_cmd_t *cmd;
    const char *cmd_name;
    QDict *input = NULL;
	QDict *args = NULL;

	input = qmp_check_input_obj(obj);
    if (!input) {
        qobject_decref(obj);
        goto err_out;
    }

    cmd_name = qdict_get_str(input, "execute");

    cmd = qmp_find_cmd(cmd_name);
    if (!cmd) {
        qerror_report(QERR_COMMAND_NOT_FOUND, cmd_name);
        goto err_out;
    }
	
	obj = qdict_get(input, "arguments");
    if (!obj) {
        args = qdict_new();
    } else {
        args = qobject_to_qdict(obj);
        QINCREF(args);
    }

    err = qmp_check_client_args(cmd, args);
    if (err < 0) {
        goto err_out;
    }

    if (handler_is_async(cmd)) {
        err = qmp_async_cmd_handler(clii, cmd, args);
        if (err) {
            goto err_out;
        }
    } else {
        qmp_call_cmd(clii, cmd, args);
    }

    goto out;

err_out:
    monitor_protocol_emitter(clii, NULL);
out:
    QDECREF(input);
    QDECREF(args);

}

static int check_key(QObject *input_obj, const char *key)
{
    const QDictEntry *ent;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return -1;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
        const char *arg_name = qdict_entry_key(ent);
        if (!strcmp(arg_name, key)) {
        	return 1;
        }
    }

    return 0;
}

static QObject* get_data_object(QObject *input_obj)
{
    const QDictEntry *ent;
    QDict *input_dict;

    if (qobject_type(input_obj) != QTYPE_QDICT) {
        qerror_report(QERR_QMP_BAD_INPUT_OBJECT, "object");
        return NULL;
    }

    input_dict = qobject_to_qdict(input_obj);

    for (ent = qdict_first(input_dict); ent; ent = qdict_next(input_dict, ent)){
        const char *arg_name = qdict_entry_key(ent);
        QObject *arg_obj = qdict_entry_value(ent);
        if (!strcmp(arg_name, COMMANDS_DATA)) {
        	return arg_obj;
        }
    }

    return NULL;
}

static void handle_carrier_command(JSONMessageParser *parser, QList *tokens, void *opaque)
{
	const char *target_name;
	const char *data_value;
	int def_target = 0;
	int def_data = 0;
    QObject *obj;
	CarrierClientInfo *clii = opaque;

	if (NULL == clii) {
		LOG("ClientInfo is null.");
		return;
	}
	
    obj = json_parser_parse(tokens, NULL);
    if (!obj) {
        qerror_report(QERR_JSON_PARSING);
    	monitor_protocol_emitter(clii, NULL);
		return;
    }

	def_target = check_key(obj, COMMANDS_TARGET);
	if (0 > def_target) {
		LOG("def_target failed.");
		return;
	} else if (0 == def_target) {
		handle_qmp_command(clii, obj);
		return;
	}

	def_data = check_key(obj, COMMANDS_DATA);
	if (0 > def_data) {
		LOG("json format error: data.");
		return;
	} else if (0 == def_data) {
		LOG("data key is not found.");
		return;
	}
	
	target_name = qdict_get_str(qobject_to_qdict(obj), COMMANDS_TARGET);
		
	if (!strcmp(target_name, TARGET_QMP)) {
		handle_qmp_command(clii, get_data_object(obj));
	} else {
		data_value = qdict_get_str(qobject_to_qdict(obj), COMMANDS_DATA);
		if (!strcmp(target_name, TARGET_SELF)) {
			clii->target = data_value;
    		monitor_protocol_emitter(clii, NULL);
		}
	}
}

static Monitor *monitor_create(void)
{
    Monitor *mon;

	// TODO: event related work
	//key_timer = qemu_new_timer_ns(vm_clock, release_keys, NULL);
    //monitor_protocol_event_init();

    mon = g_malloc0(sizeof(*mon));
	memset(mon, 0, sizeof(*mon));

	return mon;
}

static int device_initialize(void)
{
	// currently nothing to do with it.
	return 1;
}

static void carrier_close(CarrierState *cs)
{
	if (cs->listen_fd >= 0) {
		closesocket(cs->listen_fd);
	}

	if (cs->mon != NULL) {
		g_free(cs->mon);
	}
	//TODO: device close

	g_free(cs);
}

static int carrier_write(int fd, const uint8_t *buf, int len)
{
	return send_all(fd, buf, len);
}

#ifndef _WIN32
static ssize_t carrier_recv(int fd, char *buf, size_t len)
{
	struct msghdr msg = { NULL, };
    struct iovec iov[1];
    union {
        struct cmsghdr cmsg;
        char control[CMSG_SPACE(sizeof(int))];
    } msg_control;
    int flags = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = len;

    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &msg_control;
    msg.msg_controllen = sizeof(msg_control);

#ifdef MSG_CMSG_CLOEXEC
    flags |= MSG_CMSG_CLOEXEC;
#endif
    return recvmsg(fd, &msg, flags);
}

#else
static ssize_t carrier_recv(int fd, char *buf, size_t len)
{
    return qemu_recv(fd, buf, len, 0);
}
#endif

static void carrier_read (CarrierClientInfo *clii)
{
	uint8_t buf[READ_BUF_LEN];
	int len, size;
	len = sizeof(buf);

	if (!clii) {
		LOG ("read client info is NULL.");
		return;
	}

	size = carrier_recv(clii->client_fd, (void *)buf, len);
	if (0 == size) {
        closesocket(clii->client_fd);
		QTAILQ_REMOVE(&clients, clii, next);
		g_free(clii);
	} else if (0 < size) {
	    json_message_parser_feed(&clii->parser, (const char *) buf, size);
	}
}

static QObject *get_carrier_greeting(void)
{
    QObject *ver = NULL;

    qmp_marshal_input_query_version(NULL, NULL, &ver);
    return qobject_from_jsonf("{'Carrier':{'version': %p,'capabilities': [true]}}",ver);
}

static void epoll_cli_add(CarrierState *cs, int fd)
{
	struct epoll_event events;

	/* event control set for read event */
	events.events = EPOLLIN;
	events.data.fd = fd; 

	if( epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, fd, &events) < 0 )
	{
		LOG("Epoll control fails.in epoll_cli_add.");
	}
}

static CarrierClientInfo *carrier_find_client(int fd)
{
	CarrierClientInfo *clii;

    QTAILQ_FOREACH(clii, &clients, next) {
        if (clii->client_fd == fd)
        	return clii;
    }
	return NULL;
}

static int carrier_add_client(CarrierState *cs, int fd) 
{
	QObject *data;
	CarrierClientInfo *clii = g_malloc0(sizeof(CarrierClientInfo));

	clii->client_fd = fd;
	clii->cs = cs;
    json_message_parser_init(&clii->parser, handle_carrier_command, clii);
	epoll_cli_add(cs, fd);

	QTAILQ_INSERT_TAIL(&clients, clii, next);

	data = get_carrier_greeting();
	monitor_json_emitter(clii, data);
    qobject_decref(data);

	return 0;
}

static void carrier_accept(CarrierState *cs)
{
    struct sockaddr_in saddr;
#ifndef _WIN32
    struct sockaddr_un uaddr;
#endif
    struct sockaddr *addr;
    socklen_t len;
    int fd;

    for(;;) {
#ifndef _WIN32
	if (cs->is_unix) {
	    len = sizeof(uaddr);
	    addr = (struct sockaddr *)&uaddr;
	} else
#endif
	{
	    len = sizeof(saddr);
	    addr = (struct sockaddr *)&saddr;
	}
        fd = qemu_accept(cs->listen_fd, addr, &len);
        if (0 > fd && EINTR != errno) {
            return;
        } else if (0 <= fd) {
            break;
        }
    }
	if (0 > carrier_add_client(cs, fd)) {
		LOG("failed to add client.");
	}
}

static void epoll_init(CarrierState *cs) 
{
	struct epoll_event events;

	cs->epoll_fd = epoll_create(MAX_EVENTS);
	if(cs->epoll_fd < 0)
	{
        closesocket(cs->listen_fd);
	}

	events.events = EPOLLIN;
	events.data.fd = cs->listen_fd;

	if( epoll_ctl(cs->epoll_fd, EPOLL_CTL_ADD, cs->listen_fd, &events) < 0 )
	{
		close(cs->listen_fd);
		close(cs->epoll_fd);
	}
}

static int socket_initialize(CarrierState *cs, QemuOpts *opts)
{
	int fd = -1;

    fd = inet_listen_opts(opts, 0, NULL);
	if (0 > fd) {
		return -1;
	}

    socket_set_nonblock(fd);

	cs->listen_fd = fd;
	epoll_init(cs);

	return 0;
}

static void carrier_loop(CarrierState *cs)
{
	int i,nfds;

	nfds = epoll_wait(cs->epoll_fd,cs->events,MAX_EVENTS,-1);
	if (nfds == 0){
		return;
	}

	if (nfds < 0) {
		LOG("epoll wait error.");
		return;
	} 

	for( i = 0 ; i < nfds ; i++ )
	{
		if (cs->events[i].data.fd == cs->listen_fd) {
			carrier_accept(cs);
			continue;
		}
		carrier_read(carrier_find_client(cs->events[i].data.fd));
	}
}

static void* carrier_initialize(void* args) 
{
	char port[8];
	int ret = 1;
	CarrierState *cs = NULL;
	QemuOpts *opts = NULL;
	Error *local_err = NULL;
	Monitor* mon = NULL;

	opts = qemu_opts_create(qemu_find_opts("carrier"), "carrier", 1, &local_err);
    if (error_is_set(&local_err)) {
        qerror_report_err(local_err);
        error_free(local_err);
        return NULL;
    }

	sprintf(port, "%d", (int)args);

    qemu_opt_set(opts, "host", HOST_LISTEN_ADDR);
    qemu_opt_set(opts, "port", port);

	cs = g_malloc0(sizeof(CarrierState));

	ret = socket_initialize(cs, opts);
	if (0 > ret) {
		LOG("socket initialization failed.");
		return NULL;
	}

	mon = monitor_create();
	if (NULL == mon) {
		LOG("monitor initialization failed.");
		carrier_close(cs);
		return NULL;
	}

	cs->mon = mon;

	ret = device_initialize();
	if (0 > ret) {
		LOG("device initialization failed.");
		carrier_close(cs);
		return NULL;
	}

	while(1) {
		carrier_loop(cs);
	}

	return (void*)ret;
}

int stop_carrier(void)
{
	return 0;
}

int start_carrier(int server_port)
{
	pthread_t thread_id;

	if (0 != pthread_create(&thread_id, NULL, carrier_initialize, (void*)server_port)) {
		LOG("pthread creation failed.");
		return -1;
	}
	return 0;
}
