#ifndef _ANALYZE_HPP
#define _ANALYZE_HPP

#include "loop.hpp"

/**
 * For all basicblocks B except the root, finds all variable assignments
 * occuring between B and idom(B) and stores the variable names in B.
 */
void findVariableAssignments(jitdata*);

#endif
