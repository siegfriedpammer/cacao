/* src/vm/statistics.hpp - exports global varables for statistics

   Copyright (C) 1996-2005, 2006, 2007, 2008
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


#ifndef STATISTICS_HPP_
#define STATISTICS_HPP_ 1

#if defined(ENABLE_STATISTICS)

#include "config.h"

#include <stdint.h>

#include "vm/types.hpp"

#include "vm/global.hpp"

#include "toolbox/OStream.hpp"

#include <vector>

namespace cacao {
/**
 * Superclass of statistic group entries.
 */
class StatEntry {
protected:
	const char* name;
	const char* description;
public:
	/**
	 * Constructor.
	 */
	StatEntry(const char* name, const char* description) : name(name), description(description) {}
	/**
	 * Print the statistics information to an OStream.
	 * @param[in,out] O     output stream
	 */
	virtual void print(OStream &O) const = 0;
	
	virtual ~StatEntry() {}

};

/**
 * Statistics group.
 * Used to collect a set of StatEntry's
 */
class StatGroup : public StatEntry {
protected:
	typedef std::vector<StatEntry*> StatEntryList;
	/**
	 * List of members.
	 * @note: this must be a pointer. If it is changes to a normal variable strange
	 * things will happen!
	 */
	StatEntryList *members;

	StatGroup(const char* name, const char* description) : StatEntry(name, description) {
		members = new StatEntryList();
	}
public:
	/**
	 * __the__ root group.
	 * @note never use this variable directly (use StatGroup::root() instead).
	 * Used for internal purposes only!
	 */
	//static StatGroup root_rg;
	/**
	 * Get the root group
	 */
	static StatGroup& root() {
		static StatGroup rg("vm","vm");
		return rg;
	}

	/**
	 * Create a new statistics group.
	 * @param[in] name name of the group
	 * @param[in] description description of the group
	 * @param[in] group parent group.
	 */
	StatGroup(const char* name, const char* description, StatGroup &group) : StatEntry(name, description) {
		members = new StatEntryList();
		group.add(this);
	}

	~StatGroup() {
		delete members;
	}

	/**
	 * Add an entry to the group
	 */
	void add(StatEntry *re) {
		members->push_back(re);
	}

	virtual void print(OStream &O) const {
		O << setw(10) << left << name << right <<"   " << description << nl;
		//O << indent;
		for(StatEntryList::const_iterator i = members->begin(), e= members->end(); i != e; ++i) {
			StatEntry* re = *i;
			re->print(O);
		}
		//O << dedent;
	}
};

/**
 * Summary group.
 * Used to collect a set of StatEntry's
 */
class StatSumGroup : public StatGroup {
public:

	/**
	 * Create a new statistics group.
	 * @param[in] name name of the group
	 * @param[in] description description of the group
	 * @param[in] group parent group.
	 */
	StatSumGroup(const char* name, const char* description, StatGroup &group)
			: StatGroup(name,description,group) {}
	virtual void print(OStream &O) const {
		O << setw(10) << left << name << right <<"   " << description << nl;
		//O << indent;
		for(StatEntryList::const_iterator i = members->begin(), e= members->end(); i != e; ++i) {
			StatEntry* re = *i;
			re->print(O);
		}
		//O << dedent;
	}
};

/**
 * Statistics Distribution.
 */
template<typename _COUNT_TYPE, typename _INDEX_TYPE>
class StatDist : public StatEntry {
private:
	const int table_size; //< size of the table
	_COUNT_TYPE  *table;  //< table
	_INDEX_TYPE start;
	_INDEX_TYPE end;
	_INDEX_TYPE step;
public:
	/**
	 * Create a new statistics distribution.
	 * @param[in] name name of the variable
	 * @param[in] description description of the variable
	 * @param[in] parent parent group.
	 * @param[in] start first entry in the distribution table
	 * @param[in] end last entry in the distribution table
	 * @param[in] init initial value of the distribution table entries
	 */
	StatDist(const char* name, const char* description, StatGroup &parent,
			_INDEX_TYPE start, _INDEX_TYPE end, _INDEX_TYPE step, _COUNT_TYPE init)
			: StatEntry(name, description), table_size((end - start)/step + 1),
			start(start), end(end), step(step) {
		assert(step);
		parent.add(this);
		table = new _COUNT_TYPE[table_size + 1];

		for(int i = 0; i <= table_size; ++i ) {
			table[i] = init;
		}
	}

	virtual ~StatDist() {
		delete table;
	}

	virtual void print(OStream &O) const {
		/*O
		  << setw(30) << name
		  //<< setw(10) << var
		  << " : " << description << nl;
		*/
		O << description << '(' << name << "):" << nl;

		O << ' ';
		for(int i = 0; i < table_size; ++i ) {
			O << setw(5) << start + i * step << ']';
		}
		O << '(' << setw(4) << start + (table_size-1)*step << nl;
		for(int i = 0; i < table_size + 1; ++i ) {
			O << setw(6) << table[i];
		}
		O << nl;
	}
	/// index operator
	inline _COUNT_TYPE& operator[](const _INDEX_TYPE i) {
		if (i <= start) {
			return table[0];
		}
		if (i > end) {
			return table[table_size];
		}
		return table[( (i-start)%step == 0 ? (i-start)/step :  (i-start+step)/step )];
	}
};

