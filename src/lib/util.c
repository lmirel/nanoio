/*
 * Utility routines
 *
 * Copyright (c) 2016, Wind River Systems, Inc.
 * All rights reserved.
 *
 * See "LICENSE" for license terms.
 *
 * Author:
 *      Lans Zhang <jia.zhang@windriver.com>
 */

#include <nnio.h>

static bool show_verbose;

bool
nnio_util_verbose(void)
{
	return show_verbose;
}

void
nnio_util_set_verbosity(bool verbose)
{
	show_verbose = verbose;
}
