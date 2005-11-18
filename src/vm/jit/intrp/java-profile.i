if (VM_IS_INST(*ip, 0)) {
  add_inst(b, "ICONST");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1)) {
  add_inst(b, "LCONST");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 2)) {
  add_inst(b, "ACONST");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 3)) {
  add_inst(b, "ACONST1");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 4)) {
  add_inst(b, "PATCHER_ACONST");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 5)) {
  add_inst(b, "ILOAD");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 6)) {
  add_inst(b, "LLOAD");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 7)) {
  add_inst(b, "ALOAD");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 8)) {
  add_inst(b, "IALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 9)) {
  add_inst(b, "LALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 10)) {
  add_inst(b, "AALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 11)) {
  add_inst(b, "BALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 12)) {
  add_inst(b, "CALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 13)) {
  add_inst(b, "SALOAD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 14)) {
  add_inst(b, "ISTORE");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 15)) {
  add_inst(b, "LSTORE");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 16)) {
  add_inst(b, "ASTORE");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 17)) {
  add_inst(b, "IASTORE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 18)) {
  add_inst(b, "LASTORE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 19)) {
  add_inst(b, "AASTORE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 20)) {
  add_inst(b, "BASTORE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 21)) {
  add_inst(b, "CASTORE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 22)) {
  add_inst(b, "POP");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 23)) {
  add_inst(b, "POP2");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 24)) {
  add_inst(b, "DUP");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 25)) {
  add_inst(b, "DUP_X1");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 26)) {
  add_inst(b, "DUP_X2");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 27)) {
  add_inst(b, "DUP2");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 28)) {
  add_inst(b, "DUP2_X1");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 29)) {
  add_inst(b, "DUP2_X2");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 30)) {
  add_inst(b, "SWAP");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 31)) {
  add_inst(b, "IADD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 32)) {
  add_inst(b, "LADD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 33)) {
  add_inst(b, "FADD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 34)) {
  add_inst(b, "DADD");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 35)) {
  add_inst(b, "ISUB");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 36)) {
  add_inst(b, "LSUB");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 37)) {
  add_inst(b, "FSUB");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 38)) {
  add_inst(b, "DSUB");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 39)) {
  add_inst(b, "IMUL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 40)) {
  add_inst(b, "LMUL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 41)) {
  add_inst(b, "FMUL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 42)) {
  add_inst(b, "DMUL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 43)) {
  add_inst(b, "IDIV");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 44)) {
  add_inst(b, "IDIVPOW2");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 45)) {
  add_inst(b, "LDIV");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 46)) {
  add_inst(b, "LDIVPOW2");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 47)) {
  add_inst(b, "FDIV");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 48)) {
  add_inst(b, "DDIV");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 49)) {
  add_inst(b, "IREM");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 50)) {
  add_inst(b, "IREMPOW2");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 51)) {
  add_inst(b, "LREM");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 52)) {
  add_inst(b, "LREMPOW2");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 53)) {
  add_inst(b, "FREM");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 54)) {
  add_inst(b, "DREM");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 55)) {
  add_inst(b, "INEG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 56)) {
  add_inst(b, "LNEG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 57)) {
  add_inst(b, "FNEG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 58)) {
  add_inst(b, "DNEG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 59)) {
  add_inst(b, "ISHL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 60)) {
  add_inst(b, "LSHL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 61)) {
  add_inst(b, "ISHR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 62)) {
  add_inst(b, "LSHR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 63)) {
  add_inst(b, "IUSHR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 64)) {
  add_inst(b, "LUSHR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 65)) {
  add_inst(b, "IAND");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 66)) {
  add_inst(b, "LAND");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 67)) {
  add_inst(b, "IOR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 68)) {
  add_inst(b, "LOR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 69)) {
  add_inst(b, "IXOR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 70)) {
  add_inst(b, "LXOR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 71)) {
  add_inst(b, "IINC");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 72)) {
  add_inst(b, "I2L");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 73)) {
  add_inst(b, "I2F");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 74)) {
  add_inst(b, "I2D");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 75)) {
  add_inst(b, "L2I");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 76)) {
  add_inst(b, "L2F");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 77)) {
  add_inst(b, "L2D");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 78)) {
  add_inst(b, "F2I");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 79)) {
  add_inst(b, "F2L");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 80)) {
  add_inst(b, "F2D");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 81)) {
  add_inst(b, "D2I");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 82)) {
  add_inst(b, "D2L");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 83)) {
  add_inst(b, "D2F");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 84)) {
  add_inst(b, "INT2BYTE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 85)) {
  add_inst(b, "INT2CHAR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 86)) {
  add_inst(b, "INT2SHORT");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 87)) {
  add_inst(b, "LCMP");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 88)) {
  add_inst(b, "FCMPL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 89)) {
  add_inst(b, "FCMPG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 90)) {
  add_inst(b, "DCMPL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 91)) {
  add_inst(b, "DCMPG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 92)) {
  add_inst(b, "IFEQ");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 93)) {
  add_inst(b, "IFNE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 94)) {
  add_inst(b, "IFLT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 95)) {
  add_inst(b, "IFGE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 96)) {
  add_inst(b, "IFGT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 97)) {
  add_inst(b, "IFLE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 98)) {
  add_inst(b, "IF_ICMPEQ");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 99)) {
  add_inst(b, "IF_ICMPNE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 100)) {
  add_inst(b, "IF_ICMPLT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 101)) {
  add_inst(b, "IF_ICMPGE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 102)) {
  add_inst(b, "IF_ICMPGT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 103)) {
  add_inst(b, "IF_ICMPLE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 104)) {
  add_inst(b, "IF_LCMPEQ");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 105)) {
  add_inst(b, "IF_LCMPNE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 106)) {
  add_inst(b, "IF_LCMPLT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 107)) {
  add_inst(b, "IF_LCMPGE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 108)) {
  add_inst(b, "IF_LCMPGT");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 109)) {
  add_inst(b, "IF_LCMPLE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 110)) {
  add_inst(b, "IF_ACMPEQ");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 111)) {
  add_inst(b, "IF_ACMPNE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 112)) {
  add_inst(b, "GOTO");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 113)) {
  add_inst(b, "JSR");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 114)) {
  add_inst(b, "RET");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 115)) {
  add_inst(b, "TABLESWITCH");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 116)) {
  add_inst(b, "LOOKUPSWITCH");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 117)) {
  add_inst(b, "IRETURN");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 118)) {
  add_inst(b, "LRETURN");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 119)) {
  add_inst(b, "RETURN");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 120)) {
  add_inst(b, "GETSTATIC_CELL");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 121)) {
  add_inst(b, "GETSTATIC_INT");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 122)) {
  add_inst(b, "GETSTATIC_LONG");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 123)) {
  add_inst(b, "PUTSTATIC_CELL");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 124)) {
  add_inst(b, "PUTSTATIC_INT");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 125)) {
  add_inst(b, "PUTSTATIC_LONG");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 126)) {
  add_inst(b, "GETFIELD_CELL");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 127)) {
  add_inst(b, "GETFIELD_INT");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 128)) {
  add_inst(b, "GETFIELD_LONG");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 129)) {
  add_inst(b, "PUTFIELD_CELL");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 130)) {
  add_inst(b, "PUTFIELD_INT");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 131)) {
  add_inst(b, "PUTFIELD_LONG");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 132)) {
  add_inst(b, "INVOKEVIRTUAL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 133)) {
  add_inst(b, "INVOKESTATIC");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 134)) {
  add_inst(b, "INVOKESPECIAL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 135)) {
  add_inst(b, "INVOKEINTERFACE");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 136)) {
  add_inst(b, "NEW");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 137)) {
  add_inst(b, "NEWARRAY_BOOLEAN");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 138)) {
  add_inst(b, "NEWARRAY_BYTE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 139)) {
  add_inst(b, "NEWARRAY_CHAR");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 140)) {
  add_inst(b, "NEWARRAY_SHORT");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 141)) {
  add_inst(b, "NEWARRAY_INT");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 142)) {
  add_inst(b, "NEWARRAY_LONG");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 143)) {
  add_inst(b, "NEWARRAY_FLOAT");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 144)) {
  add_inst(b, "NEWARRAY_DOUBLE");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 145)) {
  add_inst(b, "NEWARRAY");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 146)) {
  add_inst(b, "ARRAYLENGTH");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 147)) {
  add_inst(b, "ATHROW");
  ip += 1;
  return;
}
if (VM_IS_INST(*ip, 148)) {
  add_inst(b, "CHECKCAST");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 149)) {
  add_inst(b, "ARRAYCHECKCAST");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 150)) {
  add_inst(b, "PATCHER_ARRAYCHECKCAST");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 151)) {
  add_inst(b, "INSTANCEOF");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 152)) {
  add_inst(b, "ARRAYINSTANCEOF");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 153)) {
  add_inst(b, "MONITORENTER");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 154)) {
  add_inst(b, "MONITOREXIT");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 155)) {
  add_inst(b, "CHECKNULL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 156)) {
  add_inst(b, "MULTIANEWARRAY");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 157)) {
  add_inst(b, "IFNULL");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 158)) {
  add_inst(b, "IFNONNULL");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 159)) {
  add_inst(b, "PATCHER_GETSTATIC_INT");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 160)) {
  add_inst(b, "PATCHER_GETSTATIC_LONG");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 161)) {
  add_inst(b, "PATCHER_GETSTATIC_CELL");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 162)) {
  add_inst(b, "PATCHER_PUTSTATIC_INT");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 163)) {
  add_inst(b, "PATCHER_PUTSTATIC_LONG");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 164)) {
  add_inst(b, "PATCHER_PUTSTATIC_CELL");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 165)) {
  add_inst(b, "PATCHER_GETFIELD_INT");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 166)) {
  add_inst(b, "PATCHER_GETFIELD_LONG");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 167)) {
  add_inst(b, "PATCHER_GETFIELD_CELL");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 168)) {
  add_inst(b, "PATCHER_PUTFIELD_INT");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 169)) {
  add_inst(b, "PATCHER_PUTFIELD_LONG");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 170)) {
  add_inst(b, "PATCHER_PUTFIELD_CELL");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 171)) {
  add_inst(b, "PATCHER_MULTIANEWARRAY");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 172)) {
  add_inst(b, "PATCHER_INVOKESTATIC");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 173)) {
  add_inst(b, "PATCHER_INVOKESPECIAL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 174)) {
  add_inst(b, "PATCHER_INVOKEVIRTUAL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 175)) {
  add_inst(b, "PATCHER_INVOKEINTERFACE");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 176)) {
  add_inst(b, "PATCHER_CHECKCAST");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 177)) {
  add_inst(b, "PATCHER_INSTANCEOF");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 178)) {
  add_inst(b, "TRANSLATE");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 179)) {
  add_inst(b, "NATIVECALL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 180)) {
  add_inst(b, "TRACENATIVECALL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 181)) {
  add_inst(b, "PATCHER_NATIVECALL");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 182)) {
  add_inst(b, "TRACECALL");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 183)) {
  add_inst(b, "TRACERETURN");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 184)) {
  add_inst(b, "TRACELRETURN");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 185)) {
  add_inst(b, "END");
  ip += 1;
  return;
}
if (VM_IS_INST(*ip, 186)) {
  add_inst(b, "_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 187)) {
  add_inst(b, "_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 188)) {
  add_inst(b, "_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 189)) {
  add_inst(b, "_ICONST_CASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 190)) {
  add_inst(b, "_ICONST_ICONST_CASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 191)) {
  add_inst(b, "_DUP_ICONST_ICONST_CASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 192)) {
  add_inst(b, "_CASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 193)) {
  add_inst(b, "_CASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 194)) {
  add_inst(b, "_CASTORE_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 195)) {
  add_inst(b, "_CASTORE_DUP_ICONST_ICONST_CASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 196)) {
  add_inst(b, "_DUP_ICONST_ICONST_CASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 197)) {
  add_inst(b, "_ICONST_CASTORE_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 198)) {
  add_inst(b, "_ICONST_CASTORE_DUP_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 199)) {
  add_inst(b, "_ICONST_CASTORE_DUP_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 200)) {
  add_inst(b, "_ICONST_ICONST_CASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 201)) {
  add_inst(b, "_ICONST_ICONST_CASTORE_DUP_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 202)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 203)) {
  add_inst(b, "_ALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 204)) {
  add_inst(b, "_ICONST_IASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 205)) {
  add_inst(b, "_ICONST_ICONST_IASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 206)) {
  add_inst(b, "_ALOAD_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 207)) {
  add_inst(b, "_DUP_ICONST_ICONST_IASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 208)) {
  add_inst(b, "_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 209)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 210)) {
  add_inst(b, "_IASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 211)) {
  add_inst(b, "_IASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 212)) {
  add_inst(b, "_DUP_ICONST_ICONST_IASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 213)) {
  add_inst(b, "_ICONST_IASTORE_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 214)) {
  add_inst(b, "_ICONST_IASTORE_DUP_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 215)) {
  add_inst(b, "_ICONST_ICONST_IASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 216)) {
  add_inst(b, "_ICONST_ICONST_IASTORE_DUP_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 217)) {
  add_inst(b, "_IASTORE_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 218)) {
  add_inst(b, "_IASTORE_DUP_ICONST_ICONST_IASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 219)) {
  add_inst(b, "_ICONST_IASTORE_DUP_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 220)) {
  add_inst(b, "_ICONST_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 221)) {
  add_inst(b, "_AASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 222)) {
  add_inst(b, "_AASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 223)) {
  add_inst(b, "_ASTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 224)) {
  add_inst(b, "_DUP_ICONST_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 225)) {
  add_inst(b, "_ALOAD_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 226)) {
  add_inst(b, "_ACONST_AASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 227)) {
  add_inst(b, "_ICONST_ACONST_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 228)) {
  add_inst(b, "_AASTORE_DUP_ICONST_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 229)) {
  add_inst(b, "_DUP_ICONST_ACONST_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 230)) {
  add_inst(b, "_ACONST_AASTORE_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 231)) {
  add_inst(b, "_ACONST_AASTORE_DUP_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 232)) {
  add_inst(b, "_DUP_ICONST_ACONST_AASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 233)) {
  add_inst(b, "_ICONST_ACONST_AASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 234)) {
  add_inst(b, "_ICONST_ACONST_AASTORE_DUP_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 235)) {
  add_inst(b, "_AASTORE_DUP_ICONST_ACONST_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 236)) {
  add_inst(b, "_ACONST_AASTORE_DUP_ICONST_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 237)) {
  add_inst(b, "_POP_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 238)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 239)) {
  add_inst(b, "_ACONST_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 240)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 241)) {
  add_inst(b, "_POP_ALOAD_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 242)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 243)) {
  add_inst(b, "_ALOAD_ACONST_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 244)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 245)) {
  add_inst(b, "_POP_ALOAD_ACONST_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 246)) {
  add_inst(b, "_ILOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 247)) {
  add_inst(b, "_ILOAD_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 248)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 249)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 250)) {
  add_inst(b, "_DUP_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 251)) {
  add_inst(b, "_GETSTATIC_CELL_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 252)) {
  add_inst(b, "_ILOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 253)) {
  add_inst(b, "_ICONST_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 254)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 255)) {
  add_inst(b, "_GETSTATIC_CELL_ICONST_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 256)) {
  add_inst(b, "_GETSTATIC_CELL_ICONST_ICONST_IASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 257)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 258)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 259)) {
  add_inst(b, "_PUTSTATIC_CELL_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 260)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 261)) {
  add_inst(b, "_ACONST_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 262)) {
  add_inst(b, "_ISTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 263)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 264)) {
  add_inst(b, "_ISTORE_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 265)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 266)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 267)) {
  add_inst(b, "_GETFIELD_INT_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 268)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 269)) {
  add_inst(b, "_ISTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 270)) {
  add_inst(b, "_ALOAD_IFNONNULL_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 271)) {
  add_inst(b, "_DUP_ACONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 272)) {
  add_inst(b, "_ICONST_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 273)) {
  add_inst(b, "_ALOAD_IFNULL_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 274)) {
  add_inst(b, "_GETSTATIC_CELL_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 275)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 276)) {
  add_inst(b, "_ALOAD_ILOAD_PUTFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 277)) {
  add_inst(b, "_ICONST_ISTORE_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 278)) {
  add_inst(b, "_ICONST_IADD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 279)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 280)) {
  add_inst(b, "_ICONST_IF_ICMPNE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 281)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 282)) {
  add_inst(b, "_DUP_GETFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 283)) {
  add_inst(b, "_ALOAD_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 284)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 285)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 286)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 287)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 288)) {
  add_inst(b, "_ILOAD_AALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 289)) {
  add_inst(b, "_ACONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 290)) {
  add_inst(b, "_ASTORE_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 291)) {
  add_inst(b, "_CHECKCAST_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 292)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 293)) {
  add_inst(b, "_ASTORE_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 294)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 295)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 296)) {
  add_inst(b, "_CHECKCAST_ASTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 297)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 298)) {
  add_inst(b, "_IADD_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 299)) {
  add_inst(b, "_ASTORE_ALOAD_IFNULL_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 300)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 301)) {
  add_inst(b, "_ALOAD_ACONST_PUTFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 302)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 303)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 304)) {
  add_inst(b, "_ALOAD_MONITOREXIT_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 305)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 306)) {
  add_inst(b, "_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 307)) {
  add_inst(b, "_ASTORE_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 308)) {
  add_inst(b, "_GETFIELD_CELL_ARRAYLENGTH_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 309)) {
  add_inst(b, "_ICONST_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 310)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 311)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 312)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 313)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 314)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 315)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ARRAYLENGTH_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 316)) {
  add_inst(b, "_ALOAD_ILOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 317)) {
  add_inst(b, "_ICONST_IAND_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 318)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 319)) {
  add_inst(b, "_ASTORE_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 320)) {
  add_inst(b, "_AASTORE_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 321)) {
  add_inst(b, "_GETSTATIC_CELL_ACONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 322)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 323)) {
  add_inst(b, "_GETFIELD_CELL_IFNONNULL_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 324)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_IFNONNULL_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 325)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 326)) {
  add_inst(b, "_ICONST_BASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 327)) {
  add_inst(b, "_ALOAD_ALOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 328)) {
  add_inst(b, "_ICONST_IF_ICMPEQ_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 329)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 330)) {
  add_inst(b, "_ILOAD_IADD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 331)) {
  add_inst(b, "_ALOAD_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 332)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 333)) {
  add_inst(b, "_ICONST_IADD_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 334)) {
  add_inst(b, "_GETFIELD_CELL_IFNULL_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 335)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 336)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 337)) {
  add_inst(b, "_ICONST_ISUB_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 338)) {
  add_inst(b, "_ASTORE_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 339)) {
  add_inst(b, "_ACONST_ASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 340)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 341)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 342)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_IFNULL_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 343)) {
  add_inst(b, "_GETFIELD_INT_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 344)) {
  add_inst(b, "_MONITORENTER_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 345)) {
  add_inst(b, "_ILOAD_IFEQ_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 346)) {
  add_inst(b, "_ILOAD_IFGE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 347)) {
  add_inst(b, "_ASTORE_ALOAD_IFNONNULL_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 348)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 349)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ICONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 350)) {
  add_inst(b, "_ALOAD_GETSTATIC_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 351)) {
  add_inst(b, "_GETFIELD_CELL_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 352)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 353)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 354)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IFEQ_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 355)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 356)) {
  add_inst(b, "_ISTORE_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 357)) {
  add_inst(b, "_ARRAYLENGTH_IF_ICMPLT_");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 358)) {
  add_inst(b, "_ALOAD_ICONST_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 359)) {
  add_inst(b, "_ICONST_IALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 360)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 361)) {
  add_inst(b, "_ASTORE_ICONST_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 362)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 363)) {
  add_inst(b, "_ICONST_ICONST_BASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 364)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 365)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 366)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 367)) {
  add_inst(b, "_DUP_ALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 368)) {
  add_inst(b, "_DUP_ICONST_ICONST_BASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 369)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 370)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 371)) {
  add_inst(b, "_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 372)) {
  add_inst(b, "_GETFIELD_INT_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 373)) {
  add_inst(b, "_BASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 374)) {
  add_inst(b, "_BASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 375)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 376)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 377)) {
  add_inst(b, "_BASTORE_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 378)) {
  add_inst(b, "_BASTORE_DUP_ICONST_ICONST_BASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 379)) {
  add_inst(b, "_DUP_ICONST_ICONST_BASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 380)) {
  add_inst(b, "_ICONST_BASTORE_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 381)) {
  add_inst(b, "_ICONST_BASTORE_DUP_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 382)) {
  add_inst(b, "_ICONST_BASTORE_DUP_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 383)) {
  add_inst(b, "_ICONST_ICONST_BASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 384)) {
  add_inst(b, "_ICONST_ICONST_BASTORE_DUP_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 385)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 386)) {
  add_inst(b, "_GETFIELD_CELL_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 387)) {
  add_inst(b, "_DUP_ACONST_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 388)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 389)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 390)) {
  add_inst(b, "_ISTORE_ILOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 391)) {
  add_inst(b, "_POP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 392)) {
  add_inst(b, "_INSTANCEOF_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 393)) {
  add_inst(b, "_ALOAD_MONITORENTER_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 394)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 395)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 396)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 397)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ICONST_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 398)) {
  add_inst(b, "_ACONST_ACONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 399)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 400)) {
  add_inst(b, "_ICONST_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 401)) {
  add_inst(b, "_DUP_ACONST_ACONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 402)) {
  add_inst(b, "_ALOAD_ILOAD_AALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 403)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 404)) {
  add_inst(b, "_ICONST_ISTORE_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 405)) {
  add_inst(b, "_ICONST_AALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 406)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 407)) {
  add_inst(b, "_ISTORE_ICONST_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 408)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ICONST_IADD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 409)) {
  add_inst(b, "_IADD_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 410)) {
  add_inst(b, "_ILOAD_IFLT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 411)) {
  add_inst(b, "_GETFIELD_INT_IFNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 412)) {
  add_inst(b, "_GETSTATIC_CELL_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 413)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 414)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 415)) {
  add_inst(b, "_ASTORE_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 416)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 417)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 418)) {
  add_inst(b, "_ALOAD_INSTANCEOF_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 419)) {
  add_inst(b, "_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 420)) {
  add_inst(b, "_LCONST_LAND_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 421)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 422)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 423)) {
  add_inst(b, "_ALOAD_ILOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 424)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 425)) {
  add_inst(b, "_ILOAD_IFNE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 426)) {
  add_inst(b, "_ILOAD_ILOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 427)) {
  add_inst(b, "_ALOAD_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 428)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 429)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 430)) {
  add_inst(b, "_ICONST_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 431)) {
  add_inst(b, "_GETFIELD_INT_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 432)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IF_ICMPNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 433)) {
  add_inst(b, "_GETSTATIC_CELL_ACONST_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 434)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 435)) {
  add_inst(b, "_ALOAD_MONITORENTER_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 436)) {
  add_inst(b, "_ILOAD_IF_ICMPLT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 437)) {
  add_inst(b, "_ASTORE_ICONST_ISTORE_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 438)) {
  add_inst(b, "_ALOAD_CHECKCAST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 439)) {
  add_inst(b, "_ALOAD_ILOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 440)) {
  add_inst(b, "_ICONST_IADD_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 441)) {
  add_inst(b, "_DUP_ASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 442)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 443)) {
  add_inst(b, "_ILOAD_ILOAD_IF_ICMPLT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 444)) {
  add_inst(b, "_ICONST_IF_ICMPLE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 445)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ISTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 446)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 447)) {
  add_inst(b, "_ILOAD_ISUB_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 448)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IFNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 449)) {
  add_inst(b, "_GETFIELD_CELL_ASTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 450)) {
  add_inst(b, "_ISTORE_ILOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 451)) {
  add_inst(b, "_ILOAD_IINC_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 452)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_IF_ICMPNE_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 453)) {
  add_inst(b, "_ALOAD_ACONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 454)) {
  add_inst(b, "_POP_GETSTATIC_CELL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 455)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 456)) {
  add_inst(b, "_POP_GETSTATIC_CELL_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 457)) {
  add_inst(b, "_ALOAD_ICONST_IALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 458)) {
  add_inst(b, "_GETFIELD_INT_PUTFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 459)) {
  add_inst(b, "_GETSTATIC_INT_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 460)) {
  add_inst(b, "_ALOAD_ALOAD_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 461)) {
  add_inst(b, "_AALOAD_ASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 462)) {
  add_inst(b, "_ALOAD_ICONST_AALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 463)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 464)) {
  add_inst(b, "_ASTORE_ALOAD_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 465)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 466)) {
  add_inst(b, "_PUTFIELD_INT_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 467)) {
  add_inst(b, "_PUTSTATIC_CELL_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 468)) {
  add_inst(b, "_DUP_ALOAD_GETFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 469)) {
  add_inst(b, "_ALOAD_ICONST_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 470)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 471)) {
  add_inst(b, "_GETSTATIC_CELL_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 472)) {
  add_inst(b, "_LALOAD_LCONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 473)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 474)) {
  add_inst(b, "_IADD_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 475)) {
  add_inst(b, "_ILOAD_ALOAD_ARRAYLENGTH_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 476)) {
  add_inst(b, "_ACONST_ASTORE_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 477)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 478)) {
  add_inst(b, "_ILOAD_ILOAD_IADD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 479)) {
  add_inst(b, "_ICONST_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 480)) {
  add_inst(b, "_ICONST_IF_ICMPLT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 481)) {
  add_inst(b, "_LALOAD_LCONST_LAND_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 482)) {
  add_inst(b, "_ACONST_ICONST_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 483)) {
  add_inst(b, "_GETSTATIC_CELL_PUTFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 484)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 485)) {
  add_inst(b, "_ICONST_DUP2_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 486)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 487)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_CELL_ARRAYLENGTH_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 488)) {
  add_inst(b, "_GETFIELD_INT_ICONST_ISUB_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 489)) {
  add_inst(b, "_ILOAD_ICONST_IAND_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 490)) {
  add_inst(b, "_CHECKNULL_MONITORENTER_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 491)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ASTORE_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 492)) {
  add_inst(b, "_ILOAD_ICONST_IADD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 493)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 494)) {
  add_inst(b, "_ASTORE_CHECKNULL_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 495)) {
  add_inst(b, "_ASTORE_CHECKNULL_MONITORENTER_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 496)) {
  add_inst(b, "_DUP_ASTORE_CHECKNULL_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 497)) {
  add_inst(b, "_DUP_ASTORE_CHECKNULL_MONITORENTER_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 498)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPLE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 499)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 500)) {
  add_inst(b, "_ALOAD_ICONST_DUP2_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 501)) {
  add_inst(b, "_ALOAD_INSTANCEOF_IFEQ_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 502)) {
  add_inst(b, "_ASTORE_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 503)) {
  add_inst(b, "_IINC_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 504)) {
  add_inst(b, "_AALOAD_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 505)) {
  add_inst(b, "_ICONST_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 506)) {
  add_inst(b, "_ISUB_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 507)) {
  add_inst(b, "_GETFIELD_INT_IADD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 508)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_IF_ICMPLT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 509)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 510)) {
  add_inst(b, "_ALOAD_ACONST_ICONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 511)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPLT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 512)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 513)) {
  add_inst(b, "_ILOAD_ALOAD_ARRAYLENGTH_IF_ICMPLT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 514)) {
  add_inst(b, "_GETFIELD_INT_ILOAD_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 515)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 516)) {
  add_inst(b, "_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 517)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 518)) {
  add_inst(b, "_LCONST_LASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 519)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 520)) {
  add_inst(b, "_LLOAD_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 521)) {
  add_inst(b, "_BALOAD_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 522)) {
  add_inst(b, "_GETFIELD_INT_PUTFIELD_INT_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 523)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 524)) {
  add_inst(b, "_ICONST_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 525)) {
  add_inst(b, "_PUTFIELD_CELL_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 526)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 527)) {
  add_inst(b, "_GETFIELD_INT_ISUB_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 528)) {
  add_inst(b, "_DUP2_LALOAD_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 529)) {
  add_inst(b, "_GETFIELD_CELL_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 530)) {
  add_inst(b, "_CHECKNULL_MONITORENTER_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 531)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 532)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 533)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IADD_PUTFIELD_INT_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 534)) {
  add_inst(b, "_ICONST_LCONST_LASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 535)) {
  add_inst(b, "_ASTORE_CHECKNULL_MONITORENTER_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 536)) {
  add_inst(b, "_DUP_ASTORE_CHECKNULL_MONITORENTER_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 537)) {
  add_inst(b, "_ILOAD_AALOAD_ASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 538)) {
  add_inst(b, "_POP_ALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 539)) {
  add_inst(b, "_GETFIELD_INT_DUP_X1_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 540)) {
  add_inst(b, "_GETFIELD_INT_IF_ICMPLT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 541)) {
  add_inst(b, "_DUP_GETFIELD_INT_DUP_X1_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 542)) {
  add_inst(b, "_DUP_GETFIELD_INT_DUP_X1_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 543)) {
  add_inst(b, "_DUP_X1_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 544)) {
  add_inst(b, "_GETFIELD_INT_DUP_X1_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 545)) {
  add_inst(b, "_ICONST_ISTORE_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 546)) {
  add_inst(b, "_DUP_GETFIELD_INT_DUP_X1_ICONST_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 547)) {
  add_inst(b, "_DUP_X1_ICONST_IADD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 548)) {
  add_inst(b, "_DUP_X1_ICONST_IADD_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 549)) {
  add_inst(b, "_GETFIELD_INT_DUP_X1_ICONST_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 550)) {
  add_inst(b, "_GETFIELD_INT_DUP_X1_ICONST_IADD_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 551)) {
  add_inst(b, "_DUP2_LALOAD_LCONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 552)) {
  add_inst(b, "_ALOAD_GETFIELD_LONG_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 553)) {
  add_inst(b, "_LAND_LASTORE_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 554)) {
  add_inst(b, "_ICONST_ISHL_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 555)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPLT_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 556)) {
  add_inst(b, "_ICONST_ICONST_ICONST_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 557)) {
  add_inst(b, "_GETFIELD_INT_ISTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 558)) {
  add_inst(b, "_ALOAD_LLOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 559)) {
  add_inst(b, "_ASTORE_ACONST_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 560)) {
  add_inst(b, "_ALOAD_AASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 561)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ISUB_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 562)) {
  add_inst(b, "_MONITORENTER_ALOAD_GETFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 563)) {
  add_inst(b, "_DUP_X1_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 564)) {
  add_inst(b, "_GETSTATIC_CELL_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 565)) {
  add_inst(b, "_ICONST_ISTORE_ICONST_ISTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 566)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_ACONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 567)) {
  add_inst(b, "_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ACONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 568)) {
  add_inst(b, "_ILOAD_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 569)) {
  add_inst(b, "_ISTORE_ICONST_ISTORE_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 570)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_DUP_X1_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 571)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_DUP_X1_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 572)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IF_ICMPLT_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 573)) {
  add_inst(b, "_BALOAD_ICONST_IAND_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 574)) {
  add_inst(b, "_ALOAD_GETSTATIC_CELL_PUTFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 575)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 576)) {
  add_inst(b, "_DUP_ICONST_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 577)) {
  add_inst(b, "_DUP_ICONST_LCONST_LASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 578)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ISTORE_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 579)) {
  add_inst(b, "_DUP_GETFIELD_INT_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 580)) {
  add_inst(b, "_ISTORE_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 581)) {
  add_inst(b, "_DUP_ICONST_ICONST_IASTORE_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 582)) {
  add_inst(b, "_IASTORE_AASTORE_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 583)) {
  add_inst(b, "_ICONST_IASTORE_AASTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 584)) {
  add_inst(b, "_ICONST_ICONST_IASTORE_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 585)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 586)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 587)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 588)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 589)) {
  add_inst(b, "_IASTORE_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 590)) {
  add_inst(b, "_ICONST_ISHR_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 591)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 592)) {
  add_inst(b, "_GETFIELD_CELL_PUTFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 593)) {
  add_inst(b, "_IASTORE_AASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 594)) {
  add_inst(b, "_IASTORE_AASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 595)) {
  add_inst(b, "_IASTORE_AASTORE_DUP_ICONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 596)) {
  add_inst(b, "_ICONST_IASTORE_AASTORE_DUP_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 597)) {
  add_inst(b, "_ICONST_IASTORE_AASTORE_DUP_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 598)) {
  add_inst(b, "_ICONST_ICONST_IASTORE_AASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 599)) {
  add_inst(b, "_ALOAD_ALOAD_ILOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 600)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_DUP_X1_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 601)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_DUP_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 602)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_DUP_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 603)) {
  add_inst(b, "_ISUB_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 604)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 605)) {
  add_inst(b, "_IAND_IFEQ_");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 606)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_ACONST_PUTFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 607)) {
  add_inst(b, "_ILOAD_AALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 608)) {
  add_inst(b, "_ILOAD_IALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 609)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 610)) {
  add_inst(b, "_DUP_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 611)) {
  add_inst(b, "_AASTORE_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 612)) {
  add_inst(b, "_ICONST_IAND_IFEQ_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 613)) {
  add_inst(b, "_CHECKCAST_ASTORE_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 614)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_ACONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 615)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ACONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 616)) {
  add_inst(b, "_ALOAD_ICONST_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 617)) {
  add_inst(b, "_POP_ALOAD_GETFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 618)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 619)) {
  add_inst(b, "_ASTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 620)) {
  add_inst(b, "_ISTORE_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 621)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 622)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 623)) {
  add_inst(b, "_ILOAD_IADD_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 624)) {
  add_inst(b, "_IALOAD_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 625)) {
  add_inst(b, "_DUP_ACONST_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 626)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_ISUB_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 627)) {
  add_inst(b, "_ISTORE_ALOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 628)) {
  add_inst(b, "_DUP_GETFIELD_INT_ILOAD_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 629)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ILOAD_PUTFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 630)) {
  add_inst(b, "_GETFIELD_INT_IF_ICMPGE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 631)) {
  add_inst(b, "_GETFIELD_INT_IF_ICMPLE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 632)) {
  add_inst(b, "_GETSTATIC_CELL_ASTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 633)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 634)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 635)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IF_ICMPLE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 636)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ILOAD_IADD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 637)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 638)) {
  add_inst(b, "_LCONST_IF_LCMPEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 639)) {
  add_inst(b, "_POP_GOTO_");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 640)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 641)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 642)) {
  add_inst(b, "_GETFIELD_CELL_CHECKCAST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 643)) {
  add_inst(b, "_LAND_LCONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 644)) {
  add_inst(b, "_ARRAYLENGTH_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 645)) {
  add_inst(b, "_GETFIELD_INT_ILOAD_IADD_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 646)) {
  add_inst(b, "_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 647)) {
  add_inst(b, "_ILOAD_ICONST_ISHR_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 648)) {
  add_inst(b, "_INSTANCEOF_IFNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 649)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_CHECKCAST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 650)) {
  add_inst(b, "_DUP_GETFIELD_INT_ILOAD_IADD_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 651)) {
  add_inst(b, "_GETFIELD_CELL_DUP_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 652)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_DUP_ASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 653)) {
  add_inst(b, "_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 654)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 655)) {
  add_inst(b, "_ASTORE_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 656)) {
  add_inst(b, "_ASTORE_GETSTATIC_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 657)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 658)) {
  add_inst(b, "_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 659)) {
  add_inst(b, "_GETFIELD_CELL_DUP_ASTORE_CHECKNULL_MONITORENTER_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 660)) {
  add_inst(b, "_ICONST_ALOAD_ARRAYLENGTH_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 661)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 662)) {
  add_inst(b, "_DUP_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 663)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ICONST_ISUB_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 664)) {
  add_inst(b, "_ALOAD_ICONST_ALOAD_ARRAYLENGTH_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 665)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 666)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IF_ICMPGE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 667)) {
  add_inst(b, "_ALOAD_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 668)) {
  add_inst(b, "_CHECKCAST_ASTORE_ALOAD_IFNONNULL_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 669)) {
  add_inst(b, "_GETSTATIC_CELL_ASTORE_ALOAD_IFNULL_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 670)) {
  add_inst(b, "_ICONST_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 671)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 672)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 673)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 674)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 675)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 676)) {
  add_inst(b, "_ICONST_ICONST_ICONST_ICONST_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 677)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_INT_ISTORE_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 678)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 679)) {
  add_inst(b, "_PUTFIELD_INT_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 680)) {
  add_inst(b, "_ICONST_ISUB_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 681)) {
  add_inst(b, "_ISTORE_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 682)) {
  add_inst(b, "_ALOAD_ILOAD_ILOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 683)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 684)) {
  add_inst(b, "_ILOAD_ICONST_ISUB_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 685)) {
  add_inst(b, "_LAND_LCONST_IF_LCMPEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 686)) {
  add_inst(b, "_ALOAD_ILOAD_IINC_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 687)) {
  add_inst(b, "_ISTORE_ILOAD_IFGE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 688)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 689)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 690)) {
  add_inst(b, "_ILOAD_ILOAD_ISUB_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 691)) {
  add_inst(b, "_AALOAD_ACONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 692)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 693)) {
  add_inst(b, "_ILOAD_IF_ICMPGE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 694)) {
  add_inst(b, "_DUP_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 695)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 696)) {
  add_inst(b, "_ACONST_ACONST_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 697)) {
  add_inst(b, "_ACONST_ASTORE_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 698)) {
  add_inst(b, "_ALOAD_ICONST_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 699)) {
  add_inst(b, "_ASTORE_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 700)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 701)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 702)) {
  add_inst(b, "_ALOAD_ILOAD_ICONST_IADD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 703)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 704)) {
  add_inst(b, "_PUTSTATIC_CELL_ACONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 705)) {
  add_inst(b, "_ILOAD_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 706)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 707)) {
  add_inst(b, "_GETFIELD_INT_IF_ICMPEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 708)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 709)) {
  add_inst(b, "_ILOAD_ALOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 710)) {
  add_inst(b, "_ILOAD_ILOAD_ILOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 711)) {
  add_inst(b, "_ILOAD_IF_ICMPNE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 712)) {
  add_inst(b, "_ISTORE_ILOAD_ICONST_IF_ICMPEQ_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 713)) {
  add_inst(b, "_ALOAD_ASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 714)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 715)) {
  add_inst(b, "_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 716)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 717)) {
  add_inst(b, "_POP_ACONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 718)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 719)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 720)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 721)) {
  add_inst(b, "_ALOAD_ACONST_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 722)) {
  add_inst(b, "_ALOAD_ILOAD_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 723)) {
  add_inst(b, "_LCONST_LAND_LCONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 724)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 725)) {
  add_inst(b, "_IASTORE_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 726)) {
  add_inst(b, "_AASTORE_IINC_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 727)) {
  add_inst(b, "_CHECKCAST_ASTORE_ALOAD_IFNULL_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 728)) {
  add_inst(b, "_DUP_ICONST_LCONST_LASTORE_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 729)) {
  add_inst(b, "_ICONST_LCONST_LASTORE_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 730)) {
  add_inst(b, "_ICONST_LCONST_LASTORE_DUP_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 731)) {
  add_inst(b, "_ILOAD_ALOAD_AASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 732)) {
  add_inst(b, "_ILOAD_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 733)) {
  add_inst(b, "_LASTORE_DUP_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 734)) {
  add_inst(b, "_LASTORE_DUP_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 735)) {
  add_inst(b, "_LASTORE_DUP_ICONST_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 736)) {
  add_inst(b, "_LASTORE_DUP_ICONST_LCONST_LASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 737)) {
  add_inst(b, "_LCONST_LASTORE_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 738)) {
  add_inst(b, "_LCONST_LASTORE_DUP_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 739)) {
  add_inst(b, "_LCONST_LASTORE_DUP_ICONST_LCONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 740)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 741)) {
  add_inst(b, "_LASTORE_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 742)) {
  add_inst(b, "_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 743)) {
  add_inst(b, "_ICONST_IALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 744)) {
  add_inst(b, "_ILOAD_IF_ICMPGT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 745)) {
  add_inst(b, "_CALOAD_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 746)) {
  add_inst(b, "_LCONST_LAND_LCONST_IF_LCMPEQ_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 747)) {
  add_inst(b, "_ICONST_IMUL_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 748)) {
  add_inst(b, "_ILOAD_IF_ICMPLE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 749)) {
  add_inst(b, "_ISUB_PUTFIELD_INT_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 750)) {
  add_inst(b, "_CHECKCAST_PUTFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 751)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 752)) {
  add_inst(b, "_ALOAD_CHECKCAST_ASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 753)) {
  add_inst(b, "_GETFIELD_INT_IF_ICMPNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 754)) {
  add_inst(b, "_ILOAD_BALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 755)) {
  add_inst(b, "_AALOAD_ASTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 756)) {
  add_inst(b, "_PUTSTATIC_INT_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 757)) {
  add_inst(b, "_LASTORE_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 758)) {
  add_inst(b, "_ICONST_ILOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 759)) {
  add_inst(b, "_ILOAD_GETSTATIC_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 760)) {
  add_inst(b, "_ILOAD_IADD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 761)) {
  add_inst(b, "_ISTORE_ILOAD_ICONST_IF_ICMPNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 762)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_ISUB_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 763)) {
  add_inst(b, "_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 764)) {
  add_inst(b, "_GETSTATIC_INT_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 765)) {
  add_inst(b, "_IADD_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 766)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_ILOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 767)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 768)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_INT_IF_ICMPLT_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 769)) {
  add_inst(b, "_ACONST_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 770)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_ILOAD_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 771)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 772)) {
  add_inst(b, "_LASTORE_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 773)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_AALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 774)) {
  add_inst(b, "_IADD_ISTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 775)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_AASTORE_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 776)) {
  add_inst(b, "_DUP_ALOAD_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 777)) {
  add_inst(b, "_GETFIELD_CELL_GETSTATIC_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 778)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ALOAD_AASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 779)) {
  add_inst(b, "_AALOAD_ASTORE_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 780)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_IINC_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 781)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_IINC_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 782)) {
  add_inst(b, "_ILOAD_AALOAD_ASTORE_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 783)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 784)) {
  add_inst(b, "_ALOAD_ALOAD_MONITOREXIT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 785)) {
  add_inst(b, "_GETFIELD_CELL_ASTORE_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 786)) {
  add_inst(b, "_ILOAD_IFLE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 787)) {
  add_inst(b, "_ISUB_ISTORE_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 788)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 789)) {
  add_inst(b, "_IADD_ISTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 790)) {
  add_inst(b, "_IALOAD_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 791)) {
  add_inst(b, "_PUTFIELD_INT_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 792)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_ISUB_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 793)) {
  add_inst(b, "_ILOAD_CALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 794)) {
  add_inst(b, "_ALOAD_INSTANCEOF_IFNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 795)) {
  add_inst(b, "_DUP_GETSTATIC_CELL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 796)) {
  add_inst(b, "_IADD_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 797)) {
  add_inst(b, "_IAND_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 798)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 799)) {
  add_inst(b, "_IALOAD_ALOAD_ICONST_IALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 800)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_AALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 801)) {
  add_inst(b, "_ALOAD_ILOAD_IALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 802)) {
  add_inst(b, "_MONITORENTER_ALOAD_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 803)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 804)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 805)) {
  add_inst(b, "_ARRAYLENGTH_IF_ICMPLE_");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 806)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 807)) {
  add_inst(b, "_ISTORE_ILOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 808)) {
  add_inst(b, "_ICONST_ISTORE_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 809)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 810)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ASTORE_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 811)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 812)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 813)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETSTATIC_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 814)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 815)) {
  add_inst(b, "_ARRAYLENGTH_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 816)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IF_ICMPEQ_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 817)) {
  add_inst(b, "_ISHL_IOR_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 818)) {
  add_inst(b, "_ACONST_ASTORE_ACONST_ASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 819)) {
  add_inst(b, "_ICONST_ISUB_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 820)) {
  add_inst(b, "_DUP_ALOAD_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 821)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 822)) {
  add_inst(b, "_IINC_ILOAD_IFGE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 823)) {
  add_inst(b, "_GETSTATIC_CELL_IFNONNULL_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 824)) {
  add_inst(b, "_ILOAD_IADD_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 825)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 826)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 827)) {
  add_inst(b, "_ICONST_IF_ICMPGT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 828)) {
  add_inst(b, "_ICONST_ISUB_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 829)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 830)) {
  add_inst(b, "_ICONST_IAND_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 831)) {
  add_inst(b, "_LSTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 832)) {
  add_inst(b, "_ASTORE_ICONST_ISTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 833)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 834)) {
  add_inst(b, "_ICONST_BALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 835)) {
  add_inst(b, "_ALOAD_MONITORENTER_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 836)) {
  add_inst(b, "_CHECKCAST_ASTORE_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 837)) {
  add_inst(b, "_ASTORE_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 838)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 839)) {
  add_inst(b, "_GETFIELD_INT_IFLE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 840)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_ICONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 841)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ICONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 842)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 843)) {
  add_inst(b, "_LLOAD_LLOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 844)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 845)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IFLE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 846)) {
  add_inst(b, "_BASTORE_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 847)) {
  add_inst(b, "_ICONST_GETSTATIC_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 848)) {
  add_inst(b, "_ILOAD_ICONST_IAND_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 849)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ACONST_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 850)) {
  add_inst(b, "_ALOAD_ALOAD_ICONST_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 851)) {
  add_inst(b, "_DUP_X1_ICONST_IADD_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 852)) {
  add_inst(b, "_ICONST_ALOAD_ICONST_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 853)) {
  add_inst(b, "_ICONST_ISTORE_ICONST_ISTORE_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 854)) {
  add_inst(b, "_ILOAD_IFGT_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 855)) {
  add_inst(b, "_IAND_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 856)) {
  add_inst(b, "_ICONST_IF_ICMPGE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 857)) {
  add_inst(b, "_ILOAD_ILOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 858)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 859)) {
  add_inst(b, "_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ICONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 860)) {
  add_inst(b, "_ARRAYLENGTH_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 861)) {
  add_inst(b, "_GETFIELD_CELL_INSTANCEOF_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 862)) {
  add_inst(b, "_ICONST_BALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 863)) {
  add_inst(b, "_ICONST_IAND_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 864)) {
  add_inst(b, "_ILOAD_AALOAD_ASTORE_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 865)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_INT_IADD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 866)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_INSTANCEOF_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 867)) {
  add_inst(b, "_ILOAD_AALOAD_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 868)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 869)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_ICONST_PUTFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 870)) {
  add_inst(b, "_PUTFIELD_CELL_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 871)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_IADD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 872)) {
  add_inst(b, "_ILOAD_ILOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 873)) {
  add_inst(b, "_ALOAD_ICONST_BALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 874)) {
  add_inst(b, "_ALOAD_ILOAD_AALOAD_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 875)) {
  add_inst(b, "_ALOAD_ICONST_BALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 876)) {
  add_inst(b, "_ASTORE_ALOAD_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 877)) {
  add_inst(b, "_GETFIELD_CELL_ARRAYLENGTH_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 878)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ARRAYLENGTH_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 879)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 880)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 881)) {
  add_inst(b, "_ICONST_ISHL_IOR_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 882)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 883)) {
  add_inst(b, "_ALOAD_CHECKCAST_ASTORE_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 884)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_IF_ICMPEQ_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 885)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IF_ICMPNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 886)) {
  add_inst(b, "_ARRAYLENGTH_IF_ICMPNE_");
  ip += 2;
  return;
}
if (VM_IS_INST(*ip, 887)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 888)) {
  add_inst(b, "_ICONST_IADD_PUTFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 889)) {
  add_inst(b, "_ACONST_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 890)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 891)) {
  add_inst(b, "_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 892)) {
  add_inst(b, "_ICONST_BALOAD_ICONST_IAND_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 893)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_ACONST_PUTFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 894)) {
  add_inst(b, "_ILOAD_ILOAD_IADD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 895)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 896)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_ALOAD_ACONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 897)) {
  add_inst(b, "_ASTORE_ALOAD_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 898)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 899)) {
  add_inst(b, "_ILOAD_ILOAD_IF_ICMPLE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 900)) {
  add_inst(b, "_ALOAD_ACONST_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 901)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_ILOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 902)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_CELL_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 903)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_INT_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 904)) {
  add_inst(b, "_MONITORENTER_ALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 905)) {
  add_inst(b, "_AALOAD_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 906)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_ISUB_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 907)) {
  add_inst(b, "_CALOAD_ISTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 908)) {
  add_inst(b, "_ACONST_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 909)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPLT_");
  ip += 8;
  return;
}
if (VM_IS_INST(*ip, 910)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 911)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPLT_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 912)) {
  add_inst(b, "_GETFIELD_INT_ICONST_ISUB_PUTFIELD_INT_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 913)) {
  add_inst(b, "_ICONST_IALOAD_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 914)) {
  add_inst(b, "_ICONST_IALOAD_ALOAD_ICONST_IALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 915)) {
  add_inst(b, "_ICONST_ISUB_ISTORE_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 916)) {
  add_inst(b, "_ILOAD_I2F_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 917)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 918)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_ALOAD_ACONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 919)) {
  add_inst(b, "_ALOAD_ICONST_BALOAD_ICONST_IAND_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 920)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 921)) {
  add_inst(b, "_GETFIELD_INT_IADD_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 922)) {
  add_inst(b, "_ILOAD_ICONST_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 923)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 924)) {
  add_inst(b, "_DUP_ALOAD_ACONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 925)) {
  add_inst(b, "_ILOAD_ILOAD_IADD_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 926)) {
  add_inst(b, "_ILOAD_PUTFIELD_INT_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 927)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IADD_ISTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 928)) {
  add_inst(b, "_ISTORE_ALOAD_GETFIELD_INT_ISTORE_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 929)) {
  add_inst(b, "_ALOAD_ALOAD_ILOAD_AALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 930)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_ALOAD_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 931)) {
  add_inst(b, "_ALOAD_MONITORENTER_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 932)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 933)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 934)) {
  add_inst(b, "_ALOAD_ILOAD_ILOAD_ILOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 935)) {
  add_inst(b, "_FMUL_FADD_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 936)) {
  add_inst(b, "_GETFIELD_INT_ISTORE_ALOAD_GETFIELD_INT_ISTORE_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 937)) {
  add_inst(b, "_MONITORENTER_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 938)) {
  add_inst(b, "_IADD_ALOAD_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 939)) {
  add_inst(b, "_ILOAD_FADD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 940)) {
  add_inst(b, "_ILOAD_ISUB_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 941)) {
  add_inst(b, "_LSTORE_LLOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 942)) {
  add_inst(b, "_GETSTATIC_CELL_ILOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 943)) {
  add_inst(b, "_ICONST_ALOAD_GETFIELD_INT_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 944)) {
  add_inst(b, "_ICONST_IADD_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 945)) {
  add_inst(b, "_ILOAD_ILOAD_IF_ICMPGE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 946)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 947)) {
  add_inst(b, "_FMUL_F2I_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 948)) {
  add_inst(b, "_GETFIELD_CELL_ASTORE_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 949)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IADD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 950)) {
  add_inst(b, "_ASTORE_ACONST_ASTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 951)) {
  add_inst(b, "_GETFIELD_INT_ISTORE_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 952)) {
  add_inst(b, "_ICONST_ILOAD_IF_ICMPGT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 953)) {
  add_inst(b, "_ICONST_ISTORE_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 954)) {
  add_inst(b, "_IINC_CALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 955)) {
  add_inst(b, "_ILOAD_IINC_CALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 956)) {
  add_inst(b, "_ISTORE_ALOAD_ACONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 957)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ILOAD_IADD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 958)) {
  add_inst(b, "_FADD_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 959)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 960)) {
  add_inst(b, "_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_CELL_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 961)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_PUTFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 962)) {
  add_inst(b, "_ASTORE_ALOAD_GETFIELD_CELL_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 963)) {
  add_inst(b, "_ASTORE_ILOAD_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 964)) {
  add_inst(b, "_CHECKCAST_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 965)) {
  add_inst(b, "_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 966)) {
  add_inst(b, "_ICONST_ISTORE_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 967)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_IINC_CALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 968)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 969)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_IINC_CALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 970)) {
  add_inst(b, "_GETSTATIC_CELL_IF_ACMPNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 971)) {
  add_inst(b, "_ICONST_ALOAD_GETFIELD_CELL_IFNULL_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 972)) {
  add_inst(b, "_ILOAD_ILOAD_FADD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 973)) {
  add_inst(b, "_ISTORE_ICONST_ISTORE_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 974)) {
  add_inst(b, "_PUTFIELD_LONG_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 975)) {
  add_inst(b, "_ILOAD_ICONST_BASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 976)) {
  add_inst(b, "_GETSTATIC_CELL_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 977)) {
  add_inst(b, "_ILOAD_IF_ICMPEQ_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 978)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 979)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 980)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_ALOAD_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 981)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 982)) {
  add_inst(b, "_ICONST_ALOAD_ICONST_ALOAD_GETFIELD_INT_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 983)) {
  add_inst(b, "_POP_IINC_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 984)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_ALOAD_ICONST_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 985)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_PUTFIELD_CELL_ALOAD_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 986)) {
  add_inst(b, "_ICONST_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 987)) {
  add_inst(b, "_PUTFIELD_CELL_ICONST_ISTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 988)) {
  add_inst(b, "_ALOAD_ACONST_ACONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 989)) {
  add_inst(b, "_ALOAD_ALOAD_ALOAD_ILOAD_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 990)) {
  add_inst(b, "_GETFIELD_CELL_INSTANCEOF_IFEQ_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 991)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPGE_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 992)) {
  add_inst(b, "_ISUB_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 993)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 994)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_INSTANCEOF_IFEQ_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 995)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPGE_");
  ip += 8;
  return;
}
if (VM_IS_INST(*ip, 996)) {
  add_inst(b, "_ASTORE_GETSTATIC_CELL_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 997)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_ALOAD_ICONST_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 998)) {
  add_inst(b, "_GETSTATIC_CELL_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 999)) {
  add_inst(b, "_ICONST_IADD_ISTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1000)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPGT_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1001)) {
  add_inst(b, "_ISUB_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1002)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ISTORE_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1003)) {
  add_inst(b, "_GETFIELD_CELL_DUP_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1004)) {
  add_inst(b, "_GETSTATIC_INT_ICONST_IF_ICMPNE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 1005)) {
  add_inst(b, "_ISTORE_IINC_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1006)) {
  add_inst(b, "_PUTSTATIC_CELL_ICONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1007)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ACONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1008)) {
  add_inst(b, "_GETFIELD_CELL_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1009)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1010)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_CELL_ARRAYLENGTH_IF_ICMPNE_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1011)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1012)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ICONST_IALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1013)) {
  add_inst(b, "_ASTORE_ALOAD_INSTANCEOF_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1014)) {
  add_inst(b, "_DUP_ALOAD_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1015)) {
  add_inst(b, "_GETFIELD_CELL_ICONST_IALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1016)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1017)) {
  add_inst(b, "_ISUB_ISTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1018)) {
  add_inst(b, "_ALOAD_ALOAD_ILOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1019)) {
  add_inst(b, "_ALOAD_ILOAD_AALOAD_ASTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1020)) {
  add_inst(b, "_ISTORE_ILOAD_IFNE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1021)) {
  add_inst(b, "_LLOAD_LCONST_LAND_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1022)) {
  add_inst(b, "_AALOAD_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1023)) {
  add_inst(b, "_CHECKNULL_MONITORENTER_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1024)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1025)) {
  add_inst(b, "_ARRAYLENGTH_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1026)) {
  add_inst(b, "_DUP_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1027)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IAND_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1028)) {
  add_inst(b, "_GETFIELD_INT_PUTFIELD_INT_ALOAD_ALOAD_GETFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1029)) {
  add_inst(b, "_LLOAD_LCONST_LAND_LCONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1030)) {
  add_inst(b, "_ASTORE_ACONST_ASTORE_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1031)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1032)) {
  add_inst(b, "_GETFIELD_INT_LOOKUPSWITCH_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1033)) {
  add_inst(b, "_ICONST_IOR_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1034)) {
  add_inst(b, "_ILOAD_I2L_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1035)) {
  add_inst(b, "_POP_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1036)) {
  add_inst(b, "_ALOAD_AASTORE_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1037)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1038)) {
  add_inst(b, "_IINC_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1039)) {
  add_inst(b, "_ILOAD_IADD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1040)) {
  add_inst(b, "_CHECKCAST_PUTFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1041)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_ICONST_BASTORE_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1042)) {
  add_inst(b, "_IADD_DUP_X1_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1043)) {
  add_inst(b, "_IADD_DUP_X1_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1044)) {
  add_inst(b, "_IAND_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1045)) {
  add_inst(b, "_ICONST_PUTSTATIC_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1046)) {
  add_inst(b, "_ILOAD_AALOAD_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1047)) {
  add_inst(b, "_ACONST_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1048)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_INT_IFNE_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1049)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_IADD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1050)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_INT_IFNE_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1051)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPNE_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1052)) {
  add_inst(b, "_IALOAD_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1053)) {
  add_inst(b, "_ILOAD_FSUB_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1054)) {
  add_inst(b, "_POP_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1055)) {
  add_inst(b, "_AALOAD_PUTFIELD_CELL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1056)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1057)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_ICONST_BASTORE_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1058)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_LOOKUPSWITCH_");
  ip += 8;
  return;
}
if (VM_IS_INST(*ip, 1059)) {
  add_inst(b, "_ALOAD_ICONST_LALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1060)) {
  add_inst(b, "_ASTORE_ALOAD_GETSTATIC_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1061)) {
  add_inst(b, "_GETFIELD_INT_TABLESWITCH_");
  ip += 8;
  return;
}
if (VM_IS_INST(*ip, 1062)) {
  add_inst(b, "_GETSTATIC_CELL_DUP_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1063)) {
  add_inst(b, "_ICONST_LALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1064)) {
  add_inst(b, "_ALOAD_ALOAD_ICONST_ALOAD_ARRAYLENGTH_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1065)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1066)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_ASTORE_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1067)) {
  add_inst(b, "_GETSTATIC_CELL_ILOAD_AALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1068)) {
  add_inst(b, "_ICONST_ISTORE_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1069)) {
  add_inst(b, "_ILOAD_ALOAD_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1070)) {
  add_inst(b, "_ILOAD_INT2CHAR_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1071)) {
  add_inst(b, "_ILOAD_TABLESWITCH_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1072)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1073)) {
  add_inst(b, "_ACONST_ACONST_ACONST_ACONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1074)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ICONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1075)) {
  add_inst(b, "_ALOAD_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1076)) {
  add_inst(b, "_ARRAYLENGTH_ICONST_ISUB_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1077)) {
  add_inst(b, "_BALOAD_ICONST_IAND_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1078)) {
  add_inst(b, "_ILOAD_IADD_PUTFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1079)) {
  add_inst(b, "_ALOAD_ALOAD_ACONST_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1080)) {
  add_inst(b, "_CALOAD_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1081)) {
  add_inst(b, "_ILOAD_ICONST_IF_ICMPGE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1082)) {
  add_inst(b, "_ACONST_MONITOREXIT_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1083)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_IF_ICMPLE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 1084)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1085)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_IF_ICMPNE_");
  ip += 8;
  return;
}
if (VM_IS_INST(*ip, 1086)) {
  add_inst(b, "_ALOAD_ICONST_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1087)) {
  add_inst(b, "_GETFIELD_INT_ALOAD_GETFIELD_CELL_GETFIELD_INT_IF_ICMPEQ_");
  ip += 9;
  return;
}
if (VM_IS_INST(*ip, 1088)) {
  add_inst(b, "_GETFIELD_LONG_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1089)) {
  add_inst(b, "_ICONST_AALOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1090)) {
  add_inst(b, "_IOR_INT2SHORT_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1091)) {
  add_inst(b, "_LLOAD_LCONST_LAND_LCONST_IF_LCMPEQ_");
  ip += 7;
  return;
}
if (VM_IS_INST(*ip, 1092)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1093)) {
  add_inst(b, "_AALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1094)) {
  add_inst(b, "_ALOAD_ALOAD_GETFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1095)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1096)) {
  add_inst(b, "_ALOAD_DUP_GETFIELD_INT_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1097)) {
  add_inst(b, "_ALOAD_GETFIELD_LONG_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1098)) {
  add_inst(b, "_ALOAD_ILOAD_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1099)) {
  add_inst(b, "_ALOAD_ILOAD_ALOAD_ILOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1100)) {
  add_inst(b, "_DUP_GETFIELD_INT_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1101)) {
  add_inst(b, "_ICONST_ACONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1102)) {
  add_inst(b, "_PUTFIELD_CELL_ICONST_ISTORE_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1103)) {
  add_inst(b, "_AASTORE_ALOAD_ICONST_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1104)) {
  add_inst(b, "_ALOAD_ILOAD_ALOAD_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1105)) {
  add_inst(b, "_ALOAD_MONITOREXIT_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1106)) {
  add_inst(b, "_ASTORE_GETSTATIC_CELL_ACONST_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1107)) {
  add_inst(b, "_CHECKCAST_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1108)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_CALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1109)) {
  add_inst(b, "_IADD_ISTORE_ALOAD_GETFIELD_INT_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1110)) {
  add_inst(b, "_MONITOREXIT_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1111)) {
  add_inst(b, "_ASTORE_ALOAD_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1112)) {
  add_inst(b, "_ISTORE_ICONST_ISTORE_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1113)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_ICONST_ACONST_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1114)) {
  add_inst(b, "_PUTFIELD_CELL_ALOAD_IFNULL_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 1115)) {
  add_inst(b, "_ALOAD_ILOAD_I2F_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1116)) {
  add_inst(b, "_ALOAD_PUTFIELD_CELL_GOTO_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 1117)) {
  add_inst(b, "_ARRAYLENGTH_ALOAD_GETFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1118)) {
  add_inst(b, "_ASTORE_ALOAD_ALOAD_PUTFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1119)) {
  add_inst(b, "_DUP2_IALOAD_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1120)) {
  add_inst(b, "_DUP_ILOAD_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1121)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IADD_DUP_X1_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1122)) {
  add_inst(b, "_GETFIELD_INT_ICONST_IADD_DUP_X1_PUTFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1123)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ALOAD_DUP_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1124)) {
  add_inst(b, "_IADD_PUTFIELD_INT_ALOAD_DUP_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1125)) {
  add_inst(b, "_IAND_ICONST_ISHL_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1126)) {
  add_inst(b, "_ICONST_IADD_DUP_X1_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1127)) {
  add_inst(b, "_ICONST_IADD_DUP_X1_PUTFIELD_INT_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1128)) {
  add_inst(b, "_ICONST_IAND_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1129)) {
  add_inst(b, "_ICONST_IAND_ICONST_ISHL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1130)) {
  add_inst(b, "_ILOAD_AALOAD_PUTFIELD_CELL_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1131)) {
  add_inst(b, "_LLOAD_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1132)) {
  add_inst(b, "_ALOAD_ICONST_IALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1133)) {
  add_inst(b, "_ARRAYLENGTH_ACONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1134)) {
  add_inst(b, "_ASTORE_ALOAD_INSTANCEOF_IFEQ_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1135)) {
  add_inst(b, "_DUP_GETFIELD_INT_ICONST_IADD_DUP_X1_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1136)) {
  add_inst(b, "_FADD_ISTORE_ILOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1137)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_CELL_ILOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1138)) {
  add_inst(b, "_IALOAD_ICONST_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1139)) {
  add_inst(b, "_ICONST_ALOAD_ALOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1140)) {
  add_inst(b, "_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1141)) {
  add_inst(b, "_ISTORE_ICONST_ILOAD_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1142)) {
  add_inst(b, "_ISUB_ALOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1143)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IFGE_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 1144)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_IF_ICMPEQ_");
  ip += 5;
  return;
}
if (VM_IS_INST(*ip, 1145)) {
  add_inst(b, "_ALOAD_IF_ACMPNE_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 1146)) {
  add_inst(b, "_F2I_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1147)) {
  add_inst(b, "_FMUL_F2I_PUTFIELD_INT_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1148)) {
  add_inst(b, "_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_ALOAD_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1149)) {
  add_inst(b, "_GETFIELD_INT_IFGE_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1150)) {
  add_inst(b, "_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1151)) {
  add_inst(b, "_ALOAD_ARRAYLENGTH_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1152)) {
  add_inst(b, "_ALOAD_GETFIELD_INT_ICONST_IAND_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1153)) {
  add_inst(b, "_CALOAD_ILOAD_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1154)) {
  add_inst(b, "_DUP_ICONST_ICONST_ICONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1155)) {
  add_inst(b, "_IADD_ISTORE_GOTO_");
  ip += 3;
  return;
}
if (VM_IS_INST(*ip, 1156)) {
  add_inst(b, "_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1157)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_INT_IF_ICMPLE_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1158)) {
  add_inst(b, "_ILOAD_ILOAD_ISUB_ISTORE_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1159)) {
  add_inst(b, "_INT2BYTE_BASTORE_");
  ip += 1;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1160)) {
  add_inst(b, "_AALOAD_PUTFIELD_CELL_ALOAD_GETFIELD_CELL_ILOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1161)) {
  add_inst(b, "_ACONST_ASTORE_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1162)) {
  add_inst(b, "_ALOAD_ALOAD_PUTFIELD_CELL_GOTO_");
  ip += 6;
  return;
}
if (VM_IS_INST(*ip, 1163)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_GETFIELD_INT_PUTFIELD_INT_ALOAD_");
  ip += 9;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1164)) {
  add_inst(b, "_ALOAD_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1165)) {
  add_inst(b, "_ALOAD_ICONST_ALOAD_ICONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1166)) {
  add_inst(b, "_ALOAD_ICONST_LALOAD_LCONST_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1167)) {
  add_inst(b, "_ALOAD_MONITORENTER_ALOAD_GETFIELD_CELL_");
  ip += 5;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1168)) {
  add_inst(b, "_BALOAD_ICONST_IAND_ICONST_ISHL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1169)) {
  add_inst(b, "_DUP_X1_PUTFIELD_CELL_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1170)) {
  add_inst(b, "_FMUL_ISTORE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1171)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1172)) {
  add_inst(b, "_GETFIELD_CELL_ILOAD_AALOAD_PUTFIELD_CELL_ALOAD_");
  ip += 7;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1173)) {
  add_inst(b, "_ICONST_ALOAD_GETFIELD_INT_ALOAD_GETFIELD_INT_");
  ip += 8;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1174)) {
  add_inst(b, "_ICONST_ILOAD_ISUB_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1175)) {
  add_inst(b, "_ICONST_LALOAD_LCONST_");
  ip += 4;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1176)) {
  add_inst(b, "_ILOAD_ALOAD_GETFIELD_CELL_ALOAD_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1177)) {
  add_inst(b, "_ISTORE_ILOAD_IFEQ_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1178)) {
  add_inst(b, "_ISUB_PUTFIELD_INT_ALOAD_GETFIELD_CELL_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1179)) {
  add_inst(b, "_BALOAD_ICONST_IAND_ALOAD_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1180)) {
  add_inst(b, "_GETFIELD_INT_IASTORE_");
  ip += 3;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1181)) {
  add_inst(b, "_IADD_ALOAD_ARRAYLENGTH_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1182)) {
  add_inst(b, "_ILOAD_INT2BYTE_");
  ip += 2;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1183)) {
  add_inst(b, "_ILOAD_ISTORE_GOTO_");
  ip += 4;
  return;
}
if (VM_IS_INST(*ip, 1184)) {
  add_inst(b, "_ISTORE_ILOAD_ALOAD_GETFIELD_INT_");
  ip += 6;
  goto _endif_;
}
if (VM_IS_INST(*ip, 1185)) {
  add_inst(b, "_PUTFIELD_INT_ALOAD_ALOAD_ALOAD_");
  ip += 6;
  goto _endif_;
}
