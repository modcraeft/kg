#ifndef KG_H
#define KG_H

#include <stdint.h>
#include <stddef.h>

typedef uint32_t EntityID;
#define INVALID_ID 0

typedef struct {
    char** s;
    size_t n, cap;
} StringTable;

typedef struct {
    EntityID s, p, o;
} Triple;

typedef struct {
    Triple* t;
    size_t n, cap;
} TripleStore;

typedef struct {
    StringTable strings;
    TripleStore triples;
} KGContext;

/* Core API */
void kg_init(KGContext* ctx);
EntityID kg_intern(KGContext* ctx, const char* str);
const char* kg_str(KGContext* ctx, EntityID id);
void kg_add(KGContext* ctx, EntityID s, EntityID p, EntityID o);
void kg_free(KGContext* ctx);
int kg_load_text(KGContext* ctx, const char* path);

#endif
