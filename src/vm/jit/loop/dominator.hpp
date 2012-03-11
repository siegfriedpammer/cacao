#ifndef _DOMINATOR_HPP
#define _DOMINATOR_HPP

#include "loop.hpp"

// The number of the basicblock jd->ld->root.
#define ROOT_NUMBER (-1)

void createRoot(jitdata*);
void calculateDominators(jitdata*);
void buildDominatorTree(jitdata*);

#endif
