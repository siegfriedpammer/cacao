/* src/vm/jit/compiler2/DataSegment.hpp - DataSegment

   Copyright (C) 2013
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

#ifndef _JIT_COMPILER2_DATASEGMENT
#define _JIT_COMPILER2_DATASEGMENT

#include "vm/jit/compiler2/Segment.hpp"

// forward declaration
struct constant_FMIref;

namespace cacao {
namespace jit {
namespace compiler2 {

enum DataSegmentType {
	DoubleID,
	LongID,
	FloatID,
	IntID,
	FMIRefID
};

typedef ConstTag<DataSegmentType,double,DoubleID> DSDouble;
typedef ConstTag<DataSegmentType,int64_t,LongID> DSLong;
typedef ConstTag<DataSegmentType,float,FloatID> DSFloat;
typedef ConstTag<DataSegmentType,int32_t,IntID> DSInt;
typedef PointerTag<DataSegmentType,constant_FMIref,FMIRefID> DSFMIRef;

typedef Segment<DataSegmentType,NormalRefCategory> DataSegment;
typedef SegRef<DataSegmentType,NormalRefCategory> DataFragment;


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_DATASEGMENT */


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
