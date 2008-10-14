/* src/vm/jit/jitcache.h - jit compiler output caching

   Copyright (C) 2008
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

#ifndef _JITCACHE_H
#define _JITCACHE_H

#include "vm/jit/patcher-common.hpp"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ENABLE_JITCACHE)

#include "config.h"

#include <stdint.h>

#include "vm/class.hpp"
#include "vm/method.h"

typedef enum cachedreftype {
	CRT_CODEINFO,
	CRT_NUM,
	CRT_ENTRYPOINT,
	CRT_CODEGEN_FINISH_NATIVE_CALL,
	CRT_ASM_HANDLE_EXCEPTION, /* 4 */
	CRT_ASM_HANDLE_NAT_EXCEPTION,
	CRT_OBJECT_HEADER,
	CRT_BUILTIN, /* 7 */
	CRT_BUILTIN_FP,
	CRT_STRING,
	CRT_CLASSINFO, /* 10 */
	CRT_CLASSINFO_INDEX,
	CRT_CLASSINFO_INTERFACETABLE,
	CRT_CLASSINFO_VFTBL,
	CRT_METHODINFO_STUBROUTINE, /* 14 */
	CRT_METHODINFO_TABLE,
	CRT_METHODINFO_INTERFACETABLE,
	CRT_METHODINFO_METHODOFFSET,
	CRT_FIELDINFO_VALUE, /* 18 */
	CRT_FIELDINFO_OFFSET,
	CRT_FIELDINFO_OFFSET_HIGH,
	CRT_JUMPREFERENCE /* 21 */
} cachedreftype;

/* cachedref_t *****************************************************************

   A cached reference contains information about a code or data position
   which needs patching after restoring the it from disk.

*******************************************************************************/

struct cachedref_t {
	cachedreftype type;			/* type of the cached reference */
	s4			  md_patch;		/* machine dependent back patching */
	s4       	  disp;         /* displacement of ref in the data segment    */
	void*       ref;          /* reference passed                           */
};

/*
typedef struct mru_entry_t {
	classinfo *clazz;
	mutex_t  lock;
}
*/

/* typedefs *******************************************************************/

typedef void (*serializerfptr) (int, patchref_t *, methodinfo *);
typedef void (*deserializerfptr) (patchref_t *, int, methodinfo *);

/* jitcache_patcher_function_list_t typedef ***********************************/

struct jitcache_patcher_function_list_t {
	functionptr patcher;
	serializerfptr serializer;
	deserializerfptr deserializer;
};

/* function prototypes ********************************************************/

void	jitcache_list_create(codeinfo *code);

void	jitcache_list_reset(codeinfo *code);

void	jitcache_list_free(codeinfo *code);

void	jitcache_add_cached_ref_jd(jitdata *jd, cachedreftype type, void* ref);

void	jitcache_add_cached_ref_md_jd(jitdata *jd, cachedreftype type, s4 md_patch, void* ref);

void	jitcache_add_cached_ref(codeinfo *code, cachedreftype type, void* ref, s4 disp);

void    jitcache_store(methodinfo *m);

u1      jitcache_load(methodinfo *m);

void	jitcache_handle_cached_ref(cachedref_t *cr, codeinfo *code);

void  jitcache_quit();

void	jitcache_freeclass(classinfo *);

#define JITCACHE_ADD_CACHED_REF_JD(jd, type, ref) \
	(jitcache_add_cached_ref_jd(jd, type, (void*) ref))

#define JITCACHE_ADD_CACHED_REF_JD_COND(jd, type, ref, COND) \
	if (COND) \
	(jitcache_add_cached_ref_jd(jd, type, (void*) ref))

#define JITCACHE_ADD_CACHED_REF_MD_JD(jd, type, md_patch, ref) \
	(jitcache_add_cached_ref_md_jd(jd, type, md_patch, (void*) ref))

#define JITCACHE_ADD_CACHED_REF(code, type, ref, disp) \
	(jitcache_add_cached_ref(code, type, (void*) ref, disp))

#define JITCACHE_ADD_CACHED_REF_COND(code, type, ref, disp, COND) \
	if (COND) \
		jitcache_add_cached_ref(code, type, (void*) ref, disp)

#else

#define JITCACHE_ADD_CACHED_REF_JD(jd, type, ref) \
	while (0) { }

#define JITCACHE_ADD_CACHED_REF_JD_COND(jd, type, ref, COND) \
	while (0) { }

#define JITCACHE_ADD_CACHED_REF_MD_JD(jd, type, md_patch, ref) \
	while (0) { }

#define JITCACHE_ADD_CACHED_REF(code, type, ref, disp) \
	while (0) { }

#define JITCACHE_ADD_CACHED_REF_COND(code, type, ref, disp, COND) \
	while (0) { }

#endif /* ENABLE_JITCACHE */

#ifdef __cplusplus
}
#endif


#endif /* _JITCACHE_HPP */


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
