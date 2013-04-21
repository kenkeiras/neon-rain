/* Standard C headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* X11 Headers */
#include <X11/Xlib.h>
#include "vroot.h"


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
void hsl2rgb(double h, double s, double l,
             double *r, double *g, double *b){

    double P, Q;

    if (( l == 0 ) || ( s == 0 )){
        *r = *r = *b = l;
        return;
    }

    h /= 255.;

    if (l < 0.5){
        Q = l * (1.0 + s);
    }
    else{
        Q = l + s - (l * s);
    }

    P = 2.0 * l - Q;

    *r = 255 * Hue_to_RGB(P, Q, h + 1.0/3.0);
    *g = 255 * Hue_to_RGB(P, Q, h);
    *b = 255 * Hue_to_RGB(P, Q, h - 1.0/3.0);
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
    XColor xc, sc;
    int pixsize; /* Pixel size */
    char *display_id;
    Pixmap double_buffer;


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

    /* get attributes of the root window */
    XGetWindowAttributes(dpy, root, &wa);

    /* Here starts the action: */
    while (1){
        sleep(1);
    }

    /* It actually never reaches here :P */
    XCloseDisplay (dpy);
}
