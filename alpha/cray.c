/****************************** asmpart.c **************************************
*                                                                              *
*   is an assembly language file, but called .c to fake the preprocessor.      *
*   It contains the Java-C interace functions for Alpha processors.           *
*                                                                              *
*   Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst              *
*                                                                              *
*   See file COPYRIGHT for information on usage and disclaimer of warranties   *
*                                                                              *
*   Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at            *
*            Andreas  Krall      EMAIL: cacao@complang.tuwien.ac.at            *
*                                                                              *
*   Last Change: 1997/04/26                                                    *
*                                                                              *
*******************************************************************************/

#define v0      r0

#define t0      r1
#define t1      r2
#define t2      r3
#define t3      r4
#define t4      r5
#define t5      r6
#define t6      r7
#define t7      r8

#define	s0      r9
#define	s1      r10
#define	s2      r11
#define	s3      r12
#define	s4      r13
#define	s5      r14
#define	s6      r15

#define a0      r16
#define a1      r17
#define a2      r18
#define a3      r19
#define a4      r20
#define a5      r21

#define t8      r22
#define t9      r23
#define t10     r24
#define t11     r25
#define ra      r26
#define t12     r27

#define pv      t12
#define AT      .
#define gp      r29
#define sp      r30
#define zero    r31

#define PAL_imb 134


/*	.set    noat
	.set    noreorder
*/

	
/********************* exported functions and variables ***********************/

/*	.globl has_no_x_instr_set
	.globl synchronize_caches
	.globl asm_calljavamethod
	.globl asm_call_jit_compiler
	.globl asm_dumpregistersandcall
*/

/*************************** imported functions *******************************/

/*	.globl  compiler_compile
*/

/*********************** function has_no_x_instr_set ***************************
*                                                                              *
*   determines if the byte support instruction set (21164a and higher)         *
*   is available.                                                              *
*                                                                              *
*******************************************************************************/
	zero <- t0
	
	.ident    has_no_x_instr_set
	.psect code_sec, code
has_no_x_instr_set::

/* 	.long   0x47e03c20                  ; amask   1,r0
*/

	zap   r0,1,r0

	jmp     zero,(ra)                   ; return

	.end    has_no_x_instr_set


/********************* function synchronize_caches ****************************/

	.ident    synchronize_caches
	.psect code_sec, code
synchronize_caches::

	call_pal PAL_imb                    ; synchronise instruction cache
	jmp     zero,(ra)                   ; return

	.end    synchronize_caches



/********************* function asm_calljavamethod *****************************
*                                                                              *
*   This function calls a Java-method (which possibly needs compilation)       *
*   with up to 4 parameters.                                                   *
*                                                                              *
*   This functions calls the JIT-compiler which eventually translates the      *
*   method into machine code.                                                  *
*                                                                              *
*   An possibly throwed exception will be returned to the caller as function   *
*   return value, so the java method cannot return a fucntion value (this      *
*   function usually calls 'main' and '<clinit>' which do not return a         *
*   function value).                                                           *
*                                                                              *
*   C-prototype:                                                               *
*    javaobject_header *asm_calljavamethod (methodinfo *m,                     *
*         void *arg1, void *arg2, void *arg3, void *arg4);                     *
*                                                                              *
*******************************************************************************/

	.ident    asm_calljavamethod
	.psect codesect3, code
	.external asm_call_jit_compiler
	
asm_calljavamethod::

;	ldgp    gp,0(pv)
;	.prologue  1
	lda     sp,-24(sp)                  ; allocate stack space
	stq     ra,0(sp)                    ; save return address

	stq     r16,16(sp)                  ; save method pointer for compiler
	lda     r0,16(sp)                   ; pass pointer to method pointer via r0

	bis     r17,r17,r16                 ; pass the remaining parameters
	bis     r18,r18,r17
	bis     r19,r19,r18
	bis     r20,r20,r19

	la     r28,asm_call_jit_compiler   ; fake virtual function call
	; Changed!! from lda
	
	stq     r28,8(sp)                   ; store function address
	bis     sp,sp,r28                   ; set method pointer

	ldq     pv,8(r28)                   ; method call as in Java
	jmp     ra,(pv)                     ; call JIT compiler

	ldq     ra,0(sp)                    ; restore return address
	lda     sp,24(sp)                   ; free stack space

	bis     r1,r1,r0                    ; pass exception to caller (C)
	jmp     zero,(ra)

	.end    asm_calljavamethod


