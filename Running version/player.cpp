// =====================================================
// BEAUTIFUL GIRL - SIDE VIEW  (run cycle)
// build: g++ player.cpp -o player -framework OpenGL -framework GLUT
// keys : Space = jump,  P = pause,  R = reset,  + / - = run speed,  Esc = quit
// =====================================================

#ifdef _WIN32
#include <windows.h>
#include <GL/glut.h>
#else
#include <GLUT/glut.h>
#endif
#include <math.h>
#include <stdlib.h>

#include "player.h"

#define PI 3.14159265f

// skin, and the shaded skin used for the limbs on the far side
#define SKIN      1.00f, 0.72f, 0.52f
#define SKIN_DARK 0.86f, 0.58f, 0.40f

static float gPhase  = 0.0f;   // position in the run cycle, radians
static float gRate   = 9.0f;   // radians/sec -- how fast she runs

static float gJumpY  = 0.0f;   // height above the ground, pixels
static float gJumpV  = 0.0f;   // vertical speed, pixels/sec
static float gLand   = 0.0f;   // countdown for the landing puff

#define JUMP_V   330.0f        // take-off speed  -> about 60px high
#define GRAVITY  900.0f

static float mixf(float a, float b, float k) { return a + (b - a) * k; }

// =====================================================
// HELPERS
// =====================================================

static void drawRect(float x1, float y1, float x2, float y2)
{
    glBegin(GL_QUADS);
        glVertex2f(x1, y1);
        glVertex2f(x2, y1);
        glVertex2f(x2, y2);
        glVertex2f(x1, y2);
    glEnd();
}

static void drawCircle(float cx, float cy, float r)
{
    glBegin(GL_POLYGON);
    for (int i = 0; i < 40; i++)
    {
        float a = (i * 2 * PI) / 40;
        glVertex2f(cx + r * cos(a), cy + r * sin(a));
    }
    glEnd();
}

// =====================================================
// LIMBS
// Each is built in its own local space -- the joint sits at
// (0,0) and the bone hangs down -Y -- so a glRotatef swings it.
// =====================================================

// shoe, drawn at the ankle of whatever shin called it
static void drawShoe(float w)
{
    glColor3f(0.18f, 0.05f, 0.12f);
    glBegin(GL_POLYGON);
        glVertex2f(-w * 0.75f,  1.0f);
        glVertex2f( w * 0.75f,  1.0f);
        glVertex2f( w * 2.10f, -3.0f);
        glVertex2f(-w * 0.75f, -4.5f);
    glEnd();
}

// hipA: + swings the thigh forward.  kneeA: - folds the shin back (knees only bend one way)
static void drawLeg(float hipX, float hipY, float hipA, float kneeA,
                    float r, float g, float b)
{
    const float thigh = 19.0f, shin = 18.0f, w = 8.0f;

    glPushMatrix();
    glTranslatef(hipX, hipY, 0);
    glRotatef(hipA, 0, 0, 1);

        glColor3f(r, g, b);
        drawRect(-w * 0.5f, -thigh, w * 0.5f, 0);
        drawCircle(0, -thigh, w * 0.5f);          // knee joint keeps the bend solid

        glTranslatef(0, -thigh, 0);
        glRotatef(kneeA, 0, 0, 1);

            glColor3f(r, g, b);
            drawRect(-w * 0.45f, -shin, w * 0.45f, 0);
            glTranslatef(0, -shin, 0);
            drawShoe(w);

    glPopMatrix();
}

// shoulderA: + swings the arm forward.  elbowA: + folds the forearm forward
static void drawArm(float sx, float sy, float shoulderA, float elbowA,
                    float r, float g, float b)
{
    const float upper = 15.0f, fore = 14.0f, w = 7.0f;

    glPushMatrix();
    glTranslatef(sx, sy, 0);
    glRotatef(shoulderA, 0, 0, 1);

        glColor3f(r, g, b);
        drawRect(-w * 0.5f, -upper, w * 0.5f, 0);
        drawCircle(0, -upper, w * 0.5f);          // elbow

        glTranslatef(0, -upper, 0);
        glRotatef(elbowA, 0, 0, 1);

            drawRect(-w * 0.45f, -fore, w * 0.45f, 0);
            drawCircle(0, -fore, w * 0.62f);      // hand

    glPopMatrix();
}

