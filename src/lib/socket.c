/*
 * nanomsg socket
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

int
nnio_socket_open(int protocol, int tx_timeout, int rx_timeout,
		 const char *socket_name, int linger_timeout)
{
	int sock;

	sock = nn_socket(AF_SP, protocol);
	nnio_error_assert(sock >= 0, "Unable to create socket");

	nnio_socket_set_tx_timeout(sock, tx_timeout);
	nnio_socket_set_rx_timeout(sock, rx_timeout);
	nnio_socket_set_name(sock, socket_name);
	nnio_socket_set_linger_timeout(sock, linger_timeout);

	return sock;
}

void
nnio_socket_close(int sock)
{
	int err;

	do {
		int rc = nn_close(sock);
		if (!rc)
			return;

		err = nn_errno();
		err("Closing socket interrupted\n");
	} while (err == EINTR);

	nnio_error_assert(0, "Unable to close socket");
}

int
nnio_socket_set_tx_timeout(int sock, int timeout)
{
	if (timeout < 0)
		return -1;

	int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDTIMEO,
			       &timeout, sizeof(timeout));
	nnio_error_assert(!rc, "Unable to set tx timeout");

	return rc;
}

int
nnio_socket_set_rx_timeout(int sock, int timeout)
{
	if (timeout < 0)
		return -1;

	int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVTIMEO,
			       &timeout, sizeof(timeout));
	nnio_error_assert(!rc, "Unable to set rx timeout");

	return rc;
}

int
nnio_socket_set_linger_timeout(int sock, int timeout)
{
	if (timeout < 0)
		return -1;

	int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_LINGER,
			       &timeout, sizeof(timeout));
	nnio_error_assert(!rc, "Unable to set linger timeout");

	return rc;
}

int
nnio_socket_set_name(int sock, const char *name)
{
	if (!name)
		return -1;

	int rc = nn_setsockopt(sock, NN_SOL_SOCKET, NN_SOCKET_NAME,
			       name, strlen(name));
	nnio_error_assert(!rc, "Unable to set socket name");

	return rc;
}

int
nnio_socket_tx(int sock, void *data, unsigned int data_len)
{
	int err;

	do {
		int rc = nn_send(sock, data, data_len, 0);
		if (rc >= 0) {
			dbg("sending %d-byte data ...\n", rc);
			return rc;
		}

		err = nn_errno();
	} while (err == EINTR);

	if (err == ETIMEDOUT) {
		dbg("Tx timeout\n");
		return -1;
	}

	if (err == EAGAIN) {
		err("Try to tx again\n");
		return -1;
	}

	nnio_error_assert(0, "Failed to send the data");

	return -1;
}

int
nnio_socket_rx(int sock, void **data, unsigned int *data_len)
{
	int err;

	do {
		int len = nn_recv(sock, data, NN_MSG, 0);
		if (len >= 0) {
			/* nn_recv() may leak EAGAIN sometimes */
			errno = 0;
			*data_len = len;
			return len;
		}

		err = nn_errno();
	} while (err == EINTR);

	if (err == ETIMEDOUT) {
		dbg("Rx timeout\n");
		return -1;
	}

	if (err == EAGAIN) {
		err("Try to rx again\n");
		return -1;
	}

	nnio_error_assert(0, "Failed to receive the data");

	return -1;
}

int
nnio_socket_tx_iov(int sock, struct nn_iovec *iov, unsigned int nr_iov)
{
	struct nn_msghdr hdr;

	memset(&hdr, 0, sizeof(hdr));
	hdr.msg_iov = iov;
	hdr.msg_iovlen = nr_iov;

	int err;

	do {
		int len = nn_sendmsg(sock, &hdr, 0);
		if (len >= 0)
			return len;

		err = nn_errno();
	} while (err == EINTR);

	if (err == ETIMEDOUT) {
		dbg("Tx iov timeout\n");
		return -1;
	}

	if (err == EAGAIN) {
		err("Need to tx iov again\n");
		return -1;
	}

	nnio_error_assert(0, "Failed to send iov data");

	return -1;
}
