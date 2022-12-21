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

#define burm_root_NT 1
#define burm_reg_NT 2
#define burm_const_NT 3
#define burm_cmp_NT 4
int burm_max_nt = 4;

char *burm_ntname[] = {
	0,
	"root",
	"reg",
	"const",
	"cmp",
	0
};

struct burm_state {
	int op;
	STATEPTR_TYPE left, right;
	short cost[5];
	struct {
		unsigned burm_root:6;
		unsigned burm_reg:7;
		unsigned burm_const:2;
		unsigned burm_cmp:3;
	} rule;
};

static short burm_nts_0[] = { 0 };
static short burm_nts_1[] = { burm_reg_NT, 0 };
static short burm_nts_2[] = { burm_reg_NT, burm_reg_NT, 0 };
static short burm_nts_3[] = { burm_const_NT, 0 };
static short burm_nts_4[] = { burm_cmp_NT, 0 };
static short burm_nts_5[] = { burm_const_NT, burm_reg_NT, 0 };
static short burm_nts_6[] = { burm_reg_NT, burm_const_NT, 0 };

short *burm_nts[] = {
	0,	/* 0 */
	burm_nts_0,	/* 1 */
	burm_nts_1,	/* 2 */
	burm_nts_2,	/* 3 */
	burm_nts_1,	/* 4 */
	burm_nts_3,	/* 5 */
	burm_nts_4,	/* 6 */
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
	burm_nts_2,	/* 17 */
	burm_nts_2,	/* 18 */
	burm_nts_2,	/* 19 */
	burm_nts_2,	/* 20 */
	burm_nts_2,	/* 21 */
	burm_nts_2,	/* 22 */
	burm_nts_2,	/* 23 */
	burm_nts_2,	/* 24 */
	burm_nts_2,	/* 25 */
	burm_nts_2,	/* 26 */
	burm_nts_2,	/* 27 */
	burm_nts_2,	/* 28 */
	burm_nts_2,	/* 29 */
	burm_nts_1,	/* 30 */
	burm_nts_1,	/* 31 */
	burm_nts_1,	/* 32 */
	burm_nts_1,	/* 33 */
	burm_nts_2,	/* 34 */
	burm_nts_2,	/* 35 */
	burm_nts_2,	/* 36 */
	burm_nts_2,	/* 37 */
	burm_nts_5,	/* 38 */
	burm_nts_5,	/* 39 */
	burm_nts_6,	/* 40 */
	burm_nts_6,	/* 41 */
	burm_nts_2,	/* 42 */
	burm_nts_2,	/* 43 */
	burm_nts_2,	/* 44 */
	burm_nts_2,	/* 45 */
	burm_nts_5,	/* 46 */
	burm_nts_5,	/* 47 */
	burm_nts_6,	/* 48 */
	burm_nts_6,	/* 49 */
	burm_nts_2,	/* 50 */
	burm_nts_2,	/* 51 */
	burm_nts_2,	/* 52 */
	burm_nts_2,	/* 53 */
	burm_nts_5,	/* 54 */
	burm_nts_5,	/* 55 */
	burm_nts_6,	/* 56 */
	burm_nts_6,	/* 57 */
	burm_nts_2,	/* 58 */
	burm_nts_2,	/* 59 */
	burm_nts_2,	/* 60 */
	burm_nts_2,	/* 61 */
	burm_nts_2,	/* 62 */
	burm_nts_2,	/* 63 */
	burm_nts_2,	/* 64 */
	burm_nts_2,	/* 65 */
	burm_nts_2,	/* 66 */
	burm_nts_2,	/* 67 */
	burm_nts_2,	/* 68 */
	burm_nts_2,	/* 69 */
	burm_nts_2,	/* 70 */
	burm_nts_2,	/* 71 */
	burm_nts_2,	/* 72 */
	burm_nts_2,	/* 73 */
	burm_nts_2,	/* 74 */
	burm_nts_2,	/* 75 */
	burm_nts_2,	/* 76 */
	burm_nts_2,	/* 77 */
	burm_nts_6,	/* 78 */
	burm_nts_6,	/* 79 */
	burm_nts_5,	/* 80 */
	burm_nts_5,	/* 81 */
	burm_nts_1,	/* 82 */
	burm_nts_1,	/* 83 */
	burm_nts_1,	/* 84 */
	burm_nts_1,	/* 85 */
	burm_nts_0,	/* 86 */
	burm_nts_0,	/* 87 */
	burm_nts_1,	/* 88 */
	burm_nts_1,	/* 89 */
	burm_nts_1,	/* 90 */
	burm_nts_1,	/* 91 */
	burm_nts_1,	/* 92 */
	burm_nts_1,	/* 93 */
	burm_nts_1,	/* 94 */
	burm_nts_1,	/* 95 */
	burm_nts_1,	/* 96 */
	burm_nts_1,	/* 97 */
	burm_nts_1,	/* 98 */
	burm_nts_1,	/* 99 */
	burm_nts_1,	/* 100 */
	burm_nts_1,	/* 101 */
	burm_nts_1,	/* 102 */
	burm_nts_2,	/* 103 */
	burm_nts_0,	/* 104 */
	burm_nts_1,	/* 105 */
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
	burm_nts_1,	/* 118 */
	burm_nts_1,	/* 119 */
	burm_nts_1,	/* 120 */
	burm_nts_1,	/* 121 */
	burm_nts_1,	/* 122 */
	burm_nts_2,	/* 123 */
	burm_nts_1,	/* 124 */
	burm_nts_0,	/* 125 */
	burm_nts_1,	/* 126 */
	burm_nts_1,	/* 127 */
	burm_nts_1,	/* 128 */
	burm_nts_1,	/* 129 */
	burm_nts_1,	/* 130 */
	burm_nts_3,	/* 131 */
	burm_nts_3,	/* 132 */
	burm_nts_3,	/* 133 */
	burm_nts_3,	/* 134 */
	burm_nts_3,	/* 135 */
	burm_nts_0,	/* 136 */
	burm_nts_1,	/* 137 */
	burm_nts_1,	/* 138 */
	burm_nts_1,	/* 139 */
	burm_nts_1,	/* 140 */
	burm_nts_1,	/* 141 */
	burm_nts_1,	/* 142 */
	burm_nts_1,	/* 143 */
	burm_nts_1,	/* 144 */
	burm_nts_2,	/* 145 */
	burm_nts_2,	/* 146 */
	burm_nts_2,	/* 147 */
	burm_nts_2,	/* 148 */
	burm_nts_2,	/* 149 */
	burm_nts_2,	/* 150 */
	burm_nts_2,	/* 151 */
	burm_nts_2,	/* 152 */
	burm_nts_2,	/* 153 */
	burm_nts_2,	/* 154 */
	burm_nts_2,	/* 155 */
	burm_nts_2,	/* 156 */
	burm_nts_2,	/* 157 */
	burm_nts_2,	/* 158 */
	burm_nts_1,	/* 159 */
	burm_nts_1,	/* 160 */
	burm_nts_0,	/* 161 */
	burm_nts_0,	/* 162 */
	burm_nts_0,	/* 163 */
	burm_nts_2,	/* 164 */
	burm_nts_2,	/* 165 */
	burm_nts_2,	/* 166 */
	burm_nts_2,	/* 167 */
	burm_nts_0,	/* 168 */
	burm_nts_0,	/* 169 */
	burm_nts_2,	/* 170 */
	burm_nts_1,	/* 171 */
	burm_nts_0,	/* 172 */
};