// bodice -- the original art stopped at the waist and left a gap up to the neck
static void drawBodice()
{
    glColor3f(0.95f, 0.22f, 0.48f);
    glBegin(GL_POLYGON);
        glVertex2f(-12, 41);
        glVertex2f(  9, 41);
        glVertex2f( 10, 55);
        glVertex2f(  4, 60);
        glVertex2f( -5, 60);
        glVertex2f(-12, 56);
    glEnd();
    drawCircle(4, 55, 7);            // shoulder cap
    glColor3f(1.0f, 0.40f, 0.62f);
    glBegin(GL_POLYGON);             // lit side
        glVertex2f(-9, 43);
        glVertex2f( 1, 43);
        glVertex2f( 2, 55);
        glVertex2f(-8, 54);
    glEnd();
}

// skirt: hem rides up and trails behind while she runs
static void drawDress(float hem, float flow)
{
    glColor3f(0.95f, 0.22f, 0.48f);
    glBegin(GL_POLYGON);
        glVertex2f(-13, 43);
        glVertex2f(  8, 43);
        glVertex2f( 17 + flow * 0.5f, hem - flow);
        glVertex2f(-19 - flow * 1.6f, hem + flow * 1.2f);
    glEnd();

    glColor3f(1.0f, 0.40f, 0.62f);
    glBegin(GL_POLYGON);
        glVertex2f(-8, 40);
        glVertex2f( 4, 40);
        glVertex2f( 9 + flow * 0.3f, hem + 5 - flow * 0.6f);
        glVertex2f(-11 - flow * 1.0f, hem + 5 + flow * 0.8f);
    glEnd();
}

// kicked-up sand, oldest puff drifting furthest back
static void drawDust(float p, float air)
{
    for (int i = 0; i < 3; i++)
    {
        float k = fmodf(p / (2 * PI) + i * 0.33f, 1.0f);
        glColor4f(0.86f, 0.76f, 0.55f, 0.40f * (1.0f - k) * (1.0f - air));
        drawCircle(-12 - 48 * k, 4 + 12 * k, 3 + 10 * k);
    }
}

// =====================================================
// BEAUTIFUL GIRL - SIDE VIEW
// p   = phase in the run cycle (radians)
// air = 0 running on the ground .. 1 fully in the jump pose
// =====================================================

