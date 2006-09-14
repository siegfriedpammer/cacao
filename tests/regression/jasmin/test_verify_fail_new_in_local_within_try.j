.class public test_verify_fail_new_in_local_within_try
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

	.catch java/lang/Throwable from start_try to end_try using handler

	ldc 1
	istore 1

	new test_verify_fail_backward_with_new_in_local
start_try:
	astore 2
	; ERROR: VerifyError

	aload 2
	dup
	invokespecial test_verify_fail_backward_with_new_in_local/<init>()V
end_try:

	getstatic java/lang/System/out Ljava/io/PrintStream;
	swap
	invokevirtual java/io/PrintStream/println(Ljava/lang/Object;)V

	return

handler:
	return
.end method
