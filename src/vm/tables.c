/* tables.c - 

   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003
   R. Grafl, A. Krall, C. Kruegel, C. Oates, R. Obermaisser,
   M. Probst, S. Ring, E. Steiner, C. Thalinger, D. Thuernbeck,
   P. Tomsich, J. Wenninger

   This file is part of CACAO.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   Contact: cacao@complang.tuwien.ac.at

   Authors: Reinhard Grafl

   Changes: Mark Probst
            Andreas Krall

   Contains support functions for:
       - Reading of Java class files
       - Unicode symbols
       - the heap
       - additional support functions

   $Id: tables.c 1087 2004-05-26 21:27:03Z twisti $

*/

#include "global.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include "types.h"
#include "main.h"
#include "tables.h"
#include "loader.h"
#include "asmpart.h"
#include "threads/thread.h"
#include "threads/locks.h"
#include "toolbox/logging.h"
#include "toolbox/memory.h"


/* statistics */
int count_utf_len = 0;         /* size of utf hash                  */
int count_utf_new = 0;         /* calls of utf_new                  */
int count_utf_new_found  = 0;  /* calls of utf_new with fast return */

hashtable utf_hash;     /* hashtable for utf8-symbols */
hashtable string_hash;  /* hashtable for javastrings  */
hashtable class_hash;   /* hashtable for classes      */

/******************************************************************************
 *********************** hashtable functions **********************************
 ******************************************************************************/

/* hashsize must be power of 2 */

#define UTF_HASHSTART   16384   /* initial size of utf-hash */    
#define HASHSTART        2048   /* initial size of javastring and class-hash */


/******************** function: init_hashtable ******************************

    Initializes a hashtable structure and allocates memory.
    The parameter size specifies the initial size of the hashtable.
	
*****************************************************************************/

void init_hashtable(hashtable *hash, u4 size)
{
	u4 i;

	hash->entries = 0;
	hash->size    = size;
	hash->ptr     = MNEW(void*, size);

	/* clear table */
	for (i = 0; i < size; i++) hash->ptr[i] = NULL;
}


/*********************** function: tables_init  *****************************

    creates hashtables for symboltables 
	(called once at startup)			 
	
*****************************************************************************/

void tables_init()
{
	init_hashtable(&utf_hash,    UTF_HASHSTART);  /* hashtable for utf8-symbols */
	init_hashtable(&string_hash, HASHSTART);      /* hashtable for javastrings */
	init_hashtable(&class_hash,  HASHSTART);      /* hashtable for classes */ 
	
#ifdef STATISTICS
	if (opt_stat)
		count_utf_len += sizeof(utf*) * utf_hash.size;
#endif

}


/********************** function: tables_close ******************************

        free memory for hashtables		      
	
*****************************************************************************/

void tables_close(stringdeleter del)
{
	utf *u = NULL;
	literalstring *s;
	u4 i;
	
	/* dispose utf symbols */
	for (i = 0; i < utf_hash.size; i++) {
		u = utf_hash.ptr[i];
		while (u) {
			/* process elements in external hash chain */
			utf *nextu = u->hashlink;
			MFREE(u->text, u1, u->blength);
			FREE(u, utf);
			u = nextu;
		}	
	}

	/* dispose javastrings */
	for (i = 0; i < string_hash.size; i++) {
		s = string_hash.ptr[i];
		while (u) {
			/* process elements in external hash chain */
			literalstring *nexts = s->hashlink;
			del(s->string);
			FREE(s, literalstring);
			s = nexts;
		}	
	}

	/* dispose hashtable structures */
	MFREE(utf_hash.ptr,    void*, utf_hash.size);
	MFREE(string_hash.ptr, void*, string_hash.size);
	MFREE(class_hash.ptr,  void*, class_hash.size);
}


/********************* function: utf_display *********************************

	write utf symbol to stdout (debugging purposes)

******************************************************************************/

void utf_display(utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */

	if (!u)
		return;

    while (utf_ptr < endpos) {
		/* read next unicode character */                
		u2 c = utf_nextu2(&utf_ptr);
		if (c >= 32 && c <= 127) printf("%c", c);
		else printf("?");
	}

	fflush(stdout);
}


