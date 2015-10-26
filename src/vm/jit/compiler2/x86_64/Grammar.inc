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
/* 20 */	AREFInstID,	/* stm: AREFInstID(stm,stm) */
/* 21 */	ASTOREInstID,	/* stm: ASTOREInstID(stm,stm) */
/* 22 */	ALOADInstID,	/* stm: ALOADInstID(stm) */
/* 23 */	LOADInstID,	/* stm: LOADInstID */
/* 24 */	IFInstID,	/* stm: IFInstID(stm,stm) */
/* 25 */	TABLESWITCHInstID,	/* stm: TABLESWITCHInstID(stm) */
/* 26 */	LOOKUPSWITCHInstID,	/* stm: LOOKUPSWITCHInstID(stm) */
/* 27 */	RETURNInstID,	/* stm: RETURNInstID(stm) */
/* 28 */	NoInstID,	/* stm: NoInstID */
/* 29 */	AddImmImm,	/* stm: ADDInstID(CONSTInstID,CONSTInstID) */
/* 30 */	AddRegImm,	/* stm: ADDInstID(stm,CONSTInstID) */
/* 31 */	SubRegImm,	/* stm: SUBInstID(stm,CONSTInstID) */
/* 32 */	MulRegImm,	/* stm: MULInstID(stm,CONSTInstID) */
/* 33 */	BaseIndexDisplacement,	/* stm: ADDInstID(ADDInstID(stm,stm),CONSTInstID) */
/* 34 */	BaseIndexDisplacement2,	/* stm: ADDInstID(stm,ADDInstID(stm,CONSTInstID)) */
/* 35 */	BaseIndexMultiplier,	/* stm: ADDInstID(stm,MULInstID(stm,CONSTInstID)) */
/* 36 */	BaseIndexMultiplier2,	/* stm: ADDInstID(MULInstID(stm,CONSTInstID),stm) */
/* 37 */	BaseIndexMultiplierDisplacement,	/* stm: ADDInstID(ADDInstID(stm,MULInstID(stm,CONSTInstID)),CONSTInstID) */
/* 38 */	BaseIndexMultiplierDisplacement2,	/* stm: ADDInstID(stm,ADDInstID(MULInstID(stm,CONSTInstID),CONSTInstID)) */
/* 39 */	ARef,	/* stm: AREFInstID(stm,stm) */
/* 40 */	ALoad,	/* stm: ALOADInstID(AREFInstID(stm,stm)) */
/* 41 */	AStore,	/* stm: ASTOREInstID(AREFInstID(stm,stm),stm) */
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
		unsigned int burm_stm:6;
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
static short burm_nts_3[] = { burm_stm_NT, burm_stm_NT, burm_stm_NT, 0 };

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
	burm_nts_2,	/* 21 */
	burm_nts_1,	/* 22 */
	burm_nts_0,	/* 23 */
	burm_nts_2,	/* 24 */
	burm_nts_1,	/* 25 */
	burm_nts_1,	/* 26 */
	burm_nts_1,	/* 27 */
	burm_nts_0,	/* 28 */
	burm_nts_0,	/* 29 */
	burm_nts_1,	/* 30 */
	burm_nts_1,	/* 31 */
	burm_nts_1,	/* 32 */
	burm_nts_2,	/* 33 */
	burm_nts_2,	/* 34 */
	burm_nts_2,	/* 35 */
	burm_nts_2,	/* 36 */
	burm_nts_2,	/* 37 */
	burm_nts_2,	/* 38 */
	burm_nts_2,	/* 39 */
	burm_nts_2,	/* 40 */
	burm_nts_3,	/* 41 */
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
/* 20 */	"AREFInstID",	/* stm: AREFInstID(stm,stm) */
/* 21 */	"ASTOREInstID",	/* stm: ASTOREInstID(stm,stm) */
/* 22 */	"ALOADInstID",	/* stm: ALOADInstID(stm) */
/* 23 */	"LOADInstID",	/* stm: LOADInstID */
/* 24 */	"IFInstID",	/* stm: IFInstID(stm,stm) */
/* 25 */	"TABLESWITCHInstID",	/* stm: TABLESWITCHInstID(stm) */
/* 26 */	"LOOKUPSWITCHInstID",	/* stm: LOOKUPSWITCHInstID(stm) */
/* 27 */	"RETURNInstID",	/* stm: RETURNInstID(stm) */
/* 28 */	"NoInstID",	/* stm: NoInstID */
/* 29 */	"AddImmImm",	/* stm: ADDInstID(CONSTInstID,CONSTInstID) */
/* 30 */	"AddRegImm",	/* stm: ADDInstID(stm,CONSTInstID) */
/* 31 */	"SubRegImm",	/* stm: SUBInstID(stm,CONSTInstID) */
/* 32 */	"MulRegImm",	/* stm: MULInstID(stm,CONSTInstID) */
/* 33 */	"BaseIndexDisplacement",	/* stm: ADDInstID(ADDInstID(stm,stm),CONSTInstID) */
/* 34 */	"BaseIndexDisplacement2",	/* stm: ADDInstID(stm,ADDInstID(stm,CONSTInstID)) */
/* 35 */	"BaseIndexMultiplier",	/* stm: ADDInstID(stm,MULInstID(stm,CONSTInstID)) */
/* 36 */	"BaseIndexMultiplier2",	/* stm: ADDInstID(MULInstID(stm,CONSTInstID),stm) */
/* 37 */	"BaseIndexMultiplierDisplacement",	/* stm: ADDInstID(ADDInstID(stm,MULInstID(stm,CONSTInstID)),CONSTInstID) */
/* 38 */	"BaseIndexMultiplierDisplacement2",	/* stm: ADDInstID(stm,ADDInstID(MULInstID(stm,CONSTInstID),CONSTInstID)) */
/* 39 */	"ARef",	/* stm: AREFInstID(stm,stm) */
/* 40 */	"ALoad",	/* stm: ALOADInstID(AREFInstID(stm,stm)) */
/* 41 */	"AStore",	/* stm: ASTOREInstID(AREFInstID(stm,stm),stm) */
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
/* 20 */	1,	/* AREFInstID */
/* 21 */	1,	/* ASTOREInstID */
/* 22 */	1,	/* ALOADInstID */
/* 23 */	1,	/* LOADInstID */
/* 24 */	1,	/* IFInstID */
/* 25 */	1,	/* TABLESWITCHInstID */
/* 26 */	1,	/* LOOKUPSWITCHInstID */
/* 27 */	1,	/* RETURNInstID */
/* 28 */	1,	/* NoInstID */
/* 29 */	0,	/* AddImmImm */
/* 30 */	0,	/* AddRegImm */
/* 31 */	0,	/* SubRegImm */
/* 32 */	0,	/* MulRegImm */
/* 33 */	0,	/* BaseIndexDisplacement */
/* 34 */	0,	/* BaseIndexDisplacement2 */
/* 35 */	0,	/* BaseIndexMultiplier */
/* 36 */	0,	/* BaseIndexMultiplier2 */
/* 37 */	0,	/* BaseIndexMultiplierDisplacement */
/* 38 */	0,	/* BaseIndexMultiplierDisplacement2 */
/* 39 */	0,	/* ARef */
/* 40 */	0,	/* ALoad */
/* 41 */	0,	/* AStore */
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
/* 20 */	"stm: AREFInstID(stm,stm)",
/* 21 */	"stm: ASTOREInstID(stm,stm)",
/* 22 */	"stm: ALOADInstID(stm)",
/* 23 */	"stm: LOADInstID",
/* 24 */	"stm: IFInstID(stm,stm)",
/* 25 */	"stm: TABLESWITCHInstID(stm)",
/* 26 */	"stm: LOOKUPSWITCHInstID(stm)",
/* 27 */	"stm: RETURNInstID(stm)",
/* 28 */	"stm: NoInstID",
/* 29 */	"stm: ADDInstID(CONSTInstID,CONSTInstID)",
/* 30 */	"stm: ADDInstID(stm,CONSTInstID)",
/* 31 */	"stm: SUBInstID(stm,CONSTInstID)",
/* 32 */	"stm: MULInstID(stm,CONSTInstID)",
/* 33 */	"stm: ADDInstID(ADDInstID(stm,stm),CONSTInstID)",
/* 34 */	"stm: ADDInstID(stm,ADDInstID(stm,CONSTInstID))",
/* 35 */	"stm: ADDInstID(stm,MULInstID(stm,CONSTInstID))",
/* 36 */	"stm: ADDInstID(MULInstID(stm,CONSTInstID),stm)",
/* 37 */	"stm: ADDInstID(ADDInstID(stm,MULInstID(stm,CONSTInstID)),CONSTInstID)",
/* 38 */	"stm: ADDInstID(stm,ADDInstID(MULInstID(stm,CONSTInstID),CONSTInstID))",
/* 39 */	"stm: AREFInstID(stm,stm)",
/* 40 */	"stm: ALOADInstID(AREFInstID(stm,stm))",
/* 41 */	"stm: ASTOREInstID(AREFInstID(stm,stm),stm)",
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


