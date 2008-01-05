/* src/vm/jit/python.c - Python pass

   Copyright (C) 1996-2005, 2006, 2007 R. Grafl, A. Krall, C. Kruegel,
   C. Oates, R. Obermaisser, M. Platter, M. Probst, S. Ring,
   E. Steiner, C. Thalinger, D. Thuernbeck, P. Tomsich, C. Ullrich,
   J. Wenninger, Institut f. Computersprachen - TU Wien

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

   Note: this code is currently alpha and needs to be documented.

   This code wraps the jitdata structure into a python object and
   makes it possible to implement a compiler pass as python function.

   The wrapping of cacao types to python objects is meant to be easy and
   straight-forward.
 
   Cacao structs a wrapped into a python ``wrapper'' object, the state of 
   which consists of:

    * A void pointer.
	* A pointer to a class function (see class_func), which implements the
	  object's behaviour.

   Arrays and collection-like data structures are wrapped into a python
   ``iterator'' object, the state of wich consists of:

    * A void pointer.
	* Another void pointer that is the cursor.
	* A pointer to a iterator function (see iterator_func) which implements
	  the iterator's behaviour.

   Because in python field names are identified as strings, to avoid a lot
   of string comparisons, we translate the field as early as possible into 
   an integer constant. This is achieved using the field_map array.

   We could have used a wrapper generator like swig, but we don't want to 
   wrap the rather low level C api to python 1:1. When wrappig stuff, try
   to do it rather high level and in a pythonic way. Examples:

    * Bad: instruction.flags and cacao.FLAG_UNRESOLVED == 0
	* Good: instruction.is_unresolved
	* Bad: for i in range(0, bb.icount): instr = bb.instructions[i]
	* Good: for instr in bb.instructions
*/

#include <Python.h>

#include "vm/jit/python.h"
#include "vm/jit/show.h"

/*
 * Defs
 */

struct class_state {
	jitdata *jd;
	void *vp;
};

typedef struct class_state class_state;

union class_arg {
	struct {
		int field;
		PyObject **result;
	} get;
	struct {
		int field;
		PyObject *value;
	} set;
	struct {
		PyObject *args;
		PyObject **result;
	} call;
	struct {
		PyObject **result;
	} str;
};

typedef union class_arg class_arg;

enum class_op {
	CLASS_SET_FIELD,
	CLASS_GET_FIELD,
	CLASS_CALL,
	CLASS_STR
};

typedef enum class_op class_op;

typedef int(*class_func)(class_op, class_state *, class_arg *);
#define CLASS_FUNC(name) int name(class_op op, class_state *state, class_arg *arg)

struct iterator_state {
	jitdata *jd;
	void *data;
	void *pos;
};

union iterator_arg {
	struct {
		PyObject **result;
	} get;
	struct {
		unsigned int index;
		PyObject **result;
	} subscript;
	int length;
};

typedef union iterator_arg iterator_arg;

typedef struct iterator_state iterator_state;

enum iterator_op {
	ITERATOR_INIT,
	ITERATOR_GET,
	ITERATOR_FORWARD,
	ITERATOR_END,
	ITERATOR_SUBSCRIPT,
	ITERATOR_LENGTH
};

typedef enum iterator_op iterator_op;

typedef int(*iterator_func)(iterator_op op, iterator_state *state, iterator_arg *arg);
#define ITERATOR_FUNC(name) int name (iterator_op op, iterator_state *state, iterator_arg *arg)
#define ITERATOR_SUBSCRIPT_CHECK(end) if (arg->subscript.index >= (end)) return -1

struct field_map_entry {
	const char *name;
	int tag;
};

typedef struct field_map_entry field_map_entry;

enum field {
	F_BASIC_BLOCKS,
	F_CLASS_CALL_RETURN_TYPE,
	F_CLASS_CALL_ARGS,
	F_CLASSREF,
	F_DST,
	F_DESCRIPTOR,
	F_EXCEPTION_HANDLER,
	F_FIELD_TYPE,
	F_FIELD,
	F_INSTRUCTIONS,
	F_IS_CLASS_CONSTANT,
	F_IS_UNRESOLVED,
	F_LOCAL_METHODINFO,
	F_KLASS,
	F_LINE,
	F_LOCAL_MAP,
	F_METHOD,
	F_NAME,
	F_NAME_EX,
	F_NR,
	F_OFFSET,
	F_OPCODE,
	F_OPCODE_EX,
	F_PARAM_TYPES,
	F_PREDECESSORS,
	F_REACHED,
	F_RETURN_TYPE,
	F_S1,
	F_S2,
	F_S3,
	F_SHOW,
	F_SUCCESSORS,
	F_TYPE,
	F_UNRESOLVED_FIELD,
	F_VARS
};

