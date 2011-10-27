normal exceptions-----------------------------------------

throw new Exception():                                 OK
java.lang.Exception
   at extest.main(extest.java:26)

throw new Exception() (from subroutines):              OK
java.lang.Exception
   at extest.sub4(extest.java:299)
   at extest.sub3(extest.java:295)
   at extest.sub2(extest.java:291)
   at extest.sub(extest.java:287)
   at extest.main(extest.java:40)

NullPointerException:                                  OK
java.lang.NullPointerException
   at extest.main(extest.java:50)


exceptions thrown in JIT code-----------------------------

ArithmeticException (only w/ -softnull):               OK
java.lang.ArithmeticException: / by zero
   at extest.main(extest.java:64)

ArrayIndexOutOfBoundsException:                        OK
java.lang.ArrayIndexOutOfBoundsException: -889275714
   at extest.main(extest.java:81)

ArrayStoreException:                                   OK
java.lang.ArrayStoreException
   at extest.main(extest.java:99)

ClassCastException:                                    OK
java.lang.ClassCastException: java/lang/Object
   at extest.main(extest.java:109)

NegativeArraySizeException (newarray):                 OK
java.lang.NegativeArraySizeException
   at extest.main(extest.java:118)

NegativeArraySizeException (multianewarray):           OK
java.lang.NegativeArraySizeException
   at extest.main(extest.java:127)

OutOfMemoryError:                                      OK
java.lang.OutOfMemoryError
   at extest.main(extest.java:137)

OutOfMemoryError (multianewarray):                     OK
java.lang.OutOfMemoryError
   at extest.main(extest.java:146)


exceptions in leaf functions------------------------------

ArithmeticException:                                   OK
java.lang.ArithmeticException: / by zero
   at extest.aesub(extest.java:303)
   at extest.main(extest.java:160)

ArrayIndexOutOfBoundsException:                        OK
java.lang.ArrayIndexOutOfBoundsException: -559038737
   at extest.aioobesub(extest.java:307)
   at extest.main(extest.java:169)

ClassCastException:                                    OK
java.lang.ClassCastException: java/lang/Object
   at extest.ccesub(extest.java:311)
   at extest.main(extest.java:186)

NullPointerException:                                  OK
java.lang.NullPointerException
   at extest.npesub(extest.java:315)
   at extest.main(extest.java:195)

Exception in <clinit> triggered from a leaf method:    OK
java.lang.ExceptionInInitializerError
   at extest.main(extest.java:204)
Caused by: java.lang.NullPointerException
   at extest_clinit_patcher.<clinit>(extest.java:374)
   at extest.main(extest.java:204)


exception related things----------------------------------

load/link an exception class in asmpart:               OK
java.lang.Exception
   at extest.main(extest.java:218)


native stub exceptions------------------------------------

NullPointerException in <clinit>:                      OK
java.lang.ExceptionInInitializerError
   at extest.main(extest.java:234)
Caused by: java.lang.NullPointerException
   at extest_clinit.<clinit>(extest.java:363)
   at extest.main(extest.java:234)

UnsatisfiedLinkError:                                  OK
java.lang.UnsatisfiedLinkError: nsub
   at extest.main(extest.java:243)

NullPointerException (native):                         OK
java.lang.NullPointerException
   at java.lang.System.arraycopy(System.java:297)
   at extest.main(extest.java:252)


special exceptions----------------------------------------

OutOfMemoryError (array clone):                        OK
java.lang.OutOfMemoryError
   at [B.clone(Native Method)
   at extest.main(extest.java:268)


exception thrown to command-line--------------------------

NullPointerException (without catch):
Exception in thread "main" java.lang.NullPointerException
   at extest.main(extest.java:282)