/********************* function: utf_display *********************************

	write utf symbol to stdout (debugging purposes)

******************************************************************************/

void utf_display_classname(utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */

	if (!u)
		return;

    while (utf_ptr < endpos) {
		/* read next unicode character */                
		u2 c = utf_nextu2(&utf_ptr);
		if (c == '/') c = '.';
		if (c >= 32 && c <= 127) printf("%c", c);
		else printf("?");
	}

	fflush(stdout);
}


/************************* function: log_utf *********************************

	log utf symbol

******************************************************************************/

void log_utf(utf *u)
{
	char buf[MAXLOGTEXT];
	utf_sprint(buf, u);
	dolog("%s", buf);
}


/********************** function: log_plain_utf ******************************

	log utf symbol (without printing "LOG: " and newline)

******************************************************************************/

void log_plain_utf(utf *u)
{
	char buf[MAXLOGTEXT];
	utf_sprint(buf, u);
	dolog_plain("%s", buf);
}


/************************ function: utf_sprint *******************************
	
    write utf symbol into c-string (debugging purposes)						 

******************************************************************************/

void utf_sprint(char *buffer, utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */ 
    u2 pos = 0;                  /* position in c-string           */

    while (utf_ptr < endpos) 
		/* copy next unicode character */       
		buffer[pos++] = utf_nextu2(&utf_ptr);

    /* terminate string */
    buffer[pos] = '\0';
}


/************************ function: utf_sprint_classname *********************
	
    write utf symbol into c-string (debugging purposes)

******************************************************************************/ 

void utf_sprint_classname(char *buffer, utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */ 
    u2 pos = 0;                  /* position in c-string           */

    while (utf_ptr < endpos) {
		/* copy next unicode character */       
		u2 c = utf_nextu2(&utf_ptr);
		if (c == '/') c = '.';
		buffer[pos++] = c;
	}

    /* terminate string */
    buffer[pos] = '\0';
}


/********************* Funktion: utf_fprint **********************************
	
    write utf symbol into file		

******************************************************************************/

void utf_fprint(FILE *file, utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */ 

    if (!u)
		return;

    while (utf_ptr < endpos) { 
		/* read next unicode character */                
		u2 c = utf_nextu2(&utf_ptr);				

		if (c >= 32 && c <= 127) fprintf(file, "%c", c);
		else fprintf(file, "?");
	}
}


/********************* Funktion: utf_fprint **********************************
	
    write utf symbol into file		

******************************************************************************/

void utf_fprint_classname(FILE *file, utf *u)
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */ 

    if (!u)
		return;

    while (utf_ptr < endpos) { 
		/* read next unicode character */                
		u2 c = utf_nextu2(&utf_ptr);				
		if (c == '/') c = '.';

		if (c >= 32 && c <= 127) fprintf(file, "%c", c);
		else fprintf(file, "?");
	}
}


/****************** internal function: utf_hashkey ***************************

	The hashkey is computed from the utf-text by using up to 8 characters.
	For utf-symbols longer than 15 characters 3 characters are taken from
	the beginning and the end, 2 characters are taken from the middle.

******************************************************************************/ 

#define nbs(val) ((u4) *(++text) << val) /* get next byte, left shift by val  */
#define fbs(val) ((u4) *(  text) << val) /* get first byte, left shift by val */