/* Keep it soreted alphabetically, so we can support binary search in future. */
struct field_map_entry field_map[] = {
	{ "basic_blocks", F_BASIC_BLOCKS },
	{ "call_return_type", F_CLASS_CALL_RETURN_TYPE },
	{ "call_args", F_CLASS_CALL_ARGS },
	{ "classref", F_CLASSREF },
	{ "dst", F_DST },
	{ "descriptor", F_DESCRIPTOR },
	{ "exception_handler", F_EXCEPTION_HANDLER },
	{ "field", F_FIELD },
	{ "field_type", F_FIELD_TYPE },
	{ "instructions", F_INSTRUCTIONS },
	{ "is_unresolved", F_IS_UNRESOLVED },
	{ "is_class_constant", F_IS_CLASS_CONSTANT },
	{ "klass", F_KLASS },
	{ "line", F_LINE },
	{ "local_map", F_LOCAL_MAP },
	{ "local_methodinfo", F_LOCAL_METHODINFO },
	{ "method", F_METHOD },
	{ "name", F_NAME },
	{ "name_ex", F_NAME_EX },
	{ "nr", F_NR },
	{ "offset", F_OFFSET },
	{ "opcode", F_OPCODE },
	{ "opcode_ex", F_OPCODE_EX },
	{ "param_types", F_PARAM_TYPES },
	{ "predecessors", F_PREDECESSORS },
	{ "reached", F_REACHED },
	{ "return_type", F_RETURN_TYPE },
	{ "s1", F_S1 },
	{ "s2", F_S2 },
	{ "s3", F_S3 },
	{ "show", F_SHOW },
	{ "successors", F_SUCCESSORS },
	{ "type", F_TYPE },
	{ "unresolved_field", F_UNRESOLVED_FIELD },
	{ "vars", F_VARS },
	{ NULL, 0 }
};

int field_find(const char *key) {
	field_map_entry *it;

	for (it = field_map; it->name; ++it) {
		if (strcmp(it->name, key) == 0) {
			return it->tag;
		}
	}

	return -1;
}

/*
 * Python
 */

struct wrapper {
	PyObject_HEAD
	class_state state;
	class_func func;
};

typedef struct wrapper wrapper;

PyObject *wrapper_getattr(wrapper *w, PyObject *fld) {
	class_arg arg;
	PyObject *result;

	arg.get.field = field_find(PyString_AsString(fld));
	if (arg.get.field == -1) {
		return NULL;
	}

	arg.get.result = &result;

	if (w->func(CLASS_GET_FIELD, &w->state, &arg) == -1) {
		return NULL;
	}

	return result;
}

int wrapper_setattr(wrapper *w, PyObject *fld, PyObject *value) {
	class_arg arg;

	arg.set.field = field_find(PyString_AsString(fld));
	if (arg.set.field == -1) {
		return -1;
	}
	arg.set.value = value;

	if (w->func(CLASS_SET_FIELD, &w->state, &arg) == -1) {
		return -1;
	}

	return 0;
}

extern PyTypeObject wrapper_type;

int wrapper_compare(wrapper *a, wrapper *b) {
	if (PyObject_TypeCheck(b, &wrapper_type)) {
		if (a->state.vp < b->state.vp) {
			return -1;
		} else if (a->state.vp > b->state.vp) {
			return 1;
		} else {
			return 0;
		}
	} else {
		/* If classes differ, compare classes */
		return PyObject_Compare(a->ob_type, b->ob_type);
	}
}

long wrapper_hash(wrapper *a) {
	return (long)a->state.vp;
}

PyObject *wrapper_call(wrapper *w, PyObject *args, PyObject *kw) {
	class_arg arg;
	PyObject *result;

	arg.call.args = args;
	arg.call.result = &result;

	if (w->func(CLASS_CALL, &w->state, &arg) == -1) {
		return NULL;
	}

	return result;
}

