/*
 * nnio reader
 *
 * Copyright (c) 2016, Wind River Systems, Inc.
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *	  Lans Zhang <jia.zhang@windriver.com>
 */

#include <nnio.h>

static void
show_usage(const char *prog)
{
	info_cont("usage: %s <options> <url>\n", prog);
	info_cont("\noptions:\n");
	info_cont("  --help, -h: Print this help information\n");
	info_cont("  --version, -V: Show version number\n");
	info_cont("  --verbose, -v: Show verbose messages\n");
	info_cont("  --quite, -q: Don't show banner information\n");
	info_cont("  --rx-timeout, -r: Set the socket rx timeout\n");
	info_cont("  --socket-name, -n: Set the socket name\n");
	info_cont("  --local-endpoint, -L: <url> argument is local\n");
	info_cont("\nurl:\n");
	info_cont("  Specify the transport\n");
}

static void
exit_notify(void)
{
	if (nnio_util_verbose()) {
		int err = nn_errno();

		info("nanoread exiting with %d (%s)\n", err,
		     nn_strerror(err));
	}
}

int
main(int argc, char **argv)
{
	atexit(exit_notify);

	nnio_options_t options = {
		.show_usage = show_usage,
	};

	nnio_options_parse(argc, argv, &options);

	if (!options.quite)
		nnio_show_banner(argv[0]);

	int sock = nnio_socket_open(NN_PULL, -1, options.rx_timeout,
				    options.socket_name,
				    options.linger_timeout);
	if (sock < 0)
		return -1;

	nnio_error_assert(options.local_endpoint, "-L option required");

	int rc;
	int ep = nnio_endpoint_add_local(sock, *options.local_endpoint);
	if (ep < 0) {
		rc = -1;
		goto err_add_endpoint;
	}

	dbg("preparing to receive data from socket ...\n");

	void *data;
	unsigned int data_len;
	rc = nnio_socket_rx(sock, &data, &data_len);
	if (rc < 0) {
		dbg("Failed to receive data from socket\n");
		goto err_socket_rx;
	}

	rc = 0;

	if (!data_len) {
		if (nnio_util_verbose())
			info("read socket EOF\n");

		goto err_eof;
	}

	dbg("reading %d-byte from socket ... \n", data_len);

	fprintf(stdout, "%s", (char *)data);
	fflush(stdout);

	if (nnio_util_verbose())
		info("Total rx length: %d-byte\n", data_len);

err_eof:
	nnio_free_data(data);

err_socket_rx:
	nnio_endpoint_delete(sock, ep);

err_add_endpoint:
	nnio_socket_close(sock);

	return rc;
}
