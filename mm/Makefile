# cacao/mm/Makefile
# $Id: Makefile 52 1998-11-09 23:06:35Z phil $

AR = ar
COMBINE = $(AR) rcs mm.o

mm.o: actualtarget Makefile

actualtarget: old

clean:
	rm -rf *.o *~

old: heap.old.o
	rm -f mm.o; $(COMBINE) heap.old.o

heap.old.o: heap.old.c ../tables.h ../global.h







# Targets below this point are experimental and will usually
# not work reliably








new: heap.allocator.o allocator.o 
	rm -f mm.o; $(COMBINE) heap.allocator.o allocator.o

newer: heap.bitmap.o allocator.o bitmap.o
	rm -f mm.o; $(COMBINE) heap.bitmap.o allocator.o bitmap.o

gc: heap.o allocator.o bitmap.o
	rm -f mm.o; $(COMBINE) heap.o allocator.o bitmap.o

gc2: heap2.o allocator.o bitmap2.o
	rm -f mm.o; $(COMBINE) heap2.o allocator.o bitmap2.o

heap2.o: heap2.c allocator.h mm.h bitmap2.h

heap.o: heap.c allocator.h bitmap2.h

bitmap2.o: bitmap2.c bitmap2.h allocator.h mm.h

heap.allocator.o: heap.c allocator.h

heap.bitmap.o: heap.bitmap.c bitmap.h allocator.h

bitmap.o: bitmap.c bitmap.h

allocator.o: allocator.h allocator.c 

gc.o:  heap.h gc.c 


#
# These are local overrides for various environment variables in Emacs.
# Please do not remove this and leave it at the end of the file, where
# Emacs will automagically detect them.
# ---------------------------------------------------------------------
# Local variables:
# mode: Makefile
# indent-tabs-mode: t
# tab-width: 4
# End:
#

