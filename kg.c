#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef uint32_t EntityID;
#define INVALID_ID ((EntityID)0)

typedef struct {
    char** strings;
    size_t count;
    size_t capacity;
} StringTable;

typedef struct {
    EntityID subject;
    EntityID predicate;
    EntityID object;
} Triple;

typedef struct {
    Triple* triples;
    size_t count;
    size_t capacity;
} TripleStore;

typedef struct {
    StringTable strings;
    TripleStore triples;
} KGContext;

//Dynamic array helper
static void* grow(void* ptr, size_t elem_size, size_t* cap) {
    if (*cap == 0) *cap = 64;
    else *cap *= 2;
    return realloc(ptr, elem_size * (*cap));
}

//Init
void kg_init(KGContext* ctx) {
    ctx->strings.strings = NULL;
    ctx->strings.count = ctx->strings.capacity = 0;
    ctx->triples.triples = NULL;
    ctx->triples.count = ctx->triples.capacity = 0;
}

//Intern string - return EntityID
EntityID kg_intern(KGContext* ctx, const char* str) {
    for (size_t i = 0; i < ctx->strings.count; ++i)
        if (strcmp(ctx->strings.strings[i], str) == 0)
            return (EntityID)(i + 1);

    if (ctx->strings.count == ctx->strings.capacity)
        ctx->strings.strings = grow(ctx->strings.strings, sizeof(char*), &ctx->strings.capacity);

    ctx->strings.strings[ctx->strings.count] = strdup(str);
    return ++ctx->strings.count;
}

const char* kg_str(KGContext* ctx, EntityID id) {
    if (id == 0 || id > ctx->strings.count) return "<invalid>";
    return ctx->strings.strings[id - 1];
}

void kg_add(KGContext* ctx, EntityID s, EntityID p, EntityID o) {
    if (ctx->triples.count == ctx->triples.capacity)
        ctx->triples.triples = grow(ctx->triples.triples, sizeof(Triple), &ctx->triples.capacity);
    ctx->triples.triples[ctx->triples.count++] = (Triple){s, p, o};
}

//MACROS - for pre-interned EntityIDs
#define LIT(ctx, literal)     kg_intern(ctx, literal)     // for string literals only
#define ADD(ctx, s, p, o)     kg_add(ctx, s, p, o)        // use after entities are created!

void kg_print(const KGContext* ctx) {
    printf("=== Knowledge Graph ===\n");
    printf("Entities: %zu\n", ctx->strings.count);
    printf("Triples : %zu\n\n", ctx->triples.count);
    for (size_t i = 0; i < ctx->triples.count; ++i) {
        Triple t = ctx->triples.triples[i];
        printf("%s --[%s]--> %s\n",
               kg_str((KGContext*)ctx, t.subject),
               kg_str((KGContext*)ctx, t.predicate),
               kg_str((KGContext*)ctx, t.object));
    }
}

void kg_free(KGContext* ctx) {
    for (size_t i = 0; i < ctx->strings.count; ++i) free(ctx->strings.strings[i]);
    free(ctx->strings.strings);
    free(ctx->triples.triples);
}

int main(void) {
    KGContext ctx;
    kg_init(&ctx);

    //Create predicate entities once
    EntityID IS_A       = LIT(&ctx, "is-a");
    EntityID PART_OF    = LIT(&ctx, "part-of");
    EntityID CONTAINS   = LIT(&ctx, "contains");
    EntityID NEXT_TO    = LIT(&ctx, "next-to");
    EntityID HAS_LETTER = LIT(&ctx, "has-letter");

    //Create leaf entities
    EntityID A  = LIT(&ctx, "A");
    EntityID H  = LIT(&ctx, "H");
    EntityID E  = LIT(&ctx, "E");
    EntityID L  = LIT(&ctx, "L");
    EntityID O  = LIT(&ctx, "O");
    EntityID W  = LIT(&ctx, "W");
    EntityID R  = LIT(&ctx, "R");
    EntityID D  = LIT(&ctx, "D");

    EntityID hello = LIT(&ctx, "hello");
    EntityID world = LIT(&ctx, "world");
    EntityID sentence = LIT(&ctx, "Hello world.");
    EntityID paragraph = LIT(&ctx, "first-paragraph");

    //Now build the graph using raw EntityIDs
    ADD(&ctx, LIT(&ctx, "A"),            IS_A,       LIT(&ctx, "letter"));
    ADD(&ctx, hello,                     IS_A,       LIT(&ctx, "word"));
    ADD(&ctx, world,                     IS_A,       LIT(&ctx, "word"));
    ADD(&ctx, sentence,                  IS_A,       LIT(&ctx, "sentence"));
    ADD(&ctx, paragraph,                 IS_A,       LIT(&ctx, "paragraph"));

    //Letter composition
    ADD(&ctx, hello, HAS_LETTER, H);
    ADD(&ctx, hello, HAS_LETTER, E);
    ADD(&ctx, hello, HAS_LETTER, L);
    ADD(&ctx, hello, HAS_LETTER, L);
    ADD(&ctx, hello, HAS_LETTER, O);

    ADD(&ctx, world, HAS_LETTER, W);
    ADD(&ctx, world, HAS_LETTER, O);
    ADD(&ctx, world, HAS_LETTER, R);
    ADD(&ctx, world, HAS_LETTER, L);
    ADD(&ctx, world, HAS_LETTER, D);

    //Sentence structure
    ADD(&ctx, sentence, CONTAINS, hello);
    ADD(&ctx, sentence, CONTAINS, world);
    ADD(&ctx, hello,    NEXT_TO,  world);

    //Hierarchy
    ADD(&ctx, sentence, PART_OF, paragraph);

    //Print result
    kg_print(&ctx);

    // Cleanup
    kg_free(&ctx);

    return 0;
}
