/* src/vm/jit/python.c - Python pass

   Copyright (C) 2007, 2008
   CACAOVM - Verein zu Foerderung der freien virtuellen Machine CACAO

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
   wrap the rather low level C api to python 1:1. When wrapping stuff, try
   to do it rather high level and in a pythonic way. Examples:

    * Bad: instruction.flags and cacao.FLAG_UNRESOLVED == 0
	* Good: instruction.is_unresolved
	* Bad: for i in range(0, bb.icount): instr = bb.instructions[i]
	* Good: for instr in bb.instructions

  Adding instructions or variables is currently problematic, because it 
  requires to resize fixed sized arrays. Reallocating an array means that
  all elements are possibly moved, their addresses are changed and the 
  associated python object become invalid. Further, usually there is the 
  need to add several instructions, which possibly results in several 
  reallocations of the array. A good solution would be:

   * Copy-on-write the array (ex. bptr->instructions) into a python list,
     and put that list into the dictionnary of the parent object.
   * When the python parent object is destroyed, recreate the array from the
     list.
   * From python, bptr.instructions will return either the wrapped array, or
     the list from the dictionnary.

*/

#include <Python.h>
#include <structmember.h>

#include "vm/global.h"
#include "vm/jit/python.h"
#include "vm/jit/show.h"
#if defined(ENABLE_THREADS)
# include "threads/lock-common.h"
#endif

#if defined(ENABLE_THREADS)
static java_object_t *python_global_lock;
#endif

/*
 * Defs
 */

typedef struct root_state root_state;

struct root_state {
	jitdata *jd;
	PyObject *object_cache;
};

typedef struct class_state class_state;

struct class_state {
	root_state *root;
	void *vp;
};

union class_arg {
	struct {
		int is_method;
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
		int method;
		PyObject *args;
		PyObject **result;
	} method_call;
	struct {
		PyObject **result;
	} str;
	void *key;
};

typedef union class_arg class_arg;

enum class_op {
	CLASS_SET_FIELD,
	CLASS_GET_FIELD,
	CLASS_CALL,
	CLASS_STR,
	CLASS_METHOD_CALL
};

typedef enum class_op class_op;

typedef int(*class_func)(class_op, class_state *, class_arg *);
#define CLASS_FUNC(name) int name(class_op op, class_state *state, class_arg *arg)
#define CLASS_FUNC_CALL(name) name(op, state, arg)

struct iterator_state {
	root_state *root;
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
	struct {
		unsigned int index;
		PyObject *value;
	} setitem;
};

typedef union iterator_arg iterator_arg;

typedef struct iterator_state iterator_state;

enum iterator_op {
	ITERATOR_INIT,
	ITERATOR_GET,
	ITERATOR_FORWARD,
	ITERATOR_END,
	ITERATOR_SUBSCRIPT,
	ITERATOR_LENGTH,
	ITERATOR_SETITEM
};

typedef enum iterator_op iterator_op;

typedef int(*iterator_func)(iterator_op op, iterator_state *state, iterator_arg *arg);
#define ITERATOR_FUNC(name) int name (iterator_op op, iterator_state *state, iterator_arg *arg)
#define ITERATOR_SUBSCRIPT_CHECK(end) if (arg->subscript.index >= (end)) return -1
#define ITERATOR_SETITEM_CHECK(end) if (arg->setitem.index >= (end)) return -1

typedef struct method_state method_state;

struct method_state {
	int method;
	class_state *cstate;
};

struct field_map_entry {
	const char *name;
	int tag;
};

typedef struct field_map_entry field_map_entry;

enum field {
	F_BASIC_BLOCKS,
	F_CALL_RETURN_TYPE,
	F_CALL_ARGS,
	F_CLASSREF,
	F_CONTROL_FLOW,
	F_CONTROL_FLOW_EX,
	F_DATA_FLOW,
	F_DATA_FLOW_EX,
	F_DESCRIPTOR,
	F_DOM_SUCCESSORS,
	F_DOMINANCE_FRONTIER,
	F_DST,
	F_END,
	F_EXCEPTION_HANDLER,
	F_EXCEPTION_TABLE,
	F_FIELD,
	F_FIELD_TYPE,
	F_HANDLER,
	F_HAS_CALL_ARGS,
	F_HAS_DST,
	F_IDOM,
	F_INDEX,
	F_INSTRUCTIONS,
	F_INTERFACE_MAP,
	F_IN_VARS,
	F_IS_CLASS_CONSTANT,
	F_IS_IN_MEMORY,
	F_IS_INOUT,
	F_IS_LOCAL,
	F_IS_PREALLOCATED,
	F_IS_SAVED,
	F_IS_TEMPORARY,
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
	F_OUT_VARS,
	F_PARAMS,
	F_PARAM_TYPES,
	F_PEI,
	F_PEI_EX,
	F_PREDECESSORS,
	F_REACHED,
	F_REGISTER_OFFSET,
	F_RETURN_TYPE,
	F_S,
	F_SHOW,
	F_SUCCESSORS,
	F_START,
	F_TYPE,
	F_UNRESOLVED_FIELD,
	F_UNUSED,
	F_VARS
};