namespace {

	/*


	TODO

	displacement with SUBInst!!!
	indexmultiplierdisplacement



	*/

	bool isMultiplier(CONSTInst* c){
		s8 val = c->get_value();
		if ((val == 2) || (val == 4) || (val == 8)) return true;
		return false;
	}

	bool isConstEncodable(CONSTInst* c){
		return fits_into<s4>(c->get_value());
	}

	bool isDiscreteValue(Instruction* a){
		return (a->get_type() == Type::ByteTypeID	|| 
				a->get_type() == Type::IntTypeID	|| 
				a->get_type() == Type::LongTypeID);
	}

	int encodeDiscreteCost(Instruction* a, Value* c){
		if (!isDiscreteValue(a))
			return 0x7fff;
		if (!isConstEncodable(c->to_Instruction()->to_CONSTInst()))
			return 0x7fff;
		return 1;
	}

	/*
	LEA Encodings
		* CONSTInst is always assumed to be second parameter due to how SSA Graph is created
		  If it is also used as first parameter, handle all other permutations as well
		* Permutations in nesting MULInst/stm within the add have to be handled, 
		  hence the rules and methods with "...2"
	*/

	int calcBaseIndexDisplacementCost(Instruction* a){
		/*
		ADDInstID
			ADDInstID
				stm, 
				stm
			CONSTInstID
		*/
		if (isConstEncodable(a->get_operand(1)->to_Instruction()->to_CONSTInst()))
			return 1;
		return 0x7fff;
	}