static u4 utf_hashkey(char *text, u4 length)
{
	char *start_pos = text; /* pointer to utf text */
	u4 a;

	switch (length) {		
	        
	case 0: /* empty string */
		return 0;

	case 1: return fbs(0);
	case 2: return fbs(0) ^ nbs(3);
	case 3: return fbs(0) ^ nbs(3) ^ nbs(5);
	case 4: return fbs(0) ^ nbs(2) ^ nbs(4) ^ nbs(6);
	case 5: return fbs(0) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(6);
	case 6: return fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(5) ^ nbs(6);
	case 7: return fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(5) ^ nbs(6);
	case 8: return fbs(0) ^ nbs(1) ^ nbs(2) ^ nbs(3) ^ nbs(4) ^ nbs(5) ^ nbs(6) ^ nbs(7);

	case 9:
		a = fbs(0);
		a ^= nbs(1);
		a ^= nbs(2);
		text++;
		return a ^ nbs(4) ^ nbs(5) ^ nbs(6) ^ nbs(7) ^ nbs(8);

	case 10:
		a = fbs(0);
		text++;
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		text++;
		return a ^ nbs(6) ^ nbs(7) ^ nbs(8) ^ nbs(9);

	case 11:
		a = fbs(0);
		text++;
		a ^= nbs(2);
		a ^= nbs(3);
		a ^= nbs(4);
		text++;
		return a ^ nbs(6) ^ nbs(7) ^ nbs(8) ^ nbs(9) ^ nbs(10);

	case 12:
		a = fbs(0);
		text += 2;
		a ^= nbs(2);
		a ^= nbs(3);
		text++;
		a ^= nbs(5);
		a ^= nbs(6);
		a ^= nbs(7);
		text++;
		return a ^ nbs(9) ^ nbs(10);

	case 13:
		a = fbs(0);
		a ^= nbs(1);
		text++;
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;	
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		return a ^ nbs(9) ^ nbs(10);

	case 14:
		a = fbs(0);
		text += 2;	
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;	
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		return a ^ nbs(9) ^ nbs(10) ^ nbs(11);

	case 15:
		a = fbs(0);
		text += 2;	
		a ^= nbs(3);
		a ^= nbs(4);
		text += 2;	
		a ^= nbs(7);
		a ^= nbs(8);
		text += 2;
		return a ^ nbs(9) ^ nbs(10) ^ nbs(11);

	default:  /* 3 characters from beginning */
		a = fbs(0);
		text += 2;
		a ^= nbs(3);
		a ^= nbs(4);

		/* 2 characters from middle */
		text = start_pos + (length / 2);
		a ^= fbs(5);
		text += 2;
		a ^= nbs(6);	

		/* 3 characters from end */
		text = start_pos + length - 4;

		a ^= fbs(7);
		text++;

		return a ^ nbs(10) ^ nbs(11);
    }
}


/*************************** function: utf_hashkey ***************************

    compute the hashkey of a unicode string

******************************************************************************/ 

u4 unicode_hashkey(u2 *text, u2 len)
{
	return utf_hashkey((char*) text, len);
}


/************************ function: utf_new **********************************

	Creates a new utf-symbol, the text of the symbol is passed as a 
	u1-array. The function searches the utf-hashtable for a utf-symbol 
	with this text. On success the element returned, otherwise a new 
	hashtable element is created.

	If the number of entries in the hashtable exceeds twice the size of the
	hashtable slots a reorganization of the hashtable is done and the utf 
	symbols are copied to a new hashtable with doubled size.

******************************************************************************/