PyObject *wrapper_str(wrapper *w) {
	class_arg arg;
	PyObject *result;
	arg.str.result = &result;
	if (w->func(CLASS_STR, &w->state, &arg) == -1) {
		return PyString_FromFormat("Wrapper(0x%p)", w->state.vp);
	} else {
		return result;
	}
}

PyTypeObject wrapper_type = {
	PyObject_HEAD_INIT(NULL)
	0, /* ob_size */
	"wrapper", /* tp_name */
	sizeof(wrapper), /* tp_basicsize */
	0, /* tp_itemsize */
	0, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	wrapper_compare, /* tp_compare */
	wrapper_str, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	wrapper_hash, /* tp_hash */
	wrapper_call, /* tp_call */
	wrapper_str, /* tp_str */
	wrapper_getattr, /* tp_getattro */
	wrapper_setattr, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	0, /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	0, /* tp_iter */
	0, /* tp_iternext */
	0, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	0, /* tp_init */
	0, /* tp_alloc */
	PyType_GenericNew, /* tp_new */
};

struct iterator {
	PyObject_HEAD
	iterator_func func;
	iterator_state state;
};

typedef struct iterator iterator;

PyObject *iterator_iter(struct iterator *it) {
	Py_INCREF(it);
	return (PyObject *)it;
}

PyObject *iterator_iternext(struct iterator *it) {
	PyObject *ret;
	iterator_arg arg;

	if (it->func(ITERATOR_END, &it->state, NULL)) {
		return NULL;
	} else {
		arg.get.result = &ret;
		it->func(ITERATOR_GET, &it->state, &arg);
		it->func(ITERATOR_FORWARD, &it->state, NULL);
		return ret;
	}
}

PyObject *iterator_getitem(struct iterator *it, PyObject* item) {
	iterator_arg arg;
	PyObject *ret;

	if (PyInt_Check(item)) {
		arg.subscript.index = PyInt_AS_LONG(item);
		arg.subscript.result = &ret;
		if (index < 0) { 
			return NULL;
		} else if (it->func(ITERATOR_SUBSCRIPT, &it->state, &arg) != -1) {
			return ret;
		} else {
			return NULL;
		}
	} else {
		return NULL;
	}
}

int iterator_length(struct iterator *it) {
	iterator_arg arg;
	if (it->func(ITERATOR_LENGTH, &it->state, &arg) == -1) {
		return -1;
	} else {
		return arg.length;
	}
}

PyMappingMethods iterator_mapping = {
	iterator_length,
	iterator_getitem,
	0
};

PyTypeObject iterator_type = {
	PyObject_HEAD_INIT(NULL)
	0, /* ob_size */
	"iterator", /* tp_name */
	sizeof(iterator), /* tp_basicsize */
	0, /* tp_itemsize */
	0, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_compare */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	&iterator_mapping, /* tp_as_mapping */
	0, /* tp_hash */
	0, /* tp_call */
	0, /* tp_str */
	0, /* tp_getattro */
	0, /* tp_setattro */
	0, /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
	0, /* tp_doc */
	0, /* tp_traverse */
	0, /* tp_clear */
	0, /* tp_richcompare */
	0, /* tp_weaklistoffset */
	iterator_iter, /* tp_iter */
	iterator_iternext, /* tp_iternext */
	0, /* tp_methods */
	0, /* tp_members */
	0, /* tp_getset */
	0, /* tp_base */
	0, /* tp_dict */
	0, /* tp_descr_get */
	0, /* tp_descr_set */
	0, /* tp_dictoffset */
	0, /* tp_init */
	0, /* tp_alloc */
	PyType_GenericNew, /* tp_new */
};

/*
 * Utils
 */

int set_int(int *p, PyObject *o) {
	if (PyInt_Check(o)) {
		*p = PyInt_AsLong(o);	
		return 0;
	} else {
		return -1;
	}
}

int get_int(PyObject **o, int p) {
	*o = Py_BuildValue("i", p);
	return 0;
}

int get_string(PyObject **o, const char *str) {
	*o = PyString_FromString(str);
	return 0;
}

