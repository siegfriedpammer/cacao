/* vmlog - high-speed logging for free VMs                  */
/* Copyright (C) 2006 Edwin Steiner <edwin.steiner@gmx.net> */
/*               2007 Peter Molnar <peter.molnar@wm.sk>     */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "vmlog.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <getopt.h>

char *g_idxfname;
char *g_strfname;
char *g_logfname;
int g_idxlen;
int g_strlen;
int g_nstrings;
vmlog_string_entry *g_idxmap;
char *g_strmap;
unsigned char *g_matches;
vmlog_ringbuf *g_ringbuf = NULL;
vmlog_log *g_log = NULL;
vmlog_thread_log *g_thread_log = NULL;

char *g_cmd = "vmlogfilter";

char *opt_src_prefix = NULL;
char *opt_dest_prefix = NULL;
char *opt_filter_include = NULL;
char *opt_filter_exclude = NULL;
char *opt_src_threadidx = "0";
char *opt_dest_threadidx = "100";

#define VMLOGDUMP_RINGBUF_SIZE (16 * 1024)
#define VMLOGDUMP_PREFETCH 1024
#if 0
#	define LOG(...) fprintf(stderr, "LOG: " __VA_ARGS__)
#else
#	define LOG(...)
#endif

#define USAGE \
	"Usage: %s <options>\n" \
	"\n" \
	"Mandatory options include:\n" \
	"\n" \
	"\t-p <prefix>      Source prefix.\n" \
	"\n" \
	"Optional options include:\n" \
	"\n" \
	"\t-t <threadidx>   Source thread index. Defaults to 0.\n" \
	"\t-P <prefix>      Destination prefix, defaults to source prefix.\n" \
	"\t-T <threadidx>   Destination thread index. Defaults to 100.\n" \
	"\t-i <regex>       Filter include.\n" \
	"\t-x <regex>       Filter exclude.\n" \
	"\n" 

static void *xmalloc(int size) {
	void *ret = malloc(size);
	if (ret == NULL) {
		vmlog_die("Malloc of size %d failed.", size);
	}
	return ret;
}

static void usage(int exit_status) {
	fprintf(stderr, USAGE, g_cmd);
	exit(exit_status);
}

static void parse_options(int argc, char **argv) {
	int ret;

	while ((ret = getopt(argc, argv, "i:x:p:t:P:T:h")) != -1) {
		switch (ret) {
			case 'i':
				opt_filter_include = optarg;
				break;
			case 'x':
				opt_filter_exclude = optarg;
				break;
			case 't':
				opt_src_threadidx = optarg;
				break;
			case 'p':
				opt_src_prefix = optarg;
				if (opt_dest_prefix == NULL) {
					opt_dest_prefix = optarg;
				}
			case 'T':
				opt_dest_threadidx = optarg;
				break;
			case 'P':
				opt_dest_prefix = optarg;
				break;
			case 'h':
				usage(EXIT_SUCCESS);
				break;
			default:
				usage(EXIT_FAILURE);
				break;
		}
	}

	if ((opt_src_prefix == NULL) || (opt_dest_prefix == NULL)) {
		usage(EXIT_FAILURE);
	}
}

static void match_strings() {
	regex_t regex[2];
	char *filter[2];
	int default_match[2] = { 1, 0 };
	int i, err, res;
	char err_buf[128];
	char *str = NULL;
	int str_size = 0;
	vmlog_string_entry *strent;
	unsigned char *match;

	/* Initialize */

	filter[0] = opt_filter_include;
	filter[1] = opt_filter_exclude;

	/* Compile regexes */

	for (i = 0; i < 2; ++i) {
		if (filter[i] != NULL) {
			err = regcomp(&regex[i], filter[i], REG_EXTENDED | REG_NOSUB);
			if (err != 0) {
				regerror(err, &regex[i], err_buf, sizeof(err_buf));
				vmlog_die(
					"Invalid regex %s: %s",
					filter[i], err_buf
				);
			}
		}
	}

	/* Allocate array. For each string one byte with flags. */

	g_matches = (unsigned char *)xmalloc(g_nstrings * sizeof(unsigned char));

	/* Traverse strings and match */

	for (strent = g_idxmap, match = g_matches; strent != g_idxmap + g_nstrings; ++strent, ++match) {
		*match = 0;

		if ((strent->len + 1) > str_size) {
			str_size = strent->len + 1;
			free(str);
			str = (char *)xmalloc(str_size);
		}
		memcpy(str, g_strmap + strent->ofs, strent->len);
		str[strent->len] = '\0';

		for (i = 0; i < 2; ++i) {
			if (filter[i] != NULL) {
				res = regexec(&regex[i], str, 0, NULL, 0);
				if (res == 0) {
					*match |= (1 << i);
					LOG("String `%s' matches regex `%d (%s)'\n", str, i, filter[i]);
				}
			} else {
				*match |= (default_match[i] << i);
			}
		}
	}
}

/* see src/vm/jit/show.c in the cacao source tree for documentation on the below. */

