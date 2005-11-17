if (VM_IS_INST(*ip, 0)) {
  fputs("ICONST", vm_out);
{
Cell vConst;
vm_Cell2v(IMM_ARG(IPTOS,305397760 ),vConst);
fputs(" vConst=", vm_out); printarg_v(vConst);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1)) {
  fputs("LCONST", vm_out);
{
s8 l;
vm_twoCell2l((Cell)IMM_ARG(IP[1],305397761 ), (Cell)IMM_ARG(IPTOS,305397762 ), l)
fputs(" l=", vm_out); printarg_l(l);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 2)) {
  fputs("ACONST", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397763 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 3)) {
  fputs("ILOAD", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397764 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 4)) {
  fputs("LLOAD", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397765 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 5)) {
  fputs("ALOAD", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397766 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 6)) {
  fputs("IALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 7)) {
  fputs("LALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 8)) {
  fputs("AALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 9)) {
  fputs("BALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 10)) {
  fputs("CALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 11)) {
  fputs("SALOAD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 12)) {
  fputs("ISTORE", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397767 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 13)) {
  fputs("LSTORE", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397768 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 14)) {
  fputs("ASTORE", vm_out);
{
Cell vLocal;
vm_Cell2v(IMM_ARG(IPTOS,305397769 ),vLocal);
fputs(" vLocal=", vm_out); printarg_v(vLocal);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 15)) {
  fputs("IASTORE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 16)) {
  fputs("LASTORE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 17)) {
  fputs("AASTORE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 18)) {
  fputs("BASTORE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 19)) {
  fputs("CASTORE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 20)) {
  fputs("POP", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 21)) {
  fputs("POP2", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 22)) {
  fputs("DUP", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 23)) {
  fputs("DUP_X1", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 24)) {
  fputs("DUP_X2", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 25)) {
  fputs("DUP2", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 26)) {
  fputs("DUP2_X1", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 27)) {
  fputs("DUP2_X2", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 28)) {
  fputs("SWAP", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 29)) {
  fputs("IADD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 30)) {
  fputs("LADD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 31)) {
  fputs("FADD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 32)) {
  fputs("DADD", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 33)) {
  fputs("ISUB", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 34)) {
  fputs("LSUB", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 35)) {
  fputs("FSUB", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 36)) {
  fputs("DSUB", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 37)) {
  fputs("IMUL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 38)) {
  fputs("LMUL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 39)) {
  fputs("FMUL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 40)) {
  fputs("DMUL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 41)) {
  fputs("IDIV", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 42)) {
  fputs("IDIVPOW2", vm_out);
{
s4 ishift;
vm_Cell2i(IMM_ARG(IPTOS,305397770 ),ishift);
fputs(" ishift=", vm_out); printarg_i(ishift);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 43)) {
  fputs("LDIV", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 44)) {
  fputs("LDIVPOW2", vm_out);
{
s4 ishift;
vm_Cell2i(IMM_ARG(IPTOS,305397771 ),ishift);
fputs(" ishift=", vm_out); printarg_i(ishift);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 45)) {
  fputs("FDIV", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 46)) {
  fputs("DDIV", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 47)) {
  fputs("IREM", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 48)) {
  fputs("IREMPOW2", vm_out);
{
s4 imask;
vm_Cell2i(IMM_ARG(IPTOS,305397772 ),imask);
fputs(" imask=", vm_out); printarg_i(imask);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 49)) {
  fputs("LREM", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 50)) {
  fputs("LREMPOW2", vm_out);
{
s8 lmask;
vm_twoCell2l((Cell)IMM_ARG(IP[1],305397773 ), (Cell)IMM_ARG(IPTOS,305397774 ), lmask)
fputs(" lmask=", vm_out); printarg_l(lmask);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 51)) {
  fputs("FREM", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 52)) {
  fputs("DREM", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 53)) {
  fputs("INEG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 54)) {
  fputs("LNEG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 55)) {
  fputs("FNEG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 56)) {
  fputs("DNEG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 57)) {
  fputs("ISHL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 58)) {
  fputs("LSHL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 59)) {
  fputs("ISHR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 60)) {
  fputs("LSHR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 61)) {
  fputs("IUSHR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 62)) {
  fputs("LUSHR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 63)) {
  fputs("IAND", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 64)) {
  fputs("LAND", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 65)) {
  fputs("IOR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 66)) {
  fputs("LOR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 67)) {
  fputs("IXOR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 68)) {
  fputs("LXOR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 69)) {
  fputs("IINC", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397775 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
s4 iConst;
vm_Cell2i(IMM_ARG(IP[1],305397776 ),iConst);
fputs(" iConst=", vm_out); printarg_i(iConst);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 70)) {
  fputs("I2L", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 71)) {
  fputs("I2F", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 72)) {
  fputs("I2D", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 73)) {
  fputs("L2I", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 74)) {
  fputs("L2F", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 75)) {
  fputs("L2D", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 76)) {
  fputs("F2I", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 77)) {
  fputs("F2L", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 78)) {
  fputs("F2D", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 79)) {
  fputs("D2I", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 80)) {
  fputs("D2L", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 81)) {
  fputs("D2F", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 82)) {
  fputs("INT2BYTE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 83)) {
  fputs("INT2CHAR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 84)) {
  fputs("INT2SHORT", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 85)) {
  fputs("LCMP", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 86)) {
  fputs("FCMPL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 87)) {
  fputs("FCMPG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 88)) {
  fputs("DCMPL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 89)) {
  fputs("DCMPG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 90)) {
  fputs("IFEQ", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397777 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 91)) {
  fputs("IFNE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397778 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 92)) {
  fputs("IFLT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397779 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 93)) {
  fputs("IFGE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397780 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 94)) {
  fputs("IFGT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397781 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 95)) {
  fputs("IFLE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397782 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 96)) {
  fputs("IF_ICMPEQ", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397783 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 97)) {
  fputs("IF_ICMPNE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397784 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 98)) {
  fputs("IF_ICMPLT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397785 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 99)) {
  fputs("IF_ICMPGE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397786 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 100)) {
  fputs("IF_ICMPGT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397787 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 101)) {
  fputs("IF_ICMPLE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397788 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 102)) {
  fputs("IF_LCMPEQ", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397789 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 103)) {
  fputs("IF_LCMPNE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397790 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 104)) {
  fputs("IF_LCMPLT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397791 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 105)) {
  fputs("IF_LCMPGE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397792 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 106)) {
  fputs("IF_LCMPGT", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397793 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 107)) {
  fputs("IF_LCMPLE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397794 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 108)) {
  fputs("IF_ACMPEQ", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397795 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 109)) {
  fputs("IF_ACMPNE", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397796 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 110)) {
  fputs("GOTO", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397797 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 111)) {
  fputs("JSR", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397798 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 112)) {
  fputs("RET", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397799 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 113)) {
  fputs("TABLESWITCH", vm_out);
{
s4 iLow;
vm_Cell2i(IMM_ARG(IPTOS,305397800 ),iLow);
fputs(" iLow=", vm_out); printarg_i(iLow);
}
{
s4 iRange;
vm_Cell2i(IMM_ARG(IP[1],305397801 ),iRange);
fputs(" iRange=", vm_out); printarg_i(iRange);
}
{
u1 * addrSegment;
vm_Cell2addr(IMM_ARG(IP[2],305397802 ),addrSegment);
fputs(" addrSegment=", vm_out); printarg_addr(addrSegment);
}
{
s4 iOffset;
vm_Cell2i(IMM_ARG(IP[3],305397803 ),iOffset);
fputs(" iOffset=", vm_out); printarg_i(iOffset);
}
{
Inst * ainstDefault;
vm_Cell2ainst(IMM_ARG(IP[4],305397804 ),ainstDefault);
fputs(" ainstDefault=", vm_out); printarg_ainst(ainstDefault);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 114)) {
  fputs("LOOKUPSWITCH", vm_out);
{
s4 iNpairs;
vm_Cell2i(IMM_ARG(IPTOS,305397805 ),iNpairs);
fputs(" iNpairs=", vm_out); printarg_i(iNpairs);
}
{
u1 * addrSegment;
vm_Cell2addr(IMM_ARG(IP[1],305397806 ),addrSegment);
fputs(" addrSegment=", vm_out); printarg_addr(addrSegment);
}
{
s4 iOffset;
vm_Cell2i(IMM_ARG(IP[2],305397807 ),iOffset);
fputs(" iOffset=", vm_out); printarg_i(iOffset);
}
{
Inst * ainstDefault;
vm_Cell2ainst(IMM_ARG(IP[3],305397808 ),ainstDefault);
fputs(" ainstDefault=", vm_out); printarg_ainst(ainstDefault);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 115)) {
  fputs("IRETURN", vm_out);
{
Cell vOffsetFP;
vm_Cell2v(IMM_ARG(IPTOS,305397809 ),vOffsetFP);
fputs(" vOffsetFP=", vm_out); printarg_v(vOffsetFP);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 116)) {
  fputs("LRETURN", vm_out);
{
Cell vOffsetFP;
vm_Cell2v(IMM_ARG(IPTOS,305397810 ),vOffsetFP);
fputs(" vOffsetFP=", vm_out); printarg_v(vOffsetFP);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 117)) {
  fputs("RETURN", vm_out);
{
Cell vOffsetFP;
vm_Cell2v(IMM_ARG(IPTOS,305397811 ),vOffsetFP);
fputs(" vOffsetFP=", vm_out); printarg_v(vOffsetFP);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 118)) {
  fputs("GETSTATIC_CELL", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397812 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397813 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 119)) {
  fputs("GETSTATIC_INT", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397814 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397815 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 120)) {
  fputs("GETSTATIC_LONG", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397816 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397817 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 121)) {
  fputs("PUTSTATIC_CELL", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397818 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397819 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 122)) {
  fputs("PUTSTATIC_INT", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397820 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397821 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 123)) {
  fputs("PUTSTATIC_LONG", vm_out);
{
u1 * addr;
vm_Cell2addr(IMM_ARG(IPTOS,305397822 ),addr);
fputs(" addr=", vm_out); printarg_addr(addr);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397823 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 124)) {
  fputs("GETFIELD_CELL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397824 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397825 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 125)) {
  fputs("GETFIELD_INT", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397826 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397827 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 126)) {
  fputs("GETFIELD_LONG", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397828 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397829 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 127)) {
  fputs("PUTFIELD_CELL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397830 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397831 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 128)) {
  fputs("PUTFIELD_INT", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397832 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397833 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 129)) {
  fputs("PUTFIELD_LONG", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397834 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397835 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 130)) {
  fputs("INVOKEVIRTUAL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397836 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397837 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397838 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 131)) {
  fputs("INVOKESTATIC", vm_out);
{
Inst ** aaTarget;
vm_Cell2aaTarget(IMM_ARG(IPTOS,305397839 ),aaTarget);
fputs(" aaTarget=", vm_out); printarg_aaTarget(aaTarget);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397840 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397841 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 132)) {
  fputs("INVOKESPECIAL", vm_out);
{
Inst ** aaTarget;
vm_Cell2aaTarget(IMM_ARG(IPTOS,305397842 ),aaTarget);
fputs(" aaTarget=", vm_out); printarg_aaTarget(aaTarget);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397843 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397844 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 133)) {
  fputs("INVOKEINTERFACE", vm_out);
{
s4 iInterfaceOffset;
vm_Cell2i(IMM_ARG(IPTOS,305397845 ),iInterfaceOffset);
fputs(" iInterfaceOffset=", vm_out); printarg_i(iInterfaceOffset);
}
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IP[1],305397846 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[2],305397847 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[3],305397848 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 134)) {
  fputs("NEW", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 135)) {
  fputs("NEWARRAY_BOOLEAN", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 136)) {
  fputs("NEWARRAY_BYTE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 137)) {
  fputs("NEWARRAY_CHAR", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 138)) {
  fputs("NEWARRAY_SHORT", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 139)) {
  fputs("NEWARRAY_INT", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 140)) {
  fputs("NEWARRAY_LONG", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 141)) {
  fputs("NEWARRAY_FLOAT", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 142)) {
  fputs("NEWARRAY_DOUBLE", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 143)) {
  fputs("NEWARRAY", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 144)) {
  fputs("ARRAYLENGTH", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 145)) {
  fputs("ATHROW", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 146)) {
  fputs("CHECKCAST", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397849 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397850 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 147)) {
  fputs("ARRAYCHECKCAST", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397851 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397852 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 148)) {
  fputs("INSTANCEOF", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397853 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397854 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 149)) {
  fputs("ARRAYINSTANCEOF", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 150)) {
  fputs("MONITORENTER", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 151)) {
  fputs("MONITOREXIT", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 152)) {
  fputs("CHECKNULL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 153)) {
  fputs("MULTIANEWARRAY", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397855 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
{
s4 iSize;
vm_Cell2i(IMM_ARG(IP[1],305397856 ),iSize);
fputs(" iSize=", vm_out); printarg_i(iSize);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[2],305397857 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 154)) {
  fputs("IFNULL", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397858 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 155)) {
  fputs("IFNONNULL", vm_out);
{
Inst * ainstTarget;
vm_Cell2ainst(IMM_ARG(IPTOS,305397859 ),ainstTarget);
fputs(" ainstTarget=", vm_out); printarg_ainst(ainstTarget);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 156)) {
  fputs("PATCHER_GETSTATIC_INT", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397860 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397861 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 157)) {
  fputs("PATCHER_GETSTATIC_LONG", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397862 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397863 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 158)) {
  fputs("PATCHER_GETSTATIC_CELL", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397864 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397865 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 159)) {
  fputs("PATCHER_PUTSTATIC_INT", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397866 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397867 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 160)) {
  fputs("PATCHER_PUTSTATIC_LONG", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397868 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397869 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 161)) {
  fputs("PATCHER_PUTSTATIC_CELL", vm_out);
{
java_objectheader * aRef;
vm_Cell2aRef(IMM_ARG(IPTOS,305397870 ),aRef);
fputs(" aRef=", vm_out); printarg_aRef(aRef);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397871 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 162)) {
  fputs("PATCHER_GETFIELD_INT", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397872 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397873 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 163)) {
  fputs("PATCHER_GETFIELD_LONG", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397874 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397875 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 164)) {
  fputs("PATCHER_GETFIELD_CELL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397876 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397877 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 165)) {
  fputs("PATCHER_PUTFIELD_INT", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397878 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397879 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 166)) {
  fputs("PATCHER_PUTFIELD_LONG", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397880 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397881 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 167)) {
  fputs("PATCHER_PUTFIELD_CELL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397882 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
unresolved_field * auf;
vm_Cell2auf(IMM_ARG(IP[1],305397883 ),auf);
fputs(" auf=", vm_out); printarg_auf(auf);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 168)) {
  fputs("NEW1", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397884 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 169)) {
  fputs("PATCHER_NEW", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397885 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 170)) {
  fputs("NEWARRAY1", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397886 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 171)) {
  fputs("PATCHER_NEWARRAY", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397887 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 172)) {
  fputs("PATCHER_MULTIANEWARRAY", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397888 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
{
s4 iSize;
vm_Cell2i(IMM_ARG(IP[1],305397889 ),iSize);
fputs(" iSize=", vm_out); printarg_i(iSize);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[2],305397890 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 173)) {
  fputs("ARRAYINSTANCEOF1", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397891 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 174)) {
  fputs("PATCHER_ARRAYINSTANCEOF", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397892 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 175)) {
  fputs("PATCHER_INVOKESTATIC", vm_out);
{
Inst ** aaTarget;
vm_Cell2aaTarget(IMM_ARG(IPTOS,305397893 ),aaTarget);
fputs(" aaTarget=", vm_out); printarg_aaTarget(aaTarget);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397894 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397895 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 176)) {
  fputs("PATCHER_INVOKESPECIAL", vm_out);
{
Inst ** aaTarget;
vm_Cell2aaTarget(IMM_ARG(IPTOS,305397896 ),aaTarget);
fputs(" aaTarget=", vm_out); printarg_aaTarget(aaTarget);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397897 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397898 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 177)) {
  fputs("PATCHER_INVOKEVIRTUAL", vm_out);
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IPTOS,305397899 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[1],305397900 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[2],305397901 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 178)) {
  fputs("PATCHER_INVOKEINTERFACE", vm_out);
{
s4 iInterfaceoffset;
vm_Cell2i(IMM_ARG(IPTOS,305397902 ),iInterfaceoffset);
fputs(" iInterfaceoffset=", vm_out); printarg_i(iInterfaceoffset);
}
{
Cell vOffset;
vm_Cell2v(IMM_ARG(IP[1],305397903 ),vOffset);
fputs(" vOffset=", vm_out); printarg_v(vOffset);
}
{
s4 iNargs;
vm_Cell2i(IMM_ARG(IP[2],305397904 ),iNargs);
fputs(" iNargs=", vm_out); printarg_i(iNargs);
}
{
unresolved_method * aum;
vm_Cell2aum(IMM_ARG(IP[3],305397905 ),aum);
fputs(" aum=", vm_out); printarg_aum(aum);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 179)) {
  fputs("PATCHER_CHECKCAST", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397906 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397907 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 180)) {
  fputs("PATCHER_ARRAYCHECKCAST", vm_out);
{
vftbl_t * avftbl;
vm_Cell2avftbl(IMM_ARG(IPTOS,305397908 ),avftbl);
fputs(" avftbl=", vm_out); printarg_avftbl(avftbl);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397909 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 181)) {
  fputs("PATCHER_INSTANCEOF", vm_out);
{
classinfo * aClass;
vm_Cell2aClass(IMM_ARG(IPTOS,305397910 ),aClass);
fputs(" aClass=", vm_out); printarg_aClass(aClass);
}
{
constant_classref * acr;
vm_Cell2acr(IMM_ARG(IP[1],305397911 ),acr);
fputs(" acr=", vm_out); printarg_acr(acr);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 182)) {
  fputs("TRANSLATE", vm_out);
{
methodinfo * am;
vm_Cell2am(IMM_ARG(IPTOS,305397912 ),am);
fputs(" am=", vm_out); printarg_am(am);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 183)) {
  fputs("NATIVECALL", vm_out);
{
methodinfo * am;
vm_Cell2am(IMM_ARG(IPTOS,305397913 ),am);
fputs(" am=", vm_out); printarg_am(am);
}
{
functionptr af;
vm_Cell2af(IMM_ARG(IP[1],305397914 ),af);
fputs(" af=", vm_out); printarg_af(af);
}
{
u1 * addrcif;
vm_Cell2addr(IMM_ARG(IP[2],305397915 ),addrcif);
fputs(" addrcif=", vm_out); printarg_addr(addrcif);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 184)) {
  fputs("TRACENATIVECALL", vm_out);
{
methodinfo * am;
vm_Cell2am(IMM_ARG(IPTOS,305397916 ),am);
fputs(" am=", vm_out); printarg_am(am);
}
{
functionptr af;
vm_Cell2af(IMM_ARG(IP[1],305397917 ),af);
fputs(" af=", vm_out); printarg_af(af);
}
{
u1 * addrcif;
vm_Cell2addr(IMM_ARG(IP[2],305397918 ),addrcif);
fputs(" addrcif=", vm_out); printarg_addr(addrcif);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 185)) {
  fputs("PATCHER_NATIVECALL", vm_out);
{
methodinfo * am;
vm_Cell2am(IMM_ARG(IPTOS,305397919 ),am);
fputs(" am=", vm_out); printarg_am(am);
}
{
functionptr af;
vm_Cell2af(IMM_ARG(IP[1],305397920 ),af);
fputs(" af=", vm_out); printarg_af(af);
}
{
u1 * addrcif;
vm_Cell2addr(IMM_ARG(IP[2],305397921 ),addrcif);
fputs(" addrcif=", vm_out); printarg_addr(addrcif);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 186)) {
  fputs("TRACECALL", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 187)) {
  fputs("TRACERETURN", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 188)) {
  fputs("TRACELRETURN", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 189)) {
  fputs("END", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 190)) {
  fputs("_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397922 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 191)) {
  fputs("_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397923 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397924 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 192)) {
  fputs("_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397925 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397926 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 193)) {
  fputs("_ICONST_CASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397927 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 194)) {
  fputs("_ICONST_ICONST_CASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397928 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397929 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 195)) {
  fputs("_DUP_ICONST_ICONST_CASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397930 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397931 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 196)) {
  fputs("_CASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 197)) {
  fputs("_CASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397932 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 198)) {
  fputs("_CASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397933 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397934 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 199)) {
  fputs("_CASTORE_DUP_ICONST_ICONST_CASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397935 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397936 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 200)) {
  fputs("_DUP_ICONST_ICONST_CASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397937 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397938 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 201)) {
  fputs("_ICONST_CASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397939 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 202)) {
  fputs("_ICONST_CASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397940 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397941 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 203)) {
  fputs("_ICONST_CASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397942 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397943 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397944 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 204)) {
  fputs("_ICONST_ICONST_CASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397945 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397946 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 205)) {
  fputs("_ICONST_ICONST_CASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397947 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397948 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397949 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 206)) {
  fputs("_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397950 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397951 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397952 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 207)) {
  fputs("_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397953 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397954 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 208)) {
  fputs("_ICONST_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397955 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 209)) {
  fputs("_ICONST_ICONST_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397956 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397957 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 210)) {
  fputs("_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397958 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397959 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 211)) {
  fputs("_DUP_ICONST_ICONST_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397960 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397961 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 212)) {
  fputs("_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397962 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397963 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 213)) {
  fputs("_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397964 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397965 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397966 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 214)) {
  fputs("_IASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 215)) {
  fputs("_IASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397967 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 216)) {
  fputs("_DUP_ICONST_ICONST_IASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397968 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397969 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 217)) {
  fputs("_ICONST_IASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397970 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 218)) {
  fputs("_ICONST_IASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397971 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397972 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 219)) {
  fputs("_ICONST_ICONST_IASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397973 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397974 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 220)) {
  fputs("_ICONST_ICONST_IASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397975 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397976 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397977 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 221)) {
  fputs("_IASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397978 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397979 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 222)) {
  fputs("_IASTORE_DUP_ICONST_ICONST_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397980 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397981 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 223)) {
  fputs("_ICONST_IASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397982 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397983 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305397984 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 224)) {
  fputs("_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397985 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397986 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 225)) {
  fputs("_AASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 226)) {
  fputs("_AASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397987 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 227)) {
  fputs("_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397988 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397989 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 228)) {
  fputs("_DUP_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397990 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397991 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 229)) {
  fputs("_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397992 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397993 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 230)) {
  fputs("_ACONST_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397994 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 231)) {
  fputs("_ICONST_ACONST_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397995 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397996 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 232)) {
  fputs("_AASTORE_DUP_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397997 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305397998 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 233)) {
  fputs("_DUP_ICONST_ACONST_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305397999 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398000 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 234)) {
  fputs("_ACONST_AASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398001 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 235)) {
  fputs("_ACONST_AASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398002 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398003 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 236)) {
  fputs("_DUP_ICONST_ACONST_AASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398004 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398005 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 237)) {
  fputs("_ICONST_ACONST_AASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398006 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398007 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 238)) {
  fputs("_ICONST_ACONST_AASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398008 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398009 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398010 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 239)) {
  fputs("_AASTORE_DUP_ICONST_ACONST_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398011 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398012 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 240)) {
  fputs("_ACONST_AASTORE_DUP_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398013 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398014 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398015 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 241)) {
  fputs("_POP_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398016 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 242)) {
  fputs("_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398017 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398018 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398019 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 243)) {
  fputs("_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398020 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398021 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 244)) {
  fputs("_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398022 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398023 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398024 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 245)) {
  fputs("_POP_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398025 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398026 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 246)) {
  fputs("_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398027 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398028 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398029 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 247)) {
  fputs("_ALOAD_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398030 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398031 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398032 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 248)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398033 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398034 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398035 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398036 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 249)) {
  fputs("_POP_ALOAD_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398037 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398038 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398039 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 250)) {
  fputs("_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398040 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398041 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 251)) {
  fputs("_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398042 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398043 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 252)) {
  fputs("_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398044 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398045 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398046 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 253)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398047 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398048 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398049 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398050 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 254)) {
  fputs("_DUP_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398051 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 255)) {
  fputs("_GETSTATIC_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398052 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398053 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398054 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 256)) {
  fputs("_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398055 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398056 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 257)) {
  fputs("_ICONST_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398057 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398058 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 258)) {
  fputs("_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398059 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398060 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398061 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 259)) {
  fputs("_GETSTATIC_CELL_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398062 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398063 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398064 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398065 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 260)) {
  fputs("_GETSTATIC_CELL_ICONST_ICONST_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398066 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398067 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398068 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398069 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 261)) {
  fputs("_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398070 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398071 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398072 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 262)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398073 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398074 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398075 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398076 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 263)) {
  fputs("_PUTSTATIC_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398077 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398078 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398079 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 264)) {
  fputs("_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398080 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398081 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398082 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 265)) {
  fputs("_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398083 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398084 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 266)) {
  fputs("_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398085 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398086 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 267)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398087 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398088 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398089 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398090 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 268)) {
  fputs("_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398091 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398092 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 269)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398093 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398094 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398095 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398096 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 270)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398097 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398098 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398099 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398100 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 271)) {
  fputs("_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398101 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398102 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398103 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 272)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398104 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398105 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398106 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398107 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398108 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 273)) {
  fputs("_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398109 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398110 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 274)) {
  fputs("_ALOAD_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398111 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398112 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 275)) {
  fputs("_DUP_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398113 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 276)) {
  fputs("_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398114 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398115 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 277)) {
  fputs("_ALOAD_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398116 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398117 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 278)) {
  fputs("_GETSTATIC_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398118 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398119 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398120 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 279)) {
  fputs("_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398121 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398122 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398123 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 280)) {
  fputs("_ALOAD_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398124 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398125 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398126 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398127 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 281)) {
  fputs("_ICONST_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398128 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398129 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398130 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 282)) {
  fputs("_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398131 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 283)) {
  fputs("_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398132 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398133 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398134 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 284)) {
  fputs("_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398135 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398136 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 285)) {
  fputs("_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398137 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398138 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398139 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398140 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 286)) {
  fputs("_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398141 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398142 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 287)) {
  fputs("_ALOAD_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398143 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 288)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398144 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398145 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398146 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398147 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 289)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398148 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398149 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398150 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398151 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398152 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 290)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398153 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398154 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398155 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 291)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398156 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398157 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398158 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398159 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 292)) {
  fputs("_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398160 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 293)) {
  fputs("_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398161 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398162 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 294)) {
  fputs("_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398163 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398164 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 295)) {
  fputs("_CHECKCAST_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398165 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398166 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398167 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 296)) {
  fputs("_GETFIELD_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398168 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398169 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398170 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 297)) {
  fputs("_ASTORE_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398171 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398172 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398173 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 298)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398174 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398175 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398176 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398177 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 299)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398178 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398179 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398180 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398181 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 300)) {
  fputs("_CHECKCAST_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398182 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398183 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398184 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398185 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 301)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398186 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398187 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398188 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398189 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 302)) {
  fputs("_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398190 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398191 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 303)) {
  fputs("_ASTORE_ALOAD_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398192 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398193 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398194 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 304)) {
  fputs("_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398195 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398196 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398197 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 305)) {
  fputs("_ALOAD_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398198 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398199 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398200 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398201 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 306)) {
  fputs("_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398202 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 307)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398203 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398204 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398205 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398206 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398207 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 308)) {
  fputs("_ALOAD_MONITOREXIT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398208 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 309)) {
  fputs("_ILOAD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398209 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398210 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398211 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398212 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 310)) {
  fputs("_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398213 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398214 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398215 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398216 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398217 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 311)) {
  fputs("_ASTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398218 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398219 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398220 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398221 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 312)) {
  fputs("_GETFIELD_CELL_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398222 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398223 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 313)) {
  fputs("_ICONST_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398224 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398225 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 314)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398226 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398227 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398228 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398229 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398230 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 315)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398231 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398232 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398233 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398234 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398235 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 316)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398236 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398237 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398238 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398239 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398240 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398241 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 317)) {
  fputs("_PUTFIELD_CELL_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398242 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398243 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398244 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398245 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 318)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398246 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398247 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398248 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398249 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398250 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398251 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 319)) {
  fputs("_ALOAD_GETFIELD_CELL_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398252 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398253 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398254 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 320)) {
  fputs("_ALOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398255 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398256 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398257 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 321)) {
  fputs("_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398258 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 322)) {
  fputs("_PUTFIELD_INT_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398259 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398260 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398261 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398262 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 323)) {
  fputs("_ASTORE_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398263 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398264 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 324)) {
  fputs("_AASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398265 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398266 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 325)) {
  fputs("_GETSTATIC_CELL_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398267 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398268 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398269 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398270 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 326)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398271 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398272 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398273 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398274 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 327)) {
  fputs("_GETFIELD_CELL_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398275 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398276 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398277 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 328)) {
  fputs("_ALOAD_GETFIELD_CELL_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398278 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398279 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398280 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398281 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 329)) {
  fputs("_DUP_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398282 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398283 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398284 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 330)) {
  fputs("_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398285 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 331)) {
  fputs("_ALOAD_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398286 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398287 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398288 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 332)) {
  fputs("_ICONST_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398289 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398290 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 333)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398291 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398292 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398293 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398294 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398295 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 334)) {
  fputs("_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398296 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 335)) {
  fputs("_ALOAD_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398297 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398298 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398299 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 336)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398300 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398301 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398302 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398303 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 337)) {
  fputs("_ICONST_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398304 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398305 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398306 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 338)) {
  fputs("_GETFIELD_CELL_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398307 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398308 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398309 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 339)) {
  fputs("_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398310 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398311 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398312 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398313 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398314 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398315 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 340)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398316 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398317 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398318 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398319 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 341)) {
  fputs("_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398320 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 342)) {
  fputs("_ASTORE_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398321 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398322 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 343)) {
  fputs("_ACONST_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398323 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398324 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 344)) {
  fputs("_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398325 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398326 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398327 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398328 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398329 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398330 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 345)) {
  fputs("_ILOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398331 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398332 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398333 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398334 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 346)) {
  fputs("_ALOAD_GETFIELD_CELL_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398335 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398336 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398337 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398338 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 347)) {
  fputs("_GETFIELD_INT_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398339 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398340 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398341 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 348)) {
  fputs("_MONITORENTER_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398342 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 349)) {
  fputs("_ILOAD_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398343 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398344 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 350)) {
  fputs("_ILOAD_IFGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398345 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398346 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 351)) {
  fputs("_ASTORE_ALOAD_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398347 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398348 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398349 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 352)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398350 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398351 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398352 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398353 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398354 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 353)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398355 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398356 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398357 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398358 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398359 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398360 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 354)) {
  fputs("_ALOAD_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398361 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398362 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398363 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 355)) {
  fputs("_GETFIELD_CELL_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398364 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398365 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398366 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 356)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398367 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398368 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398369 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 357)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398370 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398371 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398372 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398373 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398374 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398375 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 358)) {
  fputs("_ALOAD_GETFIELD_INT_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398376 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398377 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398378 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398379 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 359)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398380 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398381 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398382 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398383 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 360)) {
  fputs("_ISTORE_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398384 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398385 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 361)) {
  fputs("_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398386 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 362)) {
  fputs("_ALOAD_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398387 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398388 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398389 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 363)) {
  fputs("_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398390 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 364)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398391 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398392 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398393 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398394 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398395 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398396 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398397 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 365)) {
  fputs("_ASTORE_ICONST_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398398 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398399 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398400 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 366)) {
  fputs("_GETFIELD_INT_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398401 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398402 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398403 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 367)) {
  fputs("_ICONST_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398404 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398405 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 368)) {
  fputs("_PUTFIELD_INT_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398406 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398407 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398408 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398409 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 369)) {
  fputs("_ILOAD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398410 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398411 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398412 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398413 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 370)) {
  fputs("_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398414 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398415 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398416 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398417 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 371)) {
  fputs("_DUP_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398418 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398419 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 372)) {
  fputs("_DUP_ICONST_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398420 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398421 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 373)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398422 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398423 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398424 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398425 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 374)) {
  fputs("_ALOAD_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398426 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398427 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398428 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398429 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 375)) {
  fputs("_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398430 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398431 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398432 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398433 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398434 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 376)) {
  fputs("_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398435 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398436 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398437 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 377)) {
  fputs("_BASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 378)) {
  fputs("_BASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398438 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 379)) {
  fputs("_ALOAD_GETFIELD_CELL_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398439 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398440 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398441 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398442 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 380)) {
  fputs("_PUTFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398443 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398444 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398445 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398446 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 381)) {
  fputs("_BASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398447 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398448 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 382)) {
  fputs("_BASTORE_DUP_ICONST_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398449 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398450 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 383)) {
  fputs("_DUP_ICONST_ICONST_BASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398451 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398452 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 384)) {
  fputs("_ICONST_BASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398453 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 385)) {
  fputs("_ICONST_BASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398454 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398455 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 386)) {
  fputs("_ICONST_BASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398456 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398457 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398458 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 387)) {
  fputs("_ICONST_ICONST_BASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398459 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398460 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 388)) {
  fputs("_ICONST_ICONST_BASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398461 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398462 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398463 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 389)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398464 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398465 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398466 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398467 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398468 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398469 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398470 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 390)) {
  fputs("_GETFIELD_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398471 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398472 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398473 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 391)) {
  fputs("_DUP_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398474 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398475 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 392)) {
  fputs("_ALOAD_GETFIELD_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398476 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398477 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398478 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398479 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 393)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398480 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398481 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398482 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398483 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398484 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 394)) {
  fputs("_ISTORE_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398485 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398486 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398487 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 395)) {
  fputs("_POP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398488 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 396)) {
  fputs("_INSTANCEOF_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398489 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398490 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398491 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 397)) {
  fputs("_ALOAD_MONITORENTER_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398492 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 398)) {
  fputs("_ILOAD_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398493 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398494 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398495 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 399)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398496 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398497 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398498 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398499 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398500 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398501 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 400)) {
  fputs("_DUP_GETFIELD_INT_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398502 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398503 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398504 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 401)) {
  fputs("_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398505 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398506 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398507 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398508 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398509 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398510 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398511 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 402)) {
  fputs("_ACONST_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398512 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398513 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398514 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 403)) {
  fputs("_IADD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398515 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398516 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398517 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 404)) {
  fputs("_ICONST_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398518 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398519 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398520 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398521 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 405)) {
  fputs("_DUP_ACONST_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398522 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398523 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398524 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 406)) {
  fputs("_ALOAD_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398525 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398526 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 407)) {
  fputs("_ISTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398527 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398528 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398529 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398530 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 408)) {
  fputs("_ICONST_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398531 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398532 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398533 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 409)) {
  fputs("_ICONST_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398534 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 410)) {
  fputs("_GETFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398535 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398536 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398537 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398538 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 411)) {
  fputs("_ISTORE_ICONST_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398539 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398540 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398541 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 412)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398542 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398543 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398544 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398545 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 413)) {
  fputs("_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398546 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 414)) {
  fputs("_ILOAD_IFLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398547 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398548 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 415)) {
  fputs("_GETFIELD_INT_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398549 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398550 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398551 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 416)) {
  fputs("_GETSTATIC_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398552 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398553 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398554 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 417)) {
  fputs("_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398555 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398556 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398557 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398558 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398559 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398560 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 418)) {
  fputs("_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398561 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398562 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398563 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398564 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398565 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 419)) {
  fputs("_ASTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398566 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398567 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398568 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 420)) {
  fputs("_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398569 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398570 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398571 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398572 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398573 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 421)) {
  fputs("_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398574 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398575 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398576 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398577 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398578 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 422)) {
  fputs("_ALOAD_INSTANCEOF_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398579 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398580 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398581 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 423)) {
  fputs("_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398582 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398583 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398584 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398585 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398586 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398587 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 424)) {
  fputs("_LCONST_LAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398588 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398589 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 425)) {
  fputs("_ILOAD_ICONST_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398590 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398591 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398592 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 426)) {
  fputs("_DUP_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398593 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398594 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398595 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398596 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398597 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 427)) {
  fputs("_ALOAD_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398598 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398599 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398600 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 428)) {
  fputs("_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398601 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398602 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398603 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398604 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398605 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398606 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398607 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 429)) {
  fputs("_ILOAD_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398608 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398609 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 430)) {
  fputs("_ILOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398610 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398611 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398612 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 431)) {
  fputs("_ALOAD_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398613 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398614 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398615 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 432)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398616 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398617 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398618 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398619 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398620 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 433)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398621 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398622 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398623 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398624 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398625 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 434)) {
  fputs("_ICONST_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398626 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398627 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398628 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 435)) {
  fputs("_GETFIELD_INT_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398629 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398630 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398631 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 436)) {
  fputs("_GETFIELD_INT_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398632 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398633 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398634 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398635 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 437)) {
  fputs("_GETSTATIC_CELL_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398636 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398637 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398638 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398639 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 438)) {
  fputs("_ISTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398640 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398641 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398642 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398643 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 439)) {
  fputs("_ALOAD_MONITORENTER_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398644 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398645 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 440)) {
  fputs("_ILOAD_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398646 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398647 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 441)) {
  fputs("_ASTORE_ICONST_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398648 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398649 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398650 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398651 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 442)) {
  fputs("_ALOAD_CHECKCAST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398652 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398653 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398654 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 443)) {
  fputs("_ALOAD_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398655 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398656 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398657 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 444)) {
  fputs("_ICONST_IADD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398658 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398659 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398660 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398661 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 445)) {
  fputs("_DUP_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398662 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 446)) {
  fputs("_PUTFIELD_CELL_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398663 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398664 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398665 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398666 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 447)) {
  fputs("_ILOAD_ILOAD_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398667 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398668 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398669 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 448)) {
  fputs("_ICONST_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398670 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398671 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 449)) {
  fputs("_ALOAD_GETFIELD_INT_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398672 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398673 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398674 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398675 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 450)) {
  fputs("_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398676 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398677 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398678 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398679 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398680 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398681 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398682 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 451)) {
  fputs("_ILOAD_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398683 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 452)) {
  fputs("_ALOAD_GETFIELD_INT_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398684 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398685 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398686 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398687 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 453)) {
  fputs("_GETFIELD_CELL_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398688 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398689 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398690 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398691 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 454)) {
  fputs("_ISTORE_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398692 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398693 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398694 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 455)) {
  fputs("_ILOAD_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398695 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398696 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398697 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 456)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398698 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398699 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398700 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398701 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398702 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 457)) {
  fputs("_ALOAD_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398703 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398704 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398705 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 458)) {
  fputs("_POP_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398706 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398707 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 459)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398708 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398709 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398710 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398711 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398712 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 460)) {
  fputs("_POP_GETSTATIC_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398713 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398714 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398715 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 461)) {
  fputs("_ALOAD_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398716 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398717 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 462)) {
  fputs("_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398718 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398719 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398720 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398721 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 463)) {
  fputs("_GETSTATIC_INT_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398722 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398723 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398724 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 464)) {
  fputs("_ALOAD_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398725 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398726 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398727 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 465)) {
  fputs("_AALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398728 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 466)) {
  fputs("_ALOAD_ICONST_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398729 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398730 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 467)) {
  fputs("_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398731 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398732 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398733 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398734 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398735 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 468)) {
  fputs("_ASTORE_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398736 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398737 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398738 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 469)) {
  fputs("_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398739 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398740 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398741 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398742 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398743 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398744 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305398745 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 470)) {
  fputs("_PUTFIELD_INT_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398746 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398747 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398748 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 471)) {
  fputs("_PUTSTATIC_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398749 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398750 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398751 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 472)) {
  fputs("_DUP_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398752 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398753 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398754 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 473)) {
  fputs("_ALOAD_ICONST_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398755 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398756 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398757 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 474)) {
  fputs("_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398758 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398759 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398760 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398761 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398762 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398763 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 475)) {
  fputs("_GETSTATIC_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398764 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398765 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398766 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 476)) {
  fputs("_LALOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398767 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398768 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 477)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398769 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398770 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398771 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398772 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398773 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 478)) {
  fputs("_IADD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398774 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 479)) {
  fputs("_ILOAD_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398775 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398776 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 480)) {
  fputs("_ACONST_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398777 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398778 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398779 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 481)) {
  fputs("_GETFIELD_CELL_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398780 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398781 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398782 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398783 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 482)) {
  fputs("_ILOAD_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398784 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398785 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 483)) {
  fputs("_ICONST_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398786 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398787 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 484)) {
  fputs("_ICONST_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398788 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398789 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 485)) {
  fputs("_LALOAD_LCONST_LAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398790 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398791 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 486)) {
  fputs("_ACONST_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398792 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398793 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398794 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 487)) {
  fputs("_GETSTATIC_CELL_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398795 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398796 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398797 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398798 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 488)) {
  fputs("_PUTFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398799 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398800 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398801 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398802 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398803 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 489)) {
  fputs("_ICONST_DUP2_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398804 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 490)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398805 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398806 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398807 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398808 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398809 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 491)) {
  fputs("_ILOAD_ALOAD_GETFIELD_CELL_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398810 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398811 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398812 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398813 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 492)) {
  fputs("_GETFIELD_INT_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398814 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398815 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398816 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 493)) {
  fputs("_ILOAD_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398817 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398818 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 494)) {
  fputs("_CHECKNULL_MONITORENTER_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 495)) {
  fputs("_ALOAD_GETFIELD_CELL_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398819 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398820 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398821 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398822 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398823 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 496)) {
  fputs("_ILOAD_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398824 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398825 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 497)) {
  fputs("_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398826 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398827 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398828 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398829 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398830 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398831 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 498)) {
  fputs("_ASTORE_CHECKNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398832 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 499)) {
  fputs("_ASTORE_CHECKNULL_MONITORENTER_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398833 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 500)) {
  fputs("_DUP_ASTORE_CHECKNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398834 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 501)) {
  fputs("_DUP_ASTORE_CHECKNULL_MONITORENTER_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398835 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 502)) {
  fputs("_ILOAD_ICONST_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398836 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398837 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398838 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 503)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398839 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398840 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398841 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398842 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398843 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398844 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 504)) {
  fputs("_ALOAD_ICONST_DUP2_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398845 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398846 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 505)) {
  fputs("_ALOAD_INSTANCEOF_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398847 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398848 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398849 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398850 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 506)) {
  fputs("_ASTORE_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398851 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398852 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398853 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398854 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398855 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 507)) {
  fputs("_IINC_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398856 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398857 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398858 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 508)) {
  fputs("_AALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398859 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 509)) {
  fputs("_ICONST_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398860 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398861 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398862 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398863 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 510)) {
  fputs("_ISUB_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398864 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 511)) {
  fputs("_GETFIELD_INT_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398865 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398866 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 512)) {
  fputs("_ALOAD_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398867 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398868 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 513)) {
  fputs("_ALOAD_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398869 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398870 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398871 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398872 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 514)) {
  fputs("_ALOAD_ACONST_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398873 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398874 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398875 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398876 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 515)) {
  fputs("_ILOAD_ICONST_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398877 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398878 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398879 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 516)) {
  fputs("_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398880 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398881 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398882 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398883 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398884 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398885 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 517)) {
  fputs("_ILOAD_ALOAD_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398886 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398887 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398888 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 518)) {
  fputs("_GETFIELD_INT_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398889 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398890 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398891 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 519)) {
  fputs("_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398892 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398893 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398894 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398895 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 520)) {
  fputs("_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398896 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398897 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398898 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 521)) {
  fputs("_ALOAD_GETFIELD_INT_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398899 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398900 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398901 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 522)) {
  fputs("_LCONST_LASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398902 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398903 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 523)) {
  fputs("_PUTFIELD_INT_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398904 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398905 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398906 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398907 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 524)) {
  fputs("_LLOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398908 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398909 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398910 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 525)) {
  fputs("_BALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398911 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 526)) {
  fputs("_GETFIELD_INT_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398912 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398913 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398914 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398915 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398916 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 527)) {
  fputs("_GETFIELD_CELL_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398917 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398918 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398919 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398920 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 528)) {
  fputs("_ICONST_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398921 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398922 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398923 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 529)) {
  fputs("_PUTFIELD_CELL_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398924 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398925 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398926 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 530)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398927 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398928 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398929 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398930 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398931 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 531)) {
  fputs("_GETFIELD_INT_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398932 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398933 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 532)) {
  fputs("_DUP2_LALOAD_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 533)) {
  fputs("_GETFIELD_CELL_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398934 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398935 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 534)) {
  fputs("_CHECKNULL_MONITORENTER_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398936 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 535)) {
  fputs("_GETFIELD_CELL_ALOAD_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398937 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398938 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398939 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 536)) {
  fputs("_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398940 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398941 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398942 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398943 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398944 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 537)) {
  fputs("_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398945 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398946 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398947 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398948 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398949 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305398950 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 538)) {
  fputs("_ICONST_LCONST_LASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398951 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398952 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398953 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 539)) {
  fputs("_ASTORE_CHECKNULL_MONITORENTER_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398954 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398955 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 540)) {
  fputs("_DUP_ASTORE_CHECKNULL_MONITORENTER_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398956 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398957 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 541)) {
  fputs("_ILOAD_AALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398958 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398959 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 542)) {
  fputs("_POP_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398960 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398961 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 543)) {
  fputs("_GETFIELD_INT_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398962 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398963 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 544)) {
  fputs("_GETFIELD_INT_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398964 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398965 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398966 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 545)) {
  fputs("_DUP_GETFIELD_INT_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398967 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398968 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 546)) {
  fputs("_DUP_GETFIELD_INT_DUP_X1_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398969 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398970 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398971 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 547)) {
  fputs("_DUP_X1_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398972 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 548)) {
  fputs("_GETFIELD_INT_DUP_X1_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398973 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398974 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398975 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 549)) {
  fputs("_ICONST_ISTORE_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398976 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398977 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398978 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 550)) {
  fputs("_DUP_GETFIELD_INT_DUP_X1_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398979 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398980 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398981 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 551)) {
  fputs("_DUP_X1_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398982 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 552)) {
  fputs("_DUP_X1_ICONST_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398983 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398984 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398985 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 553)) {
  fputs("_GETFIELD_INT_DUP_X1_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398986 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398987 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398988 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 554)) {
  fputs("_GETFIELD_INT_DUP_X1_ICONST_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398989 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398990 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398991 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305398992 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305398993 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 555)) {
  fputs("_DUP2_LALOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398994 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398995 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 556)) {
  fputs("_ALOAD_GETFIELD_LONG_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398996 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305398997 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305398998 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 557)) {
  fputs("_LAND_LASTORE_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 558)) {
  fputs("_ICONST_ISHL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305398999 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 559)) {
  fputs("_ILOAD_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399000 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399001 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399002 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399003 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399004 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 560)) {
  fputs("_ICONST_ICONST_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399005 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399006 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399007 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399008 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 561)) {
  fputs("_GETFIELD_INT_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399009 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399010 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399011 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399012 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 562)) {
  fputs("_ALOAD_LLOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399013 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399014 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 563)) {
  fputs("_ASTORE_ACONST_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399015 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399016 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399017 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 564)) {
  fputs("_ALOAD_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399018 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 565)) {
  fputs("_ALOAD_GETFIELD_INT_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399019 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399020 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399021 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 566)) {
  fputs("_MONITORENTER_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399022 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399023 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399024 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 567)) {
  fputs("_DUP_X1_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399025 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399026 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 568)) {
  fputs("_GETSTATIC_CELL_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399027 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399028 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399029 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 569)) {
  fputs("_ICONST_ISTORE_ICONST_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399030 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399031 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399032 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399033 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 570)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399034 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399035 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399036 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399037 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399038 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 571)) {
  fputs("_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399039 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399040 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399041 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399042 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399043 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399044 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 572)) {
  fputs("_ILOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399045 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399046 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 573)) {
  fputs("_ISTORE_ICONST_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399047 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399048 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399049 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399050 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 574)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399051 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399052 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399053 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 575)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_DUP_X1_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399054 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399055 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399056 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399057 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 576)) {
  fputs("_ALOAD_GETFIELD_INT_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399058 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399059 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399060 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399061 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 577)) {
  fputs("_BALOAD_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399062 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 578)) {
  fputs("_ALOAD_GETSTATIC_CELL_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399063 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399064 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399065 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399066 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399067 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 579)) {
  fputs("_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399068 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399069 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399070 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399071 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399072 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399073 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399074 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 580)) {
  fputs("_DUP_ICONST_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399075 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399076 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399077 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 581)) {
  fputs("_DUP_ICONST_LCONST_LASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399078 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399079 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399080 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 582)) {
  fputs("_ALOAD_GETFIELD_INT_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399081 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399082 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399083 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399084 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399085 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 583)) {
  fputs("_DUP_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399086 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399087 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399088 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 584)) {
  fputs("_ISTORE_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399089 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399090 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399091 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 585)) {
  fputs("_DUP_ICONST_ICONST_IASTORE_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399092 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399093 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 586)) {
  fputs("_IASTORE_AASTORE_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 587)) {
  fputs("_ICONST_IASTORE_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399094 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 588)) {
  fputs("_ICONST_ICONST_IASTORE_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399095 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399096 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 589)) {
  fputs("_ALOAD_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399097 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399098 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399099 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399100 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399101 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 590)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399102 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399103 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399104 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399105 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399106 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399107 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 591)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399108 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399109 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399110 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399111 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399112 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399113 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 592)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399114 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399115 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399116 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399117 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399118 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 593)) {
  fputs("_IASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399119 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 594)) {
  fputs("_ICONST_ISHR_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399120 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 595)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399121 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399122 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399123 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399124 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 596)) {
  fputs("_GETFIELD_CELL_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399125 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399126 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399127 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399128 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 597)) {
  fputs("_IASTORE_AASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 598)) {
  fputs("_IASTORE_AASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399129 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 599)) {
  fputs("_IASTORE_AASTORE_DUP_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399130 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399131 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 600)) {
  fputs("_ICONST_IASTORE_AASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399132 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 601)) {
  fputs("_ICONST_IASTORE_AASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399133 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399134 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 602)) {
  fputs("_ICONST_ICONST_IASTORE_AASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399135 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399136 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 603)) {
  fputs("_ALOAD_ALOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399137 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399138 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399139 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399140 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 604)) {
  fputs("_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399141 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399142 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399143 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399144 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399145 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 605)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399146 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399147 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399148 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399149 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 606)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399150 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399151 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399152 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399153 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399154 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399155 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 607)) {
  fputs("_ISUB_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399156 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399157 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 608)) {
  fputs("_ALOAD_GETFIELD_CELL_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399158 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399159 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399160 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 609)) {
  fputs("_IAND_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399161 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 610)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399162 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399163 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399164 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399165 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399166 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399167 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399168 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 611)) {
  fputs("_ILOAD_AALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399169 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399170 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 612)) {
  fputs("_ILOAD_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399171 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 613)) {
  fputs("_ALOAD_ALOAD_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399172 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399173 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399174 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399175 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399176 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 614)) {
  fputs("_DUP_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399177 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 615)) {
  fputs("_AASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399178 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 616)) {
  fputs("_ICONST_IAND_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399179 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399180 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 617)) {
  fputs("_CHECKCAST_ASTORE_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399181 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399182 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399183 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399184 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399185 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 618)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399186 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399187 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399188 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399189 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399190 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 619)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399191 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399192 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399193 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399194 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399195 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399196 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 620)) {
  fputs("_ALOAD_ICONST_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399197 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399198 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399199 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399200 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399201 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 621)) {
  fputs("_POP_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399202 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399203 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399204 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 622)) {
  fputs("_ALOAD_ALOAD_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399205 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399206 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399207 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399208 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 623)) {
  fputs("_ASTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399209 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399210 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 624)) {
  fputs("_ISTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399211 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399212 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399213 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 625)) {
  fputs("_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399214 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399215 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399216 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399217 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399218 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399219 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399220 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 626)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399221 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399222 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399223 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399224 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399225 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399226 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 627)) {
  fputs("_ILOAD_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399227 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399228 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399229 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 628)) {
  fputs("_IALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399230 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 629)) {
  fputs("_DUP_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399231 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399232 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 630)) {
  fputs("_DUP_GETFIELD_INT_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399233 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399234 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399235 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 631)) {
  fputs("_ISTORE_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399236 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399237 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399238 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 632)) {
  fputs("_DUP_GETFIELD_INT_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399239 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399240 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399241 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 633)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399242 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399243 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399244 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399245 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399246 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399247 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399248 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 634)) {
  fputs("_GETFIELD_INT_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399249 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399250 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399251 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 635)) {
  fputs("_GETFIELD_INT_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399252 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399253 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399254 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 636)) {
  fputs("_GETSTATIC_CELL_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399255 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399256 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399257 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399258 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 637)) {
  fputs("_ISTORE_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399259 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399260 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399261 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399262 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399263 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 638)) {
  fputs("_PUTFIELD_CELL_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399264 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399265 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399266 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399267 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399268 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399269 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 639)) {
  fputs("_ALOAD_GETFIELD_INT_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399270 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399271 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399272 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399273 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 640)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399274 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399275 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399276 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399277 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 641)) {
  fputs("_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399278 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399279 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399280 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399281 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399282 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399283 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 642)) {
  fputs("_LCONST_IF_LCMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399284 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399285 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399286 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 643)) {
  fputs("_POP_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399287 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 644)) {
  fputs("_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399288 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399289 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399290 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399291 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399292 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399293 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399294 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 645)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399295 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399296 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399297 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399298 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399299 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399300 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399301 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 646)) {
  fputs("_GETFIELD_CELL_CHECKCAST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399302 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399303 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399304 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399305 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 647)) {
  fputs("_LAND_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399306 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399307 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 648)) {
  fputs("_ARRAYLENGTH_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399308 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 649)) {
  fputs("_GETFIELD_INT_ILOAD_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399309 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399310 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399311 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399312 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399313 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 650)) {
  fputs("_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399314 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399315 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399316 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399317 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399318 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399319 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 651)) {
  fputs("_ILOAD_ICONST_ISHR_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399320 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399321 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 652)) {
  fputs("_INSTANCEOF_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399322 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399323 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399324 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 653)) {
  fputs("_ALOAD_GETFIELD_CELL_CHECKCAST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399325 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399326 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399327 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399328 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399329 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 654)) {
  fputs("_DUP_GETFIELD_INT_ILOAD_IADD_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399330 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399331 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399332 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399333 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399334 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 655)) {
  fputs("_GETFIELD_CELL_DUP_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399335 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399336 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399337 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 656)) {
  fputs("_ALOAD_GETFIELD_CELL_DUP_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399338 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399339 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399340 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399341 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 657)) {
  fputs("_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399342 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399343 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399344 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 658)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399345 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399346 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399347 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399348 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399349 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 659)) {
  fputs("_ASTORE_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399350 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399351 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399352 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399353 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 660)) {
  fputs("_ASTORE_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399354 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399355 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399356 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 661)) {
  fputs("_ALOAD_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399357 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399358 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399359 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399360 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 662)) {
  fputs("_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399361 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399362 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399363 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 663)) {
  fputs("_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_MONITORENTER_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399364 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399365 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399366 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 664)) {
  fputs("_ICONST_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399367 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399368 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 665)) {
  fputs("_ALOAD_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399369 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399370 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399371 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399372 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399373 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 666)) {
  fputs("_DUP_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399374 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 667)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399375 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399376 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399377 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399378 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 668)) {
  fputs("_ALOAD_ICONST_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399379 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399380 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399381 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 669)) {
  fputs("_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399382 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399383 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399384 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399385 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399386 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 670)) {
  fputs("_ALOAD_GETFIELD_INT_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399387 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399388 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399389 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399390 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 671)) {
  fputs("_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399391 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399392 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399393 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399394 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399395 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399396 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 672)) {
  fputs("_CHECKCAST_ASTORE_ALOAD_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399397 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399398 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399399 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399400 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399401 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 673)) {
  fputs("_GETSTATIC_CELL_ASTORE_ALOAD_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399402 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399403 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399404 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399405 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399406 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 674)) {
  fputs("_ICONST_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399407 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399408 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399409 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 675)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399410 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399411 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399412 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399413 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399414 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399415 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 676)) {
  fputs("_GETFIELD_CELL_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399416 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399417 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399418 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399419 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 677)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399420 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399421 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399422 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399423 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399424 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 678)) {
  fputs("_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399425 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399426 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399427 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399428 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399429 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 679)) {
  fputs("_GETFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399430 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399431 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399432 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399433 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 680)) {
  fputs("_ICONST_ICONST_ICONST_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399434 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399435 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399436 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399437 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399438 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 681)) {
  fputs("_ISTORE_ALOAD_GETFIELD_INT_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399439 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399440 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399441 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399442 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399443 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 682)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399444 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399445 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399446 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399447 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399448 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 683)) {
  fputs("_PUTFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399449 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399450 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399451 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 684)) {
  fputs("_ICONST_ISUB_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399452 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399453 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399454 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 685)) {
  fputs("_ISTORE_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399455 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399456 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 686)) {
  fputs("_ALOAD_ILOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399457 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399458 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399459 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399460 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 687)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399461 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399462 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399463 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399464 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399465 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 688)) {
  fputs("_ILOAD_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399466 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399467 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 689)) {
  fputs("_LAND_LCONST_IF_LCMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399468 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399469 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399470 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 690)) {
  fputs("_ALOAD_ILOAD_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399471 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399472 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399473 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399474 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 691)) {
  fputs("_ISTORE_ILOAD_IFGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399475 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399476 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399477 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 692)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399478 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399479 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399480 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399481 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 693)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399482 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399483 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399484 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399485 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399486 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399487 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 694)) {
  fputs("_ILOAD_ILOAD_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399488 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399489 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 695)) {
  fputs("_AALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399490 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 696)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399491 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399492 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399493 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399494 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399495 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399496 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399497 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 697)) {
  fputs("_ILOAD_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399498 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399499 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 698)) {
  fputs("_DUP_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399500 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399501 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 699)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399502 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399503 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399504 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399505 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399506 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399507 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 700)) {
  fputs("_ACONST_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399508 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399509 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399510 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 701)) {
  fputs("_ACONST_ASTORE_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399511 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399512 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399513 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 702)) {
  fputs("_ALOAD_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399514 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399515 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399516 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 703)) {
  fputs("_ASTORE_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399517 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399518 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399519 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399520 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399521 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 704)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399522 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399523 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399524 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399525 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399526 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399527 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 705)) {
  fputs("_GETFIELD_CELL_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399528 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399529 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399530 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399531 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 706)) {
  fputs("_ALOAD_ILOAD_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399532 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399533 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399534 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 707)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399535 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399536 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399537 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399538 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399539 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399540 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399541 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 708)) {
  fputs("_PUTSTATIC_CELL_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399542 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399543 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399544 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399545 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 709)) {
  fputs("_ILOAD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399546 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399547 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 710)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399548 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399549 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399550 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399551 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399552 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 711)) {
  fputs("_GETFIELD_INT_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399553 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399554 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399555 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 712)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399556 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399557 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399558 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399559 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399560 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 713)) {
  fputs("_ILOAD_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399561 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399562 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399563 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 714)) {
  fputs("_ILOAD_ILOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399564 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399565 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399566 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399567 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 715)) {
  fputs("_ILOAD_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399568 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399569 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 716)) {
  fputs("_ISTORE_ILOAD_ICONST_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399570 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399571 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399572 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399573 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 717)) {
  fputs("_ALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399574 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399575 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 718)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399576 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399577 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399578 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399579 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399580 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399581 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 719)) {
  fputs("_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399582 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399583 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399584 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399585 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399586 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 720)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399587 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399588 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399589 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399590 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399591 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 721)) {
  fputs("_POP_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399592 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 722)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399593 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399594 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399595 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399596 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399597 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399598 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 723)) {
  fputs("_PUTFIELD_INT_ALOAD_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399599 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399600 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399601 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 724)) {
  fputs("_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399602 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399603 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399604 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399605 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399606 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 725)) {
  fputs("_ALOAD_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399607 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399608 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399609 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 726)) {
  fputs("_ALOAD_ILOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399610 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399611 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399612 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 727)) {
  fputs("_LCONST_LAND_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399613 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399614 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399615 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399616 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 728)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399617 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399618 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399619 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399620 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399621 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399622 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399623 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 729)) {
  fputs("_IASTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399624 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399625 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 730)) {
  fputs("_AASTORE_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399626 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399627 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 731)) {
  fputs("_CHECKCAST_ASTORE_ALOAD_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399628 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399629 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399630 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399631 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399632 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 732)) {
  fputs("_DUP_ICONST_LCONST_LASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399633 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399634 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399635 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 733)) {
  fputs("_ICONST_LCONST_LASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399636 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399637 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399638 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 734)) {
  fputs("_ICONST_LCONST_LASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399639 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399640 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399641 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399642 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 735)) {
  fputs("_ILOAD_ALOAD_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399643 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399644 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 736)) {
  fputs("_ILOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399645 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399646 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399647 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 737)) {
  fputs("_LASTORE_DUP_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 738)) {
  fputs("_LASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399648 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 739)) {
  fputs("_LASTORE_DUP_ICONST_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399649 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399650 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399651 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 740)) {
  fputs("_LASTORE_DUP_ICONST_LCONST_LASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399652 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399653 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399654 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 741)) {
  fputs("_LCONST_LASTORE_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399655 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399656 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 742)) {
  fputs("_LCONST_LASTORE_DUP_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399657 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399658 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399659 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 743)) {
  fputs("_LCONST_LASTORE_DUP_ICONST_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399660 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399661 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399662 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399663 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399664 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 744)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399665 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399666 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399667 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399668 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399669 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399670 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 745)) {
  fputs("_LASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399671 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 746)) {
  fputs("_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399672 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399673 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399674 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399675 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399676 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 747)) {
  fputs("_ICONST_IALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399677 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399678 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 748)) {
  fputs("_ILOAD_IF_ICMPGT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399679 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399680 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 749)) {
  fputs("_CALOAD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399681 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 750)) {
  fputs("_LCONST_LAND_LCONST_IF_LCMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399682 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399683 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399684 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399685 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399686 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 751)) {
  fputs("_ICONST_IMUL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399687 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 752)) {
  fputs("_ILOAD_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399688 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399689 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 753)) {
  fputs("_ISUB_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399690 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399691 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399692 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 754)) {
  fputs("_CHECKCAST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399693 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399694 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399695 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399696 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 755)) {
  fputs("_DUP_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399697 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399698 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399699 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399700 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399701 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 756)) {
  fputs("_ALOAD_CHECKCAST_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399702 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399703 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399704 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399705 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 757)) {
  fputs("_GETFIELD_INT_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399706 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399707 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399708 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 758)) {
  fputs("_ILOAD_BALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399709 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 759)) {
  fputs("_AALOAD_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399710 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399711 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 760)) {
  fputs("_PUTSTATIC_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399712 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399713 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399714 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 761)) {
  fputs("_LASTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399715 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399716 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 762)) {
  fputs("_ICONST_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399717 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399718 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399719 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 763)) {
  fputs("_ILOAD_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399720 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399721 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399722 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 764)) {
  fputs("_ILOAD_IADD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399723 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399724 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 765)) {
  fputs("_ISTORE_ILOAD_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399725 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399726 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399727 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399728 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 766)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399729 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399730 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399731 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399732 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399733 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 767)) {
  fputs("_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399734 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399735 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399736 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399737 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399738 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399739 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 768)) {
  fputs("_GETSTATIC_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399740 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399741 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399742 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 769)) {
  fputs("_IADD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399743 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 770)) {
  fputs("_GETFIELD_CELL_ALOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399744 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399745 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399746 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399747 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399748 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 771)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399749 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399750 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399751 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399752 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399753 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399754 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 772)) {
  fputs("_ILOAD_ALOAD_GETFIELD_INT_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399755 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399756 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399757 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399758 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399759 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 773)) {
  fputs("_ACONST_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399760 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399761 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 774)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399762 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399763 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399764 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399765 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399766 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399767 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 775)) {
  fputs("_ALOAD_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399768 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399769 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399770 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399771 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399772 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399773 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399774 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 776)) {
  fputs("_LASTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399775 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 777)) {
  fputs("_GETFIELD_CELL_ICONST_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399776 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399777 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399778 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 778)) {
  fputs("_IADD_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399779 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399780 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 779)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399781 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399782 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399783 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399784 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399785 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 780)) {
  fputs("_DUP_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399786 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399787 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399788 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 781)) {
  fputs("_GETFIELD_CELL_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399789 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399790 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399791 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399792 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 782)) {
  fputs("_GETFIELD_CELL_ILOAD_ALOAD_AASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399793 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399794 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399795 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399796 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 783)) {
  fputs("_AALOAD_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399797 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399798 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 784)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399799 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399800 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399801 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399802 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399803 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399804 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 785)) {
  fputs("_GETFIELD_CELL_ILOAD_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399805 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399806 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399807 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399808 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399809 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 786)) {
  fputs("_ILOAD_AALOAD_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399810 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399811 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399812 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 787)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399813 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399814 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399815 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399816 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399817 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399818 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 788)) {
  fputs("_ALOAD_ALOAD_MONITOREXIT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399819 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399820 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 789)) {
  fputs("_GETFIELD_CELL_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399821 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399822 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399823 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399824 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 790)) {
  fputs("_ILOAD_IFLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399825 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399826 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 791)) {
  fputs("_ISUB_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399827 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399828 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 792)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399829 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399830 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399831 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399832 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399833 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 793)) {
  fputs("_IADD_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399834 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399835 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 794)) {
  fputs("_IALOAD_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399836 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399837 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 795)) {
  fputs("_PUTFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399838 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399839 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399840 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 796)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399841 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399842 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399843 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399844 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399845 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399846 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 797)) {
  fputs("_ILOAD_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399847 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 798)) {
  fputs("_ALOAD_INSTANCEOF_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399848 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399849 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399850 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399851 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 799)) {
  fputs("_DUP_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399852 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399853 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 800)) {
  fputs("_IADD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399854 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 801)) {
  fputs("_IAND_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399855 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 802)) {
  fputs("_ISTORE_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399856 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399857 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399858 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399859 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399860 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 803)) {
  fputs("_IALOAD_ALOAD_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399861 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399862 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 804)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399863 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399864 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399865 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399866 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 805)) {
  fputs("_ALOAD_ILOAD_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399867 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399868 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 806)) {
  fputs("_MONITORENTER_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399869 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399870 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399871 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 807)) {
  fputs("_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399872 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399873 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399874 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399875 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399876 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399877 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 808)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399878 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399879 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399880 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399881 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399882 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399883 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 809)) {
  fputs("_ARRAYLENGTH_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399884 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 810)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399885 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399886 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399887 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399888 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399889 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 811)) {
  fputs("_ISTORE_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399890 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399891 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399892 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 812)) {
  fputs("_ICONST_ISTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399893 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399894 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399895 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399896 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399897 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 813)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399898 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399899 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399900 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399901 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399902 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 814)) {
  fputs("_ALOAD_GETFIELD_CELL_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399903 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399904 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399905 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399906 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399907 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 815)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399908 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399909 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399910 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399911 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 816)) {
  fputs("_ISTORE_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399912 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399913 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399914 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399915 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399916 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 817)) {
  fputs("_ALOAD_GETFIELD_CELL_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399917 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399918 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399919 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399920 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399921 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 818)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399922 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399923 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399924 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399925 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399926 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 819)) {
  fputs("_ARRAYLENGTH_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399927 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 820)) {
  fputs("_GETFIELD_INT_ICONST_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399928 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399929 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399930 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399931 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 821)) {
  fputs("_ISHL_IOR_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 822)) {
  fputs("_ACONST_ASTORE_ACONST_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399932 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399933 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399934 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399935 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 823)) {
  fputs("_ICONST_ISUB_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399936 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399937 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 824)) {
  fputs("_DUP_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399938 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399939 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 825)) {
  fputs("_ICONST_PUTFIELD_INT_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399940 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399941 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399942 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399943 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 826)) {
  fputs("_IINC_ILOAD_IFGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399944 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399945 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399946 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399947 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 827)) {
  fputs("_GETSTATIC_CELL_IFNONNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399948 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399949 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399950 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 828)) {
  fputs("_ILOAD_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399951 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399952 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 829)) {
  fputs("_ALOAD_ALOAD_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399953 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399954 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399955 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399956 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 830)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399957 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399958 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399959 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399960 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399961 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 831)) {
  fputs("_ICONST_IF_ICMPGT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399962 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399963 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 832)) {
  fputs("_ICONST_ISUB_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399964 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399965 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399966 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399967 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 833)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399968 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399969 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399970 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399971 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399972 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399973 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305399974 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 834)) {
  fputs("_ICONST_IAND_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399975 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399976 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 835)) {
  fputs("_LSTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399977 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399978 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 836)) {
  fputs("_ASTORE_ICONST_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399979 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399980 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399981 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399982 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 837)) {
  fputs("_IADD_PUTFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399983 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399984 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399985 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399986 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399987 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 838)) {
  fputs("_ICONST_BALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399988 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 839)) {
  fputs("_ALOAD_MONITORENTER_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399989 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399990 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399991 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399992 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 840)) {
  fputs("_CHECKCAST_ASTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399993 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305399994 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305399995 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305399996 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305399997 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305399998 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 841)) {
  fputs("_ASTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305399999 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400000 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400001 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400002 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 842)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400003 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400004 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400005 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400006 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400007 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400008 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400009 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 843)) {
  fputs("_GETFIELD_INT_IFLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400010 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400011 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400012 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 844)) {
  fputs("_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400013 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400014 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400015 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400016 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400017 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400018 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 845)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400019 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400020 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400021 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400022 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400023 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400024 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 846)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400025 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400026 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400027 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400028 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400029 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 847)) {
  fputs("_LLOAD_LLOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400030 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400031 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 848)) {
  fputs("_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400032 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400033 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400034 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400035 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400036 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400037 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400038 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 849)) {
  fputs("_ALOAD_GETFIELD_INT_IFLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400039 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400040 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400041 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400042 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 850)) {
  fputs("_BASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400043 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 851)) {
  fputs("_ICONST_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400044 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400045 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400046 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 852)) {
  fputs("_ILOAD_ICONST_IAND_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400047 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400048 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400049 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 853)) {
  fputs("_PUTFIELD_INT_ALOAD_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400050 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400051 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400052 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400053 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400054 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400055 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 854)) {
  fputs("_ALOAD_ALOAD_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400056 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400057 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400058 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400059 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 855)) {
  fputs("_DUP_X1_ICONST_IADD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400060 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400061 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400062 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400063 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 856)) {
  fputs("_ICONST_ALOAD_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400064 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400065 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400066 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400067 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 857)) {
  fputs("_ICONST_ISTORE_ICONST_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400068 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400069 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400070 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400071 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400072 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 858)) {
  fputs("_ILOAD_IFGT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400073 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400074 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 859)) {
  fputs("_IAND_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400075 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 860)) {
  fputs("_ICONST_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400076 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400077 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 861)) {
  fputs("_ILOAD_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400078 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400079 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400080 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 862)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400081 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400082 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400083 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400084 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400085 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 863)) {
  fputs("_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400086 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400087 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400088 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400089 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400090 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400091 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 864)) {
  fputs("_ARRAYLENGTH_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400092 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 865)) {
  fputs("_GETFIELD_CELL_INSTANCEOF_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400093 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400094 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400095 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400096 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 866)) {
  fputs("_ICONST_BALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400097 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400098 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 867)) {
  fputs("_ICONST_IAND_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400099 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400100 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 868)) {
  fputs("_ILOAD_AALOAD_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400101 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400102 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400103 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 869)) {
  fputs("_ILOAD_ALOAD_GETFIELD_INT_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400104 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400105 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400106 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400107 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 870)) {
  fputs("_ALOAD_GETFIELD_CELL_INSTANCEOF_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400108 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400109 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400110 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400111 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400112 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 871)) {
  fputs("_ILOAD_AALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400113 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400114 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 872)) {
  fputs("_ILOAD_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400115 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400116 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400117 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400118 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400119 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 873)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400120 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400121 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400122 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400123 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400124 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400125 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400126 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 874)) {
  fputs("_PUTFIELD_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400127 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400128 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400129 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 875)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400130 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400131 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400132 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400133 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400134 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 876)) {
  fputs("_ILOAD_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400135 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400136 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400137 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 877)) {
  fputs("_ALOAD_ICONST_BALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400138 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400139 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 878)) {
  fputs("_ALOAD_ILOAD_AALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400140 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400141 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400142 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 879)) {
  fputs("_ALOAD_ICONST_BALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400143 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400144 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400145 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 880)) {
  fputs("_ASTORE_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400146 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400147 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400148 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400149 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 881)) {
  fputs("_GETFIELD_CELL_ARRAYLENGTH_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400150 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400151 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400152 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 882)) {
  fputs("_ALOAD_GETFIELD_CELL_ARRAYLENGTH_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400153 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400154 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400155 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400156 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 883)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400157 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400158 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400159 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400160 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400161 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400162 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400163 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 884)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400164 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400165 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400166 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400167 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400168 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400169 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 885)) {
  fputs("_ICONST_ISHL_IOR_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400170 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 886)) {
  fputs("_ALOAD_ARRAYLENGTH_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400171 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400172 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 887)) {
  fputs("_ALOAD_CHECKCAST_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400173 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400174 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400175 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400176 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400177 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 888)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400178 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400179 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400180 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400181 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400182 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 889)) {
  fputs("_ALOAD_GETFIELD_INT_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400183 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400184 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400185 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400186 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 890)) {
  fputs("_ARRAYLENGTH_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400187 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 891)) {
  fputs("_GETFIELD_CELL_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400188 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400189 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400190 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400191 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400192 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 892)) {
  fputs("_ICONST_IADD_PUTFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400193 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400194 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400195 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400196 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400197 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400198 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 893)) {
  fputs("_ACONST_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400199 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400200 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400201 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400202 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400203 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 894)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400204 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400205 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400206 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400207 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400208 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400209 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 895)) {
  fputs("_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400210 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400211 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400212 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 896)) {
  fputs("_ICONST_BALOAD_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400213 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400214 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 897)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_ACONST_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400215 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400216 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400217 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400218 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400219 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400220 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400221 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 898)) {
  fputs("_ILOAD_ILOAD_IADD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400222 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400223 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400224 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 899)) {
  fputs("_ALOAD_ALOAD_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400225 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400226 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400227 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400228 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 900)) {
  fputs("_ALOAD_PUTFIELD_CELL_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400229 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400230 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400231 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400232 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400233 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 901)) {
  fputs("_ASTORE_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400234 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400235 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400236 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 902)) {
  fputs("_IADD_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400237 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400238 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400239 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400240 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400241 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 903)) {
  fputs("_ILOAD_ILOAD_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400242 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400243 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400244 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 904)) {
  fputs("_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400245 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400246 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400247 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400248 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400249 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400250 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 905)) {
  fputs("_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400251 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400252 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400253 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400254 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400255 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400256 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400257 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 906)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_CELL_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400258 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400259 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400260 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400261 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400262 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400263 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400264 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 907)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400265 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400266 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400267 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400268 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400269 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400270 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 908)) {
  fputs("_MONITORENTER_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400271 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400272 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 909)) {
  fputs("_AALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400273 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 910)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400274 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400275 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400276 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400277 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 911)) {
  fputs("_CALOAD_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400278 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400279 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 912)) {
  fputs("_ACONST_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400280 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400281 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400282 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 913)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400283 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400284 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400285 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400286 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400287 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400288 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400289 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 914)) {
  fputs("_GETFIELD_CELL_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400290 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400291 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400292 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400293 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 915)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPLT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400294 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400295 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400296 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400297 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400298 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400299 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 916)) {
  fputs("_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400300 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400301 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400302 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400303 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400304 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400305 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 917)) {
  fputs("_ICONST_IALOAD_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400306 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400307 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400308 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 918)) {
  fputs("_ICONST_IALOAD_ALOAD_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400309 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400310 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400311 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 919)) {
  fputs("_ICONST_ISUB_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400312 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400313 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400314 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 920)) {
  fputs("_ILOAD_I2F_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400315 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 921)) {
  fputs("_PUTFIELD_INT_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400316 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400317 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400318 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400319 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400320 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400321 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 922)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400322 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400323 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400324 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400325 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400326 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400327 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 923)) {
  fputs("_ALOAD_ICONST_BALOAD_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400328 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400329 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400330 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 924)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400331 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400332 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400333 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400334 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400335 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400336 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400337 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 925)) {
  fputs("_GETFIELD_INT_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400338 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400339 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400340 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 926)) {
  fputs("_ILOAD_ICONST_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400341 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400342 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400343 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 927)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400344 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400345 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400346 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400347 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400348 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400349 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400350 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400351 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 928)) {
  fputs("_DUP_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400352 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400353 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 929)) {
  fputs("_ILOAD_ILOAD_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400354 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400355 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400356 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 930)) {
  fputs("_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400357 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400358 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400359 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400360 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400361 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400362 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400363 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 931)) {
  fputs("_ALOAD_GETFIELD_INT_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400364 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400365 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400366 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400367 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 932)) {
  fputs("_ISTORE_ALOAD_GETFIELD_INT_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400368 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400369 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400370 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400371 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400372 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400373 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 933)) {
  fputs("_ALOAD_ALOAD_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400374 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400375 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400376 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 934)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400377 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400378 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400379 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400380 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400381 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400382 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400383 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 935)) {
  fputs("_ALOAD_MONITORENTER_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400384 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400385 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400386 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 936)) {
  fputs("_GETFIELD_CELL_ICONST_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400387 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400388 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400389 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400390 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400391 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400392 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 937)) {
  fputs("_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400393 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400394 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400395 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400396 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 938)) {
  fputs("_ALOAD_ILOAD_ILOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400397 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400398 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400399 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400400 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400401 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 939)) {
  fputs("_FMUL_FADD_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 940)) {
  fputs("_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400402 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400403 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400404 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400405 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400406 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400407 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400408 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 941)) {
  fputs("_MONITORENTER_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400409 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400410 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400411 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400412 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 942)) {
  fputs("_IADD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400413 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400414 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400415 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 943)) {
  fputs("_ILOAD_FADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400416 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 944)) {
  fputs("_ILOAD_ISUB_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400417 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400418 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 945)) {
  fputs("_LSTORE_LLOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400419 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400420 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 946)) {
  fputs("_GETSTATIC_CELL_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400421 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400422 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400423 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400424 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 947)) {
  fputs("_ICONST_ALOAD_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400425 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400426 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400427 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400428 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400429 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 948)) {
  fputs("_ICONST_IADD_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400430 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400431 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400432 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400433 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400434 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400435 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 949)) {
  fputs("_ILOAD_ILOAD_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400436 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400437 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400438 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 950)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400439 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400440 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400441 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400442 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400443 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 951)) {
  fputs("_FMUL_F2I_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 952)) {
  fputs("_GETFIELD_CELL_ASTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400444 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400445 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400446 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400447 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400448 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400449 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 953)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400450 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400451 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400452 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400453 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400454 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400455 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 954)) {
  fputs("_ASTORE_ACONST_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400456 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400457 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400458 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400459 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 955)) {
  fputs("_GETFIELD_INT_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400460 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400461 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400462 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400463 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 956)) {
  fputs("_ICONST_ILOAD_IF_ICMPGT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400464 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400465 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400466 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 957)) {
  fputs("_ICONST_ISTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400467 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400468 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400469 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400470 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 958)) {
  fputs("_IINC_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400471 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400472 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 959)) {
  fputs("_ILOAD_IINC_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400473 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400474 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400475 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 960)) {
  fputs("_ISTORE_ALOAD_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400476 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400477 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400478 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 961)) {
  fputs("_ALOAD_GETFIELD_INT_ILOAD_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400479 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400480 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400481 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400482 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 962)) {
  fputs("_FADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400483 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 963)) {
  fputs("_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400484 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400485 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400486 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400487 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400488 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400489 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 964)) {
  fputs("_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400490 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400491 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400492 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400493 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400494 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400495 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400496 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400497 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 965)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400498 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400499 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400500 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400501 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400502 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400503 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400504 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400505 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 966)) {
  fputs("_ASTORE_ALOAD_GETFIELD_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400506 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400507 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400508 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400509 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400510 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 967)) {
  fputs("_ASTORE_ILOAD_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400511 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400512 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400513 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 968)) {
  fputs("_CHECKCAST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400514 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400515 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400516 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 969)) {
  fputs("_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400517 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400518 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400519 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400520 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400521 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400522 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 970)) {
  fputs("_ICONST_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400523 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400524 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400525 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 971)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_IINC_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400526 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400527 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400528 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400529 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400530 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400531 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 972)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400532 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400533 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400534 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400535 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400536 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400537 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400538 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400539 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 973)) {
  fputs("_GETFIELD_CELL_ILOAD_IINC_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400540 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400541 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400542 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400543 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400544 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 974)) {
  fputs("_GETSTATIC_CELL_IF_ACMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400545 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400546 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400547 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 975)) {
  fputs("_ICONST_ALOAD_GETFIELD_CELL_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400548 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400549 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400550 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400551 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400552 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 976)) {
  fputs("_ILOAD_ILOAD_FADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400553 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400554 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 977)) {
  fputs("_ISTORE_ICONST_ISTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400555 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400556 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400557 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400558 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 978)) {
  fputs("_PUTFIELD_LONG_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400559 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400560 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400561 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 979)) {
  fputs("_ILOAD_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400562 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400563 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 980)) {
  fputs("_GETSTATIC_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400564 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400565 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400566 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400567 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 981)) {
  fputs("_ILOAD_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400568 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400569 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 982)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400570 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400571 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400572 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400573 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400574 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400575 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 983)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400576 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400577 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400578 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400579 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400580 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 984)) {
  fputs("_GETFIELD_CELL_ICONST_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400581 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400582 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400583 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400584 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400585 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 985)) {
  fputs("_IADD_PUTFIELD_INT_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400586 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400587 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400588 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 986)) {
  fputs("_ICONST_ALOAD_ICONST_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400589 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400590 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400591 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400592 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400593 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400594 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 987)) {
  fputs("_POP_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400595 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400596 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 988)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400597 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400598 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400599 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400600 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400601 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400602 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 989)) {
  fputs("_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400603 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400604 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400605 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400606 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400607 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400608 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400609 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 990)) {
  fputs("_ICONST_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400610 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400611 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400612 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400613 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400614 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400615 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 991)) {
  fputs("_PUTFIELD_CELL_ICONST_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400616 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400617 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400618 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400619 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 992)) {
  fputs("_ALOAD_ACONST_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400620 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400621 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400622 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400623 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 993)) {
  fputs("_ALOAD_ALOAD_ALOAD_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400624 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400625 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400626 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400627 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400628 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 994)) {
  fputs("_GETFIELD_CELL_INSTANCEOF_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400629 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400630 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400631 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400632 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400633 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 995)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400634 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400635 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400636 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400637 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400638 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400639 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 996)) {
  fputs("_ISUB_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400640 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 997)) {
  fputs("_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400641 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400642 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400643 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400644 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400645 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400646 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 998)) {
  fputs("_ALOAD_GETFIELD_CELL_INSTANCEOF_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400647 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400648 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400649 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400650 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400651 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400652 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 999)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400653 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400654 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400655 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400656 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400657 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400658 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400659 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1000)) {
  fputs("_ASTORE_GETSTATIC_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400660 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400661 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400662 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400663 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1001)) {
  fputs("_GETFIELD_CELL_ICONST_ALOAD_ICONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400664 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400665 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400666 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400667 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400668 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400669 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1002)) {
  fputs("_GETSTATIC_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400670 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400671 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400672 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400673 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400674 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1003)) {
  fputs("_ICONST_IADD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400675 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400676 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1004)) {
  fputs("_ILOAD_ICONST_IF_ICMPGT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400677 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400678 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400679 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1005)) {
  fputs("_ISUB_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400680 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1006)) {
  fputs("_ALOAD_GETFIELD_INT_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400681 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400682 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400683 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400684 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400685 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1007)) {
  fputs("_GETFIELD_CELL_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400686 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400687 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400688 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400689 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1008)) {
  fputs("_GETSTATIC_INT_ICONST_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400690 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400691 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400692 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400693 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1009)) {
  fputs("_ISTORE_IINC_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400694 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400695 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400696 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1010)) {
  fputs("_PUTSTATIC_CELL_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400697 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400698 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400699 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400700 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1011)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400701 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400702 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400703 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400704 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400705 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1012)) {
  fputs("_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400706 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400707 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400708 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400709 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400710 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400711 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400712 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400713 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1013)) {
  fputs("_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400714 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400715 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400716 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400717 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400718 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1014)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400719 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400720 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400721 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400722 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400723 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400724 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1015)) {
  fputs("_ALOAD_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400725 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400726 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400727 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400728 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400729 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400730 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400731 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1016)) {
  fputs("_ALOAD_GETFIELD_CELL_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400732 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400733 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400734 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400735 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1017)) {
  fputs("_ASTORE_ALOAD_INSTANCEOF_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400736 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400737 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400738 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400739 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1018)) {
  fputs("_DUP_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400740 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400741 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400742 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1019)) {
  fputs("_GETFIELD_CELL_ICONST_IALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400743 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400744 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400745 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1020)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400746 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400747 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400748 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400749 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400750 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1021)) {
  fputs("_ISUB_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400751 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400752 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1022)) {
  fputs("_ALOAD_ALOAD_ILOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400753 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400754 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400755 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400756 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1023)) {
  fputs("_ALOAD_ILOAD_AALOAD_ASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400757 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400758 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400759 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1024)) {
  fputs("_ISTORE_ILOAD_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400760 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400761 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400762 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1025)) {
  fputs("_LLOAD_LCONST_LAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400763 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400764 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400765 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1026)) {
  fputs("_AALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400766 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1027)) {
  fputs("_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400767 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400768 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400769 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400770 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1028)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400771 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400772 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400773 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400774 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400775 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400776 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400777 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1029)) {
  fputs("_ARRAYLENGTH_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400778 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1030)) {
  fputs("_DUP_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400779 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400780 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400781 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400782 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1031)) {
  fputs("_GETFIELD_INT_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400783 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400784 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400785 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1032)) {
  fputs("_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400786 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400787 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400788 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400789 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400790 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400791 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400792 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400793 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1033)) {
  fputs("_LLOAD_LCONST_LAND_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400794 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400795 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400796 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400797 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400798 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1034)) {
  fputs("_ASTORE_ACONST_ASTORE_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400799 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400800 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400801 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400802 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1035)) {
  fputs("_GETFIELD_CELL_ILOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400803 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400804 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400805 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400806 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400807 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400808 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1036)) {
  fputs("_GETFIELD_INT_LOOKUPSWITCH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400809 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400810 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400811 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400812 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400813 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400814 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1037)) {
  fputs("_ICONST_IOR_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400815 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1038)) {
  fputs("_ILOAD_I2L_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400816 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1039)) {
  fputs("_POP_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400817 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400818 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1040)) {
  fputs("_ALOAD_AASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400819 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400820 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1041)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400821 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400822 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400823 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400824 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400825 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400826 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400827 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305400828 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1042)) {
  fputs("_IINC_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400829 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400830 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400831 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1043)) {
  fputs("_ILOAD_IADD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400832 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400833 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1044)) {
  fputs("_CHECKCAST_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400834 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400835 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400836 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400837 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400838 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1045)) {
  fputs("_GETFIELD_CELL_ILOAD_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400839 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400840 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400841 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400842 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1046)) {
  fputs("_IADD_DUP_X1_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1047)) {
  fputs("_IADD_DUP_X1_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400843 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400844 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1048)) {
  fputs("_IAND_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400845 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1049)) {
  fputs("_ICONST_PUTSTATIC_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400846 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400847 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400848 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1050)) {
  fputs("_ILOAD_AALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400849 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400850 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1051)) {
  fputs("_ACONST_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400851 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400852 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1052)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_INT_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400853 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400854 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400855 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400856 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400857 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400858 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1053)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_IADD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400859 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400860 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400861 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400862 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1054)) {
  fputs("_GETFIELD_CELL_GETFIELD_INT_IFNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400863 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400864 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400865 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400866 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400867 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1055)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400868 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400869 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400870 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400871 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400872 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400873 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1056)) {
  fputs("_IALOAD_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400874 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1057)) {
  fputs("_ILOAD_FSUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400875 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1058)) {
  fputs("_POP_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400876 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400877 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400878 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400879 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1059)) {
  fputs("_AALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400880 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400881 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1060)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400882 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400883 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400884 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400885 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400886 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400887 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400888 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1061)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_ICONST_BASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400889 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400890 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400891 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400892 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400893 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1062)) {
  fputs("_ALOAD_GETFIELD_INT_LOOKUPSWITCH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400894 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400895 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400896 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400897 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400898 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400899 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400900 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1063)) {
  fputs("_ALOAD_ICONST_LALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400901 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400902 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1064)) {
  fputs("_ASTORE_ALOAD_GETSTATIC_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400903 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400904 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400905 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400906 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1065)) {
  fputs("_GETFIELD_INT_TABLESWITCH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400907 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400908 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400909 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400910 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400911 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400912 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400913 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1066)) {
  fputs("_GETSTATIC_CELL_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400914 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400915 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1067)) {
  fputs("_ICONST_LALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400916 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1068)) {
  fputs("_ALOAD_ALOAD_ICONST_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400917 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400918 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400919 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400920 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1069)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400921 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400922 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400923 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400924 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400925 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400926 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400927 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1070)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400928 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400929 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400930 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400931 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400932 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1071)) {
  fputs("_GETSTATIC_CELL_ILOAD_AALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400933 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400934 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400935 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1072)) {
  fputs("_ICONST_ISTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400936 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400937 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400938 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400939 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400940 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1073)) {
  fputs("_ILOAD_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400941 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400942 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400943 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1074)) {
  fputs("_ILOAD_INT2CHAR_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400944 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1075)) {
  fputs("_ILOAD_TABLESWITCH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400945 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400946 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400947 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400948 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400949 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400950 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1076)) {
  fputs("_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400951 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400952 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400953 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400954 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400955 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400956 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1077)) {
  fputs("_ACONST_ACONST_ACONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400957 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400958 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400959 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400960 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1078)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400961 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400962 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400963 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400964 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400965 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1079)) {
  fputs("_ALOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400966 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400967 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400968 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1080)) {
  fputs("_ARRAYLENGTH_ICONST_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400969 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1081)) {
  fputs("_BALOAD_ICONST_IAND_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400970 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400971 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1082)) {
  fputs("_ILOAD_IADD_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400972 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400973 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400974 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400975 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1083)) {
  fputs("_ALOAD_ALOAD_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400976 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400977 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400978 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400979 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1084)) {
  fputs("_CALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400980 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1085)) {
  fputs("_ILOAD_ICONST_IF_ICMPGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400981 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400982 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400983 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1086)) {
  fputs("_ACONST_MONITOREXIT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400984 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1087)) {
  fputs("_ALOAD_ARRAYLENGTH_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400985 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400986 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1088)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400987 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400988 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400989 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400990 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400991 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400992 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1089)) {
  fputs("_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305400993 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305400994 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305400995 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305400996 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305400997 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305400998 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305400999 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1090)) {
  fputs("_ALOAD_ICONST_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401000 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401001 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401002 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401003 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401004 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401005 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401006 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1091)) {
  fputs("_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401007 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401008 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401009 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401010 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401011 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401012 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401013 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305401014 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1092)) {
  fputs("_GETFIELD_LONG_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401015 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401016 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401017 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1093)) {
  fputs("_ICONST_AALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401018 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401019 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1094)) {
  fputs("_IOR_INT2SHORT_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1095)) {
  fputs("_LLOAD_LCONST_LAND_LCONST_IF_LCMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401020 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401021 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401022 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401023 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401024 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401025 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1096)) {
  fputs("_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401026 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401027 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401028 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401029 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401030 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401031 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401032 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1097)) {
  fputs("_AALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401033 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401034 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401035 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1098)) {
  fputs("_ALOAD_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401036 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401037 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401038 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401039 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401040 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401041 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401042 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1099)) {
  fputs("_ALOAD_ARRAYLENGTH_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401043 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401044 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1100)) {
  fputs("_ALOAD_DUP_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401045 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401046 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401047 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401048 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1101)) {
  fputs("_ALOAD_GETFIELD_LONG_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401049 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401050 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401051 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401052 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1102)) {
  fputs("_ALOAD_ILOAD_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401053 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401054 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401055 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401056 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401057 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1103)) {
  fputs("_ALOAD_ILOAD_ALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401058 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401059 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401060 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401061 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1104)) {
  fputs("_DUP_GETFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401062 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401063 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401064 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1105)) {
  fputs("_ICONST_ACONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401065 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401066 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401067 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1106)) {
  fputs("_PUTFIELD_CELL_ICONST_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401068 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401069 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401070 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401071 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401072 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1107)) {
  fputs("_AASTORE_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401073 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401074 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1108)) {
  fputs("_ALOAD_ILOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401075 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401076 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401077 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401078 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1109)) {
  fputs("_ALOAD_MONITOREXIT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401079 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401080 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1110)) {
  fputs("_ASTORE_GETSTATIC_CELL_ACONST_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401081 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401082 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401083 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401084 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401085 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1111)) {
  fputs("_CHECKCAST_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401086 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401087 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401088 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401089 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1112)) {
  fputs("_GETFIELD_CELL_ILOAD_CALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401090 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401091 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401092 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1113)) {
  fputs("_IADD_ISTORE_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401093 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401094 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401095 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401096 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1114)) {
  fputs("_MONITOREXIT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401097 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1115)) {
  fputs("_ASTORE_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401098 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401099 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401100 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401101 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401102 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401103 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1116)) {
  fputs("_ISTORE_ICONST_ISTORE_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401104 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401105 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401106 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401107 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1117)) {
  fputs("_PUTFIELD_CELL_ALOAD_ICONST_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401108 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401109 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401110 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401111 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401112 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1118)) {
  fputs("_PUTFIELD_CELL_ALOAD_IFNULL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401113 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401114 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401115 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401116 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1119)) {
  fputs("_ALOAD_ILOAD_I2F_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401117 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401118 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1120)) {
  fputs("_ALOAD_PUTFIELD_CELL_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401119 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401120 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401121 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401122 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1121)) {
  fputs("_ARRAYLENGTH_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401123 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401124 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401125 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1122)) {
  fputs("_ASTORE_ALOAD_ALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401126 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401127 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401128 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401129 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401130 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1123)) {
  fputs("_DUP2_IALOAD_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1124)) {
  fputs("_DUP_ILOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401131 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401132 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1125)) {
  fputs("_GETFIELD_INT_ICONST_IADD_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401133 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401134 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401135 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1126)) {
  fputs("_GETFIELD_INT_ICONST_IADD_DUP_X1_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401136 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401137 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401138 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401139 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401140 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1127)) {
  fputs("_IADD_PUTFIELD_INT_ALOAD_DUP_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401141 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401142 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401143 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1128)) {
  fputs("_IADD_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401144 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401145 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401146 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401147 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401148 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1129)) {
  fputs("_IAND_ICONST_ISHL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401149 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1130)) {
  fputs("_ICONST_IADD_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401150 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1131)) {
  fputs("_ICONST_IADD_DUP_X1_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401151 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401152 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401153 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1132)) {
  fputs("_ICONST_IAND_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401154 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401155 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1133)) {
  fputs("_ICONST_IAND_ICONST_ISHL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401156 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401157 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1134)) {
  fputs("_ILOAD_AALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401158 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401159 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401160 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1135)) {
  fputs("_LLOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401161 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401162 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1136)) {
  fputs("_ALOAD_ICONST_IALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401163 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401164 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401165 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1137)) {
  fputs("_ARRAYLENGTH_ACONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401166 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1138)) {
  fputs("_ASTORE_ALOAD_INSTANCEOF_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401167 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401168 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401169 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401170 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401171 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1139)) {
  fputs("_DUP_GETFIELD_INT_ICONST_IADD_DUP_X1_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401172 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401173 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401174 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1140)) {
  fputs("_FADD_ISTORE_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401175 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401176 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1141)) {
  fputs("_GETFIELD_CELL_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401177 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401178 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401179 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401180 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401181 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1142)) {
  fputs("_IALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401182 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1143)) {
  fputs("_ICONST_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401183 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401184 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401185 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1144)) {
  fputs("_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401186 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401187 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401188 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401189 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1145)) {
  fputs("_ISTORE_ICONST_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401190 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401191 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401192 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1146)) {
  fputs("_ISUB_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401193 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1147)) {
  fputs("_ALOAD_GETFIELD_INT_IFGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401194 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401195 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401196 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401197 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1148)) {
  fputs("_ALOAD_GETFIELD_INT_IF_ICMPEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401198 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401199 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401200 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401201 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1149)) {
  fputs("_ALOAD_IF_ACMPNE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401202 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401203 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1150)) {
  fputs("_F2I_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401204 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401205 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1151)) {
  fputs("_FMUL_F2I_PUTFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401206 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401207 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1152)) {
  fputs("_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401208 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401209 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401210 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401211 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401212 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401213 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401214 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1153)) {
  fputs("_GETFIELD_INT_IFGE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401215 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401216 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401217 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1154)) {
  fputs("_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401218 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401219 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401220 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401221 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401222 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1155)) {
  fputs("_ALOAD_ARRAYLENGTH_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401223 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401224 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1156)) {
  fputs("_ALOAD_GETFIELD_INT_ICONST_IAND_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401225 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401226 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401227 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401228 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1157)) {
  fputs("_CALOAD_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401229 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1158)) {
  fputs("_DUP_ICONST_ICONST_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401230 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401231 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401232 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1159)) {
  fputs("_IADD_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401233 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401234 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1160)) {
  fputs("_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401235 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401236 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401237 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401238 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401239 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401240 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1161)) {
  fputs("_ILOAD_ALOAD_GETFIELD_INT_IF_ICMPLE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401241 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401242 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401243 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401244 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401245 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1162)) {
  fputs("_ILOAD_ILOAD_ISUB_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401246 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401247 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401248 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1163)) {
  fputs("_INT2BYTE_BASTORE_", vm_out);
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1164)) {
  fputs("_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401249 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401250 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401251 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401252 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401253 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401254 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1165)) {
  fputs("_ACONST_ASTORE_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401255 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401256 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401257 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401258 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401259 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1166)) {
  fputs("_ALOAD_ALOAD_PUTFIELD_CELL_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401260 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401261 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401262 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401263 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401264 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1167)) {
  fputs("_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401265 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401266 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401267 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401268 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401269 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401270 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401271 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
{
Cell _IP7;
vm_Cell2Cell(IMM_ARG(IP[7],305401272 ),_IP7);
fputs(" _IP7=", vm_out); printarg_Cell(_IP7);
}
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1168)) {
  fputs("_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401273 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401274 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401275 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401276 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401277 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401278 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1169)) {
  fputs("_ALOAD_ICONST_ALOAD_ICONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401279 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401280 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401281 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401282 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1170)) {
  fputs("_ALOAD_ICONST_LALOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401283 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401284 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401285 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401286 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1171)) {
  fputs("_ALOAD_MONITORENTER_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401287 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401288 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401289 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401290 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1172)) {
  fputs("_BALOAD_ICONST_IAND_ICONST_ISHL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401291 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401292 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1173)) {
  fputs("_DUP_X1_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401293 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401294 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1174)) {
  fputs("_FMUL_ISTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401295 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1175)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401296 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401297 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401298 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401299 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401300 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1176)) {
  fputs("_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401301 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401302 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401303 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401304 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401305 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401306 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1177)) {
  fputs("_ICONST_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401307 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401308 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401309 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401310 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401311 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
{
Cell _IP5;
vm_Cell2Cell(IMM_ARG(IP[5],305401312 ),_IP5);
fputs(" _IP5=", vm_out); printarg_Cell(_IP5);
}
{
Cell _IP6;
vm_Cell2Cell(IMM_ARG(IP[6],305401313 ),_IP6);
fputs(" _IP6=", vm_out); printarg_Cell(_IP6);
}
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1178)) {
  fputs("_ICONST_ILOAD_ISUB_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401314 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401315 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1179)) {
  fputs("_ICONST_LALOAD_LCONST_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401316 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401317 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401318 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1180)) {
  fputs("_ILOAD_ALOAD_GETFIELD_CELL_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401319 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401320 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401321 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401322 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401323 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1181)) {
  fputs("_ISTORE_ILOAD_IFEQ_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401324 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401325 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401326 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1182)) {
  fputs("_ISUB_PUTFIELD_INT_ALOAD_GETFIELD_CELL_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401327 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401328 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401329 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401330 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401331 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1183)) {
  fputs("_BALOAD_ICONST_IAND_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401332 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401333 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1184)) {
  fputs("_GETFIELD_INT_IASTORE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401334 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401335 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1185)) {
  fputs("_IADD_ALOAD_ARRAYLENGTH_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401336 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1186)) {
  fputs("_ILOAD_INT2BYTE_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401337 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1187)) {
  fputs("_ILOAD_ISTORE_GOTO_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401338 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401339 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401340 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1188)) {
  fputs("_ISTORE_ILOAD_ALOAD_GETFIELD_INT_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401341 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401342 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401343 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401344 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401345 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1189)) {
  fputs("_PUTFIELD_INT_ALOAD_ALOAD_ALOAD_", vm_out);
{
Cell _IP0;
vm_Cell2Cell(IMM_ARG(IPTOS,305401346 ),_IP0);
fputs(" _IP0=", vm_out); printarg_Cell(_IP0);
}
{
Cell _IP1;
vm_Cell2Cell(IMM_ARG(IP[1],305401347 ),_IP1);
fputs(" _IP1=", vm_out); printarg_Cell(_IP1);
}
{
Cell _IP2;
vm_Cell2Cell(IMM_ARG(IP[2],305401348 ),_IP2);
fputs(" _IP2=", vm_out); printarg_Cell(_IP2);
}
{
Cell _IP3;
vm_Cell2Cell(IMM_ARG(IP[3],305401349 ),_IP3);
fputs(" _IP3=", vm_out); printarg_Cell(_IP3);
}
{
Cell _IP4;
vm_Cell2Cell(IMM_ARG(IP[4],305401350 ),_IP4);
fputs(" _IP4=", vm_out); printarg_Cell(_IP4);
}
  ip += 6;
  goto _endif_;
}