	int calcBaseIndexDisplacement2Cost(Instruction* a){
		/*
		ADDInstID
				stm 
				ADDInstID
					stm
					CONSTInstID
		*/
		if (isConstEncodable(a->get_operand(1)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst()))
			return 1;
		return 0x7fff;
	}

	int calcBaseIndexMultiplierCost(Instruction* a){
		/*
		ADDInstID
			stm, 
			MULInstID
				stm, 
				CONSTInstID
		*/
		if (isMultiplier(a->get_operand(1)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst()))
			return 1;
		return 0x7fff;
	}

	int calcBaseIndexMultiplier2Cost(Instruction* a){
		/*
		ADDInstID
			MULInstID
				stm, 
				CONSTInstID
			stm
		*/
		if (isMultiplier(a->get_operand(0)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst()))
			return 1;
		return 0x7fff;
	}

	int calcBaseIndexMultiplierDisplacementCost(Instruction* a){
		/*
		ADDInstID
			ADDInstID
				stm, 
				MULInstID
					stm, 
					CONSTInstID
			CONSTInstID
		*/

		if ((isConstEncodable(a->get_operand(1)->to_Instruction()->to_CONSTInst())) &&
			(isMultiplier(a->get_operand(0)->to_Instruction()->get_operand(1)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst())))
			return 1;
		return 0x7fff;
	}

