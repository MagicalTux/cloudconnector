#include <gnutls/gnutls.h>

struct ssl_context {
	gnutls_session_t session;
};

bool ssl_init();
void ssl_config_init();

struct network_connection;

bool ssl_session_init(struct network_connection *);

