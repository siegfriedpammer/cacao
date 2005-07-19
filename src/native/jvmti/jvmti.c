/* src/native/jvmti.c - implementation of the Java Virtual Machine Tool 
                        Interface functions

   Copyright (C) 1996-2005 R. Grafl, A. Krall, C. Kruegel, C. Oates,
   R. Obermaisser, M. Platter, M. Probst, S. Ring, E. Steiner,
   C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich, J. Wenninger,
   Institut f. Computersprachen - TU Wien

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Author: Martin Platter

   Changes:             

   
   $Id: jvmti.c 3066 2005-07-19 12:35:37Z twisti $

*/


#include "native/jni.h"
#include "native/jvmti/jvmti.h"
#include "vm/global.h"
#include "vm/loader.h"
#include "vm/builtin.h"
#include "vm/jit/asmpart.h"
#include "mm/boehm.h"
#include "toolbox/logging.h"
#include "vm/options.h"
#include "cacao/cacao.h"
#include "vm/stringlocal.h"
#include "mm/memory.h"
#include "threads/native/threads.h"
#include "vm/exceptions.h"

#include <string.h>
#include <linux/unistd.h>
#include <sys/time.h>

static jvmtiPhase phase; 

typedef struct {
    jvmtiEnv env;
    jvmtiEventCallbacks callbacks;
    jobject *events;   /* hashtable for enabled/disabled jvmtiEvents */
    jvmtiCapabilities capabilities;
    void *EnvironmentLocalStorage;
} environment;


/* jmethodID and jclass caching */
static jclass ihmclass = NULL;
static jmethodID ihmmid = NULL;

jvmtiEnv JVMTI_EnvTable;
jvmtiCapabilities JVMTI_Capabilities;

#define CHECK_PHASE_START  if (!(0 
#define CHECK_PHASE(chkphase) || (phase == chkphase)
#define CHECK_PHASE_END  )) return JVMTI_ERROR_WRONG_PHASE


/* SetEventNotificationMode ****************************************************

   Control the generation of events

*******************************************************************************/

jvmtiError
SetEventNotificationMode (jvmtiEnv * env, jvmtiEventMode mode,
			  jvmtiEvent event_type, jthread event_thread, ...)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
    
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");    
    
    return JVMTI_ERROR_NONE;
}


/* GetAllThreads ***************************************************************

   Get all live threads

*******************************************************************************/

