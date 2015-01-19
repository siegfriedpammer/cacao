/* src/vm/jit/compiler2/GrammarBase.brg - Generated Basic Grammar

   Copyright (C) 2013
   CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

/*
WARNING: THIS FILE IS AUTO-GENERATED! DO NOT ALTER!
Instead have a look at the generator (src/vm/jit/compiler2/instruction_gen.py and contrib/patternmatching/)
and the input file (instruction_table.csv).
*/

/* src/vm/jit/Grammar.inc - Generated Pattern Matching include file

Copyright (C) 1996-2013
CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO

This file is part of CACAO.
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

*/

/* THIS FILE IS GENERATED!!! DO NOT EDIT!!!
   See contrib/patternmatching for further information
*/

#ifdef GLOBALDEFS
#undef GLOBALDEFS

enum RuleId {
/* 0 */	NoRule,
/* 1 */	NOPInstID,	/* stm: NOPInstID */
/* 2 */	POPInstID,	/* stm: POPInstID */
/* 3 */	CHECKNULLInstID,	/* stm: CHECKNULLInstID */
/* 4 */	ARRAYLENGTHInstID,	/* stm: ARRAYLENGTHInstID(stm) */
/* 5 */	ARRAYBOUNDSCHECKInstID,	/* stm: ARRAYBOUNDSCHECKInstID(stm,stm) */
/* 6 */	NEGInstID,	/* stm: NEGInstID(stm) */
/* 7 */	CASTInstID,	/* stm: CASTInstID(stm) */
/* 8 */	ADDInstID,	/* stm: ADDInstID(stm,stm) */
/* 9 */	SUBInstID,	/* stm: SUBInstID(stm,stm) */
/* 10 */	MULInstID,	/* stm: MULInstID(stm,stm) */
/* 11 */	DIVInstID,	/* stm: DIVInstID(stm,stm) */
/* 12 */	REMInstID,	/* stm: REMInstID(stm,stm) */
/* 13 */	SHLInstID,	/* stm: SHLInstID */
/* 14 */	USHRInstID,	/* stm: USHRInstID */
/* 15 */	ANDInstID,	/* stm: ANDInstID(stm,stm) */
/* 16 */	ORInstID,	/* stm: ORInstID(stm,stm) */
/* 17 */	XORInstID,	/* stm: XORInstID(stm,stm) */
/* 18 */	CMPInstID,	/* stm: CMPInstID(stm,stm) */
/* 19 */	CONSTInstID,	/* stm: CONSTInstID */
/* 20 */	ALOADInstID,	/* stm: ALOADInstID(stm,stm) */
/* 21 */	LOADInstID,	/* stm: LOADInstID */
/* 22 */	IFInstID,	/* stm: IFInstID(stm,stm) */
/* 23 */	TABLESWITCHInstID,	/* stm: TABLESWITCHInstID(stm) */
/* 24 */	LOOKUPSWITCHInstID,	/* stm: LOOKUPSWITCHInstID(stm) */
/* 25 */	RETURNInstID,	/* stm: RETURNInstID(stm) */
/* 26 */	NoInstID,	/* stm: NoInstID */
/* 27 */	AddImmImm,	/* stm: ADDInstID(CONSTInstID,CONSTInstID) */
/* 28 */	AddRegImm,	/* stm: ADDInstID(stm,CONSTInstID) */
/* 0 */DUMMYREMOVE
};
#endif

#ifdef GENDEFS
#undef GENDEFS

#define burm_stm_NT 1

struct burm_state {
	burm_state(NODEPTR_TYPE root) : root(root) {
		for (int i=1; i <= 1; ++i) cost[i] = 0x7fff;
	};

	short cost[2];
	struct {
		unsigned int burm_stm:5;
	} rule;
	bool isLeaf;
	NODEPTR_TYPE root;
};


#endif

#ifdef GENSTATIC
#undef GENSTATIC

static short burm_nts_0[] = { 0 };
static short burm_nts_1[] = { burm_stm_NT, 0 };
static short burm_nts_2[] = { burm_stm_NT, burm_stm_NT, 0 };