char burm_arity[] = {
	0,	/* 0=NOP */
	0,	/* 1=ACONST */
	1,	/* 2=CHECKNULL */
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
	2,	/* 46=IALOAD */
	2,	/* 47=LALOAD */
	2,	/* 48=FALOAD */
	2,	/* 49=DALOAD */
	2,	/* 50=AALOAD */
	2,	/* 51=BALOAD */
	2,	/* 52=CALOAD */
	2,	/* 53=SALOAD */
	1,	/* 54=ISTORE */
	1,	/* 55=LSTORE */
	1,	/* 56=FSTORE */
	1,	/* 57=DSTORE */
	1,	/* 58=ASTORE */
	0,	/* 59=IF_LEQ */
	0,	/* 60=IF_LNE */
	0,	/* 61=IF_LLT */
	0,	/* 62=IF_LGE */
	0,	/* 63=IF_LGT */
	0,	/* 64=IF_LLE */
	2,	/* 65=IF_LCMPEQ */
	2,	/* 66=IF_LCMPNE */
	2,	/* 67=IF_LCMPLT */
	2,	/* 68=IF_LCMPGE */
	2,	/* 69=IF_LCMPGT */
	2,	/* 70=IF_LCMPLE */
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
	1,	/* 87=POP */
	2,	/* 88=POP2 */
	0,	/* 89=DUP */
	0,	/* 90=DUP_X1 */
	0,	/* 91=DUP_X2 */
	0,	/* 92=DUP2 */
	0,	/* 93=DUP2_X1 */
	0,	/* 94=DUP2_X2 */
	0,	/* 95=SWAP */
	2,	/* 96=IADD */
	2,	/* 97=LADD */
	2,	/* 98=FADD */
	2,	/* 99=DADD */
	2,	/* 100=ISUB */
	2,	/* 101=LSUB */
	2,	/* 102=FSUB */
	2,	/* 103=DSUB */
	2,	/* 104=IMUL */
	2,	/* 105=LMUL */
	2,	/* 106=FMUL */
	2,	/* 107=DMUL */
	2,	/* 108=IDIV */
	2,	/* 109=LDIV */
	2,	/* 110=FDIV */
	2,	/* 111=DDIV */
	2,	/* 112=IREM */
	2,	/* 113=LREM */
	2,	/* 114=FREM */
	2,	/* 115=DREM */
	1,	/* 116=INEG */
	1,	/* 117=LNEG */
	1,	/* 118=FNEG */
	1,	/* 119=DNEG */
	2,	/* 120=ISHL */
	2,	/* 121=LSHL */
	2,	/* 122=ISHR */
	2,	/* 123=LSHR */
	2,	/* 124=IUSHR */
	2,	/* 125=LUSHR */
	2,	/* 126=IAND */
	2,	/* 127=LAND */
	2,	/* 128=IOR */
	2,	/* 129=LOR */
	2,	/* 130=IXOR */
	2,	/* 131=LXOR */
	0,	/* 132=IINC */
	1,	/* 133=I2L */
	1,	/* 134=I2F */
	1,	/* 135=I2D */
	1,	/* 136=L2I */
	1,	/* 137=L2F */
	1,	/* 138=L2D */
	1,	/* 139=F2I */
	1,	/* 140=F2L */
	1,	/* 141=F2D */
	1,	/* 142=D2I */
	1,	/* 143=D2L */
	1,	/* 144=D2F */
	1,	/* 145=INT2BYTE */
	1,	/* 146=INT2CHAR */
	1,	/* 147=INT2SHORT */
	2,	/* 148=LCMP */
	2,	/* 149=FCMPL */
	2,	/* 150=FCMPG */
	2,	/* 151=DCMPL */
	2,	/* 152=DCMPG */
	1,	/* 153=IFEQ */
	1,	/* 154=IFNE */
	1,	/* 155=IFLT */
	1,	/* 156=IFGE */
	1,	/* 157=IFGT */
	1,	/* 158=IFLE */
	2,	/* 159=IF_ICMPEQ */
	2,	/* 160=IF_ICMPNE */
	2,	/* 161=IF_ICMPLT */
	2,	/* 162=IF_ICMPGE */
	2,	/* 163=IF_ICMPGT */
	2,	/* 164=IF_ICMPLE */
	2,	/* 165=IF_ACMPEQ */
	2,	/* 166=IF_ACMPNE */
	0,	/* 167=GOTO */
	0,	/* 168=JSR */
	0,	/* 169=RET */
	1,	/* 170=TABLESWITCH */
	1,	/* 171=LOOKUPSWITCH */
	1,	/* 172=IRETURN */
	1,	/* 173=LRETURN */
	1,	/* 174=FRETURN */
	1,	/* 175=DRETURN */
	1,	/* 176=ARETURN */
	0,	/* 177=RETURN */
	0,	/* 178=GETSTATIC */
	1,	/* 179=PUTSTATIC */
	1,	/* 180=GETFIELD */
	2,	/* 181=PUTFIELD */
	2,	/* 182=INVOKEVIRTUAL */
	2,	/* 183=INVOKESPECIAL */
	2,	/* 184=INVOKESTATIC */
	2,	/* 185=INVOKEINTERFACE */
	0,	/* 186=UNDEF186 */
	0,	/* 187=NEW */
	0,	/* 188=NEWARRAY */
	0,	/* 189=ANEWARRAY */
	1,	/* 190=ARRAYLENGTH */
	1,	/* 191=ATHROW */
	1,	/* 192=CHECKCAST */
	1,	/* 193=INSTANCEOF */
	0,	/* 194=MONITORENTER */
	0,	/* 195=MONITOREXIT */
	0,	/* 196=UNDEF196 */
	2,	/* 197=MULTIANEWARRAY */
	1,	/* 198=IFNULL */
	1,	/* 199=IFNONNULL */
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
	2,	/* 255=BUILTIN */
	0,	/* 256 */
	0,	/* 257 */
	0,	/* 258 */
	0,	/* 259 */
	0,	/* 260 */
	0,	/* 261 */
	0,	/* 262 */
	0,	/* 263 */
	0,	/* 264 */
	0,	/* 265 */
	0,	/* 266 */
	0,	/* 267 */
	0,	/* 268 */
	0,	/* 269 */
	0,	/* 270 */
	0,	/* 271 */
	0,	/* 272 */
	0,	/* 273 */
	0,	/* 274 */
	0,	/* 275 */
	0,	/* 276 */
	0,	/* 277 */
	0,	/* 278 */
	0,	/* 279 */
	0,	/* 280 */
	0,	/* 281 */
	0,	/* 282 */
	0,	/* 283 */
	0,	/* 284 */
	0,	/* 285 */
	0,	/* 286 */
	0,	/* 287 */
	0,	/* 288 */
	0,	/* 289 */
	0,	/* 290 */
	0,	/* 291 */
	0,	/* 292 */
	0,	/* 293 */
	0,	/* 294 */
	0,	/* 295 */
	0,	/* 296 */
	0,	/* 297 */
	0,	/* 298 */
	0,	/* 299 */
	0,	/* 300=RESULT */
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
	/* 256 */	0,
	/* 257 */	0,
	/* 258 */	0,
	/* 259 */	0,
	/* 260 */	0,
	/* 261 */	0,
	/* 262 */	0,
	/* 263 */	0,
	/* 264 */	0,
	/* 265 */	0,
	/* 266 */	0,
	/* 267 */	0,
	/* 268 */	0,
	/* 269 */	0,
	/* 270 */	0,
	/* 271 */	0,
	/* 272 */	0,
	/* 273 */	0,
	/* 274 */	0,
	/* 275 */	0,
	/* 276 */	0,
	/* 277 */	0,
	/* 278 */	0,
	/* 279 */	0,
	/* 280 */	0,
	/* 281 */	0,
	/* 282 */	0,
	/* 283 */	0,
	/* 284 */	0,
	/* 285 */	0,
	/* 286 */	0,
	/* 287 */	0,
	/* 288 */	0,
	/* 289 */	0,
	/* 290 */	0,
	/* 291 */	0,
	/* 292 */	0,
	/* 293 */	0,
	/* 294 */	0,
	/* 295 */	0,
	/* 296 */	0,
	/* 297 */	0,
	/* 298 */	0,
	/* 299 */	0,
	/* 300 */	"RESULT",
};

