/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* X11 Headers */
#include <X11/Xlib.h>
#include "vroot.h"


/* OpenGL Headers */
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#define MAX_CIRCLES 7
#define RAINING_SPEED 25

struct circle {
    int centerX;
    int centerY;
    int outerRadius;
    int innerRadius;
    int innerAdvanceSpeed;
    int outerAdvanceSpeed;
    int hue;
    int sat;
};

struct rgb {
    double r;
    double g;
    double b;
};

/* Hue to Red-Green-Blue */
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

/* Hue-Saturation-Light to RGB (0 - 1) */
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


// Took from http://slabode.exofire.net/circle_draw.shtml
void DrawCircle(float cx, float cy, float r, int num_segments){
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

void paint_circle(struct circle circle, XWindowAttributes wa) {
    struct rgb color = hsl2rgb(circle.hue, 1, 0.75);
    glColor3f(color.r, color.g, color.b);

    int radius;
    for (radius = circle.innerRadius;radius < circle.outerRadius; radius++){
        DrawCircle(circle.centerX, circle.centerY, radius, 60);
    }
}


struct circle create_circle(XWindowAttributes wa){
    int speed = rand() % 5 + 2;
    struct circle circle = {.centerX = rand() % wa.width,
                            .centerY = rand() % wa.height,
                            .outerRadius = 50,
                            .innerRadius = 1,
                            .innerAdvanceSpeed = speed,
                            .outerAdvanceSpeed = speed / (((rand() % 25) + 101) / 100.0f),
                            .hue = rand() % 256
    };

    return circle;
}


int refresh_circles(XWindowAttributes wa, struct circle circles[], int circle_num){
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
        circles[i].innerRadius += circles[i].innerAdvanceSpeed;
        circles[i].outerRadius += circles[i].outerAdvanceSpeed;
        j++;
    }
    return j;
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
    XEvent                  xev;


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



    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);

    while(1) {
        XGetWindowAttributes(dpy, win, &gwa);
        glLoadIdentity();

        glOrtho(0, gwa.width, gwa.height, 0, 0.0f, 100.0f);

        glClearColor(0, 0, 0, 0.5f);
        glClearDepth( 1.0f);
        glClear(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT);

        glViewport(0, 0, gwa.width, gwa.height);

        circle_num = refresh_circles(gwa, circles, circle_num);
        glXSwapBuffers(dpy, win);

        usleep(100000);
    }
}