/* Keep it soreted alphabetically, so we can support binary search in future. */
struct field_map_entry field_map[] = {
	{ "basic_blocks", F_BASIC_BLOCKS },
	{ "call_return_type", F_CALL_RETURN_TYPE },
	{ "call_args", F_CALL_ARGS },
	{ "classref", F_CLASSREF },
	{ "control_flow", F_CONTROL_FLOW },
	{ "control_flow_ex", F_CONTROL_FLOW_EX },
	{ "data_flow", F_DATA_FLOW },
	{ "data_flow_ex", F_DATA_FLOW_EX },
	{ "descriptor", F_DESCRIPTOR },
	{ "dom_successors", F_DOM_SUCCESSORS },
	{ "dominance_frontier", F_DOMINANCE_FRONTIER },
	{ "dst", F_DST },
	{ "end", F_END },
	{ "exception_handler", F_EXCEPTION_HANDLER },
	{ "exception_table", F_EXCEPTION_TABLE },
	{ "field", F_FIELD },
	{ "field_type", F_FIELD_TYPE },
	{ "handler", F_HANDLER },
	{ "has_call_args", F_HAS_CALL_ARGS },
	{ "has_dst", F_HAS_DST },
	{ "idom", F_IDOM, },
	{ "index", F_INDEX },
	{ "instructions", F_INSTRUCTIONS },
	{ "interface_map", F_INTERFACE_MAP },
	{ "in_vars", F_IN_VARS },
	{ "is_class_constant", F_IS_CLASS_CONSTANT },
	{ "is_inout", F_IS_INOUT },
	{ "is_in_memory", F_IS_IN_MEMORY },
	{ "is_local", F_IS_LOCAL },
	{ "is_preallocated", F_IS_PREALLOCATED },
	{ "is_saved", F_IS_SAVED },
	{ "is_temporary", F_IS_TEMPORARY },
	{ "is_unresolved", F_IS_UNRESOLVED },
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
	{ "out_vars", F_OUT_VARS },
	{ "params", F_PARAMS },
	{ "param_types", F_PARAM_TYPES },
	{ "pei", F_PEI },
	{ "pei_ex", F_PEI_EX },
	{ "predecessors", F_PREDECESSORS },
	{ "reached", F_REACHED },
	{ "register_offset", F_REGISTER_OFFSET },
	{ "return_type", F_RETURN_TYPE },
	{ "s", F_S },
	{ "show", F_SHOW },
	{ "start", F_START },
	{ "successors", F_SUCCESSORS },
	{ "type", F_TYPE },
	{ "unresolved_field", F_UNRESOLVED_FIELD },
	{ "unused", F_UNUSED },
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

typedef struct method method;

struct method {
	PyObject_HEAD;
	class_func func;
	method_state state;
};

PyObject *method_call(method *m, PyObject *args, PyObject *kw) {
	class_arg arg;
	PyObject *result = NULL;

	arg.method_call.method = m->state.method;
	arg.method_call.args = args;
	arg.method_call.result = &result;

	if (m->func(CLASS_METHOD_CALL, m->state.cstate, &arg) == -1) {
		return NULL;
	}

	if (result == NULL) {
		Py_INCREF(Py_None);
		result = Py_None;
	}

	return result;
}

PyTypeObject method_type = {
	PyObject_HEAD_INIT(NULL)
	0, /* ob_size */
	"method", /* tp_name */
	sizeof(method), /* tp_basicsize */
	0, /* tp_itemsize */
	0, /* tp_dealloc */
	0, /* tp_print */
	0, /* tp_getattr */
	0, /* tp_setattr */
	0, /* tp_compare */
	0, /* tp_repr */
	0, /* tp_as_number */
	0, /* tp_as_sequence */
	0, /* tp_as_mapping */
	0, /* tp_hash */
	method_call, /* tp_call */
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

struct wrapper {
	PyObject_HEAD
	class_state state;
	class_func func;
	PyObject *dict;
};

typedef struct wrapper wrapper;

PyObject *wrapper_getattr(wrapper *w, PyObject *fld) {
	class_arg arg;
	PyObject *result;

	/* First, try generic getattr */

	result = PyObject_GenericGetAttr(w, fld);

	if (result != NULL) {
		return result;
	}

	/* Exception is set here */

	arg.get.field = field_find(PyString_AsString(fld));
	if (arg.get.field == -1) {
		goto failout;
	}

	arg.get.is_method = 0;
	arg.get.result = &result;

	if (w->func(CLASS_GET_FIELD, &w->state, &arg) == -1) {
		goto failout;
	}

	if (arg.get.is_method) {
		result = PyObject_CallObject((PyObject *)&method_type, NULL);
		method *m = (method *)result;
		m->func = w->func;
		m->state.method = arg.get.field;
		m->state.cstate = &w->state;
	}

	PyErr_Clear();

	return result;

failout:

	return NULL;
}

int wrapper_setattr(wrapper *w, PyObject *fld, PyObject *value) {
	class_arg arg;

	arg.set.field = field_find(PyString_AsString(fld));
	if (arg.set.field == -1) {
		goto failout;
	}
	arg.set.value = value;

	if (w->func(CLASS_SET_FIELD, &w->state, &arg) == -1) {
		goto failout;
	}

	return 0;

failout:

	return PyObject_GenericSetAttr(w, fld, value);
}

extern PyTypeObject wrapper_type;

inline void *wrapper_key(wrapper *w) {
	return w->state.vp;
}

int wrapper_compare(wrapper *a, wrapper *b) {
	void *keya, *keyb;
	if (PyObject_TypeCheck(b, &wrapper_type)) {
		keya = wrapper_key(a);
		keyb = wrapper_key(b);
		if (keya < keyb) {
			return -1;
		} else if (keya > keyb) {
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
	return (long)wrapper_key(a);
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
	offsetof(wrapper, dict), /* tp_dictoffset */
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

int iterator_setitem(struct iterator *it, PyObject *item, PyObject *value) {
	iterator_arg arg;
	if (PyInt_Check(item)) {
		arg.setitem.index = PyInt_AS_LONG(item);
		arg.setitem.value = value;
		if (it->func(ITERATOR_SETITEM, &it->state, &arg) != -1) {
			return 0;
		} else {
			return -1;
		}
	} else {
		return -1;
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
	iterator_setitem
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

int set_s4(s4 *p, PyObject *o) {
	if (PyInt_Check(o)) {
		*p = PyInt_AsLong(o);	
		return 0;
	} else {
		return -1;
	}
}

int set_s4_flag(s4 *p, s4 flag, PyObject *o) {
	if (o == Py_True) {
		*p |= flag;
		return 0;
	} else if (o == Py_False) {
		*p &= ~flag;
		return 0;
	} else {
		return -1;
	}
}

int get_int(PyObject **o, int p) {
	*o = PyInt_FromLong(p);
	return 0;
}

int get_string(PyObject **o, const char *str) {
	*o = PyString_FromString(str);
	return 0;
}

int get_obj(PyObject **res, class_func f, root_state *root, void *p) {
	if (p == NULL) {
		return get_none(res);
	} else {
		PyObject *key = PyInt_FromLong((long)p);
		PyObject *o = PyDict_GetItem(root->object_cache, key);
		if (o == NULL) {
			o = PyObject_CallObject((PyObject *)&wrapper_type, NULL);
			struct wrapper * w = (struct wrapper *)o;
			w->func = f;
			w->state.root = root;
			w->state.vp = p;
			PyDict_SetItem(root->object_cache, key, o);
		} else {
			Py_INCREF(o);
		}
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
	
int get_iter(PyObject **res, iterator_func f, root_state *root, void *p) {
	PyObject *o = PyObject_CallObject((PyObject *)&iterator_type, NULL);
	struct iterator * it = (struct iterator *)o;
	it->func = f;
	it->state.root = root;
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

void *get_vp(PyObject *o, class_func func) {
	if (o->ob_type == &wrapper_type) {
		if (((wrapper *)o)->func == func) {
			return ((wrapper *)o)->state.vp;
		}
	}
	return NULL;
}

/*
 * Implemnetation
 */

CLASS_FUNC(basicblock_func);
CLASS_FUNC(classinfo_func);
CLASS_FUNC(constant_classref_func);
CLASS_FUNC(methodinfo_func);
CLASS_FUNC(varinfo_func);

int get_varinfo(PyObject **res, root_state *root, s4 index) {
	return get_obj(res, varinfo_func, root, root->jd->var + index);
}

static inline int instruction_opcode_ex(instruction *iptr) {
	if (iptr->opc == ICMD_BUILTIN) {
		return iptr->sx.s23.s3.bte->opcode;
	} else {
		return iptr->opc;
	}
}

ITERATOR_FUNC(call_args_iter_func) {
	instruction *iptr = (instruction *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = iptr->sx.s23.s2.args;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = iptr->s1.argcount;
			return 0;
		case ITERATOR_GET:
			/* return get_int(arg->get.result, *(int *)state->pos);*/
			return get_varinfo(arg->get.result, state->root, *(int *)state->pos);
		case ITERATOR_END:
			return state->pos == iptr->sx.s23.s2.args + iptr->s1.argcount;
		case ITERATOR_FORWARD:
			state->pos = ((int *)state->pos) + 1;
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(iptr->s1.argcount);
			return get_int(arg->subscript.result, iptr->sx.s23.s2.args[arg->subscript.index]);
		case ITERATOR_SETITEM:
			ITERATOR_SETITEM_CHECK(iptr->s1.argcount);
			return set_s4(iptr->sx.s23.s2.args + arg->setitem.index, arg->setitem.value);
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
					return get_obj(arg->get.result, classinfo_func, state->root, fi->clazz);
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
						return get_obj(arg->get.result, constant_classref_func, state->root, uf->fieldref->p.classref);
					}
				case F_DESCRIPTOR:
					return get_string(arg->get.result, uf->fieldref->descriptor->text);
				case F_FIELD:
					if (IS_FMIREF_RESOLVED(uf->fieldref)) {
						return get_obj(arg->get.result, fieldinfo_func, state->root, uf->fieldref->p.field);
					} else {
						return get_none(arg->get.result);
					}
				case F_IS_UNRESOLVED:
					return get_bool(arg->get.result, !IS_FMIREF_RESOLVED(uf->fieldref));
			}
	}
	return -1;
}

static inline int instruction_num_s(instruction *iptr) {
	switch (icmd_table[iptr->opc].dataflow) {
		case DF_1_TO_0:
		case DF_1_TO_1:
		case DF_COPY:
		case DF_MOVE:
			return 1;
		case DF_2_TO_0:
		case DF_2_TO_1:
			return 2;
		case DF_3_TO_0:
		case DF_3_TO_1:
			return 3;
		default:
			return 0;
	}
}

static inline s4 *instruction_get_s(instruction *iptr, int s) {
	switch (s) {
		case 0:
			return &(iptr->s1.varindex);
		case 1:
			return &(iptr->sx.s23.s2.varindex);
		case 2:
			return &(iptr->sx.s23.s3.varindex);
	}
}

ITERATOR_FUNC(s_iter_func) {
	instruction *iptr = (instruction *)state->data;
	uintptr_t pos = (uintptr_t)state->pos;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = (void *)0;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = instruction_num_s(iptr);
			return 0;
		case ITERATOR_GET:
			return get_varinfo(arg->get.result, state->root, 
				*instruction_get_s(iptr, pos));
		case ITERATOR_END:
			return pos == instruction_num_s(iptr);
		case ITERATOR_FORWARD:
			state->pos = (void *)(pos + 1);
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(3);
			return get_varinfo(arg->subscript.result, state->root, 
				*instruction_get_s(iptr, arg->subscript.index));
		case ITERATOR_SETITEM:
			ITERATOR_SETITEM_CHECK(3);
			return set_s4(instruction_get_s(iptr, arg->setitem.index), 
				arg->setitem.value);
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
					return get_int(arg->get.result, instruction_opcode_ex(iptr));
				case F_NAME:
					return get_string(arg->get.result, icmd_table[iptr->opc].name);
				case F_NAME_EX:
					return get_string(arg->get.result, icmd_table[instruction_opcode_ex(iptr)].name);
				case F_S:
					return get_iter(arg->get.result, s_iter_func, state->root, iptr);	
				case F_DST:
					return get_varinfo(arg->get.result, state->root, 
						iptr->dst.varindex);
				case F_HAS_DST:
					if (
						(icmd_table[iptr->opc].dataflow == DF_INVOKE) ||
						(icmd_table[iptr->opc].dataflow == DF_BUILTIN)
					) {
						return get_bool(
							arg->get.result,
							instruction_call_site(iptr)->returntype.type != TYPE_VOID
						);
					}
					return get_bool(arg->get.result, icmd_table[iptr->opc].dataflow >= DF_DST_BASE);
				case F_CALL_RETURN_TYPE:
					return get_int(arg->get.result, instruction_call_site(iptr)->returntype.type);
				case F_CALL_ARGS:
					return get_iter(arg->get.result, call_args_iter_func, state->root, iptr);	
				case F_HAS_CALL_ARGS:
					return get_bool(arg->get.result,
						icmd_table[iptr->opc].dataflow == DF_INVOKE ||
						icmd_table[iptr->opc].dataflow == DF_BUILTIN ||
						icmd_table[iptr->opc].dataflow == DF_N_TO_1
					);
				case F_IS_UNRESOLVED:
					return get_bool(arg->get.result, iptr->flags.bits & INS_FLAG_UNRESOLVED);
				case F_IS_CLASS_CONSTANT:
					return get_bool(arg->get.result, iptr->flags.bits & INS_FLAG_CLASS);
				case F_KLASS:
					return get_obj(arg->get.result, classinfo_func, state->root, iptr->sx.val.c.cls);
				case F_CLASSREF:
					return get_obj(arg->get.result, constant_classref_func, state->root, iptr->sx.val.c.ref);
				case F_LOCAL_METHODINFO:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_none(arg->get.result);
					} else {	
						return get_obj(arg->get.result, methodinfo_func, 
							state->root, iptr->sx.s23.s3.fmiref->p.method);
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
						return get_obj(arg->get.result, fieldinfo_func, state->root, iptr->sx.s23.s3.fmiref->p.field);
					}
					break;
				case F_UNRESOLVED_FIELD:
					if (INSTRUCTION_IS_UNRESOLVED(iptr)) {
						return get_obj(arg->get.result, unresolved_field_func, state->root, iptr->sx.s23.s3.uf);
					} else {
						return get_none(arg->get.result);
					}
					break;
				case F_LINE:
					return get_int(arg->get.result, iptr->line);
				case F_PEI:
					return get_bool(arg->get.result, icmd_table[iptr->opc].flags & ICMDTABLE_PEI);
				case F_PEI_EX:
					return get_bool(arg->get.result, icmd_table[instruction_opcode_ex(iptr)].flags & ICMDTABLE_PEI);
				case F_DATA_FLOW:
					return get_int(arg->get.result, icmd_table[iptr->opc].dataflow);
				case F_DATA_FLOW_EX:
					return get_int(arg->get.result, icmd_table[instruction_opcode_ex(iptr)].dataflow);
				case F_CONTROL_FLOW:
					return get_int(arg->get.result, icmd_table[iptr->opc].controlflow);
				case F_CONTROL_FLOW_EX:
					return get_int(arg->get.result, icmd_table[instruction_opcode_ex(iptr)].controlflow);
				case F_SHOW:
					arg->get.is_method = 1;
					return 0;
			}
		case CLASS_SET_FIELD:
			switch (arg->set.field) {
				case F_DST:
					return set_s4(&(iptr->dst.varindex), arg->set.value);
				case F_OPCODE:
					return set_s4(&(iptr->opc), arg->set.value);
			}
		case CLASS_METHOD_CALL:
			switch (arg->method_call.method) {
				case F_SHOW:
					show_icmd(state->root->jd, iptr, 1, SHOW_CFG);
					return 0;
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
			return get_obj(arg->get.result, basicblock_func, state->root, *(basicblock **)state->pos);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->predecessorcount);
			return get_obj(arg->subscript.result, basicblock_func, state->root, 
				bptr->predecessors[arg->subscript.index]);
		case ITERATOR_END:
			return 
				(state->pos == (bptr->predecessors + bptr->predecessorcount)) ||
				(bptr->predecessorcount < 0);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock **)state->pos) + 1;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = bptr->predecessorcount;
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
			return get_obj(arg->get.result, basicblock_func, state->root, *(basicblock **)state->pos);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->successorcount);
			return get_obj(arg->subscript.result, basicblock_func, state->root, 
				bptr->successors[arg->subscript.index]);
		case ITERATOR_END:
			return 
				(state->pos == (bptr->successors + bptr->successorcount)) || 
				(bptr->successorcount < 0);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock **)state->pos) + 1;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = bptr->successorcount;
			return 0;
	}

