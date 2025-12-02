#include "../include/kg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef uint32_t EntityID;
#define INVALID_ID 0


static void* grow(void* p, size_t es, size_t* cap) {
    if (*cap == 0) *cap = 64; else *cap *= 2;
    return realloc(p, es * (*cap));
}

void kg_init(KGContext* ctx) {
    ctx->strings.s = NULL; ctx->strings.n = ctx->strings.cap = 0;
    ctx->triples.t = NULL; ctx->triples.n = ctx->triples.cap = 0;
}

EntityID kg_intern(KGContext* ctx, const char* str) {
    for (size_t i = 0; i < ctx->strings.n; ++i)
        if (strcmp(ctx->strings.s[i], str) == 0)
            return (EntityID)(i + 1);
    if (ctx->strings.n == ctx->strings.cap)
        ctx->strings.s = grow(ctx->strings.s, sizeof(char*), &ctx->strings.cap);
    ctx->strings.s[ctx->strings.n] = strdup(str);
    return ++ctx->strings.n;
}

const char* kg_str(KGContext* ctx, EntityID id) {
    return (id == 0 || id > ctx->strings.n) ? "<invalid>" : ctx->strings.s[id-1];
}

void kg_add(KGContext* ctx, EntityID s, EntityID p, EntityID o) {
    if (ctx->triples.n == ctx->triples.cap)
        ctx->triples.t = grow(ctx->triples.t, sizeof(Triple), &ctx->triples.cap);
    ctx->triples.t[ctx->triples.n++] = (Triple){s, p, o};
}

void kg_print(const KGContext* ctx) {
    printf("Knowledge Graph\n");
    printf("Entities: %zu\n", ctx->strings.n);
    printf("Triples : %zu\n\n", ctx->triples.n);
    for (size_t i = 0; i < ctx->triples.n; ++i) {
        Triple t = ctx->triples.t[i];

        printf("%s --[%s]--> %s\n",
            kg_str((KGContext*)ctx, t.s),
            kg_str((KGContext*)ctx, t.p),
            kg_str((KGContext*)ctx, t.o));
    }
}

void kg_free(KGContext* ctx) {
    for (size_t i = 0; i < ctx->strings.n; ++i) free(ctx->strings.s[i]);
    free(ctx->strings.s); free(ctx->triples.t);
}

int kg_load_text(KGContext* ctx, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) { perror("fopen"); return -1; }

    EntityID doc      = kg_intern(ctx, "document");
    EntityID contains = kg_intern(ctx, "contains");
    EntityID next_to  = kg_intern(ctx, "next-to");
    EntityID part_of  = kg_intern(ctx, "part-of");

    char* line = NULL;
    size_t len = 0;
    int sentence_id = 0;

    while (getline(&line, &len, f) != -1) {
        char* l = line;
        while (isspace(*l)) l++;
        if (*l == '\0' || *l == '#') continue;

        sentence_id++;
        char sent_name[32];
        snprintf(sent_name, sizeof(sent_name), "sentence-%d", sentence_id);
        EntityID sentence = kg_intern(ctx, sent_name);

        kg_add(ctx, sentence, part_of, doc);

        char* token = strtok(l, " \t\r\n.,!?;:\n");
        EntityID prev = 0;

        while (token) {
            // Skip very short tokens
            if (strlen(token) < 1) { token = strtok(NULL, " \t\r\n.,!?;:"); continue; }

            EntityID word = kg_intern(ctx, token);
            kg_add(ctx, sentence, contains, word);
            if (prev) kg_add(ctx, prev, next_to, word);
            prev = word;

            token = strtok(NULL, " \t\r\n.,!?;:");
        }
    }

    free(line);
    fclose(f);
    return 0;
}

int main(int argc, char** argv) {

    KGContext ctx;
    kg_init(&ctx);

    if (argc >= 2) {
        printf("Loading text file: %s\n\n", argv[1]);
        if (kg_load_text(&ctx, argv[1]) != 0) return 1;
    } else {
        fprintf(stderr, "Usage: kg text_file.txt\n\n");
		return 1;
    }

    kg_print(&ctx);
    kg_free(&ctx);

    return 0;
}
