/* src/vm/rt-timing.hpp - POSIX real-time timing utilities

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

/** @file
 * This file contain the real-time timing utilities.
 *
 * For day to day use only the following macros are of importance:
 * - RT_REGISTER_TIMER(var,name,description)
 * - RT_REGISTER_GROUP_TIMER(var,name,description,group)
 * - RT_REGISTER_GROUP(var,name,description)
 * - RT_REGISTER_SUBGROUP(var,name,description,group)
 * - RT_TIMER_START(var)
 * - RT_TIMER_STOP(var)
 * - RT_TIMER_STOPSTART(var1,var2)
 *
 * A typical usage would look something like this: *
 * @code
 * // NOTE: toplevel, outside a function!
 * RT_REGISTER_GROUP(my_group,"my-group","this is my very own timing group");
 * RT_REGISTER_GROUP_TIMER(my_timer1,"my-timer1","this is my first timer",my_group);
 * RT_REGISTER_GROUP_TIMER(my_timer2,"my-timer1","this is my second timer",my_group);
 *
 * void do_something() {
 *    RT_TIMER_START(my_timer1);
 *    // do some work
 *    RT_TIMER_STOPSTART(my_timer1,my_timer2);
 *    // do even more work
 *    RT_TIMER_STOP(my_timer2);
 * }
 * @endcode
 *
 * @note
 * The implementation of the RT_REGISTER_* macros is a bit tricky. In general
 * we simply want to create global variables for groups and timers. Unfortunately
 * RTGroup and RTTimer are no "plain old datatypes" (pod). There is a constructor
 * which is dependant on other global non-pod variables. The C++ standard does not
 * guarantee any specific order of construction so it might be the case that
 * a subgroup is registered before its super group. There are many dirty tricks to
 * work around this issue. The one we are using does the following. We create a global
 * variable as well as a function which returns this variable. The C++ standard
 * guarantees that a global variable is initialized before its first use. Therefore,
 * if we access the group/timer variables only through these functions it is
 * guaranteed that everything is initialized in the right order.
 * Also note that the variable must be global (i.e. they can not be declared as
 * static function variables) otherwise the objects might be destroyed eagerly.
 */

/**
 * @def RT_REGISTER_TIMER(var,name,description)
 * Register a new (toplevel) timer. Create a timer and add it to the toplevel
 * timing group RTGroup::root().
 *
 * @note This macro creates a new global variable with the name var_rt and
 * a function RTTimer& var(). Consider using namespaces to avoid name clashes.
 */

/**
 * @def RT_REGISTER_GROUP_TIMER(var,name,description,group)
 * Register a new timer. Create a timer and add it to group specified.
 *
 * @note This macro creates a new global variable with the name var_rt and
 * a function RTTimer& var(). Consider using namespaces to avoid name clashes.
 */

/**
 * @def RT_REGISTER_GROUP(var,name,description)
 * Register a new (toplevel) group. Create a group and add it to the toplevel
 * timing group RTGroup::root().
 *
 * @note This macro creates a new global variable with the name var_rg and
 * a function RTGroup& var(). Consider using namespaces to avoid name clashes.
 */

/**
 * @def RT_REGISTER_SUBGROUP(var,name,description,group)
 * Register a new subgroup. Create a group and add it to group specified.
 *
 * @note This macro creates a new global variable with the name var_rg and
 * a function RTGroup& var(). Consider using namespaces to avoid name clashes.
 */

/**
 * @def RT_TIMER_START(var)
 * Start the timer var.
 */

/**
 * @def RT_TIMER_STOP(var)
 * Stop the timer var.
 */

/**
 * @def RT_TIMER_STOPSTART(var1,var2)
 * Stop the timer var1 and start the timer var2
 */
#ifndef _RT_TIMING_HPP
#define _RT_TIMING_HPP

#include "config.h"

#if defined(ENABLE_RT_TIMING)

#include <time.h>

#include "vm/types.h"

#include "mm/memory.hpp"

#include "vm/global.h"


#define RT_TIMING_GET_TIME(ts) \
	rt_timing_gettime(&(ts));

#define RT_TIMING_TIME_DIFF(a,b,index) \
	rt_timing_time_diff(&(a),&(b),(index));

#define RT_TIMING_JIT_CHECKS       0
#define RT_TIMING_JIT_PARSE        1
#define RT_TIMING_JIT_STACK        2
#define RT_TIMING_JIT_TYPECHECK    3
#define RT_TIMING_JIT_LOOP         4
#define RT_TIMING_JIT_IFCONV       5
#define RT_TIMING_JIT_ALLOC        6
#define RT_TIMING_JIT_RPLPOINTS    7
#define RT_TIMING_JIT_CODEGEN      8
#define RT_TIMING_JIT_TOTAL        9