static void drawPlayerPose(float x, float y, float p, float air)
{
    // ---- the cycle ----------------------------------
    float bob    = 4.0f * fabs(sin(p)) * (1 - air); // planted at mid-stance, up in mid-flight
    float lean   = mixf(5.0f, 8.0f, air);          // she leans into the run, harder in the air
    float hipF   =  32 * sin(p);                    // front leg
    float hipB   =  32 * sin(p + PI);               // back leg, half a cycle behind
    float kneeF  = -30 - 30 * cos(p + 0.6f);        // folds after toe-off, straight when reaching
    float kneeB  = -30 - 30 * cos(p + PI + 0.6f);
    float armF   =  30 * sin(p + PI);               // arms oppose the legs
    float armB   =  30 * sin(p);
    float hem    =  22 + 2 * sin(p);
    float flow   =   4 + 3 * sin(p);
    float hairA  = -10 + 6 * sin(p);

    // leap pose: lead leg tucked up in front, trailing leg kicked out behind,
    // lead arm thrown overhead, skirt and hair blown up by the rise
    hipF  = mixf(hipF,  48.0f,  air);
    kneeF = mixf(kneeF, -55.0f, air);
    hipB  = mixf(hipB, -38.0f,  air);
    kneeB = mixf(kneeB, -78.0f, air);
    armF  = mixf(armF,  60.0f,  air);
    armB  = mixf(armB, -55.0f,  air);
    hem   = mixf(hem,   28.0f,  air);
    flow  = mixf(flow,   9.0f,  air);
    hairA = mixf(hairA, -16.0f, air);

    glPushMatrix();
    glTranslatef(x, y + bob, 0);
    glTranslatef(0, 42, 0);
    glRotatef(-lean, 0, 0, 1);
    glTranslatef(0, -42, 0);

    drawDust(p, air);

    // =================================================
    // FAR LEG + FAR ARM  (behind the dress, shaded)
    // =================================================

    drawLeg(4, 42, hipB, kneeB, SKIN_DARK);
    drawArm(1, 55, armB, mixf(70, 45, air), SKIN_DARK);

    // =================================================
    // NEAR LEG
    // =================================================

    drawLeg(-2, 42, hipF, kneeF, SKIN);

    // =================================================
    // DRESS
    // =================================================

    drawDress(hem, flow);
    drawBodice();

    // =================================================
    // DRESS WAIST
    // =================================================

    glColor3f(0.75f, 0.10f, 0.30f);

    drawRect(-11, 39, 8, 45);

    // =================================================
    // NECK
    // =================================================

    glColor3f(SKIN);

    drawRect(-4, 58, 5, 69);

    // =================================================
    // HEAD
    // =================================================

    glColor3f(SKIN);

    drawCircle(2, 84, 18);

    // =================================================
    // EAR
    // =================================================

    glColor3f(0.95f, 0.62f, 0.45f);

    drawCircle(17, 83, 5);

    // =================================================
    // LONG HAIR  (swings from the head, trailing behind)
    // =================================================

    glPushMatrix();
    glTranslatef(2, 84, 0);
    glRotatef(hairA, 0, 0, 1);
    glTranslatef(-2, -84, 0);

        glColor3f(0.12f, 0.045f, 0.02f);

        drawCircle(-4, 96, 18);

        drawCircle(-16, 80, 15);
        drawCircle(-19, 60, 14);
        drawCircle(-18, 42, 12);

        drawRect(-31, 42, -5, 80);

        // =============================================
        // HAIR HIGHLIGHT
        // =============================================

        glColor3f(0.32f, 0.12f, 0.05f);

        glLineWidth(3);

        glBegin(GL_LINES);

        glVertex2f(-13, 99);
        glVertex2f(-24, 58);

        glVertex2f(-7, 98);
        glVertex2f(-18, 48);

        glEnd();

        // =============================================
        // HAIR BAND
        // =============================================

        glColor3f(1.0f, 0.15f, 0.42f);

        glLineWidth(3);

        glBegin(GL_LINES);

        glVertex2f(-16, 95);
        glVertex2f(8, 100);

        glEnd();

    glPopMatrix();

    // =================================================
    // NOSE
    // =================================================

    glColor3f(SKIN);

    glBegin(GL_TRIANGLES);

    glVertex2f(18, 86);
    glVertex2f(27, 82);
    glVertex2f(18, 78);

    glEnd();

    // =================================================
    // EYE
    // =================================================

    glColor3f(0.02f, 0.02f, 0.02f);

    drawCircle(11, 89, 2.5);

    // =================================================
    // EYELASH
    // =================================================

    glLineWidth(2);

    glBegin(GL_LINES);

    glVertex2f(12, 91);
    glVertex2f(16, 93);

    glEnd();

    // =================================================
    // MOUTH
    // =================================================

    glColor3f(0.65f, 0.10f, 0.18f);

    glBegin(GL_LINES);

    glVertex2f(20, 74);
    glVertex2f(25, 74);

    glEnd();

    // =================================================
    // NEAR ARM  (in front of everything)
    // =================================================

    drawArm(6, 55, armF, mixf(70, 25, air), SKIN);

    glPopMatrix();
}