utf *utf_new_int(char *text, u2 length)
{
	u4 key;            /* hashkey computed from utf-text */
	u4 slot;           /* slot in hashtable */
	utf *u;            /* hashtable element */
	u2 i;

#ifdef STATISTICS
	if (opt_stat)
		count_utf_new++;
#endif

	key  = utf_hashkey(text, length);
	slot = key & (utf_hash.size-1);
	u    = utf_hash.ptr[slot];

	/* search external hash chain for utf-symbol */
	while (u) {
		if (u->blength == length) {

			/* compare text of hashtable elements */
			for (i = 0; i < length; i++)
				if (text[i] != u->text[i]) goto nomatch;
			
#ifdef STATISTICS
			if (opt_stat)
				count_utf_new_found++;
#endif
/*			log_text("symbol found in hash table");*/
			/* symbol found in hashtable */
/*   					utf_display(u);
					{
						utf blup;
						blup.blength=length;
						blup.text=text;
						utf_display(&blup);
					}*/
			return u;
		}
	nomatch:
		u = u->hashlink; /* next element in external chain */
	}

#ifdef STATISTICS
	if (opt_stat)
		count_utf_len += sizeof(utf) + length;
#endif

	/* location in hashtable found, create new utf element */
	u = NEW(utf);
	u->blength  = length;               /* length in bytes of utfstring       */
	u->hashlink = utf_hash.ptr[slot];   /* link in external hashchain         */
	u->text     = mem_alloc(length + 1);/* allocate memory for utf-text       */
	memcpy(u->text, text, length);      /* copy utf-text                      */
	u->text[length] = '\0';
	utf_hash.ptr[slot] = u;             /* insert symbol into table           */

	utf_hash.entries++;                 /* update number of entries           */

	if (utf_hash.entries > (utf_hash.size * 2)) {

        /* reorganization of hashtable, average length of 
           the external chains is approx. 2                */  

		u4 i;
		utf *u;
		hashtable newhash; /* the new hashtable */

		/* create new hashtable, double the size */
		init_hashtable(&newhash, utf_hash.size * 2);
		newhash.entries = utf_hash.entries;

#ifdef STATISTICS
		if (opt_stat)
			count_utf_len += sizeof(utf*) * utf_hash.size;
#endif

		/* transfer elements to new hashtable */
		for (i = 0; i < utf_hash.size; i++) {
			u = (utf *) utf_hash.ptr[i];
			while (u) {
				utf *nextu = u->hashlink;
				u4 slot = utf_hashkey(u->text, u->blength) & (newhash.size - 1);
						
				u->hashlink = (utf *) newhash.ptr[slot];
				newhash.ptr[slot] = u;

				/* follow link in external hash chain */
				u = nextu;
			}
		}
	
		/* dispose old table */
		MFREE(utf_hash.ptr, void*, utf_hash.size);
		utf_hash = newhash;
	}

	return u;
}


utf *utf_new(char *text, u2 length)
{
    utf *r;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_lock();
#endif

    r = utf_new_int(text, length);

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_unlock();
#endif

    return r;
}


/********************* function: utf_new_char ********************************

    creates a new utf symbol, the text for this symbol is passed
    as a c-string ( = char* )

******************************************************************************/

utf *utf_new_char(char *text)
{
	return utf_new(text, strlen(text));
}


/********************* function: utf_new_char ********************************

    creates a new utf symbol, the text for this symbol is passed
    as a c-string ( = char* )
    "." characters are going to be replaced by "/". since the above function is
    used often, this is a separte function, instead of an if

******************************************************************************/

utf *utf_new_char_classname(char *text)
{
	if (strchr(text, '.')) {
		char *txt = strdup(text);
		char *end = txt + strlen(txt);
		char *c;
		utf *tmpRes;
		for (c = txt; c < end; c++)
			if (*c == '.') *c = '/';
		tmpRes = utf_new(txt, strlen(txt));
		free(txt);
		return tmpRes;

	} else
		return utf_new(text, strlen(text));
}


/************************** Funktion: utf_show ******************************

    writes the utf symbols in the utfhash to stdout and
    displays the number of external hash chains grouped 
    according to the chainlength
    (debugging purposes)

*****************************************************************************/