#define RT_TIMING_LINK_RESOLVE     10
#define RT_TIMING_LINK_C_VFTBL     11
#define RT_TIMING_LINK_ABSTRACT    12
#define RT_TIMING_LINK_C_IFTBL     13
#define RT_TIMING_LINK_F_VFTBL     14
#define RT_TIMING_LINK_OFFSETS     15
#define RT_TIMING_LINK_F_IFTBL     16
#define RT_TIMING_LINK_FINALIZER   17
#define RT_TIMING_LINK_EXCEPTS     18
#define RT_TIMING_LINK_SUBCLASS    19
#define RT_TIMING_LINK_TOTAL       20

#define RT_TIMING_LOAD_CHECKS      21
#define RT_TIMING_LOAD_NDPOOL      22
#define RT_TIMING_LOAD_CPOOL       23
#define RT_TIMING_LOAD_SETUP       24
#define RT_TIMING_LOAD_FIELDS      25
#define RT_TIMING_LOAD_METHODS     26
#define RT_TIMING_LOAD_CLASSREFS   27
#define RT_TIMING_LOAD_DESCS       28
#define RT_TIMING_LOAD_SETREFS     29
#define RT_TIMING_LOAD_PARSEFDS    30
#define RT_TIMING_LOAD_PARSEMDS    31
#define RT_TIMING_LOAD_PARSECP     32
#define RT_TIMING_LOAD_VERIFY      33
#define RT_TIMING_LOAD_ATTRS       34
#define RT_TIMING_LOAD_TOTAL       35

#define RT_TIMING_LOAD_BOOT_LOOKUP 36
#define RT_TIMING_LOAD_BOOT_ARRAY  37
#define RT_TIMING_LOAD_BOOT_SUCK   38
#define RT_TIMING_LOAD_BOOT_LOAD   39
#define RT_TIMING_LOAD_BOOT_CACHE  40
#define RT_TIMING_LOAD_BOOT_TOTAL  41

#define RT_TIMING_LOAD_CL_LOOKUP   42
#define RT_TIMING_LOAD_CL_PREPARE  43
#define RT_TIMING_LOAD_CL_JAVA     44
#define RT_TIMING_LOAD_CL_CACHE    45

#define RT_TIMING_NEW_OBJECT       46
#define RT_TIMING_NEW_ARRAY        47

#define RT_TIMING_GC_ALLOC         48
#define RT_TIMING_GC_SUSPEND       49
#define RT_TIMING_GC_ROOTSET1      50
#define RT_TIMING_GC_MARK          51
#define RT_TIMING_GC_COMPACT       52
#define RT_TIMING_GC_ROOTSET2      53
#define RT_TIMING_GC_TOTAL         54

#define RT_TIMING_REPLACE          55

#define RT_TIMING_1                56
#define RT_TIMING_2                57
#define RT_TIMING_3                58
#define RT_TIMING_4                59

#define RT_TIMING_N                60

#ifdef __cplusplus
extern "C" {
#endif

void rt_timing_gettime(struct timespec *ts);

void rt_timing_time_diff(struct timespec *a,struct timespec *b,int index);

long rt_timing_diff_usec(struct timespec *a,struct timespec *b);

void rt_timing_print_time_stats(FILE *file);

#ifdef __cplusplus
}
#endif

#include "toolbox/OStream.hpp"

