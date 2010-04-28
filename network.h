
struct network_connection {
	int fd;
	struct ssl_context *ssl_ctx;
	struct sockaddr *remote;
	int remote_len;
	bool stream; // false=udp true=tcp
	bool server;

	// read buffer
	void *read_buf;
	size_t read_buf_size, read_buf_pos;
	// write buffer
	void *write_buf;
	size_t write_buf_size, write_buf_pos;
};

void network_config_init();
bool network_init();
void network_sleep();

ssize_t network_read(struct network_connection *net, void*buf, size_t size);
ssize_t network_write(struct network_connection *net, const void*buf, size_t size);

