#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else /* !_WIN32 */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif /* !_WIN32 */

#include "net/slirp.h"
#include "sdb.h"
#include "nbd.h"

/* QSOCKET_CALL is used to deal with the fact that EINTR happens pretty
 * easily in QEMU since we use SIGALRM to implement periodic timers
 */

#ifdef _WIN32
#  define  QSOCKET_CALL(_ret,_cmd)   \
	    do { _ret = (_cmd); } while ( _ret < 0 && WSAGetLastError() == WSAEINTR )
#else
#  define  QSOCKET_CALL(_ret,_cmd)   \
	    do { \
			errno = 0; \
			do { _ret = (_cmd); } while ( _ret < 0 && errno == EINTR ); \
		} while (0);
#endif

#ifdef _WIN32

#include <errno.h>

static int  winsock_error;

#define  WINSOCK_ERRORS_LIST \
    EE(WSA_INVALID_HANDLE,EINVAL,"invalid handle") \
    EE(WSA_NOT_ENOUGH_MEMORY,ENOMEM,"not enough memory") \
    EE(WSA_INVALID_PARAMETER,EINVAL,"invalid parameter") \
    EE(WSAEINTR,EINTR,"interrupted function call") \
    EE(WSAEALREADY,EALREADY,"operation already in progress") \
    EE(WSAEBADF,EBADF,"bad file descriptor") \
    EE(WSAEACCES,EACCES,"permission denied") \
    EE(WSAEFAULT,EFAULT,"bad address") \
    EE(WSAEINVAL,EINVAL,"invalid argument") \
    EE(WSAEMFILE,EMFILE,"too many opened files") \
    EE(WSAEWOULDBLOCK,EWOULDBLOCK,"resource temporarily unavailable") \
    EE(WSAEINPROGRESS,EINPROGRESS,"operation now in progress") \
    EE(WSAEALREADY,EAGAIN,"operation already in progress") \
    EE(WSAENOTSOCK,EBADF,"socket operation not on socket") \
    EE(WSAEDESTADDRREQ,EDESTADDRREQ,"destination address required") \
    EE(WSAEMSGSIZE,EMSGSIZE,"message too long") \
    EE(WSAEPROTOTYPE,EPROTOTYPE,"wrong protocol type for socket") \
    EE(WSAENOPROTOOPT,ENOPROTOOPT,"bad protocol option") \
    EE(WSAEADDRINUSE,EADDRINUSE,"address already in use") \
    EE(WSAEADDRNOTAVAIL,EADDRNOTAVAIL,"cannot assign requested address") \
    EE(WSAENETDOWN,ENETDOWN,"network is down") \
    EE(WSAENETUNREACH,ENETUNREACH,"network unreachable") \
    EE(WSAENETRESET,ENETRESET,"network dropped connection on reset") \
    EE(WSAECONNABORTED,ECONNABORTED,"software caused connection abort") \
    EE(WSAECONNRESET,ECONNRESET,"connection reset by peer") \
    EE(WSAENOBUFS,ENOBUFS,"no buffer space available") \
    EE(WSAEISCONN,EISCONN,"socket is already connected") \
    EE(WSAENOTCONN,ENOTCONN,"socket is not connected") \
    EE(WSAESHUTDOWN,ESHUTDOWN,"cannot send after socket shutdown") \
    EE(WSAETOOMANYREFS,ETOOMANYREFS,"too many references") \
    EE(WSAETIMEDOUT,ETIMEDOUT,"connection timed out") \
    EE(WSAECONNREFUSED,ECONNREFUSED,"connection refused") \
    EE(WSAELOOP,ELOOP,"cannot translate name") \
    EE(WSAENAMETOOLONG,ENAMETOOLONG,"name too long") \
    EE(WSAEHOSTDOWN,EHOSTDOWN,"host is down") \
    EE(WSAEHOSTUNREACH,EHOSTUNREACH,"no route to host") \

typedef struct {
    int          winsock;
    int          unix;
    const char*  string;
} WinsockError;

static const WinsockError  _winsock_errors[] = {
#define  EE(w,u,s)   { w, u, s },
    WINSOCK_ERRORS_LIST
#undef   EE
    { -1, -1, NULL }
};

