/* src/vm/package.cpp - Java boot-package functions

   Copyright (C) 2007, 2008
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

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

#include "toolbox/list.h"

#include "mm/memory.h"

#include "vm/options.h"
#include "vm/package.hpp"
#include "vm/string.hpp"
#include "vm/utf8.h"


/* internal property structure ************************************************/

typedef struct list_package_entry_t list_package_entry_t;

struct list_package_entry_t {
/* 	java_string_t *packagename; */
	utf           *packagename;
	listnode_t     linkage;
};


/* global variables ***********************************************************/

static list_t *list_package = NULL;


/**
 * Initialize the package list.
 */
void Package::initialize(void)
{
	TRACESUBSYSTEMINITIALIZATION("package_init");

	/* create the properties list */

	list_package = list_create(OFFSET(list_package_entry_t, linkage));
}


/**
 * Add a package to the boot-package list.
 *
 * @param packagename Package name as Java string.
 */
/* void package_add(java_handle_t *packagename) */
void Package::add(utf *packagename)
{
/*  	java_string_t        *s; */
	list_package_entry_t *lpe;

	/* Intern the Java string to get a unique address. */

/* 	s = javastring_intern(packagename); */

	/* Check if the package is already stored. */

	if (Package::find(packagename) != NULL)
		return;

	/* Add the package. */

#if !defined(NDEBUG)
	if (opt_DebugPackage) {
		log_start();
		log_print("[package_add: packagename=");
		utf_display_printable_ascii(packagename);
		log_print("]");
		log_finish();
	}
#endif

	lpe = NEW(list_package_entry_t);

	lpe->packagename = packagename;

	list_add_last(list_package, lpe);
}


/**
 * Find a package in the list.
 *
 * @param packagename Package name as Java string.
 *
 * @return Package name as Java string.
 */
/* java_handle_t *package_find(java_handle_t *packagename) */
utf* Package::find(utf *packagename)
{
/* 	java_string_t        *s; */
	list_t               *l;
	list_package_entry_t *lpe;

	/* Intern the Java string to get a unique address. */

/* 	s = javastring_intern(packagename); */

	/* For convenience. */

	l = list_package;

	for (lpe = (list_package_entry_t*) list_first(l); lpe != NULL; lpe = (list_package_entry_t*) list_next(l, lpe)) {
/* 		if (lpe->packagename == s) */
		if (lpe->packagename == packagename)
			return lpe->packagename;
	}

	return NULL;
}


/* Legacy C interface *********************************************************/

extern "C" {

utf* Package_find(utf *packagename) { return Package::find(packagename); }

}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:
 */