	return  -1;
}

ITERATOR_FUNC(dom_successors_iter_func) {
	basicblock *bptr = (basicblock *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->domsuccessors;
			return 0;
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->root, *(basicblock **)state->pos);
		case ITERATOR_END:
			return (state->pos == (bptr->domsuccessors + bptr->domsuccessorcount));
		case ITERATOR_FORWARD:
			state->pos = ((basicblock **)state->pos) + 1;
			return 0;
		case ITERATOR_LENGTH:
			arg->length = bptr->domsuccessorcount;
			return 0;
	}

	return  -1;
}

ITERATOR_FUNC(dominance_frontier_iter_func) {
	basicblock *bptr = (basicblock *)state->data;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->domfrontier;
			return 0;
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->root, *(basicblock **)state->pos);
		case ITERATOR_END:
			return (state->pos == (bptr->domfrontier + bptr->domfrontiercount));
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
			return get_obj(arg->get.result, instruction_func, state->root, state->pos);
		case ITERATOR_FORWARD:
			state->pos = ((instruction *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (bptr->iinstr + bptr->icount);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->icount);
			return get_obj(arg->subscript.result, instruction_func, state->root, bptr->iinstr + arg->subscript.index);
		case ITERATOR_LENGTH:
			arg->length = bptr->icount;
			return 0;
	}
	return -1;
}

