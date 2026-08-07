// ============================================================================
//  GLUT 2D Platformer Game Scene - Infinite Scroller
//
//  A pixel-art style platformer backdrop rendered with legacy OpenGL / GLUT.
//  The whole scene is authored against a 735 x 412 reference frame and mapped
//  onto a fixed 1200 x 700 virtual canvas, so the composition never changes
//  when the window is resized.
//
//  The composed scene (ground, stepped platforms, bushes, crates) scrolls
//  left forever, exactly like the ground/obstacles in the Chrome dinosaur
//  game, by repeating the same 735-unit-wide tile side by side and sliding
//  a "camera" offset across it. The cloud layer scrolls too, but slower
//  (parallax), so the sky reads as further away than the terrain.
//
//  Press ESC to quit, SPACE (or P) to pause/resume the scroll.
//  Run with:  ./game_macos --dump out.ppm  to render a single frame (at the
//  very start of the scroll) to a Portable Pixmap file, for visual
//  regression checks against the reference image.
// ============================================================================

#ifdef __APPLE__
    #include <GLUT/glut.h>
    #include <OpenGL/gl.h>
#else
    #include <GL/glut.h>
    #include <GL/gl.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== CANVAS ====================

// Virtual drawing canvas. Every coordinate below is expressed in the
// reference-art space (735 x 412) and converted with IX/IY/IW/IH.
static const float CANVAS_W = 1200.0f;
static const float CANVAS_H = 700.0f;
static const float ART_W    = 735.0f;
static const float ART_H    = 412.0f;
static const float SX       = CANVAS_W / ART_W;   // 1.6327
static const float SY       = CANVAS_H / ART_H;   // 1.6990

int windowWidth  = 1200;
int windowHeight = 700;

static const char* dumpPath = NULL;   // --dump <file> writes one frame and exits

// ==================== SCROLL STATE ====================
//
// The whole hand-authored scene is one 735-unit-wide "tile" in art space.
// scrollX is how far the world has slid left, in art-space units, since the
// program started. To draw the world at a given moment we place several
// copies of the tile side by side at world offsets ...,-735,0,735,1470,...
// and subtract scrollX from every coordinate before scaling to canvas space
// - so content continuously slides off the left edge while an identical
// copy slides in from the right, forever.
//
// currentScrollX / tileOriginX are set by display() right before each group
// of draw calls (clouds get a slower "parallax" scrollX than the terrain),
// so none of the drawing functions below need to know scrolling exists.
static double scrollX  = 0.0;     // ground-layer world offset, art units
static double speed    = 130.0;   // current scroll speed, art units / second
static const double BASE_SPEED  = 130.0;
static const double MAX_SPEED   = 320.0;
static const double ACCEL       = 2.0;      // speed gained per second, dino-game style
static const float  CLOUD_PARALLAX = 0.35f; // clouds drift slower than the ground
static const float  TILE_W = ART_W;         // one repeating tile = the full art frame

static float currentScrollX = 0.0f;   // active scroll offset for the layer being drawn
static float tileOriginX    = 0.0f;   // world-space x of the tile currently being drawn

static bool paused    = false;
static int  lastTimeMs = 0;

// art-space -> canvas-space helpers
static inline float IX(float x) { return (x + tileOriginX - currentScrollX) * SX; }
static inline float IY(float y) { return (ART_H - y) * SY; }   // y flips
static inline float IW(float w) { return w * SX; }
static inline float IH(float h) { return h * SY; }

// ==================== COLOR DEFINITIONS ====================

void setColor(float r, float g, float b) { glColor3f(r, g, b); }
void setColorA(float r, float g, float b, float a) { glColor4f(r, g, b, a); }

// -- sky ---------------------------------------------------------------------
static const float SKY_TOP[3]     = {0.255f, 0.643f, 0.969f};  // #41A4F7
static const float SKY_BOTTOM[3]  = {0.447f, 0.922f, 0.988f};  // #72EBFC
static const float HAZE[3]        = {0.560f, 1.000f, 1.000f};  // lightening wash
static const float CLOUD_GREY[3]  = {0.820f, 0.914f, 0.929f};  // #D1E9ED
static const float CLOUD_CREAM[3] = {0.961f, 0.996f, 0.933f};  // #F5FEEE
static const float CLOUD_WHITE[3] = {0.996f, 0.996f, 0.996f};  // #FEFEFE
static const float CLOUD_SHADE[3] = {0.855f, 0.965f, 0.976f};  // flat-bottom band
static const float SPECK[3]       = {0.596f, 0.882f, 0.957f};  // pale sky specks