short burm_cost[][4] = {
	{ 0 },	/* 0 */
	{ 0 },	/* 1 = reg: NOP */
	{ 0 },	/* 2 = root: POP(reg) */
	{ 0 },	/* 3 = root: POP2(reg,reg) */
	{ 0 },	/* 4 = root: reg */
	{ 1 },	/* 5 = reg: const */
	{ 0 },	/* 6 = reg: cmp */
	{ 0 },	/* 7 = const: LCONST */
	{ 0 },	/* 8 = const: ICONST */
	{ 100 },	/* 9 = reg: ACONST */
	{ 100 },	/* 10 = reg: DCONST */
	{ 100 },	/* 11 = reg: FCONST */
	{ 100 },	/* 12 = reg: ALOAD */
	{ 100 },	/* 13 = reg: DLOAD */
	{ 100 },	/* 14 = reg: FLOAD */
	{ 100 },	/* 15 = reg: LLOAD */
	{ 100 },	/* 16 = reg: ILOAD */
	{ 100 },	/* 17 = reg: AALOAD(reg,reg) */
	{ 100 },	/* 18 = reg: BALOAD(reg,reg) */
	{ 100 },	/* 19 = reg: CALOAD(reg,reg) */
	{ 100 },	/* 20 = reg: DALOAD(reg,reg) */
	{ 100 },	/* 21 = reg: FALOAD(reg,reg) */
	{ 100 },	/* 22 = reg: LALOAD(reg,reg) */
	{ 100 },	/* 23 = reg: IALOAD(reg,reg) */
	{ 100 },	/* 24 = reg: SALOAD(reg,reg) */
	{ 100 },	/* 25 = cmp: DCMPG(reg,reg) */
	{ 100 },	/* 26 = cmp: DCMPL(reg,reg) */
	{ 100 },	/* 27 = cmp: FCMPG(reg,reg) */
	{ 100 },	/* 28 = cmp: FCMPL(reg,reg) */
	{ 100 },	/* 29 = cmp: LCMP(reg,reg) */
	{ 100 },	/* 30 = reg: DNEG(reg) */
	{ 100 },	/* 31 = reg: FNEG(reg) */
	{ 100 },	/* 32 = reg: LNEG(reg) */
	{ 100 },	/* 33 = reg: INEG(reg) */
	{ 100 },	/* 34 = reg: DADD(reg,reg) */
	{ 100 },	/* 35 = reg: FADD(reg,reg) */
	{ 100 },	/* 36 = reg: LADD(reg,reg) */
	{ 100 },	/* 37 = reg: IADD(reg,reg) */
	{ 10000 },	/* 38 = reg: LADD(const,reg) */
	{ 10000 },	/* 39 = reg: IADD(const,reg) */
	{ 10000 },	/* 40 = reg: LADD(reg,const) */
	{ 10000 },	/* 41 = reg: IADD(reg,const) */
	{ 100 },	/* 42 = reg: DMUL(reg,reg) */
	{ 100 },	/* 43 = reg: FMUL(reg,reg) */
	{ 100 },	/* 44 = reg: IMUL(reg,reg) */
	{ 100 },	/* 45 = reg: LMUL(reg,reg) */
	{ 10000 },	/* 46 = reg: IMUL(const,reg) */
	{ 10000 },	/* 47 = reg: LMUL(const,reg) */
	{ 10000 },	/* 48 = reg: IMUL(reg,const) */
	{ 10000 },	/* 49 = reg: LMUL(reg,const) */
	{ 100 },	/* 50 = reg: DREM(reg,reg) */
	{ 100 },	/* 51 = reg: FREM(reg,reg) */
	{ 100 },	/* 52 = reg: IREM(reg,reg) */
	{ 100 },	/* 53 = reg: LREM(reg,reg) */
	{ 10000 },	/* 54 = reg: IREM(const,reg) */
	{ 10000 },	/* 55 = reg: LREM(const,reg) */
	{ 10000 },	/* 56 = reg: IREM(reg,const) */
	{ 10000 },	/* 57 = reg: LREM(reg,const) */
	{ 100 },	/* 58 = reg: DSUB(reg,reg) */
	{ 100 },	/* 59 = reg: FSUB(reg,reg) */
	{ 100 },	/* 60 = reg: ISUB(reg,reg) */
	{ 100 },	/* 61 = reg: LSUB(reg,reg) */
	{ 100 },	/* 62 = reg: DDIV(reg,reg) */
	{ 100 },	/* 63 = reg: FDIV(reg,reg) */
	{ 100 },	/* 64 = reg: LDIV(reg,reg) */
	{ 100 },	/* 65 = reg: IDIV(reg,reg) */
	{ 100 },	/* 66 = reg: IAND(reg,reg) */
	{ 100 },	/* 67 = reg: LAND(reg,reg) */
	{ 100 },	/* 68 = reg: IOR(reg,reg) */
	{ 100 },	/* 69 = reg: LOR(reg,reg) */
	{ 100 },	/* 70 = reg: IXOR(reg,reg) */
	{ 100 },	/* 71 = reg: LXOR(reg,reg) */
	{ 100 },	/* 72 = reg: ISHL(reg,reg) */
	{ 100 },	/* 73 = reg: ISHR(reg,reg) */
	{ 100 },	/* 74 = reg: LSHL(reg,reg) */
	{ 100 },	/* 75 = reg: LSHR(reg,reg) */
	{ 100 },	/* 76 = reg: IUSHR(reg,reg) */
	{ 100 },	/* 77 = reg: LUSHR(reg,reg) */
	{ 10000 },	/* 78 = reg: IUSHR(reg,const) */
	{ 10000 },	/* 79 = reg: LUSHR(reg,const) */
	{ 10000 },	/* 80 = reg: IUSHR(const,reg) */
	{ 10000 },	/* 81 = reg: LUSHR(const,reg) */
	{ 100 },	/* 82 = reg: ARRAYLENGTH(reg) */
	{ 100 },	/* 83 = reg: CHECKCAST(reg) */
	{ 100 },	/* 84 = reg: INSTANCEOF(reg) */
	{ 100 },	/* 85 = reg: CHECKNULL(reg) */
	{ 100 },	/* 86 = reg: COPY */
	{ 100 },	/* 87 = reg: MOVE */
	{ 100 },	/* 88 = reg: D2F(reg) */
	{ 100 },	/* 89 = reg: D2I(reg) */
	{ 100 },	/* 90 = reg: D2L(reg) */
	{ 100 },	/* 91 = reg: F2D(reg) */
	{ 100 },	/* 92 = reg: F2I(reg) */
	{ 100 },	/* 93 = reg: F2L(reg) */
	{ 100 },	/* 94 = reg: I2D(reg) */
	{ 100 },	/* 95 = reg: I2F(reg) */
	{ 100 },	/* 96 = reg: I2L(reg) */
	{ 100 },	/* 97 = reg: INT2BYTE(reg) */
	{ 100 },	/* 98 = reg: INT2CHAR(reg) */
	{ 100 },	/* 99 = reg: INT2SHORT(reg) */
	{ 100 },	/* 100 = reg: L2D(reg) */
	{ 100 },	/* 101 = reg: L2F(reg) */
	{ 100 },	/* 102 = reg: L2I(reg) */
	{ 100 },	/* 103 = reg: MULTIANEWARRAY(reg,reg) */
	{ 100 },	/* 104 = reg: GETEXCEPTION */
	{ 100 },	/* 105 = reg: GETFIELD(reg) */
	{ 100 },	/* 106 = reg: GETSTATIC */
	{ 100 },	/* 107 = reg: JSR */
	{ 100 },	/* 108 = reg: PHI */
	{ 100 },	/* 109 = reg: RESULT */
	{ 100 },	/* 110 = root: AASTORE */
	{ 100 },	/* 111 = root: BASTORE */
	{ 100 },	/* 112 = root: CASTORE */
	{ 100 },	/* 113 = root: DASTORE */
	{ 100 },	/* 114 = root: FASTORE */
	{ 100 },	/* 115 = root: IASTORE */
	{ 100 },	/* 116 = root: LASTORE */
	{ 100 },	/* 117 = root: SASTORE */
	{ 100 },	/* 118 = root: ASTORE(reg) */
	{ 100 },	/* 119 = root: DSTORE(reg) */
	{ 100 },	/* 120 = root: FSTORE(reg) */
	{ 100 },	/* 121 = root: ISTORE(reg) */
	{ 100 },	/* 122 = root: LSTORE(reg) */
	{ 100 },	/* 123 = root: PUTFIELD(reg,reg) */
	{ 100 },	/* 124 = root: PUTSTATIC(reg) */
	{ 100 },	/* 125 = root: IINC */
	{ 100 },	/* 126 = root: ARETURN(reg) */
	{ 100 },	/* 127 = root: DRETURN(reg) */
	{ 100 },	/* 128 = root: FRETURN(reg) */
	{ 100 },	/* 129 = root: IRETURN(reg) */
	{ 100 },	/* 130 = root: LRETURN(reg) */
	{ 10000 },	/* 131 = root: ARETURN(const) */
	{ 10000 },	/* 132 = root: DRETURN(const) */
	{ 10000 },	/* 133 = root: FRETURN(const) */
	{ 10000 },	/* 134 = root: IRETURN(const) */
	{ 10000 },	/* 135 = root: LRETURN(const) */
	{ 100 },	/* 136 = root: RETURN */
	{ 100 },	/* 137 = root: IFEQ(reg) */
	{ 100 },	/* 138 = root: IFGE(reg) */
	{ 100 },	/* 139 = root: IFGT(reg) */
	{ 100 },	/* 140 = root: IFLE(reg) */
	{ 100 },	/* 141 = root: IFLT(reg) */
	{ 100 },	/* 142 = root: IFNE(reg) */
	{ 100 },	/* 143 = root: IFNONNULL(reg) */
	{ 100 },	/* 144 = root: IFNULL(reg) */
	{ 100 },	/* 145 = root: IF_ACMPEQ(reg,reg) */
	{ 100 },	/* 146 = root: IF_ACMPNE(reg,reg) */
	{ 100 },	/* 147 = root: IF_ICMPEQ(reg,reg) */
	{ 100 },	/* 148 = root: IF_ICMPGE(reg,reg) */
	{ 100 },	/* 149 = root: IF_ICMPGT(reg,reg) */
	{ 100 },	/* 150 = root: IF_ICMPLE(reg,reg) */
	{ 100 },	/* 151 = root: IF_ICMPLT(reg,reg) */
	{ 100 },	/* 152 = root: IF_ICMPNE(reg,reg) */
	{ 100 },	/* 153 = root: IF_LCMPEQ(reg,reg) */
	{ 100 },	/* 154 = root: IF_LCMPGE(reg,reg) */
	{ 100 },	/* 155 = root: IF_LCMPGT(reg,reg) */
	{ 100 },	/* 156 = root: IF_LCMPLE(reg,reg) */
	{ 100 },	/* 157 = root: IF_LCMPLT(reg,reg) */
	{ 100 },	/* 158 = root: IF_LCMPNE(reg,reg) */
	{ 100 },	/* 159 = root: LOOKUPSWITCH(reg) */
	{ 100 },	/* 160 = root: TABLESWITCH(reg) */
	{ 100 },	/* 161 = root: INLINE_BODY */
	{ 100 },	/* 162 = root: INLINE_END */
	{ 100 },	/* 163 = root: INLINE_START */
	{ 100 },	/* 164 = reg: INVOKEINTERFACE(reg,reg) */
	{ 100 },	/* 165 = reg: INVOKESPECIAL(reg,reg) */
	{ 100 },	/* 166 = reg: INVOKESTATIC(reg,reg) */
	{ 100 },	/* 167 = reg: INVOKEVIRTUAL(reg,reg) */
	{ 100 },	/* 168 = root: GOTO */
	{ 100 },	/* 169 = root: RET */
	{ 100 },	/* 170 = reg: BUILTIN(reg,reg) */
	{ 100 },	/* 171 = root: ATHROW(reg) */
	{ 100 },	/* 172 = root: BREAKPOINT */
};