ITERATOR_FUNC(in_vars_iter_func) {
	basicblock *bptr = (basicblock *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->invars;
			return 0;
		case ITERATOR_GET:
			return get_varinfo(arg->get.result, state->root, *(s4 *)(state->pos));
		case ITERATOR_FORWARD:
			state->pos = ((s4 *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (bptr->invars + bptr->indepth);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->icount);
			return get_varinfo(arg->subscript.result, state->root, bptr->invars[arg->subscript.index]);
		case ITERATOR_LENGTH:
			arg->length = bptr->indepth;
			return 0;
	}
}

ITERATOR_FUNC(out_vars_iter_func) {
	basicblock *bptr = (basicblock *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = bptr->outvars;
			return 0;
		case ITERATOR_GET:
			return get_varinfo(arg->get.result, state->root, *(s4 *)(state->pos));
		case ITERATOR_FORWARD:
			state->pos = ((s4 *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (bptr->outvars + bptr->outdepth);
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(bptr->icount);
			return get_varinfo(arg->subscript.result, state->root, bptr->outvars[arg->subscript.index]);
		case ITERATOR_LENGTH:
			arg->length = bptr->outdepth;
			return 0;
	}
}

CLASS_FUNC(basicblock_func) {
	basicblock *bptr = (basicblock *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_INSTRUCTIONS:
					return get_iter(arg->get.result, instruction_iter_func, state->root, bptr);
				case F_NR:
					return get_int(arg->get.result, bptr->nr);
				case F_PREDECESSORS:
					return get_iter(arg->get.result, predecessors_iter_func, state->root, bptr);
				case F_SUCCESSORS:
					return get_iter(arg->get.result, successors_iter_func, state->root, bptr);
				case F_REACHED:
					return get_bool(arg->get.result, bptr->flags >= BBREACHED);
				case F_EXCEPTION_HANDLER:
					return get_bool(arg->get.result, bptr->type == BBTYPE_EXH);
				case F_IDOM:
					return get_obj(arg->get.result, basicblock_func, state->root, bptr->idom);
				case F_DOM_SUCCESSORS:
					return get_iter(arg->get.result, dom_successors_iter_func, state->root, bptr);
				case F_DOMINANCE_FRONTIER:
					return get_iter(arg->get.result, dominance_frontier_iter_func, state->root, bptr);
				case F_IN_VARS:
					return get_iter(arg->get.result, in_vars_iter_func, state->root, bptr);
				case F_OUT_VARS:
					return get_iter(arg->get.result, in_vars_iter_func, state->root, bptr);
				case F_SHOW:
					arg->get.is_method = 1;
					return 0;
			}
		case CLASS_STR:
			*arg->str.result = PyString_FromFormat("BB_%d", bptr->nr);
			return 0;
		case CLASS_METHOD_CALL:
			switch (arg->method_call.method) {	
				case F_SHOW:
					show_basicblock(state->root->jd, bptr, SHOW_CFG);
					return 0;
			}
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
			return get_obj(arg->get.result, basicblock_func, state->root, state->pos);
		case ITERATOR_FORWARD:
			state->pos = ((basicblock *)(state->pos))->next;
			return 0;
		case ITERATOR_END:
			return (state->pos == NULL);
		case ITERATOR_SUBSCRIPT:
			for (bb = jd->basicblocks; bb != NULL; bb = bb->next) {
				if (bb->nr == arg->subscript.index) {
					return get_obj(arg->subscript.result, basicblock_func, state->root, bb);
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

ITERATOR_FUNC(params_iter_func) {

	methodinfo *m = (methodinfo *)state->data;
	/* param counter */
	uint16_t p = (uintptr_t)(state->pos) & 0xFFFF;
	/* local counter */
	uint16_t l = ((uintptr_t)(state->pos) >> 16) & 0xFFFF;

	int varnum;

	switch (op) {
		case ITERATOR_INIT:
			state->pos = (void *)0;
			return 0;
		case ITERATOR_END:
			return p == m->parseddesc->paramcount;
		case ITERATOR_FORWARD:
			l += (IS_2_WORD_TYPE(m->parseddesc->paramtypes[p].type) ? 2 : 1);
			p += 1;
			state->pos = (void *)(uintptr_t)((l << 16) | p);
			return 0;
		case ITERATOR_GET:
			varnum = state->root->jd->local_map[(5 * l) + m->parseddesc->paramtypes[p].type];
			return get_varinfo(arg->get.result, state->root, varnum);
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
					return get_obj(arg->get.result, classinfo_func, state->root, m->clazz);
				case F_PARAM_TYPES:
					return get_iter(arg->get.result, param_types_iter_func, state->root, m);
				case F_PARAMS:
					if (m == state->root->jd->m) {
						return get_iter(arg->get.result, params_iter_func, state->root, m);
					} else {
						return get_none(arg->get.result);
					}
				case F_RETURN_TYPE:
					return get_int(arg->get.result, m->parseddesc->returntype.type);
				case F_SHOW:
					if (m == state->root->jd->m) {
						arg->get.is_method = 1;
						return 0;
					}
			}
		case CLASS_METHOD_CALL:
			switch (arg->method_call.method) {
				case F_SHOW:
					show_method(state->root->jd, SHOW_CFG);
					return 0;
			}
	}
	return -1;
}

static inline PyObject *varinfo_str(jitdata *jd, int index, varinfo *v) {
	char type = '?';
	char kind = '?';

	switch (v->type) {
		case TYPE_INT: type = 'i'; break;
		case TYPE_LNG: type = 'l'; break;
		case TYPE_FLT: type = 'f'; break;
		case TYPE_DBL: type = 'd'; break;
		case TYPE_ADR: type = 'a'; break;
		case TYPE_RET: type = 'r'; break;
		default:       type = '?';
	}

	if (index < jd->localcount) {
		kind = 'L';
	}
	else {
		if (v->flags & PREALLOC) {
			kind = 'A';
			if (v->flags & INOUT) {
				/* PREALLOC is used to avoid allocation of TYPE_RET */
				if (v->type == TYPE_RET)
					kind = 'i';
			}
		}
		else if (v->flags & INOUT)
			kind = 'I';
		else
			kind = 'T';
	}

	if (index == -1) {
		return PyString_FromString("UNUSED");
	} else {
		return PyString_FromFormat("%c%c%d", kind, type, index);
	}
}


CLASS_FUNC(varinfo_func) {
	jitdata *jd = state->root->jd;
	varinfo *var = (varinfo *)state->vp;
	int index = var - jd->var;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_TYPE:
					return get_int(arg->get.result, var->type);
				case F_IS_LOCAL:
					return get_bool(arg->get.result, index < jd->localcount);
				case F_IS_PREALLOCATED:
					return get_bool(
						arg->get.result, 
						(index >= jd->localcount) && (var->flags & PREALLOC)
					);
				case F_IS_INOUT:
					return get_bool(
						arg->get.result, 
						(index >= jd->localcount) && !(var->flags & PREALLOC) && (var->flags & INOUT)
					);
				case F_IS_TEMPORARY:
					return get_bool(
						arg->get.result, 
						(index >= jd->localcount) && !(var->flags & PREALLOC) && !(var->flags & INOUT)
					);
				case F_IS_SAVED:
					return get_bool(arg->get.result, var->flags & SAVEDVAR);
				case F_INDEX:
					return get_int(arg->get.result, index);
				case F_UNUSED:
					return get_bool(arg->get.result, index == UNUSED);
				case F_REGISTER_OFFSET:
					return get_int(arg->get.result, var->vv.regoff);
				case F_IS_IN_MEMORY:
					return get_bool(arg->get.result, var->flags & INMEMORY);
			}
		case CLASS_SET_FIELD:
			switch (arg->set.field) {
				case F_TYPE:
					return set_s4(&(var->type), arg->set.value);
				case F_IS_LOCAL:
					if (PyBool_Check(arg->set.value)) {
						if (arg->set.value == Py_True) {
							if (jd->localcount < (index + 1)) {
								jd->localcount = (index + 1);
							}
						} else {
							if (jd->localcount > (index)) {
								jd->localcount = index;
							}
						}
						return 0;
					}
					break;
				case F_IS_SAVED:
					if (PyBool_Check(arg->set.value)) {
						if (arg->set.value == Py_True) {
							var->flags |= SAVEDVAR;
						} else {
							var->flags &= ~SAVEDVAR;
						}
						return 0;
					}
					break;
				case F_IS_PREALLOCATED:
					if (arg->set.value == Py_True) {
						var->flags |= PREALLOC;
						return 0;
					}
					break;
				case F_IS_INOUT:
					if (arg->set.value == Py_True) {
						var->flags &= ~PREALLOC;
						var->flags |= INOUT;
						return 0;
					}
					break;
				case F_IS_TEMPORARY:
					if (arg->set.value == Py_True) {
						var->flags &= ~PREALLOC;
						var->flags &= ~INOUT;
						return 0;
					}
					break;
				case F_REGISTER_OFFSET:
					return set_s4(&(var->vv.regoff), arg->set.value);
				case F_IS_IN_MEMORY:
					return set_s4_flags(&(var->flags), INMEMORY, arg->set.value);
			}
		case CLASS_STR:
			*arg->str.result = varinfo_str(jd, index, var);
			return 0;

	}
	return -1;
}

int vars_grow(jitdata *jd, unsigned size) {
	int newcount;
	if (size > 16 * 1024) {
		return 0;
	}
	if (size >= jd->varcount) {
		newcount = 2 * jd->varcount;
		if (size > newcount) {
			newcount = size;
		}
		jd->var = DMREALLOC(jd->var, varinfo, jd->varcount, newcount);
		MZERO(jd->var + jd->varcount, varinfo, (newcount - jd->varcount));
		jd->varcount = newcount;
	}
	return 1;
}

ITERATOR_FUNC(vars_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	void *vp;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = jd->var;
			return 0;
		case ITERATOR_FORWARD:
			state->pos = ((varinfo *)state->pos) + 1;
			return 0;
		case ITERATOR_END:
			return state->pos == (jd->var + jd->vartop);
		case ITERATOR_GET:
			return get_obj(arg->get.result, varinfo_func, state->root, state->pos);
		case ITERATOR_LENGTH:
			arg->length = jd->vartop;
			return 0;
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(jd->vartop);
			return get_obj(arg->subscript.result, varinfo_func, 
				state->root, jd->var + arg->subscript.index);
		case ITERATOR_SETITEM:
			ITERATOR_SETITEM_CHECK(jd->vartop);
			vp = get_vp(arg->setitem.value, varinfo_func);
			if (vp) {
				jd->var[arg->setitem.index] = *(varinfo *)vp;
				return 0;
			}

	}
	return -1;
}

ITERATOR_FUNC(map_2_iter_func) {
	int *arr = (int *)state->data;
	switch (op) {
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(5);
			return get_int(arg->subscript.result, arr[arg->subscript.index]);
		case ITERATOR_LENGTH:
			arg->length = 5;
			return 0;
		case ITERATOR_SETITEM:
			ITERATOR_SETITEM_CHECK(5);
			return set_s4(arr + arg->subscript.index, arg->setitem.value);
	}
	return -1;
}

ITERATOR_FUNC(local_map_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	switch (op) {
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(jd->maxlocals);
			return get_iter(arg->subscript.result, map_2_iter_func, state->root,
				jd->local_map + (5 * arg->subscript.index));
	}
	return -1;
}

ITERATOR_FUNC(interface_map_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	switch (op) {
		case ITERATOR_SUBSCRIPT:
			ITERATOR_SUBSCRIPT_CHECK(jd->maxinterfaces);
			return get_iter(arg->subscript.result, map_2_iter_func, state->root,
				jd->interface_map + (5 * arg->subscript.index));
	}
	return -1;
}

ITERATOR_FUNC(exception_entry_basic_blocks_iter_func) {
	exception_entry *ee = (exception_entry *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = ee->start;
			return 0;
		case ITERATOR_FORWARD:
			state->pos = ((basicblock *)state->pos)->next;
			return 0;
		case ITERATOR_END:
			return state->pos == ee->end;
		case ITERATOR_GET:
			return get_obj(arg->get.result, basicblock_func, state->root, state->pos);
	}
	return -1;
}

CLASS_FUNC(exception_entry_func) {
	exception_entry *ee = (exception_entry *)state->vp;
	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_START:
					return get_obj(arg->get.result, basicblock_func, state->root, ee->start);
				case F_END:
					return get_obj(arg->get.result, basicblock_func, state->root, ee->end);
				case F_HANDLER:
					return get_obj(arg->get.result, basicblock_func, state->root, ee->handler);
				case F_BASIC_BLOCKS:
					return get_iter(arg->get.result, exception_entry_basic_blocks_iter_func, state->root, ee);
			}
			break;
	}
	return -1;
}

