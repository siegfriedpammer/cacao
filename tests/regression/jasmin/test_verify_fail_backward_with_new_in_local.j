.class public test_verify_fail_backward_with_new_in_local
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

backward:
	new test_verify_fail_backward_with_new_in_local
	astore 2

	iload 1
	ifeq backward
	; ERROR: VerifyError

	aload 2
	dup
	invokespecial test_verify_fail_backward_with_new_in_local/<init>()V

	getstatic java/lang/System/out Ljava/io/PrintStream;
	swap
	invokevirtual java/io/PrintStream/println(Ljava/lang/Object;)V

	return
.end method
