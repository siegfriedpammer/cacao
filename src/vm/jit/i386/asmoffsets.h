#ifndef __ASMOFFSETS_H_
#define __ASMOFFSETS_H_

/* data segment offsets */

#define MethodPointer           -4
#define FrameSize               -8
#define IsSync                  -12
#define IsLeaf                  -16
#define IntSave                 -20
#define FltSave                 -24
#define LineNumberTableSize     -28
#define LineNumberTableStart    -32
#define ExTableSize             -36
#define ExTableStart            -36

#define ExEntrySize     -16
#define ExStartPC       -4
#define ExEndPC         -8
#define ExHandlerPC     -12
#define ExCatchType     -16


#define LineEntrySize   -8
#define LinePC          0
#define LineLine        -4

#endif