namespace {
/**
 * @note: http://www.gnu.org/software/libc/manual/html_node/Elapsed-Time.html
 */
inline timespec operator-(const timespec a, timespec b) {
	timespec result;
	/* Perform the carry for the later subtraction by updating b. */
	if (a.tv_nsec < b.tv_nsec) {
		int xsec = (b.tv_nsec - a.tv_nsec) / 1000000000L + 1;
		b.tv_nsec -= 1000000000L * xsec;
		b.tv_sec += xsec;
	}
	if (a.tv_nsec - b.tv_nsec > 1000000000L) {
		int xsec = (a.tv_nsec - b.tv_nsec) / 1000000000L;
		b.tv_nsec += 1000000000L * xsec;
		b.tv_sec -= xsec;
	}

	/* Compute the time remaining to wait. tv_usec is certainly positive. */
	result.tv_sec = a.tv_sec - b.tv_sec;
	result.tv_nsec = a.tv_nsec - b.tv_nsec;

	return result;
}

inline timespec operator+(const timespec a, const timespec b) {
	timespec result;
	result.tv_sec = a.tv_sec + b.tv_sec;
	result.tv_nsec = a.tv_nsec + b.tv_nsec;
	if (result.tv_nsec > 1000000000L) {
		result.tv_nsec -= 1000000000L;
		result.tv_sec ++;
	}

	return result;
}

inline void operator+=(timespec &result, const timespec b) {
	result.tv_sec += b.tv_sec;
	result.tv_nsec += b.tv_nsec;
	if (result.tv_nsec > 1000000000L) {
		result.tv_nsec -= 1000000000L;
		result.tv_sec ++;
	}
}

inline timespec operator/(const timespec &a, const int b) {
	long int r; // remainder
	timespec result;

	if (b==0) {
		result.tv_sec = -1;
		result.tv_nsec = -1;
		return result;
	}

	r = a.tv_sec % b;
	result.tv_sec = a.tv_sec / b;
	result.tv_nsec = r * 1000000000L / b;
	result.tv_nsec += a.tv_nsec / b;

	return result;
}

/**
 * @note: this is not accurate
 */
inline long int operator/(const timespec &a, const timespec &b) {
	if (a.tv_sec != 0) {
		if (b.tv_sec == 0) {
			return -1;
		} else {
			return a.tv_sec / b.tv_sec;
		}
	} else {
		if (b.tv_nsec == 0) {
			return -1;
		} else {
			return a.tv_nsec / b.tv_nsec;
		}
	}
}

inline bool operator==(const timespec &a, const timespec &b) {
	return (a.tv_sec == b.tv_sec) && (a.tv_nsec == b.tv_nsec);
}
} // end anonymous namespace

namespace cacao {
namespace {
inline OStream& operator<<(OStream &ostr, timespec ts) {
	const char *unit;
	if (ts.tv_sec >= 10) {
		// display seconds if at least 10 sec
		ostr << ts.tv_sec;
		unit = "sec";
	} else {
		ts.tv_nsec += ts.tv_sec * 1000000000L;
		if (ts.tv_nsec >= 100000000) {
			// display milliseconds if at least 100ms
			ostr << ts.tv_nsec/1000000;
			unit = "msec";
		} else {
			// otherwise display microseconds
			ostr << ts.tv_nsec/1000;
			unit = "usec";
		}
	}
	ostr << setw(5) << unit;
	return ostr;
}
} // end anonymous namespace
} // end namespace cacao

#include <vector>

namespace cacao {
/**
 * Superclass of real-time group entries.
 */
class RTEntry {
protected:
	const char* name;
	const char* description;
public:
	static timespec invalid_ts;   //< invalid time stamp
	/**
	 * Constructor.
	 */
	RTEntry(const char* name, const char* description) : name(name), description(description) {}
	/**
	 * Print the timing information to an OStream.
	 * @param[in,out] O     output stream
	 * @param[in]     ref   time reference. Used to calculate percentage .
	 *	Normally the time of the parent.
	 */
	virtual void print(OStream &O,timespec ref) const = 0;
	/**
	 * Get the elapsed time of a RTEntry.
	 */
	virtual timespec time() const = 0;

};

/**
 * Real-time group.
 * Used to collect a set of RTEntry's
 */
class RTGroup : public RTEntry {
private:
	typedef std::vector<RTEntry*> RTEntryList;
	/**
	 * List of members.
	 * @note: this must be a pointer. If it is changes to a normal variable strange
	 * things will happen!
	 */
	RTEntryList *members;

	RTGroup(const char* name, const char* description) : RTEntry(name, description) {
		members = new RTEntryList();
	}
public:
	/**
	 * __the__ root group.
	 * @note never use this variable directly (use RTGroup::root() instead).
	 * Used for internal purposes only!
	 */
	static RTGroup root_rg;
	/**
	 * Get the root group
	 */
	static RTGroup& root() {
		return root_rg;
	}

	/**
	 * Create a new real-time group.
	 * @param[in] name name of the group
	 * @param[in] description description of the group
	 * @param[in] group parent group.
	 */
	RTGroup(const char* name, const char* description, RTGroup &group) : RTEntry(name, description) {
		members = new RTEntryList();
		group.add(this);
	}

	~RTGroup() {
		delete members;
	}

	/**
	 * Add an entry to the group
	 */
	void add(RTEntry *re) {
		members->push_back(re);
	}

	virtual timespec time() const {
		timespec time = {0,0};
		for(RTEntryList::const_iterator i = members->begin(), e = members->end(); i != e; ++i) {
			RTEntry* re = *i;
			time += re->time();
		}
		return time;
	}

