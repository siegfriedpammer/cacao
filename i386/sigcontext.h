#ifndef _ASMAXP_SIGCONTEXT_H
#define _ASMAXP_SIGCONTEXT_H

struct sigcontext_struct {

	/*
	* what should we have here? I'd probably better use the same
	* stack layout as OSF/1, just in case we ever want to try
	* running their binaries.. 
	*
	* This is the basic layout, but I don't know if we'll ever
	* actually fill in all the values..
	*/

	long          sc_onstack;    /* sigstack state to restore       */
	long          sc_mask;       /* signal mask to restore          */
	long          sc_pc;         /* pc at time of signal            */
	long          sc_ps;         /* psl to retore                   */
	long          sc_regs[32];   /* processor regs 0 to 31          */
	long          sc_ownedfp;    /* fp has been used                */
	long          sc_fpregs[32]; /* fp regs 0 to 31                 */
	unsigned long sc_fpcr;       /* floating point control register */
	unsigned long sc_fp_control; /* software fpcr                   */
	                             /* rest is unused                  */
	unsigned long sc_reserved1, sc_reserved2;
	unsigned long sc_ssize;
	char          *sc_sbase;
	unsigned long sc_traparg_a0;
	unsigned long sc_traparg_a1;
	unsigned long sc_traparg_a2;
	unsigned long sc_fp_trap_pc;
	unsigned long sc_fp_trigger_sum;
	unsigned long sc_fp_trigger_inst;
	unsigned long sc_retcode[2];
};

#endif
