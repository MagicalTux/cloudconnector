#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "ssl.h"
#include "network.h"
#include "cfg_files.h"

void do_fork() {
	// TODO HERE: FORK (after checking for config var core_fork ?)
}

bool stop;

int main(int argc, char *argv[]) {
	stop = false;
	config_add("cloudconnector.conf", CONFIG_CORE);
	ssl_config_init();
	network_config_init();
	if (!config_parse(CONFIG_CORE)) return 1;
	if (!log_init()) return 1;
	do_fork();
	log_printf("CloudConnector initializing on pid %d", getpid());
	if (!ssl_init()) return 1;
	if (!network_init()) return 1;

	while(!stop) {
		network_sleep();
	}

	return 0;
}

