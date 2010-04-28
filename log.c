#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "log.h"

bool log_init() {
	return true;
}

void log_perror() {
	fprintf(stderr, "System error: %s\n", strerror(errno));
}

int log_printf(const char *format, ...) {
	int res;
	va_list ap;
	va_start(ap, format);
	res = vfprintf(stderr, format, ap);
	va_end(ap);

	fprintf(stderr, "\n");

	return res;
}

void log_ssl_func(int level, const char* msg) {
	log_printf("%s", msg);
}

