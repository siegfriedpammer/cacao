{
	builtintable_entry *bte;
    classref_or_classinfo cls;
    varinfo *dv;               /* output variable of current instruction */
	jitdata *jd = state->jd;

	bte = state->iptr->sx.s23.s3.bte;
	dv = VAROP(state->iptr->dst);

	/* XXX this is an ugly if-chain but twisti did not want a function */
	/* pointer in builtintable_entry for this, so here you go.. ;)     */

	if (ISBUILTIN(BUILTIN_new)) {
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_new without class");
		cls = state->iptr[-1].sx.val.c;
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_NEWOBJECT(dv->typeinfo,state->iptr);
	}
	else if (ISBUILTIN(BUILTIN_newarray_boolean)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_BOOLEAN);
	}
	else if (ISBUILTIN(BUILTIN_newarray_char)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_CHAR);
	}
	else if (ISBUILTIN(BUILTIN_newarray_float)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_FLOAT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_double)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_DOUBLE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_byte)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_BYTE);
	}
	else if (ISBUILTIN(BUILTIN_newarray_short)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_SHORT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_int)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_INT);
	}
	else if (ISBUILTIN(BUILTIN_newarray_long)) {
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		dv->type = TYPE_ADR;
		TYPEINFO_INIT_PRIMITIVE_ARRAY(dv->typeinfo,ARRAYTYPE_LONG);
	}
	else if (ISBUILTIN(BUILTIN_newarray))
	{
		TYPECHECK_INT(state->iptr->sx.s23.s2.args[0]);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_newarray without class");
		/* XXX check that it is an array class(ref) */
		dv->type = TYPE_ADR;
		typeinfo_init_class(&(dv->typeinfo),state->iptr[-1].sx.val.c);
	}
	else if (ISBUILTIN(BUILTIN_arrayinstanceof))
	{
		TYPECHECK_ADR(state->iptr->sx.s23.s2.args[0]);
		if (state->iptr[-1].opc != ICMD_ACONST)
			TYPECHECK_VERIFYERROR_bool("illegal instruction: builtin_arrayinstanceof without class");
		dv->type = TYPE_INT;
		/* XXX check that it is an array class(ref) */
	}
	else {
		methoddesc *md;
		s4 i;
		u1 rtype;
		varinfo *av;

		/* verify a generic builtin call */

		TYPECHECK_COUNT(stat_ins_builtin_gen);

		md = bte->md;
		i = md->paramcount;

		/* check the types of the arguments on the stack */

		for (i--; i >= 0; i--) {
			av = VAR(state->iptr->sx.s23.s2.args[i]);

			if (av->type != md->paramtypes[i].type) {
				TYPECHECK_VERIFYERROR_bool("parameter type mismatch for builtin method");
			}

#ifdef TYPECHECK_DEBUG
			/* generic builtins may only take primitive types and java.lang.Object references */
			if (av->type == TYPE_ADR && md->paramtypes[i].classref->name != utf_java_lang_Object) {
				*exceptionptr = new_internalerror("generic builtin method with non-generic reference parameter");
				return false;
			}
#endif
		}

		/* set the return type */

		rtype = md->returntype.type;
		if (rtype != TYPE_VOID) {
			varinfo *dv;

			dv = VAROP(state->iptr->dst);
			dv->type = rtype;
			if (!typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dv->typeinfo)))
				return false;
		}

		return true;
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
