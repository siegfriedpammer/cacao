/* src/threads/threadlist.cpp - thread list

   Copyright (C) 2008, 2009
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

#include <algorithm>

#include "threads/mutex.hpp"
#include "threads/threadlist.hpp"
#include "threads/thread.hpp"

#include "toolbox/list.hpp"
#include "toolbox/logging.hpp"

#include "vm/jit/stacktrace.hpp"


/* class variables */

Mutex               ThreadList::_mutex;                // a mutex for all thread lists

List<threadobject*> ThreadList::_active_thread_list;   // list of active threads
List<threadobject*> ThreadList::_free_thread_list;     // list of free threads
List<int32_t>       ThreadList::_free_index_list;      // list of free thread indexes

int32_t             ThreadList::_number_of_started_java_threads;
int32_t             ThreadList::_number_of_active_java_threads;
int32_t             ThreadList::_peak_of_active_java_threads;
int32_t             ThreadList::_number_of_non_daemon_threads;

int32_t             ThreadList::_last_index = 0;


/**
 * Dumps info for all threads running in the VM.  This function is
 * called when SIGQUIT (<ctrl>-\) is sent to the VM.
 */
void ThreadList::dump_threads()
{
	// XXX we should stop the world here and remove explicit
	//     thread suspension from the loop below.
	// Lock the thread lists.
	lock();

	printf("Full thread dump CACAO "VERSION_FULL":\n");

	// Iterate over all started threads.
	threadobject* self = THREADOBJECT;
	for (List<threadobject*>::iterator it = _active_thread_list.begin(); it != _active_thread_list.end(); it++) {
		threadobject* t = *it;

		// Ignore threads which are in state NEW.
		if (t->state == THREAD_STATE_NEW)
			continue;

		/* Suspend the thread (and ignore return value). */

		if (t != self)
			(void) threads_suspend_thread(t, SUSPEND_REASON_DUMP);

		/* Print thread info. */

		printf("\n");
		thread_print_info(t);
		printf("\n");

		/* Print trace of thread. */

		stacktrace_print_of_thread(t);

		/* Resume the thread (and ignore return value). */

		if (t != self)
			(void) threads_resume_thread(t, SUSPEND_REASON_DUMP);
	}

	// Unlock the thread lists.
	unlock();
}


/**
 * Fills the passed list with all currently active threads. Creating a copy
 * of the thread list here, is the only way to ensure we do not end up in a
 * dead-lock when iterating over the list.
 *
 * @param list list class to be filled
 */
void ThreadList::get_active_threads(List<threadobject*> &list)
{
	// Lock the thread lists.
	lock();

	// Use the assignment operator to create a copy of the thread list.
	list = _active_thread_list;

	// Unlock the thread lists.
	unlock();
}


/**
 * Fills the passed list with all currently active threads which should be
 * visible to Java. Creating a copy of the thread list here, is the only way
 * to ensure we do not end up in a dead-lock when iterating over the list.
 *
 * @param list list class to be filled
 */
void ThreadList::get_active_java_threads(List<threadobject*> &list)
{
	// Lock the thread lists.
	lock();

	// Iterate over all active threads.
	for (List<threadobject*>::iterator it = _active_thread_list.begin(); it != _active_thread_list.end(); it++) {
		threadobject* t = *it;

		// We skip internal threads.
		if (t->flags & THREAD_FLAG_INTERNAL)
			continue;

		list.push_back(t);
	}

	// Unlock the thread lists.
	unlock();
}


/**
 * Return a free thread object.
 *
 * @return free thread object or NULL if none available
 */
threadobject* ThreadList::get_free_thread()
{
	threadobject* t = NULL;

	lock();

	// Do we have free threads in the free-list?
	if (_free_thread_list.empty() == false) {
		// Yes, get the index and remove it from the free list.
		t = _free_thread_list.front();
		_free_thread_list.remove(t);
	}

	unlock();

	return t;
}


/**
 * Return a free thread index.
 *
 * @return free thread index
 */
int32_t ThreadList::get_free_thread_index()
{
	int32_t index;

	lock();

	// Do we have free indexes in the free-list?
	if (_free_index_list.empty() == false) {
		// Yes, get the index and remove it from the free list.
		index = _free_index_list.front();
		_free_index_list.remove(index);
	}
	else {
		// Get a new the thread index.
		index = ++_last_index;
	}

	unlock();

	return index;
}


