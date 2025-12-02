#include "../include/kg.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define WINDOW_W 1200
#define WINDOW_H 800
#define NODE_RADIUS 7

typedef struct { float x, y; } Vec2;
static Vec2* positions = NULL;

static void* grow(void* p, size_t elem_size, size_t* cap) {
    if (*cap == 0) *cap = 64;
    else *cap *= 2;
    return realloc(p, elem_size * (*cap));
}

static void kg_init_standalone(KGContext* ctx) {
    ctx->strings.s = NULL; ctx->strings.n = ctx->strings.cap = 0;
    ctx->triples.t = NULL; ctx->triples.n = ctx->triples.cap = 0;
}

static EntityID intern(KGContext* ctx, const char* str) {
    for (size_t i = 0; i < ctx->strings.n; ++i)
        if (strcmp(ctx->strings.s[i], str) == 0)
            return (EntityID)(i + 1);
    if (ctx->strings.n == ctx->strings.cap)
        ctx->strings.s = grow(ctx->strings.s, sizeof(char*), &ctx->strings.cap);
    ctx->strings.s[ctx->strings.n] = strdup(str);
    return ++ctx->strings.n;
}

static void add_triple(KGContext* ctx, EntityID s, EntityID p, EntityID o) {
    if (ctx->triples.n == ctx->triples.cap)
        ctx->triples.t = grow(ctx->triples.t, sizeof(Triple), &ctx->triples.cap);
    ctx->triples.t[ctx->triples.n++] = (Triple){s, p, o};
}

static void kg_free_standalone(KGContext* ctx) {
    for (size_t i = 0; i < ctx->strings.n; ++i) free(ctx->strings.s[i]);
    free(ctx->strings.s);
    free(ctx->triples.t);
}

static int load_text(KGContext* ctx, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) { perror("fopen"); return -1; }

    EntityID doc      = intern(ctx, "document");
    EntityID contains = intern(ctx, "contains");
    EntityID next_to  = intern(ctx, "next-to");
    EntityID part_of  = intern(ctx, "part-of");

    char* line = NULL; size_t len = 0;
    int sentence_id = 0;

    while (getline(&line, &len, f) != -1) {
        char* l = line;
        while (isspace(*l)) l++;
        if (*l == '\0' || *l == '#') continue;

        sentence_id++;
        char sname[32];
        snprintf(sname, sizeof(sname), "sentence-%d", sentence_id);
        EntityID sent = intern(ctx, sname);
        add_triple(ctx, sent, part_of, doc);

        char* tok = strtok(l, " \t\r\n.,!?;:'\"()");
        EntityID prev = 0;
        while (tok) {
            if (strlen(tok) == 0) { tok = strtok(NULL, " \t\r\n.,!?;:'\"()"); continue; }
            EntityID word = intern(ctx, tok);
            add_triple(ctx, sent, contains, word);
            if (prev) add_triple(ctx, prev, next_to, word);
            prev = word;
            tok = strtok(NULL, " \t\r\n.,!?;:'\"()");
        }
    }
    free(line); fclose(f);
    return 0;
}

static void layout_circle(KGContext* ctx) {
    positions = realloc(positions, ctx->strings.n * sizeof(Vec2));
    float cx = WINDOW_W / 2.0f;
    float cy = WINDOW_H / 2.0f;
    float radius = fminf(WINDOW_W, WINDOW_H) * 0.35f;

    for (size_t i = 0; i < ctx->strings.n; ++i) {
        float angle = 2.0f * M_PI * i / (ctx->strings.n ? ctx->strings.n : 1);
        positions[i].x = cx + radius * cosf(angle);
        positions[i].y = cy + radius * sinf(angle);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <textfile>\n", argv[0]);
        return 1;
    }

    KGContext kg;
    kg_init_standalone(&kg);

    if (load_text(&kg, argv[1]) != 0) {
        kg_free_standalone(&kg);
        return 1;
    }

    layout_circle(&kg);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        kg_free_standalone(&kg);
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Knowledge Graph", 0, 0, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;
        }

        SDL_SetRenderDrawColor(ren, 20, 20, 40, 255);
        SDL_RenderClear(ren);

        //Draw edges
        SDL_SetRenderDrawColor(ren, 100, 180, 255, 200);
        for (size_t i = 0; i < kg.triples.n; ++i) {
            Triple t = kg.triples.t[i];
            if (t.s == 0 || t.o == 0) continue;
            Vec2 a = positions[t.s - 1];
            Vec2 b = positions[t.o - 1];
            SDL_RenderDrawLine(ren, (int)a.x, (int)a.y, (int)b.x, (int)b.y);
        }

        //Draw nodes
        SDL_SetRenderDrawColor(ren, 255, 230, 100, 255);
        for (size_t i = 0; i < kg.strings.n; ++i) {
            Vec2 p = positions[i];
            for (int dy = -NODE_RADIUS; dy <= NODE_RADIUS; ++dy) {
                for (int dx = -NODE_RADIUS; dx <= NODE_RADIUS; ++dx) {
                    if (dx*dx + dy*dy <= NODE_RADIUS*NODE_RADIUS)
                        SDL_RenderDrawPoint(ren, (int)p.x + dx, (int)p.y + dy);
                }
            }
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    free(positions);
    kg_free_standalone(&kg);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
