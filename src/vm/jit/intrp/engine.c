#include <assert.h>

/*  #define VM_DEBUG */
#define USE_spTOS

#include "arch.h"
#include "vm/jit/intrp/intrp.h"

#include "cacao/cacao.h"
#include "vm/builtin.h"
#include "vm/exceptions.h"
#include "vm/loader.h"
#include "vm/options.h"
#include "vm/jit/codegen.inc.h"
#include "vm/jit/methodheader.h"
#include "vm/jit/patcher.h"

#define FFCALL 0

#if FFCALL
# include "ffcall/avcall/avcall.h"
#else
# include "libffi/include/ffi.h"
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
# ifndef USE_MD_THREAD_STUFF
#  include "machine-instr.h"
# else
#  include "threads/native/generic-primitives.h"
# endif
#endif

#if !defined(STORE_ORDER_BARRIER) && !defined(USE_THREADS)
#define STORE_ORDER_BARRIER() /* nothing */
#endif


/* threading macros */
#  define NEXT_P0
#  define IP		(ip)
#  define SET_IP(p)	({ip=(p); NEXT_P0;})
#  define NEXT_INST	(*IP)
#  define INC_IP(const_inc)	({ ip+=(const_inc);})
#  define DEF_CA
#  define NEXT_P1	
#  define NEXT_P2	({goto **(ip++);})
#  define EXEC(XT)	({goto *(XT);})

#define NEXT ({DEF_CA NEXT_P1; NEXT_P2;})
#define IPTOS NEXT_INST

#if defined(USE_spTOS)
#define IF_spTOS(x) x
#else
#define IF_spTOS(x)
#define spTOS (sp[0])
#endif

/* conversion on fetch */

#ifdef VM_PROFILING
#define SUPER_END  vm_count_block(IP)
#else
#define SUPER_END
#define vm_uncount_block(_ip)	/* nothing */
#endif

#if defined(USE_THREADS) && defined(NATIVE_THREADS)

#define global_sp    (*(Cell **)&(THREADINFO->_global_sp))

#else /* defined(USE_THREADS) && defined(NATIVE_THREADS) */

#define MAX_STACK_SIZE 128*1024
static char stack[MAX_STACK_SIZE];

static Cell *_global_sp = (Cell *)(stack+MAX_STACK_SIZE);
#define global_sp    _global_sp

#endif /* defined(USE_THREADS) && defined(NATIVE_THREADS) */

#define CLEAR_global_sp (global_sp=NULL)