// -- ground ------------------------------------------------------------------
static const float OUTLINE[3]     = {0.055f, 0.180f, 0.208f};  // #0E2E35
static const float GRASS_LIGHT[3] = {0.702f, 0.886f, 0.478f};  // #B3E27A
static const float GRASS_MID[3]   = {0.376f, 0.631f, 0.506f};  // #60A181
static const float GRASS_DARK[3]  = {0.278f, 0.553f, 0.596f};  // #478D98
static const float GRASS_TUFT[3]  = {0.310f, 0.470f, 0.290f};  // little "v" marks

// -- masonry -----------------------------------------------------------------
static const float BRICK_CAP[3]    = {0.827f, 0.459f, 0.431f}; // #D3756E
static const float BRICK_LIGHT[3]  = {0.937f, 0.537f, 0.439f}; // #EF8970
static const float BRICK_MID[3]    = {0.882f, 0.502f, 0.451f}; // #E18073
static const float BRICK_ACCENT[3] = {0.788f, 0.435f, 0.451f}; // odd darker brick
static const float BRICK_MORTAR[3] = {0.745f, 0.365f, 0.329f}; // #BE5D54
static const float BRICK_HILITE[3] = {1.000f, 0.702f, 0.498f}; // #FFB37F
static const float SHADOW[4]       = {0.196f, 0.188f, 0.600f, 0.45f};

// -- foliage -----------------------------------------------------------------
static const float BUSH_LIGHT[3] = {0.698f, 0.863f, 0.471f};
static const float BUSH_MID[3]   = {0.443f, 0.702f, 0.435f};
static const float BUSH_DARK[3]  = {0.278f, 0.553f, 0.596f};

// -- blocks ------------------------------------------------------------------
static const float BLOCK_OUTLINE[3] = {0.176f, 0.137f, 0.310f}; // deep indigo
static const float BLOCK_TOP[3]     = {1.000f, 0.933f, 0.604f}; // pale yellow lip
static const float BLOCK_FILL[3]    = {1.000f, 0.675f, 0.451f}; // #FFAC73
static const float BLOCK_FILL_LO[3] = {0.973f, 0.616f, 0.443f}; // #F89D71
static const float BLOCK_GLOSS[3]   = {1.000f, 0.812f, 0.663f};
static const float BLOCK_SHADE[3]   = {0.855f, 0.494f, 0.475f};

static void use(const float c[3])            { glColor3f(c[0], c[1], c[2]); }
static void useA(const float c[3], float a)  { glColor4f(c[0], c[1], c[2], a); }

// ==================== DRAWING PRIMITIVES ====================

// Deterministic hash -> [0,1).  Keeps the "hand drawn" jitter stable per frame.
static float hash01(int a, int b) {
    unsigned int h = (unsigned int)(a * 374761393 + b * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFFu) / 65536.0f;
}

// Draw a solid rectangle (canvas coordinates)
void drawFilledRect(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();
}

// Draw a rectangle outline (canvas coordinates)
void drawRectOutline(float x, float y, float width, float height) {
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + width, y);
        glVertex2f(x + width, y + height);
        glVertex2f(x, y + height);
    glEnd();
}

// Draw a triangle (canvas coordinates)
void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3) {
    glBegin(GL_TRIANGLES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glVertex2f(x3, y3);
    glEnd();
}

// Draw a circle (canvas coordinates)
void drawCircle(float x, float y, float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        for (int i = 0; i <= segments; i++) {
            float a = 2.0f * 3.14159265f * i / segments;
            glVertex2f(x + radius * cosf(a), y + radius * sinf(a));
        }
    glEnd();
}

// Draw an ellipse (canvas coordinates) - circles in art space become ellipses
// on the canvas because the horizontal and vertical scales differ slightly.
void drawEllipse(float cx, float cy, float rx, float ry, int segments) {
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);
        for (int i = 0; i <= segments; i++) {
            float a = 2.0f * 3.14159265f * i / segments;
            glVertex2f(cx + rx * cosf(a), cy + ry * sinf(a));
        }
    glEnd();
}

// --- art-space convenience wrappers -----------------------------------------

// Rectangle given in art space by its TOP-left corner and size.
void rectI(float x, float y, float w, float h) {
    drawFilledRect(IX(x), IY(y + h), IW(w), IH(h));
}

// Circle given in art space (radius in art pixels).
void blobI(float cx, float cy, float r) {
    drawEllipse(IX(cx), IY(cy), IW(r), IH(r), 28);
}

// Filled polygon from an art-space point list {x0,y0, x1,y1, ...}
void polyI(const float* pts, int count) {
    glBegin(GL_TRIANGLE_FAN);
        for (int i = 0; i < count; i++) glVertex2f(IX(pts[i * 2]), IY(pts[i * 2 + 1]));
    glEnd();
}

// Thick line strip through an art-space point list.
void strokeI(const float* pts, int count, float widthArt) {
    glLineWidth(widthArt * SX);
    glBegin(GL_LINE_STRIP);
        for (int i = 0; i < count; i++) glVertex2f(IX(pts[i * 2]), IY(pts[i * 2 + 1]));
    glEnd();
    glLineWidth(1.0f);
}

