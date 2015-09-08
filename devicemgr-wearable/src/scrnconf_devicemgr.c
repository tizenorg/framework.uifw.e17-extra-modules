#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include "scrnconf_devicemgr.h"

#define SCRNCONF_CHK(cond, val) {if (!(cond)) { fprintf (stderr, "[%s] : '%s' failed.\n", __func__, #cond); return val; }}

int
scrnconf_ext_send_status (Display *dpy, Utilx_Scrnconf_Status sc_stat, Utilx_Scrnconf_Dispmode sc_dispmode)
{
    SCRNCONF_CHK (dpy, 0);

    Window win = DefaultRootWindow(dpy);
	XEvent xev;
	Atom scrnconf_atom = None;

    scrnconf_atom = XInternAtom (dpy, STR_ATOM_SCRNCONF_STATUS, False);

	xev.xclient.window = win;
	xev.xclient.type = ClientMessage;
	xev.xclient.message_type = scrnconf_atom;
	xev.xclient.format = 32;
	xev.xclient.data.s[0] = sc_stat;
	xev.xclient.data.s[1] = sc_dispmode;

	XSendEvent(dpy, win, False, StructureNotifyMask, &xev);
	XSync(dpy, False);

    return 1;
}

int
scrnconf_ext_update_get_perperty (Display *dpy, char *str_output, char *str_stat, char *str_res, char *str_dispmode)
{
    SCRNCONF_CHK (dpy, 0);

    Window win = DefaultRootWindow(dpy);
    XTextProperty xtp;
	Atom scrnconf_atom = None;
    char * str = NULL;
    int size = 0;

    scrnconf_atom = XInternAtom (dpy, STR_ATOM_SCRNCONF_INFO, False);

    size = strlen(str_output) + strlen(str_stat) + strlen(str_res) + strlen(str_dispmode) + 4;

    str = calloc (size, sizeof(char));
    if (str)
      {
         snprintf (str, size, "%s,%s,%s,%s", str_output, str_stat, str_res, str_dispmode);

         xtp.value = (unsigned char *)str;
         xtp.format = 8;
         xtp.encoding = XA_STRING;
         xtp.nitems = size;
         XSetTextProperty (dpy, win, &xtp, scrnconf_atom);

         free (str);
      }
    return 1;
}
