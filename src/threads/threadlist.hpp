/* src/threads/threadlist.hpp - different thread-lists

   Copyright (C) 1996-2011
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


#ifndef _THREADLIST_HPP
#define _THREADLIST_HPP

#include "config.h"
#include <utility>
#include <stdint.h>

#include "threads/thread.hpp"

#include "toolbox/list.hpp"

#include "vm/global.hpp"


/* ThreadList *****************************************************************/

class ThreadList {
private:
	static Mutex               _mutex;              // a mutex for all thread lists

	static List<threadobject*> _active_thread_list; // list of active threads
	static List<threadobject*> _free_thread_list;   // list of free threads

	// Thread counters visible to Java.
	static int32_t             _number_of_started_java_threads;
	static int32_t             _number_of_active_java_threads;
	static int32_t             _peak_of_active_java_threads;

	// Thread counters for internal usage.
	static int32_t             _number_of_non_daemon_threads;

	static int32_t             _last_index;

	static void                 remove_from_active_thread_list(threadobject* t);
private:
	// Comparator class.
	class comparator : public std::binary_function<threadobject*, int32_t, bool> {
	public:
		bool operator() (const threadobject* t, const int32_t index) const
		{
			return (t->index == index);
		}
	};

public:
	static Mutex&         mutex() { return _mutex; }

	static void           wait_cond(Condition *cond) { cond->wait(_mutex); }

	static void           add_to_active_thread_list(threadobject* t);

	// Thread management methods.
	static threadobject*  get_main_thread();
	static void           get_free_thread(threadobject **t, int32_t *index);
	static threadobject*  get_thread_by_index(int32_t index);
	static threadobject*  get_thread_from_java_object(java_handle_t* h);
	static void           release_thread(threadobject* t, bool needs_deactivate);
	static void           deactivate_thread(threadobject *t);

	// Thread listing methods.
	static void           get_active_threads(List<threadobject*> &list);
	static void           get_active_java_threads(List<threadobject*> &list);

	// Thread counting methods visible to Java.
	static int32_t        get_number_of_started_java_threads();
	static int32_t        get_number_of_active_java_threads();
	static int32_t        get_number_of_daemon_java_threads();
	static int32_t        get_peak_of_active_java_threads();
	static void           reset_peak_of_active_java_threads();

	// Thread counting methods for internal use.
	static int32_t        get_number_of_active_threads();
	static int32_t        get_number_of_non_daemon_threads();

	// Debugging methods.
	static void           dump_threads();
};

inline void ThreadList::add_to_active_thread_list(threadobject* t)
{
	MutexLocker(mutex());

	_active_thread_list.push_back(t);
	t->is_in_active_list = true;

	// Update counter variables.
	if ((t->flags & THREAD_FLAG_INTERNAL) == 0) {
		_number_of_started_java_threads++;
		_number_of_active_java_threads++;
		_peak_of_active_java_threads = MAX(_peak_of_active_java_threads, _number_of_active_java_threads);
	}
}

inline void ThreadList::remove_from_active_thread_list(threadobject* t)
{
	MutexLocker(mutex());

	_active_thread_list.remove(t);
	t->is_in_active_list = false;

	// Update counter variables.
	if ((t->flags & THREAD_FLAG_INTERNAL) == 0) {
		_number_of_active_java_threads--;
	}
}

inline threadobject* ThreadList::get_main_thread()
{
	MutexLocker(mutex());

	return _active_thread_list.front();
}

inline int32_t ThreadList::get_number_of_active_threads()
{
	MutexLocker(mutex());

	return _active_thread_list.size();
}

inline int32_t ThreadList::get_number_of_started_java_threads()
{
	MutexLocker(mutex());

	return _number_of_started_java_threads;
}

inline int32_t ThreadList::get_number_of_active_java_threads()
{
	MutexLocker(mutex());

	return _number_of_active_java_threads;
}

inline int32_t ThreadList::get_peak_of_active_java_threads()
{
	MutexLocker(mutex());

	return _peak_of_active_java_threads;
}

inline void ThreadList::reset_peak_of_active_java_threads()
{
	MutexLocker(mutex());

	_peak_of_active_java_threads = _number_of_active_java_threads;
}

#endif // _THREADLIST_HPP


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
