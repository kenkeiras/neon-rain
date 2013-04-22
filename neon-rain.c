/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* X11 Headers */
#include <X11/Xlib.h>
#include "vroot.h"


#define MAX_CIRCLES 256

struct circle {
    int centerX;
    int centerY;
    int outerRadius;
    int innerRadius;
    XColor color;
};

struct rgb {
    int r;
    int g;
    int b;
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

/* Hue-Saturation-Light to Red-Green-Blue */
struct rgb hsl2rgb(double h, double s, double l){

    double P, Q;

    if (( l == 0 ) || ( s == 0 )){
        return (struct rgb) {.r = l, .g = l, .b = l};
    }

    h /= 255.;

    if (l < 0.5){
        Q = l * (1.0 + s);
    }
    else{
        Q = l + s - (l * s);
    }

    P = 2.0 * l - Q;

    return (struct rgb) {
        .r = 255 * Hue_to_RGB(P, Q, h + 1.0/3.0),
        .g = 255 * Hue_to_RGB(P, Q, h),
        .b = 255 * Hue_to_RGB(P, Q, h - 1.0/3.0)};
}


void paint_circle(struct circle circle, Display *dpy, Pixmap double_buffer, GC gc, XWindowAttributes wa){
    int border;
    int narcs = circle.outerRadius - circle.innerRadius;

    XArc arcs[narcs];

    int i;
    for (border = circle.innerRadius, i = 0;border < circle.outerRadius; border++, i++){
        arcs[i] = (XArc) {.x = circle.centerX - (border / 2),
                          .y = circle.centerY - (border / 2),
                          .width = border,
                          .height = border,
                          .angle1 = 0,
                          .angle2 = 360 * 64
        };
    }

    XSetForeground(dpy, gc, circle.color.pixel);
    XDrawArcs(dpy, double_buffer, gc, arcs, narcs);
}



/* Main function, too big :/ */
int main (int argc, char **argv){
    srandom(time(NULL)); /* Pseudo-randomness initialization */


    /* ### Variable declaration ### */
    /* X11 variables */
    Display *dpy;
    Window root;
    XWindowAttributes wa;
    GC gc;
    char *display_id;
    Pixmap double_buffer;
    char color_buffer[12]; /* Color string buffer */


    /* Rain variables */
    struct circle circles[MAX_CIRCLES];
    int raining_speed = 25;
    int spread_speed = 2;
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
            window_height = atoi(argv[2]);
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
    /* create a GC for drawing in the window */
    gc = XCreateGC (dpy, root, 0, NULL);


    /* Here starts the action: */
    while (1){
        /* get attributes of the root window (could have changed) */
        XGetWindowAttributes(dpy, root, &wa);

        /* Clear the double buffer */
        double_buffer = XCreatePixmap(
            dpy, root, wa.width, wa.height, wa.depth);


        if ((circle_num == 0) ||
            ((circle_num < MAX_CIRCLES) && ((rand() % 256) < raining_speed))){


            // Circle properties
            circles[circle_num] = (struct circle) {.centerX = rand() % wa.width,
                                                   .centerY = rand() % wa.height,
                                                   .outerRadius = 50,
                                                   .innerRadius = 1
            };

            // Circle color
            struct rgb color = hsl2rgb(rand() % 256,
                                       1.0f,
                                       100);

            sprintf(color_buffer, "rgb:%02x/%02x/%02x", color.r, color.g, color.b);

            XColor stub;
            XAllocNamedColor(dpy,
                             DefaultColormapOfScreen(
                                 DefaultScreenOfDisplay(dpy)),
                             color_buffer, &circles[circle_num].color, &stub);


            circle_num++;
            printf("-> %i\n", circle_num);
        }

        int i;
        for (i = 0; i < circle_num; i++){
            paint_circle(circles[i], dpy, double_buffer, gc, wa);
            circles[i].innerRadius += spread_speed;
            circles[i].outerRadius += spread_speed;
        }


        XCopyArea(dpy, double_buffer, root,
                  gc, 0, 0, wa.width, wa.height, 0, 0);

        usleep(100000);
    }

    /* It actually never reaches here :P */
    XCloseDisplay (dpy);
}