void utf_show()
{

#define CHAIN_LIMIT 20               /* limit for seperated enumeration */

	u4 chain_count[CHAIN_LIMIT]; /* numbers of chains */
	u4 max_chainlength = 0;      /* maximum length of the chains */
	u4 sum_chainlength = 0;      /* sum of the chainlengths */
	u4 beyond_limit = 0;         /* number of utf-symbols in chains with length>=CHAIN_LIMIT-1 */
	u4 i;

	printf ("UTF-HASH:\n");

	/* show element of utf-hashtable */
	for (i=0; i<utf_hash.size; i++) {
		utf *u = utf_hash.ptr[i];
		if (u) {
			printf ("SLOT %d: ", (int) i);
			while (u) {
				printf ("'");
				utf_display (u);
				printf ("' ");
				u = u->hashlink;
			}	
			printf ("\n");
		}
		
	}

	printf ("UTF-HASH: %d slots for %d entries\n", 
			(int) utf_hash.size, (int) utf_hash.entries );


	if (utf_hash.entries == 0)
		return;

	printf("chains:\n  chainlength    number of chains    %% of utfstrings\n");

	for (i=0;i<CHAIN_LIMIT;i++)
		chain_count[i]=0;

	/* count numbers of hashchains according to their length */
	for (i=0; i<utf_hash.size; i++) {
		  
		utf *u = (utf*) utf_hash.ptr[i];
		u4 chain_length = 0;

		/* determine chainlength */
		while (u) {
			u = u->hashlink;
			chain_length++;
		}

		/* update sum of all chainlengths */
		sum_chainlength+=chain_length;

		/* determine the maximum length of the chains */
		if (chain_length>max_chainlength)
			max_chainlength = chain_length;

		/* update number of utf-symbols in chains with length>=CHAIN_LIMIT-1 */
		if (chain_length>=CHAIN_LIMIT) {
			beyond_limit+=chain_length;
			chain_length=CHAIN_LIMIT-1;
		}

		/* update number of hashchains of current length */
		chain_count[chain_length]++;
	}

	/* display results */  
	for (i=1;i<CHAIN_LIMIT-1;i++) 
		printf("       %2d %17d %18.2f%%\n",i,chain_count[i],(((float) chain_count[i]*i*100)/utf_hash.entries));
	  
	printf("     >=%2d %17d %18.2f%%\n",CHAIN_LIMIT-1,chain_count[CHAIN_LIMIT-1],((float) beyond_limit*100)/utf_hash.entries);


	printf("max. chainlength:%5d\n",max_chainlength);

	/* avg. chainlength = sum of chainlengths / number of chains */
	printf("avg. chainlength:%5.2f\n",(float) sum_chainlength / (utf_hash.size-chain_count[0]));
}

/******************************************************************************
*********************** Misc support functions ********************************
******************************************************************************/


/******************** Function: desc_to_type **********************************
   
	Determines the corresponding Java base data type for a given type
	descriptor.
	
******************************************************************************/

u2 desc_to_type(utf *descriptor)
{
	char *utf_ptr = descriptor->text;  /* current position in utf text */
	char logtext[MAXLOGTEXT];

	if (descriptor->blength < 1) panic("Type-Descriptor is empty string");
	
	switch (*utf_ptr++) {
	case 'B': 
	case 'C':
	case 'I':
	case 'S':  
	case 'Z':  return TYPE_INT;
	case 'D':  return TYPE_DOUBLE;
	case 'F':  return TYPE_FLOAT;
	case 'J':  return TYPE_LONG;
	case 'L':
	case '[':  return TYPE_ADDRESS;
	}
			
	sprintf(logtext, "Invalid Type-Descriptor: ");
	utf_sprint(logtext+strlen(logtext), descriptor);
	error("%s",logtext);

	return 0;
}


/********************** Function: desc_typesize *******************************

	Calculates the lenght in bytes needed for a data element of the type given
	by its type descriptor.
	
******************************************************************************/

u2 desc_typesize(utf *descriptor)
{
	switch (desc_to_type(descriptor)) {
	case TYPE_INT:     return 4;
	case TYPE_LONG:    return 8;
	case TYPE_FLOAT:   return 4;
	case TYPE_DOUBLE:  return 8;
	case TYPE_ADDRESS: return sizeof(voidptr);
	default:           return 0;
	}
}


/********************** function: utf_nextu2 *********************************

    read the next unicode character from the utf string and
    increment the utf-string pointer accordingly

******************************************************************************/

u2 utf_nextu2(char **utf_ptr) 
{
    /* uncompressed unicode character */
    u2 unicode_char = 0;
    /* current position in utf text */	
    unsigned char *utf = (unsigned char *) (*utf_ptr);
    /* bytes representing the unicode character */
    unsigned char ch1, ch2, ch3;
    /* number of bytes used to represent the unicode character */
    int len = 0;
	
    switch ((ch1 = utf[0]) >> 4) {
	default: /* 1 byte */
		(*utf_ptr)++;
		return ch1;
	case 0xC: 
	case 0xD: /* 2 bytes */
		if (((ch2 = utf[1]) & 0xC0) == 0x80) {
			unsigned char high = ch1 & 0x1F;
			unsigned char low  = ch2 & 0x3F;
			unicode_char = (high << 6) + low;
			len = 2;
		}
		break;

	case 0xE: /* 2 or 3 bytes */
		if (((ch2 = utf[1]) & 0xC0) == 0x80) {
			if (((ch3 = utf[2]) & 0xC0) == 0x80) {
				unsigned char low  = ch3 & 0x3f;
				unsigned char mid  = ch2 & 0x3f;
				unsigned char high = ch1 & 0x0f;
				unicode_char = (((high << 6) + mid) << 6) + low;
				len = 3;
			} else
				len = 2;					   
		}
		break;
    }

    /* update position in utf-text */
    *utf_ptr = (char *) (utf + len);
    return unicode_char;
}


