/* src/vm/rt-timing.c - POSIX real-time timing utilities

   Copyright (C) 1996-2005, 2006 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Contact: cacao@cacaojvm.org

   Authors: Edwin Steiner

   Changes:

   $Id$

*/


#include "config.h"
#include "vm/types.h"

#include <assert.h>
#include <time.h>
#include <errno.h>

#include "vm/rt-timing.h"
#include "mm/memory.h"
#include "vm/global.h"

struct rt_timing_stat {
	int index;
	const char *name;
};

static struct rt_timing_stat rt_timing_stat_defs[] = {
	{ RT_TIMING_CHECKS,    "checks at beginning" },
	{ RT_TIMING_PARSE,     "parse" },
	{ RT_TIMING_STACK,     "analyse_stack" },
	{ RT_TIMING_TYPECHECK, "typecheck" },
	{ RT_TIMING_LOOP,      "loop" },
	{ RT_TIMING_IFCONV,    "if conversion" },
	{ RT_TIMING_ALLOC,     "register allocation" },
	{ RT_TIMING_RPLPOINTS, "replacement point generation" },
	{ RT_TIMING_CODEGEN,   "codegen" },
	{ RT_TIMING_TOTAL,     "total" },
	{ 0,                   NULL }
};

static long long rt_timing_sum[RT_TIMING_N] = { 0 };

void rt_timing_gettime(struct timespec *ts)
{
	if (clock_gettime(CLOCK_THREAD_CPUTIME_ID,ts) != 0) {
		fprintf(stderr,"could not get time by clock_gettime: %s\n",strerror(errno));
		abort();
	}
}

static long rt_timing_diff_usec(struct timespec *a,struct timespec *b)
{
	long diff;
	time_t atime;

	diff = (b->tv_nsec - a->tv_nsec) / 1000;
	atime = a->tv_sec;
	while (atime < b->tv_sec) {
		atime++;
		diff += 1000000;
	}
	return diff;
}

void rt_timing_diff(struct timespec *a,struct timespec *b,int index)
{
	long diff;

	diff = rt_timing_diff_usec(a,b);
	rt_timing_sum[index] += diff;
}

void rt_timing_print_time_stats(FILE *file)
{
	struct rt_timing_stat *stats;
	double total;

	stats = rt_timing_stat_defs;
	total = rt_timing_sum[RT_TIMING_TOTAL];
	while (stats->name) {
		fprintf(file,"%12lld usec %3.0f%% %s\n",
				rt_timing_sum[stats->index],
				(total != 0.0) ? rt_timing_sum[stats->index] / total * 100.0 : 0.0,
				stats->name);
		stats++;
	}
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