int get_obj(PyObject **res, class_func f, jitdata *jd, void *p) {
	if (p == NULL) {
		return get_none(res);
	} else {
		PyObject *o = PyObject_CallObject((PyObject *)&wrapper_type, NULL);
		struct wrapper * w = (struct wrapper *)o;
		w->func = f;
		w->state.jd = jd;
		w->state.vp = p;
		*res = o;
		return 0;
	}
}

int get_true(PyObject **res) {
	Py_INCREF(Py_True);
	*res = Py_True;
	return 0;
}

int get_false(PyObject **res) {
	Py_INCREF(Py_False);
	*res = Py_False;
	return 0;
}

int get_none(PyObject **res) {
	Py_INCREF(Py_None);
	*res = Py_None;
	return 0;
}

int get_bool(PyObject **res, int cond) {
	return cond ? get_true(res) : get_false(res);
}
	
int get_iter(PyObject **res, iterator_func f, jitdata *jd, void *p) {
	PyObject *o = PyObject_CallObject((PyObject *)&iterator_type, NULL);
	struct iterator * it = (struct iterator *)o;
	it->func = f;
	it->state.jd = jd;
	it->state.data = p;
	f(ITERATOR_INIT, &it->state, NULL);
	*res = o;
	return 0;
}

int add_const(PyObject *module, const char *name, int value) {
	PyObject *pyvalue = PyInt_FromLong(value);
	if (pyvalue != NULL) {
		PyModule_AddObject(module, name, pyvalue);
	}
}

/*
 * Implemnetation
 */

CLASS_FUNC(basicblock_func);
CLASS_FUNC(classinfo_func);
CLASS_FUNC(constant_classref_func);
CLASS_FUNC(methodinfo_func);

static inline methoddesc *instruction_call_site(instruction *iptr) {
	if (iptr->opc == ICMD_BUILTIN) {
		return iptr->sx.s23.s3.bte->md;
	} else if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
		return iptr->sx.s23.s3.um->methodref->parseddesc.md;
	} else {
		return iptr->sx.s23.s3.fmiref->p.method->parseddesc;
	}
}

ITERATOR_FUNC(call_args_iter_func) {
	instruction *iptr = (instruction *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = iptr->sx.s23.s2.args;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = instruction_call_site(iptr)->paramcount;
			return 0;
		case ITERATOR_GET:
			return get_int(arg->get.result, *(int *)state->pos);
		case ITERATOR_END:
			return state->pos == (iptr->sx.s23.s2.args + instruction_call_site(iptr)->paramcount);
		case ITERATOR_FORWARD:
			state->pos = ((int *)state->pos) + 1;
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(instruction_call_site(iptr)->paramcount);
			return get_int(arg->subscript.result, iptr->sx.s23.s2.args[arg->subscript.index]);
	}
	return -1;
}

CLASS_FUNC(fieldinfo_func) {
	fieldinfo *fi = (fieldinfo *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_TYPE:
					return get_int(arg->get.result, fi->type);
				case F_OFFSET:
					return get_int(arg->get.result, fi->offset);
				case F_NAME:
					return get_string(arg->get.result, fi->name->text);
				case F_KLASS:
					return get_obj(arg->get.result, classinfo_func, state->jd, fi->class);
			}
	}

	return -1;
}

CLASS_FUNC(unresolved_field_func) {
	unresolved_field *uf = (unresolved_field *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_NAME:
					return get_string(arg->get.result, uf->fieldref->name->text);
				case F_CLASSREF:
					if (IS_FMIREF_RESOLVED(uf->fieldref)) {
						return get_none(arg->get.result);
					} else {
						return get_obj(arg->get.result, constant_classref_func, state->jd, uf->fieldref->p.classref);
					}
				case F_DESCRIPTOR:
					return get_string(arg->get.result, uf->fieldref->descriptor->text);
				case F_FIELD:
					if (IS_FMIREF_RESOLVED(uf->fieldref)) {
						return get_obj(arg->get.result, fieldinfo_func, state->jd, uf->fieldref->p.field);
					} else {
						return get_none(arg->get.result);
					}
				case F_IS_UNRESOLVED:
					return get_bool(arg->get.result, !IS_FMIREF_RESOLVED(uf->fieldref));
			}
	}
	return -1;
}

CLASS_FUNC(instruction_show_func) {
	switch (op) {
		case CLASS_CALL:
			show_icmd(state->jd, (instruction *)state->vp, 1, SHOW_REGS);	
			return get_none(arg->get.result);
	}
	return -1;
}