/****************** function asm_call_jit_compiler *****************************
*                                                                              *
*   invokes the compiler for untranslated JavaVM methods.                      *
*                                                                              *
*   Register R0 contains a pointer to the method info structure (prepared      *
*   by createcompilerstub). Using the return address in R26 and the            *
*   offset in the LDA instruction or using the value in methodptr R28 the      *
*   patching address for storing the method address can be computed:           *
*                                                                              *
*   method address was either loaded using                                     *
*   M_LDQ (REG_PV, REG_PV, a)        ; invokestatic/special    (r27)           *
*   M_LDA (REG_PV, REG_RA, low)                                                *
*   M_LDAH(REG_PV, REG_RA, high)     ; optional                                *
*   or                                                                         *
*   M_LDQ (REG_PV, REG_METHODPTR, m) ; invokevirtual/interace (r28)           *
*   in the static case the method pointer can be computed using the            *
*   return address and the lda function following the jmp instruction          *
*                                                                              *
*******************************************************************************/


	.ident    asm_call_jit_compiler
	.psect code_sec1, code
	.external compiler_compile

asm_call_jit_compiler::

; 	ldgp    gp,0(pv)
;	.prologue  1
	ldl     r22,-8(ra)              ; load instruction LDQ PV,xxx(ryy)
	srl     r22,16,r22              ; shift right register number ryy
	and     r22,31,r22              ; isolate register number
	subl    r22,28,r22              ; test for REG_METHODPTR
	beq     r22,noregchange       
	ldl     r22,0(ra)               ; load instruction LDA PV,xxx(RA)
	sll     r22,48,r22
	sra     r22,48,r22              ; isolate offset
	addq    r22,ra,r28              ; compute update address
	ldl     r22,4(ra)               ; load instruction LDAH PV,xxx(PV)
	srl     r22,16,r22              ; isolate instruction code
	lda     r22,-0x177b(r22)        ; test for LDAH
	bne     r22,noregchange       
	ldl     r22,0(ra)               ; load instruction LDA PV,xxx(RA)
	sll     r22,16,r22              ; compute high offset
	addl    r22,0,r22               ; sign extend high offset
	addq    r22,r28,r28             ; compute update address
noregchange:
	lda     sp,-14*8(sp)            ; reserve stack space
	stq     r16,0*8(sp)             ; save all argument registers
	stq     r17,1*8(sp)             ; they could be used by method
	stq     r18,2*8(sp)
	stq     r19,3*8(sp)
	stq     r20,4*8(sp)
	stq     r21,5*8(sp)
	stt     f16,6*8(sp)
	stt     f17,7*8(sp)
	stt     f18,8*8(sp)
	stt     f19,9*8(sp)
	stt     f20,10*8(sp)
	stt     f21,11*8(sp)
	stq     r28,12*8(sp)            ; save method pointer
	stq     ra,13*8(sp)             ; save return address

	ldq     r16,0(r0)               ; pass 'methodinfo' pointer to
	bsr     ra,compiler_compile     ; compiler
