#ifndef _ASMOFFSETS_H_
#define _ASMOFFSETS_H_

#define MethodPointer   -8
#define FrameSize       -12
#define IsSync          -16
#define IsLeaf          -20
#define IntSave         -24
#define FltSave         -28
#define ExTableSize     -32
#define ExTableStart    -32

#define ExEntrySize     -32
#define ExStartPC       -8
#define ExEndPC         -16
#define ExHandlerPC     -24
#define ExCatchType     -32

#endif
