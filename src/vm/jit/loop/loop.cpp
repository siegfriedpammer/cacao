#include "loop.hpp"
#include "dominator.hpp"
#include "toolbox/logging.hpp"

void removeArrayBoundChecks(jitdata* jd)
{
	log_message_method("calculateDominators: ", jd->m);
	createRoot(jd);
	calculateDominators(jd);
	buildDominatorTree(jd);
	printBasicBlocks(jd);
}


