typedef struct burm_state *STATEPTR_TYPE;

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "grammar.h"
#ifndef ALLOC
#define ALLOC(n) malloc(n)
#endif

#ifndef burm_assert
#define burm_assert(x,y) if (!(x)) { extern void abort(void); y; abort(); }
#endif

#define burm_reg_NT 1
int burm_max_nt = 1;

char *burm_ntname[] = {
	0,
	"reg",
	0
};

struct burm_state {
	int op;
	STATEPTR_TYPE left, right;
	short cost[2];
	struct {
		unsigned burm_reg:9;
	} rule;
};

static short burm_nts_0[] = { 0 };

short *burm_nts[] = {
	0,	/* 0 */
	burm_nts_0,	/* 1 */
	burm_nts_0,	/* 2 */
	burm_nts_0,	/* 3 */
	burm_nts_0,	/* 4 */
	burm_nts_0,	/* 5 */
	burm_nts_0,	/* 6 */
	burm_nts_0,	/* 7 */
	burm_nts_0,	/* 8 */
	burm_nts_0,	/* 9 */
	burm_nts_0,	/* 10 */
	burm_nts_0,	/* 11 */
	burm_nts_0,	/* 12 */
	burm_nts_0,	/* 13 */
	burm_nts_0,	/* 14 */
	burm_nts_0,	/* 15 */
	burm_nts_0,	/* 16 */
	burm_nts_0,	/* 17 */
	burm_nts_0,	/* 18 */
	burm_nts_0,	/* 19 */
	burm_nts_0,	/* 20 */
	burm_nts_0,	/* 21 */
	burm_nts_0,	/* 22 */
	burm_nts_0,	/* 23 */
	burm_nts_0,	/* 24 */
	burm_nts_0,	/* 25 */
	burm_nts_0,	/* 26 */
	burm_nts_0,	/* 27 */
	burm_nts_0,	/* 28 */
	burm_nts_0,	/* 29 */
	burm_nts_0,	/* 30 */
	burm_nts_0,	/* 31 */
	burm_nts_0,	/* 32 */
	burm_nts_0,	/* 33 */
	burm_nts_0,	/* 34 */
	burm_nts_0,	/* 35 */
	burm_nts_0,	/* 36 */
	burm_nts_0,	/* 37 */
	burm_nts_0,	/* 38 */
	burm_nts_0,	/* 39 */
	burm_nts_0,	/* 40 */
	burm_nts_0,	/* 41 */
	burm_nts_0,	/* 42 */
	burm_nts_0,	/* 43 */
	burm_nts_0,	/* 44 */
	burm_nts_0,	/* 45 */
	burm_nts_0,	/* 46 */
	burm_nts_0,	/* 47 */
	burm_nts_0,	/* 48 */
	burm_nts_0,	/* 49 */
	burm_nts_0,	/* 50 */
	burm_nts_0,	/* 51 */
	burm_nts_0,	/* 52 */
	burm_nts_0,	/* 53 */
	burm_nts_0,	/* 54 */
	burm_nts_0,	/* 55 */
	burm_nts_0,	/* 56 */
	burm_nts_0,	/* 57 */
	burm_nts_0,	/* 58 */
	burm_nts_0,	/* 59 */
	burm_nts_0,	/* 60 */
	burm_nts_0,	/* 61 */
	burm_nts_0,	/* 62 */
	burm_nts_0,	/* 63 */
	burm_nts_0,	/* 64 */
	burm_nts_0,	/* 65 */
	burm_nts_0,	/* 66 */
	burm_nts_0,	/* 67 */
	burm_nts_0,	/* 68 */
	burm_nts_0,	/* 69 */
	burm_nts_0,	/* 70 */
	burm_nts_0,	/* 71 */
	burm_nts_0,	/* 72 */
	burm_nts_0,	/* 73 */
	burm_nts_0,	/* 74 */
	burm_nts_0,	/* 75 */
	burm_nts_0,	/* 76 */
	burm_nts_0,	/* 77 */
	burm_nts_0,	/* 78 */
	burm_nts_0,	/* 79 */
	burm_nts_0,	/* 80 */
	burm_nts_0,	/* 81 */
	burm_nts_0,	/* 82 */
	burm_nts_0,	/* 83 */
	burm_nts_0,	/* 84 */
	burm_nts_0,	/* 85 */
	burm_nts_0,	/* 86 */
	burm_nts_0,	/* 87 */
	burm_nts_0,	/* 88 */
	burm_nts_0,	/* 89 */
	burm_nts_0,	/* 90 */
	burm_nts_0,	/* 91 */
	burm_nts_0,	/* 92 */
	burm_nts_0,	/* 93 */
	burm_nts_0,	/* 94 */
	burm_nts_0,	/* 95 */
	burm_nts_0,	/* 96 */
	burm_nts_0,	/* 97 */
	burm_nts_0,	/* 98 */
	burm_nts_0,	/* 99 */
	burm_nts_0,	/* 100 */
	burm_nts_0,	/* 101 */
	burm_nts_0,	/* 102 */
	burm_nts_0,	/* 103 */
	burm_nts_0,	/* 104 */
	burm_nts_0,	/* 105 */
	burm_nts_0,	/* 106 */
	burm_nts_0,	/* 107 */
	burm_nts_0,	/* 108 */
	burm_nts_0,	/* 109 */
	burm_nts_0,	/* 110 */
	burm_nts_0,	/* 111 */
	burm_nts_0,	/* 112 */
	burm_nts_0,	/* 113 */
	burm_nts_0,	/* 114 */
	burm_nts_0,	/* 115 */
	burm_nts_0,	/* 116 */
	burm_nts_0,	/* 117 */
	burm_nts_0,	/* 118 */
	burm_nts_0,	/* 119 */
	burm_nts_0,	/* 120 */
	burm_nts_0,	/* 121 */
	burm_nts_0,	/* 122 */
	burm_nts_0,	/* 123 */
	burm_nts_0,	/* 124 */
	burm_nts_0,	/* 125 */
	burm_nts_0,	/* 126 */
	burm_nts_0,	/* 127 */
	burm_nts_0,	/* 128 */
	burm_nts_0,	/* 129 */
	burm_nts_0,	/* 130 */
	burm_nts_0,	/* 131 */
	burm_nts_0,	/* 132 */
	burm_nts_0,	/* 133 */
	burm_nts_0,	/* 134 */
	burm_nts_0,	/* 135 */
	burm_nts_0,	/* 136 */
	burm_nts_0,	/* 137 */
	burm_nts_0,	/* 138 */
	burm_nts_0,	/* 139 */
	burm_nts_0,	/* 140 */
	burm_nts_0,	/* 141 */
	burm_nts_0,	/* 142 */
	burm_nts_0,	/* 143 */
	burm_nts_0,	/* 144 */
	burm_nts_0,	/* 145 */
	burm_nts_0,	/* 146 */
	burm_nts_0,	/* 147 */
	burm_nts_0,	/* 148 */
	burm_nts_0,	/* 149 */
	burm_nts_0,	/* 150 */
	burm_nts_0,	/* 151 */
	burm_nts_0,	/* 152 */
	burm_nts_0,	/* 153 */
	burm_nts_0,	/* 154 */
	burm_nts_0,	/* 155 */
	burm_nts_0,	/* 156 */
	burm_nts_0,	/* 157 */
	burm_nts_0,	/* 158 */
	burm_nts_0,	/* 159 */
	burm_nts_0,	/* 160 */
	burm_nts_0,	/* 161 */
	burm_nts_0,	/* 162 */
	burm_nts_0,	/* 163 */
	burm_nts_0,	/* 164 */
	burm_nts_0,	/* 165 */
	burm_nts_0,	/* 166 */
	burm_nts_0,	/* 167 */
	burm_nts_0,	/* 168 */
	burm_nts_0,	/* 169 */
	burm_nts_0,	/* 170 */
	burm_nts_0,	/* 171 */
	burm_nts_0,	/* 172 */
	burm_nts_0,	/* 173 */
	burm_nts_0,	/* 174 */
	burm_nts_0,	/* 175 */
	burm_nts_0,	/* 176 */
	burm_nts_0,	/* 177 */
	burm_nts_0,	/* 178 */
	burm_nts_0,	/* 179 */
	burm_nts_0,	/* 180 */
	burm_nts_0,	/* 181 */
	burm_nts_0,	/* 182 */
	burm_nts_0,	/* 183 */
	burm_nts_0,	/* 184 */
	burm_nts_0,	/* 185 */
	burm_nts_0,	/* 186 */
	burm_nts_0,	/* 187 */
	burm_nts_0,	/* 188 */
	burm_nts_0,	/* 189 */
	burm_nts_0,	/* 190 */
	burm_nts_0,	/* 191 */
	burm_nts_0,	/* 192 */
	burm_nts_0,	/* 193 */
	burm_nts_0,	/* 194 */
	burm_nts_0,	/* 195 */
	burm_nts_0,	/* 196 */
	burm_nts_0,	/* 197 */
	burm_nts_0,	/* 198 */
	burm_nts_0,	/* 199 */
	burm_nts_0,	/* 200 */
	burm_nts_0,	/* 201 */
	burm_nts_0,	/* 202 */
	burm_nts_0,	/* 203 */
	burm_nts_0,	/* 204 */
	burm_nts_0,	/* 205 */
	burm_nts_0,	/* 206 */
	burm_nts_0,	/* 207 */
	burm_nts_0,	/* 208 */
	burm_nts_0,	/* 209 */
	burm_nts_0,	/* 210 */
	burm_nts_0,	/* 211 */
	burm_nts_0,	/* 212 */
	burm_nts_0,	/* 213 */
	burm_nts_0,	/* 214 */
	burm_nts_0,	/* 215 */
	burm_nts_0,	/* 216 */
	burm_nts_0,	/* 217 */
	burm_nts_0,	/* 218 */
	burm_nts_0,	/* 219 */
	burm_nts_0,	/* 220 */
	burm_nts_0,	/* 221 */
	burm_nts_0,	/* 222 */
	burm_nts_0,	/* 223 */
	burm_nts_0,	/* 224 */
	burm_nts_0,	/* 225 */
	burm_nts_0,	/* 226 */
	burm_nts_0,	/* 227 */
	burm_nts_0,	/* 228 */
	burm_nts_0,	/* 229 */
	burm_nts_0,	/* 230 */
	burm_nts_0,	/* 231 */
	burm_nts_0,	/* 232 */
	burm_nts_0,	/* 233 */
	burm_nts_0,	/* 234 */
	burm_nts_0,	/* 235 */
	burm_nts_0,	/* 236 */
	burm_nts_0,	/* 237 */
	burm_nts_0,	/* 238 */
	burm_nts_0,	/* 239 */
	burm_nts_0,	/* 240 */
	burm_nts_0,	/* 241 */
	burm_nts_0,	/* 242 */
	burm_nts_0,	/* 243 */
	burm_nts_0,	/* 244 */
	burm_nts_0,	/* 245 */
	burm_nts_0,	/* 246 */
	burm_nts_0,	/* 247 */
	burm_nts_0,	/* 248 */
	burm_nts_0,	/* 249 */
	burm_nts_0,	/* 250 */
	burm_nts_0,	/* 251 */
	burm_nts_0,	/* 252 */
	burm_nts_0,	/* 253 */
	burm_nts_0,	/* 254 */
	burm_nts_0,	/* 255 */
	burm_nts_0,	/* 256 */
};

char burm_arity[] = {
	0,	/* 0=NOP */
	0,	/* 1=ACONST */
	0,	/* 2=CHECKNULL */
	0,	/* 3=ICONST */
	0,	/* 4=UNDEF4 */
	0,	/* 5=IDIVPOW2 */
	0,	/* 6=LDIVPOW2 */
	0,	/* 7=UNDEF7 */
	0,	/* 8=UNDEF8 */
	0,	/* 9=LCONST */
	0,	/* 10=LCMPCONST */
	0,	/* 11=FCONST */
	0,	/* 12=UNDEF12 */
	0,	/* 13=UNDEF13 */
	0,	/* 14=DCONST */
	0,	/* 15=COPY */
	0,	/* 16=MOVE */
	0,	/* 17=UNDEF17 */
	0,	/* 18=UNDEF18 */
	0,	/* 19=UNDEF19 */
	0,	/* 20=UNDEF20 */
	0,	/* 21=ILOAD */
	0,	/* 22=LLOAD */
	0,	/* 23=FLOAD */
	0,	/* 24=DLOAD */
	0,	/* 25=ALOAD */
	0,	/* 26=IADDCONST */
	0,	/* 27=ISUBCONST */
	0,	/* 28=IMULCONST */
	0,	/* 29=IANDCONST */
	0,	/* 30=IORCONST */
	0,	/* 31=IXORCONST */
	0,	/* 32=ISHLCONST */
	0,	/* 33=ISHRCONST */
	0,	/* 34=IUSHRCONST */
	0,	/* 35=IREMPOW2 */
	0,	/* 36=LADDCONST */
	0,	/* 37=LSUBCONST */
	0,	/* 38=LMULCONST */
	0,	/* 39=LANDCONST */
	0,	/* 40=LORCONST */
	0,	/* 41=LXORCONST */
	0,	/* 42=LSHLCONST */
	0,	/* 43=LSHRCONST */
	0,	/* 44=LUSHRCONST */
	0,	/* 45=LREMPOW2 */
	0,	/* 46=IALOAD */
	0,	/* 47=LALOAD */
	0,	/* 48=FALOAD */
	0,	/* 49=DALOAD */
	0,	/* 50=AALOAD */
	0,	/* 51=BALOAD */
	0,	/* 52=CALOAD */
	0,	/* 53=SALOAD */
	0,	/* 54=ISTORE */
	0,	/* 55=LSTORE */
	0,	/* 56=FSTORE */
	0,	/* 57=DSTORE */
	0,	/* 58=ASTORE */
	0,	/* 59=IF_LEQ */
	0,	/* 60=IF_LNE */
	0,	/* 61=IF_LLT */
	0,	/* 62=IF_LGE */
	0,	/* 63=IF_LGT */
	0,	/* 64=IF_LLE */
	0,	/* 65=IF_LCMPEQ */
	0,	/* 66=IF_LCMPNE */
	0,	/* 67=IF_LCMPLT */
	0,	/* 68=IF_LCMPGE */
	0,	/* 69=IF_LCMPGT */
	0,	/* 70=IF_LCMPLE */
	0,	/* 71=UNDEF71 */
	0,	/* 72=UNDEF72 */
	0,	/* 73=UNDEF73 */
	0,	/* 74=UNDEF74 */
	0,	/* 75=UNDEF75 */
	0,	/* 76=UNDEF76 */
	0,	/* 77=UNDEF77 */
	0,	/* 78=UNDEF78 */
	0,	/* 79=IASTORE */
	0,	/* 80=LASTORE */
	0,	/* 81=FASTORE */
	0,	/* 82=DASTORE */
	0,	/* 83=AASTORE */
	0,	/* 84=BASTORE */
	0,	/* 85=CASTORE */
	0,	/* 86=SASTORE */
	0,	/* 87=POP */
	0,	/* 88=POP2 */
	0,	/* 89=DUP */
	0,	/* 90=DUP_X1 */
	0,	/* 91=DUP_X2 */
	0,	/* 92=DUP2 */
	0,	/* 93=DUP2_X1 */
	0,	/* 94=DUP2_X2 */
	0,	/* 95=SWAP */
	0,	/* 96=IADD */
	0,	/* 97=LADD */
	0,	/* 98=FADD */
	0,	/* 99=DADD */
	0,	/* 100=ISUB */
	0,	/* 101=LSUB */
	0,	/* 102=FSUB */
	0,	/* 103=DSUB */
	0,	/* 104=IMUL */
	0,	/* 105=LMUL */
	0,	/* 106=FMUL */
	0,	/* 107=DMUL */
	0,	/* 108=IDIV */
	0,	/* 109=LDIV */
	0,	/* 110=FDIV */
	0,	/* 111=DDIV */
	0,	/* 112=IREM */
	0,	/* 113=LREM */
	0,	/* 114=FREM */
	0,	/* 115=DREM */
	0,	/* 116=INEG */
	0,	/* 117=LNEG */
	0,	/* 118=FNEG */
	0,	/* 119=DNEG */
	0,	/* 120=ISHL */
	0,	/* 121=LSHL */
	0,	/* 122=ISHR */
	0,	/* 123=LSHR */
	0,	/* 124=IUSHR */
	0,	/* 125=LUSHR */
	0,	/* 126=IAND */
	0,	/* 127=LAND */
	0,	/* 128=IOR */
	0,	/* 129=LOR */
	0,	/* 130=IXOR */
	0,	/* 131=LXOR */
	0,	/* 132=IINC */
	0,	/* 133=I2L */
	0,	/* 134=I2F */
	0,	/* 135=I2D */
	0,	/* 136=L2I */
	0,	/* 137=L2F */
	0,	/* 138=L2D */
	0,	/* 139=F2I */
	0,	/* 140=F2L */
	0,	/* 141=F2D */
	0,	/* 142=D2I */
	0,	/* 143=D2L */
	0,	/* 144=D2F */
	0,	/* 145=INT2BYTE */
	0,	/* 146=INT2CHAR */
	0,	/* 147=INT2SHORT */
	0,	/* 148=LCMP */
	0,	/* 149=FCMPL */
	0,	/* 150=FCMPG */
	0,	/* 151=DCMPL */
	0,	/* 152=DCMPG */
	0,	/* 153=IFEQ */
	0,	/* 154=IFNE */
	0,	/* 155=IFLT */
	0,	/* 156=IFGE */
	0,	/* 157=IFGT */
	0,	/* 158=IFLE */
	0,	/* 159=IF_ICMPEQ */
	0,	/* 160=IF_ICMPNE */
	0,	/* 161=IF_ICMPLT */
	0,	/* 162=IF_ICMPGE */
	0,	/* 163=IF_ICMPGT */
	0,	/* 164=IF_ICMPLE */
	0,	/* 165=IF_ACMPEQ */
	0,	/* 166=IF_ACMPNE */
	0,	/* 167=GOTO */
	0,	/* 168=JSR */
	0,	/* 169=RET */
	0,	/* 170=TABLESWITCH */
	0,	/* 171=LOOKUPSWITCH */
	0,	/* 172=IRETURN */
	0,	/* 173=LRETURN */
	0,	/* 174=FRETURN */
	0,	/* 175=DRETURN */
	0,	/* 176=ARETURN */
	0,	/* 177=RETURN */
	0,	/* 178=GETSTATIC */
	0,	/* 179=PUTSTATIC */
	0,	/* 180=GETFIELD */
	0,	/* 181=PUTFIELD */
	0,	/* 182=INVOKEVIRTUAL */
	0,	/* 183=INVOKESPECIAL */
	0,	/* 184=INVOKESTATIC */
	0,	/* 185=INVOKEINTERFACE */
	0,	/* 186=UNDEF186 */
	0,	/* 187=NEW */
	0,	/* 188=NEWARRAY */
	0,	/* 189=ANEWARRAY */
	0,	/* 190=ARRAYLENGTH */
	0,	/* 191=ATHROW */
	0,	/* 192=CHECKCAST */
	0,	/* 193=INSTANCEOF */
	0,	/* 194=MONITORENTER */
	0,	/* 195=MONITOREXIT */
	0,	/* 196=UNDEF196 */
	0,	/* 197=MULTIANEWARRAY */
	0,	/* 198=IFNULL */
	0,	/* 199=IFNONNULL */
	0,	/* 200=UNDEF200 */
	0,	/* 201=UNDEF201 */
	0,	/* 202=BREAKPOINT */
	0,	/* 203=UNDEF203 */
	0,	/* 204=IASTORECONST */
	0,	/* 205=LASTORECONST */
	0,	/* 206=FASTORECONST */
	0,	/* 207=DASTORECONST */
	0,	/* 208=AASTORECONST */
	0,	/* 209=BASTORECONST */
	0,	/* 210=CASTORECONST */
	0,	/* 211=SASTORECONST */
	0,	/* 212=PUTSTATICCONST */
	0,	/* 213=PUTFIELDCONST */
	0,	/* 214=IMULPOW2 */
	0,	/* 215=LMULPOW2 */
	0,	/* 216=IF_FCMPEQ */
	0,	/* 217=IF_FCMPNE */
	0,	/* 218=IF_FCMPL_LT */
	0,	/* 219=IF_FCMPL_GE */
	0,	/* 220=IF_FCMPL_GT */
	0,	/* 221=IF_FCMPL_LE */
	0,	/* 222=IF_FCMPG_LT */
	0,	/* 223=IF_FCMPG_GE */
	0,	/* 224=IF_FCMPG_GT */
	0,	/* 225=IF_FCMPG_LE */
	0,	/* 226=IF_DCMPEQ */
	0,	/* 227=IF_DCMPNE */
	0,	/* 228=IF_DCMPL_LT */
	0,	/* 229=IF_DCMPL_GE */
	0,	/* 230=IF_DCMPL_GT */
	0,	/* 231=IF_DCMPL_LE */
	0,	/* 232=IF_DCMPG_LT */
	0,	/* 233=IF_DCMPG_GE */
	0,	/* 234=IF_DCMPG_GT */
	0,	/* 235=IF_DCMPG_LE */
	0,	/* 236=UNDEF236 */
	0,	/* 237=UNDEF237 */
	0,	/* 238=UNDEF238 */
	0,	/* 239=UNDEF239 */
	0,	/* 240=UNDEF240 */
	0,	/* 241=UNDEF241 */
	0,	/* 242=UNDEF242 */
	0,	/* 243=UNDEF243 */
	0,	/* 244=UNDEF244 */
	0,	/* 245=UNDEF245 */
	0,	/* 246=UNDEF246 */
	0,	/* 247=UNDEF247 */
	0,	/* 248=UNDEF248 */
	0,	/* 249=GETEXCEPTION */
	0,	/* 250=PHI */
	0,	/* 251=INLINE_START */
	0,	/* 252=INLINE_END */
	0,	/* 253=INLINE_BODY */
	0,	/* 254=UNDEF254 */
	0,	/* 255=BUILTIN */
};