ITERATOR_FUNC(exception_table_iter_func) {
	jitdata *jd = (jitdata *)state->data;
	switch (op) {
		case ITERATOR_INIT:
			state->pos = jd->exceptiontable;
			return 0;
		case ITERATOR_FORWARD:
			state->pos = ((exception_entry *)state->pos)->down;
			return 0;
		case ITERATOR_END:
			return state->pos == NULL;
		case ITERATOR_GET:
			return get_obj(arg->get.result, exception_entry_func, state->root, state->pos);
	}
	return -1;
}

CLASS_FUNC(jd_func) {
	jitdata *jd = (jitdata *)state->vp;

	switch (op) {
		case CLASS_GET_FIELD:
			switch (arg->get.field) {
				case F_BASIC_BLOCKS:
					return get_iter(arg->get.result, basicblocks_iter_func, state->root, jd);
				case F_METHOD:
					return get_obj(arg->get.result, methodinfo_func, state->root, jd->m);
				case F_VARS:
					return get_iter(arg->get.result, vars_iter_func, state->root, jd);
				case F_LOCAL_MAP:
					return get_iter(arg->get.result, local_map_iter_func, state->root, jd);
				case F_INTERFACE_MAP:
					return get_iter(arg->get.result, interface_map_iter_func, state->root, jd);
				case F_EXCEPTION_TABLE:
					return get_iter(arg->get.result, exception_table_iter_func, state->root, jd);
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

	/* data flow */

	c(DF_0_TO_0);
	c(DF_1_TO_0);
	c(DF_2_TO_0);
	c(DF_3_TO_0);
	c(DF_DST_BASE);
	c(DF_0_TO_1);
	c(DF_1_TO_1);
	c(DF_2_TO_1);
	c(DF_3_TO_1);
	c(DF_N_TO_1);
	c(DF_INVOKE);
	c(DF_BUILTIN);
	c(DF_COPY);
	c(DF_MOVE);
	c(DF_DUP);
	c(DF_DUP_X1);
	c(DF_DUP_X2);
	c(DF_DUP2);
	c(DF_DUP2_X1);
	c(DF_DUP2_X2);
	c(DF_SWAP);
	c(DF_LOAD);
	c(DF_STORE);
	c(DF_IINC);
	c(DF_POP);
	c(DF_POP2);

	/* control flow */

	c(CF_NORMAL);
	c(CF_IF);
	c(CF_END_BASE);
	c(CF_END);
	c(CF_GOTO);
	c(CF_TABLE);
	c(CF_LOOKUP);
	c(CF_JSR);
	c(CF_RET);

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
	if (PyType_Ready(&method_type) < 0) return;

	m = Py_InitModule3("cacao", NULL, NULL);
	if (m != NULL) {
		constants(m);
	}

#if defined(ENABLE_THREADS)
	python_global_lock = NEW(java_object_t);
	LOCK_INIT_OBJECT_LOCK(python_global_lock);
#endif

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
	PyObject *objcache = NULL;
	int success = 0;
	root_state root;

	LOCK_MONITOR_ENTER(python_global_lock);

	pymodname = PyString_FromString(module);
	pymod = PyImport_Import(pymodname);

	root.jd = jd;
	root.object_cache = objcache = PyDict_New();

	if (pymod != NULL) {
		pydict = PyModule_GetDict(pymod);
		pyfunc = PyDict_GetItemString(pydict, function);
		if (pyfunc != NULL && PyCallable_Check(pyfunc)) {
			pyargs = PyTuple_New(1);

			if (get_obj(&pyarg, jd_func, &root, jd) != -1) {
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
	Py_XDECREF(objcache);

	LOCK_MONITOR_EXIT(python_global_lock);

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