// Rounded rectangle in art space (top-left origin).
void roundRectI(float x, float y, float w, float h, float r) {
    const int SEG = 5;
    glBegin(GL_TRIANGLE_FAN);
        glVertex2f(IX(x + w * 0.5f), IY(y + h * 0.5f));
        // corners: TL, TR, BR, BL
        float cx[4] = {x + r, x + w - r, x + w - r, x + r};
        float cy[4] = {y + r, y + r, y + h - r, y + h - r};
        float a0[4] = {180.0f, 270.0f, 0.0f, 90.0f};
        for (int c = 0; c < 4; c++) {
            for (int s = 0; s <= SEG; s++) {
                float a = (a0[c] + 90.0f * s / SEG) * 3.14159265f / 180.0f;
                glVertex2f(IX(cx[c] + r * cosf(a)), IY(cy[c] + r * sinf(a)));
            }
        }
        glVertex2f(IX(x), IY(y + r));   // close the fan on the first corner point
    glEnd();
}

// Outline of a rounded rectangle, drawn as a ring of quads so the stroke has a
// real art-space thickness instead of depending on GL line width limits.
void roundRectOutlineI(float x, float y, float w, float h, float r, float t) {
    rectI(x, y, w, t);                 // top
    rectI(x, y + h - t, w, t);         // bottom
    rectI(x, y, t, h);                 // left
    rectI(x + w - t, y, t, h);         // right
}

// Legacy helper kept from the original scene: a sine "grass" line.
void drawWavyGrass(float startX, float startY, float length, float amplitude) {
    glBegin(GL_LINE_STRIP);
        for (float x = startX; x < startX + length; x += 5)
            glVertex2f(x, startY + amplitude * sinf((x - startX) * 0.05f));
    glEnd();
}

// ==================== SKY ====================

void drawSky() {
    glBegin(GL_QUADS);
        glColor3fv(SKY_BOTTOM); glVertex2f(0.0f,     0.0f);
        glColor3fv(SKY_BOTTOM); glVertex2f(CANVAS_W, 0.0f);
        glColor3fv(SKY_TOP);    glVertex2f(CANVAS_W, CANVAS_H);
        glColor3fv(SKY_TOP);    glVertex2f(0.0f,     CANVAS_H);
    glEnd();
}

// A billowing cloud silhouette: a run of overlapping lobes sitting on a base
// line, which is how every cloud mass in the reference art is built.
struct Lobe { float x, y, r; };

static void drawLobes(const Lobe* l, int n, float baseY) {
    for (int i = 0; i < n; i++) blobI(l[i].x, l[i].y, l[i].r);
    // fill the body between the lobes and the base line
    for (int i = 0; i < n; i++) {
        if (l[i].y < baseY) rectI(l[i].x - l[i].r, l[i].y, l[i].r * 2.0f, baseY - l[i].y);
    }
}

// Pale wash clouds sitting just above the sky gradient.
//
// NOTE: these are drawn as flat opaque fills, not alpha blended. Overlapping
// alpha-blended lobes of the same colour re-blend at every overlap, which
// leaves visible ring-shaped seams where circles cross - exactly the "bubbly"
// look we don't want. A solid fill has no such seams: painting the same
// colour twice is indistinguishable from painting it once.
void drawHazeClouds() {
    use(HAZE);

    // upper-left field - a few big, closely-spaced lobes so the silhouette
    // rolls smoothly instead of reading as a row of separate bumps
    static const Lobe a[] = {
        {-30, 95, 62}, {32, 78, 58}, {90, 100, 46}, {8, 148, 56}, {66, 152, 48}
    };
    drawLobes(a, 5, 320);

    // cumulus rising on the left of centre
    static const Lobe b[] = {
        {158, 205, 44}, {200, 190, 46}, {234, 210, 40}, {200, 240, 42}
    };
    drawLobes(b, 4, 320);

    // right-hand field, behind the big cream mass
    static const Lobe c[] = {
        {615, 62, 42}, {655, 40, 42}, {693, 62, 38}, {665, 96, 34}
    };
    drawLobes(c, 4, 150);

    // low bank behind the stepped platforms
    static const Lobe d[] = {
        {474, 240, 38}, {512, 222, 38}, {548, 240, 32}, {450, 262, 32}
    };
    drawLobes(d, 4, 320);
}

// Muted blue-grey puffs that read as cloud in shadow.
void drawShadowClouds() {
    use(CLOUD_GREY);
    static const Lobe a[] = {
        {8, 232, 34}, {40, 246, 30}
    };
    drawLobes(a, 2, 312);
    static const Lobe b[] = {
        {102, 250, 30}, {134, 242, 30}, {158, 258, 26}, {110, 270, 26}
    };
    drawLobes(b, 4, 312);
    static const Lobe c[] = {
        {350, 268, 24}, {382, 260, 26}, {412, 268, 22}
    };
    drawLobes(c, 3, 312);
    static const Lobe d[] = {
        {650, 72, 24}, {672, 58, 26}, {690, 82, 22}
    };
    drawLobes(d, 3, 132);
}

