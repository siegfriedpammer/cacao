/***************************** nat/util.c **************************************

	Copyright (c) 1997 A. Krall, R. Grafl, M. Gschwind, M. Probst

	See file COPYRIGHT for information on usage and disclaimer of warranties

	Contains the native functions for class java.util.

	Authors: Reinhard Grafl      EMAIL: cacao@complang.tuwien.ac.at

	Last Change: 1996/11/14

*******************************************************************************/

#include <time.h>

/**************************** java.util.Date **********************************/


struct java_lang_String* java_util_Date_toString 
	(struct java_util_Date* this) 
{
	time_t tt;
	char *buffer;
	int l;
	
	if (!this->valueValid) java_util_Date_computeValue(this);
	tt = builtin_l2i ( builtin_ldiv(this->value, builtin_i2l(1000) ) );

	buffer = ctime (&tt);
	l = strlen(buffer);
	while (l>1 && buffer[l-1]=='\n') l--;
	buffer[l] = '\0';
	
	return (java_lang_String*) javastring_new_char( buffer );
}


struct java_lang_String* java_util_Date_toLocaleString 
	(struct java_util_Date* this)
{
	time_t tt;
	char *buffer;
	int l;

	if (!this->valueValid) java_util_Date_computeValue(this);
	tt = builtin_l2i ( builtin_ldiv(this->value, builtin_i2l(1000) ) );

	buffer = asctime (localtime(&tt));
	l = strlen(buffer);
	while (l>1 && buffer[l-1]=='\n') l--;
	buffer[l] = '\0';

	return (java_lang_String*) javastring_new_char ( buffer );
}

struct java_lang_String* java_util_Date_toGMTString 
	(struct java_util_Date* this)
{
	time_t tt;
	char *buffer;
	int l;

	if (!this->valueValid) java_util_Date_computeValue(this);
	tt = builtin_l2i ( builtin_ldiv(this->value, builtin_i2l(1000) ) );

	buffer = asctime (gmtime(&tt));
	l = strlen(buffer);
	while (l>1 && buffer[l-1]=='\n') l--;
	buffer[l] = '\0';
	
	
	return (java_lang_String*) javastring_new_char ( buffer );
}



void java_util_Date_expand (struct java_util_Date* this)
{
	struct tm *t;
	time_t tt;
	
	if (this->expanded) return;

	tt = builtin_l2i ( builtin_ldiv(this->value, builtin_i2l(1000) ) );
	t = gmtime (&tt);
	this->tm_millis = 
	  builtin_l2i( builtin_lrem (this->value, builtin_i2l(1000) ) );
	this->tm_sec = t->tm_sec;
	this->tm_min = t->tm_min;
	this->tm_hour = t->tm_hour;
	this->tm_mday = t->tm_mday;
	this->tm_mon = t->tm_mon;
	this->tm_wday = t->tm_wday;
	this->tm_yday = t->tm_yday;
	this->tm_year = t->tm_year;
	this->tm_isdst = t->tm_isdst;
	this->expanded = 1;
}

void java_util_Date_computeValue (struct java_util_Date* this)
{
	struct tm t;
	
	if (this->valueValid) return;
	t.tm_sec = this->tm_sec;
	t.tm_min = this->tm_min;
	t.tm_hour = this->tm_hour;
	t.tm_mday = this->tm_mday;
	t.tm_mon = this->tm_mon;
	t.tm_wday = this->tm_wday;
	t.tm_yday = this->tm_yday;
	t.tm_year = this->tm_year;
	t.tm_isdst = this->tm_isdst;
	this->value = 
	  builtin_ladd( 
	   builtin_lmul( builtin_i2l(mktime(&t)), builtin_i2l(1000)), 
	   builtin_i2l(this->tm_millis)
	  );
	this->valueValid = 1;	
}
