/*
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://www.wtfpl.net/ for more details.
 */

/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

/* X11 Headers */
#include <X11/Xlib.h>
#include "vroot.h"


/* OpenGL Headers */
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

/* Shader files */
#include "empty_shader.frag.c"

// Max. circles on screen at a time
#define MAX_CIRCLES 7

// Probability (divided by 256) to generate a new circle
#define RAINING_SPEED 25


struct circle {
    int centerX;
    int centerY;
    int outerRadius; // Big radius
    int innerRadius; // Little radius
    int outerAdvanceSpeed; // Big radius spreading speed
    int innerAdvanceSpeed; // Little radius spreading speed
    int hue; // Circle color hue
    int sat; // Circle color saturation
};

// RGB color representation
struct rgb {
    double r;
    double g;
    double b;
};


int max(int x, int y){
    if (x > y){
        return x;
    }
    else{
        return y;
    }
}


static int check_shader_compilation(GLuint shader, const char* src){
    GLint n;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &n);
    if( n == GL_FALSE ) {
        GLchar *info_log;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &n);
        info_log = malloc(n);
        glGetShaderInfoLog(shader, n, &n, info_log);
        fprintf(stderr, "----- >8 ----\n%s\n----- >8 ----\n", src);
        fprintf(stderr, "Shader compilation failed: %*s\n", n, info_log);
        free(info_log);
        return 0;
    }
    return 1;
}


/**
 * Represent hue into RGB.
 *  hsl2rgb auxiliary function.
 *
 */
double Hue_to_RGB(double P, double Q, double H){
    if (H < 0) {
        H += 1.0;
    }
    else if (H > 1.0) {
        H -= 1.0;
    }

    if ((6 * H) < 1.0) {
        return P + (Q - P) * 6 * H;
    }
    if ((2 * H) < 1.0) {
        return Q;
    }
    if ((3 * H) < 2.0) {
        return P + (Q - P) * (2.0/3.0 - H) * 6;
    }
    return P;
}


/**
 * Convert the HSL color representation to RGB.
 *
 * @param h Input color hue.
 * @param s Input color saturation.
 * @param l Input color lightness.
 *
 * @return The rgb color.
 */
struct rgb hsl2rgb(double h, double s, double l){

    double P, Q;

    if (( l == 0 ) || ( s == 0 )){
        return (struct rgb){.r = l, .g = l, .b = l};
    }

    h /= 255.;

    if (l < 0.5){
        Q = l * (1.0 + s);
    }
    else{
        Q = l + s - (l * s);
    }

    P = 2.0 * l - Q;

    return (struct rgb) {.r = Hue_to_RGB(P, Q, h + 1.0/3.0),
            .g = Hue_to_RGB(P, Q, h),
            .b = Hue_to_RGB(P, Q, h - 1.0/3.0)};
}


// Took shamelessly from http://slabode.exofire.net/circle_draw.shtml
void DrawCircle(float cx, float cy, float r, int num_segments){
    assert(num_segments != 0);
    float theta = 2 * 3.1415926 / ((float)num_segments);
    float c = cosf(theta);//precalculate the sine and cosine
    float s = sinf(theta);
    float t;

    float x = r;//we start at angle = 0
    float y = 0;

    glBegin(GL_LINE_LOOP);
    int ii;
    for(ii = 0; ii < num_segments; ii++){
        glVertex2f(x + cx, y + cy);//output vertex

        //apply the rotation matrix
        t = x;
        x = c * x - s * y;
        y = s * t + c * y;
    }
    glEnd();
}


/**
 * Paints a circle in the screen.
 *
 * @param circle The circle to paint.
 * @param wa     Screen window attributes.
 *
 */
void paint_circle(struct circle circle, XWindowAttributes wa) {
    struct rgb color = hsl2rgb(circle.hue, 1, 0.5);
    glColor4f(color.r, color.g, color.b, 0.5);

    int radius;
    for (radius = circle.innerRadius;radius < circle.outerRadius; radius++){
        DrawCircle(circle.centerX, circle.centerY, radius, 60);
    }
}


/**
 * Generate a new circle.
 *  Most parameters are intended to be random inside some parameters.
 *
 * @param wa The window attributes (to create the circle inside it).
 *
 * @return The generated circle.
 *
 */
struct circle create_circle(XWindowAttributes wa){
    int speed = rand() % 5 + 2;
    int outer_radius = rand() % 50 + 25;
    struct circle circle = {.centerX = rand() % wa.width,
                            .centerY = rand() % wa.height,
                            .outerRadius = outer_radius,
                            .innerRadius = max(1, outer_radius - ((rand() % 25 ) + 25)),
                            .innerAdvanceSpeed = speed,
                            .outerAdvanceSpeed = speed / (((rand() % 25) + 101) / 100.0f),
                            .hue = rand() % 256
    };