char *burm_string[] = {
	/* 0 */	0,
	/* 1 */	"reg: NOP",
	/* 2 */	"root: POP(reg)",
	/* 3 */	"root: POP2(reg,reg)",
	/* 4 */	"root: reg",
	/* 5 */	"reg: const",
	/* 6 */	"reg: cmp",
	/* 7 */	"const: LCONST",
	/* 8 */	"const: ICONST",
	/* 9 */	"reg: ACONST",
	/* 10 */	"reg: DCONST",
	/* 11 */	"reg: FCONST",
	/* 12 */	"reg: ALOAD",
	/* 13 */	"reg: DLOAD",
	/* 14 */	"reg: FLOAD",
	/* 15 */	"reg: LLOAD",
	/* 16 */	"reg: ILOAD",
	/* 17 */	"reg: AALOAD(reg,reg)",
	/* 18 */	"reg: BALOAD(reg,reg)",
	/* 19 */	"reg: CALOAD(reg,reg)",
	/* 20 */	"reg: DALOAD(reg,reg)",
	/* 21 */	"reg: FALOAD(reg,reg)",
	/* 22 */	"reg: LALOAD(reg,reg)",
	/* 23 */	"reg: IALOAD(reg,reg)",
	/* 24 */	"reg: SALOAD(reg,reg)",
	/* 25 */	"cmp: DCMPG(reg,reg)",
	/* 26 */	"cmp: DCMPL(reg,reg)",
	/* 27 */	"cmp: FCMPG(reg,reg)",
	/* 28 */	"cmp: FCMPL(reg,reg)",
	/* 29 */	"cmp: LCMP(reg,reg)",
	/* 30 */	"reg: DNEG(reg)",
	/* 31 */	"reg: FNEG(reg)",
	/* 32 */	"reg: LNEG(reg)",
	/* 33 */	"reg: INEG(reg)",
	/* 34 */	"reg: DADD(reg,reg)",
	/* 35 */	"reg: FADD(reg,reg)",
	/* 36 */	"reg: LADD(reg,reg)",
	/* 37 */	"reg: IADD(reg,reg)",
	/* 38 */	"reg: LADD(const,reg)",
	/* 39 */	"reg: IADD(const,reg)",
	/* 40 */	"reg: LADD(reg,const)",
	/* 41 */	"reg: IADD(reg,const)",
	/* 42 */	"reg: DMUL(reg,reg)",
	/* 43 */	"reg: FMUL(reg,reg)",
	/* 44 */	"reg: IMUL(reg,reg)",
	/* 45 */	"reg: LMUL(reg,reg)",
	/* 46 */	"reg: IMUL(const,reg)",
	/* 47 */	"reg: LMUL(const,reg)",
	/* 48 */	"reg: IMUL(reg,const)",
	/* 49 */	"reg: LMUL(reg,const)",
	/* 50 */	"reg: DREM(reg,reg)",
	/* 51 */	"reg: FREM(reg,reg)",
	/* 52 */	"reg: IREM(reg,reg)",
	/* 53 */	"reg: LREM(reg,reg)",
	/* 54 */	"reg: IREM(const,reg)",
	/* 55 */	"reg: LREM(const,reg)",
	/* 56 */	"reg: IREM(reg,const)",
	/* 57 */	"reg: LREM(reg,const)",
	/* 58 */	"reg: DSUB(reg,reg)",
	/* 59 */	"reg: FSUB(reg,reg)",
	/* 60 */	"reg: ISUB(reg,reg)",
	/* 61 */	"reg: LSUB(reg,reg)",
	/* 62 */	"reg: DDIV(reg,reg)",
	/* 63 */	"reg: FDIV(reg,reg)",
	/* 64 */	"reg: LDIV(reg,reg)",
	/* 65 */	"reg: IDIV(reg,reg)",
	/* 66 */	"reg: IAND(reg,reg)",
	/* 67 */	"reg: LAND(reg,reg)",
	/* 68 */	"reg: IOR(reg,reg)",
	/* 69 */	"reg: LOR(reg,reg)",
	/* 70 */	"reg: IXOR(reg,reg)",
	/* 71 */	"reg: LXOR(reg,reg)",
	/* 72 */	"reg: ISHL(reg,reg)",
	/* 73 */	"reg: ISHR(reg,reg)",
	/* 74 */	"reg: LSHL(reg,reg)",
	/* 75 */	"reg: LSHR(reg,reg)",
	/* 76 */	"reg: IUSHR(reg,reg)",
	/* 77 */	"reg: LUSHR(reg,reg)",
	/* 78 */	"reg: IUSHR(reg,const)",
	/* 79 */	"reg: LUSHR(reg,const)",
	/* 80 */	"reg: IUSHR(const,reg)",
	/* 81 */	"reg: LUSHR(const,reg)",
	/* 82 */	"reg: ARRAYLENGTH(reg)",
	/* 83 */	"reg: CHECKCAST(reg)",
	/* 84 */	"reg: INSTANCEOF(reg)",
	/* 85 */	"reg: CHECKNULL(reg)",
	/* 86 */	"reg: COPY",
	/* 87 */	"reg: MOVE",
	/* 88 */	"reg: D2F(reg)",
	/* 89 */	"reg: D2I(reg)",
	/* 90 */	"reg: D2L(reg)",
	/* 91 */	"reg: F2D(reg)",
	/* 92 */	"reg: F2I(reg)",
	/* 93 */	"reg: F2L(reg)",
	/* 94 */	"reg: I2D(reg)",
	/* 95 */	"reg: I2F(reg)",
	/* 96 */	"reg: I2L(reg)",
	/* 97 */	"reg: INT2BYTE(reg)",
	/* 98 */	"reg: INT2CHAR(reg)",
	/* 99 */	"reg: INT2SHORT(reg)",
	/* 100 */	"reg: L2D(reg)",
	/* 101 */	"reg: L2F(reg)",
	/* 102 */	"reg: L2I(reg)",
	/* 103 */	"reg: MULTIANEWARRAY(reg,reg)",
	/* 104 */	"reg: GETEXCEPTION",
	/* 105 */	"reg: GETFIELD(reg)",
	/* 106 */	"reg: GETSTATIC",
	/* 107 */	"reg: JSR",
	/* 108 */	"reg: PHI",
	/* 109 */	"reg: RESULT",
	/* 110 */	"root: AASTORE",
	/* 111 */	"root: BASTORE",
	/* 112 */	"root: CASTORE",
	/* 113 */	"root: DASTORE",
	/* 114 */	"root: FASTORE",
	/* 115 */	"root: IASTORE",
	/* 116 */	"root: LASTORE",
	/* 117 */	"root: SASTORE",
	/* 118 */	"root: ASTORE(reg)",
	/* 119 */	"root: DSTORE(reg)",
	/* 120 */	"root: FSTORE(reg)",
	/* 121 */	"root: ISTORE(reg)",
	/* 122 */	"root: LSTORE(reg)",
	/* 123 */	"root: PUTFIELD(reg,reg)",
	/* 124 */	"root: PUTSTATIC(reg)",
	/* 125 */	"root: IINC",
	/* 126 */	"root: ARETURN(reg)",
	/* 127 */	"root: DRETURN(reg)",
	/* 128 */	"root: FRETURN(reg)",
	/* 129 */	"root: IRETURN(reg)",
	/* 130 */	"root: LRETURN(reg)",
	/* 131 */	"root: ARETURN(const)",
	/* 132 */	"root: DRETURN(const)",
	/* 133 */	"root: FRETURN(const)",
	/* 134 */	"root: IRETURN(const)",
	/* 135 */	"root: LRETURN(const)",
	/* 136 */	"root: RETURN",
	/* 137 */	"root: IFEQ(reg)",
	/* 138 */	"root: IFGE(reg)",
	/* 139 */	"root: IFGT(reg)",
	/* 140 */	"root: IFLE(reg)",
	/* 141 */	"root: IFLT(reg)",
	/* 142 */	"root: IFNE(reg)",
	/* 143 */	"root: IFNONNULL(reg)",
	/* 144 */	"root: IFNULL(reg)",
	/* 145 */	"root: IF_ACMPEQ(reg,reg)",
	/* 146 */	"root: IF_ACMPNE(reg,reg)",
	/* 147 */	"root: IF_ICMPEQ(reg,reg)",
	/* 148 */	"root: IF_ICMPGE(reg,reg)",
	/* 149 */	"root: IF_ICMPGT(reg,reg)",
	/* 150 */	"root: IF_ICMPLE(reg,reg)",
	/* 151 */	"root: IF_ICMPLT(reg,reg)",
	/* 152 */	"root: IF_ICMPNE(reg,reg)",
	/* 153 */	"root: IF_LCMPEQ(reg,reg)",
	/* 154 */	"root: IF_LCMPGE(reg,reg)",
	/* 155 */	"root: IF_LCMPGT(reg,reg)",
	/* 156 */	"root: IF_LCMPLE(reg,reg)",
	/* 157 */	"root: IF_LCMPLT(reg,reg)",
	/* 158 */	"root: IF_LCMPNE(reg,reg)",
	/* 159 */	"root: LOOKUPSWITCH(reg)",
	/* 160 */	"root: TABLESWITCH(reg)",
	/* 161 */	"root: INLINE_BODY",
	/* 162 */	"root: INLINE_END",
	/* 163 */	"root: INLINE_START",
	/* 164 */	"reg: INVOKEINTERFACE(reg,reg)",
	/* 165 */	"reg: INVOKESPECIAL(reg,reg)",
	/* 166 */	"reg: INVOKESTATIC(reg,reg)",
	/* 167 */	"reg: INVOKEVIRTUAL(reg,reg)",
	/* 168 */	"root: GOTO",
	/* 169 */	"root: RET",
	/* 170 */	"reg: BUILTIN(reg,reg)",
	/* 171 */	"root: ATHROW(reg)",
	/* 172 */	"root: BREAKPOINT",
};

static short burm_decode_root[] = {
	0,
	2,
	3,
	4,
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
	168,
	169,
	171,
	172,
};

static short burm_decode_reg[] = {
	0,
	1,
	5,
	6,
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
	164,
	165,
	166,
	167,
	170,
};

static short burm_decode_const[] = {
	0,
	7,
	8,
};

static short burm_decode_cmp[] = {
	0,
	25,
	26,
	27,
	28,
	29,
};