/**
 * Statistics Variable.
 */
template<typename _T, _T init>
class StatVar : public StatEntry {
private:
	_T  var;    //< variable
public:
	/**
	 * Create a new statistics variable.
	 * @param[in] name name of the variable
	 * @param[in] description description of the variable
	 * @param[in] group parent group.
	 */
	StatVar(const char* name, const char* description, StatGroup &parent)
			: StatEntry(name, description), var(init) {
		parent.add(this);
	}

	void print(OStream &O) const {
		O
		  << setw(30) << name
		  << setw(10) << var
		  << " : " << description << nl;
	}
	/// maximum
	inline StatVar& max(const _T& i) {
	  if (i > var)
		var = i;
	  return *this;
	}

	/// mininum
	inline StatVar& min(const _T& i) {
	  if (i < var)
		var = i;
	  return *this;
	}

	/// prefix operator
	inline StatVar& operator++() {
	  ++var;
	  return *this;
	}
	/// postfix operator
	inline StatVar operator++(int) {
	  StatVar copy(*this);
	  ++var;
	  return copy;
	}
	/// increment operator
	inline StatVar& operator+=(const _T& i) {
	  var+=i;
	  return *this;
	}
	/// prefix operator
	inline StatVar& operator--() {
	  --var;
	  return *this;
	}
	/// postfix operator
	inline StatVar operator--(int) {
	  StatVar copy(*this);
	  --var;
	  return copy;
	}
	/// decrement operator
	inline StatVar& operator-=(const _T& i) {
	  var-=i;
	  return *this;
	}
};

} // end namespace cacao

/* statistic macros ***********************************************************/

#if 0
#define STAT_REGISTER_VAR(var,name,description)                                \
	static inline cacao::StatVar& var() {	                                   \
		static cacao::StatVar var(name,description,cacao::StatGroup::root());  \
		return var;                                                            \
	}
#define STAT_REGISTER_GROUP_VAR(var,name,description,group)                    \
	static inline cacao::StatVar& var() {	                                   \
		static cacao::StatVar var(name,description,group());                   \
		return var;                                                            \
	}
#endif
#define STAT_REGISTER_VAR(type, var, init, name, description) \
	static cacao::StatVar<type,init> var(name,description,cacao::StatGroup::root());

#define STAT_REGISTER_GROUP_VAR(type, var, init, name, description, group) \
	static cacao::StatVar<type,init> var(name,description,group());

#define STAT_REGISTER_DIST(counttype,indextype, var, start, end, step, init, name, description) \
	static cacao::StatDist<counttype,indextype> var(name,description,cacao::StatGroup::root(),start,end,step,init);

#define STAT_REGISTER_GROUP(var,name,description)                              \
	inline cacao::StatGroup& var##_group() {                                   \
		static cacao::StatGroup var(name,description,cacao::StatGroup::root());\
		return var;                                                            \
	}


#define STAT_REGISTER_SUBGROUP(var,name,description,group)                     \
	inline cacao::StatGroup& var##_group() {                                   \
		static cacao::StatGroup var(name,description, group##_group());        \
		return var;                                                            \
	}

#define STATISTICS(x) \
    do { \
        if (opt_stat) { \
            x; \
        } \
    } while (0)
#else
#define STATISTICS(x)    /* nothing */
#endif

#ifdef ENABLE_STATISTICS

/* in_  inline statistics */

#define IN_MAX                  9
#define IN_UNIQUEVIRT           0x0000
#define IN_UNIQUE_INTERFACE     0x0001
#define IN_OUTSIDERS            0x0004
#define IN_MAXDEPTH             0x0008
#define IN_MAXCODE              0x0010
#define IN_JCODELENGTH          0x0020
#define IN_EXCEPTION            0x0040
#define IN_NOT_UNIQUE_VIRT      0x0080
#define IN_NOT_UNIQUE_INTERFACE 0x0100

#define N_UNIQUEVIRT            0
#define N_UNIQUE_INTERFACE      1
#define N_OUTSIDERS             2
#define N_MAXDEPTH		3
#define N_MAXCODE               4
#define N_JCODELENGTH           5
#define N_EXCEPTION            6
#define N_NOT_UNIQUE_VIRT       7
#define N_NOT_UNIQUE_INTERFACE  8


/* global variables ***********************************************************/

extern s4 codememusage;
extern s4 maxcodememusage;

extern s4 memoryusage;
extern s4 maxmemusage;

extern s4 maxdumpsize;

extern s4 globalallocateddumpsize;
extern s4 globaluseddumpsize;


