/*
 * nanomsg endpoint
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
nnio_endpoint_add_local(int sock, const char *endpoint)
{
	if (!endpoint)
		return -1;

	int rc = nn_bind(sock, endpoint);
	if (rc >= 0) {
		/* nn_connect() may leak EAGAIN sometimes */
		errno = 0;
		return rc;
	}

	nnio_error_assert(rc >= 0, "Unable to add the local endpoint");

	return rc;
}

int
nnio_endpoint_add_remote(int sock, const char *endpoint)
{
	if (!endpoint)
		return -1;

	int rc = nn_connect(sock, endpoint);
	if (rc >= 0) {
		/* nn_connect() may leak ENOENT sometimes */
		errno = 0;
		return rc;
	}

	/* ECONNREFUSED is returned if the remote endpoint currently
	 * is not able to be connected. In this case the assertion is
	 * not appropriate.
	 */
	int err = nn_errno();
	nnio_error_assert(err == ECONNREFUSED, "Unable to add remote endpoint");

	err("Unable to connect remote endpoint\n");
	return -1;
}

void
nnio_endpoint_delete(int sock, int endpoint)
{
	int err;

	do {
		int rc = nn_shutdown(sock, endpoint);
		if (!rc)
			return;

		err = nn_errno();
	} while (err == EINTR);

	nnio_error_assert(0, "Unable to close socket");
}