/********************* function: is_valid_utf ********************************

    return true if the given string is a valid UTF-8 string

    utf_ptr...points to first character
    end_pos...points after last character

******************************************************************************/

static unsigned long min_codepoint[6] = {0,1L<<7,1L<<11,1L<<16,1L<<21,1L<<26};

bool
is_valid_utf(char *utf_ptr,char *end_pos)
{
	int bytes;
	int len,i;
	char c;
	unsigned long v;

	if (end_pos < utf_ptr) return false;
	bytes = end_pos - utf_ptr;
	while (bytes--) {
		c = *utf_ptr++;
		/*dolog("%c %02x",c,c);*/
		if (!c) return false;                     /* 0x00 is not allowed */
		if ((c & 0x80) == 0) continue;            /* ASCII */

		if      ((c & 0xe0) == 0xc0) len = 1;     /* 110x xxxx */
		else if ((c & 0xf0) == 0xe0) len = 2;     /* 1110 xxxx */
		else if ((c & 0xf8) == 0xf0) len = 3;     /* 1111 0xxx */
		else if ((c & 0xfc) == 0xf8) len = 4;     /* 1111 10xx */
		else if ((c & 0xfe) == 0xfc) len = 5;     /* 1111 110x */
		else return false;                        /* invalid leading byte */

		if (len > 2) return false;                /* Java limitation */

		v = (unsigned long)c & (0x3f >> len);
		
		if ((bytes -= len) < 0) return false;     /* missing bytes */

		for (i = len; i--; ) {
			c = *utf_ptr++;
			/*dolog("    %c %02x",c,c);*/
			if ((c & 0xc0) != 0x80)               /* 10xx xxxx */
				return false;
			v = (v<<6) | (c & 0x3f);
		}

		/*		dolog("v=%d",v);*/

		if (v == 0) {
			if (len != 1) return false;           /* Java special */
		}
		else {
			/* Sun Java seems to allow overlong UTF-8 encodings */
			
			if (v < min_codepoint[len]) { /* overlong UTF-8 */
				if (!opt_liberalutf)
					fprintf(stderr,"WARNING: Overlong UTF-8 sequence found.\n");
				/* XXX change this to panic? */
			}
		}

		/* surrogates in UTF-8 seem to be allowed in Java classfiles */
		/* if (v >= 0xd800 && v <= 0xdfff) return false; */ /* surrogates */

		/* even these seem to be allowed */
		/* if (v == 0xfffe || v == 0xffff) return false; */ /* invalid codepoints */
	}

	return true;
}
 
/********************* function: is_valid_name *******************************

    return true if the given string may be used as a class/field/method name.
    (Currently this only disallows empty strings and control characters.)

    NOTE: The string is assumed to have passed is_valid_utf!

    utf_ptr...points to first character
    end_pos...points after last character

******************************************************************************/

bool
is_valid_name(char *utf_ptr,char *end_pos)
{
	if (end_pos <= utf_ptr) return false; /* disallow empty names */

	while (utf_ptr < end_pos) {
		unsigned char c = *utf_ptr++;

		if (c < 0x20) return false; /* disallow control characters */
		if (c == 0xc0 && (unsigned char)*utf_ptr == 0x80) return false; /* disallow zero */
	}
	return true;
}

bool
is_valid_name_utf(utf *u)
{
	return is_valid_name(u->text,utf_end(u));
}

/******************** Function: class_new **************************************

    searches for the class with the specified name in the classes hashtable,
    if there is no such class a new classinfo structure is created and inserted
    into the list of classes to be loaded

*******************************************************************************/

