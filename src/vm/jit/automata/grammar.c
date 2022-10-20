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

#define burm_root_NT 1
#define burm_reg_NT 2
int burm_max_nt = 2;

char *burm_ntname[] = {
	0,
	"root",
	"reg",
	0
};

struct burm_state {
	int op;
	STATEPTR_TYPE left, right;
	short cost[3];
	struct {
		unsigned burm_root:7;
		unsigned burm_reg:7;
	} rule;
};

static short burm_nts_0[] = { burm_reg_NT, 0 };
static short burm_nts_1[] = { 0 };
static short burm_nts_2[] = { burm_reg_NT, burm_reg_NT, 0 };

short *burm_nts[] = {
	0,	/* 0 */
	burm_nts_0,	/* 1 */
	burm_nts_1,	/* 2 */
	burm_nts_1,	/* 3 */
	burm_nts_1,	/* 4 */
	burm_nts_1,	/* 5 */
	burm_nts_1,	/* 6 */
	burm_nts_1,	/* 7 */
	burm_nts_1,	/* 8 */
	burm_nts_1,	/* 9 */
	burm_nts_1,	/* 10 */
	burm_nts_0,	/* 11 */
	burm_nts_1,	/* 12 */
	burm_nts_1,	/* 13 */
	burm_nts_1,	/* 14 */
	burm_nts_1,	/* 15 */
	burm_nts_1,	/* 16 */
	burm_nts_1,	/* 17 */
	burm_nts_1,	/* 18 */
	burm_nts_2,	/* 19 */
	burm_nts_2,	/* 20 */
	burm_nts_2,	/* 21 */
	burm_nts_2,	/* 22 */
	burm_nts_2,	/* 23 */
	burm_nts_2,	/* 24 */
	burm_nts_2,	/* 25 */
	burm_nts_2,	/* 26 */
	burm_nts_0,	/* 27 */
	burm_nts_0,	/* 28 */
	burm_nts_0,	/* 29 */
	burm_nts_0,	/* 30 */
	burm_nts_0,	/* 31 */
	burm_nts_1,	/* 32 */
	burm_nts_1,	/* 33 */
	burm_nts_1,	/* 34 */
	burm_nts_1,	/* 35 */
	burm_nts_1,	/* 36 */
	burm_nts_1,	/* 37 */
	burm_nts_1,	/* 38 */
	burm_nts_1,	/* 39 */
	burm_nts_2,	/* 40 */
	burm_nts_0,	/* 41 */
	burm_nts_0,	/* 42 */
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
	burm_nts_2,	/* 59 */
	burm_nts_2,	/* 60 */
	burm_nts_2,	/* 61 */
	burm_nts_0,	/* 62 */
	burm_nts_0,	/* 63 */
	burm_nts_0,	/* 64 */
	burm_nts_0,	/* 65 */
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
	burm_nts_1,	/* 78 */
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
	burm_nts_2,	/* 94 */
	burm_nts_2,	/* 95 */
	burm_nts_2,	/* 96 */
	burm_nts_2,	/* 97 */
	burm_nts_2,	/* 98 */
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
	burm_nts_2,	/* 113 */
	burm_nts_2,	/* 114 */
	burm_nts_2,	/* 115 */
	burm_nts_2,	/* 116 */
	burm_nts_2,	/* 117 */
	burm_nts_2,	/* 118 */
	burm_nts_2,	/* 119 */
	burm_nts_2,	/* 120 */
	burm_nts_2,	/* 121 */
	burm_nts_2,	/* 122 */
	burm_nts_2,	/* 123 */
	burm_nts_2,	/* 124 */
	burm_nts_2,	/* 125 */
	burm_nts_2,	/* 126 */
	burm_nts_1,	/* 127 */
	burm_nts_1,	/* 128 */
	burm_nts_1,	/* 129 */
	burm_nts_0,	/* 130 */
	burm_nts_0,	/* 131 */
	burm_nts_0,	/* 132 */
	burm_nts_0,	/* 133 */
	burm_nts_0,	/* 134 */
	burm_nts_0,	/* 135 */
	burm_nts_0,	/* 136 */
	burm_nts_1,	/* 137 */
	burm_nts_1,	/* 138 */
	burm_nts_0,	/* 139 */
	burm_nts_0,	/* 140 */
	burm_nts_2,	/* 141 */
	burm_nts_2,	/* 142 */
	burm_nts_2,	/* 143 */
	burm_nts_2,	/* 144 */
	burm_nts_2,	/* 145 */
	burm_nts_0,	/* 146 */
	burm_nts_0,	/* 147 */
	burm_nts_1,	/* 148 */
	burm_nts_0,	/* 149 */
	burm_nts_0,	/* 150 */
	burm_nts_0,	/* 151 */
	burm_nts_0,	/* 152 */
	burm_nts_1,	/* 153 */
	burm_nts_1,	/* 154 */
	burm_nts_1,	/* 155 */
	burm_nts_1,	/* 156 */
	burm_nts_1,	/* 157 */
	burm_nts_1,	/* 158 */
	burm_nts_2,	/* 159 */
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
	1,	/* 59=IF_LEQ */
	1,	/* 60=IF_LNE */
	1,	/* 61=IF_LLT */
	1,	/* 62=IF_LGE */
	1,	/* 63=IF_LGT */
	1,	/* 64=IF_LLE */
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
	0,	/* 197=MULTIANEWARRAY */
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
	{ 100 },	/* 1 = root: reg */
	{ 100 },	/* 2 = reg: NOP */
	{ 100 },	/* 3 = root: POP */
	{ 100 },	/* 4 = root: POP2 */
	{ 100 },	/* 5 = reg: RESULT */
	{ 100 },	/* 6 = reg: ACONST */
	{ 100 },	/* 7 = reg: ICONST */
	{ 100 },	/* 8 = reg: LCONST */
	{ 100 },	/* 9 = reg: FCONST */
	{ 100 },	/* 10 = reg: DCONST */
	{ 100 },	/* 11 = reg: CHECKNULL(reg) */
	{ 100 },	/* 12 = reg: COPY */
	{ 100 },	/* 13 = reg: MOVE */
	{ 100 },	/* 14 = reg: ILOAD */
	{ 100 },	/* 15 = reg: LLOAD */
	{ 100 },	/* 16 = reg: FLOAD */
	{ 100 },	/* 17 = reg: DLOAD */
	{ 100 },	/* 18 = reg: ALOAD */
	{ 100 },	/* 19 = reg: IALOAD(reg,reg) */
	{ 100 },	/* 20 = reg: LALOAD(reg,reg) */
	{ 100 },	/* 21 = reg: FALOAD(reg,reg) */
	{ 100 },	/* 22 = reg: DALOAD(reg,reg) */
	{ 100 },	/* 23 = reg: AALOAD(reg,reg) */
	{ 100 },	/* 24 = reg: BALOAD(reg,reg) */
	{ 100 },	/* 25 = reg: CALOAD(reg,reg) */
	{ 100 },	/* 26 = reg: SALOAD(reg,reg) */
	{ 100 },	/* 27 = root: ISTORE(reg) */
	{ 100 },	/* 28 = root: LSTORE(reg) */
	{ 100 },	/* 29 = root: FSTORE(reg) */
	{ 100 },	/* 30 = root: DSTORE(reg) */
	{ 100 },	/* 31 = root: ASTORE(reg) */
	{ 100 },	/* 32 = root: IASTORE */
	{ 100 },	/* 33 = root: LASTORE */
	{ 100 },	/* 34 = root: FASTORE */
	{ 100 },	/* 35 = root: DASTORE */
	{ 100 },	/* 36 = root: AASTORE */
	{ 100 },	/* 37 = root: BASTORE */
	{ 100 },	/* 38 = root: CASTORE */
	{ 100 },	/* 39 = root: SASTORE */
	{ 100 },	/* 40 = reg: IADD(reg,reg) */
	{ 10 },	/* 41 = reg: IADD(reg,ICONST) */
	{ 10 },	/* 42 = reg: IADD(ICONST,reg) */
	{ 100 },	/* 43 = reg: LADD(reg,reg) */
	{ 100 },	/* 44 = reg: FADD(reg,reg) */
	{ 100 },	/* 45 = reg: DADD(reg,reg) */
	{ 100 },	/* 46 = reg: ISUB(reg,reg) */
	{ 100 },	/* 47 = reg: LSUB(reg,reg) */
	{ 100 },	/* 48 = reg: FSUB(reg,reg) */
	{ 100 },	/* 49 = reg: DSUB(reg,reg) */
	{ 100 },	/* 50 = reg: IMUL(reg,reg) */
	{ 100 },	/* 51 = reg: LMUL(reg,reg) */
	{ 100 },	/* 52 = reg: FMUL(reg,reg) */
	{ 100 },	/* 53 = reg: DMUL(reg,reg) */
	{ 100 },	/* 54 = reg: IDIV(reg,reg) */
	{ 100 },	/* 55 = reg: LDIV(reg,reg) */
	{ 100 },	/* 56 = reg: FDIV(reg,reg) */
	{ 100 },	/* 57 = reg: DDIV(reg,reg) */
	{ 100 },	/* 58 = reg: IREM(reg,reg) */
	{ 100 },	/* 59 = reg: LREM(reg,reg) */
	{ 100 },	/* 60 = reg: FREM(reg,reg) */
	{ 100 },	/* 61 = reg: DREM(reg,reg) */
	{ 100 },	/* 62 = reg: INEG(reg) */
	{ 100 },	/* 63 = reg: LNEG(reg) */
	{ 100 },	/* 64 = reg: FNEG(reg) */
	{ 100 },	/* 65 = reg: DNEG(reg) */
	{ 100 },	/* 66 = reg: ISHL(reg,reg) */
	{ 100 },	/* 67 = reg: LSHL(reg,reg) */
	{ 100 },	/* 68 = reg: ISHR(reg,reg) */
	{ 100 },	/* 69 = reg: LSHR(reg,reg) */
	{ 100 },	/* 70 = reg: IUSHR(reg,reg) */
	{ 100 },	/* 71 = reg: LUSHR(reg,reg) */
	{ 100 },	/* 72 = reg: IAND(reg,reg) */
	{ 100 },	/* 73 = reg: LAND(reg,reg) */
	{ 100 },	/* 74 = reg: IOR(reg,reg) */
	{ 100 },	/* 75 = reg: LOR(reg,reg) */
	{ 100 },	/* 76 = reg: IXOR(reg,reg) */
	{ 100 },	/* 77 = reg: LXOR(reg,reg) */
	{ 100 },	/* 78 = root: IINC */
	{ 100 },	/* 79 = reg: I2L(reg) */
	{ 100 },	/* 80 = reg: I2F(reg) */
	{ 100 },	/* 81 = reg: I2D(reg) */
	{ 100 },	/* 82 = reg: L2I(reg) */
	{ 100 },	/* 83 = reg: L2F(reg) */
	{ 100 },	/* 84 = reg: L2D(reg) */
	{ 100 },	/* 85 = reg: F2I(reg) */
	{ 100 },	/* 86 = reg: F2L(reg) */
	{ 100 },	/* 87 = reg: F2D(reg) */
	{ 100 },	/* 88 = reg: D2I(reg) */
	{ 100 },	/* 89 = reg: D2L(reg) */
	{ 100 },	/* 90 = reg: D2F(reg) */
	{ 100 },	/* 91 = reg: INT2BYTE(reg) */
	{ 100 },	/* 92 = reg: INT2CHAR(reg) */
	{ 100 },	/* 93 = reg: INT2SHORT(reg) */
	{ 100 },	/* 94 = reg: LCMP(reg,reg) */
	{ 100 },	/* 95 = reg: FCMPL(reg,reg) */
	{ 100 },	/* 96 = reg: FCMPG(reg,reg) */
	{ 100 },	/* 97 = reg: DCMPL(reg,reg) */
	{ 100 },	/* 98 = reg: DCMPG(reg,reg) */
	{ 100 },	/* 99 = root: IFEQ(reg) */
	{ 100 },	/* 100 = root: IFEQ(reg) */
	{ 100 },	/* 101 = root: IFNE(reg) */
	{ 100 },	/* 102 = root: IFLT(reg) */
	{ 100 },	/* 103 = root: IFGE(reg) */
	{ 100 },	/* 104 = root: IFGT(reg) */
	{ 100 },	/* 105 = root: IFLE(reg) */
	{ 100 },	/* 106 = root: IF_LEQ(reg) */
	{ 100 },	/* 107 = root: IF_LEQ(reg) */
	{ 100 },	/* 108 = root: IF_LNE(reg) */
	{ 100 },	/* 109 = root: IF_LLT(reg) */
	{ 100 },	/* 110 = root: IF_LGE(reg) */
	{ 100 },	/* 111 = root: IF_LGT(reg) */
	{ 100 },	/* 112 = root: IF_LLE(reg) */
	{ 100 },	/* 113 = root: IF_ICMPEQ(reg,reg) */
	{ 100 },	/* 114 = root: IF_ICMPNE(reg,reg) */
	{ 100 },	/* 115 = root: IF_ICMPLT(reg,reg) */
	{ 100 },	/* 116 = root: IF_ICMPGE(reg,reg) */
	{ 100 },	/* 117 = root: IF_ICMPGT(reg,reg) */
	{ 100 },	/* 118 = root: IF_ICMPLE(reg,reg) */
	{ 100 },	/* 119 = root: IF_LCMPEQ(reg,reg) */
	{ 100 },	/* 120 = root: IF_LCMPNE(reg,reg) */
	{ 100 },	/* 121 = root: IF_LCMPLT(reg,reg) */
	{ 100 },	/* 122 = root: IF_LCMPGE(reg,reg) */
	{ 100 },	/* 123 = root: IF_LCMPGT(reg,reg) */
	{ 100 },	/* 124 = root: IF_LCMPLE(reg,reg) */
	{ 100 },	/* 125 = root: IF_ACMPEQ(reg,reg) */
	{ 100 },	/* 126 = root: IF_ACMPNE(reg,reg) */
	{ 100 },	/* 127 = root: GOTO */
	{ 100 },	/* 128 = reg: JSR */
	{ 100 },	/* 129 = root: RET */
	{ 100 },	/* 130 = root: TABLESWITCH(reg) */
	{ 100 },	/* 131 = root: LOOKUPSWITCH(reg) */
	{ 100 },	/* 132 = root: IRETURN(reg) */
	{ 100 },	/* 133 = root: LRETURN(reg) */
	{ 100 },	/* 134 = root: FRETURN(reg) */
	{ 100 },	/* 135 = root: DRETURN(reg) */
	{ 100 },	/* 136 = root: ARETURN(reg) */
	{ 100 },	/* 137 = root: RETURN */
	{ 100 },	/* 138 = reg: GETSTATIC */
	{ 100 },	/* 139 = root: PUTSTATIC(reg) */
	{ 100 },	/* 140 = reg: GETFIELD(reg) */
	{ 100 },	/* 141 = root: PUTFIELD(reg,reg) */
	{ 100 },	/* 142 = reg: INVOKEVIRTUAL(reg,reg) */
	{ 100 },	/* 143 = reg: INVOKESPECIAL(reg,reg) */
	{ 100 },	/* 144 = reg: INVOKESTATIC(reg,reg) */
	{ 100 },	/* 145 = reg: INVOKEINTERFACE(reg,reg) */
	{ 100 },	/* 146 = reg: CHECKCAST(reg) */
	{ 100 },	/* 147 = reg: INSTANCEOF(reg) */
	{ 100 },	/* 148 = reg: MULTIANEWARRAY */
	{ 100 },	/* 149 = reg: ARRAYLENGTH(reg) */
	{ 100 },	/* 150 = root: ATHROW(reg) */
	{ 100 },	/* 151 = root: IFNULL(reg) */
	{ 100 },	/* 152 = root: IFNONNULL(reg) */
	{ 100 },	/* 153 = root: BREAKPOINT */
	{ 100 },	/* 154 = reg: GETEXCEPTION */
	{ 100 },	/* 155 = reg: PHI */
	{ 100 },	/* 156 = root: INLINE_START */
	{ 100 },	/* 157 = root: INLINE_END */
	{ 100 },	/* 158 = root: INLINE_BODY */
	{ 100 },	/* 159 = reg: BUILTIN(reg,reg) */
};