jvmtiError
GetAllThreads (jvmtiEnv * env, jint * threads_count_ptr,
	       jthread ** threads_ptr)
{
    int i = 0; 
    threadobject* thread;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((threads_count_ptr == NULL) || (threads_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    thread = mainthreadobj;
    do {
        i++;
        thread = thread->info.prev;
    } while (thread != mainthreadobj);
    
    *threads_count_ptr = i;

    *threads_ptr = (jthread*) heap_allocate(sizeof(jthread) * i,true,NULL);
    i=0;
    do {
        memcpy(*threads_ptr[i],thread,sizeof(jthread));
        thread = thread->info.prev;
        i++;
    } while (thread != mainthreadobj);

    return JVMTI_ERROR_NONE;
}


/* SuspendThread ***************************************************************

   

*******************************************************************************/

jvmtiError
SuspendThread (jvmtiEnv * env, jthread thread)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ResumeThread (jvmtiEnv * env, jthread thread)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
StopThread (jvmtiEnv * env, jthread thread, jobject exception)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
InterruptThread (jvmtiEnv * env, jthread thread)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* GetThreadInfo ***************************************************************

   Get thread information. The fields of the jvmtiThreadInfo structure are 
   filled in with details of the specified thread.

*******************************************************************************/

jvmtiError
GetThreadInfo (jvmtiEnv * env, jthread thread, jvmtiThreadInfo * info_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* GetOwnedMonitorInfo *********************************************************

   

*******************************************************************************/

jvmtiError
GetOwnedMonitorInfo (jvmtiEnv * env, jthread thread,
		     jint * owned_monitor_count_ptr,
		     jobject ** owned_monitors_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetCurrentContendedMonitor (jvmtiEnv * env, jthread thread,
			    jobject * monitor_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RunAgentThread (jvmtiEnv * env, jthread thread, jvmtiStartFunction proc,
		const void *arg, jint priority)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetTopThreadGroups (jvmtiEnv * env, jint * group_count_ptr,
		    jthreadGroup ** groups_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadGroupInfo (jvmtiEnv * env, jthreadGroup group,
		    jvmtiThreadGroupInfo * info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadGroupChildren (jvmtiEnv * env, jthreadGroup group,
			jint * thread_count_ptr, jthread ** threads_ptr,
			jint * group_count_ptr, jthreadGroup ** groups_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetFrameCount (jvmtiEnv * env, jthread thread, jint * count_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetThreadState **************************************************************

   Get the state of a thread. 

*******************************************************************************/

jvmtiError
GetThreadState (jvmtiEnv * env, jthread thread, jint * thread_state_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetFrameLocation (jvmtiEnv * env, jthread thread, jint depth,
		  jmethodID * method_ptr, jlocation * location_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
NotifyFramePop (jvmtiEnv * env, jthread thread, jint depth)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetLocalObject (jvmtiEnv * env,
		jthread thread, jint depth, jint slot, jobject * value_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetLocalInt (jvmtiEnv * env,
	     jthread thread, jint depth, jint slot, jint * value_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetLocalLong (jvmtiEnv * env, jthread thread, jint depth, jint slot,
	      jlong * value_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetLocalFloat (jvmtiEnv * env, jthread thread, jint depth, jint slot,
	       jfloat * value_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetLocalDouble (jvmtiEnv * env, jthread thread, jint depth, jint slot,
		jdouble * value_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetLocalObject (jvmtiEnv * env, jthread thread, jint depth, jint slot,
		jobject value)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetLocalInt (jvmtiEnv * env, jthread thread, jint depth, jint slot,
	     jint value)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetLocalLong (jvmtiEnv * env, jthread thread, jint depth, jint slot,
	      jlong value)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetLocalFloat (jvmtiEnv * env, jthread thread, jint depth, jint slot,
	       jfloat value)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetLocalDouble (jvmtiEnv * env, jthread thread, jint depth, jint slot,
		jdouble value)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
CreateRawMonitor (jvmtiEnv * env, const char *name,
		  jrawMonitorID * monitor_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
DestroyRawMonitor (jvmtiEnv * env, jrawMonitorID monitor)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RawMonitorEnter (jvmtiEnv * env, jrawMonitorID monitor)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RawMonitorExit (jvmtiEnv * env, jrawMonitorID monitor)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RawMonitorWait (jvmtiEnv * env, jrawMonitorID monitor, jlong millis)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RawMonitorNotify (jvmtiEnv * env, jrawMonitorID monitor)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RawMonitorNotifyAll (jvmtiEnv * env, jrawMonitorID monitor)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetBreakpoint (jvmtiEnv * env, jmethodID method, jlocation location)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ClearBreakpoint (jvmtiEnv * env, jmethodID method, jlocation location)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetFieldAccessWatch (jvmtiEnv * env, jclass klass, jfieldID field)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ClearFieldAccessWatch (jvmtiEnv * env, jclass klass, jfieldID field)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetFieldModificationWatch (jvmtiEnv * env, jclass klass, jfieldID field)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ClearFieldModificationWatch (jvmtiEnv * env, jclass klass, jfieldID field)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* Allocate ********************************************************************

   Allocate an area of memory through the JVM TI allocator. The allocated 
   memory should be freed with Deallocate

*******************************************************************************/

jvmtiError
Allocate (jvmtiEnv * env, jlong size, unsigned char **mem_ptr)
{
    
    if (mem_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
    if (size < 0) return JVMTI_ERROR_ILLEGAL_ARGUMENT;

    *mem_ptr = heap_allocate(sizeof(size),true,NULL);
    if (*mem_ptr == NULL) 
        return JVMTI_ERROR_OUT_OF_MEMORY;
    else
        return JVMTI_ERROR_NONE;
    
}


/* Deallocate ******************************************************************

   Deallocate mem using the JVM TI allocator.

*******************************************************************************/

jvmtiError
Deallocate (jvmtiEnv * env, unsigned char *mem)
{
    /* let Boehm GC do the job */
    return JVMTI_ERROR_NONE;
}


/* GetClassSignature ************************************************************

   For the class indicated by klass, return the JNI type signature and the 
   generic signature of the class.

*******************************************************************************/

jvmtiError
GetClassSignature (jvmtiEnv * env, jclass klass, char **signature_ptr,
		   char **generic_ptr)
{
    int nsize,psize;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
       
    if ((generic_ptr== NULL)||(signature_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    nsize=((classinfo*)klass)->name->blength;
    psize=((classinfo*)klass)->packagename->blength;

    *signature_ptr = (char*) 
        heap_allocate(sizeof(char)* nsize+psize+4,true,NULL);

    *signature_ptr[0]='L';
    memcpy(&(*signature_ptr[1]),((classinfo*)klass)->packagename->text, psize);
    *signature_ptr[psize+2]='/';
    memcpy(&(*signature_ptr[psize+3]),((classinfo*)klass)->name->text, nsize);
    *signature_ptr[nsize+psize+3]=';';
    *signature_ptr[nsize+psize+4]='\0';

    *generic_ptr = NULL;

    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetClassStatus (jvmtiEnv * env, jclass klass, jint * status_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetSourceFileName **********************************************************

   For the class indicated by klass, return the source file name.

*******************************************************************************/

jvmtiError
GetSourceFileName (jvmtiEnv * env, jclass klass, char **source_name_ptr)
{
    int size; 

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((klass == NULL)||(source_name_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;
    
    size = (((classinfo*)klass)->sourcefile->blength);

    *source_name_ptr = (char*) heap_allocate(sizeof(char)* size,true,NULL);
    
    memcpy(*source_name_ptr,((classinfo*)klass)->sourcefile->text, size);

    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetClassModifiers (jvmtiEnv * env, jclass klass, jint * modifiers_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetClassMethods *************************************************************

   For the class indicated by klass, return a count of methods and a list of 
   method IDs

*******************************************************************************/

jvmtiError
GetClassMethods (jvmtiEnv * env, jclass klass, jint * method_count_ptr,
		 jmethodID ** methods_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((klass == NULL)||(methods_ptr == NULL)||(method_count_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    *method_count_ptr = (jint)((classinfo*)klass)->methodscount;
    *methods_ptr = (jmethodID*) 
        heap_allocate(sizeof(jmethodID) * (*method_count_ptr),true,NULL);
    
    memcpy (*methods_ptr, ((classinfo*)klass)->methods, 
            sizeof(jmethodID) * (*method_count_ptr));
    
    return JVMTI_ERROR_NONE;
}


/* GetClassFields *************************************************************

   For the class indicated by klass, return a count of fields and a list of 
   field IDs.

*******************************************************************************/

jvmtiError
GetClassFields (jvmtiEnv * env, jclass klass, jint * field_count_ptr,
		jfieldID ** fields_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((klass == NULL)||(fields_ptr == NULL)||(field_count_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    *field_count_ptr = (jint)((classinfo*)klass)->fieldscount;
    *fields_ptr = (jfieldID*) 
        heap_allocate(sizeof(jfieldID) * (*field_count_ptr),true,NULL);
    
    memcpy (*fields_ptr, ((classinfo*)klass)->fields, 
            sizeof(jfieldID) * (*field_count_ptr));
    
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetImplementedInterfaces (jvmtiEnv * env, jclass klass,
			  jint * interface_count_ptr,
			  jclass ** interfaces_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* IsInterface ****************************************************************

   Determines whether a class object reference represents an interface.

*******************************************************************************/

jvmtiError
IsInterface (jvmtiEnv * env, jclass klass, jboolean * is_interface_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((klass == NULL)||(is_interface_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;
    
    *is_interface_ptr = (((classinfo*)klass)->flags & ACC_INTERFACE);

    return JVMTI_ERROR_NONE;
}

/* IsArrayClass ***************************************************************

   Determines whether a class object reference represents an array.

*******************************************************************************/

jvmtiError
IsArrayClass (jvmtiEnv * env, jclass klass, jboolean * is_array_class_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if (is_array_class_ptr == NULL) 
        return JVMTI_ERROR_NULL_POINTER;

    *is_array_class_ptr = ((classinfo*)klass)->name->text[0] == '[';

    return JVMTI_ERROR_NONE;
}


/* GetClassLoader *************************************************************

   For the class indicated by klass, return via classloader_ptr a reference to 
   the class loader for the class.

*******************************************************************************/

jvmtiError
GetClassLoader (jvmtiEnv * env, jclass klass, jobject * classloader_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((klass == NULL)||(classloader_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    *classloader_ptr = (jobject)((classinfo*)klass)->classloader;
 
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetObjectHashCode (jvmtiEnv * env, jobject object, jint * hash_code_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetObjectMonitorUsage (jvmtiEnv * env, jobject object,
		       jvmtiMonitorUsage * info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetFieldName ***************************************************************

   For the field indicated by klass and field, return the field name and 
   signature.

*******************************************************************************/

jvmtiError
GetFieldName (jvmtiEnv * env, jclass klass, jfieldID field,
	      char **name_ptr, char **signature_ptr, char **generic_ptr)
{
    int size; 

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((field == NULL)||(name_ptr == NULL)||(signature_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;
    
    size = (((fieldinfo*)field)->name->blength);
    *name_ptr = (char*) heap_allocate(sizeof(char)* size,true,NULL);    
    memcpy(*name_ptr,((fieldinfo*)field)->name->text, size);

    size = (((fieldinfo*)field)->descriptor->blength);
    *signature_ptr = (char*) heap_allocate(sizeof(char)* size,true,NULL);    
    memcpy(*signature_ptr,((fieldinfo*)field)->descriptor->text, size);

    *generic_ptr = NULL;

    return JVMTI_ERROR_NONE;
}


/* GetFieldDeclaringClass *****************************************************

   For the field indicated by klass and field return the class that defined it 
   The declaring class will either be klass, a superclass, or an implemented 
   interface.

*******************************************************************************/

jvmtiError
GetFieldDeclaringClass (jvmtiEnv * env, jclass klass, jfieldID field,
			jclass * declaring_class_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    /* todo: find declaring class  */

    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetFieldModifiers (jvmtiEnv * env, jclass klass, jfieldID field,
		   jint * modifiers_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
IsFieldSynthetic (jvmtiEnv * env, jclass klass, jfieldID field,
		  jboolean * is_synthetic_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetMethodName ***************************************************************

   For the method indicated by method, return the method name via name_ptr and 
   method signature via signature_ptr.

*******************************************************************************/

jvmtiError
GetMethodName (jvmtiEnv * env, jmethodID method, char **name_ptr,
	       char **signature_ptr, char **generic_ptr)
{
    methodinfo* m = (methodinfo*)method;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((method == NULL) || (name_ptr == NULL) || (signature_ptr == NULL)
        || (generic_ptr == NULL)) return JVMTI_ERROR_NULL_POINTER;

    *name_ptr = (char*)heap_allocate(m->name->blength,true,NULL);
    memcpy(*name_ptr, m->name->text, m->name->blength);

    *signature_ptr = (char*)heap_allocate(m->descriptor->blength,true,NULL);
    memcpy(*signature_ptr, m->descriptor->text, m->descriptor->blength);
    
    /* there is no generic signature attribute */
    *generic_ptr = NULL;

    return JVMTI_ERROR_NONE;
}


/* GetMethodDeclaringClass *****************************************************

  For the method indicated by method, return the class that defined it.

*******************************************************************************/

jvmtiError
GetMethodDeclaringClass (jvmtiEnv * env, jmethodID method,
			 jclass * declaring_class_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
     
    if ((method == NULL) || (declaring_class_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;
    
    *declaring_class_ptr = (jclass)((methodinfo*)method)->class;
    
    return JVMTI_ERROR_NONE;
}


/* GetMethodModifiers **********************************************************

   For the method indicated by method, return the access flags.

*******************************************************************************/

jvmtiError
GetMethodModifiers (jvmtiEnv * env, jmethodID method, jint * modifiers_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((method == NULL) || (modifiers_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    *modifiers_ptr = (jint) (((methodinfo*)method)->flags);

    return JVMTI_ERROR_NONE;
}


/* GetMaxLocals ****************************************************************

   For the method indicated by method, return the number of local variable slots 
   used by the method, including the local variables used to pass parameters to 
   the method on its invocation.

*******************************************************************************/

jvmtiError
GetMaxLocals (jvmtiEnv * env, jmethodID method, jint * max_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((method == NULL)||(max_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;    
    
    if (((methodinfo*)method)->flags & ACC_NATIVE)  
        return JVMTI_ERROR_NATIVE_METHOD;
   
    *max_ptr = (jint) ((methodinfo*)method)->maxlocals;

    return JVMTI_ERROR_NONE;
}



/* GetArgumentsSize ************************************************************

   Return the number of local variable slots used by the method's arguments.

*******************************************************************************/

jvmtiError
GetArgumentsSize (jvmtiEnv * env, jmethodID method, jint * size_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((method == NULL)||(size_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;    
    
    if (((methodinfo*)method)->flags & ACC_NATIVE)  
        return JVMTI_ERROR_NATIVE_METHOD;

    *size_ptr = (jint)((methodinfo*)method)->paramcount;
    return JVMTI_ERROR_NONE;
}



/* GetLineNumberTable ***********************************************************

   For the method indicated by method, return a table of source line number 
   entries.

*******************************************************************************/

jvmtiError
GetLineNumberTable (jvmtiEnv * env, jmethodID method,
		    jint * entry_count_ptr, jvmtiLineNumberEntry ** table_ptr)
{
    int i;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
   
    if ((method == NULL) || (entry_count_ptr == NULL) || (table_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;    
    if (((methodinfo*)method)->flags & ACC_NATIVE)  
        return JVMTI_ERROR_NATIVE_METHOD;
    if (((methodinfo*)method)->linenumbers == NULL) 
        return JVMTI_ERROR_ABSENT_INFORMATION;

    *entry_count_ptr= (jint)((methodinfo*)method)->linenumbercount;
    *table_ptr = (jvmtiLineNumberEntry*) heap_allocate(
        sizeof(jvmtiLineNumberEntry) * (*entry_count_ptr),true,NULL);


    for (i=0; i < *entry_count_ptr; i++) {
        (*table_ptr[i]).start_location = 
            (jlocation) method->linenumbers[i].start_pc;
        (*table_ptr[i]).line_number = 
            (jint) ((methodinfo*)method)->linenumbers[i].line_number;
    }
    
    return JVMTI_ERROR_NONE;
}


/* GetMethodLocation ***********************************************************

   For the method indicated by method, return the beginning and ending addresses 
   through start_location_ptr and end_location_ptr. In cacao this points to 
   entry point in machine code and length of machine code

*******************************************************************************/

jvmtiError
GetMethodLocation (jvmtiEnv * env, jmethodID method,
		   jlocation * start_location_ptr,
		   jlocation * end_location_ptr)
{
    methodinfo* m = (methodinfo*)method;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((method == NULL) || (start_location_ptr == NULL) || 
        (end_location_ptr == NULL)) return JVMTI_ERROR_NULL_POINTER;
    
    *start_location_ptr = (jlocation)m->mcode;
    *end_location_ptr = (jlocation)(m->mcode)+m->mcodelength;
    return JVMTI_ERROR_NONE;
}


/* GetLocalVariableTable *******************************************************

   Return local variable information.

*******************************************************************************/

jvmtiError
GetLocalVariableTable (jvmtiEnv * env, jmethodID method,
		       jint * entry_count_ptr,
		       jvmtiLocalVariableEntry ** table_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");

    return JVMTI_ERROR_NONE;
}


/* GetBytecode *****************************************************************

   For the method indicated by method, return the byte codes that implement the 
   method.

*******************************************************************************/

jvmtiError
GetBytecodes (jvmtiEnv * env, jmethodID method,
	      jint * bytecode_count_ptr, unsigned char **bytecodes_ptr)
{
    methodinfo* m = (methodinfo*)method;;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((method == NULL) || (bytecode_count_ptr == NULL) || 
        (bytecodes_ptr == NULL)) return JVMTI_ERROR_NULL_POINTER;

    *bytecode_count_ptr = m->jcodelength;
    *bytecodes_ptr = (unsigned char*)heap_allocate(m->jcodelength,true,NULL);
    memcpy(*bytecodes_ptr, m->jcode, m->jcodelength);

    return JVMTI_ERROR_NONE;
}


/* IsMethodNative **************************************************************

   For the method indicated by method, return a value indicating whether the 
   method is a native function

*******************************************************************************/

jvmtiError
IsMethodNative (jvmtiEnv * env, jmethodID method, jboolean * is_native_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
    
    if ((method == NULL)||(is_native_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;    

    if (((methodinfo*)method)->flags & ACC_NATIVE) 
        *is_native_ptr = JNI_TRUE;
    else
        *is_native_ptr = JNI_FALSE;

    return JVMTI_ERROR_NONE;
}


/* IsMethodSynthetic ***********************************************************

   return a value indicating whether the method is synthetic. Synthetic methods 
   are generated by the compiler but not present in the original source code.

*******************************************************************************/

jvmtiError
IsMethodSynthetic (jvmtiEnv * env, jmethodID method,
		   jboolean * is_synthetic_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetLoadedClasses ************************************************************

   Return an array of all classes loaded in the virtual machine.

*******************************************************************************/

jvmtiError
GetLoadedClasses (jvmtiEnv * env, jint * class_count_ptr,
		  jclass ** classes_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if (class_count_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
    if (classes_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetClassLoaderClasses *******************************************************

   Returns an array of those classes for which this class loader has been 
   recorded as an initiating loader.

*******************************************************************************/

jvmtiError
GetClassLoaderClasses (jvmtiEnv * env, jobject initiating_loader,
		       jint * class_count_ptr, jclass ** classes_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if (class_count_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
    if (classes_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
        
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
PopFrame (jvmtiEnv * env, jthread thread)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
RedefineClasses (jvmtiEnv * env, jint class_count,
		 const jvmtiClassDefinition * class_definitions)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetVersionNumber ***********************************************************

   Return the JVM TI version identifier.   

*******************************************************************************/

jvmtiError
GetVersionNumber (jvmtiEnv * env, jint * version_ptr)
{
    if (version_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;

    *version_ptr = JVMTI_VERSION_1_0;
    
    return JVMTI_ERROR_NONE;
}


/* GetCapabilities ************************************************************

   Returns via capabilities_ptr the optional JVM TI features which this 
   environment currently possesses.

*******************************************************************************/

jvmtiError
GetCapabilities (jvmtiEnv * env, jvmtiCapabilities * capabilities_ptr)
{
    if (capabilities_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;

    memcpy(capabilities_ptr, &(((environment*) env)->capabilities), sizeof(JVMTI_Capabilities));

    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetSourceDebugExtension (jvmtiEnv * env, jclass klass,
			 char **source_debug_extension_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* IsMethodObsolete ************************************************************

   Determine if a method ID refers to an obsolete method version. 

*******************************************************************************/

jvmtiError
IsMethodObsolete (jvmtiEnv * env, jmethodID method,
		  jboolean * is_obsolete_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SuspendThreadList (jvmtiEnv * env, jint request_count,
		   const jthread * request_list, jvmtiError * results)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ResumeThreadList (jvmtiEnv * env, jint request_count,
		  const jthread * request_list, jvmtiError * results)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetAllStackTraces (jvmtiEnv * env, jint max_frame_count,
		   jvmtiStackInfo ** stack_info_ptr, jint * thread_count_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadListStackTraces (jvmtiEnv * env, jint thread_count,
			  const jthread * thread_list,
			  jint max_frame_count,
			  jvmtiStackInfo ** stack_info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadLocalStorage (jvmtiEnv * env, jthread thread, void **data_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetThreadLocalStorage (jvmtiEnv * env, jthread thread, const void *data)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetStackTrace (jvmtiEnv * env, jthread thread, jint start_depth,
	       jint max_frame_count, jvmtiFrameInfo * frame_buffer,
	       jint * count_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetTag (jvmtiEnv * env, jobject object, jlong * tag_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
SetTag (jvmtiEnv * env, jobject object, jlong tag)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
ForceGarbageCollection (jvmtiEnv * env)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
IterateOverObjectsReachableFromObject (jvmtiEnv * env, jobject object,
				       jvmtiObjectReferenceCallback
				       object_reference_callback,
				       void *user_data)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
IterateOverReachableObjects (jvmtiEnv * env, jvmtiHeapRootCallback
			     heap_root_callback,
			     jvmtiStackReferenceCallback
			     stack_ref_callback,
			     jvmtiObjectReferenceCallback
			     object_ref_callback, void *user_data)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
IterateOverHeap (jvmtiEnv * env, jvmtiHeapObjectFilter object_filter,
		 jvmtiHeapObjectCallback heap_object_callback,
		 void *user_data)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
IterateOverInstancesOfClass (jvmtiEnv * env, jclass klass,
			     jvmtiHeapObjectFilter object_filter,
			     jvmtiHeapObjectCallback
			     heap_object_callback, void *user_data)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetObjectsWithTags (jvmtiEnv * env, jint tag_count, const jlong * tags,
		    jint * count_ptr, jobject ** object_result_ptr,
		    jlong ** tag_result_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* SetJNIFunctionTable **********************************************************

   Set the JNI function table in all current and future JNI environments

*******************************************************************************/

jvmtiError
SetJNIFunctionTable (jvmtiEnv * env,
		     const jniNativeInterface * function_table)
{ 
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;;
    
    if (function_table == NULL) return JVMTI_ERROR_NULL_POINTER;
    ptr_env = (void*)heap_allocate(sizeof(jniNativeInterface),true,NULL);
    memcpy(ptr_env, function_table, sizeof(jniNativeInterface));
    return JVMTI_ERROR_NONE;
}


/* GetJNIFunctionTable *********************************************************

   Get the JNI function table. The JNI function table is copied into allocated 
   memory.

*******************************************************************************/

jvmtiError
GetJNIFunctionTable (jvmtiEnv * env, jniNativeInterface ** function_table)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if (function_table == NULL) return JVMTI_ERROR_NULL_POINTER;
    *function_table = (jniNativeInterface*)
        heap_allocate(sizeof(jniNativeInterface),true,NULL);
    memcpy(*function_table, ptr_env, sizeof(jniNativeInterface));
    return JVMTI_ERROR_NONE;
}


/* SetEventCallbacks **********************************************************

   Set the functions to be called for each event. The callbacks are specified 
   by supplying a replacement function table.

*******************************************************************************/

jvmtiError
SetEventCallbacks (jvmtiEnv * env,
		   const jvmtiEventCallbacks * callbacks,
		   jint size_of_callbacks)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if (size_of_callbacks < 0) return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    
    if (callbacks == NULL) { /* remove the existing callbacks */
        memset(&(((environment* )env)->callbacks), 0, sizeof(jvmtiEventCallbacks));
    }

    memcpy (&(((environment* )env)->callbacks),callbacks,size_of_callbacks);

    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GenerateEvents (jvmtiEnv * env, jvmtiEvent event_type)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* GetExtensionFunctions ******************************************************

   Returns the set of extension functions.

*******************************************************************************/

jvmtiError
GetExtensionFunctions (jvmtiEnv * env, jint * extension_count_ptr,
		       jvmtiExtensionFunctionInfo ** extensions)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((extension_count_ptr== NULL)||(extensions == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    /* cacao has no extended functions yet */
    *extension_count_ptr = 0;

    return JVMTI_ERROR_NONE;
}


/* GetExtensionEvents *********************************************************

   Returns the set of extension events.

*******************************************************************************/

jvmtiError
GetExtensionEvents (jvmtiEnv * env, jint * extension_count_ptr,
		    jvmtiExtensionEventInfo ** extensions)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((extension_count_ptr== NULL)||(extensions == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    /* cacao has no extended events yet */
    *extension_count_ptr = 0;

    return JVMTI_ERROR_NONE;
}


/* SetExtensionEventCallback **************************************************

   Sets the callback function for an extension event and enables the event.

*******************************************************************************/

jvmtiError
SetExtensionEventCallback (jvmtiEnv * env, jint extension_event_index,
			   jvmtiExtensionEvent callback)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    /* cacao has no extended events yet */
    return JVMTI_ERROR_ILLEGAL_ARGUMENT;
}


/* DisposeEnvironment **********************************************************

   Shutdown a JVM TI connection created with JNI GetEnv.

*******************************************************************************/

jvmtiError
DisposeEnvironment (jvmtiEnv * env)
{
    ((environment* )env)->events = NULL;
    ((environment* )env)->EnvironmentLocalStorage = NULL;
    /* let Boehm GC do the rest */
    return JVMTI_ERROR_NONE;
}


/* GetErrorName ***************************************************************

   Return the symbolic name for an error code.

*******************************************************************************/

#define COPY_RESPONSE(name_ptr,str) *name_ptr = (char*) heap_allocate(sizeof(str),true,NULL); \
                                    memcpy(*name_ptr, &str, sizeof(str)); \
                                    break

jvmtiError
GetErrorName (jvmtiEnv * env, jvmtiError error, char **name_ptr)
{
    if (name_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;

    switch (error) {
    case JVMTI_ERROR_NONE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NONE");
    case JVMTI_ERROR_NULL_POINTER : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NULL_POINTER"); 
    case JVMTI_ERROR_OUT_OF_MEMORY : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_OUT_OF_MEMORY");
    case JVMTI_ERROR_ACCESS_DENIED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_ACCESS_DENIED");
    case JVMTI_ERROR_UNATTACHED_THREAD : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNATTACHED_THREAD");
    case JVMTI_ERROR_INVALID_ENVIRONMENT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_ENVIRONMENT"); 
    case JVMTI_ERROR_WRONG_PHASE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_WRONG_PHASE");
    case JVMTI_ERROR_INTERNAL : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INTERNAL");
    case JVMTI_ERROR_INVALID_PRIORITY : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_PRIORITY");
    case JVMTI_ERROR_THREAD_NOT_SUSPENDED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_THREAD_NOT_SUSPENDED");
    case JVMTI_ERROR_THREAD_SUSPENDED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_THREAD_SUSPENDED");
    case JVMTI_ERROR_THREAD_NOT_ALIVE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_THREAD_NOT_ALIVE");
    case JVMTI_ERROR_CLASS_NOT_PREPARED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_CLASS_NOT_PREPARED");
    case JVMTI_ERROR_NO_MORE_FRAMES : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NO_MORE_FRAMES");
    case JVMTI_ERROR_OPAQUE_FRAME : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_OPAQUE_FRAME");
    case JVMTI_ERROR_DUPLICATE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_DUPLICATE");
    case JVMTI_ERROR_NOT_FOUND : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NOT_FOUND");
    case JVMTI_ERROR_NOT_MONITOR_OWNER : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NOT_MONITOR_OWNER");
    case JVMTI_ERROR_INTERRUPT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INTERRUPT");
    case JVMTI_ERROR_UNMODIFIABLE_CLASS : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNMODIFIABLE_CLASS");
    case JVMTI_ERROR_NOT_AVAILABLE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NOT_AVAILABLE");
    case JVMTI_ERROR_ABSENT_INFORMATION : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_ABSENT_INFORMATION");
    case JVMTI_ERROR_INVALID_EVENT_TYPE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_EVENT_TYPE");
    case JVMTI_ERROR_NATIVE_METHOD : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NATIVE_METHOD");
    case JVMTI_ERROR_INVALID_THREAD : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_THREAD");
    case JVMTI_ERROR_INVALID_FIELDID : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_FIELDID");
    case JVMTI_ERROR_INVALID_METHODID : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_METHODID");
    case JVMTI_ERROR_INVALID_LOCATION : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_LOCATION");
    case JVMTI_ERROR_INVALID_OBJECT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_OBJECT");
    case JVMTI_ERROR_INVALID_CLASS : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_CLASS");
    case JVMTI_ERROR_TYPE_MISMATCH : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_TYPE_MISMATCH");
    case JVMTI_ERROR_INVALID_SLOT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_SLOT");
    case JVMTI_ERROR_MUST_POSSESS_CAPABILITY : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_MUST_POSSESS_CAPABILITY");
    case JVMTI_ERROR_INVALID_THREAD_GROUP : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_THREAD_GROUP");
    case JVMTI_ERROR_INVALID_MONITOR : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_MONITOR");
    case JVMTI_ERROR_ILLEGAL_ARGUMENT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_ILLEGAL_ARGUMENT");
    case JVMTI_ERROR_INVALID_TYPESTATE : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_TYPESTATE");
    case JVMTI_ERROR_UNSUPPORTED_VERSION : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_VERSION");
    case JVMTI_ERROR_INVALID_CLASS_FORMAT : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_INVALID_CLASS_FORMAT");
    case JVMTI_ERROR_CIRCULAR_CLASS_DEFINITION : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_CIRCULAR_CLASS_DEFINITION");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_ADDED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_ADDED");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED");
    case JVMTI_ERROR_FAILS_VERIFICATION : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_FAILS_VERIFICATION");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_HIERARCHY_CHANGED");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_DELETED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_DELETED");
    case JVMTI_ERROR_NAMES_DONT_MATCH : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_NAMES_DONT_MATCH");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_CLASS_MODIFIERS_CHANGED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_CLASS_MODIFIERS_CHANGED");
    case JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_MODIFIERS_CHANGED : 
        COPY_RESPONSE (name_ptr, "JVMTI_ERROR_UNSUPPORTED_REDEFINITION_METHOD_MODIFIERS_CHANGED");
    default:
        return JVMTI_ERROR_ILLEGAL_ARGUMENT;
    }
    return JVMTI_ERROR_NONE;
}

/* GetJLocationFormat **********************************************************

   This function describes the representation of jlocation used in this VM.

*******************************************************************************/

jvmtiError
GetJLocationFormat (jvmtiEnv * env, jvmtiJlocationFormat * format_ptr)
{
    *format_ptr = JVMTI_JLOCATION_MACHINEPC;
    return JVMTI_ERROR_NONE;
}


/* GetSystemProperties ********************************************************

   The list of VM system property keys which may be used with GetSystemProperty 
   is returned.

*******************************************************************************/

jvmtiError
GetSystemProperties (jvmtiEnv * env, jint * count_ptr, char ***property_ptr)
{
    jmethodID mid;
    jmethodID moremid;
    classinfo *sysclass, *propclass, *enumclass;
    java_objectheader *sysprop, *keys, *obj;
    char* ch;
    int i;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((count_ptr == NULL) || (property_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    sysclass = load_class_from_sysloader(
        utf_new_char_classname ("java/lang/System"));

    if (!sysclass) throw_main_exception_exit();

    mid = class_resolvemethod(sysclass, 
                              utf_new_char("getProperties"),
                              utf_new_char("()Ljava/util/Properties;"));
    if (!mid) throw_main_exception_exit();


    sysprop = asm_calljavafunction(mid, sysclass, NULL, NULL, NULL);
    if (!sysprop) throw_main_exception_exit();

    propclass = sysprop->vftbl->class;

    mid = class_resolvemethod(propclass, 
                              utf_new_char("size"),
                              utf_new_char("()I"));
    if (!mid) throw_main_exception_exit();

    *count_ptr = 
        JNI_JNIEnvTable.CallIntMethod(NULL, sysprop, mid);
    *property_ptr = heap_allocate(sizeof(char*) * (*count_ptr) ,true,NULL);

    mid = class_resolvemethod(propclass, 
                              utf_new_char("keys"),
                              utf_new_char("()Ljava/util/Enumeration;"));
    if (!mid) throw_main_exception_exit();

    keys = JNI_JNIEnvTable.CallObjectMethod(NULL, sysprop, mid);
    enumclass = keys->vftbl->class;
        
    moremid = class_resolvemethod(enumclass, 
                                  utf_new_char("hasMoreElements"),
                                  utf_new_char("()Z"));
    if (!moremid) throw_main_exception_exit();

    mid = class_resolvemethod(propclass, 
                              utf_new_char("nextElement"),
                              utf_new_char("()Ljava/lang/Object;"));
    if (!mid) throw_main_exception_exit();

    i = 0;
    while (JNI_JNIEnvTable.CallBooleanMethod(NULL,keys,(jmethodID)moremid)) {
        obj = JNI_JNIEnvTable.CallObjectMethod(NULL, keys, mid);
        ch = javastring_tochar(obj);
        *property_ptr[i] = heap_allocate(sizeof(char*) * strlen (ch),true,NULL);
        memcpy(*property_ptr[i], ch, strlen (ch));
        MFREE(ch,char,strlen(ch)+1);
        i++;
    }

    return JVMTI_ERROR_NONE;
}


/* GetSystemProperty **********************************************************

   Return a VM system property value given the property key.

*******************************************************************************/

jvmtiError
GetSystemProperty (jvmtiEnv * env, const char *property, char **value_ptr)
{
    jmethodID mid;
    classinfo *sysclass, *propclass;
    java_objectheader *sysprop, *obj;
    char* ch;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;

    if ((value_ptr == NULL) || (property == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    sysclass = load_class_from_sysloader(utf_new_char("java/lang/System"));
    if (!sysclass) throw_main_exception_exit();

    mid = class_resolvemethod(sysclass, 
                              utf_new_char("getProperties"),
                              utf_new_char("()Ljava/util/Properties;"));
    if (!mid) throw_main_exception_exit();

    sysprop = JNI_JNIEnvTable.CallStaticObjectMethod(NULL, (jclass)sysclass, mid);

    propclass = sysprop->vftbl->class;

    mid = class_resolvemethod(propclass, 
                              utf_new_char("getProperty"),
                              utf_new_char("(Ljava/lang/String;)Ljava/lang/String;"));
    if (!mid) throw_main_exception_exit();

    obj = (java_objectheader*)JNI_JNIEnvTable.CallObjectMethod(
        NULL, sysprop, mid, javastring_new_char(property));
    if (!obj) return JVMTI_ERROR_NOT_AVAILABLE;

    ch = javastring_tochar(obj);
    *value_ptr = heap_allocate(sizeof(char*) * strlen (ch),true,NULL);
    memcpy(*value_ptr, ch, strlen (ch));
    MFREE(ch,char,strlen(ch)+1);       

    return JVMTI_ERROR_NONE;
}


/* SetSystemProperty **********************************************************

   Set a VM system property value.

*******************************************************************************/

jvmtiError
SetSystemProperty (jvmtiEnv * env, const char *property, const char *value)
{
    jmethodID mid;
    classinfo *sysclass, *propclass;
    java_objectheader *sysprop;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE_END;
        
    if (property == NULL) return JVMTI_ERROR_NULL_POINTER;
    if (value == NULL) return JVMTI_ERROR_NOT_AVAILABLE;

    sysclass = load_class_from_sysloader(utf_new_char("java/lang/System"));
    if (!sysclass) throw_main_exception_exit();

    mid = class_resolvemethod(sysclass, 
                              utf_new_char("getProperties"),
                              utf_new_char("()Ljava/util/Properties;"));
    if (!mid) throw_main_exception_exit();

    sysprop = JNI_JNIEnvTable.CallStaticObjectMethod(NULL, (jclass)sysclass, mid);

    propclass = sysprop->vftbl->class;

    mid = class_resolvemethod(propclass, 
                              utf_new_char("setProperty"),
                              utf_new_char("(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;"));
    if (!mid) throw_main_exception_exit();

    JNI_JNIEnvTable.CallObjectMethod(
        NULL, sysprop, mid, javastring_new_char(property),javastring_new_char(value));
    
    return JVMTI_ERROR_NONE;
}

/* GetPhase ********************************************************************

   Return the current phase of VM execution

*******************************************************************************/

jvmtiError
GetPhase (jvmtiEnv * env, jvmtiPhase * phase_ptr)
{
    if (phase_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;
    
    *phase_ptr = phase;

    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetCurrentThreadCpuTimerInfo (jvmtiEnv * env, jvmtiTimerInfo * info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetCurrentThreadCpuTime (jvmtiEnv * env, jlong * nanos_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadCpuTimerInfo (jvmtiEnv * env, jvmtiTimerInfo * info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetThreadCpuTime (jvmtiEnv * env, jthread thread, jlong * nanos_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetTimerInfo (jvmtiEnv * env, jvmtiTimerInfo * info_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* GetTime ********************************************************************

   Return the current value of the system timer, in nanoseconds

*******************************************************************************/

jvmtiError
GetTime (jvmtiEnv * env, jlong * nanos_ptr)
{
    /* Note: this implementation copied directly from Japhar's, by Chris Toshok. */
    struct timeval tp;
    
    if (nanos_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;

    if (gettimeofday (&tp, NULL) == -1)
        JNI_JNIEnvTable.FatalError (NULL, "gettimeofday call failed.");
    
    *nanos_ptr = (jlong) tp.tv_sec;
    *nanos_ptr *= 1000;
    *nanos_ptr += (tp.tv_usec / 1000);

    return JVMTI_ERROR_NONE;
}

/* GetPotentialCapabilities ***************************************************

   Returns the JVM TI features that can potentially be possessed by this 
   environment at this time.

*******************************************************************************/

jvmtiError
GetPotentialCapabilities (jvmtiEnv * env,
			  jvmtiCapabilities * capabilities_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if (capabilities_ptr == NULL) return JVMTI_ERROR_NULL_POINTER;

    memcpy(capabilities_ptr, &JVMTI_Capabilities, sizeof(JVMTI_Capabilities));

    return JVMTI_ERROR_NONE;
}


jvmtiError static capabilityerror() 
{
    return JVMTI_ERROR_MUST_POSSESS_CAPABILITY;
}

#define CHECK_POTENTIAL_AVAILABLE(CAN)      \
        if ((capabilities_ptr->CAN == 1) && \
           (JVMTI_Capabilities.CAN == 0))   \
           return JVMTI_ERROR_NOT_AVAILABLE; 


/* AddCapabilities ************************************************************

   Set new capabilities by adding the capabilities pointed to by 
   capabilities_ptr. All previous capabilities are retained.

*******************************************************************************/

jvmtiError
AddCapabilities (jvmtiEnv * env, const jvmtiCapabilities * capabilities_ptr)
{
    environment* cacao_env;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((env == NULL) || (capabilities_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;
    
    cacao_env = (environment*)env;

    CHECK_POTENTIAL_AVAILABLE(can_tag_objects)
    else {
        cacao_env->capabilities.can_tag_objects = 1;
        env->GetTag = &GetTag;
        env->SetTag = &SetTag;
        env->IterateOverObjectsReachableFromObject = 
            &IterateOverObjectsReachableFromObject;
        env->IterateOverReachableObjects =
            &IterateOverReachableObjects;
        env->IterateOverHeap = &IterateOverHeap;
        env->IterateOverInstancesOfClass = &IterateOverInstancesOfClass;
        env->GetObjectsWithTags = &GetObjectsWithTags;
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_field_modification_events)
    else {
        cacao_env->capabilities.can_generate_field_modification_events = 1;
        env->SetFieldModificationWatch = &SetFieldModificationWatch;
        env->SetEventNotificationMode = &SetEventNotificationMode;
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_field_access_events)
    else {
        cacao_env->capabilities.can_generate_field_access_events = 1;
        env->SetFieldAccessWatch = &SetFieldAccessWatch;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_bytecodes)
    else {
        cacao_env->capabilities.can_get_bytecodes  = 1;
        env->GetBytecodes = &GetBytecodes;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_synthetic_attribute)
    else {
        cacao_env->capabilities.can_get_synthetic_attribute  = 1;
        env->IsFieldSynthetic = &IsFieldSynthetic;
        env->IsMethodSynthetic = &IsMethodSynthetic;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_owned_monitor_info)
    else {
        cacao_env->capabilities.can_get_owned_monitor_info  = 1;
        env->GetOwnedMonitorInfo = &GetOwnedMonitorInfo;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_current_contended_monitor)
    else {
        cacao_env->capabilities.can_get_current_contended_monitor  = 1;
        env->GetCurrentContendedMonitor = &GetCurrentContendedMonitor;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_monitor_info)
    else {
        cacao_env->capabilities.can_get_monitor_info  = 1;
        env->GetObjectMonitorUsage  = &GetObjectMonitorUsage;
    }

    CHECK_POTENTIAL_AVAILABLE(can_pop_frame)
    else {
        cacao_env->capabilities.can_pop_frame  = 1;
        env->PopFrame  = &PopFrame;
    }

    CHECK_POTENTIAL_AVAILABLE(can_redefine_classes)
    else {
        cacao_env->capabilities.can_redefine_classes  = 1;
        env->RedefineClasses = &RedefineClasses;
    }

    CHECK_POTENTIAL_AVAILABLE(can_signal_thread)
    else {
        cacao_env->capabilities.can_signal_thread  = 1;
        env->StopThread = &StopThread;
        env->InterruptThread = &InterruptThread;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_source_file_name)
    else {
        cacao_env->capabilities.can_get_source_file_name  = 1;
        env->GetSourceFileName = &GetSourceFileName;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_line_numbers)
    else {
        cacao_env->capabilities.can_get_line_numbers  = 1;
        env->GetLineNumberTable = &GetLineNumberTable;
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_source_debug_extension)
    else {
        cacao_env->capabilities.can_get_source_debug_extension  = 1;
        env->GetSourceDebugExtension = &GetSourceDebugExtension;
    }

    CHECK_POTENTIAL_AVAILABLE(can_access_local_variables)
    else {
        cacao_env->capabilities.can_access_local_variables  = 1;
        env->GetLocalObject = &GetLocalObject;
        env->GetLocalInt = &GetLocalInt;
        env->GetLocalLong = &GetLocalLong;
        env->GetLocalFloat = &GetLocalFloat;
        env->GetLocalDouble = &GetLocalDouble;
        env->SetLocalObject = &SetLocalObject;
        env->SetLocalInt = &SetLocalInt;
        env->SetLocalLong = &SetLocalLong;
        env->SetLocalFloat = &SetLocalFloat;
        env->SetLocalDouble = &SetLocalDouble;
        env->GetLocalVariableTable = &GetLocalVariableTable;
    }

    CHECK_POTENTIAL_AVAILABLE(can_maintain_original_method_order)
    else {
        cacao_env->capabilities.can_maintain_original_method_order  = 1;
        env->GetClassMethods  = &GetClassMethods;
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_single_step_events)
    else {
        cacao_env->capabilities.can_generate_single_step_events  = 1;
        env->SetEventNotificationMode = &SetEventNotificationMode;
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_exception_events)
    else {
        cacao_env->capabilities.can_generate_exception_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_frame_pop_events)
    else {
        cacao_env->capabilities.can_generate_frame_pop_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_breakpoint_events)
    else {
        cacao_env->capabilities.can_generate_breakpoint_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_suspend)
    else {
        cacao_env->capabilities.can_suspend  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_redefine_any_class)
    else {
        cacao_env->capabilities.can_redefine_any_class  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_current_thread_cpu_time)
    else {
        cacao_env->capabilities.can_get_current_thread_cpu_time  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_get_thread_cpu_time)
    else {
        cacao_env->capabilities.can_get_thread_cpu_time  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_method_entry_events)
    else {
        cacao_env->capabilities.can_generate_method_entry_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_method_exit_events)
    else {
        cacao_env->capabilities.can_generate_method_exit_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_all_class_hook_events)
    else {
        cacao_env->capabilities.can_generate_all_class_hook_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_compiled_method_load_events)
    else {
        cacao_env->capabilities.can_generate_compiled_method_load_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_monitor_events)
    else {
        cacao_env->capabilities.can_generate_monitor_events= 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_vm_object_alloc_events)
    else {
        cacao_env->capabilities.can_generate_vm_object_alloc_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_native_method_bind_events)
    else {
        cacao_env->capabilities.can_generate_native_method_bind_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_garbage_collection_events)
    else {
        cacao_env->capabilities.can_generate_garbage_collection_events  = 1;
        /* env->  = &; */
    }

    CHECK_POTENTIAL_AVAILABLE(can_generate_object_free_events)
    else {
        cacao_env->capabilities.can_generate_object_free_events  = 1;
        /* env->  = &; */
    }
    
    return JVMTI_ERROR_NONE;    
}

/* RelinquishCapabilities *****************************************************

   Relinquish the capabilities pointed to by capabilities_ptr.

*******************************************************************************/

jvmtiError
RelinquishCapabilities (jvmtiEnv * env,
			const jvmtiCapabilities * capabilities_ptr)
{
    environment* cacao_env;
    
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    if ((env == NULL) || (capabilities_ptr == NULL)) 
        return JVMTI_ERROR_NULL_POINTER;

    cacao_env = (environment*)env;
    
    if (capabilities_ptr->can_tag_objects == 1) {
        cacao_env->capabilities.can_tag_objects = 0;
        env->GetTag = &capabilityerror;
        env->SetTag = &capabilityerror;
        env->IterateOverObjectsReachableFromObject = &capabilityerror;
        env->IterateOverReachableObjects = &capabilityerror;          
        env->IterateOverHeap = &capabilityerror;
        env->IterateOverInstancesOfClass = &capabilityerror;
        env->GetObjectsWithTags = &capabilityerror;  
    }

/*    todo if ((capabilities_ptr->  == 1) {
        cacao_env->capabilities.  = 0;
        env->SetFieldModificationWatch = &capabilityerror;
        }*/


    return JVMTI_ERROR_NONE;
}

/* *****************************************************************************

   

*******************************************************************************/

jvmtiError
GetAvailableProcessors (jvmtiEnv * env, jint * processor_count_ptr)
{
      CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
  log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}

/* GetEnvironmentLocalStorage **************************************************

   Called by the agent to get the value of the JVM TI environment-local storage.

*******************************************************************************/

jvmtiError
GetEnvironmentLocalStorage (jvmtiEnv * env, void **data_ptr)
{
    if ((env == NULL) || (data_ptr == NULL)) return JVMTI_ERROR_NULL_POINTER;

    *data_ptr = ((environment*)env)->EnvironmentLocalStorage;

    return JVMTI_ERROR_NONE;
}

/* SetEnvironmentLocalStorage **************************************************

   The VM stores a pointer value associated with each environment. Agents can 
   allocate memory in which they store environment specific information.

*******************************************************************************/

jvmtiError
SetEnvironmentLocalStorage (jvmtiEnv * env, const void *data)
{
    if (env == NULL) return JVMTI_ERROR_NULL_POINTER;

    ((environment*)env)->EnvironmentLocalStorage = data;

    return JVMTI_ERROR_NONE;
}

/* AddToBootstrapClassLoaderSearch ********************************************

   After the bootstrap class loader unsuccessfully searches for a class, the 
   specified platform-dependent search path segment will be searched as well.

*******************************************************************************/

jvmtiError
AddToBootstrapClassLoaderSearch (jvmtiEnv * env, const char *segment)
{
    char* tmp_bcp;
    int ln;

    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_ONLOAD)
    CHECK_PHASE_END;

    if (segment == NULL) return JVMTI_ERROR_NULL_POINTER;

    ln = strlen(bootclasspath) + strlen(":") + strlen(segment);
    tmp_bcp = MNEW(char, ln);
    strcat(tmp_bcp, bootclasspath);
    strcat(tmp_bcp, ":");
    strcat(tmp_bcp, segment);
    MFREE(bootclasspath,char,ln);
    bootclasspath = tmp_bcp;

    return JVMTI_ERROR_NONE;
}

/* SetVerboseFlag *************************************************************

   Control verbose output. This is the output which typically is sent to stderr

*******************************************************************************/

jvmtiError
SetVerboseFlag (jvmtiEnv * env, jvmtiVerboseFlag flag, jboolean value)
{
    switch (flag) {
    case JVMTI_VERBOSE_OTHER: 
        runverbose = value;
        break;
    case JVMTI_VERBOSE_GC: 
        opt_verbosegc = value;
        break;
    case JVMTI_VERBOSE_CLASS: 
        loadverbose = value;
        break;
    case JVMTI_VERBOSE_JNI: 
        break;
    default:
        return JVMTI_ERROR_ILLEGAL_ARGUMENT;            
    }
    return JVMTI_ERROR_NONE;
}

/* GetObjectSize **************************************************************

   For the object indicated by object, return the size of the object. This size 
   is an implementation-specific approximation of the amount of storage consumed 
   by this object.

*******************************************************************************/

jvmtiError
GetObjectSize (jvmtiEnv * env, jobject object, jlong * size_ptr)
{
    CHECK_PHASE_START
    CHECK_PHASE(JVMTI_PHASE_START)
    CHECK_PHASE(JVMTI_PHASE_LIVE)
    CHECK_PHASE_END;
        
    log_text ("JVMTI-Call: IMPLEMENT ME!!!");
    return JVMTI_ERROR_NONE;
}


/* *****************************************************************************

   Environment variables

*******************************************************************************/

jvmtiCapabilities JVMTI_Capabilities = {
  0,				/* can_tag_objects */
  0,				/* can_generate_field_modification_events */
  0,				/* can_generate_field_access_events */
  1,				/* can_get_bytecodes */
  0,				/* can_get_synthetic_attribute */
  0,				/* can_get_owned_monitor_info */
  0,				/* can_get_current_contended_monitor */
  0,				/* can_get_monitor_info */
  0,				/* can_pop_frame */
  0,				/* can_redefine_classes */
  0,				/* can_signal_thread */
  1,				/* can_get_source_file_name */
  1,				/* can_get_line_numbers */
  0,				/* can_get_source_debug_extension */
  0,				/* can_access_local_variables */
  0,				/* can_maintain_original_method_order */
  0,				/* can_generate_single_step_events */
  0,				/* can_generate_exception_events */
  0,				/* can_generate_frame_pop_events */
  0,				/* can_generate_breakpoint_events */
  0,				/* can_suspend */
  0,				/* can_redefine_any_class */
  0,				/* can_get_current_thread_cpu_time */
  0,				/* can_get_thread_cpu_time */
  0,				/* can_generate_method_entry_events */
  0,				/* can_generate_method_exit_events */
  0,				/* can_generate_all_class_hook_events */
  0,				/* can_generate_compiled_method_load_events */
  0,				/* can_generate_monitor_events */
  0,				/* can_generate_vm_object_alloc_events */
  0,				/* can_generate_native_method_bind_events */
  0,				/* can_generate_garbage_collection_events */
  0,				/* can_generate_object_free_events */
};

jvmtiEnv JVMTI_EnvTable = {
    NULL,
    &SetEventNotificationMode,
    NULL,
    &GetAllThreads,
    &SuspendThread,
    &ResumeThread,
    &StopThread,
    &InterruptThread,
    &GetThreadInfo,
    &GetOwnedMonitorInfo,
    &GetCurrentContendedMonitor,
    &RunAgentThread,
    &GetTopThreadGroups,
    &GetThreadGroupInfo,
    &GetThreadGroupChildren,
    &GetFrameCount,
    &GetThreadState,
    NULL,
    &GetFrameLocation,
    &NotifyFramePop,
    &GetLocalObject,
    &GetLocalInt,
    &GetLocalLong,
    &GetLocalFloat,
    &GetLocalDouble,
    &SetLocalObject,
    &SetLocalInt,
    &SetLocalLong,
    &SetLocalFloat,
    &SetLocalDouble,
    &CreateRawMonitor,
    &DestroyRawMonitor,
    &RawMonitorEnter,
    &RawMonitorExit,
    &RawMonitorWait,
    &RawMonitorNotify,
    &RawMonitorNotifyAll,
    &SetBreakpoint,
    &ClearBreakpoint,
    NULL,
    &SetFieldAccessWatch,
    &ClearFieldAccessWatch,
    &SetFieldModificationWatch,
    &ClearFieldModificationWatch,
    NULL,
    &Allocate,
    &Deallocate,
    &GetClassSignature,
    &GetClassStatus,
    &GetSourceFileName,
    &GetClassModifiers,
    &GetClassMethods,
    &GetClassFields,
    &GetImplementedInterfaces,
    &IsInterface,
    &IsArrayClass,
    &GetClassLoader, 
    &GetObjectHashCode, 
    &GetObjectMonitorUsage, 
    &GetFieldName, 
    &GetFieldDeclaringClass, 
    &GetFieldModifiers, 
    &IsFieldSynthetic, 
    &GetMethodName, 
    &GetMethodDeclaringClass, 
    &GetMethodModifiers, 
    NULL,
    &GetMaxLocals, 
    &GetArgumentsSize, 
    &GetLineNumberTable, 
    &GetMethodLocation, 
    &GetLocalVariableTable, 
    NULL,
    NULL,
    &GetBytecodes, 
    &IsMethodNative, 
    &IsMethodSynthetic, 
    &GetLoadedClasses, 
    &GetClassLoaderClasses, 
    &PopFrame, 
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &RedefineClasses, 
    &GetVersionNumber, 
    &GetCapabilities, 
    &GetSourceDebugExtension, 
    &IsMethodObsolete, 
    &SuspendThreadList, 
    &ResumeThreadList, 
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &GetAllStackTraces, 
    &GetThreadListStackTraces, 
    &GetThreadLocalStorage, 
    &SetThreadLocalStorage, 
    &GetStackTrace, 
    NULL,
    &GetTag, 
    &SetTag, 
    &ForceGarbageCollection,
    &IterateOverObjectsReachableFromObject, 
    &IterateOverReachableObjects, 
    &IterateOverHeap, 
    &IterateOverInstancesOfClass, 
    NULL,
    &GetObjectsWithTags, 
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &SetJNIFunctionTable, 
    &GetJNIFunctionTable, 
    &SetEventCallbacks, 
    &GenerateEvents, 
    &GetExtensionFunctions, 
    &GetExtensionEvents, 
    &SetExtensionEventCallback, 
    &DisposeEnvironment,
    &GetErrorName, 
    &GetJLocationFormat, 
    &GetSystemProperties, 
    &GetSystemProperty, 
    &SetSystemProperty, 
    &GetPhase, 
    &GetCurrentThreadCpuTimerInfo, 
    &GetCurrentThreadCpuTime, 
    &GetThreadCpuTimerInfo, 
    &GetThreadCpuTime, 
    &GetTimerInfo, 
    &GetTime, 
    &GetPotentialCapabilities, 
    NULL,
    &AddCapabilities,
    &RelinquishCapabilities,
    &GetAvailableProcessors,
    NULL,
    NULL,
    &GetEnvironmentLocalStorage,
    &SetEnvironmentLocalStorage,
    &AddToBootstrapClassLoaderSearch,
    &SetVerboseFlag,
    NULL,
    NULL,
    NULL,
    &GetObjectSize
};

void jvmti_init() {
    ihmclass = load_class_from_sysloader(
        utf_new_char_classname ("java/util/IdentityHashMap"));
    if (ihmclass == NULL) {
        log_text("JVMTI-Init: unable to find java.util.IdentityHashMap");
    }
    
    ihmmid = class_resolvemethod(ihmclass, 
                            utf_new_char("<init>"), 
                            utf_new_char("()V"));
    if (ihmmid == NULL) {
        log_text("JVMTI-Init: unable to find constructor in java.util.IdentityHashMap");
    }
}

void set_jvmti_phase(jvmtiPhase p) {
    phase = p;
    switch (p) {
    case JVMTI_PHASE_ONLOAD:
        /* todo */
        break;
    case JVMTI_PHASE_PRIMORDIAL:
        /* todo */
        break;
    case JVMTI_PHASE_START: 
        /* todo: send VM Start Event*/
        break;
    case JVMTI_PHASE_LIVE: 
        /* todo: send VMInit Event */
        break;
    case JVMTI_PHASE_DEAD:
        /* todo: send VMDeath Event */
        break;
    }
}

jvmtiEnv* new_jvmtienv() {
    environment* env;
    java_objectheader *o;

    env = heap_allocate(sizeof(environment),true,NULL);
    memcpy(&(env->env),&JVMTI_EnvTable,sizeof(jvmtiEnv));
    env->events = (jobject*)builtin_new(ihmclass);
    asm_calljavafunction(ihmmid, o, NULL, NULL, NULL);
    /* To possess a capability, the agent must add the capability.*/
    memset(&(env->capabilities), 1, sizeof(jvmtiCapabilities));
    RelinquishCapabilities(&(env->env),&(env->capabilities));
    env->EnvironmentLocalStorage = NULL;
    return &(env->env);
}

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