static short *burm_nts[] = {
	0,	/* 0 */
	burm_nts_0,	/* 1 */
	burm_nts_0,	/* 2 */
	burm_nts_0,	/* 3 */
	burm_nts_1,	/* 4 */
	burm_nts_2,	/* 5 */
	burm_nts_1,	/* 6 */
	burm_nts_1,	/* 7 */
	burm_nts_2,	/* 8 */
	burm_nts_2,	/* 9 */
	burm_nts_2,	/* 10 */
	burm_nts_2,	/* 11 */
	burm_nts_2,	/* 12 */
	burm_nts_0,	/* 13 */
	burm_nts_0,	/* 14 */
	burm_nts_2,	/* 15 */
	burm_nts_2,	/* 16 */
	burm_nts_2,	/* 17 */
	burm_nts_2,	/* 18 */
	burm_nts_0,	/* 19 */
	burm_nts_2,	/* 20 */
	burm_nts_0,	/* 21 */
	burm_nts_2,	/* 22 */
	burm_nts_1,	/* 23 */
	burm_nts_1,	/* 24 */
	burm_nts_1,	/* 25 */
	burm_nts_0,	/* 26 */
	burm_nts_0,	/* 27 */
	burm_nts_1,	/* 28 */
};

static const char *burm_templates[] = {
/* 0 */	0,
/* 1 */	"NOPInstID",	/* stm: NOPInstID */
/* 2 */	"POPInstID",	/* stm: POPInstID */
/* 3 */	"CHECKNULLInstID",	/* stm: CHECKNULLInstID */
/* 4 */	"ARRAYLENGTHInstID",	/* stm: ARRAYLENGTHInstID(stm) */
/* 5 */	"ARRAYBOUNDSCHECKInstID",	/* stm: ARRAYBOUNDSCHECKInstID(stm,stm) */
/* 6 */	"NEGInstID",	/* stm: NEGInstID(stm) */
/* 7 */	"CASTInstID",	/* stm: CASTInstID(stm) */
/* 8 */	"ADDInstID",	/* stm: ADDInstID(stm,stm) */
/* 9 */	"SUBInstID",	/* stm: SUBInstID(stm,stm) */
/* 10 */	"MULInstID",	/* stm: MULInstID(stm,stm) */
/* 11 */	"DIVInstID",	/* stm: DIVInstID(stm,stm) */
/* 12 */	"REMInstID",	/* stm: REMInstID(stm,stm) */
/* 13 */	"SHLInstID",	/* stm: SHLInstID */
/* 14 */	"USHRInstID",	/* stm: USHRInstID */
/* 15 */	"ANDInstID",	/* stm: ANDInstID(stm,stm) */
/* 16 */	"ORInstID",	/* stm: ORInstID(stm,stm) */
/* 17 */	"XORInstID",	/* stm: XORInstID(stm,stm) */
/* 18 */	"CMPInstID",	/* stm: CMPInstID(stm,stm) */
/* 19 */	"CONSTInstID",	/* stm: CONSTInstID */
/* 20 */	"ALOADInstID",	/* stm: ALOADInstID(stm,stm) */
/* 21 */	"LOADInstID",	/* stm: LOADInstID */
/* 22 */	"IFInstID",	/* stm: IFInstID(stm,stm) */
/* 23 */	"TABLESWITCHInstID",	/* stm: TABLESWITCHInstID(stm) */
/* 24 */	"LOOKUPSWITCHInstID",	/* stm: LOOKUPSWITCHInstID(stm) */
/* 25 */	"RETURNInstID",	/* stm: RETURNInstID(stm) */
/* 26 */	"NoInstID",	/* stm: NoInstID */
/* 27 */	"AddImmImm",	/* stm: ADDInstID(CONSTInstID,CONSTInstID) */
/* 28 */	"AddRegImm",	/* stm: ADDInstID(stm,CONSTInstID) */
};

static char burm_isinstruction[] = {
/* 0 */	0,
/* 1 */	1,	/* NOPInstID */
/* 2 */	1,	/* POPInstID */
/* 3 */	1,	/* CHECKNULLInstID */
/* 4 */	1,	/* ARRAYLENGTHInstID */
/* 5 */	1,	/* ARRAYBOUNDSCHECKInstID */
/* 6 */	1,	/* NEGInstID */
/* 7 */	1,	/* CASTInstID */
/* 8 */	1,	/* ADDInstID */
/* 9 */	1,	/* SUBInstID */
/* 10 */	1,	/* MULInstID */
/* 11 */	1,	/* DIVInstID */
/* 12 */	1,	/* REMInstID */
/* 13 */	1,	/* SHLInstID */
/* 14 */	1,	/* USHRInstID */
/* 15 */	1,	/* ANDInstID */
/* 16 */	1,	/* ORInstID */
/* 17 */	1,	/* XORInstID */
/* 18 */	1,	/* CMPInstID */
/* 19 */	1,	/* CONSTInstID */
/* 20 */	1,	/* ALOADInstID */
/* 21 */	1,	/* LOADInstID */
/* 22 */	1,	/* IFInstID */
/* 23 */	1,	/* TABLESWITCHInstID */
/* 24 */	1,	/* LOOKUPSWITCHInstID */
/* 25 */	1,	/* RETURNInstID */
/* 26 */	1,	/* NoInstID */
/* 27 */	0,	/* AddImmImm */
/* 28 */	0,	/* AddRegImm */
};

