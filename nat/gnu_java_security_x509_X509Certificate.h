/* This file is machine generated, don't edit it !*/

/* Structure information for class: gnu/java/security/x509/X509Certificate */

typedef struct gnu_java_security_x509_X509Certificate {
   java_objectheader header;
   struct java_lang_String* type;
   java_bytearray* encoded;
   java_bytearray* tbsCertBytes;
   s4 version;
   struct java_math_BigInteger* serialNo;
   struct gnu_java_security_OID* algId;
   java_bytearray* algVal;
   struct javax_security_auth_x500_X500Principal* issuer;
   struct java_util_Date* notBefore;
   struct java_util_Date* notAfter;
   struct javax_security_auth_x500_X500Principal* subject;
   struct java_security_PublicKey* subjectKey;
   struct gnu_java_security_der_BitString* issuerUniqueId;
   struct gnu_java_security_der_BitString* subjectUniqueId;
   struct java_util_HashMap* extensions;
   struct java_util_HashSet* critOids;
   struct java_util_HashSet* nonCritOids;
   struct gnu_java_security_der_BitString* keyUsage;
   s4 basicConstraints;
   struct gnu_java_security_OID* sigAlgId;
   java_bytearray* sigAlgVal;
   java_bytearray* signature;
} gnu_java_security_x509_X509Certificate;