CLASS_FUNC(instruction_func) {

	instruction *iptr = (instruction *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_OPCODE:
					return get_int(arg->get.result, iptr->opc);
				case F_OPCODE_EX:
					if (iptr->opc == ICMD_BUILTIN) {
						return get_int(arg->get.result, iptr->sx.s23.s3.bte->opcode);
					} else {
						return get_int(arg->get.result, iptr->opc);
					}
				case F_NAME:
					return get_string(arg->get.result, icmd_table[iptr->opc].name);
				case F_NAME_EX:
					if (iptr->opc == ICMD_BUILTIN) {
						return get_string(arg->get.result, icmd_table[iptr->sx.s23.s3.bte->opcode].name);
					} else {
						return get_string(arg->get.result, icmd_table[iptr->opc].name);
					}
				case F_S1:
					return get_int(arg->get.result, iptr->s1.varindex);
				case F_S2:
					return get_int(arg->get.result, iptr->sx.s23.s2.varindex);
				case F_S3:
					return get_int(arg->get.result, iptr->sx.s23.s3.varindex);
				case F_DST:
					return get_int(arg->get.result, iptr->dst.varindex);
				case F_CLASS_CALL_RETURN_TYPE:
					return get_int(arg->get.result, instruction_call_site(iptr)->returntype.type);
				case F_CLASS_CALL_ARGS:
					return get_iter(arg->get.result, call_args_iter_func, state->jd, iptr);	
				case F_IS_UNRESOLVED:
					return get_bool(arg->get.result, iptr->flags.bits & INS_FLAG_UNRESOLVED);
				case F_IS_CLASS_CONSTANT:
					return get_bool(arg->get.result, iptr->flags.bits & INS_FLAG_CLASS);
				case F_KLASS:
					return get_obj(arg->get.result, classinfo_func, state->jd, iptr->sx.val.c.cls);
				case F_CLASSREF:
					return get_obj(arg->get.result, constant_classref_func, state->jd, iptr->sx.val.c.ref);
				case F_LOCAL_METHODINFO:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_none(arg->get.result);
					} else {	
						return get_obj(arg->get.result, methodinfo_func, 
							state->jd, iptr->sx.s23.s3.fmiref->p.method);
					}
				case F_FIELD_TYPE:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_int(arg->get.result,
							iptr->sx.s23.s3.uf->fieldref->parseddesc.fd->type);
					} else {
						return get_int(arg->get.result,
							iptr->sx.s23.s3.fmiref->p.field->type);
					}
				case F_FIELD:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_none(arg->get.result);
					} else {
						return get_obj(arg->get.result, fieldinfo_func, state->jd, iptr->sx.s23.s3.fmiref->p.field);
					}
					break;
				case F_UNRESOLVED_FIELD:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_obj(arg->get.result, unresolved_field_func, state->jd, iptr->sx.s23.s3.uf);
					} else {
						return get_none(arg->get.result);
					}
					break;
				case F_LINE:
					return get_int(arg->get.result, iptr->line);
				case F_SHOW:
					return get_obj(arg->get.result, instruction_show_func, state->jd, iptr);
			}
	}

	return -1;
}

ITERATOR_FUNC(predecessors_iter_func) {
	basicblock *bptr = (basicblock *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->predecessors;
			return 0;
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->jd, *(basicblock **)state->pos);
		case ITERATOR_END:
			return 
				(state->pos == (bptr->predecessors + bptr->predecessorcount)) ||
				(bptr->predecessorcount < 0);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock **)state->pos) + 1;
			return 0;
	}

	return -1;
}

ITERATOR_FUNC(successors_iter_func) {
	basicblock *bptr = (basicblock *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->successors;
			return 0;
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->jd, *(basicblock **)state->pos);
		case ITERATOR_END:
			return 
				(state->pos == (bptr->successors + bptr->successorcount)) || 
				(bptr->successorcount < 0);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock **)state->pos) + 1;
			return 0;
	}

	return  -1;
}