/* variables for measurements *************************************************/

extern s4 size_classinfo;
extern s4 size_fieldinfo;
extern s4 size_methodinfo;
extern s4 size_lineinfo;
extern s4 size_codeinfo;

extern s4 size_stub_native;

extern s4 size_stack_map;
extern s4 size_string;

extern s4 size_threadobject;
extern int32_t size_thread_index_t;
extern int32_t size_stacksize;

extern s4 size_lock_record;
extern s4 size_lock_hashtable;
extern s4 size_lock_waiter;

extern int32_t count_linenumbertable;
extern int32_t size_linenumbertable;

extern s4 size_patchref;

extern u8 count_calls_java_to_native;
extern u8 count_calls_native_to_java;

extern int count_const_pool_len;
extern int count_classref_len;
extern int count_parsed_desc_len;
extern int count_vftbl_len;
extern int count_all_methods;
extern int count_methods_marked_used;  /*RTA*/
extern int count_vmcode_len;
extern int count_extable_len;
extern int count_class_loads;
extern int count_class_inits;

extern int count_utf_len;               /* size of utf hash                   */
extern int count_utf_new;
extern int count_utf_new_found;

extern int count_locals_conflicts;
extern int count_locals_spilled;
extern int count_locals_register;
extern int count_ss_spilled;
extern int count_ss_register;
extern int count_methods_allocated_by_lsra;
extern int count_mem_move_bb;
extern int count_interface_size;
extern int count_argument_mem_ss;
extern int count_argument_reg_ss;
extern int count_method_in_register;
extern int count_mov_reg_reg;
extern int count_mov_mem_reg;
extern int count_mov_reg_mem;
extern int count_mov_mem_mem;

extern int count_jit_calls;
extern int count_methods;
extern int count_spills_read_ila;
extern int count_spills_read_flt;
extern int count_spills_read_dbl;
extern int count_spills_write_ila;
extern int count_spills_write_flt;
extern int count_spills_write_dbl;
extern int count_pcmd_activ;
extern int count_pcmd_drop;
extern int count_pcmd_zero;
extern int count_pcmd_const_store;
extern int count_pcmd_const_alu;
extern int count_pcmd_const_bra;
extern int count_pcmd_load;
extern int count_pcmd_move;
extern int count_load_instruction;
extern int count_pcmd_store;
extern int count_pcmd_store_comb;
extern int count_dup_instruction;
extern int count_pcmd_op;
extern int count_pcmd_mem;
extern int count_pcmd_met;
extern int count_pcmd_bra;
extern int count_pcmd_table;
extern int count_pcmd_return;
extern int count_pcmd_returnx;
extern int count_check_null;
extern int count_check_bound;
extern int count_max_basic_blocks;
extern int count_basic_blocks;
extern int count_max_javainstr;
extern int count_javainstr;
extern int count_javacodesize;
extern int count_javaexcsize;
extern int count_calls;
extern int count_tryblocks;
extern int count_code_len;
extern int count_data_len;
extern int count_cstub_len;
extern int count_max_new_stack;
extern int count_upper_bound_new_stack;

extern int count_emit_branch;
extern int count_emit_branch_8bit;
extern int count_emit_branch_16bit;
extern int count_emit_branch_32bit;
extern int count_emit_branch_64bit;

extern s4 count_branches_resolved;
extern s4 count_branches_unresolved;

extern int *count_block_stack;
extern int *count_analyse_iterations;
extern int *count_method_bb_distribution;
extern int *count_block_size_distribution;
extern int *count_store_length;
extern int *count_store_depth;
                                /* in_  inline statistics */
extern int count_in;
extern int count_in_uniqVirt;
extern int count_in_uniqIntf;
extern int count_in_rejected;
extern int count_in_rejected_mult;
extern int count_in_outsiders;
extern int count_in_uniqueVirt_not_inlined;
extern int count_in_uniqueInterface_not_inlined;
extern int count_in_maxDepth;
extern int count_in_maxMethods;

extern u2 count_in_not   [512];

/* instruction scheduler statistics *******************************************/

extern s4 count_schedule_basic_blocks;
extern s4 count_schedule_nodes;
extern s4 count_schedule_leaders;
extern s4 count_schedule_max_leaders;
extern s4 count_schedule_critical_path;

/* jni statistics *************************************************************/

extern u8 count_jni_callXmethod_calls;
extern u8 count_jni_calls;

/* function prototypes ********************************************************/

s8 getcputime(void);

void loadingtime_start(void);
void loadingtime_stop(void);
void compilingtime_start(void);
void compilingtime_stop(void);

void print_times(void);
void print_stats(void);

void statistics_print_date(void);
void statistics_print_memory_usage(void);
void statistics_print_gc_memory_usage(void);

void mem_usagelog(bool givewarnings);

void compiledinvokation(void);

#endif /* ENABLE_STATISTICS */

#endif // STATISTICS_HPP_


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