    return circle;
}


/**
 * (Possibly) create new circles, paint all to screen and refresh its state.
 *
 * @param wa Window attributes to fit the circles in.
 * @param circles The circle list.
 * @param circle_num The number of circles currently on screen.
 *
 * @return The number of circles left after the iteration.
 */
int refresh_circles(XWindowAttributes wa, struct circle circles[], int circle_num){
    // If no circles in screen or there is some space and random falls behind
    // the given value, create a new circle.
    if ((circle_num == 0) ||
        ((circle_num < MAX_CIRCLES) && ((rand() % 256) < RAINING_SPEED))){

        circles[circle_num++] = create_circle(wa);
    }

    int i, j;
    for (i = j = 0; i < circle_num; i++){

        // Delete done circles
        if (circles[i].innerRadius >= circles[i].outerRadius){
            continue;
        }

        if (i != j){
            circles[j] = circles[i];
        }

        paint_circle(circles[i], wa);
        int speed_diff = circles[i].innerAdvanceSpeed - circles[i].outerAdvanceSpeed;
        int radius_diff = circles[i].outerRadius - circles[i].innerRadius;
        if ((speed_diff > 1) && (radius_diff < (speed_diff * 10))){

            circles[i].innerAdvanceSpeed--;
        }

        circles[i].innerRadius += circles[i].innerAdvanceSpeed;
        circles[i].outerRadius += circles[i].outerAdvanceSpeed;
        j++;
    }
    return j;
}


void set_empty_shader(){
    GLuint empty_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(empty_frag_shader, 1, &EMPTY_FRAG_SHADER, NULL);
    glCompileShader(empty_frag_shader);
    assert(check_shader_compilation(empty_frag_shader, EMPTY_FRAG_SHADER));

    GLuint prog = glCreateProgram();
    glAttachShader(prog, empty_frag_shader);
    glLinkProgram(prog);
    glUseProgram(prog);
}


int main(int argc, char *argv[]) {
    srand(time(NULL));

    char *display_id;

    Display                 *dpy;
    Window                  root;
    GLint                   att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    XVisualInfo             *vi;
    Colormap                cmap;
    XSetWindowAttributes    swa;
    Window                  win;
    GLXContext              glc;
    XWindowAttributes       gwa;


    /* Rain variables */
    struct circle circles[MAX_CIRCLES];
    int circle_num = 0;

    /* ### Preparing X enviroment ### */
    /* -root for screensaver  */
    if (argc > 1 && strcmp(argv[1], "-root") == 0){

        /* open the display (connect to the X server) */
        display_id = getenv("DISPLAY");
        if (display_id == NULL){
            perror("getenv(\"DISPLAY\")");
        }

        dpy = XOpenDisplay (display_id);


        /* get the root window */
        root = DefaultRootWindow (dpy);
     }
    /* Or do you prefer a standalone window? */
    else{
        /* Temporal variables */
        int s;
        int window_width = 600;
        int window_height = 600;

        if (argc >= 4 && strcmp(argv[1], "-d") == 0){
            window_width = atoi(argv[2]);
            window_height = atoi(argv[3]);
        }

        dpy = XOpenDisplay(NULL);
        if (dpy == NULL) {
            fprintf(stderr, "Cannot open display\n");
            exit(1);
        }

        s = DefaultScreen(dpy);
        root = XCreateSimpleWindow(dpy, RootWindow(dpy, s),
                               10, 10, window_width, window_height, 1,
                               BlackPixel(dpy, s), WhitePixel(dpy, s));


        XMapWindow(dpy, root);
    }

    // Mix OpenGL and X11
    vi = glXChooseVisual(dpy, 0, att);

    if(vi == NULL) {
        printf("\n\tno appropriate visual found\n\n");
        exit(0);
    }


    cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);

    swa.colormap = cmap;
    swa.event_mask = ExposureMask | KeyPressMask;
    XGetWindowAttributes(dpy, root, &gwa);

    win = XCreateWindow(dpy, root, 0, 0, gwa.width, gwa.height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

    XMapWindow(dpy, win);
    XStoreName(dpy, root, "Neon rain");

    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);

    // OpenGL properties
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    printf("OpenGL version: %s\n", glGetString(GL_VERSION));

    set_empty_shader();

    while(1) {
        // Clean screen
        XGetWindowAttributes(dpy, win, &gwa);
        glLoadIdentity();

        glOrtho(0, gwa.width, gwa.height, 0, 0.0f, 100.0f);

        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, gwa.width, gwa.height);

        // Refresh the "raining" state
        circle_num = refresh_circles(gwa, circles, circle_num);

        // Paint it to the screen
        glXSwapBuffers(dpy, win);

        usleep(50000);
    }
}