char *burm_opname[] = {
	/* 0 */	"NOP",
	/* 1 */	"ACONST",
	/* 2 */	"CHECKNULL",
	/* 3 */	"ICONST",
	/* 4 */	"UNDEF4",
	/* 5 */	"IDIVPOW2",
	/* 6 */	"LDIVPOW2",
	/* 7 */	"UNDEF7",
	/* 8 */	"UNDEF8",
	/* 9 */	"LCONST",
	/* 10 */	"LCMPCONST",
	/* 11 */	"FCONST",
	/* 12 */	"UNDEF12",
	/* 13 */	"UNDEF13",
	/* 14 */	"DCONST",
	/* 15 */	"COPY",
	/* 16 */	"MOVE",
	/* 17 */	"UNDEF17",
	/* 18 */	"UNDEF18",
	/* 19 */	"UNDEF19",
	/* 20 */	"UNDEF20",
	/* 21 */	"ILOAD",
	/* 22 */	"LLOAD",
	/* 23 */	"FLOAD",
	/* 24 */	"DLOAD",
	/* 25 */	"ALOAD",
	/* 26 */	"IADDCONST",
	/* 27 */	"ISUBCONST",
	/* 28 */	"IMULCONST",
	/* 29 */	"IANDCONST",
	/* 30 */	"IORCONST",
	/* 31 */	"IXORCONST",
	/* 32 */	"ISHLCONST",
	/* 33 */	"ISHRCONST",
	/* 34 */	"IUSHRCONST",
	/* 35 */	"IREMPOW2",
	/* 36 */	"LADDCONST",
	/* 37 */	"LSUBCONST",
	/* 38 */	"LMULCONST",
	/* 39 */	"LANDCONST",
	/* 40 */	"LORCONST",
	/* 41 */	"LXORCONST",
	/* 42 */	"LSHLCONST",
	/* 43 */	"LSHRCONST",
	/* 44 */	"LUSHRCONST",
	/* 45 */	"LREMPOW2",
	/* 46 */	"IALOAD",
	/* 47 */	"LALOAD",
	/* 48 */	"FALOAD",
	/* 49 */	"DALOAD",
	/* 50 */	"AALOAD",
	/* 51 */	"BALOAD",
	/* 52 */	"CALOAD",
	/* 53 */	"SALOAD",
	/* 54 */	"ISTORE",
	/* 55 */	"LSTORE",
	/* 56 */	"FSTORE",
	/* 57 */	"DSTORE",
	/* 58 */	"ASTORE",
	/* 59 */	"IF_LEQ",
	/* 60 */	"IF_LNE",
	/* 61 */	"IF_LLT",
	/* 62 */	"IF_LGE",
	/* 63 */	"IF_LGT",
	/* 64 */	"IF_LLE",
	/* 65 */	"IF_LCMPEQ",
	/* 66 */	"IF_LCMPNE",
	/* 67 */	"IF_LCMPLT",
	/* 68 */	"IF_LCMPGE",
	/* 69 */	"IF_LCMPGT",
	/* 70 */	"IF_LCMPLE",
	/* 71 */	"UNDEF71",
	/* 72 */	"UNDEF72",
	/* 73 */	"UNDEF73",
	/* 74 */	"UNDEF74",
	/* 75 */	"UNDEF75",
	/* 76 */	"UNDEF76",
	/* 77 */	"UNDEF77",
	/* 78 */	"UNDEF78",
	/* 79 */	"IASTORE",
	/* 80 */	"LASTORE",
	/* 81 */	"FASTORE",
	/* 82 */	"DASTORE",
	/* 83 */	"AASTORE",
	/* 84 */	"BASTORE",
	/* 85 */	"CASTORE",
	/* 86 */	"SASTORE",
	/* 87 */	"POP",
	/* 88 */	"POP2",
	/* 89 */	"DUP",
	/* 90 */	"DUP_X1",
	/* 91 */	"DUP_X2",
	/* 92 */	"DUP2",
	/* 93 */	"DUP2_X1",
	/* 94 */	"DUP2_X2",
	/* 95 */	"SWAP",
	/* 96 */	"IADD",
	/* 97 */	"LADD",
	/* 98 */	"FADD",
	/* 99 */	"DADD",
	/* 100 */	"ISUB",
	/* 101 */	"LSUB",
	/* 102 */	"FSUB",
	/* 103 */	"DSUB",
	/* 104 */	"IMUL",
	/* 105 */	"LMUL",
	/* 106 */	"FMUL",
	/* 107 */	"DMUL",
	/* 108 */	"IDIV",
	/* 109 */	"LDIV",
	/* 110 */	"FDIV",
	/* 111 */	"DDIV",
	/* 112 */	"IREM",
	/* 113 */	"LREM",
	/* 114 */	"FREM",
	/* 115 */	"DREM",
	/* 116 */	"INEG",
	/* 117 */	"LNEG",
	/* 118 */	"FNEG",
	/* 119 */	"DNEG",
	/* 120 */	"ISHL",
	/* 121 */	"LSHL",
	/* 122 */	"ISHR",
	/* 123 */	"LSHR",
	/* 124 */	"IUSHR",
	/* 125 */	"LUSHR",
	/* 126 */	"IAND",
	/* 127 */	"LAND",
	/* 128 */	"IOR",
	/* 129 */	"LOR",
	/* 130 */	"IXOR",
	/* 131 */	"LXOR",
	/* 132 */	"IINC",
	/* 133 */	"I2L",
	/* 134 */	"I2F",
	/* 135 */	"I2D",
	/* 136 */	"L2I",
	/* 137 */	"L2F",
	/* 138 */	"L2D",
	/* 139 */	"F2I",
	/* 140 */	"F2L",
	/* 141 */	"F2D",
	/* 142 */	"D2I",
	/* 143 */	"D2L",
	/* 144 */	"D2F",
	/* 145 */	"INT2BYTE",
	/* 146 */	"INT2CHAR",
	/* 147 */	"INT2SHORT",
	/* 148 */	"LCMP",
	/* 149 */	"FCMPL",
	/* 150 */	"FCMPG",
	/* 151 */	"DCMPL",
	/* 152 */	"DCMPG",
	/* 153 */	"IFEQ",
	/* 154 */	"IFNE",
	/* 155 */	"IFLT",
	/* 156 */	"IFGE",
	/* 157 */	"IFGT",
	/* 158 */	"IFLE",
	/* 159 */	"IF_ICMPEQ",
	/* 160 */	"IF_ICMPNE",
	/* 161 */	"IF_ICMPLT",
	/* 162 */	"IF_ICMPGE",
	/* 163 */	"IF_ICMPGT",
	/* 164 */	"IF_ICMPLE",
	/* 165 */	"IF_ACMPEQ",
	/* 166 */	"IF_ACMPNE",
	/* 167 */	"GOTO",
	/* 168 */	"JSR",
	/* 169 */	"RET",
	/* 170 */	"TABLESWITCH",
	/* 171 */	"LOOKUPSWITCH",
	/* 172 */	"IRETURN",
	/* 173 */	"LRETURN",
	/* 174 */	"FRETURN",
	/* 175 */	"DRETURN",
	/* 176 */	"ARETURN",
	/* 177 */	"RETURN",
	/* 178 */	"GETSTATIC",
	/* 179 */	"PUTSTATIC",
	/* 180 */	"GETFIELD",
	/* 181 */	"PUTFIELD",
	/* 182 */	"INVOKEVIRTUAL",
	/* 183 */	"INVOKESPECIAL",
	/* 184 */	"INVOKESTATIC",
	/* 185 */	"INVOKEINTERFACE",
	/* 186 */	"UNDEF186",
	/* 187 */	"NEW",
	/* 188 */	"NEWARRAY",
	/* 189 */	"ANEWARRAY",
	/* 190 */	"ARRAYLENGTH",
	/* 191 */	"ATHROW",
	/* 192 */	"CHECKCAST",
	/* 193 */	"INSTANCEOF",
	/* 194 */	"MONITORENTER",
	/* 195 */	"MONITOREXIT",
	/* 196 */	"UNDEF196",
	/* 197 */	"MULTIANEWARRAY",
	/* 198 */	"IFNULL",
	/* 199 */	"IFNONNULL",
	/* 200 */	"UNDEF200",
	/* 201 */	"UNDEF201",
	/* 202 */	"BREAKPOINT",
	/* 203 */	"UNDEF203",
	/* 204 */	"IASTORECONST",
	/* 205 */	"LASTORECONST",
	/* 206 */	"FASTORECONST",
	/* 207 */	"DASTORECONST",
	/* 208 */	"AASTORECONST",
	/* 209 */	"BASTORECONST",
	/* 210 */	"CASTORECONST",
	/* 211 */	"SASTORECONST",
	/* 212 */	"PUTSTATICCONST",
	/* 213 */	"PUTFIELDCONST",
	/* 214 */	"IMULPOW2",
	/* 215 */	"LMULPOW2",
	/* 216 */	"IF_FCMPEQ",
	/* 217 */	"IF_FCMPNE",
	/* 218 */	"IF_FCMPL_LT",
	/* 219 */	"IF_FCMPL_GE",
	/* 220 */	"IF_FCMPL_GT",
	/* 221 */	"IF_FCMPL_LE",
	/* 222 */	"IF_FCMPG_LT",
	/* 223 */	"IF_FCMPG_GE",
	/* 224 */	"IF_FCMPG_GT",
	/* 225 */	"IF_FCMPG_LE",
	/* 226 */	"IF_DCMPEQ",
	/* 227 */	"IF_DCMPNE",
	/* 228 */	"IF_DCMPL_LT",
	/* 229 */	"IF_DCMPL_GE",
	/* 230 */	"IF_DCMPL_GT",
	/* 231 */	"IF_DCMPL_LE",
	/* 232 */	"IF_DCMPG_LT",
	/* 233 */	"IF_DCMPG_GE",
	/* 234 */	"IF_DCMPG_GT",
	/* 235 */	"IF_DCMPG_LE",
	/* 236 */	"UNDEF236",
	/* 237 */	"UNDEF237",
	/* 238 */	"UNDEF238",
	/* 239 */	"UNDEF239",
	/* 240 */	"UNDEF240",
	/* 241 */	"UNDEF241",
	/* 242 */	"UNDEF242",
	/* 243 */	"UNDEF243",
	/* 244 */	"UNDEF244",
	/* 245 */	"UNDEF245",
	/* 246 */	"UNDEF246",
	/* 247 */	"UNDEF247",
	/* 248 */	"UNDEF248",
	/* 249 */	"GETEXCEPTION",
	/* 250 */	"PHI",
	/* 251 */	"INLINE_START",
	/* 252 */	"INLINE_END",
	/* 253 */	"INLINE_BODY",
	/* 254 */	"UNDEF254",
	/* 255 */	"BUILTIN",
};