// The big soft cream cumulus banks that sit on the horizon.
void drawCreamClouds() {
    use(CLOUD_CREAM);

    // tall column of cloud on the far left
    static const Lobe a[] = {
        {38, 128, 34}, {76, 122, 30}, {6, 158, 36}, {50, 162, 36},
        {88, 176, 28}, {14, 198, 36}, {58, 206, 34}, {8, 238, 36},
        {50, 248, 34}, {14, 278, 34}, {54, 286, 30}
    };
    drawLobes(a, 11, 312);

    // rounded bank tucked in front of it
    static const Lobe b[] = {
        {126, 246, 26}, {156, 232, 27}, {184, 246, 24}, {138, 274, 26}
    };
    drawLobes(b, 4, 312);

    // bank behind the stepped platforms
    static const Lobe c[] = {
        {500, 262, 24}, {528, 248, 26}, {554, 262, 22}, {490, 282, 22}
    };
    drawLobes(c, 4, 312);

    // towering mass in the top-right corner
    static const Lobe d[] = {
        {704, 22, 36}, {735, 8, 40}, {680, 56, 30}, {712, 68, 32}, {690, 100, 26}
    };
    drawLobes(d, 5, 132);

    // small puff to its left
    static const Lobe e[] = {
        {616, 108, 18}, {638, 98, 20}
    };
    drawLobes(e, 2, 130);
}

// The single crisp, flat-bottomed cloud in the middle of the sky.
void drawMainCloud() {
    use(CLOUD_WHITE);
    static const Lobe l[] = {
        {312, 90, 15}, {338, 84, 17}, {372, 74, 24},
        {404, 77, 21}, {435, 88, 15}, {455, 92, 10}
    };
    drawLobes(l, 6, 98);
    rectI(299, 88, 164, 10);
    // soft shelf under the flat bottom
    use(CLOUD_SHADE);
    rectI(301, 98, 160, 4);
}

// Petals, dashes and drifting motes.
static void flowerI(float cx, float cy, float r) {
    blobI(cx, cy, r * 0.52f);
    for (int i = 0; i < 5; i++) {
        float a = (i * 72.0f + 18.0f) * 3.14159265f / 180.0f;
        blobI(cx + cosf(a) * r * 0.62f, cy + sinf(a) * r * 0.62f, r * 0.42f);
    }
}

static void dashI(float x, float y, float len, float thick) {
    // short diagonal tick with rounded ends
    float dx = len * 0.62f, dy = -len * 0.78f;
    blobI(x, y, thick * 0.5f);
    blobI(x + dx, y + dy, thick * 0.5f);
    float px = -dy, py = dx;
    float n = sqrtf(px * px + py * py);
    px = px / n * thick * 0.5f;  py = py / n * thick * 0.5f;
    float p[8] = { x + px, y + py, x + dx + px, y + dy + py,
                   x + dx - px, y + dy - py, x - px, y - py };
    polyI(p, 4);
}

void drawSkyParticles() {
    use(CLOUD_WHITE);
    flowerI(57, 47, 8);
    flowerI(122, 126, 11);
    flowerI(580, 82, 12);

    use(CLOUD_SHADE);
    dashI(26, 88, 13, 4.5f);
    dashI(165, 100, 11, 4.0f);
    dashI(192, 80, 10, 3.6f);
    dashI(549, 200, 10, 3.6f);
    dashI(586, 133, 9,  3.2f);
    dashI(597, 216, 9,  3.2f);
    dashI(713, 88, 9,  3.2f);
    dashI(716, 110, 9, 3.2f);

    // pale drifting motes
    use(SPECK);
    roundRectI(464, 37, 19, 18, 6);
    roundRectI(446, 160, 16, 15, 5);
    roundRectI(269, 166, 12, 12, 4);
    roundRectI(287, 167, 12, 12, 4);
}

void drawClouds() {
    drawHazeClouds();
    drawShadowClouds();
    drawCreamClouds();
    drawMainCloud();
    drawSkyParticles();
}

// ==================== TERRAIN ====================

// One block of land: a grass cap over a brick face.  The main ground and both
// stepped platforms on the right are all built from this.
struct Slab {
    float x0, x1;      // art-space left / right edge
    float top;         // art-space y of the grass surface
    float bottom;      // art-space y where the brick face ends
    float grassH;      // depth of the clean bright grass band
    float seamH;       // distance from the surface down to the wavy seam
    float capH;        // depth of the dark soil band under the seam
    int   edges;       // 1 = draw left edge, 2 = draw right edge
    const float* fins; // art-space x positions of the grass fins on the rim
    int   finCount;
    int   seed;
};