ITERATOR_FUNC(instruction_iter_func) {
	basicblock *bptr = (basicblock *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->iinstr;
			return 0;
		case ITERATOR_GET:
			return get_obj(arg->get.result, instruction_func, state->jd, state->pos);
		case ITERATOR_FORWARD:
			state->pos = ((instruction *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (bptr->iinstr + bptr->icount);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->icount);
			return get_obj(arg->subscript.result, instruction_func, state->jd, bptr->iinstr + arg->subscript.index);
		case ITERATOR_LENGTH:
			arg->length = bptr->icount;
			return 0;
	}
	return -1;
}

CLASS_FUNC(basicblock_show_func) {
	basicblock *bptr = (basicblock *)state->vp;
	switch (op) {
		case CLASS_CALL:
			show_basicblock(state->jd, bptr, SHOW_REGS);
			return get_none(arg->call.result);
	}
	return -1;
}

CLASS_FUNC(basicblock_func) {
	basicblock *bptr = (basicblock *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_INSTRUCTIONS:
					return get_iter(arg->get.result, instruction_iter_func, state->jd, bptr);
				case F_NR:
					return get_int(arg->get.result, bptr->nr);
				case F_PREDECESSORS:
					return get_iter(arg->get.result, predecessors_iter_func, state->jd, bptr);
				case F_SUCCESSORS:
					return get_iter(arg->get.result, successors_iter_func, state->jd, bptr);
				case F_REACHED:
					return get_bool(arg->get.result, bptr->flags >= BBREACHED);
				case F_EXCEPTION_HANDLER:
					return get_bool(arg->get.result, bptr->type == BBTYPE_EXH);
				case F_SHOW:
					return get_obj(arg->get.result, basicblock_show_func, state->jd, bptr);
			}
		case CLASS_STR:
			*arg->str.result = PyString_FromFormat("BB_%d", bptr->nr);
			return 0;
	}

	return -1;
}

ITERATOR_FUNC(basicblocks_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	basicblock *bb;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = jd->basicblocks;
			return 0;	
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->jd, state->pos);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock *)(state->pos))->next;
			return 0;
		case ITERATOR_END:
			return (state->pos == NULL);
		case ITERATOR_SUBSCRIPT:
			for (bb = jd->basicblocks; bb != NULL; bb = bb->next) {
				if (bb->nr == arg->subscript.index) {
					return get_obj(arg->subscript.result, basicblock_func, state->jd, bb);
				}
			}
			return -1;
	}

	return -1;
}

CLASS_FUNC(classinfo_func) {
	classinfo *c = (classinfo *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_NAME:
					return get_string(arg->get.result, c->name->text);
			}
	}
	return -1;
}

CLASS_FUNC(constant_classref_func) {
	constant_classref *cr = (constant_classref *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_NAME:
					return get_string(arg->get.result, cr->name->text);
			}
	}
	return -1;
}

ITERATOR_FUNC(param_types_iter_func) {
	methodinfo *m = (methodinfo *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = m->parseddesc->paramtypes;
			return 0;
		case ITERATOR_END:
			return state->pos == (m->parseddesc->paramtypes + m->parseddesc->paramcount);
		case ITERATOR_FORWARD:
			state->pos = ((typedesc *)state->pos) + 1;
			return 0;
		case ITERATOR_GET:
			return get_int(arg->get.result, ((typedesc *)state->pos)->type);
		case ITERATOR_LENGTH:
			arg->length = m->parseddesc->paramcount;
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(m->parseddesc->paramcount);
			return get_int(arg->subscript.result, m->parseddesc->paramtypes[arg->subscript.index].type);
	}

	return -1;
}

CLASS_FUNC(methodinfo_func) {
	methodinfo *m = (methodinfo *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_NAME:
					return get_string(arg->get.result, m->name->text);
				case F_KLASS:
					return get_obj(arg->get.result, classinfo_func, state->jd, m->class);
				case F_PARAM_TYPES:
					return get_iter(arg->get.result, param_types_iter_func, state->jd, m);
				case F_RETURN_TYPE:
					return get_int(arg->get.result, m->parseddesc->returntype.type);
			}
	}
	return -1;
}


CLASS_FUNC(varinfo_func) {
	varinfo *var = (varinfo *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_TYPE:
					return get_int(arg->get.result, var->type);
			}
	}
	return -1;
}

