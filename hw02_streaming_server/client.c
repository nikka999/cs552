#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "GL/glx.h"
#include <X11/keysym.h>
#include <stdlib.h>
#include <netpbm/ppm.h>

/** image methods */
Display *dpy, *dpy1, *dpy2, *dpy3;
Window window, window1, window2, window3;
GLXContext cx1, cx2, cx3;

int START;
int SEEK;
int STOP;
int REPEAT;

static void make_window (int width, int height, char *name, int border, Window *window, GLXContext *cx, Display **dpy);
static void make_window_seek (int width, int height, char *name, int border);
static int attributeList[] = { GLX_RGBA, GLX_RED_SIZE, 1, None };

void noborder (Display *dpy, Window win) {
    struct {
        long flags;
        long functions;
        long decorations;
        long input_mode;
    } *hints;
    
    int fmt;
    unsigned long nitems, byaf;
    Atom type;
    Atom mwmhints = XInternAtom (dpy, "_MOTIF_WM_HINTS", False);
    
    XGetWindowProperty (dpy, win, mwmhints, 0, 4, False, mwmhints,
                        &type, &fmt, &nitems, &byaf,
                        (unsigned char**)&hints);
    
    if (!hints)
        hints = (void *)malloc (sizeof *hints);
    
    hints->decorations = 0;
    hints->flags |= 2;
    
    XChangeProperty (dpy, win, mwmhints, mwmhints, 32, PropModeReplace,
                     (unsigned char *)hints, 4);
    XFlush (dpy);
    free (hints);
}

static void make_window_seek (int width, int height, char *name, int border) {
    XVisualInfo *vi;
    Colormap cmap;
    XSetWindowAttributes swa;
    GLXContext cx;
    XSizeHints sizehints;
    
    dpy = XOpenDisplay (0);
    if (!(vi = glXChooseVisual (dpy, DefaultScreen(dpy), attributeList))) {
        printf ("Can't find requested visual.\n");
        exit (1);
    }
    cx = glXCreateContext (dpy, vi, 0, GL_TRUE);
    
    swa.colormap = XCreateColormap (dpy, RootWindow (dpy, vi->screen),
                                    vi->visual, AllocNone);
    sizehints.flags = 0;
    
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;
    window = XCreateWindow (dpy, RootWindow (dpy, vi->screen),
                            0, 0, width, height,
                            0, vi->depth, InputOutput, vi->visual,
                            CWBorderPixel|CWColormap|CWEventMask, &swa);
    XMapWindow (dpy, window);
    XSetStandardProperties (dpy, window, name, name,
                            None, (void *)0, 0, &sizehints);
    
    if (!border) 
        noborder (dpy, window);
    
    glXMakeCurrent (dpy, window, cx);
}


static void make_window (int width, int height, char *name, int border,
                         Window *window, GLXContext *cx, Display **dpy) {
    XVisualInfo *vi;
    XSetWindowAttributes swa;
    XSizeHints sizehints;
    
    *dpy = XOpenDisplay (0);
    if (!(vi = glXChooseVisual (*dpy, DefaultScreen(*dpy), attributeList))) {
        printf ("Can't find requested visual.\n");
        exit (1);
    }
    *cx = glXCreateContext (*dpy, vi, 0, GL_TRUE);
    
    swa.colormap = XCreateColormap (*dpy, RootWindow (*dpy, vi->screen),
                                    vi->visual, AllocNone);
    sizehints.flags = 0;
    
    swa.border_pixel = 0;
    swa.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask;
    *window = XCreateWindow (*dpy, RootWindow (*dpy, vi->screen),
                             0, 0, width, height,
                             0, vi->depth, InputOutput, vi->visual,
                             CWBorderPixel|CWColormap|CWEventMask, &swa);
    XMapWindow (*dpy, *window);
    XSetStandardProperties (*dpy, *window, name, name,
                            None, (void *)0, 0, &sizehints);
    
    if (!border) 
        noborder (*dpy, *window);
    
}
/** EOF image */

