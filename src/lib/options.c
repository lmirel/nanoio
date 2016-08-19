/*
 * option parser
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

extern void
nnio_show_banner(const char *prog_desc);

extern void
nnio_show_version(void);

static int
check_protocol(const char *name)
{
	struct {
		const char *name;
		int protocol;
	} protocol_names[] = {
		{ "push", NN_PUSH },
		{ "pull", NN_PULL },
		{ "pub", NN_PUB },
		{ "sub", NN_SUB },
		{ "req", NN_REQ },
		{ "rep", NN_REP },
		{ "bus", NN_BUS },
		{ "pair", NN_PAIR },
		{ "surveyor", NN_SURVEYOR },
		{ "respondent", NN_RESPONDENT },
		{ NULL, 0 },
	};

	for (int i = 0; protocol_names[i].name; ++i) {
		if (!strcmp(protocol_names[i].name, name))
			return protocol_names[i].protocol;
	}

	nnio_error_assert(0, "Invalid protocol %s specified", name);

	return -1;
}

int
nnio_options_parse(int argc, char *argv[], nnio_options_t *options)
{
	char opts[] = "-hVvqp:t:r:n:RLl:e:E:";
	struct option long_opts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'V' },
		{ "verbose", no_argument, NULL, 'v' },
		{ "quite", no_argument, NULL, 'q' },
		{ "protocol", required_argument, NULL, 'p' },
		{ "tx-timeout", required_argument, NULL, 't' },
		{ "rx-timeout", required_argument, NULL, 'r' },
		{ "linger-timeout", required_argument, NULL, 'l' },
		{ "socket-name", required_argument, NULL, 'n' },
		{ "remote-endpoint", no_argument, NULL, 'R' },
		{ "local-endpoint", no_argument, NULL, 'L' },
		{ "exit-delay", no_argument, NULL, 'e' },
		{ "exec", required_argument, NULL, 'E' },
		{ 0, },	/* NULL terminated */
	};

	/* Initial all output parameters */
	options->protocol = -1;
	options->tx_timeout = -1;
	options->rx_timeout = -1;
	options->linger_timeout = -1;
	options->socket_name = NULL;
	options->local_endpoint = NULL;
	options->remote_endpoint = NULL;
	options->exit_delay = 0;
	options->exec = NULL;
	options->quite = 0;

	while (1) {
		int opt;

		opt = getopt_long(argc, argv, opts, long_opts, NULL);
		if (opt == -1)
			break;

		switch (opt) {
		case '?':
			err("Unrecognized option\n");
			return -1;
		case 'h':
			options->show_usage(argv[0]);
			exit(EXIT_SUCCESS);
		case 'V':
			nnio_show_version();
			exit(EXIT_SUCCESS);
		case 'v':
			nnio_util_set_verbosity(1);
			break;
		case 'q':
			options->quite = 1;
			break;
		case 'p':
			options->protocol = check_protocol(optarg);
			break;
		case 't':
			options->tx_timeout = atoi(optarg);
			break;
		case 'r':
			options->rx_timeout = atoi(optarg);
			break;
		case 'l':
			options->linger_timeout = atoi(optarg);
			break;
		case 'n':
			options->socket_name = optarg;
			break;
		case 'R':
			options->remote_endpoint = &options->url;
			break;
		case 'L':
			options->local_endpoint = &options->url;
			break;
		case 'e':
			options->exit_delay = atoi(optarg);
			break;
		case 'E':
			options->exec = optarg;
			break;
		case 1:
			options->url = optarg;
			break;
		default:
			return -1;
		}
	}

	if (!options->remote_endpoint && !options->local_endpoint)
		nnio_error_assert(0, "either -R or -L option required");

	nnio_error_assert(options->url, "<url> argument required");

	return 0;
}