static const char *burm_string[] = {
/* 0 */	0,
/* 1 */	"stm: NOPInstID",
/* 2 */	"stm: POPInstID",
/* 3 */	"stm: CHECKNULLInstID",
/* 4 */	"stm: ARRAYLENGTHInstID(stm)",
/* 5 */	"stm: ARRAYBOUNDSCHECKInstID(stm,stm)",
/* 6 */	"stm: NEGInstID(stm)",
/* 7 */	"stm: CASTInstID(stm)",
/* 8 */	"stm: ADDInstID(stm,stm)",
/* 9 */	"stm: SUBInstID(stm,stm)",
/* 10 */	"stm: MULInstID(stm,stm)",
/* 11 */	"stm: DIVInstID(stm,stm)",
/* 12 */	"stm: REMInstID(stm,stm)",
/* 13 */	"stm: SHLInstID",
/* 14 */	"stm: USHRInstID",
/* 15 */	"stm: ANDInstID(stm,stm)",
/* 16 */	"stm: ORInstID(stm,stm)",
/* 17 */	"stm: XORInstID(stm,stm)",
/* 18 */	"stm: CMPInstID(stm,stm)",
/* 19 */	"stm: CONSTInstID",
/* 20 */	"stm: ALOADInstID(stm,stm)",
/* 21 */	"stm: LOADInstID",
/* 22 */	"stm: IFInstID(stm,stm)",
/* 23 */	"stm: TABLESWITCHInstID(stm)",
/* 24 */	"stm: LOOKUPSWITCHInstID(stm)",
/* 25 */	"stm: RETURNInstID(stm)",
/* 26 */	"stm: NoInstID",
/* 27 */	"stm: ADDInstID(CONSTInstID,CONSTInstID)",
/* 28 */	"stm: ADDInstID(stm,CONSTInstID)",
};

static short burm_decode_stm[] = {
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
};


#endif

#ifdef GENMETHODDEFS
#undef GENMETHODDEFS

int burm_rule(shared_ptr<STATE_TYPE> state, int goalnt);
void burm_label(NODEPTR_TYPE a, NODEPTR_TYPE root);
void burm_kids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]);

#endif

#ifdef GENMETHODS
#undef GENMETHODS

int Matcher::burm_rule(shared_ptr<STATE_TYPE> state, int goalnt) {
	if (goalnt < 1 || goalnt > 1)
		ABORT_MSG("burm_rule", "Bad goal nonterminal " << goalnt << nl);
	if (!state)
		return 0;
	switch (goalnt) {
	case burm_stm_NT:	return burm_decode_stm[state->rule.burm_stm];
	default:
		ABORT_MSG("burm_rule", "Bad goal nonterminal " << goalnt << nl);
		return 0;
	}
}