classinfo *class_new_int(utf *classname)
{
	classinfo *c;     /* hashtable element */
	u4 key;           /* hashkey computed from classname */
	u4 slot;          /* slot in hashtable */
	u2 i;

	key  = utf_hashkey(classname->text, classname->blength);
	slot = key & (class_hash.size - 1);
	c    = class_hash.ptr[slot];

	/* search external hash chain for the class */
	while (c) {
		if (c->name->blength == classname->blength) {
			for (i = 0; i < classname->blength; i++)
				if (classname->text[i] != c->name->text[i]) goto nomatch;
						
			/* class found in hashtable */
			return c;
		}
			
	nomatch:
		c = c->hashlink; /* next element in external chain */
	}

	/* location in hashtable found, create new classinfo structure */

#ifdef STATISTICS
	if (opt_stat)
		count_class_infos += sizeof(classinfo);
#endif

	if (initverbose) {
		char logtext[MAXLOGTEXT];
		sprintf(logtext, "Creating class: ");
		utf_sprint_classname(logtext + strlen(logtext), classname);
		log_text(logtext);
	}

	c = GCNEW(classinfo, 1); /*JOWENN: NEW*/
	/*c=NEW(classinfo);*/
	c->vmClass = 0;
	c->flags = 0;
	c->name = classname;
	c->packagename = NULL;
	c->cpcount = 0;
	c->cptags = NULL;
	c->cpinfos = NULL;
	c->super = NULL;
	c->sub = NULL;
	c->nextsub = NULL;
	c->interfacescount = 0;
	c->interfaces = NULL;
	c->fieldscount = 0;
	c->fields = NULL;
	c->methodscount = 0;
	c->methods = NULL;
	c->linked = false;
	c->loaded = false;
	c->index = 0;
	c->instancesize = 0;
	c->header.vftbl = NULL;
	c->innerclasscount = 0;
	c->innerclass = NULL;
	c->vftbl = NULL;
	c->initialized = false;
	c->classvftbl = false;
    c->classUsed = 0;
    c->impldBy = NULL;
	c->classloader = NULL;
	c->sourcefile = NULL;
	
	/* insert class into the hashtable */
	c->hashlink = class_hash.ptr[slot];
	class_hash.ptr[slot] = c;

	/* update number of hashtable-entries */
	class_hash.entries++;

	if (class_hash.entries > (class_hash.size * 2)) {

		/* reorganization of hashtable, average length of 
		   the external chains is approx. 2                */  

		u4 i;
		classinfo *c;
		hashtable newhash;  /* the new hashtable */

		/* create new hashtable, double the size */
		init_hashtable(&newhash, class_hash.size * 2);
		newhash.entries = class_hash.entries;

		/* transfer elements to new hashtable */
		for (i = 0; i < class_hash.size; i++) {
			c = (classinfo *) class_hash.ptr[i];
			while (c) {
				classinfo *nextc = c->hashlink;
				u4 slot = (utf_hashkey(c->name->text, c->name->blength)) & (newhash.size - 1);
						
				c->hashlink = newhash.ptr[slot];
				newhash.ptr[slot] = c;

				c = nextc;
			}
		}
	
		/* dispose old table */	
		MFREE(class_hash.ptr, void*, class_hash.size);
		class_hash = newhash;
	}

    /* Array classes need further initialization. */
    if (c->name->text[0] == '[') {
		/* Array classes are not loaded from classfiles. */
		c->loaded = true;
        class_new_array(c);
		c->packagename = array_packagename;

	} else {
		/* Find the package name */
		/* Classes in the unnamed package keep packagename == NULL. */
		char *p = utf_end(c->name) - 1;
		char *start = c->name->text;
		for (;p > start; --p) {
			if (*p == '.') {
				c->packagename = utf_new(start, p - start);
				break;
			}
		}
	}

	/* we support eager class loading and linking on demand */

	if (opt_eager) {
		/* all super classes are loaded implicitly */
/*  		if (!c->loaded) */
/*  			class_load(c); */

/*  		if (!c->linked) */
/*  			class_link(c); */
	}

	return c;
}


