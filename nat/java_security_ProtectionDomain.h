/* This file is machine generated, don't edit it !*/

#ifndef _JAVA_SECURITY_PROTECTIONDOMAIN_H
#define _JAVA_SECURITY_PROTECTIONDOMAIN_H

/* Structure information for class: java/security/ProtectionDomain */

typedef struct java_security_ProtectionDomain {
   java_objectheader header;
   struct java_security_CodeSource* code_source;
   struct java_security_PermissionCollection* perms;
   struct java_lang_ClassLoader* classloader;
   java_objectarray* principals;
   s4 staticBinding;
} java_security_ProtectionDomain;

#endif