static const float MAIN_FINS[] = {
    136, 150, 176, 191, 262, 297, 382, 440, 462, 492, 567, 592, 648, 706
};
static const float LEDGE_FINS[]  = { 549, 599 };
static const float TOWER_FINS[]  = { 623, 662, 726 };

// LEDGE.x1 and TOWER.x0 meet exactly at the same seam, and only TOWER draws
// an edge stroke there - so the two slabs read as one continuous stepped
// platform instead of leaving a stray outline floating inside the ledge cap.
static const Slab GROUND = { 0,   740, 306, 412, 20, 41, 17, 0, MAIN_FINS,  14, 3 };
static const Slab LEDGE  = { 546, 623, 216, 306, 16, 30, 10, 1, LEDGE_FINS,  2, 11 };
static const Slab TOWER  = { 623, 740, 126, 306, 20, 38,  8, 1, TOWER_FINS,  3, 23 };

static float wavyY(const Slab& s, float x) {
    return s.top + s.seamH + 3.0f * sinf(x * 0.30f) + 1.2f * sinf(x * 0.11f);
}

// --- brick face -------------------------------------------------------------

static void drawBrickFace(const Slab& s) {
    float capTop = s.top + s.seamH + 2.0f;
    float y      = capTop + s.capH;              // first course
    const float COURSE = 21.5f;

    use(BRICK_LIGHT);
    rectI(s.x0, y, s.x1 - s.x0, s.bottom - y);

    int row = 0;
    for (; y < s.bottom; y += COURSE, row++) {
        float h = COURSE;
        if (y + h > s.bottom) h = s.bottom - y;

        // alternating course tone
        use(row % 2 ? BRICK_MID : BRICK_LIGHT);
        rectI(s.x0, y, s.x1 - s.x0, h);

        // top highlight of the course
        use(BRICK_HILITE);
        rectI(s.x0, y, s.x1 - s.x0, 2.0f);

        // bed joint below the course
        if (y + COURSE < s.bottom) {
            use(BRICK_MORTAR);
            rectI(s.x0, y + COURSE - 2.0f, s.x1 - s.x0, 2.0f);
        }

        // irregular perpends, offset course by course
        static const float WIDTHS[10] = {58, 44, 96, 62, 38, 78, 52, 88, 46, 68};
        float x = s.x0 - hash01(s.seed, row) * 70.0f;
        int i = 0;
        while (x < s.x1) {
            float w = WIDTHS[(row * 3 + i + s.seed) % 10];
            if (hash01(s.seed + 91, row * 37 + i) > 0.86f) {
                use(BRICK_ACCENT);                       // odd darker brick
                float bx = x > s.x0 ? x : s.x0;
                float bw = (x + w < s.x1 ? x + w : s.x1) - bx;
                if (bw > 0) rectI(bx, y + 2.0f, bw, h - 4.0f);
            }
            x += w;
            if (x > s.x0 && x < s.x1 && y + 2.0f < s.bottom) {
                use(BRICK_MORTAR);
                rectI(x, y + 2.0f, 2.0f, (y + h < s.bottom ? h : s.bottom - y) - 2.0f);
            }
            i++;
        }
    }
}

// The large diagonal shade wash across the right half of the main ground.
static void drawGroundShade() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4fv(SHADOW);
    static const float p[] = { 452, 344, 760, 344, 760, 414, 372, 414 };
    polyI(p, 4);
    glDisable(GL_BLEND);
}

// --- grass cap --------------------------------------------------------------

// A solid band of turf with a fringe of wispy blades standing up out of it.
// baseY is where the blades are rooted, tipY how far the tallest ones reach.
static void drawBladeBand(const Slab& s, float baseY, float botY, float tipY, int seed) {
    rectI(s.x0, baseY, s.x1 - s.x0, botY - baseY);

    float reach = baseY - tipY;
    glBegin(GL_TRIANGLES);
        for (float x = s.x0 - 3.0f; x < s.x1 + 3.0f; x += 2.5f) {
            int k = (int)floorf(x / 2.5f);
            float t = hash01(seed, k);
            float h = reach * (0.22f + 0.78f * t);
            float w = 1.7f + 2.3f * hash01(seed + 17, k);
            float lean = (hash01(seed + 31, k) - 0.5f) * 3.5f;
            glVertex2f(IX(x - w),        IY(baseY + 1.5f));
            glVertex2f(IX(x + w),        IY(baseY + 1.5f));
            glVertex2f(IX(x + lean),     IY(baseY - h));
        }
    glEnd();
}