short burm_cost[][4] = {
	{ 0 },	/* 0 */
	{ 100 },	/* 1 = reg: NOP */
	{ 100 },	/* 2 = reg: ACONST */
	{ 100 },	/* 3 = reg: CHECKNULL */
	{ 100 },	/* 4 = reg: ICONST */
	{ 100 },	/* 5 = reg: UNDEF4 */
	{ 100 },	/* 6 = reg: IDIVPOW2 */
	{ 100 },	/* 7 = reg: LDIVPOW2 */
	{ 100 },	/* 8 = reg: UNDEF7 */
	{ 100 },	/* 9 = reg: UNDEF8 */
	{ 100 },	/* 10 = reg: LCONST */
	{ 100 },	/* 11 = reg: LCMPCONST */
	{ 100 },	/* 12 = reg: FCONST */
	{ 100 },	/* 13 = reg: UNDEF12 */
	{ 100 },	/* 14 = reg: UNDEF13 */
	{ 100 },	/* 15 = reg: DCONST */
	{ 100 },	/* 16 = reg: COPY */
	{ 100 },	/* 17 = reg: MOVE */
	{ 100 },	/* 18 = reg: UNDEF17 */
	{ 100 },	/* 19 = reg: UNDEF18 */
	{ 100 },	/* 20 = reg: UNDEF19 */
	{ 100 },	/* 21 = reg: UNDEF20 */
	{ 100 },	/* 22 = reg: ILOAD */
	{ 100 },	/* 23 = reg: LLOAD */
	{ 100 },	/* 24 = reg: FLOAD */
	{ 100 },	/* 25 = reg: DLOAD */
	{ 100 },	/* 26 = reg: ALOAD */
	{ 100 },	/* 27 = reg: IADDCONST */
	{ 100 },	/* 28 = reg: ISUBCONST */
	{ 100 },	/* 29 = reg: IMULCONST */
	{ 100 },	/* 30 = reg: IANDCONST */
	{ 100 },	/* 31 = reg: IORCONST */
	{ 100 },	/* 32 = reg: IXORCONST */
	{ 100 },	/* 33 = reg: ISHLCONST */
	{ 100 },	/* 34 = reg: ISHRCONST */
	{ 100 },	/* 35 = reg: IUSHRCONST */
	{ 100 },	/* 36 = reg: IREMPOW2 */
	{ 100 },	/* 37 = reg: LADDCONST */
	{ 100 },	/* 38 = reg: LSUBCONST */
	{ 100 },	/* 39 = reg: LMULCONST */
	{ 100 },	/* 40 = reg: LANDCONST */
	{ 100 },	/* 41 = reg: LORCONST */
	{ 100 },	/* 42 = reg: LXORCONST */
	{ 100 },	/* 43 = reg: LSHLCONST */
	{ 100 },	/* 44 = reg: LSHRCONST */
	{ 100 },	/* 45 = reg: LUSHRCONST */
	{ 100 },	/* 46 = reg: LREMPOW2 */
	{ 100 },	/* 47 = reg: IALOAD */
	{ 100 },	/* 48 = reg: LALOAD */
	{ 100 },	/* 49 = reg: FALOAD */
	{ 100 },	/* 50 = reg: DALOAD */
	{ 100 },	/* 51 = reg: AALOAD */
	{ 100 },	/* 52 = reg: BALOAD */
	{ 100 },	/* 53 = reg: CALOAD */
	{ 100 },	/* 54 = reg: SALOAD */
	{ 100 },	/* 55 = reg: ISTORE */
	{ 100 },	/* 56 = reg: LSTORE */
	{ 100 },	/* 57 = reg: FSTORE */
	{ 100 },	/* 58 = reg: DSTORE */
	{ 100 },	/* 59 = reg: ASTORE */
	{ 100 },	/* 60 = reg: IF_LEQ */
	{ 100 },	/* 61 = reg: IF_LNE */
	{ 100 },	/* 62 = reg: IF_LLT */
	{ 100 },	/* 63 = reg: IF_LGE */
	{ 100 },	/* 64 = reg: IF_LGT */
	{ 100 },	/* 65 = reg: IF_LLE */
	{ 100 },	/* 66 = reg: IF_LCMPEQ */
	{ 100 },	/* 67 = reg: IF_LCMPNE */
	{ 100 },	/* 68 = reg: IF_LCMPLT */
	{ 100 },	/* 69 = reg: IF_LCMPGE */
	{ 100 },	/* 70 = reg: IF_LCMPGT */
	{ 100 },	/* 71 = reg: IF_LCMPLE */
	{ 100 },	/* 72 = reg: UNDEF71 */
	{ 100 },	/* 73 = reg: UNDEF72 */
	{ 100 },	/* 74 = reg: UNDEF73 */
	{ 100 },	/* 75 = reg: UNDEF74 */
	{ 100 },	/* 76 = reg: UNDEF75 */
	{ 100 },	/* 77 = reg: UNDEF76 */
	{ 100 },	/* 78 = reg: UNDEF77 */
	{ 100 },	/* 79 = reg: UNDEF78 */
	{ 100 },	/* 80 = reg: IASTORE */
	{ 100 },	/* 81 = reg: LASTORE */
	{ 100 },	/* 82 = reg: FASTORE */
	{ 100 },	/* 83 = reg: DASTORE */
	{ 100 },	/* 84 = reg: AASTORE */
	{ 100 },	/* 85 = reg: BASTORE */
	{ 100 },	/* 86 = reg: CASTORE */
	{ 100 },	/* 87 = reg: SASTORE */
	{ 100 },	/* 88 = reg: POP */
	{ 100 },	/* 89 = reg: POP2 */
	{ 100 },	/* 90 = reg: DUP */
	{ 100 },	/* 91 = reg: DUP_X1 */
	{ 100 },	/* 92 = reg: DUP_X2 */
	{ 100 },	/* 93 = reg: DUP2 */
	{ 100 },	/* 94 = reg: DUP2_X1 */
	{ 100 },	/* 95 = reg: DUP2_X2 */
	{ 100 },	/* 96 = reg: SWAP */
	{ 100 },	/* 97 = reg: IADD */
	{ 100 },	/* 98 = reg: LADD */
	{ 100 },	/* 99 = reg: FADD */
	{ 100 },	/* 100 = reg: DADD */
	{ 100 },	/* 101 = reg: ISUB */
	{ 100 },	/* 102 = reg: LSUB */
	{ 100 },	/* 103 = reg: FSUB */
	{ 100 },	/* 104 = reg: DSUB */
	{ 100 },	/* 105 = reg: IMUL */
	{ 100 },	/* 106 = reg: LMUL */
	{ 100 },	/* 107 = reg: FMUL */
	{ 100 },	/* 108 = reg: DMUL */
	{ 100 },	/* 109 = reg: IDIV */
	{ 100 },	/* 110 = reg: LDIV */
	{ 100 },	/* 111 = reg: FDIV */
	{ 100 },	/* 112 = reg: DDIV */
	{ 100 },	/* 113 = reg: IREM */
	{ 100 },	/* 114 = reg: LREM */
	{ 100 },	/* 115 = reg: FREM */
	{ 100 },	/* 116 = reg: DREM */
	{ 100 },	/* 117 = reg: INEG */
	{ 100 },	/* 118 = reg: LNEG */
	{ 100 },	/* 119 = reg: FNEG */
	{ 100 },	/* 120 = reg: DNEG */
	{ 100 },	/* 121 = reg: ISHL */
	{ 100 },	/* 122 = reg: LSHL */
	{ 100 },	/* 123 = reg: ISHR */
	{ 100 },	/* 124 = reg: LSHR */
	{ 100 },	/* 125 = reg: IUSHR */
	{ 100 },	/* 126 = reg: LUSHR */
	{ 100 },	/* 127 = reg: IAND */
	{ 100 },	/* 128 = reg: LAND */
	{ 100 },	/* 129 = reg: IOR */
	{ 100 },	/* 130 = reg: LOR */
	{ 100 },	/* 131 = reg: IXOR */
	{ 100 },	/* 132 = reg: LXOR */
	{ 100 },	/* 133 = reg: IINC */
	{ 100 },	/* 134 = reg: I2L */
	{ 100 },	/* 135 = reg: I2F */
	{ 100 },	/* 136 = reg: I2D */
	{ 100 },	/* 137 = reg: L2I */
	{ 100 },	/* 138 = reg: L2F */
	{ 100 },	/* 139 = reg: L2D */
	{ 100 },	/* 140 = reg: F2I */
	{ 100 },	/* 141 = reg: F2L */
	{ 100 },	/* 142 = reg: F2D */
	{ 100 },	/* 143 = reg: D2I */
	{ 100 },	/* 144 = reg: D2L */
	{ 100 },	/* 145 = reg: D2F */
	{ 100 },	/* 146 = reg: INT2BYTE */
	{ 100 },	/* 147 = reg: INT2CHAR */
	{ 100 },	/* 148 = reg: INT2SHORT */
	{ 100 },	/* 149 = reg: LCMP */
	{ 100 },	/* 150 = reg: FCMPL */
	{ 100 },	/* 151 = reg: FCMPG */
	{ 100 },	/* 152 = reg: DCMPL */
	{ 100 },	/* 153 = reg: DCMPG */
	{ 100 },	/* 154 = reg: IFEQ */
	{ 100 },	/* 155 = reg: IFNE */
	{ 100 },	/* 156 = reg: IFLT */
	{ 100 },	/* 157 = reg: IFGE */
	{ 100 },	/* 158 = reg: IFGT */
	{ 100 },	/* 159 = reg: IFLE */
	{ 100 },	/* 160 = reg: IF_ICMPEQ */
	{ 100 },	/* 161 = reg: IF_ICMPNE */
	{ 100 },	/* 162 = reg: IF_ICMPLT */
	{ 100 },	/* 163 = reg: IF_ICMPGE */
	{ 100 },	/* 164 = reg: IF_ICMPGT */
	{ 100 },	/* 165 = reg: IF_ICMPLE */
	{ 100 },	/* 166 = reg: IF_ACMPEQ */
	{ 100 },	/* 167 = reg: IF_ACMPNE */
	{ 100 },	/* 168 = reg: GOTO */
	{ 100 },	/* 169 = reg: JSR */
	{ 100 },	/* 170 = reg: RET */
	{ 100 },	/* 171 = reg: TABLESWITCH */
	{ 100 },	/* 172 = reg: LOOKUPSWITCH */
	{ 100 },	/* 173 = reg: IRETURN */
	{ 100 },	/* 174 = reg: LRETURN */
	{ 100 },	/* 175 = reg: FRETURN */
	{ 100 },	/* 176 = reg: DRETURN */
	{ 100 },	/* 177 = reg: ARETURN */
	{ 100 },	/* 178 = reg: RETURN */
	{ 100 },	/* 179 = reg: GETSTATIC */
	{ 100 },	/* 180 = reg: PUTSTATIC */
	{ 100 },	/* 181 = reg: GETFIELD */
	{ 100 },	/* 182 = reg: PUTFIELD */
	{ 100 },	/* 183 = reg: INVOKEVIRTUAL */
	{ 100 },	/* 184 = reg: INVOKESPECIAL */
	{ 100 },	/* 185 = reg: INVOKESTATIC */
	{ 100 },	/* 186 = reg: INVOKEINTERFACE */
	{ 100 },	/* 187 = reg: UNDEF186 */
	{ 100 },	/* 188 = reg: NEW */
	{ 100 },	/* 189 = reg: NEWARRAY */
	{ 100 },	/* 190 = reg: ANEWARRAY */
	{ 100 },	/* 191 = reg: ARRAYLENGTH */
	{ 100 },	/* 192 = reg: ATHROW */
	{ 100 },	/* 193 = reg: CHECKCAST */
	{ 100 },	/* 194 = reg: INSTANCEOF */
	{ 100 },	/* 195 = reg: MONITORENTER */
	{ 100 },	/* 196 = reg: MONITOREXIT */
	{ 100 },	/* 197 = reg: UNDEF196 */
	{ 100 },	/* 198 = reg: MULTIANEWARRAY */
	{ 100 },	/* 199 = reg: IFNULL */
	{ 100 },	/* 200 = reg: IFNONNULL */
	{ 100 },	/* 201 = reg: UNDEF200 */
	{ 100 },	/* 202 = reg: UNDEF201 */
	{ 100 },	/* 203 = reg: BREAKPOINT */
	{ 100 },	/* 204 = reg: UNDEF203 */
	{ 100 },	/* 205 = reg: IASTORECONST */
	{ 100 },	/* 206 = reg: LASTORECONST */
	{ 100 },	/* 207 = reg: FASTORECONST */
	{ 100 },	/* 208 = reg: DASTORECONST */
	{ 100 },	/* 209 = reg: AASTORECONST */
	{ 100 },	/* 210 = reg: BASTORECONST */
	{ 100 },	/* 211 = reg: CASTORECONST */
	{ 100 },	/* 212 = reg: SASTORECONST */
	{ 100 },	/* 213 = reg: PUTSTATICCONST */
	{ 100 },	/* 214 = reg: PUTFIELDCONST */
	{ 100 },	/* 215 = reg: IMULPOW2 */
	{ 100 },	/* 216 = reg: LMULPOW2 */
	{ 100 },	/* 217 = reg: IF_FCMPEQ */
	{ 100 },	/* 218 = reg: IF_FCMPNE */
	{ 100 },	/* 219 = reg: IF_FCMPL_LT */
	{ 100 },	/* 220 = reg: IF_FCMPL_GE */
	{ 100 },	/* 221 = reg: IF_FCMPL_GT */
	{ 100 },	/* 222 = reg: IF_FCMPL_LE */
	{ 100 },	/* 223 = reg: IF_FCMPG_LT */
	{ 100 },	/* 224 = reg: IF_FCMPG_GE */
	{ 100 },	/* 225 = reg: IF_FCMPG_GT */
	{ 100 },	/* 226 = reg: IF_FCMPG_LE */
	{ 100 },	/* 227 = reg: IF_DCMPEQ */
	{ 100 },	/* 228 = reg: IF_DCMPNE */
	{ 100 },	/* 229 = reg: IF_DCMPL_LT */
	{ 100 },	/* 230 = reg: IF_DCMPL_GE */
	{ 100 },	/* 231 = reg: IF_DCMPL_GT */
	{ 100 },	/* 232 = reg: IF_DCMPL_LE */
	{ 100 },	/* 233 = reg: IF_DCMPG_LT */
	{ 100 },	/* 234 = reg: IF_DCMPG_GE */
	{ 100 },	/* 235 = reg: IF_DCMPG_GT */
	{ 100 },	/* 236 = reg: IF_DCMPG_LE */
	{ 100 },	/* 237 = reg: UNDEF236 */
	{ 100 },	/* 238 = reg: UNDEF237 */
	{ 100 },	/* 239 = reg: UNDEF238 */
	{ 100 },	/* 240 = reg: UNDEF239 */
	{ 100 },	/* 241 = reg: UNDEF240 */
	{ 100 },	/* 242 = reg: UNDEF241 */
	{ 100 },	/* 243 = reg: UNDEF242 */
	{ 100 },	/* 244 = reg: UNDEF243 */
	{ 100 },	/* 245 = reg: UNDEF244 */
	{ 100 },	/* 246 = reg: UNDEF245 */
	{ 100 },	/* 247 = reg: UNDEF246 */
	{ 100 },	/* 248 = reg: UNDEF247 */
	{ 100 },	/* 249 = reg: UNDEF248 */
	{ 100 },	/* 250 = reg: GETEXCEPTION */
	{ 100 },	/* 251 = reg: PHI */
	{ 100 },	/* 252 = reg: INLINE_START */
	{ 100 },	/* 253 = reg: INLINE_END */
	{ 100 },	/* 254 = reg: INLINE_BODY */
	{ 100 },	/* 255 = reg: UNDEF254 */
	{ 100 },	/* 256 = reg: BUILTIN */
};

char *burm_string[] = {
	/* 0 */	0,
	/* 1 */	"reg: NOP",
	/* 2 */	"reg: ACONST",
	/* 3 */	"reg: CHECKNULL",
	/* 4 */	"reg: ICONST",
	/* 5 */	"reg: UNDEF4",
	/* 6 */	"reg: IDIVPOW2",
	/* 7 */	"reg: LDIVPOW2",
	/* 8 */	"reg: UNDEF7",
	/* 9 */	"reg: UNDEF8",
	/* 10 */	"reg: LCONST",
	/* 11 */	"reg: LCMPCONST",
	/* 12 */	"reg: FCONST",
	/* 13 */	"reg: UNDEF12",
	/* 14 */	"reg: UNDEF13",
	/* 15 */	"reg: DCONST",
	/* 16 */	"reg: COPY",
	/* 17 */	"reg: MOVE",
	/* 18 */	"reg: UNDEF17",
	/* 19 */	"reg: UNDEF18",
	/* 20 */	"reg: UNDEF19",
	/* 21 */	"reg: UNDEF20",
	/* 22 */	"reg: ILOAD",
	/* 23 */	"reg: LLOAD",
	/* 24 */	"reg: FLOAD",
	/* 25 */	"reg: DLOAD",
	/* 26 */	"reg: ALOAD",
	/* 27 */	"reg: IADDCONST",
	/* 28 */	"reg: ISUBCONST",
	/* 29 */	"reg: IMULCONST",
	/* 30 */	"reg: IANDCONST",
	/* 31 */	"reg: IORCONST",
	/* 32 */	"reg: IXORCONST",
	/* 33 */	"reg: ISHLCONST",
	/* 34 */	"reg: ISHRCONST",
	/* 35 */	"reg: IUSHRCONST",
	/* 36 */	"reg: IREMPOW2",
	/* 37 */	"reg: LADDCONST",
	/* 38 */	"reg: LSUBCONST",
	/* 39 */	"reg: LMULCONST",
	/* 40 */	"reg: LANDCONST",
	/* 41 */	"reg: LORCONST",
	/* 42 */	"reg: LXORCONST",
	/* 43 */	"reg: LSHLCONST",
	/* 44 */	"reg: LSHRCONST",
	/* 45 */	"reg: LUSHRCONST",
	/* 46 */	"reg: LREMPOW2",
	/* 47 */	"reg: IALOAD",
	/* 48 */	"reg: LALOAD",
	/* 49 */	"reg: FALOAD",
	/* 50 */	"reg: DALOAD",
	/* 51 */	"reg: AALOAD",
	/* 52 */	"reg: BALOAD",
	/* 53 */	"reg: CALOAD",
	/* 54 */	"reg: SALOAD",
	/* 55 */	"reg: ISTORE",
	/* 56 */	"reg: LSTORE",
	/* 57 */	"reg: FSTORE",
	/* 58 */	"reg: DSTORE",
	/* 59 */	"reg: ASTORE",
	/* 60 */	"reg: IF_LEQ",
	/* 61 */	"reg: IF_LNE",
	/* 62 */	"reg: IF_LLT",
	/* 63 */	"reg: IF_LGE",
	/* 64 */	"reg: IF_LGT",
	/* 65 */	"reg: IF_LLE",
	/* 66 */	"reg: IF_LCMPEQ",
	/* 67 */	"reg: IF_LCMPNE",
	/* 68 */	"reg: IF_LCMPLT",
	/* 69 */	"reg: IF_LCMPGE",
	/* 70 */	"reg: IF_LCMPGT",
	/* 71 */	"reg: IF_LCMPLE",
	/* 72 */	"reg: UNDEF71",
	/* 73 */	"reg: UNDEF72",
	/* 74 */	"reg: UNDEF73",
	/* 75 */	"reg: UNDEF74",
	/* 76 */	"reg: UNDEF75",
	/* 77 */	"reg: UNDEF76",
	/* 78 */	"reg: UNDEF77",
	/* 79 */	"reg: UNDEF78",
	/* 80 */	"reg: IASTORE",
	/* 81 */	"reg: LASTORE",
	/* 82 */	"reg: FASTORE",
	/* 83 */	"reg: DASTORE",
	/* 84 */	"reg: AASTORE",
	/* 85 */	"reg: BASTORE",
	/* 86 */	"reg: CASTORE",
	/* 87 */	"reg: SASTORE",
	/* 88 */	"reg: POP",
	/* 89 */	"reg: POP2",
	/* 90 */	"reg: DUP",
	/* 91 */	"reg: DUP_X1",
	/* 92 */	"reg: DUP_X2",
	/* 93 */	"reg: DUP2",
	/* 94 */	"reg: DUP2_X1",
	/* 95 */	"reg: DUP2_X2",
	/* 96 */	"reg: SWAP",
	/* 97 */	"reg: IADD",
	/* 98 */	"reg: LADD",
	/* 99 */	"reg: FADD",
	/* 100 */	"reg: DADD",
	/* 101 */	"reg: ISUB",
	/* 102 */	"reg: LSUB",
	/* 103 */	"reg: FSUB",
	/* 104 */	"reg: DSUB",
	/* 105 */	"reg: IMUL",
	/* 106 */	"reg: LMUL",
	/* 107 */	"reg: FMUL",
	/* 108 */	"reg: DMUL",
	/* 109 */	"reg: IDIV",
	/* 110 */	"reg: LDIV",
	/* 111 */	"reg: FDIV",
	/* 112 */	"reg: DDIV",
	/* 113 */	"reg: IREM",
	/* 114 */	"reg: LREM",
	/* 115 */	"reg: FREM",
	/* 116 */	"reg: DREM",
	/* 117 */	"reg: INEG",
	/* 118 */	"reg: LNEG",
	/* 119 */	"reg: FNEG",
	/* 120 */	"reg: DNEG",
	/* 121 */	"reg: ISHL",
	/* 122 */	"reg: LSHL",
	/* 123 */	"reg: ISHR",
	/* 124 */	"reg: LSHR",
	/* 125 */	"reg: IUSHR",
	/* 126 */	"reg: LUSHR",
	/* 127 */	"reg: IAND",
	/* 128 */	"reg: LAND",
	/* 129 */	"reg: IOR",
	/* 130 */	"reg: LOR",
	/* 131 */	"reg: IXOR",
	/* 132 */	"reg: LXOR",
	/* 133 */	"reg: IINC",
	/* 134 */	"reg: I2L",
	/* 135 */	"reg: I2F",
	/* 136 */	"reg: I2D",
	/* 137 */	"reg: L2I",
	/* 138 */	"reg: L2F",
	/* 139 */	"reg: L2D",
	/* 140 */	"reg: F2I",
	/* 141 */	"reg: F2L",
	/* 142 */	"reg: F2D",
	/* 143 */	"reg: D2I",
	/* 144 */	"reg: D2L",
	/* 145 */	"reg: D2F",
	/* 146 */	"reg: INT2BYTE",
	/* 147 */	"reg: INT2CHAR",
	/* 148 */	"reg: INT2SHORT",
	/* 149 */	"reg: LCMP",
	/* 150 */	"reg: FCMPL",
	/* 151 */	"reg: FCMPG",
	/* 152 */	"reg: DCMPL",
	/* 153 */	"reg: DCMPG",
	/* 154 */	"reg: IFEQ",
	/* 155 */	"reg: IFNE",
	/* 156 */	"reg: IFLT",
	/* 157 */	"reg: IFGE",
	/* 158 */	"reg: IFGT",
	/* 159 */	"reg: IFLE",
	/* 160 */	"reg: IF_ICMPEQ",
	/* 161 */	"reg: IF_ICMPNE",
	/* 162 */	"reg: IF_ICMPLT",
	/* 163 */	"reg: IF_ICMPGE",
	/* 164 */	"reg: IF_ICMPGT",
	/* 165 */	"reg: IF_ICMPLE",
	/* 166 */	"reg: IF_ACMPEQ",
	/* 167 */	"reg: IF_ACMPNE",
	/* 168 */	"reg: GOTO",
	/* 169 */	"reg: JSR",
	/* 170 */	"reg: RET",
	/* 171 */	"reg: TABLESWITCH",
	/* 172 */	"reg: LOOKUPSWITCH",
	/* 173 */	"reg: IRETURN",
	/* 174 */	"reg: LRETURN",
	/* 175 */	"reg: FRETURN",
	/* 176 */	"reg: DRETURN",
	/* 177 */	"reg: ARETURN",
	/* 178 */	"reg: RETURN",
	/* 179 */	"reg: GETSTATIC",
	/* 180 */	"reg: PUTSTATIC",
	/* 181 */	"reg: GETFIELD",
	/* 182 */	"reg: PUTFIELD",
	/* 183 */	"reg: INVOKEVIRTUAL",
	/* 184 */	"reg: INVOKESPECIAL",
	/* 185 */	"reg: INVOKESTATIC",
	/* 186 */	"reg: INVOKEINTERFACE",
	/* 187 */	"reg: UNDEF186",
	/* 188 */	"reg: NEW",
	/* 189 */	"reg: NEWARRAY",
	/* 190 */	"reg: ANEWARRAY",
	/* 191 */	"reg: ARRAYLENGTH",
	/* 192 */	"reg: ATHROW",
	/* 193 */	"reg: CHECKCAST",
	/* 194 */	"reg: INSTANCEOF",
	/* 195 */	"reg: MONITORENTER",
	/* 196 */	"reg: MONITOREXIT",
	/* 197 */	"reg: UNDEF196",
	/* 198 */	"reg: MULTIANEWARRAY",
	/* 199 */	"reg: IFNULL",
	/* 200 */	"reg: IFNONNULL",
	/* 201 */	"reg: UNDEF200",
	/* 202 */	"reg: UNDEF201",
	/* 203 */	"reg: BREAKPOINT",
	/* 204 */	"reg: UNDEF203",
	/* 205 */	"reg: IASTORECONST",
	/* 206 */	"reg: LASTORECONST",
	/* 207 */	"reg: FASTORECONST",
	/* 208 */	"reg: DASTORECONST",
	/* 209 */	"reg: AASTORECONST",
	/* 210 */	"reg: BASTORECONST",
	/* 211 */	"reg: CASTORECONST",
	/* 212 */	"reg: SASTORECONST",
	/* 213 */	"reg: PUTSTATICCONST",
	/* 214 */	"reg: PUTFIELDCONST",
	/* 215 */	"reg: IMULPOW2",
	/* 216 */	"reg: LMULPOW2",
	/* 217 */	"reg: IF_FCMPEQ",
	/* 218 */	"reg: IF_FCMPNE",
	/* 219 */	"reg: IF_FCMPL_LT",
	/* 220 */	"reg: IF_FCMPL_GE",
	/* 221 */	"reg: IF_FCMPL_GT",
	/* 222 */	"reg: IF_FCMPL_LE",
	/* 223 */	"reg: IF_FCMPG_LT",
	/* 224 */	"reg: IF_FCMPG_GE",
	/* 225 */	"reg: IF_FCMPG_GT",
	/* 226 */	"reg: IF_FCMPG_LE",
	/* 227 */	"reg: IF_DCMPEQ",
	/* 228 */	"reg: IF_DCMPNE",
	/* 229 */	"reg: IF_DCMPL_LT",
	/* 230 */	"reg: IF_DCMPL_GE",
	/* 231 */	"reg: IF_DCMPL_GT",
	/* 232 */	"reg: IF_DCMPL_LE",
	/* 233 */	"reg: IF_DCMPG_LT",
	/* 234 */	"reg: IF_DCMPG_GE",
	/* 235 */	"reg: IF_DCMPG_GT",
	/* 236 */	"reg: IF_DCMPG_LE",
	/* 237 */	"reg: UNDEF236",
	/* 238 */	"reg: UNDEF237",
	/* 239 */	"reg: UNDEF238",
	/* 240 */	"reg: UNDEF239",
	/* 241 */	"reg: UNDEF240",
	/* 242 */	"reg: UNDEF241",
	/* 243 */	"reg: UNDEF242",
	/* 244 */	"reg: UNDEF243",
	/* 245 */	"reg: UNDEF244",
	/* 246 */	"reg: UNDEF245",
	/* 247 */	"reg: UNDEF246",
	/* 248 */	"reg: UNDEF247",
	/* 249 */	"reg: UNDEF248",
	/* 250 */	"reg: GETEXCEPTION",
	/* 251 */	"reg: PHI",
	/* 252 */	"reg: INLINE_START",
	/* 253 */	"reg: INLINE_END",
	/* 254 */	"reg: INLINE_BODY",
	/* 255 */	"reg: UNDEF254",
	/* 256 */	"reg: BUILTIN",
};

