#ifndef __mm_h_
#define __mm_h_

#include "../sysdep/types.h"
#include "../global.h"

#ifndef CACAO_NO_INLINE
#define __cacao_inline__  inline
#else
#define __cacao_inline__
#endif

#ifdef __GNUC__
#define __inline__  __cacao_inline__
#else
#define __inline__
#endif


#endif


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
 */