// Build the rim silhouette (flat surface + upright grass fins) once, so the
// fill and the outline stroke share exactly the same profile.
static int buildRim(const Slab& s, float* out, int maxPts) {
    int n = 0;
    out[n * 2] = s.x0; out[n * 2 + 1] = s.top; n++;
    for (int i = 0; i < s.finCount && n + 4 < maxPts; i++) {
        float fx = s.fins[i];
        if (fx < s.x0 - 2 || fx > s.x1) continue;
        float w = 11.0f + hash01(s.seed, i) * 6.0f;
        float h = 9.0f  + hash01(s.seed + 5, i) * 6.0f;
        out[n * 2] = fx;              out[n * 2 + 1] = s.top;            n++;
        out[n * 2] = fx + 2.0f;       out[n * 2 + 1] = s.top - h;        n++;
        out[n * 2] = fx + w * 0.48f;  out[n * 2 + 1] = s.top - h * 0.5f; n++;
        out[n * 2] = fx + w;          out[n * 2 + 1] = s.top;            n++;
    }
    out[n * 2] = s.x1; out[n * 2 + 1] = s.top; n++;
    return n;
}

static void drawGrassCap(const Slab& s) {
    float rim[128 * 2];
    int rn = buildRim(s, rim, 128);

    // bright grass surface, following the rim profile.  It is filled all the
    // way down to the seam so the blade layers never leave a gap.
    use(GRASS_LIGHT);
    float fillY = s.top + s.seamH + 6.0f;
    glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i < rn; i++) {
            glVertex2f(IX(rim[i * 2]), IY(rim[i * 2 + 1]));
            glVertex2f(IX(rim[i * 2]), IY(fillY));
        }
    glEnd();

    // two layers of turf, each fringed with blades that push up into the one above
    float under = s.seamH - s.grassH;          // depth between grass and seam
    use(GRASS_MID);
    drawBladeBand(s, s.top + s.grassH + under * 0.52f,
                     s.top + s.seamH + 5.0f,
                     s.top + s.grassH * 0.92f, s.seed + 1);
    use(GRASS_DARK);
    drawBladeBand(s, s.top + s.grassH + under * 0.78f,
                     s.top + s.seamH + 5.0f,
                     s.top + s.grassH + under * 0.24f, s.seed + 2);

    // dark soil band, its top following the wavy seam
    float capTop = s.top + s.seamH + 2.0f;
    use(BRICK_CAP);
    glBegin(GL_TRIANGLE_STRIP);
        for (float x = s.x0; x <= s.x1 + 4.0f; x += 4.0f) {
            glVertex2f(IX(x), IY(wavyY(s, x)));
            glVertex2f(IX(x), IY(capTop + s.capH));
        }
    glEnd();

    // the wavy seam itself
    use(OUTLINE);
    glBegin(GL_TRIANGLE_STRIP);
        for (float x = s.x0; x <= s.x1 + 4.0f; x += 4.0f) {
            glVertex2f(IX(x), IY(wavyY(s, x) - 2.0f));
            glVertex2f(IX(x), IY(wavyY(s, x) + 2.0f));
        }
    glEnd();

    // little "v" tufts scattered over the surface
    use(GRASS_TUFT);
    glLineWidth(1.5f * SX);
    for (float x = s.x0 + 24.0f; x < s.x1 - 14.0f; x += 52.0f) {
        if (hash01(s.seed + 7, (int)x) < 0.40f) continue;
        float ty = s.top + 7.0f + hash01(s.seed + 8, (int)x) * 4.0f;
        float w  = 10.0f + hash01(s.seed + 9, (int)x) * 5.0f;
        glBegin(GL_LINE_STRIP);
            glVertex2f(IX(x - w),        IY(ty));
            glVertex2f(IX(x - w * 0.5f), IY(ty + 4.0f));
            glVertex2f(IX(x),            IY(ty + 0.5f));
            glVertex2f(IX(x + w * 0.5f), IY(ty + 4.5f));
            glVertex2f(IX(x + w),        IY(ty + 1.0f));
        glEnd();
    }
    glLineWidth(1.0f);

    // rim stroke, on top of everything
    use(OUTLINE);
    strokeI(rim, rn, 2.2f);
}

static void drawSlabEdges(const Slab& s) {
    use(OUTLINE);
    if (s.edges & 1) rectI(s.x0, s.top, 2.4f, s.bottom - s.top);
    if (s.edges & 2) rectI(s.x1 - 2.4f, s.top, 2.4f, s.bottom - s.top);
}

void drawBrickLayer() {
    drawBrickFace(GROUND);
    drawGroundShade();
}

void drawPlatforms() {
    // stepped platforms on the right, back to front
    drawBrickFace(LEDGE);
    drawBrickFace(TOWER);
    drawGrassCap(LEDGE);
    drawSlabEdges(LEDGE);
    drawGrassCap(TOWER);
    drawSlabEdges(TOWER);
    // main ground rim sits in front of both
    drawGrassCap(GROUND);
}

