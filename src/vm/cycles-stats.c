/* src/vm/cycles-stats.c - functions for cycle count statistics

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

#if defined(ENABLE_CYCLES_STATS)

#include <stdio.h>
#include <assert.h>
#include "vm/cycles-stats.h"

struct cycles_stats_percentile {
	int         pct;
	const char *name;
};

static struct cycles_stats_percentile cycles_stats_percentile_defs[] = {
	{ 10, "10%-percentile" },
	{ 50, "median"         },
	{ 90, "90%-percentile" },
	{ 99, "99%-percentile" },
	{  0, NULL             } /* sentinel */
};

void cycles_stats_print(FILE *file,
						const char *name, int nbins, int div,
						u4 *bins, u4 count, u8 min, u8 max)
{
        s4 i;
		struct cycles_stats_percentile *pcd;
		u4 floor, ceiling;
		u8 p;
		u8 cumul;
		double percentile;

        fprintf(file,"\t%s: %u calls\n",
                name, count);
		
        fprintf(file,"\t%s cycles distribution:\n", name);
		
        fprintf(file,"\t\t%20s = %llu\n", "min", (unsigned long long)min);

		pcd = cycles_stats_percentile_defs;
		for (; pcd->name; pcd++) {
			floor   = (count * pcd->pct) / 100;
			ceiling = (count * pcd->pct + 99) / 100;
			cumul = 0;
			p = 0;
			percentile = -1.0;

			assert( ceiling <= floor + 1 );

			for (i=0; i<nbins; ++i) {

				/* { invariant: `cumul` samples are < `p` } */

				/* check if percentile lies exactly at the bin boundary */
				
				if (floor == cumul && floor == ceiling) {
					percentile = p;
					break;
				}

				/* check if percentile lies within this bin */

				if (cumul <= floor && ceiling <= cumul + bins[i]) {
					percentile = p + (double)div/2.0;
					break;
				}
				
				cumul += bins[i];
				p     += div;

				/* { invariant: `cumul` samples are < `p` } */
			}
			
			/* check if percentile lies exactly at the bin boundary */

			if (floor == cumul && floor == ceiling) {
				percentile = p;
			}

			if (percentile >= 0) {
				fprintf(file,"\t\t%20s = %.1f\n", pcd->name, percentile);
			}
			else {
				fprintf(file,"\t\t%20s = unknown (> %llu)\n", pcd->name, (unsigned long long)p);
			}
		}
		
        fprintf(file,"\t\t%20s = %llu\n", "max", (unsigned long long)max);
		
		cumul = 0;
        for (i=0; i<nbins; ++i) {
			cumul += bins[i];
            fprintf(file,"\t\t<  %5d: %10lu (%3d%%) %10lu\n",
                    (int)((i+1) * div),
					(unsigned long) cumul,
					(int)((cumul * 100) / count),
                    (unsigned long) bins[i]);
        }
		
        fprintf(file,"\t\t>= %5d: %10s (----) %10lu\n",
                (int)(nbins * div),
				"OVER",
                (unsigned long) bins[nbins]);
}

#endif /* defined(ENABLE_CYCLES_STATS) */

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
