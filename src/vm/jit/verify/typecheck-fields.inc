{
	unresolved_field *uf;
	constant_FMIref *fieldref;
	resolve_result_t result;
	typeinfo *instanceti;
	typeinfo *valueti;

	TYPECHECK_COUNT(stat_ins_field);

	instanceti = (instance) ? &(instance->typeinfo) : NULL;
	valueti    = (value && value->type == TYPE_ADR) ? &(value->typeinfo) : NULL;

	/* get the field reference from the instruction */

	if (INSTRUCTION_IS_UNRESOLVED(state->iptr)) {
		uf = state->iptr->sx.s23.s3.uf;
		fieldref = uf->fieldref;
	}
	else {
		uf = NULL;
		fieldref = state->iptr->sx.s23.s3.fmiref;
	}

	/* try to resolve the field reference lazily */

	result = resolve_field_lazy(state->m, fieldref);

	if (result == resolveSucceeded) {
		fieldinfo *fi;

		/* perform verification checks now */

		fi  = fieldref->p.field;

		result = resolve_field_verifier_checks(
				state->m, fieldref, fi->class, fi,
				instanceti, valueti,
				(instance == NULL),
				(value != NULL));
	}

	if (result == resolveFailed)
		EXCEPTION;

	/* if not resolved, yet, create an unresolved field */

	if (result != resolveSucceeded) {
		if (!uf) {
			uf = resolve_create_unresolved_field(state->m->class, 
					state->m, state->iptr);
			if (!uf)
				EXCEPTION;

			state->iptr->sx.s23.s3.uf = uf;
			state->iptr->flags.bits |= INS_FLAG_UNRESOLVED;
		}

		/* record the subtype constraints for this field access */

		if (!resolve_constrain_unresolved_field(
					uf, state->m->class, state->m,
					instanceti, valueti))
			EXCEPTION; /* XXX maybe wrap exception? */

		TYPECHECK_COUNTIF(INSTRUCTION_IS_UNRESOLVED(state->iptr),stat_ins_field_unresolved);
		TYPECHECK_COUNTIF(INSTRUCTION_IS_RESOLVED(state->iptr) && 
				!state->iptr->sx.s23.s3.fmiref->p.field->class->initialized,
				stat_ins_field_uninitialized);
	}
		
	/* write the result type */

	if (value == NULL) {
#if defined(TYPECHECK_STACKBASED)
		typedescriptor *dv;

		if (IS_2_WORD_TYPE(fieldref->parseddesc.fd->type)) {
			/* XXX CHECK_STACK_SPACE(2); */
			stack += 2;
			dv = &(stack[-1]);
			stack[0].type = TYPE_VOID;
		}
		else {
			/* CHECK_STACK_SPACE(1); XXX */
			stack += 1;
			dv = stack;
		}
#define DST dv
#else
#define DST VAROP(state->iptr->dst)
#endif
		DST->type = fieldref->parseddesc.fd->type;
		if (DST->type == TYPE_ADR) {
			if (!typeinfo_init_from_typedesc(fieldref->parseddesc.fd,NULL,&(DST->typeinfo)))
				EXCEPTION;
		}
#undef DST
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
