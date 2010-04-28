#include <gnutls/gnutls.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <gcrypt.h>

#include "ssl.h"
#include "log.h"
#include "cfg_files.h"
#include "network.h"

static gnutls_certificate_credentials_t x509_cred;
static gnutls_dh_params_t dh_params;
static gnutls_priority_t prio_cache;

static char *ssl_ca_cert;
static char *ssl_ca_crl;
static char *ssl_cert;
static char *ssl_key;
static int ssl_debug = 0;

void ssl_config_init() {
	config_add_var(CONFIG_CORE, "ssl_ca_cert", &ssl_ca_cert, CONF_VAR_STRING_POINTER, 1, 255, true);
	config_add_var(CONFIG_CORE, "ssl_ca_crl", &ssl_ca_crl, CONF_VAR_STRING_POINTER, 1, 255, true);
	config_add_var(CONFIG_CORE, "ssl_cert", &ssl_cert, CONF_VAR_STRING_POINTER, 1, 255, true);
	config_add_var(CONFIG_CORE, "ssl_key", &ssl_key, CONF_VAR_STRING_POINTER, 1, 255, true);
	config_add_var(CONFIG_CORE, "ssl_debug", &ssl_debug, CONF_VAR_INT, 0, 10, false);
}

static ssize_t ssl_gnutls_push(gnutls_transport_ptr_t ptr, const void *buf, size_t size) {
	struct network_connection *net = (struct network_connection *)ptr;
	return network_write(net, buf, size);
}

static ssize_t ssl_gnutls_pull(gnutls_transport_ptr_t ptr, void *buf, size_t size) {
	struct network_connection *net = (struct network_connection *)ptr;
	return network_read(net, buf, size);
}

bool ssl_session_init(struct network_connection *net) {
	net->ssl_ctx = calloc(sizeof(struct ssl_context), 1);
	gnutls_init(&net->ssl_ctx->session, GNUTLS_SERVER);
	gnutls_priority_set(net->ssl_ctx->session, prio_cache);
	gnutls_credentials_set(net->ssl_ctx->session, GNUTLS_CRD_CERTIFICATE, x509_cred);
	gnutls_certificate_server_set_request(net->ssl_ctx->session, GNUTLS_CERT_REQUEST);

	gnutls_transport_set_ptr(net->ssl_ctx->session, (gnutls_transport_ptr_t)net);
	gnutls_transport_set_push_function(net->ssl_ctx->session, ssl_gnutls_push);
	gnutls_transport_set_pull_function(net->ssl_ctx->session, ssl_gnutls_pull);

	int ret = gnutls_handshake(net->ssl_ctx->session);
	if (ret < 0) {
		gnutls_deinit (net->ssl_ctx->session);
		log_printf("gnutls handshake error: %s", gnutls_strerror (ret));
	}
	return false;
}

bool ssl_init() {
	const char *gnutls_version = gnutls_check_version("2.8.0");
	if (gnutls_version == NULL) {
		log_printf("Requires GnuTls version 2.8.0 minimum!");
		return false;
	}

	if (ssl_debug > 0)
		log_printf("Found GnuTLS version %s", gnutls_version);

	/* to disallow usage of the blocking /dev/random  */
	gcry_control (GCRYCTL_ENABLE_QUICK_RANDOM, 0);

	int res = gnutls_global_init();
	if (res != GNUTLS_E_SUCCESS) {
		log_printf("Got error %d from GnuTls", res);
		return false;
	}

	if (ssl_debug > 0) {
		log_printf("Enabling SSL debugging (ssl_debug=%d)", ssl_debug);
		gnutls_global_set_log_function(log_ssl_func);
		gnutls_global_set_log_level(ssl_debug);
	}

	if (gnutls_certificate_allocate_credentials(&x509_cred) != 0) {
		log_printf("Can't allocate creditentials");
		return false;
	}

	if (gnutls_certificate_set_x509_key_file(x509_cred, ssl_cert, ssl_key, GNUTLS_X509_FMT_PEM) != 0) {
		log_printf("Can't load keypair!");
		return false;
	}

	gnutls_dh_params_init(&dh_params);
	if (ssl_debug > 0)
		log_printf("Generating new pair of prime for Diffie-Hellman key exchange...");
	gnutls_dh_params_generate2(dh_params, 1024);
	gnutls_certificate_set_dh_params(x509_cred, dh_params);

	if (ssl_debug > 0)
		log_printf("Initializing CA...");

	if (gnutls_certificate_set_x509_trust_file(x509_cred, ssl_ca_cert, GNUTLS_X509_FMT_PEM) < 0) {
		log_printf("Failed to load CA certificate");
		return false;
	}

	if (gnutls_certificate_set_x509_crl_file(x509_cred, ssl_ca_crl, GNUTLS_X509_FMT_PEM) < 0) {
		log_printf("Failed to load CA CRL list");
		return false;
	}

	gnutls_certificate_set_verify_limits(x509_cred, 32768, 8);

	if (gnutls_priority_init(&prio_cache, "NORMAL", NULL) < 0) {
		log_printf("Failed to initialize gnutls priority cache");
		return false;
	}

	if (ssl_debug > 0)
		log_printf("SSL init complete");

	return true;
}

void ssl_close() {
	gnutls_certificate_free_credentials(x509_cred);
	gnutls_priority_deinit(prio_cache);
	gnutls_dh_params_deinit(dh_params);
	gnutls_global_deinit();
}