/* this function reads the latest winsock error code and updates
 * errno to a matching value. It also returns the new value of
 * errno.
 */
static int _fix_errno( void )
{
    const WinsockError*  werr = _winsock_errors;
    int                  unix = EINVAL;  /* generic error code */

    winsock_error = WSAGetLastError();

    for ( ; werr->string != NULL; werr++ ) {
        if (werr->winsock == winsock_error) {
            unix = werr->unix;
            break;
        }
    }
    errno = unix;
    return -1;
}

#else
static int _fix_errno( void )
{
    return -1;
}

#endif

#define  SOCKET_CALL(cmd)  \
	int  ret; \
	QSOCKET_CALL(ret, (cmd)); \
	if (ret < 0) \
		return _fix_errno(); \
	return ret; \

int socket_send(int  fd, const void*  buf, int  buflen)
{
	SOCKET_CALL(send(fd, buf, buflen, 0))
}

#ifdef _WIN32

static void
socket_close_handler( void*  _fd )
{
    int   fd = (int)_fd;
    int   ret;
    char  buff[64];

    /* we want to drain the read side of the socket before closing it */
    do {
        ret = recv( fd, buff, sizeof(buff), 0 );
    } while (ret < 0 && WSAGetLastError() == WSAEINTR);

    if (ret < 0 && WSAGetLastError() == EWOULDBLOCK)
        return;

    qemu_set_fd_handler( fd, NULL, NULL, NULL );
    closesocket( fd );
}

void
socket_close( int  fd )
{
    int  old_errno = errno;

    shutdown( fd, SD_BOTH );
    /* we want to drain the socket before closing it */
    qemu_set_fd_handler( fd, socket_close_handler, NULL, (void*)fd );

    errno = old_errno;
}

#else /* !_WIN32 */

#include <unistd.h>

void
socket_close( int  fd )
{
    int  old_errno = errno;

    shutdown( fd, SHUT_RDWR );
    close( fd );

    errno = old_errno;
}

#endif /* !_WIN32 */


int SLP_base_port;

int inet_strtoip(const char*  str, uint32_t  *ip)
{
	int  comp[4];

	if (sscanf(str, "%d.%d.%d.%d", &comp[0], &comp[1], &comp[2], &comp[3]) != 4)
		return -1;

	if ((unsigned)comp[0] >= 256 ||
			(unsigned)comp[1] >= 256 ||
			(unsigned)comp[2] >= 256 ||
			(unsigned)comp[3] >= 256)
		return -1;

	*ip = (uint32_t)((comp[0] << 24) | (comp[1] << 16) |
			(comp[2] << 8)  |  comp[3]);
	return 0;
}

void sdb_setup(void)
{
    int   tries     = 10;
    const char *base_port = "26100";
    int   success   = 0;
    int   s;
	int   port;
    uint32_t  guest_ip;
	const char *p;
	char buf[30] = {0,};

	inet_strtoip("10.0.2.16", &guest_ip);

	port = strtol(base_port, (char **)&p, 0);

	for ( ; tries > 0; tries--, port += 10 ) {
		// redir form [tcp:26100:10.0.2.16:26101]
		sprintf(buf, "tcp:%d:10.0.2.16:26101", port+1);
		if(net_slirp_redir((char*)buf) < 0)
			continue;

//		fprintf(stdout,"SDBD established on port %d\n", port+1);
		success = 1;
		break;
	}
	if (!success) {
		fprintf(stderr, "it seems too many emulator instances are running on this machine. Aborting\n" );
		exit(1);
    }

    /* Save base port. */
    SLP_base_port = port;

   /* send a simple message to the SDB host server to tell it we just started.
    * it should be listening on port 26099. if we can't reach it, don't bother
    */
    do
    {
        char tmp[32];
		
		s = tcp_socket_outgoing("127.0.0.1", SDB_HOST_PORT);
		if (s < 0) {
            fprintf(stderr, "can't create socket to talk to the SDB server");
            break;
        }
		
        sprintf(tmp,"0013host:emulator:%d",port+1);
        socket_send(s, tmp, 30);
//		printf("sent '%s' to SDB server\n", tmp);
//		fflush(stdout);

    }
    while (0);

    if (s >= 0)
    	socket_close(s);
}
