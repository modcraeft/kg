#include "../include/kg.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define WINDOW_W  1400
#define WINDOW_H  900
#define NODE_R    7
#define MARGIN    80
#define ITERATIONS 900

typedef struct { float x, y; } Vec2;
static Vec2* pos = NULL;

static void* grow(void* p, size_t es, size_t* cap) {
    if (*cap == 0) *cap = 64; else *cap *= 2;
    return realloc(p, es * (*cap));
}

static void kg_init_standalone(KGContext* ctx) {
    ctx->strings.s = NULL; ctx->strings.n = ctx->strings.cap = 0;
    ctx->triples.t = NULL; ctx->triples.n = ctx->triples.cap = 0;
}

static EntityID intern(KGContext* ctx, const char* str) {
    for (size_t i = 0; i < ctx->strings.n; ++i)
        if (strcmp(ctx->strings.s[i], str) == 0) return (EntityID)(i + 1);
    if (ctx->strings.n == ctx->strings.cap)
        ctx->strings.s = grow(ctx->strings.s, sizeof(char*), &ctx->strings.cap);
    ctx->strings.s[ctx->strings.n] = strdup(str);
    return ++ctx->strings.n;
}

static void add(KGContext* ctx, EntityID s, EntityID p, EntityID o) {
    if (ctx->triples.n == ctx->triples.cap)
        ctx->triples.t = grow(ctx->triples.t, sizeof(Triple), &ctx->triples.cap);
    ctx->triples.t[ctx->triples.n++] = (Triple){s, p, o};
}

static void kg_free_standalone(KGContext* ctx) {
    for (size_t i = 0; i < ctx->strings.n; ++i) free(ctx->strings.s[i]);
    free(ctx->strings.s); free(ctx->triples.t);
}

static int load_text(KGContext* ctx, const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) { perror("fopen"); return -1; }

    EntityID doc      = intern(ctx, "document");
    EntityID contains = intern(ctx, "contains");
    EntityID next_to  = intern(ctx, "next-to");
    EntityID part_of  = intern(ctx, "part-of");

    char* line = NULL; size_t len = 0;
    int sid = 0;

    while (getline(&line, &len, f) != -1) {
        char* l = line;
        while (isspace(*l)) l++;
        if (*l == '\0' || *l == '#') continue;

        sid++;
        char name[32];
        snprintf(name, sizeof(name), "sentence-%d", sid);
        EntityID sent = intern(ctx, name);
        add(ctx, sent, part_of, doc);

        char* tok = strtok(l, " \t\r\n.,!?;:'\"()[]");
        EntityID prev = 0;
        while (tok) {
            if (strlen(tok) == 0) { tok = strtok(NULL, " \t\r\n.,!?;:'\"()[]"); continue; }
            EntityID w = intern(ctx, tok);
            add(ctx, sent, contains, w);
            if (prev) add(ctx, prev, next_to, w);
            prev = w;
            tok = strtok(NULL, " \t\r\n.,!?;:'\"()[]");
        }
    }
    free(line); fclose(f);
    return 0;
}

static void fruchterman_reingold(KGContext* kg) {
    if (kg->strings.n == 0) return;

    pos = realloc(pos, kg->strings.n * sizeof(Vec2));
    Vec2* disp = calloc(kg->strings.n, sizeof(Vec2));

    // Initial positions
    for (size_t i = 0; i < kg->strings.n; i++) {
        pos[i].x = WINDOW_W * 0.5f + (rand() % 300 - 150);
        pos[i].y = WINDOW_H * 0.5f + (rand() % 300 - 150);
    }

    float area = WINDOW_W * WINDOW_H;
    float k = sqrtf(area / (float)kg->strings.n);
    float temp = fmaxf(WINDOW_W, WINDOW_H) / 10.0f;

    for (int iter = 0; iter < ITERATIONS; iter++) {
        // Reset
        for (size_t i = 0; i < kg->strings.n; i++) disp[i] = (Vec2){0};

        // Repulsion
        for (size_t v = 0; v < kg->strings.n; v++) {
            for (size_t u = 0; u < kg->strings.n; u++) {
                if (u == v) continue;
                float dx = pos[v].x - pos[u].x;
                float dy = pos[v].y - pos[u].y;
                float dist = sqrtf(dx*dx + dy*dy) + 0.01f;
                float force = k*k / dist;
                disp[v].x += dx/dist * force;
                disp[v].y += dy/dist * force;
            }
        }

        // Attraction
        for (size_t i = 0; i < kg->triples.n; i++) {
            EntityID v = kg->triples.t[i].s - 1;
            EntityID u = kg->triples.t[i].o - 1;
            float dx = pos[v].x - pos[u].x;
            float dy = pos[v].y - pos[u].y;
            float dist = sqrtf(dx*dx + dy*dy) + 0.01f;
            float force = dist*dist / k;
            disp[v].x -= dx/dist * force;
            disp[v].y -= dy/dist * force;
            disp[u].x += dx/dist * force;
            disp[u].y += dy/dist * force;
        }

        // Apply + cool
        for (size_t v = 0; v < kg->strings.n; v++) {
            float dlen = sqrtf(disp[v].x*disp[v].x + disp[v].y*disp[v].y);
            if (dlen > 0) {
                float limit = fminf(dlen, temp);
                pos[v].x += disp[v].x / dlen * limit;
                pos[v].y += disp[v].y / dlen * limit;
            }
            pos[v].x = fmaxf(MARGIN, fminf(WINDOW_W - MARGIN, pos[v].x));
            pos[v].y = fmaxf(MARGIN, fminf(WINDOW_H - MARGIN, pos[v].y));
        }
        temp *= 0.95f;
        if (temp < 1.0f) temp = 1.0f;
    }
    free(disp);
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

    fruchterman_reingold(&kg);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
        kg_free_standalone(&kg);
        return 1;
    }

    SDL_Window* win = SDL_CreateWindow("Knowledge Graph – Fruchterman-Reingold", 0, 0, WINDOW_W, WINDOW_H, SDL_WINDOW_BORDERLESS);

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e))
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                running = 0;

        SDL_SetRenderDrawColor(ren, 18, 22, 38, 255);
        SDL_RenderClear(ren);

        // Edges
        SDL_SetRenderDrawColor(ren, 120, 180, 255, 180);
        for (size_t i = 0; i < kg.triples.n; i++) {
            Triple t = kg.triples.t[i];
            Vec2 a = pos[t.s - 1];
            Vec2 b = pos[t.o - 1];
            SDL_RenderDrawLine(ren, (int)a.x, (int)a.y, (int)b.x, (int)b.y);
        }

        // Nodes
        SDL_SetRenderDrawColor(ren, 255, 230, 120, 255);
        for (size_t i = 0; i < kg.strings.n; i++) {
            Vec2 p = pos[i];
            for (int dy = -NODE_R; dy <= NODE_R; dy++)
                for (int dx = -NODE_R; dx <= NODE_R; dx++)
                    if (dx*dx + dy*dy <= NODE_R*NODE_R)
                        SDL_RenderDrawPoint(ren, (int)p.x + dx, (int)p.y + dy);
        }

        SDL_RenderPresent(ren);
        SDL_Delay(16);
    }

    free(pos);
    kg_free_standalone(&kg);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
