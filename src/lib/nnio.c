/*
 * nnio core
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

void
nnio_show_banner(const char *prog_desc)
{
	info_cont("\n%s\n", prog_desc);
	info_cont("(C)Copyright 2016 Wind River Systems, Inc.\n");
	info_cont("Author: Lans Zhang <jia.zhang@windriver.com>\n");
	info_cont("Version: %s+git%s\n", NANOIO_VERSION, nnio_git_commit);
	info_cont("Build Machine: %s\n", nnio_build_machine);
	info_cont("Build Time: " __DATE__ " " __TIME__ "\n\n");
}

void
nnio_show_version(void)
{
	info_cont("%s\n", NANOIO_VERSION);
}

void *
nnio_alloc_data(unsigned long data_len)
{
	return nn_allocmsg(data_len, 0);
}

void
nnio_free_data(void *data)
{
	nn_freemsg(data);
}

/* Construct argv[] for execvp() */
static char **
construct_argv(const char *argument)
{
	char *args = malloc(strlen(argument) + 1);
	nnio_error_assert(args, "Failed to allocate args");

	memcpy(args, argument, strlen(argument) + 1);

	char **argv = (char **)malloc(sizeof(char *));
	nnio_error_assert(argv, "Failed to allocate argv");

	int argc = 0;
	argv[0] = NULL;
	char *curr_arg = args;
	while (*curr_arg) {
		char *prev_arg;

		/* Skip preposed spaces */
		while (*curr_arg && isspace(*curr_arg))
			++curr_arg;

		if (*curr_arg)
			prev_arg = curr_arg++;
		else
			break;

		/* Skip characters */
		while (*curr_arg && !isspace(*curr_arg))
			++curr_arg;

		argv = realloc(argv, sizeof(char *) * (++argc + 1));
		if (!argv)
			nnio_error_assert(argv, "Failed to re-allocate argv");

		argv[argc - 1] = prev_arg;
		argv[argc] = NULL;

		if (*curr_arg)
			*curr_arg++ = 0;
	}

	return argv;
}

int
nnio_spawn(int sock, const char *exec, void *data, unsigned int data_len)
{
	int input_fds[2], output_fds[2];
	int rc;

	nnio_error_assert(signal(SIGPIPE, SIG_IGN) != SIG_ERR,
			  "Unable to capture SIGPIPE");

	/* Create two pipelines for reading and writing */
	rc = pipe(input_fds);
	nnio_error_assert(rc >= 0, "Error on creating the pipe for input");

	rc = pipe(output_fds);
	nnio_error_assert(rc >= 0, "Error on creating the pipe for output");

	const char *shm_name = "__spawn__";
	int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	nnio_error_assert(shm_fd >= 0, "Error on creating shared memory object");

	shm_unlink(shm_name);

	/* Used to sync up with child process for feeding its stdin */
	rc = ftruncate(shm_fd, sizeof(sem_t));
	nnio_error_assert(!rc, "Error on truncating");

	sem_t *lock = mmap(NULL, sizeof(*lock), PROT_READ | PROT_WRITE,
			   MAP_SHARED, shm_fd, 0);
	nnio_error_assert(lock != MAP_FAILED, "Error on mmap");

	rc = sem_init(lock, 1, 0);
	nnio_error_assert(!rc, "Error on initializing semaphore");

	pid_t child = fork();
	nnio_error_assert((int)child >= 0, "Error on forking subprocess");

	if (!child) {
		int fd = -1;

		/* For debugging */
		if (nnio_util_verbose())
			fd = dup(STDOUT_FILENO);

		close(input_fds[1]);
		close(output_fds[0]);

		/* Bind the stdin to the input endpoint of input pipe */
		dup2(input_fds[0], STDIN_FILENO);
		/* Bind the stdout/stderr to the output endpoint of output
		 * pipe */
		dup2(output_fds[1], STDOUT_FILENO);
		dup2(output_fds[1], STDERR_FILENO);

		close(input_fds[0]);
		close(output_fds[1]);

		char **argv = construct_argv(exec);
		nnio_error_assert(argv, "Error on creating argv[]");

		/* Unlocked by the parent after feeding stdin */
		sem_wait(lock);

		if (nnio_util_verbose()) {
			FILE *fp = fdopen(fd, "w");

			nnio_error_assert(fp, "Failed to open fd");
			fprintf(fp, "child starting ...\n");
			fflush(fp);
			close(fd);
		}

		execvp(argv[0], argv);

		/* Should not return */
		nnio_error_assert(0, "Error on executing subprocess");
	}

	close(input_fds[0]);
	close(output_fds[1]);

	ssize_t sz;
	while ((int)data_len > 0) {
		sz = write(input_fds[1], data, data_len);
		if (sz < 0 && errno == EPIPE)
		    break;

		nnio_error_assert(sz >= 0, "Failed to write");

		if (sz == data_len)
		    break;

		data += sz;
		data_len -= sz;
	}

	close(input_fds[1]);
	signal(SIGPIPE, SIG_DFL);

	dbg("parent syncing up ...\n");
	sem_post(lock);

	struct nn_iovec *iov = NULL;
	unsigned int nr_iov = 0;
	unsigned long total_tx_len = 0;
	while (1) {
		void *buf = nnio_alloc_data(PIPE_BUF);
		nnio_error_assert(buf, "Failed to allocate buffer");

		ssize_t buf_len = read(output_fds[0], buf, PIPE_BUF);
		nnio_error_assert(buf_len >= 0, "Failed to read");
		if (!buf_len)
		    break;

		iov = realloc(iov, (++nr_iov) * sizeof(*iov));
		nnio_error_assert(iov, "Failed to allocate iov");

		iov[nr_iov - 1].iov_base = buf;
		iov[nr_iov - 1].iov_len = buf_len;

		total_tx_len += buf_len;
	}

	close(output_fds[0]);

	sem_destroy(lock);
	munmap(lock, sizeof(*lock));
	close(shm_fd);

	dbg("preparing to send %ld-byte to socket ...\n", total_tx_len);

	rc = nnio_socket_tx_iov(sock, iov, nr_iov);
	if (rc < 0) {
		err("Failed to send %ld-byte data to socket\n",
		    total_tx_len);
	}

	for (unsigned int i = 0; i < nr_iov; ++i)
		nnio_free_data(iov[i].iov_base);
	free(iov);

	wait(NULL);

	return 0;
}