// Kept for API compatibility with the original scene description.
void drawUnderground() { /* the brick face now runs to the bottom of the frame */ }

// ==================== BUSHES ====================

struct BushLobe { float dx, dy, r; int tone; };  // 0 light, 1 mid, 2 dark

// A low, wide scalloped clump: light lobes up and to the left, deep teal lobes
// crowding the lower right, exactly as the reference art shades them.
static const float BUSH_W = 90.0f;
static const BushLobe BUSH_SHAPE[] = {
    { 10, 27, 12, 0}, { 28, 19, 14, 0}, { 48, 16, 15, 0}, { 68, 21, 13, 0},
    { 84, 29, 11, 0}, { 17, 36, 13, 0}, { 38, 31, 14, 0}, { 58, 30, 14, 1},
    { 76, 35, 12, 1}, { 30, 42, 13, 1}, { 52, 41, 14, 2}, { 71, 43, 12, 2},
    {  6, 40, 10, 1}, { 88, 41,  9, 2}
};

// One bush: silhouette stroke first, then the tonal lobes on top.
// shade pushes every lobe one tone darker, for the clumps in deep shadow.
static void drawBush(float x, float y, float scale, int flip, int shade = 0) {
    const int N = (int)(sizeof(BUSH_SHAPE) / sizeof(BUSH_SHAPE[0]));

    // outline pass - the same lobes, fattened
    use(OUTLINE);
    for (int i = 0; i < N; i++) {
        float dx = flip ? (BUSH_W - BUSH_SHAPE[i].dx) : BUSH_SHAPE[i].dx;
        blobI(x + dx * scale, y + BUSH_SHAPE[i].dy * scale,
              BUSH_SHAPE[i].r * scale + 2.0f);
    }
    // tonal pass, dark lobes last so they sit in front
    for (int pass = 0; pass < 3; pass++) {
        use(pass == 0 ? BUSH_LIGHT : pass == 1 ? BUSH_MID : BUSH_DARK);
        for (int i = 0; i < N; i++) {
            int tone = BUSH_SHAPE[i].tone + shade;
            if (tone > 2) tone = 2;
            if (tone != pass) continue;
            float dx = flip ? (BUSH_W - BUSH_SHAPE[i].dx) : BUSH_SHAPE[i].dx;
            blobI(x + dx * scale, y + BUSH_SHAPE[i].dy * scale, BUSH_SHAPE[i].r * scale);
        }
    }
}

void drawBushes() {
    drawBush(163, 258, 0.92f, 0);      // on the ground, left of centre
    drawBush(482, 262, 0.80f, 1);      // tucked against the lower platform
    drawBush(650, 240, 0.98f, 1, 1);   // beside the tall platform, in shade
    drawBush(-42, 336, 1.05f, 0);      // bottom-left corner, over the bricks
    drawBush(686, 352, 0.95f, 1, 1);   // bottom-right corner, in shade
}

// ==================== BLOCKS ====================

// The classic rivetted crate.  Three of them appear in the scene at slightly
// different sizes, so everything is parameterised on the art-space rectangle.
static void drawCrate(float x, float y, float w, float h, int gloss) {
    float r = 5.0f, t = 3.0f;

    use(BLOCK_OUTLINE);
    roundRectI(x, y, w, h, r);

    use(BLOCK_FILL);
    roundRectI(x + t, y + t, w - 2 * t, h - 2 * t, r * 0.7f);

    // pale lip along the top and a warmer floor
    use(BLOCK_TOP);
    rectI(x + t + 2.0f, y + t, w - 2 * t - 4.0f, 3.0f);
    use(BLOCK_FILL_LO);
    rectI(x + t, y + h - t - 6.0f, w - 2 * t, 6.0f);

    if (gloss) {
        // glossy crescent sweeping across the upper half - a thick arc built
        // from an inner and outer curve, not a straight bar
        use(BLOCK_GLOSS);
        float cx = x + w * 0.5f, cy = y + h * 0.24f;
        float ro = w * 0.40f, ri = ro - h * 0.075f;
        glBegin(GL_TRIANGLE_STRIP);
            for (int i = 0; i <= 12; i++) {
                float a = (195.0f - 210.0f * i / 12.0f) * 3.14159265f / 180.0f;
                glVertex2f(IX(cx + ro * cosf(a)), IY(cy + ro * sinf(a) * 0.40f));
                glVertex2f(IX(cx + ri * cosf(a)), IY(cy + ri * sinf(a) * 0.40f));
            }
        glEnd();
        use(BLOCK_SHADE);
        rectI(x + w * 0.28f, y + h * 0.74f, w * 0.44f, 3.0f);
    } else {
        // faint inner panel
        use(BLOCK_FILL_LO);
        rectI(x + t + 3.0f, y + t + 3.0f, 2.0f, h - 2 * t - 6.0f);
        rectI(x + w - t - 5.0f, y + t + 3.0f, 2.0f, h - 2 * t - 6.0f);
    }

    // corner rivets
    use(BLOCK_OUTLINE);
    float inset = 5.0f;
    roundRectI(x + t + inset,             y + t + inset,             4.0f, 3.2f, 1.2f);
    roundRectI(x + w - t - inset - 4.0f,  y + t + inset,             4.0f, 3.2f, 1.2f);
    roundRectI(x + t + inset,             y + h - t - inset - 3.2f,  4.0f, 3.2f, 1.2f);
    roundRectI(x + w - t - inset - 4.0f,  y + h - t - inset - 3.2f,  4.0f, 3.2f, 1.2f);
}

