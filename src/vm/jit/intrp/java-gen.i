void gen_ICONST(Inst **ctp, Cell vConst)
{
  gen_inst(ctp, vm_prim[0]);
  genarg_v(ctp, vConst);
}
void gen_LCONST(Inst **ctp, s8 l)
{
  gen_inst(ctp, vm_prim[1]);
  genarg_l(ctp, l);
}
void gen_ACONST(Inst **ctp, java_objectheader * aRef)
{
  gen_inst(ctp, vm_prim[2]);
  genarg_aRef(ctp, aRef);
}
void gen_ILOAD(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[3]);
  genarg_v(ctp, vLocal);
}
void gen_LLOAD(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[4]);
  genarg_v(ctp, vLocal);
}
void gen_ALOAD(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[5]);
  genarg_v(ctp, vLocal);
}
void gen_IALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[6]);
}
void gen_LALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[7]);
}
void gen_AALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[8]);
}
void gen_BALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[9]);
}
void gen_CALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[10]);
}
void gen_SALOAD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[11]);
}
void gen_ISTORE(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[12]);
  genarg_v(ctp, vLocal);
}
void gen_LSTORE(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[13]);
  genarg_v(ctp, vLocal);
}
void gen_ASTORE(Inst **ctp, Cell vLocal)
{
  gen_inst(ctp, vm_prim[14]);
  genarg_v(ctp, vLocal);
}
void gen_IASTORE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[15]);
}
void gen_LASTORE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[16]);
}
void gen_AASTORE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[17]);
}
void gen_BASTORE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[18]);
}
void gen_CASTORE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[19]);
}
void gen_POP(Inst **ctp)
{
  gen_inst(ctp, vm_prim[20]);
}
void gen_POP2(Inst **ctp)
{
  gen_inst(ctp, vm_prim[21]);
}
void gen_DUP(Inst **ctp)
{
  gen_inst(ctp, vm_prim[22]);
}
void gen_DUP_X1(Inst **ctp)
{
  gen_inst(ctp, vm_prim[23]);
}
void gen_DUP_X2(Inst **ctp)
{
  gen_inst(ctp, vm_prim[24]);
}
void gen_DUP2(Inst **ctp)
{
  gen_inst(ctp, vm_prim[25]);
}
void gen_DUP2_X1(Inst **ctp)
{
  gen_inst(ctp, vm_prim[26]);
}
void gen_DUP2_X2(Inst **ctp)
{
  gen_inst(ctp, vm_prim[27]);
}
void gen_SWAP(Inst **ctp)
{
  gen_inst(ctp, vm_prim[28]);
}
void gen_IADD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[29]);
}
void gen_LADD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[30]);
}
void gen_FADD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[31]);
}
void gen_DADD(Inst **ctp)
{
  gen_inst(ctp, vm_prim[32]);
}
void gen_ISUB(Inst **ctp)
{
  gen_inst(ctp, vm_prim[33]);
}
void gen_LSUB(Inst **ctp)
{
  gen_inst(ctp, vm_prim[34]);
}
void gen_FSUB(Inst **ctp)
{
  gen_inst(ctp, vm_prim[35]);
}
void gen_DSUB(Inst **ctp)
{
  gen_inst(ctp, vm_prim[36]);
}
void gen_IMUL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[37]);
}
void gen_LMUL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[38]);
}
void gen_FMUL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[39]);
}
void gen_DMUL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[40]);
}
void gen_IDIV(Inst **ctp)
{
  gen_inst(ctp, vm_prim[41]);
}
void gen_IDIVPOW2(Inst **ctp, s4 ishift)
{
  gen_inst(ctp, vm_prim[42]);
  genarg_i(ctp, ishift);
}
void gen_LDIV(Inst **ctp)
{
  gen_inst(ctp, vm_prim[43]);
}
void gen_LDIVPOW2(Inst **ctp, s4 ishift)
{
  gen_inst(ctp, vm_prim[44]);
  genarg_i(ctp, ishift);
}
void gen_FDIV(Inst **ctp)
{
  gen_inst(ctp, vm_prim[45]);
}
void gen_DDIV(Inst **ctp)
{
  gen_inst(ctp, vm_prim[46]);
}
void gen_IREM(Inst **ctp)
{
  gen_inst(ctp, vm_prim[47]);
}
void gen_IREMPOW2(Inst **ctp, s4 imask)
{
  gen_inst(ctp, vm_prim[48]);
  genarg_i(ctp, imask);
}
void gen_LREM(Inst **ctp)
{
  gen_inst(ctp, vm_prim[49]);
}
void gen_LREMPOW2(Inst **ctp, s8 lmask)
{
  gen_inst(ctp, vm_prim[50]);
  genarg_l(ctp, lmask);
}
void gen_FREM(Inst **ctp)
{
  gen_inst(ctp, vm_prim[51]);
}
void gen_DREM(Inst **ctp)
{
  gen_inst(ctp, vm_prim[52]);
}
void gen_INEG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[53]);
}
void gen_LNEG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[54]);
}
void gen_FNEG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[55]);
}
void gen_DNEG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[56]);
}
void gen_ISHL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[57]);
}
void gen_LSHL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[58]);
}
void gen_ISHR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[59]);
}
void gen_LSHR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[60]);
}
void gen_IUSHR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[61]);
}
void gen_LUSHR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[62]);
}
void gen_IAND(Inst **ctp)
{
  gen_inst(ctp, vm_prim[63]);
}
void gen_LAND(Inst **ctp)
{
  gen_inst(ctp, vm_prim[64]);
}
void gen_IOR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[65]);
}
void gen_LOR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[66]);
}
void gen_IXOR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[67]);
}
void gen_LXOR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[68]);
}
void gen_IINC(Inst **ctp, Cell vOffset, s4 iConst)
{
  gen_inst(ctp, vm_prim[69]);
  genarg_v(ctp, vOffset);
  genarg_i(ctp, iConst);
}
void gen_I2L(Inst **ctp)
{
  gen_inst(ctp, vm_prim[70]);
}
void gen_I2F(Inst **ctp)
{
  gen_inst(ctp, vm_prim[71]);
}
void gen_I2D(Inst **ctp)
{
  gen_inst(ctp, vm_prim[72]);
}
void gen_L2I(Inst **ctp)
{
  gen_inst(ctp, vm_prim[73]);
}
void gen_L2F(Inst **ctp)
{
  gen_inst(ctp, vm_prim[74]);
}
void gen_L2D(Inst **ctp)
{
  gen_inst(ctp, vm_prim[75]);
}
void gen_F2I(Inst **ctp)
{
  gen_inst(ctp, vm_prim[76]);
}
void gen_F2L(Inst **ctp)
{
  gen_inst(ctp, vm_prim[77]);
}
void gen_F2D(Inst **ctp)
{
  gen_inst(ctp, vm_prim[78]);
}
void gen_D2I(Inst **ctp)
{
  gen_inst(ctp, vm_prim[79]);
}
void gen_D2L(Inst **ctp)
{
  gen_inst(ctp, vm_prim[80]);
}
void gen_D2F(Inst **ctp)
{
  gen_inst(ctp, vm_prim[81]);
}
void gen_INT2BYTE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[82]);
}
void gen_INT2CHAR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[83]);
}
void gen_INT2SHORT(Inst **ctp)
{
  gen_inst(ctp, vm_prim[84]);
}
void gen_LCMP(Inst **ctp)
{
  gen_inst(ctp, vm_prim[85]);
}
void gen_FCMPL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[86]);
}
void gen_FCMPG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[87]);
}
void gen_DCMPL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[88]);
}
void gen_DCMPG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[89]);
}
void gen_IFEQ(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[90]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFNE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[91]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFLT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[92]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFGE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[93]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFGT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[94]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFLE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[95]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPEQ(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[96]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPNE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[97]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPLT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[98]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPGE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[99]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPGT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[100]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ICMPLE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[101]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPEQ(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[102]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPNE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[103]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPLT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[104]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPGE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[105]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPGT(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[106]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_LCMPLE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[107]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ACMPEQ(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[108]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IF_ACMPNE(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[109]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_GOTO(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[110]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_JSR(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[111]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_RET(Inst **ctp, Cell vOffset)
{
  gen_inst(ctp, vm_prim[112]);
  genarg_v(ctp, vOffset);
}
void gen_TABLESWITCH(Inst **ctp, s4 iLow, s4 iRange, u1 * addrSegment, s4 iOffset, Inst * ainstDefault)
{
  gen_inst(ctp, vm_prim[113]);
  genarg_i(ctp, iLow);
  genarg_i(ctp, iRange);
  genarg_addr(ctp, addrSegment);
  genarg_i(ctp, iOffset);
  genarg_ainst(ctp, ainstDefault);
}
void gen_LOOKUPSWITCH(Inst **ctp, s4 iNpairs, u1 * addrSegment, s4 iOffset, Inst * ainstDefault)
{
  gen_inst(ctp, vm_prim[114]);
  genarg_i(ctp, iNpairs);
  genarg_addr(ctp, addrSegment);
  genarg_i(ctp, iOffset);
  genarg_ainst(ctp, ainstDefault);
}
void gen_IRETURN(Inst **ctp, Cell vOffsetFP)
{
  gen_inst(ctp, vm_prim[115]);
  genarg_v(ctp, vOffsetFP);
}
void gen_LRETURN(Inst **ctp, Cell vOffsetFP)
{
  gen_inst(ctp, vm_prim[116]);
  genarg_v(ctp, vOffsetFP);
}
void gen_RETURN(Inst **ctp, Cell vOffsetFP)
{
  gen_inst(ctp, vm_prim[117]);
  genarg_v(ctp, vOffsetFP);
}
void gen_GETSTATIC_CELL(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[118]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_GETSTATIC_INT(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[119]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_GETSTATIC_LONG(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[120]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_PUTSTATIC_CELL(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[121]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_PUTSTATIC_INT(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[122]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_PUTSTATIC_LONG(Inst **ctp, u1 * addr, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[123]);
  genarg_addr(ctp, addr);
  genarg_auf(ctp, auf);
}
void gen_GETFIELD_CELL(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[124]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_GETFIELD_INT(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[125]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_GETFIELD_LONG(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[126]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PUTFIELD_CELL(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[127]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PUTFIELD_INT(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[128]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PUTFIELD_LONG(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[129]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_INVOKEVIRTUAL(Inst **ctp, Cell vOffset, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[130]);
  genarg_v(ctp, vOffset);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_INVOKESTATIC(Inst **ctp, Inst ** aaTarget, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[131]);
  genarg_aaTarget(ctp, aaTarget);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_INVOKESPECIAL(Inst **ctp, Inst ** aaTarget, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[132]);
  genarg_aaTarget(ctp, aaTarget);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_INVOKEINTERFACE(Inst **ctp, s4 iInterfaceOffset, Cell vOffset, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[133]);
  genarg_i(ctp, iInterfaceOffset);
  genarg_v(ctp, vOffset);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_NEW(Inst **ctp)
{
  gen_inst(ctp, vm_prim[134]);
}
void gen_NEWARRAY_BOOLEAN(Inst **ctp)
{
  gen_inst(ctp, vm_prim[135]);
}
void gen_NEWARRAY_BYTE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[136]);
}
void gen_NEWARRAY_CHAR(Inst **ctp)
{
  gen_inst(ctp, vm_prim[137]);
}
void gen_NEWARRAY_SHORT(Inst **ctp)
{
  gen_inst(ctp, vm_prim[138]);
}
void gen_NEWARRAY_INT(Inst **ctp)
{
  gen_inst(ctp, vm_prim[139]);
}
void gen_NEWARRAY_LONG(Inst **ctp)
{
  gen_inst(ctp, vm_prim[140]);
}
void gen_NEWARRAY_FLOAT(Inst **ctp)
{
  gen_inst(ctp, vm_prim[141]);
}
void gen_NEWARRAY_DOUBLE(Inst **ctp)
{
  gen_inst(ctp, vm_prim[142]);
}
void gen_NEWARRAY(Inst **ctp)
{
  gen_inst(ctp, vm_prim[143]);
}
void gen_ARRAYLENGTH(Inst **ctp)
{
  gen_inst(ctp, vm_prim[144]);
}
void gen_ATHROW(Inst **ctp)
{
  gen_inst(ctp, vm_prim[145]);
}
void gen_CHECKCAST(Inst **ctp, classinfo * aClass, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[146]);
  genarg_aClass(ctp, aClass);
  genarg_acr(ctp, acr);
}
void gen_ARRAYCHECKCAST(Inst **ctp, vftbl_t * avftbl, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[147]);
  genarg_avftbl(ctp, avftbl);
  genarg_acr(ctp, acr);
}
void gen_INSTANCEOF(Inst **ctp, classinfo * aClass, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[148]);
  genarg_aClass(ctp, aClass);
  genarg_acr(ctp, acr);
}
void gen_ARRAYINSTANCEOF(Inst **ctp)
{
  gen_inst(ctp, vm_prim[149]);
}
void gen_MONITORENTER(Inst **ctp)
{
  gen_inst(ctp, vm_prim[150]);
}
void gen_MONITOREXIT(Inst **ctp)
{
  gen_inst(ctp, vm_prim[151]);
}
void gen_CHECKNULL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[152]);
}
void gen_MULTIANEWARRAY(Inst **ctp, vftbl_t * avftbl, s4 iSize, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[153]);
  genarg_avftbl(ctp, avftbl);
  genarg_i(ctp, iSize);
  genarg_acr(ctp, acr);
}
void gen_IFNULL(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[154]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_IFNONNULL(Inst **ctp, Inst * ainstTarget)
{
  gen_inst(ctp, vm_prim[155]);
  genarg_ainst(ctp, ainstTarget);
}
void gen_PATCHER_GETSTATIC_INT(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[156]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_GETSTATIC_LONG(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[157]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_GETSTATIC_CELL(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[158]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTSTATIC_INT(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[159]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTSTATIC_LONG(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[160]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTSTATIC_CELL(Inst **ctp, java_objectheader * aRef, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[161]);
  genarg_aRef(ctp, aRef);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_GETFIELD_INT(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[162]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_GETFIELD_LONG(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[163]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_GETFIELD_CELL(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[164]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTFIELD_INT(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[165]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTFIELD_LONG(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[166]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_PATCHER_PUTFIELD_CELL(Inst **ctp, Cell vOffset, unresolved_field * auf)
{
  gen_inst(ctp, vm_prim[167]);
  genarg_v(ctp, vOffset);
  genarg_auf(ctp, auf);
}
void gen_NEW1(Inst **ctp, classinfo * aClass)
{
  gen_inst(ctp, vm_prim[168]);
  genarg_aClass(ctp, aClass);
}
void gen_PATCHER_NEW(Inst **ctp, classinfo * aClass)
{
  gen_inst(ctp, vm_prim[169]);
  genarg_aClass(ctp, aClass);
}
void gen_NEWARRAY1(Inst **ctp, vftbl_t * avftbl)
{
  gen_inst(ctp, vm_prim[170]);
  genarg_avftbl(ctp, avftbl);
}
void gen_PATCHER_NEWARRAY(Inst **ctp, vftbl_t * avftbl)
{
  gen_inst(ctp, vm_prim[171]);
  genarg_avftbl(ctp, avftbl);
}
void gen_PATCHER_MULTIANEWARRAY(Inst **ctp, vftbl_t * avftbl, s4 iSize, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[172]);
  genarg_avftbl(ctp, avftbl);
  genarg_i(ctp, iSize);
  genarg_acr(ctp, acr);
}
void gen_ARRAYINSTANCEOF1(Inst **ctp, vftbl_t * avftbl)
{
  gen_inst(ctp, vm_prim[173]);
  genarg_avftbl(ctp, avftbl);
}
void gen_PATCHER_ARRAYINSTANCEOF(Inst **ctp, vftbl_t * avftbl)
{
  gen_inst(ctp, vm_prim[174]);
  genarg_avftbl(ctp, avftbl);
}
void gen_PATCHER_INVOKESTATIC(Inst **ctp, Inst ** aaTarget, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[175]);
  genarg_aaTarget(ctp, aaTarget);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_PATCHER_INVOKESPECIAL(Inst **ctp, Inst ** aaTarget, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[176]);
  genarg_aaTarget(ctp, aaTarget);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_PATCHER_INVOKEVIRTUAL(Inst **ctp, Cell vOffset, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[177]);
  genarg_v(ctp, vOffset);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_PATCHER_INVOKEINTERFACE(Inst **ctp, s4 iInterfaceoffset, Cell vOffset, s4 iNargs, unresolved_method * aum)
{
  gen_inst(ctp, vm_prim[178]);
  genarg_i(ctp, iInterfaceoffset);
  genarg_v(ctp, vOffset);
  genarg_i(ctp, iNargs);
  genarg_aum(ctp, aum);
}
void gen_PATCHER_CHECKCAST(Inst **ctp, classinfo * aClass, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[179]);
  genarg_aClass(ctp, aClass);
  genarg_acr(ctp, acr);
}
void gen_PATCHER_ARRAYCHECKCAST(Inst **ctp, vftbl_t * avftbl, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[180]);
  genarg_avftbl(ctp, avftbl);
  genarg_acr(ctp, acr);
}
void gen_PATCHER_INSTANCEOF(Inst **ctp, classinfo * aClass, constant_classref * acr)
{
  gen_inst(ctp, vm_prim[181]);
  genarg_aClass(ctp, aClass);
  genarg_acr(ctp, acr);
}
void gen_TRANSLATE(Inst **ctp, methodinfo * am)
{
  gen_inst(ctp, vm_prim[182]);
  genarg_am(ctp, am);
}
void gen_NATIVECALL(Inst **ctp, methodinfo * am, functionptr af, u1 * addrcif)
{
  gen_inst(ctp, vm_prim[183]);
  genarg_am(ctp, am);
  genarg_af(ctp, af);
  genarg_addr(ctp, addrcif);
}
void gen_TRACENATIVECALL(Inst **ctp, methodinfo * am, functionptr af, u1 * addrcif)
{
  gen_inst(ctp, vm_prim[184]);
  genarg_am(ctp, am);
  genarg_af(ctp, af);
  genarg_addr(ctp, addrcif);
}
void gen_PATCHER_NATIVECALL(Inst **ctp, methodinfo * am, functionptr af, u1 * addrcif)
{
  gen_inst(ctp, vm_prim[185]);
  genarg_am(ctp, am);
  genarg_af(ctp, af);
  genarg_addr(ctp, addrcif);
}
void gen_TRACECALL(Inst **ctp)
{
  gen_inst(ctp, vm_prim[186]);
}
void gen_TRACERETURN(Inst **ctp)
{
  gen_inst(ctp, vm_prim[187]);
}
void gen_TRACELRETURN(Inst **ctp)
{
  gen_inst(ctp, vm_prim[188]);
}
void gen_END(Inst **ctp)
{
  gen_inst(ctp, vm_prim[189]);
}