typedef struct{
	char* clientid;
	int priority;
} Params;

Params params;

void print_help(void) {
	printf("usage:	-h : print help message\n"
           "	-i : specify Client ID (in form number-hostname)\n"
           "	-p : specify Client Priority\n");
}

int parse_args(int argc, char const **argv, Params *p) {
	int count = 1;
    //initialize port and workers to default, incase user does not specify one or both of them.
	p->clientid = "1-nikka";
	p->priority = 10;
    
	if (argc == 1) {
		printf("You have not specified any parameters. So we will run using default params.\n");
		print_help();
	}
	else {
		while (count < argc) {
			if (!strcmp(argv[count], "-h")) {
				print_help();
				return 1;
			}
			else if (!strcmp(argv[count], "-i")) {
				count++;
				p->clientid = (char *)argv[count];
				count++;
			}
			else if (!strcmp(argv[count], "-p")) {
				count++;
				p->priority = atoi(argv[count]);
				count++;
			}
			else {
				printf("unkown parameter: %s\n", argv[count]);
				return 1;
			}
		}
	}
	printf("Client started. ID: %s, Priority: %d\n", p->clientid, p->priority);
	return 0;
}
void *recv_listen(void *sd) {
	int fd = (int) sd;
	size_t data_len;
    size_t type;
    int cols = 160;
    int rows = 120;

    unsigned char *buf;

    int arg = (cols*rows*3)/5;

	while(1) {
        // Read type.
        // 2 = seek
        // 1 = start
        // 3 = stop
        read(fd, &type, sizeof(size_t));
        type = ntohl(type);
        
        // if seek
        if (type == 2 && SEEK == 1) {
            int exit = 0;
            // Read pixarray
            buf = (unsigned char *)malloc(arg*5);
            read(fd, (buf), arg);
            read(fd, (buf+arg), arg);
            read(fd, (buf+2*arg), arg);
            read(fd, (buf+3*arg), arg);
            read(fd, (buf+4*arg), arg);
            
            make_window_seek (cols, rows, "SEEK", 1);
            glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
            glMatrixMode (GL_PROJECTION);
            glOrtho (0, cols, 0, rows, -1, 1);
            glMatrixMode (GL_MODELVIEW);
            glRasterPos2i (0, 0);
            
            while (!exit) {
                XEvent ev;
                XNextEvent (dpy, &ev);
                switch (ev.type) {
                    case Expose:
                        glClearColor (0.5, 0.5, 0.5, 0.5);
                        glClear (GL_COLOR_BUFFER_BIT);
                        glDrawPixels (cols, rows, GL_RGB, GL_UNSIGNED_BYTE, buf);
                        break;
                        
                    case KeyPress: {
                        char buf2[100];
                        int rv;
                        KeySym ks;
                        
                        rv = XLookupString (&ev.xkey, buf2, sizeof(buf2), &ks, 0);
                        switch (ks) {
                            case XK_Escape:
                                free (buf);
                                exit = 1;
                        }
                        break;
                    }
                }
            }
            glFlush();
        } else if (START == 1 && type == 1) {
            // If start
            make_window (160, 120, "Uncompressed Data Viewer1", 1,
                         &window1, &cx1, &dpy1);
            make_window (160, 120, "Uncompressed Data Viewer2", 1,
                         &window2, &cx2, &dpy2);
            make_window (160, 120, "Uncompressed Data Viewer3", 1,
                         &window3, &cx3, &dpy3);
            
            glXMakeCurrent (dpy1, window1, cx1);
            glMatrixMode (GL_PROJECTION);
            glOrtho (0, cols, 0, rows, -1, 1);
            glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
            glMatrixMode (GL_MODELVIEW);
            glRasterPos2i (0, 0);
            
            int frame_count = 1;
            
            while (!STOP) {
                if ((!REPEAT) && (frame_count > 100)) {
                    // if no repeat, break out.
                    STOP = 1;
                    continue;
                }
                // Read pixarray
                buf = (unsigned char *)malloc(arg*5);
                read(fd, (buf), arg);
                read(fd, (buf+arg), arg);
                read(fd, (buf+2*arg), arg);
                read(fd, (buf+3*arg), arg);
                read(fd, (buf+4*arg), arg);
                
                if ((frame_count % 2) == 0)
                    glXMakeCurrent (dpy2, window2, cx2);
                else
                    if ((frame_count % 3) == 0)
                        glXMakeCurrent (dpy3, window3, cx3);
                    else
                        glXMakeCurrent (dpy1, window1, cx1);
                
                glDrawPixels (cols, rows, GL_RGB, GL_UNSIGNED_BYTE, buf);
                glFlush ();
                usleep(24100);
                
                read(fd, &type, sizeof(size_t));
                type = ntohl(type);
                if (type == 3) {
                    STOP = 1;
                }
            }

            
        } else if (STOP == 1 && type == 3){
            // do nothing.
        }
	}
}

