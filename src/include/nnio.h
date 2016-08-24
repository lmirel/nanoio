/*
 * nnio library
 *
 * Copyright (c) 2016, Wind River Systems, Inc.
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <jia.zhang@windriver.com>
 */

#ifndef NNIO_H
#define NNIO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <alloca.h>
#include <getopt.h>
#include <fcntl.h>
#include <dirent.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <linux/limits.h>

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/bus.h>
#include <nanomsg/pair.h>
#include <nanomsg/survey.h>

extern char
nnio_git_commit[];

extern char
nnio_build_machine[];

#define gettid()		syscall(__NR_gettid)

#define __pr__(level, fmt, ...)	\
	do {	\
		struct timeval tv;	\
		gettimeofday(&tv, NULL);	\
		struct tm loc;	\
		localtime_r(&tv.tv_sec, &loc);	\
		char buf[64]; \
		strftime(buf, sizeof(buf), "%Y %b %e %a %Z %T", &loc);	\
		fprintf(stderr, "%s.%ld: [" #level "]: " fmt, buf, tv.tv_usec, ##__VA_ARGS__);	\
	} while (0)

#define die(fmt, ...)	\
	do {	\
		__pr__(FAULT, fmt, ##__VA_ARGS__);	\
		exit(EXIT_FAILURE);	\
	} while (0)

#ifdef DEBUG
  #define dbg(fmt, ...)	\
	do {	\
		__pr__(DEBUG, fmt, ##__VA_ARGS__);	\
	} while (0)

  #define dbg_cont(fmt, ...)	\
	do {	\
		fprintf(stdout, fmt, ##__VA_ARGS__);	\
	} while (0)
#else
  #define dbg(fmt, ...)
  #define dbg_cont(fmt, ...)
#endif

#define info(fmt, ...)	\
	do {	\
		__pr__(INFO, fmt, ##__VA_ARGS__);	\
	} while (0)

#define info_cont(fmt, ...)	\
	fprintf(stdout, fmt, ##__VA_ARGS__)

#define warn(fmt, ...)	\
	do {	\
		__pr__(WARNING, fmt, ##__VA_ARGS__);	\
	} while (0)

#define err(fmt, ...)	\
	do {	\
		__pr__(ERROR, fmt, ##__VA_ARGS__);	\
	} while (0)

#define err_cont(fmt, ...)	\
	fprintf(stdout, fmt, ##__VA_ARGS__)

#define nnio_error_assert(condition, fmt, ...)	\
	do {	\
		if (!(condition)) {	\
			int err = errno;	\
			err(fmt ": %s\n", ##__VA_ARGS__, nn_strerror(err)); \
			exit(EXIT_FAILURE);	\
		}	\
	} while (0)

typedef struct {
	/* Input parameters */
	void (*show_usage)(const char *prog);

	/* Output parameters */
	const char *url;
	int protocol;
	int tx_timeout;		/* in millisecond */
	int rx_timeout;		/* in millisecond */
	int linger_timeout;	/* in millisecond */
	const char *socket_name;
	const char **local_endpoint;
	const char **remote_endpoint;
	int exit_delay;		/* in millisecond */
	bool quite;
	const char *exec;
	bool daemon;
	const char *log_file;
} nnio_options_t;

typedef struct {
	sem_t *lock;
	int shm_fd;
} nnio_sync_t;

nnio_sync_t *
nnio_sync_init(const char *name);

void
nnio_sync_wait(nnio_sync_t *sync);

void
nnio_sync_post(nnio_sync_t *sync);

void
nnio_sync_finish(nnio_sync_t *sync);

void
nnio_show_banner(const char *prog_desc);

void
nnio_show_version(void);

int
nnio_socket_open(int protocol, int tx_timeout, int rx_timeout,
		 const char *socket_name, int linger_timeout);

void
nnio_socket_close(int sock);

int
nnio_socket_set_tx_timeout(int sock, int timeout);

int
nnio_socket_set_rx_timeout(int sock, int timeout);

int
nnio_socket_set_linger_timeout(int sock, int timeout);

int
nnio_socket_set_name(int sock, const char *name);

int
nnio_socket_tx(int sock, void *data, unsigned int data_len);

int
nnio_socket_rx(int sock, void **data, unsigned int *data_len);

int
nnio_socket_tx_iov(int sock, struct nn_iovec *iov, unsigned int nr_iov);

int
nnio_endpoint_add_local(int sock, const char *endpoint);

int
nnio_endpoint_add_remote(int sock, const char *endpoint);

void
nnio_endpoint_delete(int sock, int endpoint);

int
nnio_options_parse(int argc, char *argv[], nnio_options_t *options);

bool
nnio_util_verbose(void);

void
nnio_util_set_verbosity(bool verbose);

void *
nnio_alloc_data(unsigned long data_len);

void
nnio_free_data(void *data);

int
nnio_spawn(int sock, const char *exec, void *data, unsigned int data_len);

#endif	/* NNIO_H */
