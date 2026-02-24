#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "herbengineC3D.c"
uint32_t *scaled_pixels = malloc(WINDOW_WIDTH * WINDOW_HEIGHT * 4);

XImage *scaled_image = XCreateImage(
    display,
    DefaultVisual(display, screen),
    DefaultDepth(display, screen),
    ZPixmap,
    0,
    (char *)scaled_pixels,
    WINDOW_WIDTH,
    WINDOW_HEIGHT,
    32,
    0
);
int main() {
    // Open X display
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Unable to open X display\n");
        return 1;
    }

    // Create a window
    int screen = DefaultScreen(display);
	Window window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0,
                                    WINDOW_WIDTH, WINDOW_HEIGHT, 1,
                                    BlackPixel(display, screen), BlackPixel(display, screen));
    XMapWindow(display, window);

	// hide the mouse
	XGrabPointer(display, window, True,
             ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
             GrabModeAsync, GrabModeAsync,
             window, None, CurrentTime);
	Pixmap blank;
	XColor dummy;
	char data[1] = {0};

	blank = XCreateBitmapFromData(display, window, data, 1, 1);
	Cursor cursor = XCreatePixmapCursor(display, blank, blank, &dummy, &dummy, 0, 0);
	XDefineCursor(display, window, cursor);
	mouse_defined = 1;

	// input
	XSelectInput(display, window,
		ExposureMask |
		KeyPressMask |
		KeyReleaseMask |
		ButtonPressMask |
		ButtonReleaseMask
	);
	
    // Create a graphics context
    GC gc = XCreateGC(display, window, 0, NULL);

    // Create an XImage for pixel manipulation
    XImage *image = XCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap,
                                 0, (char *)malloc(WIDTH * HEIGHT * 4), WIDTH, HEIGHT, 32, 0);

    // Create a pixel array
    pixels = (uint32_t *)image->data;

    // Main loop to update the window color
	init_stuff();
    w     = XKeysymToKeycode(display, XK_w);
    a     = XKeysymToKeycode(display, XK_a);
    s     = XKeysymToKeycode(display, XK_s);
    d     = XKeysymToKeycode(display, XK_d);
    shift = XKeysymToKeycode(display, XK_Shift_L);
    space = XKeysymToKeycode(display, XK_space);
    control = XKeysymToKeycode(display, XK_Control_L);
    escape = XKeysymToKeycode(display, XK_Escape);
    one     = XKeysymToKeycode(display, XK_1);
    two     = XKeysymToKeycode(display, XK_2);
    three     = XKeysymToKeycode(display, XK_3);
    four     = XKeysymToKeycode(display, XK_4);
    five     = XKeysymToKeycode(display, XK_5);
    six     = XKeysymToKeycode(display, XK_6);
    seven     = XKeysymToKeycode(display, XK_7);
    eight     = XKeysymToKeycode(display, XK_8);
    nine     = XKeysymToKeycode(display, XK_9);

	clock_gettime(CLOCK_MONOTONIC, &last);

	// ENTRY POINT
	while(1) {
		while (XPending(display)) {
			// --- Handle events ---
			XEvent event;
			XNextEvent(display, &event);

			switch (event.type) {
				case KeyPress: {
					KeyCode code = event.xkey.keycode;
					keys[code] = 1;
					break;
			    }
				case KeyRelease: {
					KeyCode code = event.xkey.keycode;
					keys[code] = 0;
					break;
			    }
				case ButtonPress: {
					int button_type = event.xbutton.button;
					if (button_type == 1) {
						mouse_left_click = 1;
					}
					else if (button_type == 3) {
						mouse_right_click = 1;
					}
					break;
			    }
				case ButtonRelease: {
					mouse_left_click = 0;
					mouse_right_click = 0;
					break;
				}
				case DestroyNotify: {
					break;
				}
			}
		}

		if (hold_mouse) {
			// handle the mouse: 
			if (! mouse_defined) {
				XDefineCursor(display, window, cursor);
				mouse_defined = 1;
				XWarpPointer(display, None, window,
				 0, 0, 0, 0,
				 (WIDTH / 2), (HEIGHT / 2));
			}
			Window root, child;
			int root_x, root_y;
			unsigned int mask;

			XQueryPointer(display, window,
						  &root, &child,
						  &root_x, &root_y,
						  &mouse.x, &mouse.y,
						  &mask);

			// Warp pointer back to center
			XWarpPointer(display, None, window,
					 0, 0, 0, 0,
					 WIDTH / 2, HEIGHT / 2);
		}
		else {
			if (mouse_defined) {
				XUndefineCursor(display, window);
				mouse_defined = 0;
			}
		}

		// --- Update ---
		update();

		// --- Render ---
		// --- Nearest neighbor scaling ---
		float scale_x = (float)WINDOW_WIDTH / WIDTH;
		float scale_y = (float)WINDOW_HEIGHT / HEIGHT;
		float scale   = scale_x < scale_y ? scale_x : scale_y;

		int draw_w = WIDTH * scale;
		int draw_h = HEIGHT * scale;

		int offset_x = (WINDOW_WIDTH - draw_w) / 2;
		int offset_y = (WINDOW_HEIGHT - draw_h) / 2;

		// Clear screen
		//memset(scaled_pixels, 0, WINDOW_WIDTH * WINDOW_HEIGHT * 4);

		// Scale pixels
		for (int y = 0; y < draw_h; y++) {
			int src_y = y / scale;
			for (int x = 0; x < draw_w; x++) {
				int src_x = x / scale;

				scaled_pixels[(y + offset_y) * WINDOW_WIDTH + (x + offset_x)] =
					pixels[src_y * WIDTH + src_x];
			}
		}

		// Draw scaled image
		XPutImage(display, window, gc,
				  scaled_image,
				  0, 0,
				  0, 0,
				  WINDOW_WIDTH, WINDOW_HEIGHT);

		XFlush(display);

		// --- Frame timing ---
		clock_gettime(CLOCK_MONOTONIC, &now);

		long elapsed = (now.tv_sec - last.tv_sec) * 1000000000L + (now.tv_nsec - last.tv_nsec);

		if (elapsed < FRAME_TIME_NS) {
			struct timespec sleep_time;
			sleep_time.tv_sec = 0;
			sleep_time.tv_nsec = FRAME_TIME_NS - elapsed;
			nanosleep(&sleep_time, NULL);
		}

		clock_gettime(CLOCK_MONOTONIC, &last);
		
	}

    // Cleanup
	cleanup();
    XDestroyImage(image);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}
