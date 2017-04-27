/* src/vm/jit/replace.hpp - on-stack replacement of methods

   Copyright (C) 1996-2013
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


#ifndef REPLACE_HPP_
#define REPLACE_HPP_ 1

#include <stddef.h>                     // for NULL
#include "config.h"                     // for ENABLE_JIT, etc
#include "md-abi.hpp"                   // for FLT_REG_CNT, INT_SAV_CNT
#include "vm/jit/jit.hpp"               // for basicblock::Type
#include "vm/types.hpp"                 // for s4, u1, ptrint, u4, s8

struct codeinfo;
struct executionstate_t;
struct methodinfo;
struct rplalloc;
struct rplpoint;
struct sourceframe_t;
struct sourcestate_t;
union replace_val_t;

#if defined(ENABLE_REPLACEMENT)

/*** structs *********************************************************/

#define RPLALLOC_STACK  -1 // allocation info for a stack variable
#define RPLALLOC_PARAM  -2 // allocation info for a call parameter
#define RPLALLOC_SYNC   -3 // allocation info for a synchronization slot

/* `rplalloc` is a compact struct for register allocation info        */

/* XXX optimize this for space efficiency */
struct rplalloc {
	s4           index;     /* local index, -1 for stack slot         */
	s4           regoff;    /* register index / stack slot offset     */
	bool         inmemory;  /* indicates whether value is stored in memory */
	unsigned int type:4;    /* TYPE_... constant                      */
};

#if INMEMORY > 0x08
#error value of INMEMORY is too big to fit in rplalloc.flags
#endif

#if !defined(NDEBUG)
#define RPLPOINT_CHECK(type)     , rplpoint::TYPE_##type
#define RPLPOINT_CHECK_BB(bptr)  , (bptr)->type
#else
#define RPLPOINT_CHECK(type)
#define RPLPOINT_CHECK_BB(bptr)
#endif

/**
 * A point in a compiled method where it is possible to recover the source
 * state to perform on-stack replacement.
 */
struct rplpoint {
	/**
	 * CAUTION: Do not change the numerical values. These are used as
	 *          indices into replace_normalize_type_map.
	 * XXX what to do about overlapping rplpoints?
	 */
	enum Type {
		TYPE_STD     = basicblock::TYPE_STD,
		TYPE_EXH     = basicblock::TYPE_EXH,
		TYPE_SBR     = basicblock::TYPE_SBR,
		TYPE_CALL    = 3,
		TYPE_INLINE  = 4,
		TYPE_RETURN  = 5,
		TYPE_BODY    = 6
	};

	enum Flag {
		FLAG_TRAPPABLE  = 0x01,  // rplpoint can be trapped
		FLAG_COUNTDOWN  = 0x02,  // count down hits
		FLAG_DEOPTIMIZE = 0x04,  // indicates a deoptimization point
		FLAG_ACTIVE     = 0x08   // trap is active
	};

	u1          *pc;               /* machine code PC of this point    */
	methodinfo  *method;           /* source method this point is in   */
	rplpoint    *parent;           /* rplpoint of the inlined body     */ /* XXX unify with code */
	rplalloc    *regalloc;         /* pointer to register index table  */
	s4           id;               /* id of the rplpoint within method */
	s4           callsize;         /* size of call code in bytes       */
	unsigned int regalloccount:20; /* number of local allocations      */
	Type         type:4;           /* type of replacement point        */
	unsigned int flags:8;          /* OR of Flag constants             */
	u1          *patch_target_addr; /* target address to patch for invoke Static/Special */ 
};

/**
 * Represents a value of a javalocal or stack variable that has been
 * recovered from the execution state.
 */
union replace_val_t {
	s4             i;
	s8             l;
	ptrint         p;
	struct {
		u4 lo;
		u4 hi;
	}              words;
	float          f;
	double         d;
	java_object_t *a;
};

/**
 * A machine-independent representation of a method's execution state.
 */
struct sourceframe_t {
	sourceframe_t *down;           /* source frame down the call chain */

	methodinfo    *method;                  /* method this frame is in */
	s4             id;
	s4             type;

	/* values */
	replace_val_t  instance;

	replace_val_t *javastack;                  /* values of stack vars */
	u1            *javastacktype;              /*  types of stack vars */
	s4             javastackdepth;             /* number of stack vars */

	replace_val_t *javalocals;                 /* values of javalocals */
	u1            *javalocaltype;              /*  types of javalocals */
	s4             javalocalcount;             /* number of javalocals */

	replace_val_t *syncslots;
	s4             syncslotcount; /* XXX do we need more than one? */

	/* mapping info */
	rplpoint      *fromrp;         /* rplpoint used to read this frame */
	codeinfo      *fromcode;              /* code this frame was using */
	rplpoint      *torp;          /* rplpoint this frame was mapped to */
	codeinfo      *tocode;            /* code this frame was mapped to */

	/* info for native frames */
	stackframeinfo_t *sfi;      /* sfi for native frames, otherwise NULL */
	s4             nativeframesize;    /* size (bytes) of native frame */
	u1            *nativepc;
	ptrint         nativesavint[INT_SAV_CNT]; /* XXX temporary */
	double         nativesavflt[FLT_REG_CNT]; /* XXX temporary */
};

struct sourcestate_t {
	sourceframe_t *frames;    /* list of source frames, from bottom up */
};


/*** prototypes ********************************************************/

void replace_free_replacement_points(codeinfo *code);

void replace_activate_replacement_points(codeinfo *code, bool mappable);
void replace_deactivate_replacement_points(codeinfo *code);

void replace_handle_countdown_trap(u1 *pc, executionstate_t *es);
bool replace_handle_replacement_trap(u1 *pc, executionstate_t *es);
void replace_handle_deoptimization_trap(u1 *pc, executionstate_t *es);

/* machine dependent functions (code in ARCH_DIR/md.c) */

#if defined(ENABLE_JIT)
void md_patch_replacement_point(u1 *pc, u1 *savedmcode, bool revert);
#endif

namespace cacao {

class OStream;

OStream& operator<<(OStream &OS, const rplpoint *rp);
OStream& operator<<(OStream &OS, const rplpoint &rp);

OStream& operator<<(OStream &OS, const rplalloc *ra);
OStream& operator<<(OStream &OS, const rplalloc &ra);

OStream& operator<<(OStream &OS, const sourceframe_t *frame);
OStream& operator<<(OStream &OS, const sourceframe_t &frame);

OStream& operator<<(OStream &OS, const sourcestate_t *ss);
OStream& operator<<(OStream &OS, const sourcestate_t &ss);

} // end namespace cacao

#endif // ENABLE_REPLACEMENT

#endif // REPLACE_HPP_


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