classinfo *class_new(utf *classname)
{
    classinfo *c;

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_lock();
#endif

    c = class_new_int(classname);

	/* we support eager class loading and linking on demand */

	if (opt_eager) {
		if (!c->loaded)
			class_load(c);

		if (!c->linked)
			class_link(c);
	}

#if defined(USE_THREADS) && defined(NATIVE_THREADS)
    tables_unlock();
#endif

    return c;
}


/******************** Function: class_get **************************************

    searches for the class with the specified name in the classes hashtable
    if there is no such class NULL is returned

*******************************************************************************/

classinfo *class_get(utf *u)
{
	classinfo *c;  /* hashtable element */ 
	u4 key;        /* hashkey computed from classname */   
	u4 slot;       /* slot in hashtable */
	u2 i;  

	key  = utf_hashkey (u->text, u->blength);
	slot = key & (class_hash.size-1);
	c    = class_hash.ptr[slot];

	/* search external hash-chain */
	while (c) {
		if (c->name->blength == u->blength) {
			
			/* compare classnames */
			for (i=0; i<u->blength; i++) 
				if (u->text[i] != c->name->text[i]) goto nomatch;

			/* class found in hashtable */				
			return c;
		}
			
	nomatch:
		c = c->hashlink;
	}

	/* class not found */
	return NULL;
}


/***************** Function: class_array_of ***********************************

    Returns an array class with the given component class.
    The array class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_array_of(classinfo *component)
{
    int namelen;
    char *namebuf;
	classinfo *c;

    /* Assemble the array class name */
    namelen = component->name->blength;
    
    if (component->name->text[0] == '[') {
        /* the component is itself an array */
        namebuf = DMNEW(char, namelen + 1);
        namebuf[0] = '[';
        memcpy(namebuf + 1, component->name->text, namelen);
        namelen++;

    } else {
        /* the component is a non-array class */
        namebuf = DMNEW(char, namelen + 3);
        namebuf[0] = '[';
        namebuf[1] = 'L';
        memcpy(namebuf + 2, component->name->text, namelen);
        namebuf[2 + namelen] = ';';
        namelen += 3;
    }

	/* load this class ;-) and link it */
	c = class_new(utf_new(namebuf, namelen));
	c->loaded = 1;
	class_link(c);

    return c;
}

/*************** Function: class_multiarray_of ********************************

    Returns an array class with the given dimension and element class.
    The array class is dynamically created if neccessary.

*******************************************************************************/

classinfo *class_multiarray_of(int dim, classinfo *element)
{
    int namelen;
    char *namebuf;

	if (dim < 1)
		panic("Invalid array dimension requested");

    /* Assemble the array class name */
    namelen = element->name->blength;
    
    if (element->name->text[0] == '[') {
        /* the element is itself an array */
        namebuf = DMNEW(char, namelen + dim);
        memcpy(namebuf + dim, element->name->text, namelen);
        namelen += dim;
    }
    else {
        /* the element is a non-array class */
        namebuf = DMNEW(char, namelen + 2 + dim);
        namebuf[dim] = 'L';
        memcpy(namebuf + dim + 1, element->name->text, namelen);
        namelen += (2 + dim);
        namebuf[namelen - 1] = ';';
    }
	memset(namebuf, '[', dim);

    return class_new(utf_new(namebuf, namelen));
}

/************************** function: utf_strlen ******************************

    determine number of unicode characters in the utf string

*******************************************************************************/

u4 utf_strlen(utf *u) 
{
    char *endpos  = utf_end(u);  /* points behind utf string       */
    char *utf_ptr = u->text;     /* current position in utf text   */
    u4 len = 0;                  /* number of unicode characters   */

    while (utf_ptr < endpos) {
		len++;
		/* next unicode character */
		utf_nextu2(&utf_ptr);
    }

    if (utf_ptr != endpos)
    	/* string ended abruptly */
		panic("illegal utf string"); 

    return len;
}


/*
 * These are local overrides for various environment variables in Emacs.
 * Please do not remove this and leave it at the end of the file, where
 * Emacs will automagically detect them.
 * ---------------------------------------------------------------------
 * Local variables:
 * mode: c
 * indent-tabs-mode: t
 * c-basic-offset: 4
 * tab-width: 4
 * End:
 */
