/*------------- RTAprint flags ------------------------------------------------------------------*/
int pCallgraph  = 0;    /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call                                     */
                        /* 3- print after each method RT parse                                   */
int pClassHeir  = 1;    /* 0 - dont print 1 - print at end from main                             */
                        /* 2 - print at end of RT parse call  3-print after each method RT parse */
int pClassHeirStatsOnly = 2;  /* usually 2 Print only the statistical summary info for class heirarchy     */

int pOpcodes    = 0;    /* 0 - don't print 1- print in parse RT 2- print in parse                */
                        /* 3 - print in both                                                     */
int pWhenMarked = 0;    /* 0 - don't print 1 - print when added to callgraph + when native parsed*/
                        /* 2 - print when marked+methods called                                  */
                        /* 3 - print when class/method looked at                                 */
int pStats = 0;         /* 0 - don't print; 1= analysis only; 2= whole unanalysed class heirarchy*/

/*-----------------------------------------------------------------------------------------------*/

bool useXTAcallgraph = false;
bool XTAOPTbypass = false;
bool XTAOPTbypass2 = false;   /* for now  invokeinterface     */
bool XTAOPTbypass3 = false;   /* print XTA classsets in stats */
int  XTAdebug = 0;
int  XTAfld = 0;


