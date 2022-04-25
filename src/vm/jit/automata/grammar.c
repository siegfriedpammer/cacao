typedef struct burm_state *STATEPTR_TYPE;

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

//#define DEBUG 1

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
		unsigned burm_reg:8;
	} rule;
};

static short burm_nts_0[] = { 0 };
static short burm_nts_1[] = { burm_reg_NT, 0 };
static short burm_nts_2[] = { burm_reg_NT, burm_reg_NT, 0 };

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
	burm_nts_1,	/* 10 */
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
	burm_nts_2,	/* 39 */
	burm_nts_2,	/* 40 */
	burm_nts_2,	/* 41 */
	burm_nts_2,	/* 42 */
	burm_nts_2,	/* 43 */
	burm_nts_2,	/* 44 */
	burm_nts_2,	/* 45 */
	burm_nts_2,	/* 46 */
	burm_nts_2,	/* 47 */
	burm_nts_2,	/* 48 */
	burm_nts_2,	/* 49 */
	burm_nts_2,	/* 50 */
	burm_nts_2,	/* 51 */
	burm_nts_2,	/* 52 */
	burm_nts_2,	/* 53 */
	burm_nts_2,	/* 54 */
	burm_nts_2,	/* 55 */
	burm_nts_2,	/* 56 */
	burm_nts_2,	/* 57 */
	burm_nts_2,	/* 58 */
	burm_nts_0,	/* 59 */
	burm_nts_0,	/* 60 */
	burm_nts_0,	/* 61 */
	burm_nts_0,	/* 62 */
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
	burm_nts_2,	/* 125 */
	burm_nts_2,	/* 126 */
	burm_nts_2,	/* 127 */
	burm_nts_2,	/* 128 */
	burm_nts_0,	/* 129 */
	burm_nts_0,	/* 130 */
	burm_nts_0,	/* 131 */
	burm_nts_1,	/* 132 */
	burm_nts_0,	/* 133 */
	burm_nts_0,	/* 134 */
	burm_nts_0,	/* 135 */
	burm_nts_0,	/* 136 */
	burm_nts_0,	/* 137 */
	burm_nts_0,	/* 138 */
	burm_nts_0,	/* 139 */
	burm_nts_0,	/* 140 */
	burm_nts_2,	/* 141 */
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
	0,	/* 116=INEG */
	0,	/* 117=LNEG */
	0,	/* 118=FNEG */
	0,	/* 119=DNEG */
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
	2,	/* 182=INVOKEVIRTUAL */
	2,	/* 183=INVOKESPECIAL */
	2,	/* 184=INVOKESTATIC */
	2,	/* 185=INVOKEINTERFACE */
	0,	/* 186=UNDEF186 */
	0,	/* 187=NEW */
	0,	/* 188=NEWARRAY */
	0,	/* 189=ANEWARRAY */
	0,	/* 190=ARRAYLENGTH */
	1,	/* 191=ATHROW */
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
	{ 100 },	/* 1 = reg: NOP */
	{ 100 },	/* 2 = reg: POP */
	{ 100 },	/* 3 = reg: POP2 */
	{ 100 },	/* 4 = reg: RESULT */
	{ 100 },	/* 5 = reg: ACONST */
	{ 100 },	/* 6 = reg: ICONST */
	{ 100 },	/* 7 = reg: LCONST */
	{ 100 },	/* 8 = reg: FCONST */
	{ 100 },	/* 9 = reg: DCONST */
	{ 100 },	/* 10 = reg: CHECKNULL(reg) */
	{ 100 },	/* 11 = reg: COPY */
	{ 100 },	/* 12 = reg: MOVE */
	{ 100 },	/* 13 = reg: ILOAD */
	{ 100 },	/* 14 = reg: LLOAD */
	{ 100 },	/* 15 = reg: FLOAD */
	{ 100 },	/* 16 = reg: DLOAD */
	{ 100 },	/* 17 = reg: ALOAD */
	{ 100 },	/* 18 = reg: IALOAD */
	{ 100 },	/* 19 = reg: LALOAD */
	{ 100 },	/* 20 = reg: FALOAD */
	{ 100 },	/* 21 = reg: DALOAD */
	{ 100 },	/* 22 = reg: AALOAD */
	{ 100 },	/* 23 = reg: BALOAD */
	{ 100 },	/* 24 = reg: CALOAD */
	{ 100 },	/* 25 = reg: SALOAD */
	{ 100 },	/* 26 = reg: ISTORE */
	{ 100 },	/* 27 = reg: LSTORE */
	{ 100 },	/* 28 = reg: FSTORE */
	{ 100 },	/* 29 = reg: DSTORE */
	{ 100 },	/* 30 = reg: ASTORE */
	{ 100 },	/* 31 = reg: IASTORE */
	{ 100 },	/* 32 = reg: LASTORE */
	{ 100 },	/* 33 = reg: FASTORE */
	{ 100 },	/* 34 = reg: DASTORE */
	{ 100 },	/* 35 = reg: AASTORE */
	{ 100 },	/* 36 = reg: BASTORE */
	{ 100 },	/* 37 = reg: CASTORE */
	{ 100 },	/* 38 = reg: SASTORE */
	{ 100 },	/* 39 = reg: IADD(reg,reg) */
	{ 100 },	/* 40 = reg: LADD(reg,reg) */
	{ 100 },	/* 41 = reg: FADD(reg,reg) */
	{ 100 },	/* 42 = reg: DADD(reg,reg) */
	{ 100 },	/* 43 = reg: ISUB(reg,reg) */
	{ 100 },	/* 44 = reg: LSUB(reg,reg) */
	{ 100 },	/* 45 = reg: FSUB(reg,reg) */
	{ 100 },	/* 46 = reg: DSUB(reg,reg) */
	{ 100 },	/* 47 = reg: IMUL(reg,reg) */
	{ 100 },	/* 48 = reg: LMUL(reg,reg) */
	{ 100 },	/* 49 = reg: FMUL(reg,reg) */
	{ 100 },	/* 50 = reg: DMUL(reg,reg) */
	{ 100 },	/* 51 = reg: IDIV(reg,reg) */
	{ 100 },	/* 52 = reg: LDIV(reg,reg) */
	{ 100 },	/* 53 = reg: FDIV(reg,reg) */
	{ 100 },	/* 54 = reg: DDIV(reg,reg) */
	{ 100 },	/* 55 = reg: IREM(reg,reg) */
	{ 100 },	/* 56 = reg: LREM(reg,reg) */
	{ 100 },	/* 57 = reg: FREM(reg,reg) */
	{ 100 },	/* 58 = reg: DREM(reg,reg) */
	{ 100 },	/* 59 = reg: INEG */
	{ 100 },	/* 60 = reg: LNEG */
	{ 100 },	/* 61 = reg: FNEG */
	{ 100 },	/* 62 = reg: DNEG */
	{ 100 },	/* 63 = reg: ISHL(reg,reg) */
	{ 100 },	/* 64 = reg: LSHL(reg,reg) */
	{ 100 },	/* 65 = reg: ISHR(reg,reg) */
	{ 100 },	/* 66 = reg: LSHR(reg,reg) */
	{ 100 },	/* 67 = reg: IUSHR(reg,reg) */
	{ 100 },	/* 68 = reg: LUSHR(reg,reg) */
	{ 100 },	/* 69 = reg: IAND(reg,reg) */
	{ 100 },	/* 70 = reg: LAND(reg,reg) */
	{ 100 },	/* 71 = reg: IOR(reg,reg) */
	{ 100 },	/* 72 = reg: LOR(reg,reg) */
	{ 100 },	/* 73 = reg: IXOR(reg,reg) */
	{ 100 },	/* 74 = reg: LXOR(reg,reg) */
	{ 100 },	/* 75 = reg: IINC */
	{ 100 },	/* 76 = reg: I2L */
	{ 100 },	/* 77 = reg: I2F */
	{ 100 },	/* 78 = reg: I2D */
	{ 100 },	/* 79 = reg: L2I */
	{ 100 },	/* 80 = reg: L2F */
	{ 100 },	/* 81 = reg: L2D */
	{ 100 },	/* 82 = reg: F2I */
	{ 100 },	/* 83 = reg: F2L */
	{ 100 },	/* 84 = reg: F2D */
	{ 100 },	/* 85 = reg: D2I */
	{ 100 },	/* 86 = reg: D2L */
	{ 100 },	/* 87 = reg: D2F */
	{ 100 },	/* 88 = reg: INT2BYTE */
	{ 100 },	/* 89 = reg: INT2CHAR */
	{ 100 },	/* 90 = reg: INT2SHORT */
	{ 100 },	/* 91 = reg: LCMP */
	{ 100 },	/* 92 = reg: FCMPL */
	{ 100 },	/* 93 = reg: FCMPG */
	{ 100 },	/* 94 = reg: DCMPL */
	{ 100 },	/* 95 = reg: DCMPG */
	{ 100 },	/* 96 = reg: IFEQ */
	{ 100 },	/* 97 = reg: IFNE */
	{ 100 },	/* 98 = reg: IFLT */
	{ 100 },	/* 99 = reg: IFGE */
	{ 100 },	/* 100 = reg: IFGT */
	{ 100 },	/* 101 = reg: IFLE */
	{ 100 },	/* 102 = reg: IF_ICMPEQ */
	{ 100 },	/* 103 = reg: IF_ICMPNE */
	{ 100 },	/* 104 = reg: IF_ICMPLT */
	{ 100 },	/* 105 = reg: IF_ICMPGE */
	{ 100 },	/* 106 = reg: IF_ICMPGT */
	{ 100 },	/* 107 = reg: IF_ICMPLE */
	{ 100 },	/* 108 = reg: IF_ACMPEQ */
	{ 100 },	/* 109 = reg: IF_ACMPNE */
	{ 100 },	/* 110 = reg: GOTO */
	{ 100 },	/* 111 = reg: JSR */
	{ 100 },	/* 112 = reg: RET */
	{ 100 },	/* 113 = reg: TABLESWITCH */
	{ 100 },	/* 114 = reg: LOOKUPSWITCH */
	{ 100 },	/* 115 = reg: IRETURN */
	{ 100 },	/* 116 = reg: LRETURN */
	{ 100 },	/* 117 = reg: FRETURN */
	{ 100 },	/* 118 = reg: DRETURN */
	{ 100 },	/* 119 = reg: ARETURN */
	{ 100 },	/* 120 = reg: RETURN */
	{ 100 },	/* 121 = reg: GETSTATIC */
	{ 100 },	/* 122 = reg: PUTSTATIC */
	{ 100 },	/* 123 = reg: GETFIELD */
	{ 100 },	/* 124 = reg: PUTFIELD */
	{ 100 },	/* 125 = reg: INVOKEVIRTUAL(reg,reg) */
	{ 100 },	/* 126 = reg: INVOKESPECIAL(reg,reg) */
	{ 100 },	/* 127 = reg: INVOKESTATIC(reg,reg) */
	{ 100 },	/* 128 = reg: INVOKEINTERFACE(reg,reg) */
	{ 100 },	/* 129 = reg: CHECKCAST */
	{ 100 },	/* 130 = reg: INSTANCEOF */
	{ 100 },	/* 131 = reg: MULTIANEWARRAY */
	{ 100 },	/* 132 = reg: ATHROW(reg) */
	{ 100 },	/* 133 = reg: IFNULL */
	{ 100 },	/* 134 = reg: IFNONNULL */
	{ 100 },	/* 135 = reg: BREAKPOINT */
	{ 100 },	/* 136 = reg: GETEXCEPTION */
	{ 100 },	/* 137 = reg: PHI */
	{ 100 },	/* 138 = reg: INLINE_START */
	{ 100 },	/* 139 = reg: INLINE_END */
	{ 100 },	/* 140 = reg: INLINE_BODY */
	{ 100 },	/* 141 = reg: BUILTIN(reg,reg) */
};

