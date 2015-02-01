#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "virt_mon_set.h"

#define VIRT_MON_CHK(cond) {if (!(cond)) { SLOG(LOG_DEBUG, "DEVICEMGR", "[%s] : '%s' failed.\n", __func__, #cond); return; }}

/* set the virtual monitor to connect */
void
virtual_monitor_set_connect (Display *dpy)
{
    VIRT_MON_CHK (dpy);

    Window win = DefaultRootWindow(dpy);
	XEvent xev;
	Atom virt_mon_atom = None;

    virt_mon_atom = XInternAtom (dpy, STR_ATOM_VIRT_MON_SET, False);

	xev.xclient.window = win;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = virt_mon_atom;
	xev.xclient.format = 32;
	xev.xclient.data.s[0] = VM_CMD_SET;
	xev.xclient.data.s[1] = 0;
	xev.xclient.data.s[2] = 0;
	xev.xclient.data.s[3] = 0;
	xev.xclient.data.s[4] = 0;

	XSendEvent(dpy, win, False, StructureNotifyMask, &xev);
	XSync(dpy, False);
}

/* set the virtual monitor to disconnect */
void
virtual_monitor_set_disconnect (Display *dpy)
{
    VIRT_MON_CHK (dpy);

    Window win = DefaultRootWindow(dpy);
	XEvent xev;
	Atom virt_mon_atom = None;

    virt_mon_atom = XInternAtom (dpy, STR_ATOM_VIRT_MON_SET, False);

	xev.xclient.window = win;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = virt_mon_atom;
	xev.xclient.format = 32;
	xev.xclient.data.s[0] = VM_CMD_UNSET;
	xev.xclient.data.s[1] = 0;
	xev.xclient.data.s[2] = 0;
	xev.xclient.data.s[3] = 0;
	xev.xclient.data.s[4] = 0;

	XSendEvent(dpy, win, False, StructureNotifyMask, &xev);
	XSync(dpy, False);
}