static short burm_decode_reg[] = {
	0,
	1,
	2,
	3,
	4,
	5,
	6,
	7,
	8,
	9,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	17,
	18,
	19,
	20,
	21,
	22,
	23,
	24,
	25,
	26,
	27,
	28,
	29,
	30,
	31,
	32,
	33,
	34,
	35,
	36,
	37,
	38,
	39,
	40,
	41,
	42,
	43,
	44,
	45,
	46,
	47,
	48,
	49,
	50,
	51,
	52,
	53,
	54,
	55,
	56,
	57,
	58,
	59,
	60,
	61,
	62,
	63,
	64,
	65,
	66,
	67,
	68,
	69,
	70,
	71,
	72,
	73,
	74,
	75,
	76,
	77,
	78,
	79,
	80,
	81,
	82,
	83,
	84,
	85,
	86,
	87,
	88,
	89,
	90,
	91,
	92,
	93,
	94,
	95,
	96,
	97,
	98,
	99,
	100,
	101,
	102,
	103,
	104,
	105,
	106,
	107,
	108,
	109,
	110,
	111,
	112,
	113,
	114,
	115,
	116,
	117,
	118,
	119,
	120,
	121,
	122,
	123,
	124,
	125,
	126,
	127,
	128,
	129,
	130,
	131,
	132,
	133,
	134,
	135,
	136,
	137,
	138,
	139,
	140,
	141,
	142,
	143,
	144,
	145,
	146,
	147,
	148,
	149,
	150,
	151,
	152,
	153,
	154,
	155,
	156,
	157,
	158,
	159,
	160,
	161,
	162,
	163,
	164,
	165,
	166,
	167,
	168,
	169,
	170,
	171,
	172,
	173,
	174,
	175,
	176,
	177,
	178,
	179,
	180,
	181,
	182,
	183,
	184,
	185,
	186,
	187,
	188,
	189,
	190,
	191,
	192,
	193,
	194,
	195,
	196,
	197,
	198,
	199,
	200,
	201,
	202,
	203,
	204,
	205,
	206,
	207,
	208,
	209,
	210,
	211,
	212,
	213,
	214,
	215,
	216,
	217,
	218,
	219,
	220,
	221,
	222,
	223,
	224,
	225,
	226,
	227,
	228,
	229,
	230,
	231,
	232,
	233,
	234,
	235,
	236,
	237,
	238,
	239,
	240,
	241,
	242,
	243,
	244,
	245,
	246,
	247,
	248,
	249,
	250,
	251,
	252,
	253,
	254,
	255,
	256,
};

