/* src/vm/jit/compiler2/CodeMemory.hpp - CodeMemory

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

#ifndef _JIT_COMPILER2_CODEMEMORY
#define _JIT_COMPILER2_CODEMEMORY

#include "vm/jit/compiler2/CodeSegment.hpp"
#include "vm/jit/compiler2/DataSegment.hpp"
#include "vm/types.hpp"

#include <map>
#include <vector>
#include <cassert>

namespace cacao {
namespace jit {
namespace compiler2 {

// forward declarations
class BeginInst;
class MachineInstruction;
class CodeMemory;

#if 0
/**
 * Code fragments
 *
 * Code fregments are used to write instructions to the CodeMemory. It abstracts
 * the fact that code memory is written upside down and achitecure dependent
 * properties such as endianess.
 *
 * It can be used just like an u1 array.
 */
class CodeFragment {
private:
	CodeMemory *parent;
	u1 *code_ptr;
	unsigned _size;
	CodeFragment(CodeMemory* parent, u1 *code_ptr,unsigned _size)
		: parent(parent), code_ptr(code_ptr), _size(_size) {}
public:
	// Copy Constructor
	CodeFragment(const CodeFragment &other) : parent(other.parent),
		code_ptr(other.code_ptr), _size(other.size()) {}
	// Copy assignment operator
	CodeFragment& operator=(const CodeFragment &other) {
		parent = other.parent;
		code_ptr = other.code_ptr;
		_size = other.size();
		return *this;
	}
	CodeFragment operator+(unsigned v) {
		assert( signed(_size) - signed(v) >= 0);
		CodeFragment CF(parent, code_ptr + v, _size - v);
		return CF;
	}
	u1& operator[](unsigned i) {
		assert(i < size());
		return *(code_ptr + i);
	}
	CodeMemory* get_CodeMemory() const {
		return parent;
	}
	unsigned size() const {
		return _size;
	}
	friend class CodeMemory;
};
#endif

/**
 * CodeMemory
 */
class CodeMemory {
private:
	typedef std::vector<u1> DataSegTy;
public:
	typedef DataSegTy::const_iterator const_data_iterator;
private:
	typedef std::map<const BeginInst*,u1*> LabelMapTy;
	typedef std::pair<const MachineInstruction*,CodeFragment> ResolvePointTy;
	typedef std::multimap<const BeginInst*,ResolvePointTy> ResolveLaterMapTy;
	typedef std::multimap<std::size_t,ResolvePointTy> ResolveDataMapTy;
	typedef std::list<ResolvePointTy> LinkListTy;

	u1             *mcodebase;      ///< base pointer of code area
	u1             *mcodeend;       ///< pointer to end of code area
	s4              mcodesize;      ///< complete size of code area (bytes)
	u1             *mcodeptr;       ///< code generation pointer

	DataSegTy dataseg;              ///< data segment

	LabelMapTy label_map;           ///< label map
	ResolveLaterMapTy resolve_map;  ///< jumps to be resolved later
	ResolveDataMapTy  resolve_data_map; ///< date segment

	LinkListTy linklist;            ///< instructions that require linking

	CodeSegment cseg;               ///< code segment
	DataSegment dseg;               ///< data segment

	#if 0
	/**
	 * get the offset from the current position to the label.
	 *
	 * @return offset from the current position or INVALID_OFFSET if label
	 * not found.
	 */
	s4 get_offset(const BeginInst *BI, u1 *current_pos) const;
	/**
	 * get the offset from the current position to the data segment index.
	 *
	 * @return offset from the current positioni
	 */
	s4 get_offset(std::size_t index, u1 *current_pos) const;
	#endif
	/**
	 */
	template<typename T,std::size_t size>
	std::size_t get_index(T t, u1 *current_pos) {
		static std::map<T,std::size_t> map;
		typename std::map<T,std::size_t>::iterator i = map.find(t);

		if (i != map.end()) {
			return i->second;
		}
		std::size_t index = dataseg.size();
		dataseg.resize(index + size);
		return index;
	}
public:
	/**
	 * indicate an invalid offset
	 */
	static const s4 INVALID_OFFSET = numeric_limits<s4>::max;

	CodeMemory();

	/// get CodeSegment
	const CodeSegment& get_CodeSegment() const { return cseg; }
	/// get CodeSegment
	CodeSegment& get_CodeSegment()             { return cseg; }
	/// get DataSegment
	const DataSegment& get_DataSegment() const { return dseg; }
	/// get DataSegment
	DataSegment& get_DataSegment()             { return dseg; }
	/**
	 * add a label to the current position.
	 */
	void add_label(const BeginInst *BI);

	s4 get_offset(CodeSegment::IdxTy to, CodeSegment::IdxTy from) const;
	s4 get_offset(CodeSegment::IdxTy to) const {
		return get_offset(to, cseg.end());
	}
	/**
	 * @deprecated this should be moved to FixedCodeMemory or so
	 */
	s4 get_offset(DataSegment::IdxTy to, CodeSegment::IdxTy from) const;
	#if 0
	/**
	 * get the offset from the current position to the label.
	 *
	 * @return offset from the current position or INVALID_OFFSET if label
	 * not found.
	 */
	s4 get_offset(const BeginInst *BI) const {
		return get_offset(BI,mcodeptr);
	}
	/**
	 * get the offset from the current position to the label.
	 *
	 * @return offset from the current position or INVALID_OFFSET if label
	 * not found.
	 */
	s4 get_offset(const BeginInst *BI, const CodeFragment &CF) const {
		u1 *current_pos = CF.code_ptr + CF.size();
		return get_offset(BI,current_pos);
	}
	/**
	 * get the offset from the current position to the data segment index.
	 *
	 * @return offset from the current positioni
	 */
	s4 get_offset(std::size_t index, const CodeFragment &CF) const { 
		u1 *current_pos = CF.code_ptr + CF.size();
		return get_offset(index,current_pos);
	}

	const_data_iterator data_begin() const {
		return dataseg.begin();
	}
	const_data_iterator data_end() const {
		return dataseg.end();
	}

	/**
	 * get data segment size (in bytes)
	 */
	std::size_t data_size() const {
		return dataseg.size();
	}
	#endif
	void require_linking(const MachineInstruction*, CodeFragment CF);

	/**
	 * add unresolved jump
	 */
	void resolve_later(const BeginInst *BI, const MachineInstruction*,
		CodeFragment CM);
	/**
	 * resolve jump
	 */
	void resolve();

	/**
	 * resolve data
	 */
	void resolve_data();

	/**
	 * get a code fragment
	 */
	CodeFragment get_CodeFragment(unsigned size);

	/**
	 * get an aligned code fragment
	 *
	 * @return A CodeFragment aligend to Target::alignment.
	 */
	CodeFragment get_aligned_CodeFragment(unsigned size);

	/**
	 * get the start address of the code memory
	 */
	u1* get_start() const { return mcodeptr; }
	/**
	 * get the end address of the code memory
	 */
	u1* get_end() const { return mcodeend; }

	u4 size() const { return get_end() - get_start(); }

	//friend class CodeFragment;
};


} // end namespace compiler2
} // end namespace jit
} // end namespace cacao

#endif /* _JIT_COMPILER2_CODEMEMORY */


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