ITERATOR_FUNC(vars_func) {
	jitdata *jd = (jitdata *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = jd->var;
			return 0;
		case ITERATOR_FORWARD:
			state->pos = ((varinfo *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (jd->var + jd->varcount);
		case ITERATOR_GET:
			return get_obj(arg->get.result, varinfo_func, state->jd, state->pos);
		case ITERATOR_LENGTH:
			arg->length = jd->varcount;
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(jd->varcount);
			return get_obj(arg->subscript.result, varinfo_func, state->jd, jd->var + arg->subscript.index);
	}
	return -1;
}

ITERATOR_FUNC(local_map_2_iter_func) {
	int *arr = (int *)state->data;
	switch (op) {
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(5);
			return get_int(arg->subscript.result, arr[arg->subscript.index]);
		case ITERATOR_LENGTH:
			arg->length = 5;
			return 0;
	}
	return -1;
}

ITERATOR_FUNC(local_map_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	switch (op) {
		case ITERATOR_SUBSCRIPT:
			/* todo ITERATOR_SUBSCRIPT_CHECK(); */
			return get_iter(arg->subscript.result, local_map_2_iter_func, state->jd,
				jd->local_map + (5 * arg->subscript.index));
	}
	return -1;
}

CLASS_FUNC(jd_func) {
	jitdata *jd = (jitdata *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_BASIC_BLOCKS:
					return get_iter(arg->get.result, basicblocks_iter_func, state->jd, jd);
				case F_METHOD:
					return get_obj(arg->get.result, methodinfo_func, state->jd, jd->m);
				case F_VARS:
					return get_iter(arg->get.result, vars_func, state->jd, jd);
				case F_LOCAL_MAP:
					return get_iter(arg->get.result, local_map_iter_func, state->jd, jd);
			}
	}

	return -1;
}

void constants(PyObject *m) {
	char buf[32];
	char *pos;
	int i;

	/* icmds */

	for (i = 0; i < sizeof(icmd_table) / sizeof(icmd_table[0]); ++i) {
		snprintf(buf, sizeof(buf), "ICMD_%s", icmd_table[i].name);
		pos = strchr(buf, ' ');
		if (pos != NULL) {
			*pos = '\0';
		}
		add_const(m, buf, i);
	}

#	define c(x) add_const(m, #x, x)

	/* types */

	c(TYPE_INT);
	c(TYPE_LNG);
	c(TYPE_ADR);
	c(TYPE_FLT);
	c(TYPE_DBL);
	c(TYPE_VOID);
	c(UNUSED);

#	undef c
}

/*
 * Pythonpass
 */

void pythonpass_init() {
	PyObject *m;
	
	Py_Initialize();
	PyEval_InitThreads();

	if (PyType_Ready(&wrapper_type) < 0) return;
	if (PyType_Ready(&iterator_type) < 0) return;

	m = Py_InitModule3("cacao", NULL, NULL);
	if (m != NULL) {
		constants(m);
	}

}

void pythonpass_cleanup() {
	Py_Finalize();
}

int pythonpass_run(jitdata *jd, const char *module, const char *function) {
	PyObject *pymodname = NULL;
	PyObject *pymod = NULL;
	PyObject *pydict = NULL;
	PyObject *pyfunc = NULL;
	PyObject *pyargs = NULL;
	PyObject *pyret = NULL;
	PyObject *pyarg = NULL;
	int success = 0;

	pymodname = PyString_FromString(module);
	pymod = PyImport_Import(pymodname);

	if (pymod != NULL) {
		pydict = PyModule_GetDict(pymod);
		pyfunc = PyDict_GetItemString(pydict, function);
		if (pyfunc != NULL && PyCallable_Check(pyfunc)) {
			pyargs = PyTuple_New(1);

			if (get_obj(&pyarg, jd_func, jd, jd) != -1) {
			}

			/* */

			PyTuple_SetItem(pyargs, 0, pyarg);

			pyret = PyObject_CallObject(pyfunc, pyargs);
			if (pyret == NULL) {
                PyErr_Print();
			} else {
				success = 1;
			}
		} else {
			if (PyErr_Occurred())
				PyErr_Print();
		}
	} else {
		PyErr_Print();
	}

	Py_XDECREF(pymodname);
	Py_XDECREF(pymod);
	Py_XDECREF(pyargs);
	Py_XDECREF(pyret);

	return (success == 1 ? 1 : 0);
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
 * vim:noexpandtab:sw=4:ts=4:
 */
