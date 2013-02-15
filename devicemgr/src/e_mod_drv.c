#include "e.h"
#include "e_randr.h"
#include "e_mod_main.h"
#include "e_mod_drv.h"

#define STR_XRR_LVDS_FUNCTION "XRR_PROPERTY_LVDS_FUNCTION"

#ifdef  _MAKE_ATOM
# undef _MAKE_ATOM
#endif

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get (s);                      \
        if (!a)                                       \
          fprintf (stderr,                             \
                   "[E-devmgr] ##s creation failed.\n"); \
   } while (0)

/* lvds function */
typedef enum
{
    XRR_OUTPUT_LVDS_FUNC_NULL,             /* null */
    XRR_OUTPUT_LVDS_FUNC_INIT_VIRTUAL,     /* virutal output connect/disconnect */
    XRR_OUTPUT_LVDS_FUNC_HIBERNATION,      /* hibernation on / off */
    XRR_OUTPUT_LVDS_FUNC_ACCESSIBLILITY,   /* accessibility */
} XRROutputPropLvdsFunc;


void
e_mod_drv_virt_mon_set (int cmd)
{
   printf ("[DeviceMgr]: set the virtual output connect/disconnect\n");

   E_Randr_Output_Info *output_info = NULL;
   Eina_List *l_output;
   Eina_Bool found_output = EINA_FALSE;
   Ecore_X_Randr_Output output_xid[1] = {0};
   Ecore_X_Atom lvds_func;
   int value[2];

   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {
        if (output_info == NULL)
            continue;

        if (!strcmp (output_info->name, "LVDS1"))
          {
             output_xid[0] = output_info->xid;
             found_output = EINA_TRUE;
          }
     }

   if (!found_output)
     {
        fprintf (stderr, "[DeviceMgr]: fail to initialize the virtual output\n");
        goto set_fail;
     }

   ecore_x_grab ();

   _MAKE_ATOM (lvds_func, STR_XRR_LVDS_FUNCTION);

   value[0] = XRR_OUTPUT_LVDS_FUNC_INIT_VIRTUAL;
   value[1] = cmd;

   /* no ecore x API for XRRChangeOutputProperty */
   XRRChangeOutputProperty (ecore_x_display_get (), output_xid[0], lvds_func, XA_INTEGER, 32,
                           PropModeReplace, (unsigned char *)&value, 2);

   /* replay through XSendMessage */

   ecore_x_ungrab ();
   ecore_x_sync ();
   return;

set_fail:
   ecore_x_ungrab ();
}

void
e_mod_drv_hib_set (int cmd)
{
   printf ("[DeviceMgr]: set the hibernation on/off\n");

   E_Randr_Output_Info *output_info = NULL;
   Eina_List *l_output;
   Eina_Bool found_output = EINA_FALSE;
   Ecore_X_Randr_Output output_xid[1] = {0};
   Ecore_X_Atom lvds_func;
   int value[2];

   EINA_LIST_FOREACH (e_randr_screen_info.rrvd_info.randr_info_12->outputs, l_output, output_info)
     {
        if (output_info == NULL)
            continue;

        if (!strcmp (output_info->name, "LVDS1"))
          {
             output_xid[0] = output_info->xid;
             found_output = EINA_TRUE;
          }
     }

   if (!found_output)
     {
        fprintf (stderr, "[DeviceMgr]: fail to initialize the virtual output\n");
        goto set_fail;
     }

   ecore_x_grab ();

   _MAKE_ATOM (lvds_func, STR_XRR_LVDS_FUNCTION);

   value[0] = XRR_OUTPUT_LVDS_FUNC_HIBERNATION;
   value[1] = cmd;

   /* no ecore x API for XRRChangeOutputProperty */
   XRRChangeOutputProperty (ecore_x_display_get (), output_xid[0], lvds_func, XA_INTEGER, 32,
                           PropModeReplace, (unsigned char *)&value, 2);

   /* replay through XSendMessage */

   ecore_x_ungrab ();
   ecore_x_sync ();
   return;

set_fail:
   ecore_x_ungrab ();
}