void drawFloatingBlocks() {
    drawCrate(130, 30, 50, 50, 1);      // hanging in the sky, upper left
}

void drawSmallBlocks() {
    drawCrate(47, 265, 46, 42, 1);      // resting on the ground
    drawCrate(679, 86, 39, 41, 0);      // resting on the tall platform
}

// ==================== DISPLAY ====================

static void writePPM(const char* path) {
    int vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    int w = vp[2], h = vp[3];
    unsigned char* buf = (unsigned char*)malloc((size_t)w * h * 3);
    if (!buf) return;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, buf);
    FILE* f = fopen(path, "wb");
    if (f) {
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (int y = h - 1; y >= 0; y--) fwrite(buf + (size_t)y * w * 3, 1, (size_t)w * 3, f);
        fclose(f);
        printf("Wrote %s (%dx%d)\n", path, w, h);
    }
    free(buf);
}

// Draw one scrolling layer as a run of side-by-side tile copies. `drawTile`
// is called once per copy with tileOriginX/currentScrollX already set, so
// every coordinate inside it lands in the right place on screen; the tiles
// either side of the visible one are included as a safety margin so nothing
// pops in/out at the edges.
template <typename Fn>
static void drawScrollingLayer(float layerScrollX, Fn drawTile) {
    currentScrollX = layerScrollX;
    int k0 = (int)floorf(layerScrollX / TILE_W) - 1;
    for (int k = k0; k <= k0 + 2; k++) {
        tileOriginX = k * TILE_W;
        drawTile();
    }
    tileOriginX = 0.0f;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // fixed virtual canvas so the composition survives any window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, CANVAS_W, 0, CANVAS_H, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    drawSky();   // one full-canvas gradient - never scrolls

    // sky layer: drifts slower than the ground for a parallax depth cue
    drawScrollingLayer((float)(scrollX * CLOUD_PARALLAX), drawClouds);

    // ground layer: terrain, stepped platforms, crates and bushes all slide
    // together at full speed, exactly like the reference composition just
    // sliding sideways forever
    drawScrollingLayer((float)scrollX, [] {
        drawBrickLayer();
        drawPlatforms();
        drawFloatingBlocks();
        drawSmallBlocks();
        drawBushes();
    });

    currentScrollX = 0.0f;

    glFlush();
    if (dumpPath) { writePPM(dumpPath); exit(0); }
    glutSwapBuffers();
}

// ==================== ANIMATION ====================

// Advances the scroll at a steady, real-time rate (not tied to frame rate),
// accelerating slowly up to a cap - the same "the game gets faster" feel as
// the Chrome dinosaur game - then reschedules itself for the next tick.
void update(int) {
    int now = glutGet(GLUT_ELAPSED_TIME);
    if (lastTimeMs == 0) lastTimeMs = now;
    double dt = (now - lastTimeMs) / 1000.0;
    lastTimeMs = now;

    if (!paused) {
        speed = speed + ACCEL * dt;
        if (speed > MAX_SPEED) speed = MAX_SPEED;
        scrollX += speed * dt;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);   // ~60 fps
}

// ==================== CALLBACKS ====================

void reshape(int w, int h) {
    windowWidth  = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    (void)x; (void)y;
    if (key == 27) exit(0);   // ESC
    if (key == ' ' || key == 'p' || key == 'P') paused = !paused;
    if (key == 'r' || key == 'R') { scrollX = 0.0; speed = BASE_SPEED; }
}

// ==================== MAIN ====================

int main(int argc, char** argv) {
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--dump") == 0) dumpPath = argv[i + 1];
        if (strcmp(argv[i], "--scroll") == 0) scrollX = atof(argv[i + 1]);
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("GLUT 2D Platformer Game - Scene");

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, update, 0);

    glClearColor(SKY_TOP[0], SKY_TOP[1], SKY_TOP[2], 1.0f);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    printf("GLUT 2D Game Scene Viewer - Infinite Scroller\n");
    printf("Press ESC to exit, SPACE/P to pause, R to reset speed\n");

    glutMainLoop();
    return 0;
}
