.class public test_verify_fail_backward_with_new_on_stack
.super java/lang/Object

; ======================================================================

.method public <init>()V
   aload_0
   invokenonvirtual java/lang/Object/<init>()V
   return
.end method

; ======================================================================

.method public static main([Ljava/lang/String;)V
	.limit stack 2
	.limit locals 3

	ldc 1
	istore 1

	aconst_null

backward:
	pop
	new test_verify_fail_backward_with_new_in_local

	iload 1
	ifeq backward
	; ERROR: VerifyError

	dup
	invokespecial test_verify_fail_backward_with_new_in_local/<init>()V

	getstatic java/lang/System/out Ljava/io/PrintStream;
	swap
	invokevirtual java/io/PrintStream/println(Ljava/lang/Object;)V

	return
.end method