// sand thrown up where she lands
static void drawLandPuff(float x, float y, float k)
{
    for (int i = 0; i < 5; i++)
    {
        float dir = (i - 2) * 13.0f;
        glColor4f(0.86f, 0.76f, 0.55f, 0.55f * k);
        drawCircle(x + dir * (1.4f - k), y + 3 + 16 * (1 - k), 4 + 7 * (1 - k));
    }
}

// =====================================================
// PUBLIC API  (see player.h)
// =====================================================

void playerUpdate(float dt)
{
    gPhase += gRate * dt;
    if (gPhase > 2 * PI) gPhase -= 2 * PI;

    if (gJumpY > 0.0f || gJumpV > 0.0f)
    {
        gJumpV -= GRAVITY * dt;
        gJumpY += gJumpV * dt;
        if (gJumpY <= 0.0f)                  // touchdown
        {
            gJumpY = 0.0f;
            gJumpV = 0.0f;
            gPhase = 0.0f;                   // land on a planted foot
            gLand  = 0.35f;
        }
    }
    if (gLand > 0.0f) gLand -= dt;
}

void playerJump()
{
    if (gJumpY == 0.0f && gJumpV == 0.0f) gJumpV = JUMP_V;
}

float playerHeight() { return gJumpY; }

void playerSetSpeed(float radiansPerSecond) { gRate = radiansPerSecond; }

void drawPlayer(float x, float y)
{
    if (gLand > 0.0f) drawLandPuff(x, y, gLand / 0.35f);

    float air = gJumpY / 14.0f;              // pose blends in as she leaves the ground
    if (air > 1.0f) air = 1.0f;

    drawPlayerPose(x, y + gJumpY, gPhase, air);
}

// =====================================================
// STANDALONE PREVIEW -- only built with -DPLAYER_DEMO,
// so these names cannot clash with the scene that links her in
// =====================================================
#ifdef PLAYER_DEMO

static float gScroll = 0.0f;
static int   gPaused = 0;


void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(0.88f, 0.78f, 0.52f);
    drawRect(0, 0, 400, 70);

    glColor3f(0.72f, 0.62f, 0.40f);
    glLineWidth(2);
    glBegin(GL_LINES);
        glVertex2f(0, 70);
        glVertex2f(400, 70);
    glEnd();

    // scrolling ground detail, so running in place still reads as forward motion
    glColor3f(0.78f, 0.68f, 0.44f);
    for (int i = 0; i < 12; i++)
    {
        float px = fmodf(i * 40.0f - gScroll, 440.0f);
        if (px < 0) px += 440.0f;
        drawCircle(px - 20, 20 + 30 * fmodf(i * 0.37f, 1.0f), 3 + (i % 3));
    }

    drawPlayer(200, 70);

    glutSwapBuffers();
}

void tick(int)
{
    if (!gPaused)
    {
        playerUpdate(0.016f);
        gScroll += gRate * 13.5f * 0.016f;   // ground keeps pace with her stride
    }
    glutPostRedisplay();
    glutTimerFunc(16, tick, 0);
}

void keyboard(unsigned char key, int, int)
{
    if (key == 27) exit(0);                                   // Esc
    if (key == ' ' && !gPaused) playerJump();
    if (key == 'p' || key == 'P') gPaused = !gPaused;
    if (key == 'r' || key == 'R') { gScroll = 0; }
    if (key == '+' || key == '=') { if (gRate < 20) playerSetSpeed(gRate + 1.5f); }
    if (key == '-' || key == '_') { if (gRate >  2) playerSetSpeed(gRate - 1.5f); }
}

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 400, 0, 320);
    glMatrixMode(GL_MODELVIEW);
}

void init()
{
    glClearColor(0.68f, 0.90f, 0.92f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 400, 0, 320);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(400, 320);
    glutInitWindowPosition(120, 120);
    glutCreateWindow("Player - Run Cycle");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(16, tick, 0);
    glutMainLoop();
    return 0;
}

#endif   // PLAYER_DEMO