/**
 * Return the number of daemon threads visible to Java.
 *
 * NOTE: This function does a linear-search over the threads list,
 *       because it is only used by the management interface.
 *
 * @return number of daemon threads
 */
int32_t ThreadList::get_number_of_daemon_java_threads(void)
{
	int number = 0;

	// Lock the thread lists.
	lock();

	// Iterate over all active threads.
	for (List<threadobject*>::iterator it = _active_thread_list.begin(); it != _active_thread_list.end(); it++) {
		threadobject* t = *it;

		// We skip internal threads.
		if (t->flags & THREAD_FLAG_INTERNAL)
			continue;

		if (thread_is_daemon(t))
			number++;
	}

	// Unlock the thread lists.
	unlock();

	return number;
}


/**
 * Return the number of non-daemon threads.
 *
 * NOTE: This function does a linear-search over the threads list,
 *       because it is only used for joining the threads.
 *
 * @return number of non daemon threads
 */
int32_t ThreadList::get_number_of_non_daemon_threads(void)
{
	int nondaemons = 0;

	lock();

	for (List<threadobject*>::iterator it = _active_thread_list.begin(); it != _active_thread_list.end(); it++) {
		threadobject* t = *it;

		if (!thread_is_daemon(t))
			nondaemons++;
	}

	unlock();

	return nondaemons;
}


/**
 * Return the thread object with the given index.
 *
 * @return thread object
 */
threadobject* ThreadList::get_thread_by_index(int32_t index)
{
	lock();

	List<threadobject*>::iterator it = find_if(_active_thread_list.begin(), _active_thread_list.end(), std::bind2nd(comparator(), index));

	// No thread found.
	if (it == _active_thread_list.end()) {
		unlock();
		return NULL;
	}

	threadobject* t = *it;

	// The thread found is in state new.
	if (t->state == THREAD_STATE_NEW) {
		unlock();
		return NULL;
	}

	unlock();
	return t;
}


/**
 * Return the Java thread object from the given thread object.
 *
 * @return Java thread object
 */
threadobject* ThreadList::get_thread_from_java_object(java_handle_t* h)
{
	List<threadobject*>::iterator it;
	threadobject* t;
	bool          equal;

	lock();

	for (it = _active_thread_list.begin(); it != _active_thread_list.end(); it++) {
		t = *it;

		LLNI_equals(t->object, h, equal);

		if (equal == true) {
			unlock();
			return t;
		}
	}

	unlock();

	return NULL;
}

void ThreadList::deactivate_thread(threadobject *t)
{
	ThreadListLocker lock;
	remove_from_active_thread_list(t);
	threads_impl_clear_heap_pointers(t); // allow it to be garbage collected
}

/**
 * Release the thread.
 *
 * @return free thread index
 */
void ThreadList::release_thread(threadobject* t, bool needs_deactivate)
{
	lock();

	if (needs_deactivate)
		// Move thread from active thread list to free thread list.
		remove_from_active_thread_list(t);
	else
		assert(!t->is_in_active_list);

	add_to_free_thread_list(t);

	// Add thread index to free index list.
	add_to_free_index_list(t->index);

	unlock();
}


/* C interface functions ******************************************************/

extern "C" {
	void ThreadList_lock() { ThreadList::lock(); }
	void ThreadList_unlock() { ThreadList::unlock(); }
	void ThreadList_dump_threads() { ThreadList::dump_threads(); }
	threadobject* ThreadList_get_free_thread() { return ThreadList::get_free_thread(); }
	int32_t ThreadList_get_free_thread_index() { return ThreadList::get_free_thread_index(); }
	void ThreadList_add_to_active_thread_list(threadobject* t) { ThreadList::add_to_active_thread_list(t); }
	threadobject* ThreadList_get_thread_by_index(int32_t index) { return ThreadList::get_thread_by_index(index); }
	threadobject* ThreadList_get_main_thread() { return ThreadList::get_main_thread(); }
	threadobject* ThreadList_get_thread_from_java_object(java_handle_t* h) { return ThreadList::get_thread_from_java_object(h); }

	int32_t ThreadList_get_number_of_non_daemon_threads() { return ThreadList::get_number_of_non_daemon_threads(); }
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