int burm_rule(STATEPTR_TYPE state, int goalnt) {
	burm_assert(goalnt >= 1 && goalnt <= 1, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	if (!state)
		return 0;
	switch (goalnt) {
	case burm_reg_NT:
		return burm_decode_reg[state->rule.burm_reg];
	default:
		burm_assert(0, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	}
	return 0;
}


STATEPTR_TYPE burm_state(int op, STATEPTR_TYPE left, STATEPTR_TYPE right) {
	int c;
	STATEPTR_TYPE p, l = left, r = right;

	if (burm_arity[op] > 0) {
		p = (STATEPTR_TYPE)ALLOC(sizeof *p);
		burm_assert(p, PANIC("ALLOC returned NULL in burm_state\n"));
		p->op = op;
		p->left = l;
		p->right = r;
		p->rule.burm_reg = 0;
		p->cost[1] =
			32767;
	}
	switch (op) {
	case 0: /* NOP */
		{
			static struct burm_state z = { 0, 0, 0,
				{	0,
					100,	/* reg: NOP */
				},{
					1,	/* reg: NOP */
				}
			};
			return &z;
		}
	case 1: /* ACONST */
		{
			static struct burm_state z = { 1, 0, 0,
				{	0,
					100,	/* reg: ACONST */
				},{
					2,	/* reg: ACONST */
				}
			};
			return &z;
		}
	case 2: /* CHECKNULL */
		{
			static struct burm_state z = { 2, 0, 0,
				{	0,
					100,	/* reg: CHECKNULL */
				},{
					3,	/* reg: CHECKNULL */
				}
			};
			return &z;
		}
	case 3: /* ICONST */
		{
			static struct burm_state z = { 3, 0, 0,
				{	0,
					100,	/* reg: ICONST */
				},{
					4,	/* reg: ICONST */
				}
			};
			return &z;
		}
	case 4: /* UNDEF4 */
		{
			static struct burm_state z = { 4, 0, 0,
				{	0,
					100,	/* reg: UNDEF4 */
				},{
					5,	/* reg: UNDEF4 */
				}
			};
			return &z;
		}
	case 5: /* IDIVPOW2 */
		{
			static struct burm_state z = { 5, 0, 0,
				{	0,
					100,	/* reg: IDIVPOW2 */
				},{
					6,	/* reg: IDIVPOW2 */
				}
			};
			return &z;
		}
	case 6: /* LDIVPOW2 */
		{
			static struct burm_state z = { 6, 0, 0,
				{	0,
					100,	/* reg: LDIVPOW2 */
				},{
					7,	/* reg: LDIVPOW2 */
				}
			};
			return &z;
		}
	case 7: /* UNDEF7 */
		{
			static struct burm_state z = { 7, 0, 0,
				{	0,
					100,	/* reg: UNDEF7 */
				},{
					8,	/* reg: UNDEF7 */
				}
			};
			return &z;
		}
	case 8: /* UNDEF8 */
		{
			static struct burm_state z = { 8, 0, 0,
				{	0,
					100,	/* reg: UNDEF8 */
				},{
					9,	/* reg: UNDEF8 */
				}
			};
			return &z;
		}
	case 9: /* LCONST */
		{
			static struct burm_state z = { 9, 0, 0,
				{	0,
					100,	/* reg: LCONST */
				},{
					10,	/* reg: LCONST */
				}
			};
			return &z;
		}
	case 10: /* LCMPCONST */
		{
			static struct burm_state z = { 10, 0, 0,
				{	0,
					100,	/* reg: LCMPCONST */
				},{
					11,	/* reg: LCMPCONST */
				}
			};
			return &z;
		}
	case 11: /* FCONST */
		{
			static struct burm_state z = { 11, 0, 0,
				{	0,
					100,	/* reg: FCONST */
				},{
					12,	/* reg: FCONST */
				}
			};
			return &z;
		}
	case 12: /* UNDEF12 */
		{
			static struct burm_state z = { 12, 0, 0,
				{	0,
					100,	/* reg: UNDEF12 */
				},{
					13,	/* reg: UNDEF12 */
				}
			};
			return &z;
		}
	case 13: /* UNDEF13 */
		{
			static struct burm_state z = { 13, 0, 0,
				{	0,
					100,	/* reg: UNDEF13 */
				},{
					14,	/* reg: UNDEF13 */
				}
			};
			return &z;
		}
	case 14: /* DCONST */
		{
			static struct burm_state z = { 14, 0, 0,
				{	0,
					100,	/* reg: DCONST */
				},{
					15,	/* reg: DCONST */
				}
			};
			return &z;
		}
	case 15: /* COPY */
		{
			static struct burm_state z = { 15, 0, 0,
				{	0,
					100,	/* reg: COPY */
				},{
					16,	/* reg: COPY */
				}
			};
			return &z;
		}
	case 16: /* MOVE */
		{
			static struct burm_state z = { 16, 0, 0,
				{	0,
					100,	/* reg: MOVE */
				},{
					17,	/* reg: MOVE */
				}
			};
			return &z;
		}
	case 17: /* UNDEF17 */
		{
			static struct burm_state z = { 17, 0, 0,
				{	0,
					100,	/* reg: UNDEF17 */
				},{
					18,	/* reg: UNDEF17 */
				}
			};
			return &z;
		}
	case 18: /* UNDEF18 */
		{
			static struct burm_state z = { 18, 0, 0,
				{	0,
					100,	/* reg: UNDEF18 */
				},{
					19,	/* reg: UNDEF18 */
				}
			};
			return &z;
		}
	case 19: /* UNDEF19 */
		{
			static struct burm_state z = { 19, 0, 0,
				{	0,
					100,	/* reg: UNDEF19 */
				},{
					20,	/* reg: UNDEF19 */
				}
			};
			return &z;
		}
	case 20: /* UNDEF20 */
		{
			static struct burm_state z = { 20, 0, 0,
				{	0,
					100,	/* reg: UNDEF20 */
				},{
					21,	/* reg: UNDEF20 */
				}
			};
			return &z;
		}
	case 21: /* ILOAD */
		{
			static struct burm_state z = { 21, 0, 0,
				{	0,
					100,	/* reg: ILOAD */
				},{
					22,	/* reg: ILOAD */
				}
			};
			return &z;
		}
	case 22: /* LLOAD */
		{
			static struct burm_state z = { 22, 0, 0,
				{	0,
					100,	/* reg: LLOAD */
				},{
					23,	/* reg: LLOAD */
				}
			};
			return &z;
		}
	case 23: /* FLOAD */
		{
			static struct burm_state z = { 23, 0, 0,
				{	0,
					100,	/* reg: FLOAD */
				},{
					24,	/* reg: FLOAD */
				}
			};
			return &z;
		}
	case 24: /* DLOAD */
		{
			static struct burm_state z = { 24, 0, 0,
				{	0,
					100,	/* reg: DLOAD */
				},{
					25,	/* reg: DLOAD */
				}
			};
			return &z;
		}
	case 25: /* ALOAD */
		{
			static struct burm_state z = { 25, 0, 0,
				{	0,
					100,	/* reg: ALOAD */
				},{
					26,	/* reg: ALOAD */
				}
			};
			return &z;
		}
	case 26: /* IADDCONST */
		{
			static struct burm_state z = { 26, 0, 0,
				{	0,
					100,	/* reg: IADDCONST */
				},{
					27,	/* reg: IADDCONST */
				}
			};
			return &z;
		}
	case 27: /* ISUBCONST */
		{
			static struct burm_state z = { 27, 0, 0,
				{	0,
					100,	/* reg: ISUBCONST */
				},{
					28,	/* reg: ISUBCONST */
				}
			};
			return &z;
		}
	case 28: /* IMULCONST */
		{
			static struct burm_state z = { 28, 0, 0,
				{	0,
					100,	/* reg: IMULCONST */
				},{
					29,	/* reg: IMULCONST */
				}
			};
			return &z;
		}
	case 29: /* IANDCONST */
		{
			static struct burm_state z = { 29, 0, 0,
				{	0,
					100,	/* reg: IANDCONST */
				},{
					30,	/* reg: IANDCONST */
				}
			};
			return &z;
		}
	case 30: /* IORCONST */
		{
			static struct burm_state z = { 30, 0, 0,
				{	0,
					100,	/* reg: IORCONST */
				},{
					31,	/* reg: IORCONST */
				}
			};
			return &z;
		}
	case 31: /* IXORCONST */
		{
			static struct burm_state z = { 31, 0, 0,
				{	0,
					100,	/* reg: IXORCONST */
				},{
					32,	/* reg: IXORCONST */
				}
			};
			return &z;
		}
	case 32: /* ISHLCONST */
		{
			static struct burm_state z = { 32, 0, 0,
				{	0,
					100,	/* reg: ISHLCONST */
				},{
					33,	/* reg: ISHLCONST */
				}
			};
			return &z;
		}
	case 33: /* ISHRCONST */
		{
			static struct burm_state z = { 33, 0, 0,
				{	0,
					100,	/* reg: ISHRCONST */
				},{
					34,	/* reg: ISHRCONST */
				}
			};
			return &z;
		}
	case 34: /* IUSHRCONST */
		{
			static struct burm_state z = { 34, 0, 0,
				{	0,
					100,	/* reg: IUSHRCONST */
				},{
					35,	/* reg: IUSHRCONST */
				}
			};
			return &z;
		}
	case 35: /* IREMPOW2 */
		{
			static struct burm_state z = { 35, 0, 0,
				{	0,
					100,	/* reg: IREMPOW2 */
				},{
					36,	/* reg: IREMPOW2 */
				}
			};
			return &z;
		}
	case 36: /* LADDCONST */
		{
			static struct burm_state z = { 36, 0, 0,
				{	0,
					100,	/* reg: LADDCONST */
				},{
					37,	/* reg: LADDCONST */
				}
			};
			return &z;
		}
	case 37: /* LSUBCONST */
		{
			static struct burm_state z = { 37, 0, 0,
				{	0,
					100,	/* reg: LSUBCONST */
				},{
					38,	/* reg: LSUBCONST */
				}
			};
			return &z;
		}
	case 38: /* LMULCONST */
		{
			static struct burm_state z = { 38, 0, 0,
				{	0,
					100,	/* reg: LMULCONST */
				},{
					39,	/* reg: LMULCONST */
				}
			};
			return &z;
		}
	case 39: /* LANDCONST */
		{
			static struct burm_state z = { 39, 0, 0,
				{	0,
					100,	/* reg: LANDCONST */
				},{
					40,	/* reg: LANDCONST */
				}
			};
			return &z;
		}
	case 40: /* LORCONST */
		{
			static struct burm_state z = { 40, 0, 0,
				{	0,
					100,	/* reg: LORCONST */
				},{
					41,	/* reg: LORCONST */
				}
			};
			return &z;
		}
	case 41: /* LXORCONST */
		{
			static struct burm_state z = { 41, 0, 0,
				{	0,
					100,	/* reg: LXORCONST */
				},{
					42,	/* reg: LXORCONST */
				}
			};
			return &z;
		}
	case 42: /* LSHLCONST */
		{
			static struct burm_state z = { 42, 0, 0,
				{	0,
					100,	/* reg: LSHLCONST */
				},{
					43,	/* reg: LSHLCONST */
				}
			};
			return &z;
		}
	case 43: /* LSHRCONST */
		{
			static struct burm_state z = { 43, 0, 0,
				{	0,
					100,	/* reg: LSHRCONST */
				},{
					44,	/* reg: LSHRCONST */
				}
			};
			return &z;
		}
	case 44: /* LUSHRCONST */
		{
			static struct burm_state z = { 44, 0, 0,
				{	0,
					100,	/* reg: LUSHRCONST */
				},{
					45,	/* reg: LUSHRCONST */
				}
			};
			return &z;
		}
	case 45: /* LREMPOW2 */
		{
			static struct burm_state z = { 45, 0, 0,
				{	0,
					100,	/* reg: LREMPOW2 */
				},{
					46,	/* reg: LREMPOW2 */
				}
			};
			return &z;
		}
	case 46: /* IALOAD */
		{
			static struct burm_state z = { 46, 0, 0,
				{	0,
					100,	/* reg: IALOAD */
				},{
					47,	/* reg: IALOAD */
				}
			};
			return &z;
		}
	case 47: /* LALOAD */
		{
			static struct burm_state z = { 47, 0, 0,
				{	0,
					100,	/* reg: LALOAD */
				},{
					48,	/* reg: LALOAD */
				}
			};
			return &z;
		}
	case 48: /* FALOAD */
		{
			static struct burm_state z = { 48, 0, 0,
				{	0,
					100,	/* reg: FALOAD */
				},{
					49,	/* reg: FALOAD */
				}
			};
			return &z;
		}
	case 49: /* DALOAD */
		{
			static struct burm_state z = { 49, 0, 0,
				{	0,
					100,	/* reg: DALOAD */
				},{
					50,	/* reg: DALOAD */
				}
			};
			return &z;
		}
	case 50: /* AALOAD */
		{
			static struct burm_state z = { 50, 0, 0,
				{	0,
					100,	/* reg: AALOAD */
				},{
					51,	/* reg: AALOAD */
				}
			};
			return &z;
		}
	case 51: /* BALOAD */
		{
			static struct burm_state z = { 51, 0, 0,
				{	0,
					100,	/* reg: BALOAD */
				},{
					52,	/* reg: BALOAD */
				}
			};
			return &z;
		}
	case 52: /* CALOAD */
		{
			static struct burm_state z = { 52, 0, 0,
				{	0,
					100,	/* reg: CALOAD */
				},{
					53,	/* reg: CALOAD */
				}
			};
			return &z;
		}
	case 53: /* SALOAD */
		{
			static struct burm_state z = { 53, 0, 0,
				{	0,
					100,	/* reg: SALOAD */
				},{
					54,	/* reg: SALOAD */
				}
			};
			return &z;
		}
	case 54: /* ISTORE */
		{
			static struct burm_state z = { 54, 0, 0,
				{	0,
					100,	/* reg: ISTORE */
				},{
					55,	/* reg: ISTORE */
				}
			};
			return &z;
		}
	case 55: /* LSTORE */
		{
			static struct burm_state z = { 55, 0, 0,
				{	0,
					100,	/* reg: LSTORE */
				},{
					56,	/* reg: LSTORE */
				}
			};
			return &z;
		}
	case 56: /* FSTORE */
		{
			static struct burm_state z = { 56, 0, 0,
				{	0,
					100,	/* reg: FSTORE */
				},{
					57,	/* reg: FSTORE */
				}
			};
			return &z;
		}
	case 57: /* DSTORE */
		{
			static struct burm_state z = { 57, 0, 0,
				{	0,
					100,	/* reg: DSTORE */
				},{
					58,	/* reg: DSTORE */
				}
			};
			return &z;
		}
	case 58: /* ASTORE */
		{
			static struct burm_state z = { 58, 0, 0,
				{	0,
					100,	/* reg: ASTORE */
				},{
					59,	/* reg: ASTORE */
				}
			};
			return &z;
		}
	case 59: /* IF_LEQ */
		{
			static struct burm_state z = { 59, 0, 0,
				{	0,
					100,	/* reg: IF_LEQ */
				},{
					60,	/* reg: IF_LEQ */
				}
			};
			return &z;
		}
	case 60: /* IF_LNE */
		{
			static struct burm_state z = { 60, 0, 0,
				{	0,
					100,	/* reg: IF_LNE */
				},{
					61,	/* reg: IF_LNE */
				}
			};
			return &z;
		}
	case 61: /* IF_LLT */
		{
			static struct burm_state z = { 61, 0, 0,
				{	0,
					100,	/* reg: IF_LLT */
				},{
					62,	/* reg: IF_LLT */
				}
			};
			return &z;
		}
	case 62: /* IF_LGE */
		{
			static struct burm_state z = { 62, 0, 0,
				{	0,
					100,	/* reg: IF_LGE */
				},{
					63,	/* reg: IF_LGE */
				}
			};
			return &z;
		}
	case 63: /* IF_LGT */
		{
			static struct burm_state z = { 63, 0, 0,
				{	0,
					100,	/* reg: IF_LGT */
				},{
					64,	/* reg: IF_LGT */
				}
			};
			return &z;
		}
	case 64: /* IF_LLE */
		{
			static struct burm_state z = { 64, 0, 0,
				{	0,
					100,	/* reg: IF_LLE */
				},{
					65,	/* reg: IF_LLE */
				}
			};
			return &z;
		}
	case 65: /* IF_LCMPEQ */
		{
			static struct burm_state z = { 65, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPEQ */
				},{
					66,	/* reg: IF_LCMPEQ */
				}
			};
			return &z;
		}
	case 66: /* IF_LCMPNE */
		{
			static struct burm_state z = { 66, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPNE */
				},{
					67,	/* reg: IF_LCMPNE */
				}
			};
			return &z;
		}
	case 67: /* IF_LCMPLT */
		{
			static struct burm_state z = { 67, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPLT */
				},{
					68,	/* reg: IF_LCMPLT */
				}
			};
			return &z;
		}
	case 68: /* IF_LCMPGE */
		{
			static struct burm_state z = { 68, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPGE */
				},{
					69,	/* reg: IF_LCMPGE */
				}
			};
			return &z;
		}
	case 69: /* IF_LCMPGT */
		{
			static struct burm_state z = { 69, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPGT */
				},{
					70,	/* reg: IF_LCMPGT */
				}
			};
			return &z;
		}
	case 70: /* IF_LCMPLE */
		{
			static struct burm_state z = { 70, 0, 0,
				{	0,
					100,	/* reg: IF_LCMPLE */
				},{
					71,	/* reg: IF_LCMPLE */
				}
			};
			return &z;
		}
	case 71: /* UNDEF71 */
		{
			static struct burm_state z = { 71, 0, 0,
				{	0,
					100,	/* reg: UNDEF71 */
				},{
					72,	/* reg: UNDEF71 */
				}
			};
			return &z;
		}
	case 72: /* UNDEF72 */
		{
			static struct burm_state z = { 72, 0, 0,
				{	0,
					100,	/* reg: UNDEF72 */
				},{
					73,	/* reg: UNDEF72 */
				}
			};
			return &z;
		}
	case 73: /* UNDEF73 */
		{
			static struct burm_state z = { 73, 0, 0,
				{	0,
					100,	/* reg: UNDEF73 */
				},{
					74,	/* reg: UNDEF73 */
				}
			};
			return &z;
		}
	case 74: /* UNDEF74 */
		{
			static struct burm_state z = { 74, 0, 0,
				{	0,
					100,	/* reg: UNDEF74 */
				},{
					75,	/* reg: UNDEF74 */
				}
			};
			return &z;
		}
	case 75: /* UNDEF75 */
		{
			static struct burm_state z = { 75, 0, 0,
				{	0,
					100,	/* reg: UNDEF75 */
				},{
					76,	/* reg: UNDEF75 */
				}
			};
			return &z;
		}
	case 76: /* UNDEF76 */
		{
			static struct burm_state z = { 76, 0, 0,
				{	0,
					100,	/* reg: UNDEF76 */
				},{
					77,	/* reg: UNDEF76 */
				}
			};
			return &z;
		}
	case 77: /* UNDEF77 */
		{
			static struct burm_state z = { 77, 0, 0,
				{	0,
					100,	/* reg: UNDEF77 */
				},{
					78,	/* reg: UNDEF77 */
				}
			};
			return &z;
		}
	case 78: /* UNDEF78 */
		{
			static struct burm_state z = { 78, 0, 0,
				{	0,
					100,	/* reg: UNDEF78 */
				},{
					79,	/* reg: UNDEF78 */
				}
			};
			return &z;
		}
	case 79: /* IASTORE */
		{
			static struct burm_state z = { 79, 0, 0,
				{	0,
					100,	/* reg: IASTORE */
				},{
					80,	/* reg: IASTORE */
				}
			};
			return &z;
		}
	case 80: /* LASTORE */
		{
			static struct burm_state z = { 80, 0, 0,
				{	0,
					100,	/* reg: LASTORE */
				},{
					81,	/* reg: LASTORE */
				}
			};
			return &z;
		}
	case 81: /* FASTORE */
		{
			static struct burm_state z = { 81, 0, 0,
				{	0,
					100,	/* reg: FASTORE */
				},{
					82,	/* reg: FASTORE */
				}
			};
			return &z;
		}
	case 82: /* DASTORE */
		{
			static struct burm_state z = { 82, 0, 0,
				{	0,
					100,	/* reg: DASTORE */
				},{
					83,	/* reg: DASTORE */
				}
			};
			return &z;
		}
	case 83: /* AASTORE */
		{
			static struct burm_state z = { 83, 0, 0,
				{	0,
					100,	/* reg: AASTORE */
				},{
					84,	/* reg: AASTORE */
				}
			};
			return &z;
		}
	case 84: /* BASTORE */
		{
			static struct burm_state z = { 84, 0, 0,
				{	0,
					100,	/* reg: BASTORE */
				},{
					85,	/* reg: BASTORE */
				}
			};
			return &z;
		}
	case 85: /* CASTORE */
		{
			static struct burm_state z = { 85, 0, 0,
				{	0,
					100,	/* reg: CASTORE */
				},{
					86,	/* reg: CASTORE */
				}
			};
			return &z;
		}
	case 86: /* SASTORE */
		{
			static struct burm_state z = { 86, 0, 0,
				{	0,
					100,	/* reg: SASTORE */
				},{
					87,	/* reg: SASTORE */
				}
			};
			return &z;
		}
	case 87: /* POP */
		{
			static struct burm_state z = { 87, 0, 0,
				{	0,
					100,	/* reg: POP */
				},{
					88,	/* reg: POP */
				}
			};
			return &z;
		}
	case 88: /* POP2 */
		{
			static struct burm_state z = { 88, 0, 0,
				{	0,
					100,	/* reg: POP2 */
				},{
					89,	/* reg: POP2 */
				}
			};
			return &z;
		}
	case 89: /* DUP */
		{
			static struct burm_state z = { 89, 0, 0,
				{	0,
					100,	/* reg: DUP */
				},{
					90,	/* reg: DUP */
				}
			};
			return &z;
		}
	case 90: /* DUP_X1 */
		{
			static struct burm_state z = { 90, 0, 0,
				{	0,
					100,	/* reg: DUP_X1 */
				},{
					91,	/* reg: DUP_X1 */
				}
			};
			return &z;
		}
	case 91: /* DUP_X2 */
		{
			static struct burm_state z = { 91, 0, 0,
				{	0,
					100,	/* reg: DUP_X2 */
				},{
					92,	/* reg: DUP_X2 */
				}
			};
			return &z;
		}
	case 92: /* DUP2 */
		{
			static struct burm_state z = { 92, 0, 0,
				{	0,
					100,	/* reg: DUP2 */
				},{
					93,	/* reg: DUP2 */
				}
			};
			return &z;
		}
	case 93: /* DUP2_X1 */
		{
			static struct burm_state z = { 93, 0, 0,
				{	0,
					100,	/* reg: DUP2_X1 */
				},{
					94,	/* reg: DUP2_X1 */
				}
			};
			return &z;
		}
	case 94: /* DUP2_X2 */
		{
			static struct burm_state z = { 94, 0, 0,
				{	0,
					100,	/* reg: DUP2_X2 */
				},{
					95,	/* reg: DUP2_X2 */
				}
			};
			return &z;
		}
	case 95: /* SWAP */
		{
			static struct burm_state z = { 95, 0, 0,
				{	0,
					100,	/* reg: SWAP */
				},{
					96,	/* reg: SWAP */
				}
			};
			return &z;
		}
	case 96: /* IADD */
		{
			static struct burm_state z = { 96, 0, 0,
				{	0,
					100,	/* reg: IADD */
				},{
					97,	/* reg: IADD */
				}
			};
			return &z;
		}
	case 97: /* LADD */
		{
			static struct burm_state z = { 97, 0, 0,
				{	0,
					100,	/* reg: LADD */
				},{
					98,	/* reg: LADD */
				}
			};
			return &z;
		}
	case 98: /* FADD */
		{
			static struct burm_state z = { 98, 0, 0,
				{	0,
					100,	/* reg: FADD */
				},{
					99,	/* reg: FADD */
				}
			};
			return &z;
		}
	case 99: /* DADD */
		{
			static struct burm_state z = { 99, 0, 0,
				{	0,
					100,	/* reg: DADD */
				},{
					100,	/* reg: DADD */
				}
			};
			return &z;
		}
	case 100: /* ISUB */
		{
			static struct burm_state z = { 100, 0, 0,
				{	0,
					100,	/* reg: ISUB */
				},{
					101,	/* reg: ISUB */
				}
			};
			return &z;
		}
	case 101: /* LSUB */
		{
			static struct burm_state z = { 101, 0, 0,
				{	0,
					100,	/* reg: LSUB */
				},{
					102,	/* reg: LSUB */
				}
			};
			return &z;
		}
	case 102: /* FSUB */
		{
			static struct burm_state z = { 102, 0, 0,
				{	0,
					100,	/* reg: FSUB */
				},{
					103,	/* reg: FSUB */
				}
			};
			return &z;
		}
	case 103: /* DSUB */
		{
			static struct burm_state z = { 103, 0, 0,
				{	0,
					100,	/* reg: DSUB */
				},{
					104,	/* reg: DSUB */
				}
			};
			return &z;
		}
	case 104: /* IMUL */
		{
			static struct burm_state z = { 104, 0, 0,
				{	0,
					100,	/* reg: IMUL */
				},{
					105,	/* reg: IMUL */
				}
			};
			return &z;
		}
	case 105: /* LMUL */
		{
			static struct burm_state z = { 105, 0, 0,
				{	0,
					100,	/* reg: LMUL */
				},{
					106,	/* reg: LMUL */
				}
			};
			return &z;
		}
	case 106: /* FMUL */
		{
			static struct burm_state z = { 106, 0, 0,
				{	0,
					100,	/* reg: FMUL */
				},{
					107,	/* reg: FMUL */
				}
			};
			return &z;
		}
	case 107: /* DMUL */
		{
			static struct burm_state z = { 107, 0, 0,
				{	0,
					100,	/* reg: DMUL */
				},{
					108,	/* reg: DMUL */
				}
			};
			return &z;
		}
	case 108: /* IDIV */
		{
			static struct burm_state z = { 108, 0, 0,
				{	0,
					100,	/* reg: IDIV */
				},{
					109,	/* reg: IDIV */
				}
			};
			return &z;
		}
	case 109: /* LDIV */
		{
			static struct burm_state z = { 109, 0, 0,
				{	0,
					100,	/* reg: LDIV */
				},{
					110,	/* reg: LDIV */
				}
			};
			return &z;
		}
	case 110: /* FDIV */
		{
			static struct burm_state z = { 110, 0, 0,
				{	0,
					100,	/* reg: FDIV */
				},{
					111,	/* reg: FDIV */
				}
			};
			return &z;
		}
	case 111: /* DDIV */
		{
			static struct burm_state z = { 111, 0, 0,
				{	0,
					100,	/* reg: DDIV */
				},{
					112,	/* reg: DDIV */
				}
			};
			return &z;
		}
	case 112: /* IREM */
		{
			static struct burm_state z = { 112, 0, 0,
				{	0,
					100,	/* reg: IREM */
				},{
					113,	/* reg: IREM */
				}
			};
			return &z;
		}
	case 113: /* LREM */
		{
			static struct burm_state z = { 113, 0, 0,
				{	0,
					100,	/* reg: LREM */
				},{
					114,	/* reg: LREM */
				}
			};
			return &z;
		}
	case 114: /* FREM */
		{
			static struct burm_state z = { 114, 0, 0,
				{	0,
					100,	/* reg: FREM */
				},{
					115,	/* reg: FREM */
				}
			};
			return &z;
		}
	case 115: /* DREM */
		{
			static struct burm_state z = { 115, 0, 0,
				{	0,
					100,	/* reg: DREM */
				},{
					116,	/* reg: DREM */
				}
			};
			return &z;
		}
	case 116: /* INEG */
		{
			static struct burm_state z = { 116, 0, 0,
				{	0,
					100,	/* reg: INEG */
				},{
					117,	/* reg: INEG */
				}
			};
			return &z;
		}
	case 117: /* LNEG */
		{
			static struct burm_state z = { 117, 0, 0,
				{	0,
					100,	/* reg: LNEG */
				},{
					118,	/* reg: LNEG */
				}
			};
			return &z;
		}
	case 118: /* FNEG */
		{
			static struct burm_state z = { 118, 0, 0,
				{	0,
					100,	/* reg: FNEG */
				},{
					119,	/* reg: FNEG */
				}
			};
			return &z;
		}
	case 119: /* DNEG */
		{
			static struct burm_state z = { 119, 0, 0,
				{	0,
					100,	/* reg: DNEG */
				},{
					120,	/* reg: DNEG */
				}
			};
			return &z;
		}
	case 120: /* ISHL */
		{
			static struct burm_state z = { 120, 0, 0,
				{	0,
					100,	/* reg: ISHL */
				},{
					121,	/* reg: ISHL */
				}
			};
			return &z;
		}
	case 121: /* LSHL */
		{
			static struct burm_state z = { 121, 0, 0,
				{	0,
					100,	/* reg: LSHL */
				},{
					122,	/* reg: LSHL */
				}
			};
			return &z;
		}
	case 122: /* ISHR */
		{
			static struct burm_state z = { 122, 0, 0,
				{	0,
					100,	/* reg: ISHR */
				},{
					123,	/* reg: ISHR */
				}
			};
			return &z;
		}
	case 123: /* LSHR */
		{
			static struct burm_state z = { 123, 0, 0,
				{	0,
					100,	/* reg: LSHR */
				},{
					124,	/* reg: LSHR */
				}
			};
			return &z;
		}
	case 124: /* IUSHR */
		{
			static struct burm_state z = { 124, 0, 0,
				{	0,
					100,	/* reg: IUSHR */
				},{
					125,	/* reg: IUSHR */
				}
			};
			return &z;
		}
	case 125: /* LUSHR */
		{
			static struct burm_state z = { 125, 0, 0,
				{	0,
					100,	/* reg: LUSHR */
				},{
					126,	/* reg: LUSHR */
				}
			};
			return &z;
		}
	case 126: /* IAND */
		{
			static struct burm_state z = { 126, 0, 0,
				{	0,
					100,	/* reg: IAND */
				},{
					127,	/* reg: IAND */
				}
			};
			return &z;
		}
	case 127: /* LAND */
		{
			static struct burm_state z = { 127, 0, 0,
				{	0,
					100,	/* reg: LAND */
				},{
					128,	/* reg: LAND */
				}
			};
			return &z;
		}
	case 128: /* IOR */
		{
			static struct burm_state z = { 128, 0, 0,
				{	0,
					100,	/* reg: IOR */
				},{
					129,	/* reg: IOR */
				}
			};
			return &z;
		}
	case 129: /* LOR */
		{
			static struct burm_state z = { 129, 0, 0,
				{	0,
					100,	/* reg: LOR */
				},{
					130,	/* reg: LOR */
				}
			};
			return &z;
		}
	case 130: /* IXOR */
		{
			static struct burm_state z = { 130, 0, 0,
				{	0,
					100,	/* reg: IXOR */
				},{
					131,	/* reg: IXOR */
				}
			};
			return &z;
		}
	case 131: /* LXOR */
		{
			static struct burm_state z = { 131, 0, 0,
				{	0,
					100,	/* reg: LXOR */
				},{
					132,	/* reg: LXOR */
				}
			};
			return &z;
		}
	case 132: /* IINC */
		{
			static struct burm_state z = { 132, 0, 0,
				{	0,
					100,	/* reg: IINC */
				},{
					133,	/* reg: IINC */
				}
			};
			return &z;
		}
	case 133: /* I2L */
		{
			static struct burm_state z = { 133, 0, 0,
				{	0,
					100,	/* reg: I2L */
				},{
					134,	/* reg: I2L */
				}
			};
			return &z;
		}
	case 134: /* I2F */
		{
			static struct burm_state z = { 134, 0, 0,
				{	0,
					100,	/* reg: I2F */
				},{
					135,	/* reg: I2F */
				}
			};
			return &z;
		}
	case 135: /* I2D */
		{
			static struct burm_state z = { 135, 0, 0,
				{	0,
					100,	/* reg: I2D */
				},{
					136,	/* reg: I2D */
				}
			};
			return &z;
		}
	case 136: /* L2I */
		{
			static struct burm_state z = { 136, 0, 0,
				{	0,
					100,	/* reg: L2I */
				},{
					137,	/* reg: L2I */
				}
			};
			return &z;
		}
	case 137: /* L2F */
		{
			static struct burm_state z = { 137, 0, 0,
				{	0,
					100,	/* reg: L2F */
				},{
					138,	/* reg: L2F */
				}
			};
			return &z;
		}
	case 138: /* L2D */
		{
			static struct burm_state z = { 138, 0, 0,
				{	0,
					100,	/* reg: L2D */
				},{
					139,	/* reg: L2D */
				}
			};
			return &z;
		}
	case 139: /* F2I */
		{
			static struct burm_state z = { 139, 0, 0,
				{	0,
					100,	/* reg: F2I */
				},{
					140,	/* reg: F2I */
				}
			};
			return &z;
		}
	case 140: /* F2L */
		{
			static struct burm_state z = { 140, 0, 0,
				{	0,
					100,	/* reg: F2L */
				},{
					141,	/* reg: F2L */
				}
			};
			return &z;
		}
	case 141: /* F2D */
		{
			static struct burm_state z = { 141, 0, 0,
				{	0,
					100,	/* reg: F2D */
				},{
					142,	/* reg: F2D */
				}
			};
			return &z;
		}
	case 142: /* D2I */
		{
			static struct burm_state z = { 142, 0, 0,
				{	0,
					100,	/* reg: D2I */
				},{
					143,	/* reg: D2I */
				}
			};
			return &z;
		}
	case 143: /* D2L */
		{
			static struct burm_state z = { 143, 0, 0,
				{	0,
					100,	/* reg: D2L */
				},{
					144,	/* reg: D2L */
				}
			};
			return &z;
		}
	case 144: /* D2F */
		{
			static struct burm_state z = { 144, 0, 0,
				{	0,
					100,	/* reg: D2F */
				},{
					145,	/* reg: D2F */
				}
			};
			return &z;
		}
	case 145: /* INT2BYTE */
		{
			static struct burm_state z = { 145, 0, 0,
				{	0,
					100,	/* reg: INT2BYTE */
				},{
					146,	/* reg: INT2BYTE */
				}
			};
			return &z;
		}
	case 146: /* INT2CHAR */
		{
			static struct burm_state z = { 146, 0, 0,
				{	0,
					100,	/* reg: INT2CHAR */
				},{
					147,	/* reg: INT2CHAR */
				}
			};
			return &z;
		}
	case 147: /* INT2SHORT */
		{
			static struct burm_state z = { 147, 0, 0,
				{	0,
					100,	/* reg: INT2SHORT */
				},{
					148,	/* reg: INT2SHORT */
				}
			};
			return &z;
		}
	case 148: /* LCMP */
		{
			static struct burm_state z = { 148, 0, 0,
				{	0,
					100,	/* reg: LCMP */
				},{
					149,	/* reg: LCMP */
				}
			};
			return &z;
		}
	case 149: /* FCMPL */
		{
			static struct burm_state z = { 149, 0, 0,
				{	0,
					100,	/* reg: FCMPL */
				},{
					150,	/* reg: FCMPL */
				}
			};
			return &z;
		}
	case 150: /* FCMPG */
		{
			static struct burm_state z = { 150, 0, 0,
				{	0,
					100,	/* reg: FCMPG */
				},{
					151,	/* reg: FCMPG */
				}
			};
			return &z;
		}
	case 151: /* DCMPL */
		{
			static struct burm_state z = { 151, 0, 0,
				{	0,
					100,	/* reg: DCMPL */
				},{
					152,	/* reg: DCMPL */
				}
			};
			return &z;
		}
	case 152: /* DCMPG */
		{
			static struct burm_state z = { 152, 0, 0,
				{	0,
					100,	/* reg: DCMPG */
				},{
					153,	/* reg: DCMPG */
				}
			};
			return &z;
		}
	case 153: /* IFEQ */
		{
			static struct burm_state z = { 153, 0, 0,
				{	0,
					100,	/* reg: IFEQ */
				},{
					154,	/* reg: IFEQ */
				}
			};
			return &z;
		}
	case 154: /* IFNE */
		{
			static struct burm_state z = { 154, 0, 0,
				{	0,
					100,	/* reg: IFNE */
				},{
					155,	/* reg: IFNE */
				}
			};
			return &z;
		}
	case 155: /* IFLT */
		{
			static struct burm_state z = { 155, 0, 0,
				{	0,
					100,	/* reg: IFLT */
				},{
					156,	/* reg: IFLT */
				}
			};
			return &z;
		}
	case 156: /* IFGE */
		{
			static struct burm_state z = { 156, 0, 0,
				{	0,
					100,	/* reg: IFGE */
				},{
					157,	/* reg: IFGE */
				}
			};
			return &z;
		}
	case 157: /* IFGT */
		{
			static struct burm_state z = { 157, 0, 0,
				{	0,
					100,	/* reg: IFGT */
				},{
					158,	/* reg: IFGT */
				}
			};
			return &z;
		}
	case 158: /* IFLE */
		{
			static struct burm_state z = { 158, 0, 0,
				{	0,
					100,	/* reg: IFLE */
				},{
					159,	/* reg: IFLE */
				}
			};
			return &z;
		}
	case 159: /* IF_ICMPEQ */
		{
			static struct burm_state z = { 159, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPEQ */
				},{
					160,	/* reg: IF_ICMPEQ */
				}
			};
			return &z;
		}
	case 160: /* IF_ICMPNE */
		{
			static struct burm_state z = { 160, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPNE */
				},{
					161,	/* reg: IF_ICMPNE */
				}
			};
			return &z;
		}
	case 161: /* IF_ICMPLT */
		{
			static struct burm_state z = { 161, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPLT */
				},{
					162,	/* reg: IF_ICMPLT */
				}
			};
			return &z;
		}
	case 162: /* IF_ICMPGE */
		{
			static struct burm_state z = { 162, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPGE */
				},{
					163,	/* reg: IF_ICMPGE */
				}
			};
			return &z;
		}
	case 163: /* IF_ICMPGT */
		{
			static struct burm_state z = { 163, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPGT */
				},{
					164,	/* reg: IF_ICMPGT */
				}
			};
			return &z;
		}
	case 164: /* IF_ICMPLE */
		{
			static struct burm_state z = { 164, 0, 0,
				{	0,
					100,	/* reg: IF_ICMPLE */
				},{
					165,	/* reg: IF_ICMPLE */
				}
			};
			return &z;
		}
	case 165: /* IF_ACMPEQ */
		{
			static struct burm_state z = { 165, 0, 0,
				{	0,
					100,	/* reg: IF_ACMPEQ */
				},{
					166,	/* reg: IF_ACMPEQ */
				}
			};
			return &z;
		}
	case 166: /* IF_ACMPNE */
		{
			static struct burm_state z = { 166, 0, 0,
				{	0,
					100,	/* reg: IF_ACMPNE */
				},{
					167,	/* reg: IF_ACMPNE */
				}
			};
			return &z;
		}
	case 167: /* GOTO */
		{
			static struct burm_state z = { 167, 0, 0,
				{	0,
					100,	/* reg: GOTO */
				},{
					168,	/* reg: GOTO */
				}
			};
			return &z;
		}
	case 168: /* JSR */
		{
			static struct burm_state z = { 168, 0, 0,
				{	0,
					100,	/* reg: JSR */
				},{
					169,	/* reg: JSR */
				}
			};
			return &z;
		}
	case 169: /* RET */
		{
			static struct burm_state z = { 169, 0, 0,
				{	0,
					100,	/* reg: RET */
				},{
					170,	/* reg: RET */
				}
			};
			return &z;
		}
	case 170: /* TABLESWITCH */
		{
			static struct burm_state z = { 170, 0, 0,
				{	0,
					100,	/* reg: TABLESWITCH */
				},{
					171,	/* reg: TABLESWITCH */
				}
			};
			return &z;
		}
	case 171: /* LOOKUPSWITCH */
		{
			static struct burm_state z = { 171, 0, 0,
				{	0,
					100,	/* reg: LOOKUPSWITCH */
				},{
					172,	/* reg: LOOKUPSWITCH */
				}
			};
			return &z;
		}
	case 172: /* IRETURN */
		{
			static struct burm_state z = { 172, 0, 0,
				{	0,
					100,	/* reg: IRETURN */
				},{
					173,	/* reg: IRETURN */
				}
			};
			return &z;
		}
	case 173: /* LRETURN */
		{
			static struct burm_state z = { 173, 0, 0,
				{	0,
					100,	/* reg: LRETURN */
				},{
					174,	/* reg: LRETURN */
				}
			};
			return &z;
		}
	case 174: /* FRETURN */
		{
			static struct burm_state z = { 174, 0, 0,
				{	0,
					100,	/* reg: FRETURN */
				},{
					175,	/* reg: FRETURN */
				}
			};
			return &z;
		}
	case 175: /* DRETURN */
		{
			static struct burm_state z = { 175, 0, 0,
				{	0,
					100,	/* reg: DRETURN */
				},{
					176,	/* reg: DRETURN */
				}
			};
			return &z;
		}
	case 176: /* ARETURN */
		{
			static struct burm_state z = { 176, 0, 0,
				{	0,
					100,	/* reg: ARETURN */
				},{
					177,	/* reg: ARETURN */
				}
			};
			return &z;
		}
	case 177: /* RETURN */
		{
			static struct burm_state z = { 177, 0, 0,
				{	0,
					100,	/* reg: RETURN */
				},{
					178,	/* reg: RETURN */
				}
			};
			return &z;
		}
	case 178: /* GETSTATIC */
		{
			static struct burm_state z = { 178, 0, 0,
				{	0,
					100,	/* reg: GETSTATIC */
				},{
					179,	/* reg: GETSTATIC */
				}
			};
			return &z;
		}
	case 179: /* PUTSTATIC */
		{
			static struct burm_state z = { 179, 0, 0,
				{	0,
					100,	/* reg: PUTSTATIC */
				},{
					180,	/* reg: PUTSTATIC */
				}
			};
			return &z;
		}
	case 180: /* GETFIELD */
		{
			static struct burm_state z = { 180, 0, 0,
				{	0,
					100,	/* reg: GETFIELD */
				},{
					181,	/* reg: GETFIELD */
				}
			};
			return &z;
		}
	case 181: /* PUTFIELD */
		{
			static struct burm_state z = { 181, 0, 0,
				{	0,
					100,	/* reg: PUTFIELD */
				},{
					182,	/* reg: PUTFIELD */
				}
			};
			return &z;
		}
	case 182: /* INVOKEVIRTUAL */
		{
			static struct burm_state z = { 182, 0, 0,
				{	0,
					100,	/* reg: INVOKEVIRTUAL */
				},{
					183,	/* reg: INVOKEVIRTUAL */
				}
			};
			return &z;
		}
	case 183: /* INVOKESPECIAL */
		{
			static struct burm_state z = { 183, 0, 0,
				{	0,
					100,	/* reg: INVOKESPECIAL */
				},{
					184,	/* reg: INVOKESPECIAL */
				}
			};
			return &z;
		}
	case 184: /* INVOKESTATIC */
		{
			static struct burm_state z = { 184, 0, 0,
				{	0,
					100,	/* reg: INVOKESTATIC */
				},{
					185,	/* reg: INVOKESTATIC */
				}
			};
			return &z;
		}
	case 185: /* INVOKEINTERFACE */
		{
			static struct burm_state z = { 185, 0, 0,
				{	0,
					100,	/* reg: INVOKEINTERFACE */
				},{
					186,	/* reg: INVOKEINTERFACE */
				}
			};
			return &z;
		}
	case 186: /* UNDEF186 */
		{
			static struct burm_state z = { 186, 0, 0,
				{	0,
					100,	/* reg: UNDEF186 */
				},{
					187,	/* reg: UNDEF186 */
				}
			};
			return &z;
		}
	case 187: /* NEW */
		{
			static struct burm_state z = { 187, 0, 0,
				{	0,
					100,	/* reg: NEW */
				},{
					188,	/* reg: NEW */
				}
			};
			return &z;
		}
	case 188: /* NEWARRAY */
		{
			static struct burm_state z = { 188, 0, 0,
				{	0,
					100,	/* reg: NEWARRAY */
				},{
					189,	/* reg: NEWARRAY */
				}
			};
			return &z;
		}
	case 189: /* ANEWARRAY */
		{
			static struct burm_state z = { 189, 0, 0,
				{	0,
					100,	/* reg: ANEWARRAY */
				},{
					190,	/* reg: ANEWARRAY */
				}
			};
			return &z;
		}
	case 190: /* ARRAYLENGTH */
		{
			static struct burm_state z = { 190, 0, 0,
				{	0,
					100,	/* reg: ARRAYLENGTH */
				},{
					191,	/* reg: ARRAYLENGTH */
				}
			};
			return &z;
		}
	case 191: /* ATHROW */
		{
			static struct burm_state z = { 191, 0, 0,
				{	0,
					100,	/* reg: ATHROW */
				},{
					192,	/* reg: ATHROW */
				}
			};
			return &z;
		}
	case 192: /* CHECKCAST */
		{
			static struct burm_state z = { 192, 0, 0,
				{	0,
					100,	/* reg: CHECKCAST */
				},{
					193,	/* reg: CHECKCAST */
				}
			};
			return &z;
		}
	case 193: /* INSTANCEOF */
		{
			static struct burm_state z = { 193, 0, 0,
				{	0,
					100,	/* reg: INSTANCEOF */
				},{
					194,	/* reg: INSTANCEOF */
				}
			};
			return &z;
		}
	case 194: /* MONITORENTER */
		{
			static struct burm_state z = { 194, 0, 0,
				{	0,
					100,	/* reg: MONITORENTER */
				},{
					195,	/* reg: MONITORENTER */
				}
			};
			return &z;
		}
	case 195: /* MONITOREXIT */
		{
			static struct burm_state z = { 195, 0, 0,
				{	0,
					100,	/* reg: MONITOREXIT */
				},{
					196,	/* reg: MONITOREXIT */
				}
			};
			return &z;
		}
	case 196: /* UNDEF196 */
		{
			static struct burm_state z = { 196, 0, 0,
				{	0,
					100,	/* reg: UNDEF196 */
				},{
					197,	/* reg: UNDEF196 */
				}
			};
			return &z;
		}
	case 197: /* MULTIANEWARRAY */
		{
			static struct burm_state z = { 197, 0, 0,
				{	0,
					100,	/* reg: MULTIANEWARRAY */
				},{
					198,	/* reg: MULTIANEWARRAY */
				}
			};
			return &z;
		}
	case 198: /* IFNULL */
		{
			static struct burm_state z = { 198, 0, 0,
				{	0,
					100,	/* reg: IFNULL */
				},{
					199,	/* reg: IFNULL */
				}
			};
			return &z;
		}
	case 199: /* IFNONNULL */
		{
			static struct burm_state z = { 199, 0, 0,
				{	0,
					100,	/* reg: IFNONNULL */
				},{
					200,	/* reg: IFNONNULL */
				}
			};
			return &z;
		}
	case 200: /* UNDEF200 */
		{
			static struct burm_state z = { 200, 0, 0,
				{	0,
					100,	/* reg: UNDEF200 */
				},{
					201,	/* reg: UNDEF200 */
				}
			};
			return &z;
		}
	case 201: /* UNDEF201 */
		{
			static struct burm_state z = { 201, 0, 0,
				{	0,
					100,	/* reg: UNDEF201 */
				},{
					202,	/* reg: UNDEF201 */
				}
			};
			return &z;
		}
	case 202: /* BREAKPOINT */
		{
			static struct burm_state z = { 202, 0, 0,
				{	0,
					100,	/* reg: BREAKPOINT */
				},{
					203,	/* reg: BREAKPOINT */
				}
			};
			return &z;
		}
	case 203: /* UNDEF203 */
		{
			static struct burm_state z = { 203, 0, 0,
				{	0,
					100,	/* reg: UNDEF203 */
				},{
					204,	/* reg: UNDEF203 */
				}
			};
			return &z;
		}
	case 204: /* IASTORECONST */
		{
			static struct burm_state z = { 204, 0, 0,
				{	0,
					100,	/* reg: IASTORECONST */
				},{
					205,	/* reg: IASTORECONST */
				}
			};
			return &z;
		}
	case 205: /* LASTORECONST */
		{
			static struct burm_state z = { 205, 0, 0,
				{	0,
					100,	/* reg: LASTORECONST */
				},{
					206,	/* reg: LASTORECONST */
				}
			};
			return &z;
		}
	case 206: /* FASTORECONST */
		{
			static struct burm_state z = { 206, 0, 0,
				{	0,
					100,	/* reg: FASTORECONST */
				},{
					207,	/* reg: FASTORECONST */
				}
			};
			return &z;
		}
	case 207: /* DASTORECONST */
		{
			static struct burm_state z = { 207, 0, 0,
				{	0,
					100,	/* reg: DASTORECONST */
				},{
					208,	/* reg: DASTORECONST */
				}
			};
			return &z;
		}
	case 208: /* AASTORECONST */
		{
			static struct burm_state z = { 208, 0, 0,
				{	0,
					100,	/* reg: AASTORECONST */
				},{
					209,	/* reg: AASTORECONST */
				}
			};
			return &z;
		}
	case 209: /* BASTORECONST */
		{
			static struct burm_state z = { 209, 0, 0,
				{	0,
					100,	/* reg: BASTORECONST */
				},{
					210,	/* reg: BASTORECONST */
				}
			};
			return &z;
		}
	case 210: /* CASTORECONST */
		{
			static struct burm_state z = { 210, 0, 0,
				{	0,
					100,	/* reg: CASTORECONST */
				},{
					211,	/* reg: CASTORECONST */
				}
			};
			return &z;
		}
	case 211: /* SASTORECONST */
		{
			static struct burm_state z = { 211, 0, 0,
				{	0,
					100,	/* reg: SASTORECONST */
				},{
					212,	/* reg: SASTORECONST */
				}
			};
			return &z;
		}
	case 212: /* PUTSTATICCONST */
		{
			static struct burm_state z = { 212, 0, 0,
				{	0,
					100,	/* reg: PUTSTATICCONST */
				},{
					213,	/* reg: PUTSTATICCONST */
				}
			};
			return &z;
		}
	case 213: /* PUTFIELDCONST */
		{
			static struct burm_state z = { 213, 0, 0,
				{	0,
					100,	/* reg: PUTFIELDCONST */
				},{
					214,	/* reg: PUTFIELDCONST */
				}
			};
			return &z;
		}
	case 214: /* IMULPOW2 */
		{
			static struct burm_state z = { 214, 0, 0,
				{	0,
					100,	/* reg: IMULPOW2 */
				},{
					215,	/* reg: IMULPOW2 */
				}
			};
			return &z;
		}
	case 215: /* LMULPOW2 */
		{
			static struct burm_state z = { 215, 0, 0,
				{	0,
					100,	/* reg: LMULPOW2 */
				},{
					216,	/* reg: LMULPOW2 */
				}
			};
			return &z;
		}
	case 216: /* IF_FCMPEQ */
		{
			static struct burm_state z = { 216, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPEQ */
				},{
					217,	/* reg: IF_FCMPEQ */
				}
			};
			return &z;
		}
	case 217: /* IF_FCMPNE */
		{
			static struct burm_state z = { 217, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPNE */
				},{
					218,	/* reg: IF_FCMPNE */
				}
			};
			return &z;
		}
	case 218: /* IF_FCMPL_LT */
		{
			static struct burm_state z = { 218, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPL_LT */
				},{
					219,	/* reg: IF_FCMPL_LT */
				}
			};
			return &z;
		}
	case 219: /* IF_FCMPL_GE */
		{
			static struct burm_state z = { 219, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPL_GE */
				},{
					220,	/* reg: IF_FCMPL_GE */
				}
			};
			return &z;
		}
	case 220: /* IF_FCMPL_GT */
		{
			static struct burm_state z = { 220, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPL_GT */
				},{
					221,	/* reg: IF_FCMPL_GT */
				}
			};
			return &z;
		}
	case 221: /* IF_FCMPL_LE */
		{
			static struct burm_state z = { 221, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPL_LE */
				},{
					222,	/* reg: IF_FCMPL_LE */
				}
			};
			return &z;
		}
	case 222: /* IF_FCMPG_LT */
		{
			static struct burm_state z = { 222, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPG_LT */
				},{
					223,	/* reg: IF_FCMPG_LT */
				}
			};
			return &z;
		}
	case 223: /* IF_FCMPG_GE */
		{
			static struct burm_state z = { 223, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPG_GE */
				},{
					224,	/* reg: IF_FCMPG_GE */
				}
			};
			return &z;
		}
	case 224: /* IF_FCMPG_GT */
		{
			static struct burm_state z = { 224, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPG_GT */
				},{
					225,	/* reg: IF_FCMPG_GT */
				}
			};
			return &z;
		}
	case 225: /* IF_FCMPG_LE */
		{
			static struct burm_state z = { 225, 0, 0,
				{	0,
					100,	/* reg: IF_FCMPG_LE */
				},{
					226,	/* reg: IF_FCMPG_LE */
				}
			};
			return &z;
		}
	case 226: /* IF_DCMPEQ */
		{
			static struct burm_state z = { 226, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPEQ */
				},{
					227,	/* reg: IF_DCMPEQ */
				}
			};
			return &z;
		}
	case 227: /* IF_DCMPNE */
		{
			static struct burm_state z = { 227, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPNE */
				},{
					228,	/* reg: IF_DCMPNE */
				}
			};
			return &z;
		}
	case 228: /* IF_DCMPL_LT */
		{
			static struct burm_state z = { 228, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPL_LT */
				},{
					229,	/* reg: IF_DCMPL_LT */
				}
			};
			return &z;
		}
	case 229: /* IF_DCMPL_GE */
		{
			static struct burm_state z = { 229, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPL_GE */
				},{
					230,	/* reg: IF_DCMPL_GE */
				}
			};
			return &z;
		}
	case 230: /* IF_DCMPL_GT */
		{
			static struct burm_state z = { 230, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPL_GT */
				},{
					231,	/* reg: IF_DCMPL_GT */
				}
			};
			return &z;
		}
	case 231: /* IF_DCMPL_LE */
		{
			static struct burm_state z = { 231, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPL_LE */
				},{
					232,	/* reg: IF_DCMPL_LE */
				}
			};
			return &z;
		}
	case 232: /* IF_DCMPG_LT */
		{
			static struct burm_state z = { 232, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPG_LT */
				},{
					233,	/* reg: IF_DCMPG_LT */
				}
			};
			return &z;
		}
	case 233: /* IF_DCMPG_GE */
		{
			static struct burm_state z = { 233, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPG_GE */
				},{
					234,	/* reg: IF_DCMPG_GE */
				}
			};
			return &z;
		}
	case 234: /* IF_DCMPG_GT */
		{
			static struct burm_state z = { 234, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPG_GT */
				},{
					235,	/* reg: IF_DCMPG_GT */
				}
			};
			return &z;
		}
	case 235: /* IF_DCMPG_LE */
		{
			static struct burm_state z = { 235, 0, 0,
				{	0,
					100,	/* reg: IF_DCMPG_LE */
				},{
					236,	/* reg: IF_DCMPG_LE */
				}
			};
			return &z;
		}
	case 236: /* UNDEF236 */
		{
			static struct burm_state z = { 236, 0, 0,
				{	0,
					100,	/* reg: UNDEF236 */
				},{
					237,	/* reg: UNDEF236 */
				}
			};
			return &z;
		}
	case 237: /* UNDEF237 */
		{
			static struct burm_state z = { 237, 0, 0,
				{	0,
					100,	/* reg: UNDEF237 */
				},{
					238,	/* reg: UNDEF237 */
				}
			};
			return &z;
		}
	case 238: /* UNDEF238 */
		{
			static struct burm_state z = { 238, 0, 0,
				{	0,
					100,	/* reg: UNDEF238 */
				},{
					239,	/* reg: UNDEF238 */
				}
			};
			return &z;
		}
	case 239: /* UNDEF239 */
		{
			static struct burm_state z = { 239, 0, 0,
				{	0,
					100,	/* reg: UNDEF239 */
				},{
					240,	/* reg: UNDEF239 */
				}
			};
			return &z;
		}
	case 240: /* UNDEF240 */
		{
			static struct burm_state z = { 240, 0, 0,
				{	0,
					100,	/* reg: UNDEF240 */
				},{
					241,	/* reg: UNDEF240 */
				}
			};
			return &z;
		}
	case 241: /* UNDEF241 */
		{
			static struct burm_state z = { 241, 0, 0,
				{	0,
					100,	/* reg: UNDEF241 */
				},{
					242,	/* reg: UNDEF241 */
				}
			};
			return &z;
		}
	case 242: /* UNDEF242 */
		{
			static struct burm_state z = { 242, 0, 0,
				{	0,
					100,	/* reg: UNDEF242 */
				},{
					243,	/* reg: UNDEF242 */
				}
			};
			return &z;
		}
	case 243: /* UNDEF243 */
		{
			static struct burm_state z = { 243, 0, 0,
				{	0,
					100,	/* reg: UNDEF243 */
				},{
					244,	/* reg: UNDEF243 */
				}
			};
			return &z;
		}
	case 244: /* UNDEF244 */
		{
			static struct burm_state z = { 244, 0, 0,
				{	0,
					100,	/* reg: UNDEF244 */
				},{
					245,	/* reg: UNDEF244 */
				}
			};
			return &z;
		}
	case 245: /* UNDEF245 */
		{
			static struct burm_state z = { 245, 0, 0,
				{	0,
					100,	/* reg: UNDEF245 */
				},{
					246,	/* reg: UNDEF245 */
				}
			};
			return &z;
		}
	case 246: /* UNDEF246 */
		{
			static struct burm_state z = { 246, 0, 0,
				{	0,
					100,	/* reg: UNDEF246 */
				},{
					247,	/* reg: UNDEF246 */
				}
			};
			return &z;
		}
	case 247: /* UNDEF247 */
		{
			static struct burm_state z = { 247, 0, 0,
				{	0,
					100,	/* reg: UNDEF247 */
				},{
					248,	/* reg: UNDEF247 */
				}
			};
			return &z;
		}
	case 248: /* UNDEF248 */
		{
			static struct burm_state z = { 248, 0, 0,
				{	0,
					100,	/* reg: UNDEF248 */
				},{
					249,	/* reg: UNDEF248 */
				}
			};
			return &z;
		}
	case 249: /* GETEXCEPTION */
		{
			static struct burm_state z = { 249, 0, 0,
				{	0,
					100,	/* reg: GETEXCEPTION */
				},{
					250,	/* reg: GETEXCEPTION */
				}
			};
			return &z;
		}
	case 250: /* PHI */
		{
			static struct burm_state z = { 250, 0, 0,
				{	0,
					100,	/* reg: PHI */
				},{
					251,	/* reg: PHI */
				}
			};
			return &z;
		}
	case 251: /* INLINE_START */
		{
			static struct burm_state z = { 251, 0, 0,
				{	0,
					100,	/* reg: INLINE_START */
				},{
					252,	/* reg: INLINE_START */
				}
			};
			return &z;
		}
	case 252: /* INLINE_END */
		{
			static struct burm_state z = { 252, 0, 0,
				{	0,
					100,	/* reg: INLINE_END */
				},{
					253,	/* reg: INLINE_END */
				}
			};
			return &z;
		}
	case 253: /* INLINE_BODY */
		{
			static struct burm_state z = { 253, 0, 0,
				{	0,
					100,	/* reg: INLINE_BODY */
				},{
					254,	/* reg: INLINE_BODY */
				}
			};
			return &z;
		}
	case 254: /* UNDEF254 */
		{
			static struct burm_state z = { 254, 0, 0,
				{	0,
					100,	/* reg: UNDEF254 */
				},{
					255,	/* reg: UNDEF254 */
				}
			};
			return &z;
		}
	case 255: /* BUILTIN */
		{
			static struct burm_state z = { 255, 0, 0,
				{	0,
					100,	/* reg: BUILTIN */
				},{
					256,	/* reg: BUILTIN */
				}
			};
			return &z;
		}
	default:
		burm_assert(0, PANIC("Bad operator %d in burm_state\n", op));
	}
	return p;
}

