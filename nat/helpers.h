#ifndef _helpers_h_
#define _helpers_h_

utf *create_methodsig(java_objectarray* types, char *retType);
classinfo *get_type(char **utf_ptr,char *desc_end, bool skip);
java_objectarray* get_parametertypes(methodinfo *m);
classinfo *get_returntype(methodinfo *m); 

#endif
