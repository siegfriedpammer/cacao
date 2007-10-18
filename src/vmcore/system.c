/* src/vm/system.c - system (OS) functions

   Copyright (C) 2007
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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

*/


#include "config.h"

#include <stdint.h>
#include <unistd.h>

#if defined(__DARWIN__)
# include <mach/mach.h>
# include <mach/mach_host.h>
# include <mach/host_info.h>
#endif

/* this should work on BSD */
/* #include <sys/sysctl.h> */


/* system_processors_online ****************************************************

   Returns the number of online processors in the system.

   RETURN:
       online processor count

*******************************************************************************/

int system_processors_online(void)
{
#if defined(_SC_NPROC_ONLN)

	return (int) sysconf(_SC_NPROC_ONLN);

#elif defined(_SC_NPROCESSORS_ONLN)

	return (int) sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(__DARWIN__)

	host_basic_info_data_t hinfo;
	mach_msg_type_number_t hinfo_count = HOST_BASIC_INFO_COUNT;
	kern_return_t rc;

	rc = host_info(mach_host_self(), HOST_BASIC_INFO,
				   (host_info_t) &hinfo, &hinfo_count);
 
	if (rc != KERN_SUCCESS) {
		return -1;
	}

	/* XXX michi: according to my infos this should be
	   hinfo.max_cpus, can someone please confirm or deny that? */
	return (int) hinfo.avail_cpus;

#elif defined(__FREEBSD__)
# error IMPLEMENT ME!

	/* this should work in BSD */
	/*
	int ncpu, mib[2], rc;
	size_t len;

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ncpu);
	rc = sysctl(mib, 2, &ncpu, &len, NULL, 0);

	return (int32_t) ncpu;
	*/

#else

	return 1;

#endif
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