#ifdef STATE_LABEL
static void burm_label1(NODEPTR_TYPE p) {
	burm_assert(p, PANIC("NULL tree in burm_label\n"));
	switch (burm_arity[OP_LABEL(p)]) {
	case 0:
		STATE_LABEL(p) = burm_state(OP_LABEL(p), 0, 0);
		break;
	case 1:
		burm_label1(LEFT_CHILD(p));
		STATE_LABEL(p) = burm_state(OP_LABEL(p),
			STATE_LABEL(LEFT_CHILD(p)), 0);
		break;
	case 2:
		burm_label1(LEFT_CHILD(p));
		burm_label1(RIGHT_CHILD(p));
		STATE_LABEL(p) = burm_state(OP_LABEL(p),
			STATE_LABEL(LEFT_CHILD(p)),
			STATE_LABEL(RIGHT_CHILD(p)));
		break;
	}
}

STATEPTR_TYPE burm_label(NODEPTR_TYPE p) {
	burm_label1(p);
	return STATE_LABEL(p)->rule.burm_reg ? STATE_LABEL(p) : 0;
}

NODEPTR_TYPE *burm_kids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]) {
	burm_assert(p, PANIC("NULL tree in burm_kids\n"));
	burm_assert(kids, PANIC("NULL kids in burm_kids\n"));
	switch (eruleno) {
	case 256: /* reg: BUILTIN */
	case 255: /* reg: UNDEF254 */
	case 254: /* reg: INLINE_BODY */
	case 253: /* reg: INLINE_END */
	case 252: /* reg: INLINE_START */
	case 251: /* reg: PHI */
	case 250: /* reg: GETEXCEPTION */
	case 249: /* reg: UNDEF248 */
	case 248: /* reg: UNDEF247 */
	case 247: /* reg: UNDEF246 */
	case 246: /* reg: UNDEF245 */
	case 245: /* reg: UNDEF244 */
	case 244: /* reg: UNDEF243 */
	case 243: /* reg: UNDEF242 */
	case 242: /* reg: UNDEF241 */
	case 241: /* reg: UNDEF240 */
	case 240: /* reg: UNDEF239 */
	case 239: /* reg: UNDEF238 */
	case 238: /* reg: UNDEF237 */
	case 237: /* reg: UNDEF236 */
	case 236: /* reg: IF_DCMPG_LE */
	case 235: /* reg: IF_DCMPG_GT */
	case 234: /* reg: IF_DCMPG_GE */
	case 233: /* reg: IF_DCMPG_LT */
	case 232: /* reg: IF_DCMPL_LE */
	case 231: /* reg: IF_DCMPL_GT */
	case 230: /* reg: IF_DCMPL_GE */
	case 229: /* reg: IF_DCMPL_LT */
	case 228: /* reg: IF_DCMPNE */
	case 227: /* reg: IF_DCMPEQ */
	case 226: /* reg: IF_FCMPG_LE */
	case 225: /* reg: IF_FCMPG_GT */
	case 224: /* reg: IF_FCMPG_GE */
	case 223: /* reg: IF_FCMPG_LT */
	case 222: /* reg: IF_FCMPL_LE */
	case 221: /* reg: IF_FCMPL_GT */
	case 220: /* reg: IF_FCMPL_GE */
	case 219: /* reg: IF_FCMPL_LT */
	case 218: /* reg: IF_FCMPNE */
	case 217: /* reg: IF_FCMPEQ */
	case 216: /* reg: LMULPOW2 */
	case 215: /* reg: IMULPOW2 */
	case 214: /* reg: PUTFIELDCONST */
	case 213: /* reg: PUTSTATICCONST */
	case 212: /* reg: SASTORECONST */
	case 211: /* reg: CASTORECONST */
	case 210: /* reg: BASTORECONST */
	case 209: /* reg: AASTORECONST */
	case 208: /* reg: DASTORECONST */
	case 207: /* reg: FASTORECONST */
	case 206: /* reg: LASTORECONST */
	case 205: /* reg: IASTORECONST */
	case 204: /* reg: UNDEF203 */
	case 203: /* reg: BREAKPOINT */
	case 202: /* reg: UNDEF201 */
	case 201: /* reg: UNDEF200 */
	case 200: /* reg: IFNONNULL */
	case 199: /* reg: IFNULL */
	case 198: /* reg: MULTIANEWARRAY */
	case 197: /* reg: UNDEF196 */
	case 196: /* reg: MONITOREXIT */
	case 195: /* reg: MONITORENTER */
	case 194: /* reg: INSTANCEOF */
	case 193: /* reg: CHECKCAST */
	case 192: /* reg: ATHROW */
	case 191: /* reg: ARRAYLENGTH */
	case 190: /* reg: ANEWARRAY */
	case 189: /* reg: NEWARRAY */
	case 188: /* reg: NEW */
	case 187: /* reg: UNDEF186 */
	case 186: /* reg: INVOKEINTERFACE */
	case 185: /* reg: INVOKESTATIC */
	case 184: /* reg: INVOKESPECIAL */
	case 183: /* reg: INVOKEVIRTUAL */
	case 182: /* reg: PUTFIELD */
	case 181: /* reg: GETFIELD */
	case 180: /* reg: PUTSTATIC */
	case 179: /* reg: GETSTATIC */
	case 178: /* reg: RETURN */
	case 177: /* reg: ARETURN */
	case 176: /* reg: DRETURN */
	case 175: /* reg: FRETURN */
	case 174: /* reg: LRETURN */
	case 173: /* reg: IRETURN */
	case 172: /* reg: LOOKUPSWITCH */
	case 171: /* reg: TABLESWITCH */
	case 170: /* reg: RET */
	case 169: /* reg: JSR */
	case 168: /* reg: GOTO */
	case 167: /* reg: IF_ACMPNE */
	case 166: /* reg: IF_ACMPEQ */
	case 165: /* reg: IF_ICMPLE */
	case 164: /* reg: IF_ICMPGT */
	case 163: /* reg: IF_ICMPGE */
	case 162: /* reg: IF_ICMPLT */
	case 161: /* reg: IF_ICMPNE */
	case 160: /* reg: IF_ICMPEQ */
	case 159: /* reg: IFLE */
	case 158: /* reg: IFGT */
	case 157: /* reg: IFGE */
	case 156: /* reg: IFLT */
	case 155: /* reg: IFNE */
	case 154: /* reg: IFEQ */
	case 153: /* reg: DCMPG */
	case 152: /* reg: DCMPL */
	case 151: /* reg: FCMPG */
	case 150: /* reg: FCMPL */
	case 149: /* reg: LCMP */
	case 148: /* reg: INT2SHORT */
	case 147: /* reg: INT2CHAR */
	case 146: /* reg: INT2BYTE */
	case 145: /* reg: D2F */
	case 144: /* reg: D2L */
	case 143: /* reg: D2I */
	case 142: /* reg: F2D */
	case 141: /* reg: F2L */
	case 140: /* reg: F2I */
	case 139: /* reg: L2D */
	case 138: /* reg: L2F */
	case 137: /* reg: L2I */
	case 136: /* reg: I2D */
	case 135: /* reg: I2F */
	case 134: /* reg: I2L */
	case 133: /* reg: IINC */
	case 132: /* reg: LXOR */
	case 131: /* reg: IXOR */
	case 130: /* reg: LOR */
	case 129: /* reg: IOR */
	case 128: /* reg: LAND */
	case 127: /* reg: IAND */
	case 126: /* reg: LUSHR */
	case 125: /* reg: IUSHR */
	case 124: /* reg: LSHR */
	case 123: /* reg: ISHR */
	case 122: /* reg: LSHL */
	case 121: /* reg: ISHL */
	case 120: /* reg: DNEG */
	case 119: /* reg: FNEG */
	case 118: /* reg: LNEG */
	case 117: /* reg: INEG */
	case 116: /* reg: DREM */
	case 115: /* reg: FREM */
	case 114: /* reg: LREM */
	case 113: /* reg: IREM */
	case 112: /* reg: DDIV */
	case 111: /* reg: FDIV */
	case 110: /* reg: LDIV */
	case 109: /* reg: IDIV */
	case 108: /* reg: DMUL */
	case 107: /* reg: FMUL */
	case 106: /* reg: LMUL */
	case 105: /* reg: IMUL */
	case 104: /* reg: DSUB */
	case 103: /* reg: FSUB */
	case 102: /* reg: LSUB */
	case 101: /* reg: ISUB */
	case 100: /* reg: DADD */
	case 99: /* reg: FADD */
	case 98: /* reg: LADD */
	case 97: /* reg: IADD */
	case 96: /* reg: SWAP */
	case 95: /* reg: DUP2_X2 */
	case 94: /* reg: DUP2_X1 */
	case 93: /* reg: DUP2 */
	case 92: /* reg: DUP_X2 */
	case 91: /* reg: DUP_X1 */
	case 90: /* reg: DUP */
	case 89: /* reg: POP2 */
	case 88: /* reg: POP */
	case 87: /* reg: SASTORE */
	case 86: /* reg: CASTORE */
	case 85: /* reg: BASTORE */
	case 84: /* reg: AASTORE */
	case 83: /* reg: DASTORE */
	case 82: /* reg: FASTORE */
	case 81: /* reg: LASTORE */
	case 80: /* reg: IASTORE */
	case 79: /* reg: UNDEF78 */
	case 78: /* reg: UNDEF77 */
	case 77: /* reg: UNDEF76 */
	case 76: /* reg: UNDEF75 */
	case 75: /* reg: UNDEF74 */
	case 74: /* reg: UNDEF73 */
	case 73: /* reg: UNDEF72 */
	case 72: /* reg: UNDEF71 */
	case 71: /* reg: IF_LCMPLE */
	case 70: /* reg: IF_LCMPGT */
	case 69: /* reg: IF_LCMPGE */
	case 68: /* reg: IF_LCMPLT */
	case 67: /* reg: IF_LCMPNE */
	case 66: /* reg: IF_LCMPEQ */
	case 65: /* reg: IF_LLE */
	case 64: /* reg: IF_LGT */
	case 63: /* reg: IF_LGE */
	case 62: /* reg: IF_LLT */
	case 61: /* reg: IF_LNE */
	case 60: /* reg: IF_LEQ */
	case 59: /* reg: ASTORE */
	case 58: /* reg: DSTORE */
	case 57: /* reg: FSTORE */
	case 56: /* reg: LSTORE */
	case 55: /* reg: ISTORE */
	case 54: /* reg: SALOAD */
	case 53: /* reg: CALOAD */
	case 52: /* reg: BALOAD */
	case 51: /* reg: AALOAD */
	case 50: /* reg: DALOAD */
	case 49: /* reg: FALOAD */
	case 48: /* reg: LALOAD */
	case 47: /* reg: IALOAD */
	case 46: /* reg: LREMPOW2 */
	case 45: /* reg: LUSHRCONST */
	case 44: /* reg: LSHRCONST */
	case 43: /* reg: LSHLCONST */
	case 42: /* reg: LXORCONST */
	case 41: /* reg: LORCONST */
	case 40: /* reg: LANDCONST */
	case 39: /* reg: LMULCONST */
	case 38: /* reg: LSUBCONST */
	case 37: /* reg: LADDCONST */
	case 36: /* reg: IREMPOW2 */
	case 35: /* reg: IUSHRCONST */
	case 34: /* reg: ISHRCONST */
	case 33: /* reg: ISHLCONST */
	case 32: /* reg: IXORCONST */
	case 31: /* reg: IORCONST */
	case 30: /* reg: IANDCONST */
	case 29: /* reg: IMULCONST */
	case 28: /* reg: ISUBCONST */
	case 27: /* reg: IADDCONST */
	case 26: /* reg: ALOAD */
	case 25: /* reg: DLOAD */
	case 24: /* reg: FLOAD */
	case 23: /* reg: LLOAD */
	case 22: /* reg: ILOAD */
	case 21: /* reg: UNDEF20 */
	case 20: /* reg: UNDEF19 */
	case 19: /* reg: UNDEF18 */
	case 18: /* reg: UNDEF17 */
	case 17: /* reg: MOVE */
	case 16: /* reg: COPY */
	case 15: /* reg: DCONST */
	case 14: /* reg: UNDEF13 */
	case 13: /* reg: UNDEF12 */
	case 12: /* reg: FCONST */
	case 11: /* reg: LCMPCONST */
	case 10: /* reg: LCONST */
	case 9: /* reg: UNDEF8 */
	case 8: /* reg: UNDEF7 */
	case 7: /* reg: LDIVPOW2 */
	case 6: /* reg: IDIVPOW2 */
	case 5: /* reg: UNDEF4 */
	case 4: /* reg: ICONST */
	case 3: /* reg: CHECKNULL */
	case 2: /* reg: ACONST */
	case 1: /* reg: NOP */
		break;
	default:
		burm_assert(0, PANIC("Bad external rule number %d in burm_kids\n", eruleno));
	}
	return kids;
}

