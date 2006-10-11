{
	unresolved_method *um;      /* struct describing the called method */
	constant_FMIref *mref;           /* reference to the called method */
	methodinfo *mi;                        /* resolved method (if any) */
	methoddesc *md;                 /* descriptor of the called method */
	utf *mname;                                         /* method name */
	utf *mclassname;                     /* name of the method's class */
	bool specialmethod;            /* true if a <...> method is called */
	int opcode;                                   /* invocation opcode */
	bool callinginit;                      /* true if <init> is called */
	instruction *ins;
	classref_or_classinfo initclass;
	typedesc *td;
	s4 argindex;                            /* argument variable index */
	varinfo *av;                                  /* argument variable */
	int i;                                                  /* counter */
    u1 rtype;                          /* return type of called method */
	resolve_result_t result;
	bool invokestatic;
	bool invokespecial;

	/* get the FMIref and the unresolved_method struct (if any) */
	/* from the instruction                                     */

	if (INSTRUCTION_IS_UNRESOLVED(state->iptr)) {
		/* unresolved method */
		um = state->iptr->sx.s23.s3.um;
		mref = um->methodref;
	}
	else {
		/* resolved method */
		um = NULL;
		mref = state->iptr->sx.s23.s3.fmiref;
	}

	/* get method descriptor and name */

	md = mref->parseddesc.md;
	mname = mref->name;

	/* get method info (if resolved) and classname */

	if (IS_FMIREF_RESOLVED(mref)) {
		mi = mref->p.method;
		mclassname = mi->class->name;
	}
	else {
		mi = NULL;
		mclassname = mref->p.classref->name;
	}

	/* gather some info about this instruction */

	opcode = state->iptr->opc;
	invokestatic = (opcode == ICMD_INVOKESTATIC);
	invokespecial = (opcode == ICMD_INVOKESPECIAL);
	specialmethod = (mname->text[0] == '<');

	/* prevent compiler warnings */

	ins = NULL;

	/* check whether we are calling <init> */
	
	callinginit = (invokespecial && mname == utf_init);
	if (specialmethod && !callinginit)
		TYPECHECK_VERIFYERROR_bool("Invalid invocation of special method");

	/* allocate parameters if necessary */
	
	if (!md->params)
		if (!descriptor_params_from_paramtypes(md,
					(opcode == ICMD_INVOKESTATIC) ? ACC_STATIC : ACC_NONE))
			return false;

	/* check parameter types */

	i = md->paramcount; /* number of parameters including 'this'*/
	while (--i >= 0) {
		LOG1("\t\tparam %d",i);
		argindex = state->iptr->sx.s23.s2.args[i];
		av = VAR(argindex);
		td = md->paramtypes + i;

		if (av->type != td->type)
			TYPECHECK_VERIFYERROR_bool("Parameter type mismatch in method invocation");

		if (av->type == TYPE_ADR) {
			LOGSTR("\t\t"); LOGINFO(&(av->typeinfo));
			if (i==0 && callinginit)
			{
				/* first argument to <init> method */
				if (!TYPEINFO_IS_NEWOBJECT(av->typeinfo))
					TYPECHECK_VERIFYERROR_bool("Calling <init> on initialized object");

				/* get the address of the NEW instruction */
				LOGSTR("\t\t"); LOGINFO(&(av->typeinfo));
				ins = (instruction *) TYPEINFO_NEWOBJECT_INSTRUCTION(av->typeinfo);
				if (ins)
					initclass = ins[-1].sx.val.c;
				else
					initclass.cls = state->m->class;
				LOGSTR("\t\tclass: "); LOGNAME(initclass); LOGNL;
			}
		}
		else {
			/* non-adress argument. if this is the first argument and we are */
			/* invoking an instance method, this is an error.                */
			if (i==0 && opcode != ICMD_INVOKESTATIC) {
				TYPECHECK_VERIFYERROR_bool("Parameter type mismatch for 'this' argument");
			}
		}
		LOG("\tok");
	}

	if (callinginit) {
		LOG("\treplacing uninitialized object");
		/* replace uninitialized object type on stack */

		/* for all live-in and live-through variables */ 
		for (i=0; i<state->iptr->s1.argcount; ++i) {
			argindex = state->iptr->sx.s23.s2.args[i];
			av = VAR(argindex);
			if (av->type == TYPE_ADR
					&& TYPEINFO_IS_NEWOBJECT(av->typeinfo)
					&& TYPEINFO_NEWOBJECT_INSTRUCTION(av->typeinfo) == ins)
			{
				LOG("\treplacing uninitialized type");

				/* If this stackslot is in the instack of
				 * this basic block we must save the type(s)
				 * we are going to replace.
				 */
				/* XXX this needs a new check */
				if (state->bptr->invars
						&& argindex >= state->bptr->invars[0] 
						&& argindex < state->bptr->varstart 
						&& !state->savedinvars)
				{
					typestate_save_invars(state);
				}

				if (!typeinfo_init_class(&(av->typeinfo),initclass))
					return false;
			}
		}

		/* replace uninitialized object type in locals */
		if (!typevector_init_object(state->jd->var, ins, initclass,
					state->numlocals))
			return false;

		/* initializing the 'this' reference? */
		if (!ins) {
			classinfo *cls;
			TYPECHECK_ASSERT(state->initmethod);
			/* { we are initializing the 'this' reference }                           */
			/* must be <init> of current class or direct superclass                   */
			/* the current class is linked, so must be its superclass. thus we can be */
			/* sure that resolving will be trivial.                                   */
			if (mi) {
				cls = mi->class;
			}
			else {
				if (!resolve_classref(state->m,mref->p.classref,resolveLazy,false,true,&cls))
					return false; /* exception */
			}

			/* if lazy resolving did not succeed, it's not one of the allowed classes */
			/* otherwise we check it directly                                         */
			if (cls == NULL || (cls != state->m->class && cls != state->m->class->super.cls)) {
				TYPECHECK_VERIFYERROR_bool("<init> calling <init> of the wrong class");
			}

			/* set our marker variable to type int */
			LOG("\tsetting <init> marker");
			typevector_store(jd->var, state->numlocals-1, TYPE_INT, NULL);
		}
		else {
			/* { we are initializing an instance created with NEW } */
			if ((IS_CLASSREF(initclass) ? initclass.ref->name : initclass.cls->name) != mclassname) {
				TYPECHECK_VERIFYERROR_bool("wrong <init> called for uninitialized reference");
			}
		}
	}

	/* try to resolve the method lazily */

	result = resolve_method_lazy(state->m, mref, invokespecial);

	/* perform verification checks */

	if (result == resolveSucceeded) {

		assert(IS_FMIREF_RESOLVED(mref));

		mi = mref->p.method;

		result = resolve_method_verifier_checks(state->m, 
												mref,
												mi,
												invokestatic);
	}

	/* check types of parameters */

	if (result == resolveSucceeded && !invokestatic)
		result = resolve_method_instance_type_checks(
				state->m, mi, 
				&(VAR(state->iptr->sx.s23.s2.args[0])->typeinfo), 
				invokespecial);

	if (result == resolveSucceeded)
		result = resolve_method_param_type_checks(
				jd, state->m, state->iptr,
				mi, invokestatic);

	/* impose loading constraints */

	if (result == resolveSucceeded) {
		/* XXX state->m->class may have to be wrong when inlining */
		if (!resolve_method_loading_constraints(state->m->class, mi))
			return false;
	}

	if (result == resolveFailed)
		return false;

	if (result == resolveSucceeded) {
		/* if this call is monomorphic, turn it into an INVOKESPECIAL */

		if ((state->iptr->opc == ICMD_INVOKEVIRTUAL)
			&& (mi->flags & (ACC_FINAL | ACC_PRIVATE)))
		{
			state->iptr->opc = ICMD_INVOKESPECIAL;
		}
	}
	else {
		/* resolution must be deferred */

		if (!um) {
			um = resolve_create_unresolved_method(state->m->class, state->m,
					mref, 
					state->iptr->opc == ICMD_INVOKESTATIC,
					invokespecial);

			if (!um)
				return false;
		}

		/* record subtype constraints for parameters */

		if (!invokestatic && !resolve_constrain_unresolved_method_instance(
					um, state->m, 
					&(VAR(state->iptr->sx.s23.s2.args[0])->typeinfo),
					invokespecial))
			return false; /* XXX maybe wrap exception */

		if (!resolve_constrain_unresolved_method_params(
					jd, um, state->m, state->iptr))
			return false; /* XXX maybe wrap exception */

		/* store the unresolved_method pointer */

		state->iptr->sx.s23.s3.um = um;
		state->iptr->flags.bits |= INS_FLAG_UNRESOLVED;
	}

	/* set the return type */

	rtype = md->returntype.type;
	if (rtype != TYPE_VOID) {
		dv->type = rtype;
		if (!typeinfo_init_from_typedesc(&(md->returntype),NULL,&(dv->typeinfo)))
			return false;
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