	int calcBaseIndexMultiplierDisplacement2Cost(Instruction* a){
		/*
		ADDInstID
			stm, 
			ADDInstID
				MULInstID
					stm, 
					CONSTInstID
				CONSTInstID
		*/
		if ((isConstEncodable(a->get_operand(1)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst())) &&
			(isMultiplier(a->get_operand(1)->to_Instruction()->get_operand(0)->to_Instruction()->get_operand(1)->to_Instruction()->to_CONSTInst())))
			return 1;
		return 0x7fff;
	}

}

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
				p->rule.burm_stm = 29;
			}
		}
		if (	/* stm: ADDInstID(stm,CONSTInstID) */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (encodeDiscreteCost(a, a->get_operand(1)));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 30;
			}
		}
		if (	/* stm: ADDInstID(ADDInstID(stm,stm),CONSTInstID) */
			OP_LABEL(LEFT_CHILD(a)) == 10 && /* ADDInstID */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (calcBaseIndexDisplacementCost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 33;
			}
		}
		if (	/* stm: ADDInstID(stm,ADDInstID(stm,CONSTInstID)) */
			OP_LABEL(RIGHT_CHILD(a)) == 10 && /* ADDInstID */
			OP_LABEL(RIGHT_CHILD(RIGHT_CHILD(a))) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(LEFT_CHILD(RIGHT_CHILD(a))))->cost[burm_stm_NT] + (calcBaseIndexDisplacement2Cost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 34;
			}
		}
		if (	/* stm: ADDInstID(stm,MULInstID(stm,CONSTInstID)) */
			OP_LABEL(RIGHT_CHILD(a)) == 12 && /* MULInstID */
			OP_LABEL(RIGHT_CHILD(RIGHT_CHILD(a))) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(LEFT_CHILD(RIGHT_CHILD(a))))->cost[burm_stm_NT] + (calcBaseIndexMultiplierCost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 35;
			}
		}
		if (	/* stm: ADDInstID(MULInstID(stm,CONSTInstID),stm) */
			OP_LABEL(LEFT_CHILD(a)) == 12 && /* MULInstID */
			OP_LABEL(RIGHT_CHILD(LEFT_CHILD(a))) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + (calcBaseIndexMultiplier2Cost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 36;
			}
		}
		if (	/* stm: ADDInstID(ADDInstID(stm,MULInstID(stm,CONSTInstID)),CONSTInstID) */
			OP_LABEL(LEFT_CHILD(a)) == 10 && /* ADDInstID */
			OP_LABEL(RIGHT_CHILD(LEFT_CHILD(a))) == 12 && /* MULInstID */
			OP_LABEL(RIGHT_CHILD(RIGHT_CHILD(LEFT_CHILD(a)))) == 21 && /* CONSTInstID */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(LEFT_CHILD(RIGHT_CHILD(LEFT_CHILD(a)))))->cost[burm_stm_NT] + (calcBaseIndexMultiplierDisplacementCost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 37;
			}
		}
		if (	/* stm: ADDInstID(stm,ADDInstID(MULInstID(stm,CONSTInstID),CONSTInstID)) */
			OP_LABEL(RIGHT_CHILD(a)) == 10 && /* ADDInstID */
			OP_LABEL(LEFT_CHILD(RIGHT_CHILD(a))) == 12 && /* MULInstID */
			OP_LABEL(RIGHT_CHILD(LEFT_CHILD(RIGHT_CHILD(a)))) == 21 && /* CONSTInstID */
			OP_LABEL(RIGHT_CHILD(RIGHT_CHILD(a))) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(RIGHT_CHILD(a)))))->cost[burm_stm_NT] + (calcBaseIndexMultiplierDisplacement2Cost(a));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 38;
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
		if (	/* stm: SUBInstID(stm,CONSTInstID) */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (encodeDiscreteCost(a, a->get_operand(1)));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 31;
			}
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
		if (	/* stm: MULInstID(stm,CONSTInstID) */
			OP_LABEL(RIGHT_CHILD(a)) == 21 /* CONSTInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (encodeDiscreteCost(a, a->get_operand(1)));
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 32;
			}
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
	case 27: /* AREFInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: AREFInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 20;
		}
		/* stm: AREFInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 39;
		}
		break;
	case 28: /* ASTOREInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ASTOREInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 21;
		}
		if (	/* stm: ASTOREInstID(AREFInstID(stm,stm),stm) */
			OP_LABEL(LEFT_CHILD(a)) == 27 /* AREFInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 41;
			}
		}
		break;
	case 29: /* ALOADInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: ALOADInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 22;
		}
		if (	/* stm: ALOADInstID(AREFInstID(stm,stm)) */
			OP_LABEL(LEFT_CHILD(a)) == 27 /* AREFInstID */
		) {
			c = (STATE_LABEL(LEFT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(LEFT_CHILD(a))))->cost[burm_stm_NT] + 1;
			if (c + 0 < p->cost[burm_stm_NT]) {
				p->cost[burm_stm_NT] = c + 0;
				p->rule.burm_stm = 40;
			}
		}
		break;
	case 31: /* LOADInstID */
		/* stm: LOADInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 23;
		}
		break;
	case 46: /* IFInstID */
		burm_label(LEFT_CHILD(a), root);
		burm_label(RIGHT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID || OP_LABEL(RIGHT_CHILD(a)) == Instruction::NoInstID);
		/* stm: IFInstID(stm,stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + (STATE_LABEL(RIGHT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 24;
		}
		break;
	case 48: /* TABLESWITCHInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: TABLESWITCHInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 25;
		}
		break;
	case 49: /* LOOKUPSWITCHInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: LOOKUPSWITCHInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 26;
		}
		break;
	case 50: /* RETURNInstID */
		burm_label(LEFT_CHILD(a), root);
		p->isLeaf = (OP_LABEL(LEFT_CHILD(a)) == Instruction::NoInstID);
		/* stm: RETURNInstID(stm) */
		c = (STATE_LABEL(LEFT_CHILD(a)))->cost[burm_stm_NT] + 1;
		if (c + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = c + 0;
			p->rule.burm_stm = 27;
		}
		break;
	case 59: /* NoInstID */
		/* stm: NoInstID */
		if (1 + 0 < p->cost[burm_stm_NT]) {
			p->cost[burm_stm_NT] = 1 + 0;
			p->rule.burm_stm = 28;
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
	case 29: /* stm: ADDInstID(CONSTInstID,CONSTInstID) */
	case 28: /* stm: NoInstID */
	case 23: /* stm: LOADInstID */
	case 19: /* stm: CONSTInstID */
	case 14: /* stm: USHRInstID */
	case 13: /* stm: SHLInstID */
	case 3: /* stm: CHECKNULLInstID */
	case 2: /* stm: POPInstID */
	case 1: /* stm: NOPInstID */
		break;
	case 32: /* stm: MULInstID(stm,CONSTInstID) */
	case 31: /* stm: SUBInstID(stm,CONSTInstID) */
	case 30: /* stm: ADDInstID(stm,CONSTInstID) */
	case 27: /* stm: RETURNInstID(stm) */
	case 26: /* stm: LOOKUPSWITCHInstID(stm) */
	case 25: /* stm: TABLESWITCHInstID(stm) */
	case 22: /* stm: ALOADInstID(stm) */
	case 7: /* stm: CASTInstID(stm) */
	case 6: /* stm: NEGInstID(stm) */
	case 4: /* stm: ARRAYLENGTHInstID(stm) */
		kids[0] = LEFT_CHILD(p);
		break;
	case 39: /* stm: AREFInstID(stm,stm) */
	case 24: /* stm: IFInstID(stm,stm) */
	case 21: /* stm: ASTOREInstID(stm,stm) */
	case 20: /* stm: AREFInstID(stm,stm) */
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
	case 40: /* stm: ALOADInstID(AREFInstID(stm,stm)) */
	case 33: /* stm: ADDInstID(ADDInstID(stm,stm),CONSTInstID) */
		kids[0] = LEFT_CHILD(LEFT_CHILD(p));
		kids[1] = RIGHT_CHILD(LEFT_CHILD(p));
		break;
	case 35: /* stm: ADDInstID(stm,MULInstID(stm,CONSTInstID)) */
	case 34: /* stm: ADDInstID(stm,ADDInstID(stm,CONSTInstID)) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = LEFT_CHILD(RIGHT_CHILD(p));
		break;
	case 36: /* stm: ADDInstID(MULInstID(stm,CONSTInstID),stm) */
		kids[0] = LEFT_CHILD(LEFT_CHILD(p));
		kids[1] = RIGHT_CHILD(p);
		break;
	case 37: /* stm: ADDInstID(ADDInstID(stm,MULInstID(stm,CONSTInstID)),CONSTInstID) */
		kids[0] = LEFT_CHILD(LEFT_CHILD(p));
		kids[1] = LEFT_CHILD(RIGHT_CHILD(LEFT_CHILD(p)));
		break;
	case 38: /* stm: ADDInstID(stm,ADDInstID(MULInstID(stm,CONSTInstID),CONSTInstID)) */
		kids[0] = LEFT_CHILD(p);
		kids[1] = LEFT_CHILD(LEFT_CHILD(RIGHT_CHILD(p)));
		break;
	case 41: /* stm: ASTOREInstID(AREFInstID(stm,stm),stm) */
		kids[0] = LEFT_CHILD(LEFT_CHILD(p));
		kids[1] = RIGHT_CHILD(LEFT_CHILD(p));
		kids[2] = RIGHT_CHILD(p);
		break;
	default:
		ABORT_MSG("burm_kids", "Bad rule number " << eruleno << nl);
	}
}


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