#define THROW0       goto throw
#define THROW(_ball) \
    { \
        global_sp = sp; \
        *exceptionptr = (stacktrace_inline_##_ball(NULL, (u1 *) fp, (functionptr) IP, (functionptr) IP)); \
        CLEAR_global_sp; \
        THROW0; \
    }

#define CHECK_NULL_PTR(ptr) \
    { \
        if ((ptr) == NULL) { \
            THROW(nullpointerexception); \
	    } \
	}

#define CHECK_OUT_OF_BOUNDS(_array, _idx)              \
        {                                            \
          if (length_array(_array) <= (u4) (_idx)) { \
            global_sp = sp; \
            *exceptionptr = stacktrace_inline_arrayindexoutofboundsexception(NULL, (u1 *) fp, (functionptr) IP, (functionptr) IP, _idx); \
		    CLEAR_global_sp; \
            THROW0; \
          } \
	}

#define CHECK_ZERO_DIVISOR(_divisor) \
  { if (_divisor == 0) \
      THROW(arithmeticexception); \
  } 

#define access_local_int(_offset) \
        ( *(Cell*)(((u1 *)fp) + (_offset)) )

#define access_local_ref(_offset) \
        ( *(void **)(((u1 *)fp) + (_offset)) )

#define access_local_cell(_offset) \
        ( *(Cell *)(((u1 *)fp) + (_offset)) )

#if 0
/* !! alignment bug */
#define access_local_long(_offset) \
        ( *(s8 *)(((u1 *)fp) + (_offset)) )
#endif

#define length_array(array)                          \
        ( ((java_arrayheader*)(array))->size )

#define access_array_int(array, index)               \
        ((((java_intarray*)(array))->data)[index])

#define access_array_long(array, index)               \
        ((((java_longarray*)(array))->data)[index])

#define access_array_char(array, index)               \
        ((((java_chararray*)(array))->data)[index])

#define access_array_short(array, index)               \
        ((((java_shortarray*)(array))->data)[index])

#define access_array_byte(array, index)               \
        ((((java_bytearray*)(array))->data)[index])

#define access_array_addr(array, index)               \
        ((((java_objectarray*)(array))->data)[index])

#define MAXLOCALS(stub) (((Cell *)stub)[1])

#if 0
#define CLEARSTACK(_start, _end) \
	 do {Cell *__start=(_start); MSET(__start,0,u1,(_end)-__start); } while (0)
#else
#define CLEARSTACK(_start, _end)
#endif


#if !FFCALL
ffi_type *cacaotype2ffitype(s4 cacaotype)
{
	switch (cacaotype) {
	case TYPE_INT:
		return &ffi_type_uint;
	case TYPE_LNG:
		return &ffi_type_sint64;
	case TYPE_FLT:
		return &ffi_type_float;
	case TYPE_DBL:
		return &ffi_type_double;
	case TYPE_ADR:
		return &ffi_type_pointer;
	case TYPE_VOID:
		return &ffi_type_void;
	default:
		assert(false);
	}
}
#endif


/* call jni function */
static Cell *nativecall(functionptr f, methodinfo *m, Cell *sp, Inst *ra, Cell *fp, u1 *addrcif)
{
#if FFCALL
	av_alist alist;
	methoddesc *md;
	Cell *p;
	Cell *endsp;
	s4 i;

	struct {
		stackframeinfo sfi;
		localref_table lrt;
	} s;

	md = m->parseddesc;

	switch (md->returntype.type) {
	case TYPE_INT:
		endsp = sp - 1 + md->paramslots;
		av_start_int(alist, f, endsp);
		break;
	case TYPE_LNG:
		endsp = sp - 2 + md->paramslots;
		av_start_longlong(alist, f, endsp);
		break;
	case TYPE_FLT:
		endsp = sp - 1 + md->paramslots;
		av_start_float(alist, f, endsp);
		break;
	case TYPE_DBL:
		endsp = sp - 2 + md->paramslots;
		av_start_double(alist, f, endsp);
		break;
	case TYPE_ADR:
		endsp = sp - 1 + md->paramslots;
		av_start_ptr(alist, f, void *, endsp);
		break;
	case TYPE_VOID:
		endsp = sp + md->paramslots;
		av_start_void(alist, f);
		break;
	default:
		assert(false);
	}

	av_ptr(alist, JNIEnv *, &env);

	if (m->flags & ACC_STATIC)
		av_ptr(alist, classinfo *, m->class);

	for (i = 0, p = sp + md->paramslots; i < md->paramcount; i++) {
		switch (md->paramtypes[i].type) {
		case TYPE_INT:
			p -= 1;
			av_int(alist, *p);
			break;
		case TYPE_LNG:
			p -= 2;
			av_longlong(alist, *(s8 *)p);
			break;
		case TYPE_FLT:
			p -= 1;
			av_float(alist, *(float *) p);
			break;
		case TYPE_DBL:
			p -= 2;
			av_double(alist, *(double *) p);
			break;
		case TYPE_ADR:
			p -= 1;
			av_ptr(alist, void *, *(void **) p);
			break;
		default:
			assert(false);
		}
	}

	global_sp = sp;

	/* create stackframe info structure */

	codegen_start_native_call(&s, (u1 *) m->entrypoint,
							  (u1 *) fp,
							  (functionptr) ra);

	av_call(alist);

	codegen_finish_native_call(&s);

	CLEAR_global_sp;

	return endsp;
#else
	methoddesc  *md = m->parseddesc; 
	ffi_cif     *pcif;
	void        *values[md->paramcount + 2];
	void       **pvalues = values;
	Cell        *p;
	Cell        *endsp;
	s4           i;
	JNIEnv      *penv;

	struct {
		stackframeinfo sfi;
		localref_table lrt;
	} s;

	pcif = (ffi_cif *) addrcif;

	/* pass env pointer */

	penv = (JNIEnv *) &env;
	*pvalues++ = &penv;

	/* for static methods, pass class pointer */

	if (m->flags & ACC_STATIC) {
		*pvalues++ = &m->class;
	}

	/* pass parameter to native function */

	for (i = 0, p = sp + md->paramslots; i < md->paramcount; i++) {
		if (IS_2_WORD_TYPE(md->paramtypes[i].type))
			p -= 2;
		else
			p--;

		*pvalues++ = p;
	}

	/* calculate position of return value */

	if (md->returntype.type == TYPE_VOID)
		endsp = sp + md->paramslots;
	else
		endsp = sp - (IS_2_WORD_TYPE(md->returntype.type) ? 2 : 1) + md->paramslots;

	global_sp = sp;

	/* create stackframe info structure */

	codegen_start_native_call((u1 *) (((ptrint ) &s) + sizeof(s)),
							  (u1 *) (ptrint) m->entrypoint,
							  (u1 *) fp, (u1 *) ra);

	ffi_call(pcif, FFI_FN(f), endsp, values);

	codegen_finish_native_call((u1 *) (((ptrint) &s) + sizeof(s)));

	CLEAR_global_sp;

	return endsp;
#endif
}


Inst *builtin_throw(Inst *ip, java_objectheader *o, Cell *fp, Cell **new_spp, Cell **new_fpp)
{
	classinfo      *c;
	s4              framesize;
	exceptionentry *ex;
	s4              exceptiontablelength;
	s4              i;

  /* for a description of the stack see IRETURN in java.vmg */
  for (; fp!=NULL;) {
	  functionptr f = codegen_findmethod((functionptr) (ip-1));

	  /* get methodinfo pointer from method header */
	  methodinfo *m = *(methodinfo **) (((u1 *) f) + MethodPointer);

	  framesize = (*((s4 *) (((u1 *) f) + FrameSize)));
	  ex = (exceptionentry *) (((u1 *) f) + ExTableStart);
	  exceptiontablelength = *((s4 *) (((u1 *) f) + ExTableSize));

	  builtin_trace_exception(o, m, ip, 1);

	  for (i = 0; i < exceptiontablelength; i++) {
		  ex--;
		  c = ex->catchtype;

		  if (c != NULL) {
			  if (!c->loaded)
				  /* XXX fix me! */
				  if (!load_class_bootstrap(c))
					  assert(0);

			  if (!c->linked)
				  if (!link_class(c))
					  assert(0);
		  }

		  if (ip-1 >= (Inst *) ex->startpc && ip-1 < (Inst *) ex->endpc &&
			  (c == NULL || builtin_instanceof(o, c))) {
			  *new_spp = (Cell *)(((u1 *)fp) - framesize - SIZEOF_VOID_P);
			  *new_fpp = fp;
			  return (Inst *) (ex->handlerpc);
		  }
	  }

	  ip = (Inst *)access_local_cell(-framesize - SIZEOF_VOID_P);
	  fp = (Cell *)access_local_cell(-framesize);
  }

  return NULL; 
}


FILE *vm_out = NULL;

#ifdef VM_DEBUG
#define NAME(_x) if (vm_debug) {fprintf(vm_out, "%lx: %-20s, ", (long)(ip-1), _x); fprintf(vm_out,"fp=%p, sp=%p", fp, sp);}
#else
#define NAME(_x)
#endif

#define LABEL(_inst) I_##_inst:
#define INST_ADDR(_inst) (&&I_##_inst)
#define LABEL2(_inst)


java_objectheader *
engine(Inst *ip0, Cell * sp, Cell * fp)
{
  Inst * ip;
  IF_spTOS(Cell   spTOS);
  static Inst   labels[] = {
#include "java-labels.i"
  };

  if (vm_debug)
      fprintf(vm_out,"entering engine(%p,%p,%p)\n",ip0,sp,fp);
  if (ip0 == NULL) {
    vm_prim = labels;
    return NULL;
  }

  /* I don't have a clue where these things come from,
     but I've put them in macros.h for the moment */
  IF_spTOS(spTOS = sp[0]);

  SET_IP(ip0);
  NEXT;

#include "java-vm.i"
#undef NAME
}


/* true on success, false on exception */
static bool asm_calljavafunction_intern(methodinfo *m,
		   void *arg1, void *arg2, void *arg3, void *arg4)
{
  java_objectheader *retval;
  Cell *sp = global_sp;
  methoddesc *md;
  functionptr entrypoint;

  md = m->parseddesc;

  CLEAR_global_sp;
  assert(sp != NULL);

  /* XXX ugly hack: thread's run() needs 5 arguments */
  assert(md->paramcount < 6);

  if (md->paramcount > 0)
    *--sp=(Cell)arg1;
  if (md->paramcount > 1)
    *--sp=(Cell)arg2;
  if (md->paramcount > 2)
    *--sp=(Cell)arg3;
  if (md->paramcount > 3)
    *--sp=(Cell)arg4;
  if (md->paramcount > 4)
    *--sp=(Cell) 0;

  entrypoint = createcalljavafunction(m);

  retval = engine((Inst *) entrypoint, sp, NULL);

  /* XXX remove the method from the method table */

  if (retval != NULL) {
	  (void)builtin_throw_exception(retval);
	  return false;
  }
  else 
	  return true;
}

s4 asm_calljavafunction_int(methodinfo *m,
		   void *arg1, void *arg2, void *arg3, void *arg4)
{
	assert(m->parseddesc->returntype.type == TYPE_INT);
	if (asm_calljavafunction_intern(m, arg1, arg2, arg3, arg4))
		return (s4)(*global_sp++);
	else
		return 0;
}

java_objectheader *asm_calljavafunction(methodinfo *m,
		   void *arg1, void *arg2, void *arg3, void *arg4)
{
	if (asm_calljavafunction_intern(m, arg1, arg2, arg3, arg4)) {
		if (m->parseddesc->returntype.type == TYPE_ADR)
			return (java_objectheader *)(*global_sp++);
		else {
			assert(m->parseddesc->returntype.type == TYPE_VOID);
			return NULL;
		}
	} else
		return NULL;
}

/* true on success, false on exception */
static bool jni_invoke_java_intern(methodinfo *m, u4 count, u4 size,
                                          jni_callblock *callblock)
{
	java_objectheader *retval;
	Cell *sp = global_sp;
	s4 i;
	functionptr entrypoint;

	CLEAR_global_sp;
	assert(sp != NULL);

	for (i = 0; i < count; i++) {
		switch (callblock[i].itemtype) {
		case TYPE_INT:
		case TYPE_FLT:
		case TYPE_ADR:
			*(--sp) = callblock[i].item;
			break;
		case TYPE_LNG:
		case TYPE_DBL:
			sp -= 2;
			*((u8 *) sp) = callblock[i].item;
			break;
		}
	}

	entrypoint = createcalljavafunction(m);

	retval = engine((Inst *) entrypoint, sp, NULL);

	/* XXX remove the method from the method table */

	if (retval != NULL) {
		(void)builtin_throw_exception(retval);
		return false;
	}
	else
		return true;
}

java_objectheader *asm_calljavafunction2(methodinfo *m, u4 count, u4 size,
                                         jni_callblock *callblock)
{
  java_objectheader *retval = NULL;
  if (jni_invoke_java_intern(m, count, size, callblock)) {
	  if (m->parseddesc->returntype.type == TYPE_ADR)
		  retval = (java_objectheader *)*global_sp++;
	  else
		  assert(m->parseddesc->returntype.type == TYPE_VOID);
	  return retval;
  } else
	  return NULL;
}

s4 asm_calljavafunction2int(methodinfo *m, u4 count, u4 size,
                            jni_callblock *callblock)
{
  s4 retval=0;

  if (jni_invoke_java_intern(m, count, size, callblock)) {
	  if (m->parseddesc->returntype.type == TYPE_INT)
		  retval = *global_sp++;
	  else
		  assert(m->parseddesc->returntype.type == TYPE_VOID);
	  return retval;
  } else
	  return 0;
}

s8 asm_calljavafunction2long(methodinfo *m, u4 count, u4 size,
                             jni_callblock *callblock)
{
  s8 retval;
  assert(m->parseddesc->returntype.type == TYPE_LNG);
  if (jni_invoke_java_intern(m, count, size, callblock)) {
	  retval = *(s8 *)global_sp;
	  global_sp += 2;
	  return retval;
  } else
	  return 0;
}

float asm_calljavafunction2float(methodinfo *m, u4 count, u4 size,
                                         jni_callblock *callblock)
{
  float retval;
  assert(m->parseddesc->returntype.type == TYPE_FLT);
  if (jni_invoke_java_intern(m, count, size, callblock)) {
	  retval = *(float *)global_sp;
	  global_sp += 1;
	  return retval;
  } else
	  return 0.0;
}

double asm_calljavafunction2double(methodinfo *m, u4 count, u4 size,
                                   jni_callblock *callblock)
{
  double retval;
  assert(m->parseddesc->returntype.type == TYPE_DBL);
  if (jni_invoke_java_intern(m, count, size, callblock)) {
	  retval = *(double *)global_sp;
	  global_sp += 2;
	  return retval;
  } else
	  return 0.0;
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
 */
