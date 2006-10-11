{
	varinfo *valueslot = NULL;
	typeinfo *instanceti = NULL;
	typeinfo *valueti = NULL;
	bool isstatic = false;
	bool isput = false;
	typeinfo constti;

	TYPECHECK_COUNT(stat_ins_field);

	if (INSTRUCTION_IS_UNRESOLVED(state->iptr)) {
		uf = state->iptr->sx.s23.s3.uf;
		fieldref = uf->fieldref;
	}
	else {
		uf = NULL;
		fieldref = state->iptr->sx.s23.s3.fmiref;
	}

	/* get opcode dependent values */

	switch (state->iptr->opc) {
		case ICMD_PUTFIELD:
			isput = true;
			valueslot = VAROP(state->iptr->sx.s23.s2);
			instanceti = &(VAROP(state->iptr->s1)->typeinfo);
			break;

		case ICMD_PUTFIELDCONST:
			isput = true;
			instanceti = &(VAROP(state->iptr->s1)->typeinfo);
putfield_const_tail:
			if (IS_ADR_TYPE(fieldref->parseddesc.fd->type)) {
				/* XXX check for java.lang.Class constant values? */
				if (state->iptr->sx.val.anyptr) {
					assert(class_java_lang_String);
					assert(class_java_lang_String->state & CLASS_LOADED);
					assert(class_java_lang_String->state & CLASS_LINKED);
					typeinfo_init_classinfo(&constti, class_java_lang_String);
				}
				else {
					TYPEINFO_INIT_NULLTYPE(constti);
				}
				valueti = &constti;
			}
			break;

		case ICMD_PUTSTATIC:
			isput = true;
			isstatic = true;
			valueslot = VAROP(state->iptr->s1);
			break;

		case ICMD_PUTSTATICCONST:
			isput = true;
			isstatic = true;
			goto putfield_const_tail;

		case ICMD_GETFIELD:
			instanceti = &(VAROP(state->iptr->s1)->typeinfo);
			break;

		case ICMD_GETSTATIC:
			isstatic = true;
			break;

		default:
			assert(false);
	}

	if (valueslot && IS_ADR_TYPE(valueslot->type)) {
		valueti = &(valueslot->typeinfo);
	}

	/* try to resolve the field reference lazily */

	result = resolve_field_lazy(state->m, fieldref);

	if (result == resolveSucceeded) {
		fieldinfo *fi;

		/* perform verification checks now */

		fi  = fieldref->p.field;

		result = resolve_field_verifier_checks(
				state->m, fieldref, fi->class, fi,
				instanceti, valueti, isstatic, isput);
	}

	if (result == resolveFailed)
		return false;

	/* if not resolved, yet, create an unresolved field */

	if (result != resolveSucceeded) {
		if (!uf) {
			uf = resolve_create_unresolved_field(state->m->class, 
					state->m, state->iptr);
			if (!uf)
				return false;

			state->iptr->sx.s23.s3.uf = uf;
			state->iptr->flags.bits |= INS_FLAG_UNRESOLVED;
		}

		/* record the subtype constraints for this field access */

		if (!resolve_constrain_unresolved_field(
					uf, state->m->class, state->m,
					instanceti, valueti))
			return false; /* XXX maybe wrap exception? */

		TYPECHECK_COUNTIF(INSTRUCTION_IS_UNRESOLVED(state->iptr),stat_ins_field_unresolved);
		TYPECHECK_COUNTIF(INSTRUCTION_IS_RESOLVED(state->iptr) && 
				!state->iptr->sx.s23.s3.fmiref->p.field->class->initialized,
				stat_ins_field_uninitialized);
	}
		
	/* write the result type */

	if (iptr->opc == ICMD_GETFIELD || iptr->opc == ICMD_GETSTATIC) {
		dv->type = fieldref->parseddesc.fd->type;
		if (dv->type == TYPE_ADR) {
			if (!typeinfo_init_from_typedesc(fieldref->parseddesc.fd, 
						NULL, &(dv->typeinfo)))
				return false;
		}
	}
}

/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 * vim:noexpandtab:sw=4:ts=4:filetype=c:
 */