char *burm_string[] = {
	/* 0 */	0,
	/* 1 */	"root: reg",
	/* 2 */	"reg: NOP",
	/* 3 */	"root: POP",
	/* 4 */	"root: POP2",
	/* 5 */	"reg: RESULT",
	/* 6 */	"reg: ACONST",
	/* 7 */	"reg: ICONST",
	/* 8 */	"reg: LCONST",
	/* 9 */	"reg: FCONST",
	/* 10 */	"reg: DCONST",
	/* 11 */	"reg: CHECKNULL(reg)",
	/* 12 */	"reg: COPY",
	/* 13 */	"reg: MOVE",
	/* 14 */	"reg: ILOAD",
	/* 15 */	"reg: LLOAD",
	/* 16 */	"reg: FLOAD",
	/* 17 */	"reg: DLOAD",
	/* 18 */	"reg: ALOAD",
	/* 19 */	"reg: IALOAD(reg,reg)",
	/* 20 */	"reg: LALOAD(reg,reg)",
	/* 21 */	"reg: FALOAD(reg,reg)",
	/* 22 */	"reg: DALOAD(reg,reg)",
	/* 23 */	"reg: AALOAD(reg,reg)",
	/* 24 */	"reg: BALOAD(reg,reg)",
	/* 25 */	"reg: CALOAD(reg,reg)",
	/* 26 */	"reg: SALOAD(reg,reg)",
	/* 27 */	"root: ISTORE(reg)",
	/* 28 */	"root: LSTORE(reg)",
	/* 29 */	"root: FSTORE(reg)",
	/* 30 */	"root: DSTORE(reg)",
	/* 31 */	"root: ASTORE(reg)",
	/* 32 */	"root: IASTORE",
	/* 33 */	"root: LASTORE",
	/* 34 */	"root: FASTORE",
	/* 35 */	"root: DASTORE",
	/* 36 */	"root: AASTORE",
	/* 37 */	"root: BASTORE",
	/* 38 */	"root: CASTORE",
	/* 39 */	"root: SASTORE",
	/* 40 */	"reg: IADD(reg,reg)",
	/* 41 */	"reg: IADD(reg,ICONST)",
	/* 42 */	"reg: IADD(ICONST,reg)",
	/* 43 */	"reg: LADD(reg,reg)",
	/* 44 */	"reg: FADD(reg,reg)",
	/* 45 */	"reg: DADD(reg,reg)",
	/* 46 */	"reg: ISUB(reg,reg)",
	/* 47 */	"reg: LSUB(reg,reg)",
	/* 48 */	"reg: FSUB(reg,reg)",
	/* 49 */	"reg: DSUB(reg,reg)",
	/* 50 */	"reg: IMUL(reg,reg)",
	/* 51 */	"reg: LMUL(reg,reg)",
	/* 52 */	"reg: FMUL(reg,reg)",
	/* 53 */	"reg: DMUL(reg,reg)",
	/* 54 */	"reg: IDIV(reg,reg)",
	/* 55 */	"reg: LDIV(reg,reg)",
	/* 56 */	"reg: FDIV(reg,reg)",
	/* 57 */	"reg: DDIV(reg,reg)",
	/* 58 */	"reg: IREM(reg,reg)",
	/* 59 */	"reg: LREM(reg,reg)",
	/* 60 */	"reg: FREM(reg,reg)",
	/* 61 */	"reg: DREM(reg,reg)",
	/* 62 */	"reg: INEG(reg)",
	/* 63 */	"reg: LNEG(reg)",
	/* 64 */	"reg: FNEG(reg)",
	/* 65 */	"reg: DNEG(reg)",
	/* 66 */	"reg: ISHL(reg,reg)",
	/* 67 */	"reg: LSHL(reg,reg)",
	/* 68 */	"reg: ISHR(reg,reg)",
	/* 69 */	"reg: LSHR(reg,reg)",
	/* 70 */	"reg: IUSHR(reg,reg)",
	/* 71 */	"reg: LUSHR(reg,reg)",
	/* 72 */	"reg: IAND(reg,reg)",
	/* 73 */	"reg: LAND(reg,reg)",
	/* 74 */	"reg: IOR(reg,reg)",
	/* 75 */	"reg: LOR(reg,reg)",
	/* 76 */	"reg: IXOR(reg,reg)",
	/* 77 */	"reg: LXOR(reg,reg)",
	/* 78 */	"root: IINC",
	/* 79 */	"reg: I2L(reg)",
	/* 80 */	"reg: I2F(reg)",
	/* 81 */	"reg: I2D(reg)",
	/* 82 */	"reg: L2I(reg)",
	/* 83 */	"reg: L2F(reg)",
	/* 84 */	"reg: L2D(reg)",
	/* 85 */	"reg: F2I(reg)",
	/* 86 */	"reg: F2L(reg)",
	/* 87 */	"reg: F2D(reg)",
	/* 88 */	"reg: D2I(reg)",
	/* 89 */	"reg: D2L(reg)",
	/* 90 */	"reg: D2F(reg)",
	/* 91 */	"reg: INT2BYTE(reg)",
	/* 92 */	"reg: INT2CHAR(reg)",
	/* 93 */	"reg: INT2SHORT(reg)",
	/* 94 */	"reg: LCMP(reg,reg)",
	/* 95 */	"reg: FCMPL(reg,reg)",
	/* 96 */	"reg: FCMPG(reg,reg)",
	/* 97 */	"reg: DCMPL(reg,reg)",
	/* 98 */	"reg: DCMPG(reg,reg)",
	/* 99 */	"root: IFEQ(reg)",
	/* 100 */	"root: IFEQ(reg)",
	/* 101 */	"root: IFNE(reg)",
	/* 102 */	"root: IFLT(reg)",
	/* 103 */	"root: IFGE(reg)",
	/* 104 */	"root: IFGT(reg)",
	/* 105 */	"root: IFLE(reg)",
	/* 106 */	"root: IF_LEQ(reg)",
	/* 107 */	"root: IF_LEQ(reg)",
	/* 108 */	"root: IF_LNE(reg)",
	/* 109 */	"root: IF_LLT(reg)",
	/* 110 */	"root: IF_LGE(reg)",
	/* 111 */	"root: IF_LGT(reg)",
	/* 112 */	"root: IF_LLE(reg)",
	/* 113 */	"root: IF_ICMPEQ(reg,reg)",
	/* 114 */	"root: IF_ICMPNE(reg,reg)",
	/* 115 */	"root: IF_ICMPLT(reg,reg)",
	/* 116 */	"root: IF_ICMPGE(reg,reg)",
	/* 117 */	"root: IF_ICMPGT(reg,reg)",
	/* 118 */	"root: IF_ICMPLE(reg,reg)",
	/* 119 */	"root: IF_LCMPEQ(reg,reg)",
	/* 120 */	"root: IF_LCMPNE(reg,reg)",
	/* 121 */	"root: IF_LCMPLT(reg,reg)",
	/* 122 */	"root: IF_LCMPGE(reg,reg)",
	/* 123 */	"root: IF_LCMPGT(reg,reg)",
	/* 124 */	"root: IF_LCMPLE(reg,reg)",
	/* 125 */	"root: IF_ACMPEQ(reg,reg)",
	/* 126 */	"root: IF_ACMPNE(reg,reg)",
	/* 127 */	"root: GOTO",
	/* 128 */	"reg: JSR",
	/* 129 */	"root: RET",
	/* 130 */	"root: TABLESWITCH(reg)",
	/* 131 */	"root: LOOKUPSWITCH(reg)",
	/* 132 */	"root: IRETURN(reg)",
	/* 133 */	"root: LRETURN(reg)",
	/* 134 */	"root: FRETURN(reg)",
	/* 135 */	"root: DRETURN(reg)",
	/* 136 */	"root: ARETURN(reg)",
	/* 137 */	"root: RETURN",
	/* 138 */	"reg: GETSTATIC",
	/* 139 */	"root: PUTSTATIC(reg)",
	/* 140 */	"reg: GETFIELD(reg)",
	/* 141 */	"root: PUTFIELD(reg,reg)",
	/* 142 */	"reg: INVOKEVIRTUAL(reg,reg)",
	/* 143 */	"reg: INVOKESPECIAL(reg,reg)",
	/* 144 */	"reg: INVOKESTATIC(reg,reg)",
	/* 145 */	"reg: INVOKEINTERFACE(reg,reg)",
	/* 146 */	"reg: CHECKCAST(reg)",
	/* 147 */	"reg: INSTANCEOF(reg)",
	/* 148 */	"reg: MULTIANEWARRAY",
	/* 149 */	"reg: ARRAYLENGTH(reg)",
	/* 150 */	"root: ATHROW(reg)",
	/* 151 */	"root: IFNULL(reg)",
	/* 152 */	"root: IFNONNULL(reg)",
	/* 153 */	"root: BREAKPOINT",
	/* 154 */	"reg: GETEXCEPTION",
	/* 155 */	"reg: PHI",
	/* 156 */	"root: INLINE_START",
	/* 157 */	"root: INLINE_END",
	/* 158 */	"root: INLINE_BODY",
	/* 159 */	"reg: BUILTIN(reg,reg)",
};

