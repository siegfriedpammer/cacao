/* ------------------------ thread.c -------------------------- */

/*
 * Put a thread to sleep.
 */
void
sleepThread(int64 time)
{
    thread** tidp;

    /* Sleep for no time */
    if (time == 0) {
	return;
    }
    
    intsDisable();

    /* Get absolute time */
    currentThread->PrivateInfo->time = time + currentTime();

    /* Find place in alarm list */
    for (tidp = &alarmList; (*tidp) != 0; tidp = &(*tidp)->next) {
	if ((*tidp)->PrivateInfo->time > currentThread->PrivateInfo->time) {
	    break;
	}
    }

    /* If I'm head of alarm list, restart alarm */
    if (tidp == &alarmList) {
	MALARM(time);
    }
    
    /* Suspend thread on it */
    suspendOnQThread(currentThread, tidp);
    
    intsRestore();
}

/*
 * Handle alarm.
 * This routine uses a different meaning of "blockInts". Formerly, it was just
 * "don't reschedule if you don't have to". Now it is "don't do ANY
 * rescheduling actions due to an expired timer". An alternative would be to
 * block SIGALARM during critical sections (by means of sigprocmask). But
 * this would be required quite often (for every outmost intsDisable(),
 * intsRestore()) and therefore would be much more expensive than just
 * setting an int flag which - sometimes - might cause an additional
 * setitimer call.
 */
static
void
alarmException(int sig)
{
    thread* tid;
    int64 time;

    /* Re-enable signal - necessary for SysV */
    signal(sig, (SIG_T)alarmException);

    /*
     * If ints are blocked, this might indicate an inconsistent state of
     * one of the thread queues (either alarmList or threadQhead/tail).
     * We better don't touch one of them in this case and come back later.
     */
    if (blockInts > 0) {
	MALARM(50);
	return;
    }

    intsDisable();

    /* Wake all the threads which need waking */
    time = currentTime();
    while (alarmList != 0 && alarmList->PrivateInfo->time <= time) {
	tid = alarmList;
	alarmList = alarmList->next;
	iresumeThread(tid);
    }

    /* Restart alarm */
    if (alarmList != 0) {
	MALARM(alarmList->PrivateInfo->time - time);
    }

    /*
     * The next bit is rather tricky.  If we don't reschedule then things
     * are fine, we exit this handler and everything continues correctly.
     * On the otherhand, if we do reschedule, we will schedule the new
     * thread with alarms blocked which is wrong.  However, we cannot
     * unblock them here incase we have just set an alarm which goes
     * off before the reschedule takes place (and we enter this routine
     * recusively which isn't good).  So, we set a flag indicating alarms
     * are blocked, and allow the rescheduler to unblock the alarm signal
     * after the context switch has been made.  At this point it's safe.
     */
    alarmBlocked = true;
    intsRestore();
    alarmBlocked = false;
}

/*
 * How many stack frames have I invoked?
 */
long
framesThread(thread* tid)
{
    long count;
    THREADFRAMES(tid, count);
    return (count);
}
