#ifndef _METHODTABLE_H
#define _METHODTABLE_H

typedef struct _mtentry mtentry;

struct _mtentry {
    u1 *start;
    u1 *end;
    mtentry *next;
};

void addmethod(u1 *start, u1 *end);
u1 *findmethod(u1 *pos);

#endif