static short burm_decode_root[] = {
	0,
	1,
	3,
	4,
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
	78,
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
	129,
	130,
	131,
	132,
	133,
	134,
	135,
	136,
	137,
	139,
	141,
	150,
	151,
	152,
	153,
	156,
	157,
	158,
};

static short burm_decode_reg[] = {
	0,
	2,
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
	128,
	138,
	140,
	142,
	143,
	144,
	145,
	146,
	147,
	148,
	149,
	154,
	155,
	159,
};

int burm_rule(STATEPTR_TYPE state, int goalnt) {
	burm_assert(goalnt >= 1 && goalnt <= 2, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	if (!state)
		return 0;
	switch (goalnt) {
	case burm_root_NT:
		return burm_decode_root[state->rule.burm_root];
	case burm_reg_NT:
		return burm_decode_reg[state->rule.burm_reg];
	default:
		burm_assert(0, PANIC("Bad goal nonterminal %d in burm_rule\n", goalnt));
	}
	return 0;
}

static void burm_closure_reg(STATEPTR_TYPE, int);

static void burm_closure_reg(STATEPTR_TYPE p, int c) {
	if (c + 100 < p->cost[burm_root_NT]) {
		p->cost[burm_root_NT] = c + 100;
		p->rule.burm_root = 1;
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
			32767;
	}
	switch (op) {
	case 0: /* NOP */
		{
			static struct burm_state z = { 0, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: NOP */
				},{
					1,	/* root: reg */
					1,	/* reg: NOP */
				}
			};
			return &z;
		}
	case 1: /* ACONST */
		{
			static struct burm_state z = { 1, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: ACONST */
				},{
					1,	/* root: reg */
					3,	/* reg: ACONST */
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
				p->rule.burm_reg = 8;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 3: /* ICONST */
		{
			static struct burm_state z = { 3, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: ICONST */
				},{
					1,	/* root: reg */
					4,	/* reg: ICONST */
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
					200,	/* root: reg */
					100,	/* reg: LCONST */
				},{
					1,	/* root: reg */
					5,	/* reg: LCONST */
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
				},{
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
					200,	/* root: reg */
					100,	/* reg: FCONST */
				},{
					1,	/* root: reg */
					6,	/* reg: FCONST */
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
				},{
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
				},{
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
					200,	/* root: reg */
					100,	/* reg: DCONST */
				},{
					1,	/* root: reg */
					7,	/* reg: DCONST */
				}
			};
			return &z;
		}
	case 15: /* COPY */
		{
			static struct burm_state z = { 15, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: COPY */
				},{
					1,	/* root: reg */
					9,	/* reg: COPY */
				}
			};
			return &z;
		}
	case 16: /* MOVE */
		{
			static struct burm_state z = { 16, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: MOVE */
				},{
					1,	/* root: reg */
					10,	/* reg: MOVE */
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
				},{
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
				},{
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
				},{
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
				},{
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
					200,	/* root: reg */
					100,	/* reg: ILOAD */
				},{
					1,	/* root: reg */
					11,	/* reg: ILOAD */
				}
			};
			return &z;
		}
	case 22: /* LLOAD */
		{
			static struct burm_state z = { 22, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: LLOAD */
				},{
					1,	/* root: reg */
					12,	/* reg: LLOAD */
				}
			};
			return &z;
		}
	case 23: /* FLOAD */
		{
			static struct burm_state z = { 23, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: FLOAD */
				},{
					1,	/* root: reg */
					13,	/* reg: FLOAD */
				}
			};
			return &z;
		}
	case 24: /* DLOAD */
		{
			static struct burm_state z = { 24, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: DLOAD */
				},{
					1,	/* root: reg */
					14,	/* reg: DLOAD */
				}
			};
			return &z;
		}
	case 25: /* ALOAD */
		{
			static struct burm_state z = { 25, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: ALOAD */
				},{
					1,	/* root: reg */
					15,	/* reg: ALOAD */
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				p->rule.burm_reg = 16;
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
				p->rule.burm_reg = 18;
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
				p->rule.burm_reg = 19;
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
				p->rule.burm_reg = 20;
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
				p->rule.burm_reg = 21;
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
				p->rule.burm_reg = 22;
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
				p->rule.burm_reg = 23;
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
				p->rule.burm_root = 4;
			}
		}
		break;
	case 55: /* LSTORE */
		assert(l);
		{	/* root: LSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 5;
			}
		}
		break;
	case 56: /* FSTORE */
		assert(l);
		{	/* root: FSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 6;
			}
		}
		break;
	case 57: /* DSTORE */
		assert(l);
		{	/* root: DSTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 7;
			}
		}
		break;
	case 58: /* ASTORE */
		assert(l);
		{	/* root: ASTORE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 8;
			}
		}
		break;
	case 59: /* IF_LEQ */
		assert(l);
		{	/* root: IF_LEQ(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 26;
			}
		}
		{	/* root: IF_LEQ(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 25;
			}
		}
		break;
	case 60: /* IF_LNE */
		assert(l);
		{	/* root: IF_LNE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 27;
			}
		}
		break;
	case 61: /* IF_LLT */
		assert(l);
		{	/* root: IF_LLT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 28;
			}
		}
		break;
	case 62: /* IF_LGE */
		assert(l);
		{	/* root: IF_LGE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 29;
			}
		}
		break;
	case 63: /* IF_LGT */
		assert(l);
		{	/* root: IF_LGT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 30;
			}
		}
		break;
	case 64: /* IF_LLE */
		assert(l);
		{	/* root: IF_LLE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 31;
			}
		}
		break;
	case 65: /* IF_LCMPEQ */
		assert(l && r);
		{	/* root: IF_LCMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 38;
			}
		}
		break;
	case 66: /* IF_LCMPNE */
		assert(l && r);
		{	/* root: IF_LCMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 39;
			}
		}
		break;
	case 67: /* IF_LCMPLT */
		assert(l && r);
		{	/* root: IF_LCMPLT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 40;
			}
		}
		break;
	case 68: /* IF_LCMPGE */
		assert(l && r);
		{	/* root: IF_LCMPGE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 41;
			}
		}
		break;
	case 69: /* IF_LCMPGT */
		assert(l && r);
		{	/* root: IF_LCMPGT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 42;
			}
		}
		break;
	case 70: /* IF_LCMPLE */
		assert(l && r);
		{	/* root: IF_LCMPLE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 43;
			}
		}
		break;
	case 71: /* UNDEF71 */
		{
			static struct burm_state z = { 71, 0, 0,
				{	0,
					32767,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					9,	/* root: IASTORE */
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
				},{
					10,	/* root: LASTORE */
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
				},{
					11,	/* root: FASTORE */
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
				},{
					12,	/* root: DASTORE */
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
				},{
					13,	/* root: AASTORE */
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
				},{
					14,	/* root: BASTORE */
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
				},{
					15,	/* root: CASTORE */
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
				},{
					16,	/* root: SASTORE */
					0,
				}
			};
			return &z;
		}
	case 87: /* POP */
		{
			static struct burm_state z = { 87, 0, 0,
				{	0,
					100,	/* root: POP */
					32767,
				},{
					2,	/* root: POP */
					0,
				}
			};
			return &z;
		}
	case 88: /* POP2 */
		{
			static struct burm_state z = { 88, 0, 0,
				{	0,
					100,	/* root: POP2 */
					32767,
				},{
					3,	/* root: POP2 */
					0,
				}
			};
			return &z;
		}
	case 89: /* DUP */
		{
			static struct burm_state z = { 89, 0, 0,
				{	0,
					32767,
					32767,
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
					0,
					0,
				}
			};
			return &z;
		}
	case 96: /* IADD */
		assert(l && r);
		if (	/* reg: IADD(ICONST,reg) */
			l->op == 3 /* ICONST */
		) {
			c = r->cost[burm_reg_NT] + 10;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 26;
				burm_closure_reg(p, c + 0);
			}
		}
		if (	/* reg: IADD(reg,ICONST) */
			r->op == 3 /* ICONST */
		) {
			c = l->cost[burm_reg_NT] + 10;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 25;
				burm_closure_reg(p, c + 0);
			}
		}
		{	/* reg: IADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 24;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 97: /* LADD */
		assert(l && r);
		{	/* reg: LADD(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 27;
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
				p->rule.burm_reg = 28;
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
				p->rule.burm_reg = 29;
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
				p->rule.burm_reg = 30;
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
				p->rule.burm_reg = 31;
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
				p->rule.burm_reg = 32;
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
				p->rule.burm_reg = 33;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 104: /* IMUL */
		assert(l && r);
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
				p->rule.burm_reg = 36;
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
				p->rule.burm_reg = 37;
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
				p->rule.burm_reg = 38;
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
				p->rule.burm_reg = 39;
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
				p->rule.burm_reg = 40;
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
				p->rule.burm_reg = 41;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 112: /* IREM */
		assert(l && r);
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
				p->rule.burm_reg = 44;
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
				p->rule.burm_reg = 45;
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
				p->rule.burm_reg = 46;
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
				p->rule.burm_reg = 47;
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
				p->rule.burm_reg = 48;
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
				p->rule.burm_reg = 49;
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
				p->rule.burm_reg = 50;
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
				p->rule.burm_reg = 51;
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
				p->rule.burm_reg = 52;
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
				p->rule.burm_reg = 53;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 124: /* IUSHR */
		assert(l && r);
		{	/* reg: IUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 54;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 125: /* LUSHR */
		assert(l && r);
		{	/* reg: LUSHR(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 55;
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
				},{
					17,	/* root: IINC */
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
				p->rule.burm_reg = 62;
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
				p->rule.burm_reg = 63;
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
				p->rule.burm_reg = 64;
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
				p->rule.burm_reg = 65;
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
				p->rule.burm_reg = 66;
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
				p->rule.burm_reg = 67;
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
				p->rule.burm_reg = 68;
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
				p->rule.burm_reg = 69;
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
				p->rule.burm_reg = 70;
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
				p->rule.burm_reg = 71;
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
				p->rule.burm_reg = 72;
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
				p->rule.burm_reg = 73;
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
				p->rule.burm_reg = 74;
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
				p->rule.burm_reg = 75;
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
				p->rule.burm_reg = 76;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 148: /* LCMP */
		assert(l && r);
		{	/* reg: LCMP(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 77;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 149: /* FCMPL */
		assert(l && r);
		{	/* reg: FCMPL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 78;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 150: /* FCMPG */
		assert(l && r);
		{	/* reg: FCMPG(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 79;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 151: /* DCMPL */
		assert(l && r);
		{	/* reg: DCMPL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 80;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 152: /* DCMPG */
		assert(l && r);
		{	/* reg: DCMPG(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 81;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 153: /* IFEQ */
		assert(l);
		{	/* root: IFEQ(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 19;
			}
		}
		{	/* root: IFEQ(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 18;
			}
		}
		break;
	case 154: /* IFNE */
		assert(l);
		{	/* root: IFNE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 20;
			}
		}
		break;
	case 155: /* IFLT */
		assert(l);
		{	/* root: IFLT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 21;
			}
		}
		break;
	case 156: /* IFGE */
		assert(l);
		{	/* root: IFGE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 22;
			}
		}
		break;
	case 157: /* IFGT */
		assert(l);
		{	/* root: IFGT(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 23;
			}
		}
		break;
	case 158: /* IFLE */
		assert(l);
		{	/* root: IFLE(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 24;
			}
		}
		break;
	case 159: /* IF_ICMPEQ */
		assert(l && r);
		{	/* root: IF_ICMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 32;
			}
		}
		break;
	case 160: /* IF_ICMPNE */
		assert(l && r);
		{	/* root: IF_ICMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 33;
			}
		}
		break;
	case 161: /* IF_ICMPLT */
		assert(l && r);
		{	/* root: IF_ICMPLT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 34;
			}
		}
		break;
	case 162: /* IF_ICMPGE */
		assert(l && r);
		{	/* root: IF_ICMPGE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 35;
			}
		}
		break;
	case 163: /* IF_ICMPGT */
		assert(l && r);
		{	/* root: IF_ICMPGT(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 36;
			}
		}
		break;
	case 164: /* IF_ICMPLE */
		assert(l && r);
		{	/* root: IF_ICMPLE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 37;
			}
		}
		break;
	case 165: /* IF_ACMPEQ */
		assert(l && r);
		{	/* root: IF_ACMPEQ(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 44;
			}
		}
		break;
	case 166: /* IF_ACMPNE */
		assert(l && r);
		{	/* root: IF_ACMPNE(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 45;
			}
		}
		break;
	case 167: /* GOTO */
		{
			static struct burm_state z = { 167, 0, 0,
				{	0,
					100,	/* root: GOTO */
					32767,
				},{
					46,	/* root: GOTO */
					0,
				}
			};
			return &z;
		}
	case 168: /* JSR */
		{
			static struct burm_state z = { 168, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: JSR */
				},{
					1,	/* root: reg */
					82,	/* reg: JSR */
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
				},{
					47,	/* root: RET */
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
				p->rule.burm_root = 48;
			}
		}
		break;
	case 171: /* LOOKUPSWITCH */
		assert(l);
		{	/* root: LOOKUPSWITCH(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 49;
			}
		}
		break;
	case 172: /* IRETURN */
		assert(l);
		{	/* root: IRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 50;
			}
		}
		break;
	case 173: /* LRETURN */
		assert(l);
		{	/* root: LRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 51;
			}
		}
		break;
	case 174: /* FRETURN */
		assert(l);
		{	/* root: FRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 52;
			}
		}
		break;
	case 175: /* DRETURN */
		assert(l);
		{	/* root: DRETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 53;
			}
		}
		break;
	case 176: /* ARETURN */
		assert(l);
		{	/* root: ARETURN(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 54;
			}
		}
		break;
	case 177: /* RETURN */
		{
			static struct burm_state z = { 177, 0, 0,
				{	0,
					100,	/* root: RETURN */
					32767,
				},{
					55,	/* root: RETURN */
					0,
				}
			};
			return &z;
		}
	case 178: /* GETSTATIC */
		{
			static struct burm_state z = { 178, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: GETSTATIC */
				},{
					1,	/* root: reg */
					83,	/* reg: GETSTATIC */
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
				p->rule.burm_root = 56;
			}
		}
		break;
	case 180: /* GETFIELD */
		assert(l);
		{	/* reg: GETFIELD(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 84;
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
				p->rule.burm_root = 57;
			}
		}
		break;
	case 182: /* INVOKEVIRTUAL */
		assert(l && r);
		{	/* reg: INVOKEVIRTUAL(reg,reg) */
			c = l->cost[burm_reg_NT] + r->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 85;
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
				p->rule.burm_reg = 86;
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
				p->rule.burm_reg = 87;
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
				p->rule.burm_reg = 88;
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
				},{
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
				},{
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
				},{
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
				},{
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
				p->rule.burm_reg = 92;
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
				p->rule.burm_root = 58;
			}
		}
		break;
	case 192: /* CHECKCAST */
		assert(l);
		{	/* reg: CHECKCAST(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_reg_NT]) {
				p->cost[burm_reg_NT] = c + 0;
				p->rule.burm_reg = 89;
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
				p->rule.burm_reg = 90;
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
				},{
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
				},{
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
				},{
					0,
					0,
				}
			};
			return &z;
		}
	case 197: /* MULTIANEWARRAY */
		{
			static struct burm_state z = { 197, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: MULTIANEWARRAY */
				},{
					1,	/* root: reg */
					91,	/* reg: MULTIANEWARRAY */
				}
			};
			return &z;
		}
	case 198: /* IFNULL */
		assert(l);
		{	/* root: IFNULL(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 59;
			}
		}
		break;
	case 199: /* IFNONNULL */
		assert(l);
		{	/* root: IFNONNULL(reg) */
			c = l->cost[burm_reg_NT] + 100;
			if (c + 0 < p->cost[burm_root_NT]) {
				p->cost[burm_root_NT] = c + 0;
				p->rule.burm_root = 60;
			}
		}
		break;
	case 200: /* UNDEF200 */
		{
			static struct burm_state z = { 200, 0, 0,
				{	0,
					32767,
					32767,
				},{
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
				},{
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
				},{
					61,	/* root: BREAKPOINT */
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
				},{
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
					200,	/* root: reg */
					100,	/* reg: GETEXCEPTION */
				},{
					1,	/* root: reg */
					93,	/* reg: GETEXCEPTION */
				}
			};
			return &z;
		}
	case 250: /* PHI */
		{
			static struct burm_state z = { 250, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: PHI */
				},{
					1,	/* root: reg */
					94,	/* reg: PHI */
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
				},{
					62,	/* root: INLINE_START */
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
				},{
					63,	/* root: INLINE_END */
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
				},{
					64,	/* root: INLINE_BODY */
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
				},{
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
				p->rule.burm_reg = 95;
				burm_closure_reg(p, c + 0);
			}
		}
		break;
	case 300: /* RESULT */
		{
			static struct burm_state z = { 300, 0, 0,
				{	0,
					200,	/* root: reg */
					100,	/* reg: RESULT */
				},{
					1,	/* root: reg */
					2,	/* reg: RESULT */
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
	case 1: /* root: reg */
		kids[0] = p;
		break;
	case 158: /* root: INLINE_BODY */
	case 157: /* root: INLINE_END */
	case 156: /* root: INLINE_START */
	case 155: /* reg: PHI */
	case 154: /* reg: GETEXCEPTION */
	case 153: /* root: BREAKPOINT */
	case 148: /* reg: MULTIANEWARRAY */
	case 138: /* reg: GETSTATIC */
	case 137: /* root: RETURN */
	case 129: /* root: RET */
	case 128: /* reg: JSR */
	case 127: /* root: GOTO */
	case 78: /* root: IINC */
	case 39: /* root: SASTORE */
	case 38: /* root: CASTORE */
	case 37: /* root: BASTORE */
	case 36: /* root: AASTORE */
	case 35: /* root: DASTORE */
	case 34: /* root: FASTORE */
	case 33: /* root: LASTORE */
	case 32: /* root: IASTORE */
	case 18: /* reg: ALOAD */
	case 17: /* reg: DLOAD */
	case 16: /* reg: FLOAD */
	case 15: /* reg: LLOAD */
	case 14: /* reg: ILOAD */
	case 13: /* reg: MOVE */
	case 12: /* reg: COPY */
	case 10: /* reg: DCONST */
	case 9: /* reg: FCONST */
	case 8: /* reg: LCONST */
	case 7: /* reg: ICONST */
	case 6: /* reg: ACONST */
	case 5: /* reg: RESULT */
	case 4: /* root: POP2 */
	case 3: /* root: POP */
	case 2: /* reg: NOP */
		break;
	case 152: /* root: IFNONNULL(reg) */
	case 151: /* root: IFNULL(reg) */
	case 150: /* root: ATHROW(reg) */
	case 149: /* reg: ARRAYLENGTH(reg) */
	case 147: /* reg: INSTANCEOF(reg) */
	case 146: /* reg: CHECKCAST(reg) */
	case 140: /* reg: GETFIELD(reg) */
	case 139: /* root: PUTSTATIC(reg) */
	case 136: /* root: ARETURN(reg) */
	case 135: /* root: DRETURN(reg) */
	case 134: /* root: FRETURN(reg) */
	case 133: /* root: LRETURN(reg) */
	case 132: /* root: IRETURN(reg) */
	case 131: /* root: LOOKUPSWITCH(reg) */
	case 130: /* root: TABLESWITCH(reg) */
	case 112: /* root: IF_LLE(reg) */
	case 111: /* root: IF_LGT(reg) */
	case 110: /* root: IF_LGE(reg) */
	case 109: /* root: IF_LLT(reg) */
	case 108: /* root: IF_LNE(reg) */
	case 107: /* root: IF_LEQ(reg) */
	case 106: /* root: IF_LEQ(reg) */
	case 105: /* root: IFLE(reg) */
	case 104: /* root: IFGT(reg) */
	case 103: /* root: IFGE(reg) */
	case 102: /* root: IFLT(reg) */
	case 101: /* root: IFNE(reg) */
	case 100: /* root: IFEQ(reg) */
	case 99: /* root: IFEQ(reg) */
	case 93: /* reg: INT2SHORT(reg) */
	case 92: /* reg: INT2CHAR(reg) */
	case 91: /* reg: INT2BYTE(reg) */
	case 90: /* reg: D2F(reg) */
	case 89: /* reg: D2L(reg) */
	case 88: /* reg: D2I(reg) */
	case 87: /* reg: F2D(reg) */
	case 86: /* reg: F2L(reg) */
	case 85: /* reg: F2I(reg) */
	case 84: /* reg: L2D(reg) */
	case 83: /* reg: L2F(reg) */
	case 82: /* reg: L2I(reg) */
	case 81: /* reg: I2D(reg) */
	case 80: /* reg: I2F(reg) */
	case 79: /* reg: I2L(reg) */
	case 65: /* reg: DNEG(reg) */
	case 64: /* reg: FNEG(reg) */
	case 63: /* reg: LNEG(reg) */
	case 62: /* reg: INEG(reg) */
	case 41: /* reg: IADD(reg,ICONST) */
	case 31: /* root: ASTORE(reg) */
	case 30: /* root: DSTORE(reg) */
	case 29: /* root: FSTORE(reg) */
	case 28: /* root: LSTORE(reg) */
	case 27: /* root: ISTORE(reg) */
	case 11: /* reg: CHECKNULL(reg) */
		kids[0] = LEFT_CHILD(p);
		break;
	case 159: /* reg: BUILTIN(reg,reg) */
	case 145: /* reg: INVOKEINTERFACE(reg,reg) */
	case 144: /* reg: INVOKESTATIC(reg,reg) */
	case 143: /* reg: INVOKESPECIAL(reg,reg) */
	case 142: /* reg: INVOKEVIRTUAL(reg,reg) */
	case 141: /* root: PUTFIELD(reg,reg) */
	case 126: /* root: IF_ACMPNE(reg,reg) */
	case 125: /* root: IF_ACMPEQ(reg,reg) */
	case 124: /* root: IF_LCMPLE(reg,reg) */
	case 123: /* root: IF_LCMPGT(reg,reg) */
	case 122: /* root: IF_LCMPGE(reg,reg) */
	case 121: /* root: IF_LCMPLT(reg,reg) */
	case 120: /* root: IF_LCMPNE(reg,reg) */
	case 119: /* root: IF_LCMPEQ(reg,reg) */
	case 118: /* root: IF_ICMPLE(reg,reg) */
	case 117: /* root: IF_ICMPGT(reg,reg) */
	case 116: /* root: IF_ICMPGE(reg,reg) */
	case 115: /* root: IF_ICMPLT(reg,reg) */
	case 114: /* root: IF_ICMPNE(reg,reg) */
	case 113: /* root: IF_ICMPEQ(reg,reg) */
	case 98: /* reg: DCMPG(reg,reg) */
	case 97: /* reg: DCMPL(reg,reg) */
	case 96: /* reg: FCMPG(reg,reg) */
	case 95: /* reg: FCMPL(reg,reg) */
	case 94: /* reg: LCMP(reg,reg) */
	case 77: /* reg: LXOR(reg,reg) */
	case 76: /* reg: IXOR(reg,reg) */
	case 75: /* reg: LOR(reg,reg) */
	case 74: /* reg: IOR(reg,reg) */
	case 73: /* reg: LAND(reg,reg) */
	case 72: /* reg: IAND(reg,reg) */
	case 71: /* reg: LUSHR(reg,reg) */
	case 70: /* reg: IUSHR(reg,reg) */
	case 69: /* reg: LSHR(reg,reg) */
	case 68: /* reg: ISHR(reg,reg) */
	case 67: /* reg: LSHL(reg,reg) */
	case 66: /* reg: ISHL(reg,reg) */
	case 61: /* reg: DREM(reg,reg) */
	case 60: /* reg: FREM(reg,reg) */
	case 59: /* reg: LREM(reg,reg) */
	case 58: /* reg: IREM(reg,reg) */
	case 57: /* reg: DDIV(reg,reg) */
	case 56: /* reg: FDIV(reg,reg) */
	case 55: /* reg: LDIV(reg,reg) */
	case 54: /* reg: IDIV(reg,reg) */
	case 53: /* reg: DMUL(reg,reg) */
	case 52: /* reg: FMUL(reg,reg) */
	case 51: /* reg: LMUL(reg,reg) */
	case 50: /* reg: IMUL(reg,reg) */
	case 49: /* reg: DSUB(reg,reg) */
	case 48: /* reg: FSUB(reg,reg) */
	case 47: /* reg: LSUB(reg,reg) */
	case 46: /* reg: ISUB(reg,reg) */
	case 45: /* reg: DADD(reg,reg) */
	case 44: /* reg: FADD(reg,reg) */
	case 43: /* reg: LADD(reg,reg) */
	case 40: /* reg: IADD(reg,reg) */
	case 26: /* reg: SALOAD(reg,reg) */
	case 25: /* reg: CALOAD(reg,reg) */
	case 24: /* reg: BALOAD(reg,reg) */
	case 23: /* reg: AALOAD(reg,reg) */
	case 22: /* reg: DALOAD(reg,reg) */
	case 21: /* reg: FALOAD(reg,reg) */
	case 20: /* reg: LALOAD(reg,reg) */
	case 19: /* reg: IALOAD(reg,reg) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = RIGHT_CHILD(p);
		break;
	case 42: /* reg: IADD(ICONST,reg) */
		kids[0] = RIGHT_CHILD(p);
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
  case 2:
    codegen_nop(bnode);
    break;
  case 3:
    codegen_nop(bnode);
    break;
  case 4:
    codegen_nop(bnode);
    break;
  case 5:
    codegen_emit_result(bnode);
    break;
  case 6:
    codegen_emit_instruction(bnode);
    break;
  case 7:
    codegen_emit_iconst(bnode);
    break;
  case 8:
    codegen_emit_lconst(bnode);
    break;
  case 9:
    codegen_emit_instruction(bnode);
    break;
  case 10:
    codegen_emit_instruction(bnode);
    break;
  case 11:
    codegen_emit_checknull(bnode);
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
    codegen_emit_copy(bnode);
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
    codegen_emit_copy(bnode);
    break;
  case 31:
    codegen_emit_astore(bnode);
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
    codegen_emit_const_instruction(bnode);
    break;
  case 42:
    codegen_emit_const_instruction(bnode);
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
    codegen_emit_instruction(bnode);
    break;
  case 97:
    codegen_emit_instruction(bnode);
    break;
  case 98:
    codegen_emit_instruction(bnode);
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
    codegen_emit_branch(bnode);
    break;
  case 111:
    codegen_emit_branch(bnode);
    break;
  case 112:
    codegen_emit_branch(bnode);
    break;
  case 113:
    codegen_emit_branch(bnode);
    break;
  case 114:
    codegen_emit_branch(bnode);
    break;
  case 115:
    codegen_emit_branch(bnode);
    break;
  case 116:
    codegen_emit_branch(bnode);
    break;
  case 117:
    codegen_emit_branch(bnode);
    break;
  case 118:
    codegen_emit_branch(bnode);
    break;
  case 119:
    codegen_emit_branch(bnode);
    break;
  case 120:
    codegen_emit_branch(bnode);
    break;
  case 121:
    codegen_emit_branch(bnode);
    break;
  case 122:
    codegen_emit_branch(bnode);
    break;
  case 123:
    codegen_emit_branch(bnode);
    break;
  case 124:
    codegen_emit_branch(bnode);
    break;
  case 125:
    codegen_emit_branch(bnode);
    break;
  case 126:
    codegen_emit_branch(bnode);
    break;
  case 127:
    codegen_emit_jump(bnode);
    break;
  case 128:
    codegen_emit_jump(bnode);
    break;
  case 129:
    codegen_emit_jump(bnode);
    break;
  case 130:
    codegen_emit_instruction(bnode);
    break;
  case 131:
    codegen_emit_lookup(bnode);
    break;
  case 132:
    codegen_emit_return(bnode);
    break;
  case 133:
    codegen_emit_return(bnode);
    break;
  case 134:
    codegen_emit_return(bnode);
    break;
  case 135:
    codegen_emit_return(bnode);
    break;
  case 136:
    codegen_emit_return(bnode);
    break;
  case 137:
    codegen_emit_return(bnode);
    break;
  case 138:
    codegen_emit_getstatic(bnode);
    break;
  case 139:
    codegen_emit_putstatic(bnode);
    break;
  case 140:
    codegen_emit_instruction(bnode);
    break;
  case 141:
    codegen_emit_instruction(bnode);
    break;
  case 142:
    codegen_emit_invoke(bnode);
    break;
  case 143:
    codegen_emit_invoke(bnode);
    break;
  case 144:
    codegen_emit_invoke(bnode);
    break;
  case 145:
    codegen_emit_invoke(bnode);
    break;
  case 146:
    codegen_emit_instruction(bnode);
    break;
  case 147:
    codegen_emit_instruction(bnode);
    break;
  case 148:
    codegen_emit_instruction(bnode);
    break;
  case 149:
    codegen_emit_arraylength(bnode);
    break;
  case 150:
    codegen_emit_throw(bnode);
    break;
  case 151:
    codegen_emit_ifnull(bnode);
    break;
  case 152:
    codegen_emit_ifnull(bnode);
    break;
  case 153:
    codegen_emit_breakpoint(bnode);
    break;
  case 154:
    codegen_emit_getexception(bnode);
    break;
  case 155:
    codegen_emit_phi(bnode);
    break;
  case 156:
    codegen_emit_inline_start(bnode);
    break;
  case 157:
    codegen_emit_inline_end(bnode);
    break;
  case 158:
    codegen_emit_inline_body(bnode);
    break;
  case 159:
    codegen_emit_builtin(bnode);
    break;
  }
}