; 	ldgp    gp,0(ra)

	call_pal PAL_imb                ; synchronise instruction cache

	ldq     r16,0*8(sp)             ; load argument registers
	ldq     r17,1*8(sp)
	ldq     r18,2*8(sp)
	ldq     r19,3*8(sp)
	ldq     r20,4*8(sp)
	ldq     r21,5*8(sp)
	ldt     f16,6*8(sp)
	ldt     f17,7*8(sp)
	ldt     f18,8*8(sp)
	ldt     f19,9*8(sp)
	ldt     f20,10*8(sp)
	ldt     f21,11*8(sp)
	ldq     r28,12*8(sp)            ; load method pointer
	ldq     ra,13*8(sp)             ; load return address
	lda     sp,14*8(sp)             ; deallocate stack area

	ldl     r22,-8(ra)              ; load instruction LDQ PV,xxx(ryy)
	sll     r22,48,r22
	sra     r22,48,r22              ; isolate offset

	addq    r22,r28,r22             ; compute update address via method pointer
 	stq     r0,0(r22)               ; save new method address there

    bis     r0,r0,pv                ; load method address into pv

	jmp     zero, (pv)              ; and call method. The method returns
	                                ; directly to the caller (ra).
								 
	.end    asm_call_jit_compiler


/****************** function asm_dumpregistersandcall **************************
*                                                                              *
*   This funtion saves all callee saved registers and calls the function       *
*   which is passed as parameter.                                              *
*                                                                              *
*   This function is needed by the garbage collector, which needs to access    *
*   all registers which are stored on the stack. Unused registers are          *
*   cleared to avoid intererances with the GC.                                *
*                                                                              *
*   void asm_dumpregistersandcall (functionptr f);                             *
*                                                                              *
*******************************************************************************/

	.ident    asm_dumpregistersandcall
	.psect code_sec2, code
asm_dumpregistersandcall::
	lda     sp,-16*8(sp)            ; allocate stack
	stq     ra,0(sp)                ; save return address

	stq     r9,1*8(sp)              ; save all callee saved registers
	stq     r10,2*8(sp)             ; intialize the remaining registers
	stq     r11,3*8(sp)
	stq     r12,4*8(sp)
	stq     r13,5*8(sp)
	stq     r14,6*8(sp)
	stq     r15,7*8(sp)
	stt     f2,8*8(sp)
	stt     f3,9*8(sp)
	stt     f4,10*8(sp)
	stt     f5,11*8(sp)
	stt     f6,12*8(sp)
	stt     f7,13*8(sp)
	stt     f8,14*8(sp)
	stt     f9,15*8(sp)

	bis     zero,zero,r0           ; intialize the remaining registers
	bis     zero,zero,r1
	bis     zero,zero,r2
	bis     zero,zero,r3
	bis     zero,zero,r4
	bis     zero,zero,r5
	bis     zero,zero,r6
	bis     zero,zero,r7
	bis     zero,zero,r8
	bis     zero,zero,r17
	bis     zero,zero,r18
	bis     zero,zero,r19
	bis     zero,zero,r20
	bis     zero,zero,r21
	bis     zero,zero,r22
	bis     zero,zero,r23
	bis     zero,zero,r24
	bis     zero,zero,r25
	bis     zero,zero,r26
	bis     zero,zero,r27
	bis     zero,zero,r28
	bis     zero,zero,r29
	cpys    f31,f31,f0
	cpys    f31,f31,f1
	cpys    f31,f31,f10
	cpys    f31,f31,f11
	cpys    f31,f31,f12
	cpys    f31,f31,f13
	cpys    f31,f31,f14
	cpys    f31,f31,f15
	cpys    f31,f31,f16
	cpys    f31,f31,f17
	cpys    f31,f31,f18
	cpys    f31,f31,f19
	cpys    f31,f31,f20
	cpys    f31,f31,f21
	cpys    f31,f31,f22
	cpys    f31,f31,f23
	cpys    f31,f31,f24
	cpys    f31,f31,f25
	cpys    f31,f31,f26
	cpys    f31,f31,f27
	cpys    f31,f31,f28
	cpys    f31,f31,f29
	cpys    f31,f31,f30

	bis     r16,r16,pv              ; load function pointer
	jmp     ra,(pv)                 ; and call function

	ldq     ra,0(sp)                ; load return address
	lda     sp,16*8(sp)             ; deallocate stack
	jmp     zero,(ra)               ; return

	.end    