void Matcher::burm_label(NODEPTR_TYPE a, NODEPTR_TYPE root) {
	int c;
	shared_ptr<STATE_TYPE> p;

	if (!a)
		ABORT_MSG("burm_label", "Null tree");
	STATE_LABEL(a) = p = shared_ptr<STATE_TYPE>(new STATE_TYPE(root));
	switch (OP_LABEL(a)) {
	case 3: /* NOPInstID */
		/* stm: NOPInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 1;
		}
		break;
	case 4: /* POPInstID */
		/* stm: POPInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 2;
		}
		break;
	case 5: /* CHECKNULLInstID */
		/* stm: CHECKNULLInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 3;
		}
		break;
	case 6: /* ARRAYLENGTHInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ARRAYLENGTHInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 4;
		}
		break;
	case 7: /* ARRAYBOUNDSCHECKInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ARRAYBOUNDSCHECKInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 5;
		}
		break;
	case 8: /* NEGInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: NEGInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 6;
		}
		break;
	case 9: /* CASTInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: CASTInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 7;
		}
		break;
	case 10: /* ADDInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ADDInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 8;
		}
		if (	/* stm: ADDInstID(CONSTInstID,CONSTInstID) */
			OP_LABEL(LEFT_CHILD(a)) == 21 && /* CONSTInstID */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = 1;
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 27;
			}
		}
		if (	/* stm: ADDInstID(stm,CONSTInstID) */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (((a->get_type() == Type::IntTypeID && a->get_operand(1)->get_type() == Type::IntTypeID) ? 1 : 1000));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 28;
			}
		}
		break;
	case 11: /* SUBInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: SUBInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 9;
		}
		break;
	case 12: /* MULInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: MULInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 10;
		}
		break;
	case 13: /* DIVInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: DIVInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 11;
		}
		break;
	case 14: /* REMInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: REMInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 12;
		}
		break;
	case 15: /* SHLInstID */
		/* stm: SHLInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 13;
		}
		break;
	case 16: /* USHRInstID */
		/* stm: USHRInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 14;
		}
		break;
	case 17: /* ANDInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ANDInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 15;
		}
		break;
	case 18: /* ORInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ORInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 16;
		}
		break;
	case 19: /* XORInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: XORInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 17;
		}
		break;
	case 20: /* CMPInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: CMPInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 18;
		}
		break;
	case 21: /* CONSTInstID */
		/* stm: CONSTInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 19;
		}
		break;
	case 28: /* ALOADInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ALOADInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 20;
		}
		break;
	case 30: /* LOADInstID */
		/* stm: LOADInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 21;
		}
		break;
	case 45: /* IFInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: IFInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 22;
		}
		break;
	case 47: /* TABLESWITCHInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: TABLESWITCHInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 23;
		}
		break;
	case 48: /* LOOKUPSWITCHInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: LOOKUPSWITCHInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 24;
		}
		break;
	case 49: /* RETURNInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: RETURNInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 25;
		}
		break;
	case 58: /* NoInstID */
		/* stm: NoInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 26;
		}
		break;
	default:
		ABORT_MSG("burm_label", "Bad terminal " << OP_LABEL(a) << nl);
	}
}

void Matcher::burm_kids(NODEPTR_TYPE p, int eruleno, NODEPTR_TYPE kids[]) {
	if (!p)
		ABORT_MSG("burm_kids", "Null tree");
	if (!kids)
		ABORT_MSG("burm_kids", "Null kids");
	switch (eruleno) {
	case 27: /* stm: ADDInstID(CONSTInstID,CONSTInstID) */
	case 26: /* stm: NoInstID */
	case 21: /* stm: LOADInstID */
	case 19: /* stm: CONSTInstID */
	case 14: /* stm: USHRInstID */
	case 13: /* stm: SHLInstID */
	case 3: /* stm: CHECKNULLInstID */
	case 2: /* stm: POPInstID */
	case 1: /* stm: NOPInstID */
		break;
	case 28: /* stm: ADDInstID(stm,CONSTInstID) */
	case 25: /* stm: RETURNInstID(stm) */
	case 24: /* stm: LOOKUPSWITCHInstID(stm) */
	case 23: /* stm: TABLESWITCHInstID(stm) */
	case 7: /* stm: CASTInstID(stm) */
	case 6: /* stm: NEGInstID(stm) */
	case 4: /* stm: ARRAYLENGTHInstID(stm) */
		kids[0] = LEFT_CHILD(p);
		break;
	case 22: /* stm: IFInstID(stm,stm) */
	case 20: /* stm: ALOADInstID(stm,stm) */
	case 18: /* stm: CMPInstID(stm,stm) */
	case 17: /* stm: XORInstID(stm,stm) */
	case 16: /* stm: ORInstID(stm,stm) */
	case 15: /* stm: ANDInstID(stm,stm) */
	case 12: /* stm: REMInstID(stm,stm) */
	case 11: /* stm: DIVInstID(stm,stm) */
	case 10: /* stm: MULInstID(stm,stm) */
	case 9: /* stm: SUBInstID(stm,stm) */
	case 8: /* stm: ADDInstID(stm,stm) */
	case 5: /* stm: ARRAYBOUNDSCHECKInstID(stm,stm) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = RIGHT_CHILD(p);
		break;
	default:
		ABORT_MSG("burm_kids", "Bad rule number " << eruleno << nl);
	}
}

// stm: ADDInstID(CONSTInstID, stm) "AddImmReg" ((a->get_type() == Type::IntTypeID && a->get_operand(0)->get_type() == Type::IntTypeID) ? 1 : 1000)

#endif

/*
* These are local overrides for various environment variables in Emacs.
* Please do not remove this and leave it at the end of the file, where
* Emacs will automagically detect them.
* ---------------------------------------------------------------------
* Local variables:
* mode: c++
* indent-tabs-mode: t
* c-basic-offset: 4
* tab-width: 4
* End:
* vim:noexpandtab:sw=4:ts=4:
*/