int burm_op_label(NODEPTR_TYPE p) {
	burm_assert(p, PANIC("NULL tree in burm_op_label\n"));
	return OP_LABEL(p);
}

STATEPTR_TYPE burm_state_label(NODEPTR_TYPE p) {
	burm_assert(p, PANIC("NULL tree in burm_state_label\n"));
	return STATE_LABEL(p);
}

NODEPTR_TYPE burm_child(NODEPTR_TYPE p, int index) {
	burm_assert(p, PANIC("NULL tree in burm_child\n"));
	switch (index) {
	case 0:	return LEFT_CHILD(p);
	case 1:	return RIGHT_CHILD(p);
	}
	burm_assert(0, PANIC("Bad index %d in burm_child\n", index));
	return 0;
}

#endif

void burm_reduce(NODEPTR_TYPE bnode, int goalnt);
void burm_reduce(NODEPTR_TYPE bnode, int goalnt)
{
  int ruleNo = burm_rule (STATE_LABEL(bnode), goalnt);
  short *nts = burm_nts[ruleNo];
  NODEPTR_TYPE kids[100];
  int i;

#if DEBUG
  fprintf (stderr, "%s %d\n", burm_opname[bnode->op], ruleNo);  /* display rule */
  fflush(stderr);
#endif
  if (ruleNo==0) {
    fprintf(stderr, "Tree cannot be derived from start symbol\n");
    exit(1);
  }
  switch (ruleNo) {
  case 1:
 codegen_nop(bnode); 
    break;
  case 2:
 codegen_emit_instruction(bnode->jd, bnode->iptr);   
    break;
  case 3:
 codegen_emit_checknull(bnode);    
    break;
  case 4:
 codegen_emit_iconst(bnode);   
    break;
  case 5:
 codegen_emit_undef(bnode);    
    break;
  case 6:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 7:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 8:
 codegen_emit_undef(bnode);    
    break;
  case 9:
 codegen_emit_undef(bnode);    
    break;
  case 10:
 codegen_emit_lconst(bnode);   
    break;
  case 11:
 codegen_emit_unknown(bnode);    
    break;
  case 12:
 codegen_emit_instruction(bnode->jd, bnode->iptr);   
    break;
  case 13:
 codegen_emit_undef(bnode);    
    break;
  case 14:
 codegen_emit_undef(bnode);    
    break;
  case 15:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 16:
 codegen_emit_copy(bnode);    
    break;
  case 17:
 codegen_emit_copy(bnode);    
    break;
  case 18:
 codegen_emit_undef(bnode);    
    break;
  case 19:
 codegen_emit_undef(bnode);    
    break;
  case 20:
 codegen_emit_undef(bnode);    
    break;
  case 21:
 codegen_emit_undef(bnode);    
    break;
  case 22:
 codegen_emit_copy(bnode);    
    break;
  case 23:
 codegen_emit_copy(bnode);    
    break;
  case 24:
 codegen_emit_copy(bnode);    
    break;
  case 25:
 codegen_emit_copy(bnode);    
    break;
  case 26:
 codegen_emit_copy(bnode);    
    break;
  case 27:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 28:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 29:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 30:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 31:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 32:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 33:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 34:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 35:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 36:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 37:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 38:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 39:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 40:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 41:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 42:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 43:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 44:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 45:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 46:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 47:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 48:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 49:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 50:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 51:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 52:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 53:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 54:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 55:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 56:
 codegen_emit_copy(bnode);    
    break;
  case 57:
 codegen_emit_copy(bnode);    
    break;
  case 58:
 codegen_emit_copy(bnode);    
    break;
  case 59:
 codegen_emit_astore(bnode);    
    break;
  case 60:
 codegen_emit_unknown(bnode);    
    break;
  case 61:
 codegen_emit_unknown(bnode);    
    break;
  case 62:
 codegen_emit_unknown(bnode);    
    break;
  case 63:
 codegen_emit_unknown(bnode);    
    break;
  case 64:
 codegen_emit_unknown(bnode);    
    break;
  case 65:
 codegen_emit_unknown(bnode);    
    break;
  case 66:
 codegen_emit_unknown(bnode);    
    break;
  case 67:
 codegen_emit_unknown(bnode);    
    break;
  case 68:
 codegen_emit_unknown(bnode);    
    break;
  case 69:
 codegen_emit_unknown(bnode);    
    break;
  case 70:
 codegen_emit_unknown(bnode);    
    break;
  case 71:
 codegen_emit_unknown(bnode);    
    break;
  case 72:
 codegen_emit_undef(bnode);    
    break;
  case 73:
 codegen_emit_undef(bnode);    
    break;
  case 74:
 codegen_emit_undef(bnode);    
    break;
  case 75:
 codegen_emit_undef(bnode);    
    break;
  case 76:
 codegen_emit_undef(bnode);    
    break;
  case 77:
 codegen_emit_undef(bnode);    
    break;
  case 78:
 codegen_emit_undef(bnode);    
    break;
  case 79:
 codegen_emit_undef(bnode);    
    break;
  case 80:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 81:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 82:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 83:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 84:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 85:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 86:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 87:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 88:
 codegen_nop(bnode);    
    break;
  case 89:
 codegen_nop(bnode);    
    break;
  case 90:
 codegen_emit_unknown(bnode);    
    break;
  case 91:
 codegen_emit_unknown(bnode);    
    break;
  case 92:
 codegen_emit_unknown(bnode);    
    break;
  case 93:
 codegen_emit_unknown(bnode);    
    break;
  case 94:
 codegen_emit_unknown(bnode);    
    break;
  case 95:
 codegen_emit_unknown(bnode);    
    break;
  case 96:
 codegen_emit_unknown(bnode);    
    break;
  case 97:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 98:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 99:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 100:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 101:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 102:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 103:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 104:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 105:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 106:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 107:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 108:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 109:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 110:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 111:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 112:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 113:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 114:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 115:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 116:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 117:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 118:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 119:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 120:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 121:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 122:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 123:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 124:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 125:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 126:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 127:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 128:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 129:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 130:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 131:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 132:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 133:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 134:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 135:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 136:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 137:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 138:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 139:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 140:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 141:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 142:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 143:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 144:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 145:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 146:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 147:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 148:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 149:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 150:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 151:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 152:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 153:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 154:
 codegen_emit_branch(bnode);    
    break;
  case 155:
 codegen_emit_branch(bnode);    
    break;
  case 156:
 codegen_emit_branch(bnode);    
    break;
  case 157:
 codegen_emit_branch(bnode);    
    break;
  case 158:
 codegen_emit_branch(bnode);    
    break;
  case 159:
 codegen_emit_branch(bnode);    
    break;
  case 160:
 codegen_emit_branch(bnode);    
    break;
  case 161:
 codegen_emit_branch(bnode);    
    break;
  case 162:
 codegen_emit_branch(bnode);    
    break;
  case 163:
 codegen_emit_branch(bnode);    
    break;
  case 164:
 codegen_emit_branch(bnode);    
    break;
  case 165:
 codegen_emit_branch(bnode);    
    break;
  case 166:
 codegen_emit_branch(bnode);    
    break;
  case 167:
 codegen_emit_branch(bnode);    
    break;
  case 168:
 codegen_emit_jump(bnode);    
    break;
  case 169:
 codegen_emit_jump(bnode);    
    break;
  case 170:
 codegen_emit_jump(bnode);    
    break;
  case 171:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 172:
 codegen_emit_lookup(bnode);    
    break;
  case 173:
 codegen_emit_return(bnode);    
    break;
  case 174:
 codegen_emit_return(bnode);    
    break;
  case 175:
 codegen_emit_return(bnode);    
    break;
  case 176:
 codegen_emit_return(bnode);    
    break;
  case 177:
 codegen_emit_return(bnode);    
    break;
  case 178:
 codegen_emit_return(bnode);    
    break;
  case 179:
 codegen_emit_getstatic(bnode);    
    break;
  case 180:
 codegen_emit_putstatic(bnode);    
    break;
  case 181:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 182:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 183:
 codegen_emit_invoke(bnode);    
    break;
  case 184:
 codegen_emit_invoke(bnode);    
    break;
  case 185:
 codegen_emit_invoke(bnode);    
    break;
  case 186:
 codegen_emit_invoke(bnode);    
    break;
  case 187:
 codegen_emit_undef(bnode);    
    break;
  case 188:
 codegen_emit_unknown(bnode);    
    break;
  case 189:
 codegen_emit_unknown(bnode);    
    break;
  case 190:
 codegen_emit_unknown(bnode);    
    break;
  case 191:
 codegen_emit_unknown(bnode);    
    break;
  case 192:
 codegen_emit_unknown(bnode);    
    break;
  case 193:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 194:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 195:
 codegen_emit_unknown(bnode);    
    break;
  case 196:
 codegen_emit_unknown(bnode);    
    break;
  case 197:
 codegen_emit_undef(bnode);    
    break;
  case 198:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 199:
 codegen_emit_ifnull(bnode);    
    break;
  case 200:
 codegen_emit_ifnull(bnode);    
    break;
  case 201:
 codegen_emit_undef(bnode);    
    break;
  case 202:
 codegen_emit_undef(bnode);    
    break;
  case 203:
 codegen_emit_breakpoint(bnode);    
    break;
  case 204:
 codegen_emit_undef(bnode);    
    break;
  case 205:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 206:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 207:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 208:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 209:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 210:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 211:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 212:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 213:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 214:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 215:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 216:
 codegen_emit_instruction(bnode->jd, bnode->iptr);    
    break;
  case 217:
 codegen_emit_unknown(bnode);    
    break;
  case 218:
 codegen_emit_unknown(bnode);    
    break;
  case 219:
 codegen_emit_unknown(bnode);    
    break;
  case 220:
 codegen_emit_unknown(bnode);    
    break;
  case 221:
 codegen_emit_unknown(bnode);    
    break;
  case 222:
 codegen_emit_unknown(bnode);    
    break;
  case 223:
 codegen_emit_unknown(bnode);    
    break;
  case 224:
 codegen_emit_unknown(bnode);    
    break;
  case 225:
 codegen_emit_unknown(bnode);    
    break;
  case 226:
 codegen_emit_unknown(bnode);    
    break;
  case 227:
 codegen_emit_unknown(bnode);    
    break;
  case 228:
 codegen_emit_unknown(bnode);    
    break;
  case 229:
 codegen_emit_unknown(bnode);    
    break;
  case 230:
 codegen_emit_unknown(bnode);    
    break;
  case 231:
 codegen_emit_unknown(bnode);    
    break;
  case 232:
 codegen_emit_unknown(bnode);    
    break;
  case 233:
 codegen_emit_unknown(bnode);    
    break;
  case 234:
 codegen_emit_unknown(bnode);    
    break;
  case 235:
 codegen_emit_unknown(bnode);    
    break;
  case 236:
 codegen_emit_unknown(bnode);    
    break;
  case 237:
 codegen_emit_undef(bnode);    
    break;
  case 238:
 codegen_emit_undef(bnode);    
    break;
  case 239:
 codegen_emit_undef(bnode);    
    break;
  case 240:
 codegen_emit_undef(bnode);    
    break;
  case 241:
 codegen_emit_undef(bnode);    
    break;
  case 242:
 codegen_emit_undef(bnode);    
    break;
  case 243:
 codegen_emit_undef(bnode);    
    break;
  case 244:
 codegen_emit_undef(bnode);    
    break;
  case 245:
 codegen_emit_undef(bnode);    
    break;
  case 246:
 codegen_emit_undef(bnode);    
    break;
  case 247:
 codegen_emit_undef(bnode);    
    break;
  case 248:
 codegen_emit_undef(bnode);    
    break;
  case 249:
 codegen_emit_undef(bnode);    
    break;
  case 250:
 codegen_emit_getexception(bnode);    
    break;
  case 251:
 codegen_emit_phi(bnode);    
    break;
  case 252:
 codegen_emit_inline_start(bnode);    
    break;
  case 253:
 codegen_emit_inline_end(bnode);    
    break;
  case 254:
 codegen_emit_inline_body(bnode);    
    break;
  case 255:
 codegen_emit_undef(bnode);    
    break;
  case 256:
 codegen_emit_builtin(bnode);    
    break;
  default:    assert (0);
  }
  burm_kids (bnode, ruleNo, kids);
  for (i = 0; nts[i]; i++)
    burm_reduce (kids[i], nts[i]);    /* reduce kids */


  switch (ruleNo) {
  case 1:

    break;
  case 2:
      
    break;
  case 3:
      
    break;
  case 4:
      
    break;
  case 5:
      
    break;
  case 6:
      
    break;
  case 7:
      
    break;
  case 8:
      
    break;
  case 9:
      
    break;
  case 10:
      
    break;
  case 11:
      
    break;
  case 12:
      
    break;
  case 13:
      
    break;
  case 14:
      
    break;
  case 15:
      
    break;
  case 16:
      
    break;
  case 17:
      
    break;
  case 18:
      
    break;
  case 19:
      
    break;
  case 20:
      
    break;
  case 21:
      
    break;
  case 22:
      
    break;
  case 23:
      
    break;
  case 24:
      
    break;
  case 25:
      
    break;
  case 26:
      
    break;
  case 27:
      
    break;
  case 28:
      
    break;
  case 29:
      
    break;
  case 30:
      
    break;
  case 31:
      
    break;
  case 32:
      
    break;
  case 33:
      
    break;
  case 34:
      
    break;
  case 35:
      
    break;
  case 36:
      
    break;
  case 37:
      
    break;
  case 38:
      
    break;
  case 39:
      
    break;
  case 40:
      
    break;
  case 41:
      
    break;
  case 42:
      
    break;
  case 43:
      
    break;
  case 44:
      
    break;
  case 45:
      
    break;
  case 46:
      
    break;
  case 47:
      
    break;
  case 48:
      
    break;
  case 49:
      
    break;
  case 50:
      
    break;
  case 51:
      
    break;
  case 52:
      
    break;
  case 53:
      
    break;
  case 54:
      
    break;
  case 55:
      
    break;
  case 56:
      
    break;
  case 57:
      
    break;
  case 58:
      
    break;
  case 59:
      
    break;
  case 60:
      
    break;
  case 61:
      
    break;
  case 62:
      
    break;
  case 63:
      
    break;
  case 64:
      
    break;
  case 65:
      
    break;
  case 66:
      
    break;
  case 67:
      
    break;
  case 68:
      
    break;
  case 69:
      
    break;
  case 70:
      
    break;
  case 71:
      
    break;
  case 72:
      
    break;
  case 73:
      
    break;
  case 74:
      
    break;
  case 75:
      
    break;
  case 76:
      
    break;
  case 77:
      
    break;
  case 78:
      
    break;
  case 79:
      
    break;
  case 80:
      
    break;
  case 81:
      
    break;
  case 82:
      
    break;
  case 83:
      
    break;
  case 84:
      
    break;
  case 85:
      
    break;
  case 86:
      
    break;
  case 87:
      
    break;
  case 88:
      
    break;
  case 89:
      
    break;
  case 90:
      
    break;
  case 91:
      
    break;
  case 92:
      
    break;
  case 93:
      
    break;
  case 94:
      
    break;
  case 95:
      
    break;
  case 96:
      
    break;
  case 97:
      
    break;
  case 98:
      
    break;
  case 99:
      
    break;
  case 100:
      
    break;
  case 101:
      
    break;
  case 102:
      
    break;
  case 103:
      
    break;
  case 104:
      
    break;
  case 105:
      
    break;
  case 106:
      
    break;
  case 107:
      
    break;
  case 108:
      
    break;
  case 109:
      
    break;
  case 110:
      
    break;
  case 111:
      
    break;
  case 112:
      
    break;
  case 113:
      
    break;
  case 114:
      
    break;
  case 115:
      
    break;
  case 116:
      
    break;
  case 117:
      
    break;
  case 118:
      
    break;
  case 119:
      
    break;
  case 120:
      
    break;
  case 121:
      
    break;
  case 122:
      
    break;
  case 123:
      
    break;
  case 124:
      
    break;
  case 125:
      
    break;
  case 126:
      
    break;
  case 127:
      
    break;
  case 128:
      
    break;
  case 129:
      
    break;
  case 130:
      
    break;
  case 131:
      
    break;
  case 132:
      
    break;
  case 133:
      
    break;
  case 134:
      
    break;
  case 135:
      
    break;
  case 136:
      
    break;
  case 137:
      
    break;
  case 138:
      
    break;
  case 139:
      
    break;
  case 140:
      
    break;
  case 141:
      
    break;
  case 142:
      
    break;
  case 143:
      
    break;
  case 144:
      
    break;
  case 145:
      
    break;
  case 146:
      
    break;
  case 147:
      
    break;
  case 148:
      
    break;
  case 149:
      
    break;
  case 150:
      
    break;
  case 151:
      
    break;
  case 152:
      
    break;
  case 153:
      
    break;
  case 154:
      
    break;
  case 155:
      
    break;
  case 156:
      
    break;
  case 157:
      
    break;
  case 158:
      
    break;
  case 159:
      
    break;
  case 160:
      
    break;
  case 161:
      
    break;
  case 162:
      
    break;
  case 163:
      
    break;
  case 164:
      
    break;
  case 165:
      
    break;
  case 166:
      
    break;
  case 167:
      
    break;
  case 168:
      
    break;
  case 169:
      
    break;
  case 170:
      
    break;
  case 171:
      
    break;
  case 172:
      
    break;
  case 173:
      
    break;
  case 174:
      
    break;
  case 175:
      
    break;
  case 176:
      
    break;
  case 177:
      
    break;
  case 178:
      
    break;
  case 179:
      
    break;
  case 180:
      
    break;
  case 181:
      
    break;
  case 182:
      
    break;
  case 183:
      
    break;
  case 184:
      
    break;
  case 185:
      
    break;
  case 186:
      
    break;
  case 187:
      
    break;
  case 188:
      
    break;
  case 189:
      
    break;
  case 190:
      
    break;
  case 191:
      
    break;
  case 192:
      
    break;
  case 193:
      
    break;
  case 194:
      
    break;
  case 195:
      
    break;
  case 196:
      
    break;
  case 197:
      
    break;
  case 198:
      
    break;
  case 199:
      
    break;
  case 200:
      
    break;
  case 201:
      
    break;
  case 202:
      
    break;
  case 203:
      
    break;
  case 204:
      
    break;
  case 205:
      
    break;
  case 206:
      
    break;
  case 207:
      
    break;
  case 208:
      
    break;
  case 209:
      
    break;
  case 210:
      
    break;
  case 211:
      
    break;
  case 212:
      
    break;
  case 213:
      
    break;
  case 214:
      
    break;
  case 215:
      
    break;
  case 216:
      
    break;
  case 217:
      
    break;
  case 218:
      
    break;
  case 219:
      
    break;
  case 220:
      
    break;
  case 221:
      
    break;
  case 222:
      
    break;
  case 223:
      
    break;
  case 224:
      
    break;
  case 225:
      
    break;
  case 226:
      
    break;
  case 227:
      
    break;
  case 228:
      
    break;
  case 229:
      
    break;
  case 230:
      
    break;
  case 231:
      
    break;
  case 232:
      
    break;
  case 233:
      
    break;
  case 234:
      
    break;
  case 235:
      
    break;
  case 236:
        
    break;
  case 237:
    
    break;
  case 238:
      
    break;
  case 239:
      
    break;
  case 240:
      
    break;
  case 241:
      
    break;
  case 242:
      
    break;
  case 243:
      
    break;
  case 244:
      
    break;
  case 245:
      
    break;
  case 246:
      
    break;
  case 247:
      
    break;
  case 248:
      
    break;
  case 249:
      
    break;
  case 250:
      
    break;
  case 251:
      
    break;
  case 252:
      
    break;
  case 253:
      
    break;
  case 254:
      
    break;
  case 255:
      
    break;
  case 256:
      
    break;
  default:    assert (0);
  }
}
