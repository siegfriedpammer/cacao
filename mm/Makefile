# cacao/mm/Makefile
# $Id: Makefile 61 1998-11-11 19:40:23Z phil $

AR = ar
COMBINE = $(AR) rcs mm.o

mm.o: actualtarget Makefile

clean:
	rm -rf *.o *~

# choose gc1 for the old collector, gc2 for the new one
actualtarget: gc2




gc1: heap.old.o
	rm -f mm.o; $(COMBINE) heap.old.o

heap.old.o: heap.old.c ../tables.h ../global.h


# Targets below this point are experimental and may not work reliably


gc2: heap2.o allocator.o bitmap2.o
	rm -f mm.o; $(COMBINE) heap2.o allocator.o bitmap2.o

heap2.o: heap2.c allocator.h mm.h bitmap2.h

bitmap2.o: bitmap2.c bitmap2.h allocator.h mm.h

allocator.o: allocator.h allocator.c 


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

