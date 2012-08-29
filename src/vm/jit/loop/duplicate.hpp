#ifndef _DUPLICATE_HPP
#define _DUPLICATE_HPP

#include "loop.hpp"

bool findFreeVariable(jitdata* jd);
void removePartiallyRedundantChecks(jitdata* jd);
void groupArrayBoundsChecks(jitdata* jd);

#endif
