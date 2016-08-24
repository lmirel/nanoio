/*
 * nnio server
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
	info_cont("  --tx-timeout, -t: Set the socket tx timeout\n");
	info_cont("  --rx-timeout, -r: Set the socket rx timeout\n");
	info_cont("  --linger-timeout, -l: Set the socket linger timeout\n");
	info_cont("  --socket-name, -n: Set the socket name\n");
	info_cont("  --local-endpoint, -L: <url> argument is local\n");
	info_cont("  --exec, -E: Execute an executable\n");
	info_cont("  --log-file, -g: Specify the log file\n");
	info_cont("  --daemon, -d: Run as daemon\n");
	info_cont("\nurl:\n");
	info_cont("  Specify the transport\n");
}

static void
exit_notify(void)
{
	if (nnio_util_verbose()) {
		int err = nn_errno();

		info("nanoserver exiting with %d (%s)\n", err,
		     nn_strerror(err));
	}
}

/*
 * Daemonlize nanoserver.
 * - Change CWD to /.
 * - Redirect stdin to /dev/null.
 * - Redirect stdout and stderr to log file.
 */
static int
daemonlize(nnio_options_t *options)
{
	if (options->daemon == true)
		nnio_error_assert(!daemon(0, 1), "Failed to daemonlize");

	if (!options->log_file)
		return 0;

	int log_fd = open(options->log_file,
			  O_WRONLY | O_CREAT | O_APPEND,
			  S_IRUSR | S_IWUSR | S_IRGRP);
	nnio_error_assert(log_fd >= 0, "Failed to open log file %s",
			  options->log_file);

	int null_fd = open("/dev/null", O_RDWR);
	nnio_error_assert(null_fd > 0, "Unable to open /dev/null");

	nnio_error_assert(dup2(null_fd, STDIN_FILENO) == STDIN_FILENO,
			  "Unable to redirect stdin to /dev/null");
	close(null_fd);

	nnio_error_assert(dup2(log_fd, STDOUT_FILENO) == STDOUT_FILENO,
			  "Unable to redirect stdout to log file");
	nnio_error_assert(dup2(log_fd, STDERR_FILENO) == STDERR_FILENO,
			  "Unable to redirect stderr to log file");
	close(log_fd);

	return 0;
}


int
main(int argc, char **argv)
{
	atexit(exit_notify);

	nnio_options_t options = {
		.show_usage = show_usage,
	};

	nnio_options_parse(argc, argv, &options);

	daemonlize(&options);

	if (!options.quite)
		nnio_show_banner(argv[0]);

	int sock = nnio_socket_open(NN_REP, options.tx_timeout,
				    options.rx_timeout, options.socket_name,
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

run_again:
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

	if (options.exec) {
		rc = nnio_spawn(sock, options.exec, data, data_len);
		if (!rc) {
			nnio_free_data(data);
			goto run_again;
		}

		goto err_eof;
	}

	/* Do an echo service */
	dbg("preparing to send %d-byte to socket ...\n", data_len);

	int len = nnio_socket_tx(sock, data, data_len);
	if (len < 0) {
		err("Failed to send %d-byte data to socket\n", data_len);
		rc = -1;
	}

err_eof:
	nnio_free_data(data);

err_socket_rx:
	/* If the tx socket is closed before the sent data received, the rx
	 * socket would be blocked forever. Essentially speaking, this is
	 * caused by the lack of the support for the linger timeout in
	 * nanomsg.
	 */
	if (options.exit_delay)
		usleep(options.exit_delay);

	dbg("closing socket ...\n");

	nnio_endpoint_delete(sock, ep);

err_add_endpoint:
	nnio_socket_close(sock);

	return rc;
}