char *burm_string[] = {
	/* 0 */	0,
	/* 1 */	"reg: NOP",
	/* 2 */	"reg: POP",
	/* 3 */	"reg: POP2",
	/* 4 */	"reg: RESULT",
	/* 5 */	"reg: ACONST",
	/* 6 */	"reg: ICONST",
	/* 7 */	"reg: LCONST",
	/* 8 */	"reg: FCONST",
	/* 9 */	"reg: DCONST",
	/* 10 */	"reg: CHECKNULL(reg)",
	/* 11 */	"reg: COPY",
	/* 12 */	"reg: MOVE",
	/* 13 */	"reg: ILOAD",
	/* 14 */	"reg: LLOAD",
	/* 15 */	"reg: FLOAD",
	/* 16 */	"reg: DLOAD",
	/* 17 */	"reg: ALOAD",
	/* 18 */	"reg: IALOAD",
	/* 19 */	"reg: LALOAD",
	/* 20 */	"reg: FALOAD",
	/* 21 */	"reg: DALOAD",
	/* 22 */	"reg: AALOAD",
	/* 23 */	"reg: BALOAD",
	/* 24 */	"reg: CALOAD",
	/* 25 */	"reg: SALOAD",
	/* 26 */	"reg: ISTORE",
	/* 27 */	"reg: LSTORE",
	/* 28 */	"reg: FSTORE",
	/* 29 */	"reg: DSTORE",
	/* 30 */	"reg: ASTORE",
	/* 31 */	"reg: IASTORE",
	/* 32 */	"reg: LASTORE",
	/* 33 */	"reg: FASTORE",
	/* 34 */	"reg: DASTORE",
	/* 35 */	"reg: AASTORE",
	/* 36 */	"reg: BASTORE",
	/* 37 */	"reg: CASTORE",
	/* 38 */	"reg: SASTORE",
	/* 39 */	"reg: IADD(reg,reg)",
	/* 40 */	"reg: LADD(reg,reg)",
	/* 41 */	"reg: FADD(reg,reg)",
	/* 42 */	"reg: DADD(reg,reg)",
	/* 43 */	"reg: ISUB(reg,reg)",
	/* 44 */	"reg: LSUB(reg,reg)",
	/* 45 */	"reg: FSUB(reg,reg)",
	/* 46 */	"reg: DSUB(reg,reg)",
	/* 47 */	"reg: IMUL(reg,reg)",
	/* 48 */	"reg: LMUL(reg,reg)",
	/* 49 */	"reg: FMUL(reg,reg)",
	/* 50 */	"reg: DMUL(reg,reg)",
	/* 51 */	"reg: IDIV(reg,reg)",
	/* 52 */	"reg: LDIV(reg,reg)",
	/* 53 */	"reg: FDIV(reg,reg)",
	/* 54 */	"reg: DDIV(reg,reg)",
	/* 55 */	"reg: IREM(reg,reg)",
	/* 56 */	"reg: LREM(reg,reg)",
	/* 57 */	"reg: FREM(reg,reg)",
	/* 58 */	"reg: DREM(reg,reg)",
	/* 59 */	"reg: INEG",
	/* 60 */	"reg: LNEG",
	/* 61 */	"reg: FNEG",
	/* 62 */	"reg: DNEG",
	/* 63 */	"reg: ISHL(reg,reg)",
	/* 64 */	"reg: LSHL(reg,reg)",
	/* 65 */	"reg: ISHR(reg,reg)",
	/* 66 */	"reg: LSHR(reg,reg)",
	/* 67 */	"reg: IUSHR(reg,reg)",
	/* 68 */	"reg: LUSHR(reg,reg)",
	/* 69 */	"reg: IAND(reg,reg)",
	/* 70 */	"reg: LAND(reg,reg)",
	/* 71 */	"reg: IOR(reg,reg)",
	/* 72 */	"reg: LOR(reg,reg)",
	/* 73 */	"reg: IXOR(reg,reg)",
	/* 74 */	"reg: LXOR(reg,reg)",
	/* 75 */	"reg: IINC",
	/* 76 */	"reg: I2L",
	/* 77 */	"reg: I2F",
	/* 78 */	"reg: I2D",
	/* 79 */	"reg: L2I",
	/* 80 */	"reg: L2F",
	/* 81 */	"reg: L2D",
	/* 82 */	"reg: F2I",
	/* 83 */	"reg: F2L",
	/* 84 */	"reg: F2D",
	/* 85 */	"reg: D2I",
	/* 86 */	"reg: D2L",
	/* 87 */	"reg: D2F",
	/* 88 */	"reg: INT2BYTE",
	/* 89 */	"reg: INT2CHAR",
	/* 90 */	"reg: INT2SHORT",
	/* 91 */	"reg: LCMP",
	/* 92 */	"reg: FCMPL",
	/* 93 */	"reg: FCMPG",
	/* 94 */	"reg: DCMPL",
	/* 95 */	"reg: DCMPG",
	/* 96 */	"reg: IFEQ",
	/* 97 */	"reg: IFNE",
	/* 98 */	"reg: IFLT",
	/* 99 */	"reg: IFGE",
	/* 100 */	"reg: IFGT",
	/* 101 */	"reg: IFLE",
	/* 102 */	"reg: IF_ICMPEQ",
	/* 103 */	"reg: IF_ICMPNE",
	/* 104 */	"reg: IF_ICMPLT",
	/* 105 */	"reg: IF_ICMPGE",
	/* 106 */	"reg: IF_ICMPGT",
	/* 107 */	"reg: IF_ICMPLE",
	/* 108 */	"reg: IF_ACMPEQ",
	/* 109 */	"reg: IF_ACMPNE",
	/* 110 */	"reg: GOTO",
	/* 111 */	"reg: JSR",
	/* 112 */	"reg: RET",
	/* 113 */	"reg: TABLESWITCH",
	/* 114 */	"reg: LOOKUPSWITCH",
	/* 115 */	"reg: IRETURN",
	/* 116 */	"reg: LRETURN",
	/* 117 */	"reg: FRETURN",
	/* 118 */	"reg: DRETURN",
	/* 119 */	"reg: ARETURN",
	/* 120 */	"reg: RETURN",
	/* 121 */	"reg: GETSTATIC",
	/* 122 */	"reg: PUTSTATIC",
	/* 123 */	"reg: GETFIELD",
	/* 124 */	"reg: PUTFIELD",
	/* 125 */	"reg: INVOKEVIRTUAL(reg,reg)",
	/* 126 */	"reg: INVOKESPECIAL(reg,reg)",
	/* 127 */	"reg: INVOKESTATIC(reg,reg)",
	/* 128 */	"reg: INVOKEINTERFACE(reg,reg)",
	/* 129 */	"reg: CHECKCAST",
	/* 130 */	"reg: INSTANCEOF",
	/* 131 */	"reg: MULTIANEWARRAY",
	/* 132 */	"reg: ATHROW(reg)",
	/* 133 */	"reg: IFNULL",
	/* 134 */	"reg: IFNONNULL",
	/* 135 */	"reg: BREAKPOINT",
	/* 136 */	"reg: GETEXCEPTION",
	/* 137 */	"reg: PHI",
	/* 138 */	"reg: INLINE_START",
	/* 139 */	"reg: INLINE_END",
	/* 140 */	"reg: INLINE_BODY",
	/* 141 */	"reg: BUILTIN(reg,reg)",
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
					5,	/* reg: ACONST */
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
				p->rule.burm_reg = 10;
			}
		}
		break;
	case 3: /* ICONST */
		{
			static struct burm_state z = { 3, 0, 0,
				{	0,
					100,	/* reg: ICONST */
				},{
					6,	/* reg: ICONST */
				}
			};
			return &z;
		}
	case 4: /* UNDEF4 */
		{
			static struct burm_state z = { 4, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
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
					7,	/* reg: LCONST */
				}
			};
			return &z;
		}
	case 10: /* LCMPCONST */
		{
			static struct burm_state z = { 10, 0, 0,
				{	0,
					32767,
				},{
					0,
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
					8,	/* reg: FCONST */
				}
			};
			return &z;
		}
	case 12: /* UNDEF12 */
		{
			static struct burm_state z = { 12, 0, 0,
				{	0,
					32767,
				},{
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
				},{
					0,
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
					9,	/* reg: DCONST */
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
					11,	/* reg: COPY */
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
					12,	/* reg: MOVE */
				}
			};
			return &z;
		}
	case 17: /* UNDEF17 */
		{
			static struct burm_state z = { 17, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
					0,
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
					13,	/* reg: ILOAD */
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
					14,	/* reg: LLOAD */
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
					15,	/* reg: FLOAD */
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
					16,	/* reg: DLOAD */
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
					17,	/* reg: ALOAD */
				}
			};
			return &z;
		}
	case 26: /* IADDCONST */
		{
			static struct burm_state z = { 26, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
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
					18,	/* reg: IALOAD */
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
					19,	/* reg: LALOAD */
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
					20,	/* reg: FALOAD */
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
					21,	/* reg: DALOAD */
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
					22,	/* reg: AALOAD */
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
					23,	/* reg: BALOAD */
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
					24,	/* reg: CALOAD */
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
					25,	/* reg: SALOAD */
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
					26,	/* reg: ISTORE */
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
					27,	/* reg: LSTORE */
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
					28,	/* reg: FSTORE */
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
					29,	/* reg: DSTORE */
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
					30,	/* reg: ASTORE */
				}
			};
			return &z;
		}
	case 59: /* IF_LEQ */
		{
			static struct burm_state z = { 59, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
				}
			};
			return &z;
		}
	case 65: /* IF_LCMPEQ */
		{
			static struct burm_state z = { 65, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 66: /* IF_LCMPNE */
		{
			static struct burm_state z = { 66, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 67: /* IF_LCMPLT */
		{
			static struct burm_state z = { 67, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 68: /* IF_LCMPGE */
		{
			static struct burm_state z = { 68, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 69: /* IF_LCMPGT */
		{
			static struct burm_state z = { 69, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 70: /* IF_LCMPLE */
		{
			static struct burm_state z = { 70, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 71: /* UNDEF71 */
		{
			static struct burm_state z = { 71, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
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
					31,	/* reg: IASTORE */
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
					32,	/* reg: LASTORE */
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
					33,	/* reg: FASTORE */
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
					34,	/* reg: DASTORE */
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
					35,	/* reg: AASTORE */
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
					36,	/* reg: BASTORE */
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
					37,	/* reg: CASTORE */
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
					38,	/* reg: SASTORE */
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
					2,	/* reg: POP */
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
					3,	/* reg: POP2 */
				}
			};
			return &z;
		}
	case 89: /* DUP */
		{
			static struct burm_state z = { 89, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
				}
			};
			return &z;
		}
	case 96: /* IADD */
		assert(l && r);
		{	/* reg: IADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 39;
			}
		}
		break;
	case 97: /* LADD */
		assert(l && r);
		{	/* reg: LADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 40;
			}
		}
		break;
	case 98: /* FADD */
		assert(l && r);
		{	/* reg: FADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 41;
			}
		}
		break;
	case 99: /* DADD */
		assert(l && r);
		{	/* reg: DADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 42;
			}
		}
		break;
	case 100: /* ISUB */
		assert(l && r);
		{	/* reg: ISUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 43;
			}
		}
		break;
	case 101: /* LSUB */
		assert(l && r);
		{	/* reg: LSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 44;
			}
		}
		break;
	case 102: /* FSUB */
		assert(l && r);
		{	/* reg: FSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 45;
			}
		}
		break;
	case 103: /* DSUB */
		assert(l && r);
		{	/* reg: DSUB(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 46;
			}
		}
		break;
	case 104: /* IMUL */
		assert(l && r);
		{	/* reg: IMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 47;
			}
		}
		break;
	case 105: /* LMUL */
		assert(l && r);
		{	/* reg: LMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 48;
			}
		}
		break;
	case 106: /* FMUL */
		assert(l && r);
		{	/* reg: FMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 49;
			}
		}
		break;
	case 107: /* DMUL */
		assert(l && r);
		{	/* reg: DMUL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 50;
			}
		}
		break;
	case 108: /* IDIV */
		assert(l && r);
		{	/* reg: IDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 51;
			}
		}
		break;
	case 109: /* LDIV */
		assert(l && r);
		{	/* reg: LDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 52;
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
			}
		}
		break;
	case 111: /* DDIV */
		assert(l && r);
		{	/* reg: DDIV(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 54;
			}
		}
		break;
	case 112: /* IREM */
		assert(l && r);
		{	/* reg: IREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 55;
			}
		}
		break;
	case 113: /* LREM */
		assert(l && r);
		{	/* reg: LREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 56;
			}
		}
		break;
	case 114: /* FREM */
		assert(l && r);
		{	/* reg: FREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 57;
			}
		}
		break;
	case 115: /* DREM */
		assert(l && r);
		{	/* reg: DREM(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 58;
			}
		}
		break;
	case 116: /* INEG */
		{
			static struct burm_state z = { 116, 0, 0,
				{	0,
					100,	/* reg: INEG */
				},{
					59,	/* reg: INEG */
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
					60,	/* reg: LNEG */
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
					61,	/* reg: FNEG */
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
					62,	/* reg: DNEG */
				}
			};
			return &z;
		}
	case 120: /* ISHL */
		assert(l && r);
		{	/* reg: ISHL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 63;
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
			}
		}
		break;
	case 122: /* ISHR */
		assert(l && r);
		{	/* reg: ISHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 65;
			}
		}
		break;
	case 123: /* LSHR */
		assert(l && r);
		{	/* reg: LSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 66;
			}
		}
		break;
	case 124: /* IUSHR */
		assert(l && r);
		{	/* reg: IUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 67;
			}
		}
		break;
	case 125: /* LUSHR */
		assert(l && r);
		{	/* reg: LUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 68;
			}
		}
		break;
	case 126: /* IAND */
		assert(l && r);
		{	/* reg: IAND(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 69;
			}
		}
		break;
	case 127: /* LAND */
		assert(l && r);
		{	/* reg: LAND(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 70;
			}
		}
		break;
	case 128: /* IOR */
		assert(l && r);
		{	/* reg: IOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 71;
			}
		}
		break;
	case 129: /* LOR */
		assert(l && r);
		{	/* reg: LOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 72;
			}
		}
		break;
	case 130: /* IXOR */
		assert(l && r);
		{	/* reg: IXOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 73;
			}
		}
		break;
	case 131: /* LXOR */
		assert(l && r);
		{	/* reg: LXOR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 74;
			}
		}
		break;
	case 132: /* IINC */
		{
			static struct burm_state z = { 132, 0, 0,
				{	0,
					100,	/* reg: IINC */
				},{
					75,	/* reg: IINC */
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
					76,	/* reg: I2L */
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
					77,	/* reg: I2F */
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
					78,	/* reg: I2D */
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
					79,	/* reg: L2I */
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
					80,	/* reg: L2F */
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
					81,	/* reg: L2D */
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
					82,	/* reg: F2I */
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
					83,	/* reg: F2L */
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
					84,	/* reg: F2D */
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
					85,	/* reg: D2I */
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
					86,	/* reg: D2L */
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
					87,	/* reg: D2F */
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
					88,	/* reg: INT2BYTE */
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
					89,	/* reg: INT2CHAR */
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
					90,	/* reg: INT2SHORT */
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
					91,	/* reg: LCMP */
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
					92,	/* reg: FCMPL */
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
					93,	/* reg: FCMPG */
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
					94,	/* reg: DCMPL */
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
					95,	/* reg: DCMPG */
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
					96,	/* reg: IFEQ */
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
					97,	/* reg: IFNE */
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
					98,	/* reg: IFLT */
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
					99,	/* reg: IFGE */
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
					100,	/* reg: IFGT */
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
					101,	/* reg: IFLE */
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
					102,	/* reg: IF_ICMPEQ */
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
					103,	/* reg: IF_ICMPNE */
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
					104,	/* reg: IF_ICMPLT */
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
					105,	/* reg: IF_ICMPGE */
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
					106,	/* reg: IF_ICMPGT */
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
					107,	/* reg: IF_ICMPLE */
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
					108,	/* reg: IF_ACMPEQ */
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
					109,	/* reg: IF_ACMPNE */
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
					110,	/* reg: GOTO */
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
					111,	/* reg: JSR */
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
					112,	/* reg: RET */
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
					113,	/* reg: TABLESWITCH */
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
					114,	/* reg: LOOKUPSWITCH */
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
					115,	/* reg: IRETURN */
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
					116,	/* reg: LRETURN */
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
					117,	/* reg: FRETURN */
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
					118,	/* reg: DRETURN */
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
					119,	/* reg: ARETURN */
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
					120,	/* reg: RETURN */
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
					121,	/* reg: GETSTATIC */
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
					122,	/* reg: PUTSTATIC */
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
					123,	/* reg: GETFIELD */
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
					124,	/* reg: PUTFIELD */
				}
			};
			return &z;
		}
	case 182: /* INVOKEVIRTUAL */
		assert(l && r);
		{	/* reg: INVOKEVIRTUAL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 125;
			}
		}
		break;
	case 183: /* INVOKESPECIAL */
		assert(l && r);
		{	/* reg: INVOKESPECIAL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 126;
			}
		}
		break;
	case 184: /* INVOKESTATIC */
		assert(l && r);
		{	/* reg: INVOKESTATIC(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 127;
			}
		}
		break;
	case 185: /* INVOKEINTERFACE */
		assert(l && r);
		{	/* reg: INVOKEINTERFACE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 128;
			}
		}
		break;
	case 186: /* UNDEF186 */
		{
			static struct burm_state z = { 186, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
					0,
				}
			};
			return &z;
		}
	case 190: /* ARRAYLENGTH */
		{
			static struct burm_state z = { 190, 0, 0,
				{	0,
					32767,
				},{
					0,
				}
			};
			return &z;
		}
	case 191: /* ATHROW */
		assert(l);
		{	/* reg: ATHROW(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 132;
			}
		}
		break;
	case 192: /* CHECKCAST */
		{
			static struct burm_state z = { 192, 0, 0,
				{	0,
					100,	/* reg: CHECKCAST */
				},{
					129,	/* reg: CHECKCAST */
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
					130,	/* reg: INSTANCEOF */
				}
			};
			return &z;
		}
	case 194: /* MONITORENTER */
		{
			static struct burm_state z = { 194, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
					0,
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
					131,	/* reg: MULTIANEWARRAY */
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
					133,	/* reg: IFNULL */
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
					134,	/* reg: IFNONNULL */
				}
			};
			return &z;
		}
	case 200: /* UNDEF200 */
		{
			static struct burm_state z = { 200, 0, 0,
				{	0,
					32767,
				},{
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
				},{
					0,
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
					135,	/* reg: BREAKPOINT */
				}
			};
			return &z;
		}
	case 203: /* UNDEF203 */
		{
			static struct burm_state z = { 203, 0, 0,
				{	0,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
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
					136,	/* reg: GETEXCEPTION */
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
					137,	/* reg: PHI */
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
					138,	/* reg: INLINE_START */
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
					139,	/* reg: INLINE_END */
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
					140,	/* reg: INLINE_BODY */
				}
			};
			return &z;
		}
	case 254: /* UNDEF254 */
		{
			static struct burm_state z = { 254, 0, 0,
				{	0,
					32767,
				},{
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
				p->rule.burm_reg = 141;
			}
		}
		break;
	case 300: /* RESULT */
		{
			static struct burm_state z = { 300, 0, 0,
				{	0,
					100,	/* reg: RESULT */
				},{
					4,	/* reg: RESULT */
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
	case 140: /* reg: INLINE_BODY */
	case 139: /* reg: INLINE_END */
	case 138: /* reg: INLINE_START */
	case 137: /* reg: PHI */
	case 136: /* reg: GETEXCEPTION */
	case 135: /* reg: BREAKPOINT */
	case 134: /* reg: IFNONNULL */
	case 133: /* reg: IFNULL */
	case 131: /* reg: MULTIANEWARRAY */
	case 130: /* reg: INSTANCEOF */
	case 129: /* reg: CHECKCAST */
	case 124: /* reg: PUTFIELD */
	case 123: /* reg: GETFIELD */
	case 122: /* reg: PUTSTATIC */
	case 121: /* reg: GETSTATIC */
	case 120: /* reg: RETURN */
	case 119: /* reg: ARETURN */
	case 118: /* reg: DRETURN */
	case 117: /* reg: FRETURN */
	case 116: /* reg: LRETURN */
	case 115: /* reg: IRETURN */
	case 114: /* reg: LOOKUPSWITCH */
	case 113: /* reg: TABLESWITCH */
	case 112: /* reg: RET */
	case 111: /* reg: JSR */
	case 110: /* reg: GOTO */
	case 109: /* reg: IF_ACMPNE */
	case 108: /* reg: IF_ACMPEQ */
	case 107: /* reg: IF_ICMPLE */
	case 106: /* reg: IF_ICMPGT */
	case 105: /* reg: IF_ICMPGE */
	case 104: /* reg: IF_ICMPLT */
	case 103: /* reg: IF_ICMPNE */
	case 102: /* reg: IF_ICMPEQ */
	case 101: /* reg: IFLE */
	case 100: /* reg: IFGT */
	case 99: /* reg: IFGE */
	case 98: /* reg: IFLT */
	case 97: /* reg: IFNE */
	case 96: /* reg: IFEQ */
	case 95: /* reg: DCMPG */
	case 94: /* reg: DCMPL */
	case 93: /* reg: FCMPG */
	case 92: /* reg: FCMPL */
	case 91: /* reg: LCMP */
	case 90: /* reg: INT2SHORT */
	case 89: /* reg: INT2CHAR */
	case 88: /* reg: INT2BYTE */
	case 87: /* reg: D2F */
	case 86: /* reg: D2L */
	case 85: /* reg: D2I */
	case 84: /* reg: F2D */
	case 83: /* reg: F2L */
	case 82: /* reg: F2I */
	case 81: /* reg: L2D */
	case 80: /* reg: L2F */
	case 79: /* reg: L2I */
	case 78: /* reg: I2D */
	case 77: /* reg: I2F */
	case 76: /* reg: I2L */
	case 75: /* reg: IINC */
	case 62: /* reg: DNEG */
	case 61: /* reg: FNEG */
	case 60: /* reg: LNEG */
	case 59: /* reg: INEG */
	case 38: /* reg: SASTORE */
	case 37: /* reg: CASTORE */
	case 36: /* reg: BASTORE */
	case 35: /* reg: AASTORE */
	case 34: /* reg: DASTORE */
	case 33: /* reg: FASTORE */
	case 32: /* reg: LASTORE */
	case 31: /* reg: IASTORE */
	case 30: /* reg: ASTORE */
	case 29: /* reg: DSTORE */
	case 28: /* reg: FSTORE */
	case 27: /* reg: LSTORE */
	case 26: /* reg: ISTORE */
	case 25: /* reg: SALOAD */
	case 24: /* reg: CALOAD */
	case 23: /* reg: BALOAD */
	case 22: /* reg: AALOAD */
	case 21: /* reg: DALOAD */
	case 20: /* reg: FALOAD */
	case 19: /* reg: LALOAD */
	case 18: /* reg: IALOAD */
	case 17: /* reg: ALOAD */
	case 16: /* reg: DLOAD */
	case 15: /* reg: FLOAD */
	case 14: /* reg: LLOAD */
	case 13: /* reg: ILOAD */
	case 12: /* reg: MOVE */
	case 11: /* reg: COPY */
	case 9: /* reg: DCONST */
	case 8: /* reg: FCONST */
	case 7: /* reg: LCONST */
	case 6: /* reg: ICONST */
	case 5: /* reg: ACONST */
	case 4: /* reg: RESULT */
	case 3: /* reg: POP2 */
	case 2: /* reg: POP */
	case 1: /* reg: NOP */
		break;
	case 132: /* reg: ATHROW(reg) */
	case 10: /* reg: CHECKNULL(reg) */
		kids[0] = LEFT_CHILD(p);
		break;
	case 141: /* reg: BUILTIN(reg,reg) */
	case 128: /* reg: INVOKEINTERFACE(reg,reg) */
	case 127: /* reg: INVOKESTATIC(reg,reg) */
	case 126: /* reg: INVOKESPECIAL(reg,reg) */
	case 125: /* reg: INVOKEVIRTUAL(reg,reg) */
	case 74: /* reg: LXOR(reg,reg) */
	case 73: /* reg: IXOR(reg,reg) */
	case 72: /* reg: LOR(reg,reg) */
	case 71: /* reg: IOR(reg,reg) */
	case 70: /* reg: LAND(reg,reg) */
	case 69: /* reg: IAND(reg,reg) */
	case 68: /* reg: LUSHR(reg,reg) */
	case 67: /* reg: IUSHR(reg,reg) */
	case 66: /* reg: LSHR(reg,reg) */
	case 65: /* reg: ISHR(reg,reg) */
	case 64: /* reg: LSHL(reg,reg) */
	case 63: /* reg: ISHL(reg,reg) */
	case 58: /* reg: DREM(reg,reg) */
	case 57: /* reg: FREM(reg,reg) */
	case 56: /* reg: LREM(reg,reg) */
	case 55: /* reg: IREM(reg,reg) */
	case 54: /* reg: DDIV(reg,reg) */
	case 53: /* reg: FDIV(reg,reg) */
	case 52: /* reg: LDIV(reg,reg) */
	case 51: /* reg: IDIV(reg,reg) */
	case 50: /* reg: DMUL(reg,reg) */
	case 49: /* reg: FMUL(reg,reg) */
	case 48: /* reg: LMUL(reg,reg) */
	case 47: /* reg: IMUL(reg,reg) */
	case 46: /* reg: DSUB(reg,reg) */
	case 45: /* reg: FSUB(reg,reg) */
	case 44: /* reg: LSUB(reg,reg) */
	case 43: /* reg: ISUB(reg,reg) */
	case 42: /* reg: DADD(reg,reg) */
	case 41: /* reg: FADD(reg,reg) */
	case 40: /* reg: LADD(reg,reg) */
	case 39: /* reg: IADD(reg,reg) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = RIGHT_CHILD(p);
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
  }
  burm_kids (bnode, ruleNo, kids);
  for (i = 0; nts[i]; i++)
    burm_reduce (kids[i], nts[i]);    /* reduce kids */


  switch (ruleNo) {
  case 1:
    codegen_nop(bnode);
    break;
  case 2:
    codegen_nop(bnode);
    break;
  case 3:
    codegen_nop(bnode);
    break;
  case 4:
    codegen_emit_result(bnode);
    break;
  case 5:
    codegen_emit_instruction(bnode);
    break;
  case 6:
    codegen_emit_iconst(bnode);
    break;
  case 7:
    codegen_emit_lconst(bnode);
    break;
  case 8:
    codegen_emit_instruction(bnode);
    break;
  case 9:
    codegen_emit_instruction(bnode);
    break;
  case 10:
    codegen_emit_checknull(bnode);
    break;
  case 11:
    codegen_emit_copy(bnode);
    break;
  case 12:
    codegen_emit_copy(bnode);
    break;
  case 13:
    codegen_emit_copy(bnode);
    break;
  case 14:
    codegen_emit_copy(bnode);
    break;
  case 15:
    codegen_emit_copy(bnode);
    break;
  case 16:
    codegen_emit_copy(bnode);
    break;
  case 17:
    codegen_emit_copy(bnode);
    break;
  case 18:
    codegen_emit_instruction(bnode);
    break;
  case 19:
    codegen_emit_instruction(bnode);
    break;
  case 20:
    codegen_emit_instruction(bnode);
    break;
  case 21:
    codegen_emit_instruction(bnode);
    break;
  case 22:
    codegen_emit_instruction(bnode);
    break;
  case 23:
    codegen_emit_instruction(bnode);
    break;
  case 24:
    codegen_emit_instruction(bnode);
    break;
  case 25:
    codegen_emit_instruction(bnode);
    break;
  case 26:
    codegen_emit_instruction(bnode);
    break;
  case 27:
    codegen_emit_copy(bnode);
    break;
  case 28:
    codegen_emit_copy(bnode);
    break;
  case 29:
    codegen_emit_copy(bnode);
    break;
  case 30:
    codegen_emit_astore(bnode);
    break;
  case 31:
    codegen_emit_instruction(bnode);
    break;
  case 32:
    codegen_emit_instruction(bnode);
    break;
  case 33:
    codegen_emit_instruction(bnode);
    break;
  case 34:
    codegen_emit_instruction(bnode);
    break;
  case 35:
    codegen_emit_instruction(bnode);
    break;
  case 36:
    codegen_emit_instruction(bnode);
    break;
  case 37:
    codegen_emit_instruction(bnode);
    break;
  case 38:
    codegen_emit_instruction(bnode);
    break;
  case 39:
    codegen_emit_instruction(bnode);
    break;
  case 40:
    codegen_emit_instruction(bnode);
    break;
  case 41:
    codegen_emit_instruction(bnode);
    break;
  case 42:
    codegen_emit_instruction(bnode);
    break;
  case 43:
    codegen_emit_instruction(bnode);
    break;
  case 44:
    codegen_emit_instruction(bnode);
    break;
  case 45:
    codegen_emit_instruction(bnode);
    break;
  case 46:
    codegen_emit_instruction(bnode);
    break;
  case 47:
    codegen_emit_instruction(bnode);
    break;
  case 48:
    codegen_emit_instruction(bnode);
    break;
  case 49:
    codegen_emit_instruction(bnode);
    break;
  case 50:
    codegen_emit_instruction(bnode);
    break;
  case 51:
    codegen_emit_instruction(bnode);
    break;
  case 52:
    codegen_emit_instruction(bnode);
    break;
  case 53:
    codegen_emit_instruction(bnode);
    break;
  case 54:
    codegen_emit_instruction(bnode);
    break;
  case 55:
    codegen_emit_instruction(bnode);
    break;
  case 56:
    codegen_emit_instruction(bnode);
    break;
  case 57:
    codegen_emit_instruction(bnode);
    break;
  case 58:
    codegen_emit_instruction(bnode);
    break;
  case 59:
    codegen_emit_instruction(bnode);
    break;
  case 60:
    codegen_emit_instruction(bnode);
    break;
  case 61:
    codegen_emit_instruction(bnode);
    break;
  case 62:
    codegen_emit_instruction(bnode);
    break;
  case 63:
    codegen_emit_instruction(bnode);
    break;
  case 64:
    codegen_emit_instruction(bnode);
    break;
  case 65:
    codegen_emit_instruction(bnode);
    break;
  case 66:
    codegen_emit_instruction(bnode);
    break;
  case 67:
    codegen_emit_instruction(bnode);
    break;
  case 68:
    codegen_emit_instruction(bnode);
    break;
  case 69:
    codegen_emit_instruction(bnode);
    break;
  case 70:
    codegen_emit_instruction(bnode);
    break;
  case 71:
    codegen_emit_instruction(bnode);
    break;
  case 72:
    codegen_emit_instruction(bnode);
    break;
  case 73:
    codegen_emit_instruction(bnode);
    break;
  case 74:
    codegen_emit_instruction(bnode);
    break;
  case 75:
    codegen_emit_instruction(bnode);
    break;
  case 76:
    codegen_emit_instruction(bnode);
    break;
  case 77:
    codegen_emit_instruction(bnode);
    break;
  case 78:
    codegen_emit_instruction(bnode);
    break;
  case 79:
    codegen_emit_instruction(bnode);
    break;
  case 80:
    codegen_emit_instruction(bnode);
    break;
  case 81:
    codegen_emit_instruction(bnode);
    break;
  case 82:
    codegen_emit_instruction(bnode);
    break;
  case 83:
    codegen_emit_instruction(bnode);
    break;
  case 84:
    codegen_emit_instruction(bnode);
    break;
  case 85:
    codegen_emit_instruction(bnode);
    break;
  case 86:
    codegen_emit_instruction(bnode);
    break;
  case 87:
    codegen_emit_instruction(bnode);
    break;
  case 88:
    codegen_emit_instruction(bnode);
    break;
  case 89:
    codegen_emit_instruction(bnode);
    break;
  case 90:
    codegen_emit_instruction(bnode);
    break;
  case 91:
    codegen_emit_instruction(bnode);
    break;
  case 92:
    codegen_emit_instruction(bnode);
    break;
  case 93:
    codegen_emit_instruction(bnode);
    break;
  case 94:
    codegen_emit_instruction(bnode);
    break;
  case 95:
    codegen_emit_instruction(bnode);
    break;
  case 96:
    codegen_emit_branch(bnode);
    break;
  case 97:
    codegen_emit_branch(bnode);
    break;
  case 98:
    codegen_emit_branch(bnode);
    break;
  case 99:
    codegen_emit_branch(bnode);
    break;
  case 100:
    codegen_emit_branch(bnode);
    break;
  case 101:
    codegen_emit_branch(bnode);
    break;
  case 102:
    codegen_emit_branch(bnode);
    break;
  case 103:
    codegen_emit_branch(bnode);
    break;
  case 104:
    codegen_emit_branch(bnode);
    break;
  case 105:
    codegen_emit_branch(bnode);
    break;
  case 106:
    codegen_emit_branch(bnode);
    break;
  case 107:
    codegen_emit_branch(bnode);
    break;
  case 108:
    codegen_emit_branch(bnode);
    break;
  case 109:
    codegen_emit_branch(bnode);
    break;
  case 110:
    codegen_emit_jump(bnode);
    break;
  case 111:
    codegen_emit_jump(bnode);
    break;
  case 112:
    codegen_emit_jump(bnode);
    break;
  case 113:
    codegen_emit_instruction(bnode);
    break;
  case 114:
    codegen_emit_lookup(bnode);
    break;
  case 115:
    codegen_emit_return(bnode);
    break;
  case 116:
    codegen_emit_return(bnode);
    break;
  case 117:
    codegen_emit_return(bnode);
    break;
  case 118:
    codegen_emit_return(bnode);
    break;
  case 119:
    codegen_emit_return(bnode);
    break;
  case 120:
    codegen_emit_return(bnode);
    break;
  case 121:
    codegen_emit_getstatic(bnode);
    break;
  case 122:
    codegen_emit_putstatic(bnode);
    break;
  case 123:
    codegen_emit_instruction(bnode);
    break;
  case 124:
    codegen_emit_instruction(bnode);
    break;
  case 125:
    codegen_emit_invoke(bnode);
    break;
  case 126:
    codegen_emit_invoke(bnode);
    break;
  case 127:
    codegen_emit_invoke(bnode);
    break;
  case 128:
    codegen_emit_invoke(bnode);
    break;
  case 129:
    codegen_emit_instruction(bnode);
    break;
  case 130:
    codegen_emit_instruction(bnode);
    break;
  case 131:
    codegen_emit_instruction(bnode);
    break;
  case 132:
    codegen_emit_throw(bnode);
    break;
  case 133:
    codegen_emit_ifnull(bnode);
    break;
  case 134:
    codegen_emit_ifnull(bnode);
    break;
  case 135:
    codegen_emit_breakpoint(bnode);
    break;
  case 136:
    codegen_emit_getexception(bnode);
    break;
  case 137:
    codegen_emit_phi(bnode);
    break;
  case 138:
    codegen_emit_inline_start(bnode);
    break;
  case 139:
    codegen_emit_inline_end(bnode);
    break;
  case 140:
    codegen_emit_inline_body(bnode);
    break;
  case 141:
    codegen_emit_builtin(bnode);
    break;
  }
}
