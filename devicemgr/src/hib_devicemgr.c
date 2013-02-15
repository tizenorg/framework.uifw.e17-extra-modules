#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "hib_set.h"

#define HIB_CHK(cond) {if (!(cond)) { fprintf (stderr, "[%s] : '%s' failed.\n", __func__, #cond); return; }}

void
hibernation_set (Display *dpy)
{
    HIB_CHK (dpy);

    Window win = DefaultRootWindow(dpy);
	XEvent xev;
	Atom hib_atom = None;

    hib_atom = XInternAtom (dpy, STR_ATOM_HIB_SET, False);

	xev.xclient.window = win;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = hib_atom;
	xev.xclient.format = 32;
	xev.xclient.data.s[0] = HIB_CMD_SET;
	xev.xclient.data.s[1] = 0;
	xev.xclient.data.s[2] = 0;
	xev.xclient.data.s[3] = 0;
	xev.xclient.data.s[4] = 0;

	XSendEvent(dpy, win, False, StructureNotifyMask, &xev);
	XSync(dpy, False);
}

void
hibernation_unset (Display *dpy)
{
    HIB_CHK (dpy);

    Window win = DefaultRootWindow(dpy);
	XEvent xev;
	Atom hib_atom = None;

    hib_atom = XInternAtom (dpy, STR_ATOM_HIB_SET, False);

	xev.xclient.window = win;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = hib_atom;
	xev.xclient.format = 32;
	xev.xclient.data.s[0] = HIB_CMD_UNSET;
	xev.xclient.data.s[1] = 0;
	xev.xclient.data.s[2] = 0;
	xev.xclient.data.s[3] = 0;
	xev.xclient.data.s[4] = 0;

	XSendEvent(dpy, win, False, StructureNotifyMask, &xev);
	XSync(dpy, False);
}

