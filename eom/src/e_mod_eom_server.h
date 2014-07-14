#ifndef _E_MOD_EOM_SERVER_H_
#define _E_MOD_EOM_SERVER_H_

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XIproto.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XKB.h>
#include <X11/Xlib.h>

#undef alloca
#include <e.h>
#include <e_randr.h>

#include "e_mod_eom_privates.h"

typedef struct _EOM_
{
   /* scrn conf configuration */
   eom_output_mode_e        default_dispmode;
   Eina_Bool            isPopUpEnabled;

   /* scrn conf preferred size */
   int                  hdmi_preferred_w;
   int                  hdmi_preferred_h;
   int                  virtual_preferred_w;
   int                  virtual_preferred_h;

   Ecore_Event_Handler *randr_output_handler;

   /* variables to set XRROutputProperty */
   RROutput             output;
} EOM;

Eina_Bool eom_server_init(E_Module *m);
void eom_server_deinit(E_Module *m);

int eom_server_send_notify (eom_notify_type_e type, int sc_output_id);

int scrnconf_ext_update_get_perperty(Display *dpy, char *str_output_id, char *str_stat, char *str_res, char *str_dispmode);

#endif /* _E_MOD_EOM_SERVER_H_ */
