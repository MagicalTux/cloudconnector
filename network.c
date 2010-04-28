#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "network.h"
#include "log.h"
#include "cfg_files.h"
#include "array.h"
#include "ssl.h"

#define EPOLL_MAX_EVENTS 16

static array_t *sockets;

static int epoll_handle;
static struct epoll_event ev, epoll_events[EPOLL_MAX_EVENTS];

static char *listen_addr = NULL;
static int port = 65534;

char *network_ip_string(struct sockaddr *addr, int addr_len) {
	char buf[64];
	switch(addr->sa_family) {
		case AF_INET:
			{
				struct sockaddr_in *addr4 = (struct sockaddr_in*)addr;
				return strdup(inet_ntop(addr4->sin_family, &addr4->sin_addr, (char*)&buf, addr_len));
			}
			break;
		case AF_INET6:
			{
				struct sockaddr_in6 *addr6 = (struct sockaddr_in6*)addr;
				return strdup(inet_ntop(addr6->sin6_family, &addr6->sin6_addr, (char*)&buf, addr_len));
			}
			break;
	}
	return NULL;
}

void network_config_init() {
	config_add_var(CONFIG_CORE, "network_bind_ip", &listen_addr, CONF_VAR_STRING_POINTER, 2, 39, true);
	config_add_var(CONFIG_CORE, "network_bind_port", &port, CONF_VAR_INT, 1, 65535, false);
}

void network_sleep() {
	int nfds = epoll_wait(epoll_handle, epoll_events, EPOLL_MAX_EVENTS, 100); // 100ms timeout
	for(int i = 0; i < nfds; i++) {
		struct network_connection *net = array_get_int(sockets, epoll_events[i].data.fd);
		if (net == NULL) continue;
		if (!net->stream) {
			log_printf("client not stream, ignoring fd %d %p", epoll_events[i].data.fd, net);
			continue; // TODO: handle udp traffic
		}

		if (net->server) {
			char addr[128];
			char *ipstr;
			socklen_t addr_len = sizeof(addr);
			int fd = accept(net->fd, (struct sockaddr*)&addr, &addr_len);
			if (fd == -1) continue; // no client for us?
			fcntl(fd, F_SETFL, O_NONBLOCK); /* stupid linux does not inherit this via accept() as bsd does */

			ipstr = network_ip_string((struct sockaddr*)&addr, addr_len);

			net = calloc(sizeof(struct network_connection), 1);
			net->fd = fd;
			net->remote = malloc(addr_len);
			memcpy(net->remote, &addr, addr_len);
			net->remote_len = addr_len;
			net->stream = true;
			net->server = false;

			log_printf("new client on fd %d %p from %s", fd, net, ipstr);

			array_insert_int(sockets, fd, net);

			ssl_session_init(net);

			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = fd;
			if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, fd, &ev) == -1) {
				log_perror();
				log_printf("Failed to add new peer to poll");
				close(fd);
				free(net->remote);
				free(net);
				continue;
			}
			continue;
		}
		log_printf("event on %d (p=%p)", epoll_events[i].data.fd, net);
	}
}

bool network_init() {
	struct sockaddr_in addr;
	struct sockaddr_in6 addr6;
	int tcp_server;
	int udp_endpoint;

	sockets = array_new();

//	const char *listen_addr = "0.0.0.0";
//	uint16_t port = 65534;

	epoll_handle = epoll_create(64);
	if (epoll_handle == -1) {
		log_perror();
		log_printf("Could not create epoll struct");
		return false;
	}

	log_printf("Creating sockets on tcp/udp %s/%d", listen_addr, port);

	int af_family = -1;
	// don't know yet which one we will use
	addr.sin_family = AF_INET;
	addr6.sin6_family = AF_INET6;
	addr.sin_port = htons(port);
	addr6.sin6_port = htons(port);

	// parse addr
	if (inet_pton(AF_INET6, listen_addr, &addr6.sin6_addr) == 0) { // failed
		if (inet_pton(AF_INET, listen_addr, &addr.sin_addr) == 0) {
			log_printf("Failed to parse listen address");
			return false;
		} else {
			af_family = AF_INET;
		}
	} else {
		af_family = AF_INET6;
	}

	tcp_server = socket(af_family, SOCK_STREAM, 0);

	if (tcp_server == -1) {
		log_perror();
		log_printf("Failed to create socket for TCP server");
		return false;
	}

	udp_endpoint = socket(af_family, SOCK_DGRAM, 0);

	if (udp_endpoint == -1) {
		log_perror();
		log_printf("Failed to create socket for UDP endpoint");
		return false;
	}

	{
		int ok = 1;
		setsockopt(tcp_server, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));
		setsockopt(udp_endpoint, SOL_SOCKET, SO_REUSEADDR, &ok, sizeof(ok));
//		setsockopt(tcp_server, IPPROTO_TCP, TCP_NODELAY, &ok, sizeof(ok));
	}

	// bind stuff
	switch(af_family) {
		case AF_INET:
			if (bind(tcp_server, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
				log_perror();
				log_printf("Failed to bind TCP server");
				return false;
			}
			if (bind(udp_endpoint, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
				log_perror();
				log_printf("Failed to bind UDP endpoint");
				return false;
			}
			break;
		case AF_INET6:
			if (bind(tcp_server, (struct sockaddr *)&addr6, sizeof(addr6)) == -1) {
				log_perror();
				log_printf("Failed to bind TCP server");
				return false;
			}
			if (bind(udp_endpoint, (struct sockaddr *)&addr6, sizeof(addr6)) == -1) {
				log_perror();
				log_printf("Failed to bind UDP endpoint");
				return false;
			}
			break;
		default:
			log_printf("Unknown address family");
			return false;
	}

	if (listen(tcp_server, 5) == -1) {
		log_perror();
		log_printf("Failed to put tcp server in listen mode!");
		return false;
	}

	fcntl(tcp_server, F_SETFL, O_NONBLOCK);
	fcntl(udp_endpoint, F_SETFL, O_NONBLOCK);

	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = tcp_server;
	if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, tcp_server, &ev) == -1) {
		log_perror();
		log_printf("epoll_ctl(EPOLL_CTL_ADD) failed");
		return false;
	}
	ev.data.fd = udp_endpoint;
	if (epoll_ctl(epoll_handle, EPOLL_CTL_ADD, udp_endpoint, &ev) == -1) {
		log_perror();
		log_printf("epoll_ctl(EPOLL_CTL_ADD) failed");
		return false;
	}

	struct network_connection *net = calloc(sizeof(struct network_connection), 1);
	net->fd = tcp_server;
	net->stream = true;
	net->server = true;
	array_insert_int(sockets, tcp_server, net);
	net = calloc(sizeof(struct network_connection), 1);
	net->fd = udp_endpoint;
	net->stream = false;
	net->server = true;
	array_insert_int(sockets, udp_endpoint, net);

//	log_printf("Network initialization complete");

	return true;
}