#define MATCHFLAGS match_flags
#define FILTERVERBOSECALLCTR g_ctr
#define STATE_IS_INITIAL() ((FILTERVERBOSECALLCTR[0] == 0) && (FILTERVERBOSECALLCTR[1] == 0))
#define STATE_IS_INCLUDE() ((FILTERVERBOSECALLCTR[0] > 0) && (FILTERVERBOSECALLCTR[1] == 0))
#define STATE_IS_EXCLUDE() (FILTERVERBOSECALLCTR[1] > 0)
#define EVENT_INCLUDE() (MATCHFLAGS & (1 << 0))
#define EVENT_EXCLUDE() (MATCHFLAGS & (1 << 1))
#define TRANSITION_NEXT_INCLUDE() ++FILTERVERBOSECALLCTR[0]
#define TRANSITION_PREV_INCLUDE() --FILTERVERBOSECALLCTR[0]
#define TRANSITION_NEXT_EXCLUDE() ++FILTERVERBOSECALLCTR[1]
#define TRANSITION_PREV_EXCLUDE() --FILTERVERBOSECALLCTR[1]

int g_ctr[2] = { 0, 0 };

static int test_enter(unsigned char match_flags) {

	int force_show = 0;

	if (STATE_IS_INITIAL()) {
		if (EVENT_INCLUDE()) {
			TRANSITION_NEXT_INCLUDE();
		}
	} else if (STATE_IS_INCLUDE()) {
		if (EVENT_EXCLUDE()) {
			TRANSITION_NEXT_EXCLUDE();
			/* just entered exclude, show this method */
			force_show = 1;
		} else if (EVENT_INCLUDE()) {
			TRANSITION_NEXT_INCLUDE();
		}
	} else if (STATE_IS_EXCLUDE()) {
		if (EVENT_EXCLUDE()) {
			TRANSITION_NEXT_EXCLUDE();
		}
	}

	return STATE_IS_INCLUDE() || force_show;
}

static int test_exit(unsigned char match_flags) {

	int force_show = 0;

	if (STATE_IS_INCLUDE()) {
		if (EVENT_INCLUDE()) {
			TRANSITION_PREV_INCLUDE();
			/* just entered initial, show this method */
			if (STATE_IS_INITIAL()) force_show = 1;
		}
	} else if (STATE_IS_EXCLUDE()) {
		if (EVENT_EXCLUDE()) {
			TRANSITION_PREV_EXCLUDE();
		}
	}

	return STATE_IS_INCLUDE() || force_show;
}

static int test_other() {
	return STATE_IS_INCLUDE();
}

static void do_filter() {
	vmlog_log_entry *logent;
	while ((logent = vmlog_ringbuf_next(g_ringbuf, VMLOGDUMP_PREFETCH)) != NULL) {
		switch (logent->tag) {
			case VMLOG_TAG_ENTER:
				if (test_enter(g_matches[logent->index])) {
					vmlog_thread_log_append(g_thread_log, logent);
					g_thread_log->seq++;
				}
				break;
			case VMLOG_TAG_LEAVE:
				if (test_exit(g_matches[logent->index])) {
					vmlog_thread_log_append(g_thread_log, logent);
					g_thread_log->seq++;
				}
				break;
			default:
				if (test_other()) {
					vmlog_thread_log_append(g_thread_log, logent);
					g_thread_log->seq++;
				}
				break;
		}
	}
}

int main(int argc,char **argv) {

	if (argc) {
		vmlog_set_progname(argv[0]);
		g_cmd = argv[0];
	}

	parse_options(argc, argv);

	g_idxfname = vmlog_concat3(opt_src_prefix,"",".idx",NULL);
	g_strfname = vmlog_concat3(opt_src_prefix,"",".str",NULL);
	g_logfname = vmlog_concat4len(
		opt_src_prefix, strlen(opt_src_prefix),
		".", 1, 
		opt_src_threadidx, strlen(opt_src_threadidx),
		".log", 4,
		NULL
	);

	g_ringbuf = vmlog_ringbuf_new(g_logfname, VMLOGDUMP_RINGBUF_SIZE);
	g_idxmap = (vmlog_string_entry *) vmlog_file_mmap(g_idxfname,&g_idxlen);
	g_nstrings = g_idxlen / sizeof(vmlog_string_entry);
	g_strmap = (char *) vmlog_file_mmap(g_strfname,&g_strlen);

	g_log = vmlog_log_new(opt_dest_prefix, 0);
	g_thread_log = vmlog_thread_log_new(
		g_log, 
		(void *)atoi(opt_dest_threadidx), 
		atoi(opt_dest_threadidx) 
	);

	match_strings();	
	do_filter();

	vmlog_thread_log_free(g_thread_log);
	vmlog_log_free(g_log);
	
	vmlog_ringbuf_free(g_ringbuf);
	g_ringbuf = NULL;
	vmlog_file_munmap(g_strmap,g_strlen);
	vmlog_file_munmap(g_idxmap,g_idxlen);

	return 0;
}