	void print(OStream &O,timespec ref = invalid_ts) const {
		timespec duration = time();
		if (ref == invalid_ts)
			ref = duration;
		// O << setw(10) << left << name << right <<"   " << description << nl;
		O << indent;
		for(RTEntryList::const_iterator i = members->begin(), e= members->end(); i != e; ++i) {
			RTEntry* re = *i;
			re->print(O,duration);
		}
		O << dedent;
		int percent = duration / (ref / 100);
		O << nl;
		O << setw(10) << duration << " "
		  << setw(4) << percent << "% "
		  << setw(20) << name << ": total time" << nl<< nl;
	}
};

/**
 * Real-time timer.
 *
 * @note could be ported to std::chrono if C++11 is available
 */
class RTTimer : public RTEntry {
private:
	timespec startstamp;
	timespec duration;
public:
	/**
	 * Create a new real-time timer.
	 * @param[in] name name of the timer
	 * @param[in] description description of the timer
	 * @param[in] group parent group.
	 */
	RTTimer(const char* name, const char* description, RTGroup &parent) : RTEntry(name, description) {
		reset();
		parent.add(this);
	}

	/**
	 * Start the timer.
	 * @see stop()
	 * @see stopstart()
	 */
	inline void start() {
		rt_timing_gettime(&startstamp);
	}

	/**
	 * Start the timer.
	 * @note Only use after a start() or stopstart()
	 * @see start()
	 * @see stopstart()
	 */
	inline void stop() {
		timespec stopstamp;
		rt_timing_gettime(&stopstamp);
		duration += stopstamp - startstamp;
	}

	/**
	 * Stop this timer and start a second timer.
	 * This method stops the current timer and starts the timer in the parameter.
	 * The advantage is that the current is only requested once as opposed to calling
	 * stop() and start() separately.
	 *
	 * @note Only use after a start() or stopstart()
	 * @see start()
	 * @see stop()
	 */
	inline void stopstart(RTTimer &timer) {
		timespec stopstamp;
		rt_timing_gettime(&stopstamp);
		duration += stopstamp - startstamp;
		timer.startstamp = stopstamp;
	}

	inline void reset() {
		duration.tv_sec = 0;
		duration.tv_nsec = 0;
	}

	virtual timespec time() const {
		return duration;
	}

	void print(OStream &O,timespec ref = invalid_ts) const {
		if (ref == invalid_ts)
			ref = time();
		int percent = duration / (ref / 100);
		O << setw(10) << duration.tv_nsec/1000 << " usec "
		  << setw(4) << percent << "% "
		  << setw(20) << name << ": " << description << nl;
	}
};

} // end namespace cacao

#define RT_REGISTER_TIMER(var,name,description)                              \
	static cacao::RTTimer var##_rt(name,description,cacao:RTGroup::root());  \
	static cacao::RTTimer& var() {	                                         \
		return var##_rt;                                                     \
	}

#define RT_REGISTER_GROUP_TIMER(var,name,description,group)                  \
	static cacao::RTTimer var##_rt(name, description, group());              \
	static cacao::RTTimer& var() {                                           \
		return var##_rt;                                                     \
	}

#define RT_REGISTER_GROUP(var,name,description)                              \
	static cacao::RTGroup var##_rg(name,description,cacao::RTGroup::root()); \
	cacao::RTGroup& var() {                                                  \
		return var##_rg;                                                     \
	}


#define RT_REGISTER_SUBGROUP(var,name,description,group)                     \
	static cacao::RTGroup var##_rg(name, description, group());              \
	cacao::RTGroup& var() {                                                  \
		return var##_rg;                                                     \
	}

#define RT_TIMER_START(var) var().start()
#define RT_TIMER_STOP(var) var().stop()
#define RT_TIMER_STOPSTART(var1,var2) var1().stopstart(var2())

#else /* !defined(ENABLE_RT_TIMING) */

#define RT_TIMING_GET_TIME(ts)
#define RT_TIMING_TIME_DIFF(a,b,index)


#define RT_REGISTER_TIMER(var,name,description)
#define RT_REGISTER_GROUP_TIMER(var,name,description,group)
#define RT_REGISTER_GROUP(var,name,description)
#define RT_REGISTER_SUBGROUP(var,name,description,group)

#define RT_TIMER_START(var)
#define RT_TIMER_STOP(var)
#define RT_TIMER_STOPSTART(var1,var2)

#endif /* defined(ENABLE_RT_TIMING) */

#endif /* _RT_TIMING_HPP */

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
