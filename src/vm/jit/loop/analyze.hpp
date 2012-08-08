#ifndef _ANALYZE_HPP
#define _ANALYZE_HPP

#include "loop.hpp"

void analyzeLoops(jitdata* jd);
void findLeaves(jitdata* jd);

/**
 * Removes all array bound checks that are fully redundant.
 */
void removeFullyRedundantChecks(jitdata*);

#endif
