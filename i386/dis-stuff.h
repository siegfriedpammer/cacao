#ifndef DIS_STUFF_H
#define DIS_STUFF_H

#include "types.h"
#include "ansidecl.h"

#define _(String) (String)

#if 0

#define _bfd_int64_low(x) ((unsigned long) (((x) & 0xffffffff)))
#define _bfd_int64_high(x) ((unsigned long) (((x) >> 32) & 0xffffffff))
#define fprintf_vma(s,x) \
  fprintf ((s), "%08lx%08lx", _bfd_int64_high (x), _bfd_int64_low (x))
#define sprintf_vma(s,x) \
  sprintf ((s), "%08lx%08lx", _bfd_int64_high (x), _bfd_int64_low (x))

typedef void *bfd;
typedef u4 bfd_vma;
typedef s4 bfd_signed_vma;
typedef u1 bfd_byte;

/* from bfd/bfd-in2.h */
#define bfd_mach_i386_i386 0
#define bfd_mach_i386_i8086 1
#define bfd_mach_i386_i386_intel_syntax 2
#define bfd_mach_x86_64 3
#define bfd_mach_x86_64_intel_syntax 4

/* from /usr/include/bfd.h */
enum bfd_architecture
{
  bfd_arch_unknown,   /* File arch not known.  */
  bfd_arch_last
  };

enum bfd_flavour
{
  bfd_target_unknown_flavour,
  bfd_target_aout_flavour,
  bfd_target_coff_flavour,
  bfd_target_ecoff_flavour,
  bfd_target_xcoff_flavour,
  bfd_target_elf_flavour,
  bfd_target_ieee_flavour,
  bfd_target_nlm_flavour,
  bfd_target_oasys_flavour,
  bfd_target_tekhex_flavour,
  bfd_target_srec_flavour,
  bfd_target_ihex_flavour,
  bfd_target_som_flavour,
  bfd_target_os9k_flavour,
  bfd_target_versados_flavour,
  bfd_target_msdos_flavour,
  bfd_target_ovax_flavour,
  bfd_target_evax_flavour,
  bfd_target_mmo_flavour,
  bfd_target_mach_o_flavour,
  bfd_target_pef_flavour,
  bfd_target_pef_xlib_flavour,
  bfd_target_sym_flavour
};

enum bfd_endian { BFD_ENDIAN_BIG, BFD_ENDIAN_LITTLE, BFD_ENDIAN_UNKNOWN };

#endif

#endif