int burm_rule(STATEPTR_TYPE state, int goalnt) {
	burm_assert(goalnt >= 1 && goalnt <= 4, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	if (!state)
		return 0;
	switch (goalnt) {
	case burm_root_NT:
		return burm_decode_root[state->rule.burm_root];
	case burm_reg_NT:
		return burm_decode_reg[state->rule.burm_reg];
	case burm_const_NT:
		return burm_decode_const[state->rule.burm_const];
	case burm_cmp_NT:
		return burm_decode_cmp[state->rule.burm_cmp];
	default:
		burm_assert(0, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	}
	return 0;
}

static void burm_closure_reg(STATEPTR_TYPE, int);
static void burm_closure_const(STATEPTR_TYPE, int);
static void burm_closure_cmp(STATEPTR_TYPE, int);

static void burm_closure_reg(STATEPTR_TYPE p, int c) {
	if (c + 0 < p->cost[burm_root_NT]) {
		p->cost[burm_root_NT] = c + 0;
		p->rule.burm_root = 3;
	}
}

static void burm_closure_const(STATEPTR_TYPE p, int c) {
	if (c + 1 < p->cost[burm_reg_NT]) {
		p->cost[burm_reg_NT] = c + 1;
		p->rule.burm_reg = 2;
		burm_closure_reg(p, c + 1);
	}
}

static void burm_closure_cmp(STATEPTR_TYPE p, int c) {
	if (c + 0 < p->cost[burm_reg_NT]) {
		p->cost[burm_reg_NT] = c + 0;
		p->rule.burm_reg = 3;
		burm_closure_reg(p, c + 0);
	}
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
		p->rule.burm_root = 0;
		p->cost[1] =
		p->cost[2] =
		p->cost[3] =
		p->cost[4] =
			32767;
	}
	switch (op) {
	case 0: /* NOP */
		{
			static struct burm_state z = { 0, 0, 0,
				{	0,
					0,	/* root: reg */
					0,	/* reg: NOP */
					32767,
					32767,
				},{
					3,	/* root: reg */
					1,	/* reg: NOP */
					0,
					0,
				}
			};
			return &z;
		}
	case 1: /* ACONST */
		{
			static struct burm_state z = { 1, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: ACONST */
					32767,
					32767,
				},{
					3,	/* root: reg */
					4,	/* reg: ACONST */
					0,
					0,
				}
			};
			return &z;
		}
	case 2: /* CHECKNULL */
		assert(l);
		{	/* reg: CHECKNULL(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 75;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 3: /* ICONST */
		{
			static struct burm_state z = { 3, 0, 0,
				{	0,
					1,	/* root: reg */
					1,	/* reg: const */
					0,	/* const: ICONST */
					32767,
				},{
					3,	/* root: reg */
					2,	/* reg: const */
					2,	/* const: ICONST */
					0,
				}
			};
			return &z;
		}
	case 4: /* UNDEF4 */
		{
			static struct burm_state z = { 4, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 5: /* IDIVPOW2 */
		{
			static struct burm_state z = { 5, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 6: /* LDIVPOW2 */
		{
			static struct burm_state z = { 6, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 7: /* UNDEF7 */
		{
			static struct burm_state z = { 7, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 8: /* UNDEF8 */
		{
			static struct burm_state z = { 8, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 9: /* LCONST */
		{
			static struct burm_state z = { 9, 0, 0,
				{	0,
					1,	/* root: reg */
					1,	/* reg: const */
					0,	/* const: LCONST */
					32767,
				},{
					3,	/* root: reg */
					2,	/* reg: const */
					1,	/* const: LCONST */
					0,
				}
			};
			return &z;
		}
	case 10: /* LCMPCONST */
		{
			static struct burm_state z = { 10, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 11: /* FCONST */
		{
			static struct burm_state z = { 11, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: FCONST */
					32767,
					32767,
				},{
					3,	/* root: reg */
					6,	/* reg: FCONST */
					0,
					0,
				}
			};
			return &z;
		}
	case 12: /* UNDEF12 */
		{
			static struct burm_state z = { 12, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 13: /* UNDEF13 */
		{
			static struct burm_state z = { 13, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 14: /* DCONST */
		{
			static struct burm_state z = { 14, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: DCONST */
					32767,
					32767,
				},{
					3,	/* root: reg */
					5,	/* reg: DCONST */
					0,
					0,
				}
			};
			return &z;
		}
	case 15: /* COPY */
		{
			static struct burm_state z = { 15, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: COPY */
					32767,
					32767,
				},{
					3,	/* root: reg */
					76,	/* reg: COPY */
					0,
					0,
				}
			};
			return &z;
		}
	case 16: /* MOVE */
		{
			static struct burm_state z = { 16, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: MOVE */
					32767,
					32767,
				},{
					3,	/* root: reg */
					77,	/* reg: MOVE */
					0,
					0,
				}
			};
			return &z;
		}
	case 17: /* UNDEF17 */
		{
			static struct burm_state z = { 17, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 18: /* UNDEF18 */
		{
			static struct burm_state z = { 18, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 19: /* UNDEF19 */
		{
			static struct burm_state z = { 19, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 20: /* UNDEF20 */
		{
			static struct burm_state z = { 20, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 21: /* ILOAD */
		{
			static struct burm_state z = { 21, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: ILOAD */
					32767,
					32767,
				},{
					3,	/* root: reg */
					11,	/* reg: ILOAD */
					0,
					0,
				}
			};
			return &z;
		}
	case 22: /* LLOAD */
		{
			static struct burm_state z = { 22, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: LLOAD */
					32767,
					32767,
				},{
					3,	/* root: reg */
					10,	/* reg: LLOAD */
					0,
					0,
				}
			};
			return &z;
		}
	case 23: /* FLOAD */
		{
			static struct burm_state z = { 23, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: FLOAD */
					32767,
					32767,
				},{
					3,	/* root: reg */
					9,	/* reg: FLOAD */
					0,
					0,
				}
			};
			return &z;
		}
	case 24: /* DLOAD */
		{
			static struct burm_state z = { 24, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: DLOAD */
					32767,
					32767,
				},{
					3,	/* root: reg */
					8,	/* reg: DLOAD */
					0,
					0,
				}
			};
			return &z;
		}
	case 25: /* ALOAD */
		{
			static struct burm_state z = { 25, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: ALOAD */
					32767,
					32767,
				},{
					3,	/* root: reg */
					7,	/* reg: ALOAD */
					0,
					0,
				}
			};
			return &z;
		}
	case 26: /* IADDCONST */
		{
			static struct burm_state z = { 26, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 27: /* ISUBCONST */
		{
			static struct burm_state z = { 27, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 28: /* IMULCONST */
		{
			static struct burm_state z = { 28, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 29: /* IANDCONST */
		{
			static struct burm_state z = { 29, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 30: /* IORCONST */
		{
			static struct burm_state z = { 30, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 31: /* IXORCONST */
		{
			static struct burm_state z = { 31, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 32: /* ISHLCONST */
		{
			static struct burm_state z = { 32, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 33: /* ISHRCONST */
		{
			static struct burm_state z = { 33, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 34: /* IUSHRCONST */
		{
			static struct burm_state z = { 34, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 35: /* IREMPOW2 */
		{
			static struct burm_state z = { 35, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 36: /* LADDCONST */
		{
			static struct burm_state z = { 36, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 37: /* LSUBCONST */
		{
			static struct burm_state z = { 37, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 38: /* LMULCONST */
		{
			static struct burm_state z = { 38, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 39: /* LANDCONST */
		{
			static struct burm_state z = { 39, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 40: /* LORCONST */
		{
			static struct burm_state z = { 40, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 41: /* LXORCONST */
		{
			static struct burm_state z = { 41, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 42: /* LSHLCONST */
		{
			static struct burm_state z = { 42, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 43: /* LSHRCONST */
		{
			static struct burm_state z = { 43, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 44: /* LUSHRCONST */
		{
			static struct burm_state z = { 44, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 45: /* LREMPOW2 */
		{
			static struct burm_state z = { 45, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 46: /* IALOAD */
		assert(l && r);
		{	/* reg: IALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 18;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 47: /* LALOAD */
		assert(l && r);
		{	/* reg: LALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 17;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 48: /* FALOAD */
		assert(l && r);
		{	/* reg: FALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 16;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 49: /* DALOAD */
		assert(l && r);
		{	/* reg: DALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 15;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 50: /* AALOAD */
		assert(l && r);
		{	/* reg: AALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 12;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 51: /* BALOAD */
		assert(l && r);
		{	/* reg: BALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 13;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 52: /* CALOAD */
		assert(l && r);
		{	/* reg: CALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 14;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 53: /* SALOAD */
		assert(l && r);
		{	/* reg: SALOAD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 19;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 54: /* ISTORE */
		assert(l);
		{	/* root: ISTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 15;
			}
		}
		break;
	case 55: /* LSTORE */
		assert(l);
		{	/* root: LSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 16;
			}
		}
		break;
	case 56: /* FSTORE */
		assert(l);
		{	/* root: FSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 14;
			}
		}
		break;
	case 57: /* DSTORE */
		assert(l);
		{	/* root: DSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 13;
			}
		}
		break;
	case 58: /* ASTORE */
		assert(l);
		{	/* root: ASTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 12;
			}
		}
		break;
	case 59: /* IF_LEQ */
		{
			static struct burm_state z = { 59, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 60: /* IF_LNE */
		{
			static struct burm_state z = { 60, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 61: /* IF_LLT */
		{
			static struct burm_state z = { 61, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 62: /* IF_LGE */
		{
			static struct burm_state z = { 62, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 63: /* IF_LGT */
		{
			static struct burm_state z = { 63, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 64: /* IF_LLE */
		{
			static struct burm_state z = { 64, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 65: /* IF_LCMPEQ */
		assert(l && r);
		{	/* root: IF_LCMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 47;
			}
		}
		break;
	case 66: /* IF_LCMPNE */
		assert(l && r);
		{	/* root: IF_LCMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 52;
			}
		}
		break;
	case 67: /* IF_LCMPLT */
		assert(l && r);
		{	/* root: IF_LCMPLT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 51;
			}
		}
		break;
	case 68: /* IF_LCMPGE */
		assert(l && r);
		{	/* root: IF_LCMPGE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 48;
			}
		}
		break;
	case 69: /* IF_LCMPGT */
		assert(l && r);
		{	/* root: IF_LCMPGT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 49;
			}
		}
		break;
	case 70: /* IF_LCMPLE */
		assert(l && r);
		{	/* root: IF_LCMPLE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 50;
			}
		}
		break;
	case 71: /* UNDEF71 */
		{
			static struct burm_state z = { 71, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 72: /* UNDEF72 */
		{
			static struct burm_state z = { 72, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 73: /* UNDEF73 */
		{
			static struct burm_state z = { 73, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 74: /* UNDEF74 */
		{
			static struct burm_state z = { 74, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 75: /* UNDEF75 */
		{
			static struct burm_state z = { 75, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 76: /* UNDEF76 */
		{
			static struct burm_state z = { 76, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 77: /* UNDEF77 */
		{
			static struct burm_state z = { 77, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 78: /* UNDEF78 */
		{
			static struct burm_state z = { 78, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 79: /* IASTORE */
		{
			static struct burm_state z = { 79, 0, 0,
				{	0,
					100,	/* root: IASTORE */
					32767,
					32767,
					32767,
				},{
					9,	/* root: IASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 80: /* LASTORE */
		{
			static struct burm_state z = { 80, 0, 0,
				{	0,
					100,	/* root: LASTORE */
					32767,
					32767,
					32767,
				},{
					10,	/* root: LASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 81: /* FASTORE */
		{
			static struct burm_state z = { 81, 0, 0,
				{	0,
					100,	/* root: FASTORE */
					32767,
					32767,
					32767,
				},{
					8,	/* root: FASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 82: /* DASTORE */
		{
			static struct burm_state z = { 82, 0, 0,
				{	0,
					100,	/* root: DASTORE */
					32767,
					32767,
					32767,
				},{
					7,	/* root: DASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 83: /* AASTORE */
		{
			static struct burm_state z = { 83, 0, 0,
				{	0,
					100,	/* root: AASTORE */
					32767,
					32767,
					32767,
				},{
					4,	/* root: AASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 84: /* BASTORE */
		{
			static struct burm_state z = { 84, 0, 0,
				{	0,
					100,	/* root: BASTORE */
					32767,
					32767,
					32767,
				},{
					5,	/* root: BASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 85: /* CASTORE */
		{
			static struct burm_state z = { 85, 0, 0,
				{	0,
					100,	/* root: CASTORE */
					32767,
					32767,
					32767,
				},{
					6,	/* root: CASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 86: /* SASTORE */
		{
			static struct burm_state z = { 86, 0, 0,
				{	0,
					100,	/* root: SASTORE */
					32767,
					32767,
					32767,
				},{
					11,	/* root: SASTORE */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 87: /* POP */
		assert(l);
		{	/* root: POP(reg) */
			c = l->cost[burm_reg_NT] + 0;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 1;
			}
		}
		break;
	case 88: /* POP2 */
		assert(l && r);
		{	/* root: POP2(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 0;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 2;
			}
		}
		break;
	case 89: /* DUP */
		{
			static struct burm_state z = { 89, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 90: /* DUP_X1 */
		{
			static struct burm_state z = { 90, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 91: /* DUP_X2 */
		{
			static struct burm_state z = { 91, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 92: /* DUP2 */
		{
			static struct burm_state z = { 92, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 93: /* DUP2_X1 */
		{
			static struct burm_state z = { 93, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 94: /* DUP2_X2 */
		{
			static struct burm_state z = { 94, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 95: /* SWAP */
		{
			static struct burm_state z = { 95, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 96: /* IADD */
		assert(l && r);
		{	/* reg: IADD(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 31;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IADD(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 29;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 27;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 97: /* LADD */
		assert(l && r);
		{	/* reg: LADD(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 30;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LADD(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 28;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 26;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 98: /* FADD */
		assert(l && r);
		{	/* reg: FADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 25;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 99: /* DADD */
		assert(l && r);
		{	/* reg: DADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 24;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 100: /* ISUB */
		assert(l && r);
		{	/* reg: ISUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 50;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 101: /* LSUB */
		assert(l && r);
		{	/* reg: LSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 51;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 102: /* FSUB */
		assert(l && r);
		{	/* reg: FSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 49;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 103: /* DSUB */
		assert(l && r);
		{	/* reg: DSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 48;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 104: /* IMUL */
		assert(l && r);
		{	/* reg: IMUL(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 38;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IMUL(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 36;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 34;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 105: /* LMUL */
		assert(l && r);
		{	/* reg: LMUL(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 39;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LMUL(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 37;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 35;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 106: /* FMUL */
		assert(l && r);
		{	/* reg: FMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 33;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 107: /* DMUL */
		assert(l && r);
		{	/* reg: DMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 32;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 108: /* IDIV */
		assert(l && r);
		{	/* reg: IDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 55;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 109: /* LDIV */
		assert(l && r);
		{	/* reg: LDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 54;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 110: /* FDIV */
		assert(l && r);
		{	/* reg: FDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 53;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 111: /* DDIV */
		assert(l && r);
		{	/* reg: DDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 52;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 112: /* IREM */
		assert(l && r);
		{	/* reg: IREM(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 46;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IREM(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 44;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 42;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 113: /* LREM */
		assert(l && r);
		{	/* reg: LREM(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 47;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LREM(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 45;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 43;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 114: /* FREM */
		assert(l && r);
		{	/* reg: FREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 41;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 115: /* DREM */
		assert(l && r);
		{	/* reg: DREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 40;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 116: /* INEG */
		assert(l);
		{	/* reg: INEG(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 23;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 117: /* LNEG */
		assert(l);
		{	/* reg: LNEG(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 22;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 118: /* FNEG */
		assert(l);
		{	/* reg: FNEG(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 21;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 119: /* DNEG */
		assert(l);
		{	/* reg: DNEG(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 20;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 120: /* ISHL */
		assert(l && r);
		{	/* reg: ISHL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 62;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 121: /* LSHL */
		assert(l && r);
		{	/* reg: LSHL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 64;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 122: /* ISHR */
		assert(l && r);
		{	/* reg: ISHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 63;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 123: /* LSHR */
		assert(l && r);
		{	/* reg: LSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 65;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 124: /* IUSHR */
		assert(l && r);
		{	/* reg: IUSHR(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 70;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IUSHR(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 68;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 66;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 125: /* LUSHR */
		assert(l && r);
		{	/* reg: LUSHR(const,reg) */
			c = l->cost[burm_const_NT] + r->cost[burm_reg_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 71;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LUSHR(reg,const) */
			c = l->cost[burm_reg_NT] + r->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 69;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: LUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 67;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 126: /* IAND */
		assert(l && r);
		{	/* reg: IAND(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 56;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 127: /* LAND */
		assert(l && r);
		{	/* reg: LAND(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 57;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 128: /* IOR */
		assert(l && r);
		{	/* reg: IOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 58;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 129: /* LOR */
		assert(l && r);
		{	/* reg: LOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 59;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 130: /* IXOR */
		assert(l && r);
		{	/* reg: IXOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 60;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 131: /* LXOR */
		assert(l && r);
		{	/* reg: LXOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 61;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 132: /* IINC */
		{
			static struct burm_state z = { 132, 0, 0,
				{	0,
					100,	/* root: IINC */
					32767,
					32767,
					32767,
				},{
					19,	/* root: IINC */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 133: /* I2L */
		assert(l);
		{	/* reg: I2L(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 86;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 134: /* I2F */
		assert(l);
		{	/* reg: I2F(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 85;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 135: /* I2D */
		assert(l);
		{	/* reg: I2D(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 84;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 136: /* L2I */
		assert(l);
		{	/* reg: L2I(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 92;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 137: /* L2F */
		assert(l);
		{	/* reg: L2F(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 91;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 138: /* L2D */
		assert(l);
		{	/* reg: L2D(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 90;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 139: /* F2I */
		assert(l);
		{	/* reg: F2I(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 82;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 140: /* F2L */
		assert(l);
		{	/* reg: F2L(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 83;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 141: /* F2D */
		assert(l);
		{	/* reg: F2D(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 81;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 142: /* D2I */
		assert(l);
		{	/* reg: D2I(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 79;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 143: /* D2L */
		assert(l);
		{	/* reg: D2L(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 80;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 144: /* D2F */
		assert(l);
		{	/* reg: D2F(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 78;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 145: /* INT2BYTE */
		assert(l);
		{	/* reg: INT2BYTE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 87;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 146: /* INT2CHAR */
		assert(l);
		{	/* reg: INT2CHAR(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 88;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 147: /* INT2SHORT */
		assert(l);
		{	/* reg: INT2SHORT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 89;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 148: /* LCMP */
		assert(l && r);
		{	/* cmp: LCMP(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_cmp_NT]) {
				p->cost[burm_cmp_NT] = c + 0;
				p->rule.burm_cmp = 5;
				burm_closure_cmp(p, c + 0);
			}
		}
		break;
	case 149: /* FCMPL */
		assert(l && r);
		{	/* cmp: FCMPL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_cmp_NT]) {
				p->cost[burm_cmp_NT] = c + 0;
				p->rule.burm_cmp = 4;
				burm_closure_cmp(p, c + 0);
			}
		}
		break;
	case 150: /* FCMPG */
		assert(l && r);
		{	/* cmp: FCMPG(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_cmp_NT]) {
				p->cost[burm_cmp_NT] = c + 0;
				p->rule.burm_cmp = 3;
				burm_closure_cmp(p, c + 0);
			}
		}
		break;
	case 151: /* DCMPL */
		assert(l && r);
		{	/* cmp: DCMPL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_cmp_NT]) {
				p->cost[burm_cmp_NT] = c + 0;
				p->rule.burm_cmp = 2;
				burm_closure_cmp(p, c + 0);
			}
		}
		break;
	case 152: /* DCMPG */
		assert(l && r);
		{	/* cmp: DCMPG(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_cmp_NT]) {
				p->cost[burm_cmp_NT] = c + 0;
				p->rule.burm_cmp = 1;
				burm_closure_cmp(p, c + 0);
			}
		}
		break;
	case 153: /* IFEQ */
		assert(l);
		{	/* root: IFEQ(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 31;
			}
		}
		break;
	case 154: /* IFNE */
		assert(l);
		{	/* root: IFNE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 36;
			}
		}
		break;
	case 155: /* IFLT */
		assert(l);
		{	/* root: IFLT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 35;
			}
		}
		break;
	case 156: /* IFGE */
		assert(l);
		{	/* root: IFGE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 32;
			}
		}
		break;
	case 157: /* IFGT */
		assert(l);
		{	/* root: IFGT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 33;
			}
		}
		break;
	case 158: /* IFLE */
		assert(l);
		{	/* root: IFLE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 34;
			}
		}
		break;
	case 159: /* IF_ICMPEQ */
		assert(l && r);
		{	/* root: IF_ICMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 41;
			}
		}
		break;
	case 160: /* IF_ICMPNE */
		assert(l && r);
		{	/* root: IF_ICMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 46;
			}
		}
		break;
	case 161: /* IF_ICMPLT */
		assert(l && r);
		{	/* root: IF_ICMPLT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 45;
			}
		}
		break;
	case 162: /* IF_ICMPGE */
		assert(l && r);
		{	/* root: IF_ICMPGE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 42;
			}
		}
		break;
	case 163: /* IF_ICMPGT */
		assert(l && r);
		{	/* root: IF_ICMPGT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 43;
			}
		}
		break;
	case 164: /* IF_ICMPLE */
		assert(l && r);
		{	/* root: IF_ICMPLE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 44;
			}
		}
		break;
	case 165: /* IF_ACMPEQ */
		assert(l && r);
		{	/* root: IF_ACMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 39;
			}
		}
		break;
	case 166: /* IF_ACMPNE */
		assert(l && r);
		{	/* root: IF_ACMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 40;
			}
		}
		break;
	case 167: /* GOTO */
		{
			static struct burm_state z = { 167, 0, 0,
				{	0,
					100,	/* root: GOTO */
					32767,
					32767,
					32767,
				},{
					58,	/* root: GOTO */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 168: /* JSR */
		{
			static struct burm_state z = { 168, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: JSR */
					32767,
					32767,
				},{
					3,	/* root: reg */
					97,	/* reg: JSR */
					0,
					0,
				}
			};
			return &z;
		}
	case 169: /* RET */
		{
			static struct burm_state z = { 169, 0, 0,
				{	0,
					100,	/* root: RET */
					32767,
					32767,
					32767,
				},{
					59,	/* root: RET */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 170: /* TABLESWITCH */
		assert(l);
		{	/* root: TABLESWITCH(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 54;
			}
		}
		break;
	case 171: /* LOOKUPSWITCH */
		assert(l);
		{	/* root: LOOKUPSWITCH(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 53;
			}
		}
		break;
	case 172: /* IRETURN */
		assert(l);
		{	/* root: IRETURN(const) */
			c = l->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 28;
			}
		}
		{	/* root: IRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 23;
			}
		}
		break;
	case 173: /* LRETURN */
		assert(l);
		{	/* root: LRETURN(const) */
			c = l->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 29;
			}
		}
		{	/* root: LRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 24;
			}
		}
		break;
	case 174: /* FRETURN */
		assert(l);
		{	/* root: FRETURN(const) */
			c = l->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 27;
			}
		}
		{	/* root: FRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 22;
			}
		}
		break;
	case 175: /* DRETURN */
		assert(l);
		{	/* root: DRETURN(const) */
			c = l->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 26;
			}
		}
		{	/* root: DRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 21;
			}
		}
		break;
	case 176: /* ARETURN */
		assert(l);
		{	/* root: ARETURN(const) */
			c = l->cost[burm_const_NT] + 10000;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 25;
			}
		}
		{	/* root: ARETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 20;
			}
		}
		break;
	case 177: /* RETURN */
		{
			static struct burm_state z = { 177, 0, 0,
				{	0,
					100,	/* root: RETURN */
					32767,
					32767,
					32767,
				},{
					30,	/* root: RETURN */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 178: /* GETSTATIC */
		{
			static struct burm_state z = { 178, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: GETSTATIC */
					32767,
					32767,
				},{
					3,	/* root: reg */
					96,	/* reg: GETSTATIC */
					0,
					0,
				}
			};
			return &z;
		}
	case 179: /* PUTSTATIC */
		assert(l);
		{	/* root: PUTSTATIC(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 18;
			}
		}
		break;
	case 180: /* GETFIELD */
		assert(l);
		{	/* reg: GETFIELD(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 95;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 181: /* PUTFIELD */
		assert(l && r);
		{	/* root: PUTFIELD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 17;
			}
		}
		break;
	case 182: /* INVOKEVIRTUAL */
		assert(l && r);
		{	/* reg: INVOKEVIRTUAL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 103;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 183: /* INVOKESPECIAL */
		assert(l && r);
		{	/* reg: INVOKESPECIAL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 101;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 184: /* INVOKESTATIC */
		assert(l && r);
		{	/* reg: INVOKESTATIC(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 102;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 185: /* INVOKEINTERFACE */
		assert(l && r);
		{	/* reg: INVOKEINTERFACE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 100;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 186: /* UNDEF186 */
		{
			static struct burm_state z = { 186, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 187: /* NEW */
		{
			static struct burm_state z = { 187, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 188: /* NEWARRAY */
		{
			static struct burm_state z = { 188, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 189: /* ANEWARRAY */
		{
			static struct burm_state z = { 189, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 190: /* ARRAYLENGTH */
		assert(l);
		{	/* reg: ARRAYLENGTH(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 72;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 191: /* ATHROW */
		assert(l);
		{	/* root: ATHROW(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 60;
			}
		}
		break;
	case 192: /* CHECKCAST */
		assert(l);
		{	/* reg: CHECKCAST(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 73;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 193: /* INSTANCEOF */
		assert(l);
		{	/* reg: INSTANCEOF(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 74;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 194: /* MONITORENTER */
		{
			static struct burm_state z = { 194, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 195: /* MONITOREXIT */
		{
			static struct burm_state z = { 195, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 196: /* UNDEF196 */
		{
			static struct burm_state z = { 196, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 197: /* MULTIANEWARRAY */
		assert(l && r);
		{	/* reg: MULTIANEWARRAY(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 93;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 198: /* IFNULL */
		assert(l);
		{	/* root: IFNULL(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 38;
			}
		}
		break;
	case 199: /* IFNONNULL */
		assert(l);
		{	/* root: IFNONNULL(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 37;
			}
		}
		break;
	case 200: /* UNDEF200 */
		{
			static struct burm_state z = { 200, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 201: /* UNDEF201 */
		{
			static struct burm_state z = { 201, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 202: /* BREAKPOINT */
		{
			static struct burm_state z = { 202, 0, 0,
				{	0,
					100,	/* root: BREAKPOINT */
					32767,
					32767,
					32767,
				},{
					61,	/* root: BREAKPOINT */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 203: /* UNDEF203 */
		{
			static struct burm_state z = { 203, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 204: /* IASTORECONST */
		{
			static struct burm_state z = { 204, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 205: /* LASTORECONST */
		{
			static struct burm_state z = { 205, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 206: /* FASTORECONST */
		{
			static struct burm_state z = { 206, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 207: /* DASTORECONST */
		{
			static struct burm_state z = { 207, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 208: /* AASTORECONST */
		{
			static struct burm_state z = { 208, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 209: /* BASTORECONST */
		{
			static struct burm_state z = { 209, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 210: /* CASTORECONST */
		{
			static struct burm_state z = { 210, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 211: /* SASTORECONST */
		{
			static struct burm_state z = { 211, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 212: /* PUTSTATICCONST */
		{
			static struct burm_state z = { 212, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 213: /* PUTFIELDCONST */
		{
			static struct burm_state z = { 213, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 214: /* IMULPOW2 */
		{
			static struct burm_state z = { 214, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 215: /* LMULPOW2 */
		{
			static struct burm_state z = { 215, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 216: /* IF_FCMPEQ */
		{
			static struct burm_state z = { 216, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 217: /* IF_FCMPNE */
		{
			static struct burm_state z = { 217, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 218: /* IF_FCMPL_LT */
		{
			static struct burm_state z = { 218, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 219: /* IF_FCMPL_GE */
		{
			static struct burm_state z = { 219, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 220: /* IF_FCMPL_GT */
		{
			static struct burm_state z = { 220, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 221: /* IF_FCMPL_LE */
		{
			static struct burm_state z = { 221, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 222: /* IF_FCMPG_LT */
		{
			static struct burm_state z = { 222, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 223: /* IF_FCMPG_GE */
		{
			static struct burm_state z = { 223, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 224: /* IF_FCMPG_GT */
		{
			static struct burm_state z = { 224, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 225: /* IF_FCMPG_LE */
		{
			static struct burm_state z = { 225, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 226: /* IF_DCMPEQ */
		{
			static struct burm_state z = { 226, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 227: /* IF_DCMPNE */
		{
			static struct burm_state z = { 227, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 228: /* IF_DCMPL_LT */
		{
			static struct burm_state z = { 228, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 229: /* IF_DCMPL_GE */
		{
			static struct burm_state z = { 229, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 230: /* IF_DCMPL_GT */
		{
			static struct burm_state z = { 230, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 231: /* IF_DCMPL_LE */
		{
			static struct burm_state z = { 231, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 232: /* IF_DCMPG_LT */
		{
			static struct burm_state z = { 232, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 233: /* IF_DCMPG_GE */
		{
			static struct burm_state z = { 233, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 234: /* IF_DCMPG_GT */
		{
			static struct burm_state z = { 234, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 235: /* IF_DCMPG_LE */
		{
			static struct burm_state z = { 235, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 236: /* UNDEF236 */
		{
			static struct burm_state z = { 236, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 237: /* UNDEF237 */
		{
			static struct burm_state z = { 237, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 238: /* UNDEF238 */
		{
			static struct burm_state z = { 238, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 239: /* UNDEF239 */
		{
			static struct burm_state z = { 239, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 240: /* UNDEF240 */
		{
			static struct burm_state z = { 240, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 241: /* UNDEF241 */
		{
			static struct burm_state z = { 241, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 242: /* UNDEF242 */
		{
			static struct burm_state z = { 242, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 243: /* UNDEF243 */
		{
			static struct burm_state z = { 243, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 244: /* UNDEF244 */
		{
			static struct burm_state z = { 244, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 245: /* UNDEF245 */
		{
			static struct burm_state z = { 245, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 246: /* UNDEF246 */
		{
			static struct burm_state z = { 246, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 247: /* UNDEF247 */
		{
			static struct burm_state z = { 247, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 248: /* UNDEF248 */
		{
			static struct burm_state z = { 248, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 249: /* GETEXCEPTION */
		{
			static struct burm_state z = { 249, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: GETEXCEPTION */
					32767,
					32767,
				},{
					3,	/* root: reg */
					94,	/* reg: GETEXCEPTION */
					0,
					0,
				}
			};
			return &z;
		}
	case 250: /* PHI */
		{
			static struct burm_state z = { 250, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: PHI */
					32767,
					32767,
				},{
					3,	/* root: reg */
					98,	/* reg: PHI */
					0,
					0,
				}
			};
			return &z;
		}
	case 251: /* INLINE_START */
		{
			static struct burm_state z = { 251, 0, 0,
				{	0,
					100,	/* root: INLINE_START */
					32767,
					32767,
					32767,
				},{
					57,	/* root: INLINE_START */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 252: /* INLINE_END */
		{
			static struct burm_state z = { 252, 0, 0,
				{	0,
					100,	/* root: INLINE_END */
					32767,
					32767,
					32767,
				},{
					56,	/* root: INLINE_END */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 253: /* INLINE_BODY */
		{
			static struct burm_state z = { 253, 0, 0,
				{	0,
					100,	/* root: INLINE_BODY */
					32767,
					32767,
					32767,
				},{
					55,	/* root: INLINE_BODY */
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 254: /* UNDEF254 */
		{
			static struct burm_state z = { 254, 0, 0,
				{	0,
					32767,
					32767,
					32767,
					32767,
				},{
					0,
					0,
					0,
					0,
				}
			};
			return &z;
		}
	case 255: /* BUILTIN */
		assert(l && r);
		{	/* reg: BUILTIN(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 104;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 300: /* RESULT */
		{
			static struct burm_state z = { 300, 0, 0,
				{	0,
					100,	/* root: reg */
					100,	/* reg: RESULT */
					32767,
					32767,
				},{
					3,	/* root: reg */
					99,	/* reg: RESULT */
					0,
					0,
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
	return STATE_LABEL(p)->rule.burm_root ? STATE_LABEL(p) : 0;
}

NODEPTR_TYPE *burm_kids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]) {
	burm_assert(p, PANIC("NULL tree in burm_kids\n"));
	burm_assert(kids, PANIC("NULL kids in burm_kids\n"));
	switch (eruleno) {
	case 172: /* root: BREAKPOINT */
	case 169: /* root: RET */
	case 168: /* root: GOTO */
	case 163: /* root: INLINE_START */
	case 162: /* root: INLINE_END */
	case 161: /* root: INLINE_BODY */
	case 136: /* root: RETURN */
	case 125: /* root: IINC */
	case 117: /* root: SASTORE */
	case 116: /* root: LASTORE */
	case 115: /* root: IASTORE */
	case 114: /* root: FASTORE */
	case 113: /* root: DASTORE */
	case 112: /* root: CASTORE */
	case 111: /* root: BASTORE */
	case 110: /* root: AASTORE */
	case 109: /* reg: RESULT */
	case 108: /* reg: PHI */
	case 107: /* reg: JSR */
	case 106: /* reg: GETSTATIC */
	case 104: /* reg: GETEXCEPTION */
	case 87: /* reg: MOVE */
	case 86: /* reg: COPY */
	case 16: /* reg: ILOAD */
	case 15: /* reg: LLOAD */
	case 14: /* reg: FLOAD */
	case 13: /* reg: DLOAD */
	case 12: /* reg: ALOAD */
	case 11: /* reg: FCONST */
	case 10: /* reg: DCONST */
	case 9: /* reg: ACONST */
	case 8: /* const: ICONST */
	case 7: /* const: LCONST */
	case 1: /* reg: NOP */
		break;
	case 171: /* root: ATHROW(reg) */
	case 160: /* root: TABLESWITCH(reg) */
	case 159: /* root: LOOKUPSWITCH(reg) */
	case 144: /* root: IFNULL(reg) */
	case 143: /* root: IFNONNULL(reg) */
	case 142: /* root: IFNE(reg) */
	case 141: /* root: IFLT(reg) */
	case 140: /* root: IFLE(reg) */
	case 139: /* root: IFGT(reg) */
	case 138: /* root: IFGE(reg) */
	case 137: /* root: IFEQ(reg) */
	case 135: /* root: LRETURN(const) */
	case 134: /* root: IRETURN(const) */
	case 133: /* root: FRETURN(const) */
	case 132: /* root: DRETURN(const) */
	case 131: /* root: ARETURN(const) */
	case 130: /* root: LRETURN(reg) */
	case 129: /* root: IRETURN(reg) */
	case 128: /* root: FRETURN(reg) */
	case 127: /* root: DRETURN(reg) */
	case 126: /* root: ARETURN(reg) */
	case 124: /* root: PUTSTATIC(reg) */
	case 122: /* root: LSTORE(reg) */
	case 121: /* root: ISTORE(reg) */
	case 120: /* root: FSTORE(reg) */
	case 119: /* root: DSTORE(reg) */
	case 118: /* root: ASTORE(reg) */
	case 105: /* reg: GETFIELD(reg) */
	case 102: /* reg: L2I(reg) */
	case 101: /* reg: L2F(reg) */
	case 100: /* reg: L2D(reg) */
	case 99: /* reg: INT2SHORT(reg) */
	case 98: /* reg: INT2CHAR(reg) */
	case 97: /* reg: INT2BYTE(reg) */
	case 96: /* reg: I2L(reg) */
	case 95: /* reg: I2F(reg) */
	case 94: /* reg: I2D(reg) */
	case 93: /* reg: F2L(reg) */
	case 92: /* reg: F2I(reg) */
	case 91: /* reg: F2D(reg) */
	case 90: /* reg: D2L(reg) */
	case 89: /* reg: D2I(reg) */
	case 88: /* reg: D2F(reg) */
	case 85: /* reg: CHECKNULL(reg) */
	case 84: /* reg: INSTANCEOF(reg) */
	case 83: /* reg: CHECKCAST(reg) */
	case 82: /* reg: ARRAYLENGTH(reg) */
	case 33: /* reg: INEG(reg) */
	case 32: /* reg: LNEG(reg) */
	case 31: /* reg: FNEG(reg) */
	case 30: /* reg: DNEG(reg) */
	case 2: /* root: POP(reg) */
		kids[0] = LEFT_CHILD(p);
		break;
	case 170: /* reg: BUILTIN(reg,reg) */
	case 167: /* reg: INVOKEVIRTUAL(reg,reg) */
	case 166: /* reg: INVOKESTATIC(reg,reg) */
	case 165: /* reg: INVOKESPECIAL(reg,reg) */
	case 164: /* reg: INVOKEINTERFACE(reg,reg) */
	case 158: /* root: IF_LCMPNE(reg,reg) */
	case 157: /* root: IF_LCMPLT(reg,reg) */
	case 156: /* root: IF_LCMPLE(reg,reg) */
	case 155: /* root: IF_LCMPGT(reg,reg) */
	case 154: /* root: IF_LCMPGE(reg,reg) */
	case 153: /* root: IF_LCMPEQ(reg,reg) */
	case 152: /* root: IF_ICMPNE(reg,reg) */
	case 151: /* root: IF_ICMPLT(reg,reg) */
	case 150: /* root: IF_ICMPLE(reg,reg) */
	case 149: /* root: IF_ICMPGT(reg,reg) */
	case 148: /* root: IF_ICMPGE(reg,reg) */
	case 147: /* root: IF_ICMPEQ(reg,reg) */
	case 146: /* root: IF_ACMPNE(reg,reg) */
	case 145: /* root: IF_ACMPEQ(reg,reg) */
	case 123: /* root: PUTFIELD(reg,reg) */
	case 103: /* reg: MULTIANEWARRAY(reg,reg) */
	case 81: /* reg: LUSHR(const,reg) */
	case 80: /* reg: IUSHR(const,reg) */
	case 79: /* reg: LUSHR(reg,const) */
	case 78: /* reg: IUSHR(reg,const) */
	case 77: /* reg: LUSHR(reg,reg) */
	case 76: /* reg: IUSHR(reg,reg) */
	case 75: /* reg: LSHR(reg,reg) */
	case 74: /* reg: LSHL(reg,reg) */
	case 73: /* reg: ISHR(reg,reg) */
	case 72: /* reg: ISHL(reg,reg) */
	case 71: /* reg: LXOR(reg,reg) */
	case 70: /* reg: IXOR(reg,reg) */
	case 69: /* reg: LOR(reg,reg) */
	case 68: /* reg: IOR(reg,reg) */
	case 67: /* reg: LAND(reg,reg) */
	case 66: /* reg: IAND(reg,reg) */
	case 65: /* reg: IDIV(reg,reg) */
	case 64: /* reg: LDIV(reg,reg) */
	case 63: /* reg: FDIV(reg,reg) */
	case 62: /* reg: DDIV(reg,reg) */
	case 61: /* reg: LSUB(reg,reg) */
	case 60: /* reg: ISUB(reg,reg) */
	case 59: /* reg: FSUB(reg,reg) */
	case 58: /* reg: DSUB(reg,reg) */
	case 57: /* reg: LREM(reg,const) */
	case 56: /* reg: IREM(reg,const) */
	case 55: /* reg: LREM(const,reg) */
	case 54: /* reg: IREM(const,reg) */
	case 53: /* reg: LREM(reg,reg) */
	case 52: /* reg: IREM(reg,reg) */
	case 51: /* reg: FREM(reg,reg) */
	case 50: /* reg: DREM(reg,reg) */
	case 49: /* reg: LMUL(reg,const) */
	case 48: /* reg: IMUL(reg,const) */
	case 47: /* reg: LMUL(const,reg) */
	case 46: /* reg: IMUL(const,reg) */
	case 45: /* reg: LMUL(reg,reg) */
	case 44: /* reg: IMUL(reg,reg) */
	case 43: /* reg: FMUL(reg,reg) */
	case 42: /* reg: DMUL(reg,reg) */
	case 41: /* reg: IADD(reg,const) */
	case 40: /* reg: LADD(reg,const) */
	case 39: /* reg: IADD(const,reg) */
	case 38: /* reg: LADD(const,reg) */
	case 37: /* reg: IADD(reg,reg) */
	case 36: /* reg: LADD(reg,reg) */
	case 35: /* reg: FADD(reg,reg) */
	case 34: /* reg: DADD(reg,reg) */
	case 29: /* cmp: LCMP(reg,reg) */
	case 28: /* cmp: FCMPL(reg,reg) */
	case 27: /* cmp: FCMPG(reg,reg) */
	case 26: /* cmp: DCMPL(reg,reg) */
	case 25: /* cmp: DCMPG(reg,reg) */
	case 24: /* reg: SALOAD(reg,reg) */
	case 23: /* reg: IALOAD(reg,reg) */
	case 22: /* reg: LALOAD(reg,reg) */
	case 21: /* reg: FALOAD(reg,reg) */
	case 20: /* reg: DALOAD(reg,reg) */
	case 19: /* reg: CALOAD(reg,reg) */
	case 18: /* reg: BALOAD(reg,reg) */
	case 17: /* reg: AALOAD(reg,reg) */
	case 3: /* root: POP2(reg,reg) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = RIGHT_CHILD(p);
		break;
	case 6: /* reg: cmp */
	case 5: /* reg: const */
	case 4: /* root: reg */
		kids[0] = p;
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
#ifdef AUTOMATON_KIND_FSM
#elif AUTOMATON_KIND_IBURG
void burm_reduce(NODEPTR_TYPE bnode, int goalnt);
#endif
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
  }
  burm_kids (bnode, ruleNo, kids);
  for (i = 0; nts[i]; i++)
    burm_reduce (kids[i], nts[i]);    /* reduce kids */


  switch (ruleNo) {
  case 5:
    codegen_push_to_reg(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 7:
    remember_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 8:
    remember_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 9:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 10:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 11:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 12:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 13:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 14:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 15:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 16:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 17:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 18:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 19:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 20:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 21:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 22:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 23:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 24:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 25:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 26:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 27:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 28:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 29:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 30:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 31:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 32:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 33:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 34:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 35:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 36:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 37:
    codegen_emit_iadd(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 38:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 39:
    codegen_emit_iaddconst_right(JITDATA(bnode), CURRENT_INST(bnode), PREV_INST_LEFT(bnode));
    break;
  case 40:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 41:
    codegen_emit_iaddconst(JITDATA(bnode), CURRENT_INST(bnode), PREV_INST_RIGHT(bnode));
    break;
  case 42:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 43:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 44:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 45:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 46:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 47:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 48:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 49:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 50:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 51:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 52:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 53:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 54:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 55:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 56:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 57:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 58:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 59:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 60:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 61:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 62:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 63:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 64:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 65:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 66:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 67:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 68:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 69:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 70:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 71:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 72:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 73:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 74:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 75:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 76:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 77:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 78:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 79:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 80:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 81:
    codegen_emit_combined_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 82:
    codegen_emit_arraylength(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 83:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 84:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 85:
    codegen_emit_checknull(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 86:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 87:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 88:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 89:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 90:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 91:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 92:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 93:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 94:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 95:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 96:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 97:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 98:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 99:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 100:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 101:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 102:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 103:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 104:
    codegen_emit_getexception(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 105:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 106:
    codegen_emit_getstatic(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 107:
    codegen_emit_jump(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 108:
    codegen_emit_phi(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 109:
    codegen_emit_result(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 110:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 111:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 112:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 113:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 114:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 115:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 116:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 117:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 118:
    codegen_emit_astore(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 119:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 120:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 121:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 122:
    codegen_emit_copy(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 123:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 124:
    codegen_emit_putstatic(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 125:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 126:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 127:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 128:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 129:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 130:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 131:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 132:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 133:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 134:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 135:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 136:
    codegen_emit_return(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 137:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 138:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 139:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 140:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 141:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 142:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 143:
    codegen_emit_ifnull(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 144:
    codegen_emit_ifnull(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 145:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 146:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 147:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 148:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 149:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 150:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 151:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 152:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 153:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 154:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 155:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 156:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 157:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 158:
    codegen_emit_branch(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 159:
    codegen_emit_lookup(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 160:
    codegen_emit_simple_instruction(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 161:
    codegen_emit_inline_body(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 162:
    codegen_emit_inline_end(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 163:
    codegen_emit_inline_start(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 164:
    codegen_emit_invoke(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 165:
    codegen_emit_invoke(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 166:
    codegen_emit_invoke(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 167:
    codegen_emit_invoke(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 168:
    codegen_emit_jump(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 169:
    codegen_emit_jump(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 170:
    codegen_emit_builtin(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 171:
    codegen_emit_throw(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  case 172:
    codegen_emit_breakpoint(JITDATA(bnode), CURRENT_INST(bnode));
    break;
  }
}