int cliConn (char *host, int port) {
    
    struct sockaddr_in name;
    struct hostent *hent;
    int sd;
    
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("(cliConn): socket() error");
        exit (-1);
    }
    
    if ((hent = gethostbyname (host)) == NULL)
        fprintf (stderr, "Host %s not found.\n", host);
    else
        bcopy (hent->h_addr, &name.sin_addr, hent->h_length);
    
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    
    /* connect port */
    if (connect (sd, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("(cliConn): connect() error");
        exit (-1);
    }
    
    return (sd);
}

int command_line(int sd) {
	char buffer[20], msg[50];
	char *ptr, *args[5];
	int i;
	size_t len;
	memset(msg, '\0', 50);
	while (1) {
		while(fgets(buffer, 50, stdin)) {
			for (i = 0; buffer[i] != '\n'; i++);
			buffer[i] = '\0';
			i = 0;
			ptr = strtok(buffer, " ");
			while (ptr != NULL) {
				args[i] = ptr;
				ptr = strtok(NULL, " ");
				i++;
			}
			if (!strcmp(args[0], "s")) {
				if (!strcmp(args[1], "start")) {
					if (i == 3)
						sprintf(msg, "%s:%d:%s:%s", params.clientid, params.priority, "start_movie", args[2]);
                    START = 1;
                    SEEK = 0;
                    STOP = 0;
                    if (atoi(args[2]) == 1) {
                        REPEAT = 1;
                    } else {
                        REPEAT = 0;
                    }
				}
				else if (!strcmp(args[1], "seek")) {
					if (i == 4)
						sprintf(msg, "%s:%d:%s:%s:%s", params.clientid, params.priority, "seek_movie", args[2], args[3]);
                    START = 0;
                    SEEK = 1;
                    STOP = 0;
                    REPEAT = 0;
				}
				else if (!strcmp(args[1], "stop")) {
					if (i == 2)
						sprintf(msg, "%s:%d:%s", params.clientid, params.priority, "stop_movie");
                    START = 0;
                    SEEK = 0;
                    STOP = 1;
                    REPEAT = 0;
				}
				if (*msg != '\0') {
					len = strlen(msg);
					len = htonl(len);
					write(sd, &len, sizeof(size_t));
					printf("the msg is: %s\n", msg);
					write(sd, msg, strlen(msg));
					memset(msg, 0, 50);
				}
			}
			else if(!strcmp(args[0], "q")) {
				// close(sd);
				return 1;
			}
		}
	}
}

int main (int argc, char const *argv[])
{
	int rc;
	// char buffer[20], msg[50];
	pthread_t listener;
    
    // INIT flags. 
    START = 0;
    SEEK = 0;
    STOP = 0;
    REPEAT = 0;
    
	char *req = "stop_movie:batman";
	rc = parse_args(argc, argv, &params);
	if (rc) exit(0);
  	int sd = cliConn ("localhost", 5050);
	rc = pthread_create(&listener, NULL, recv_listen, (void *) sd);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	rc = command_line(sd);
	if (rc) exit(0);		
  	return 0;
}


