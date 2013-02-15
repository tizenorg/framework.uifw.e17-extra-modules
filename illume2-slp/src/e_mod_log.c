#include "e_illume_private.h"
#include "e_mod_log.h"

/* local function prototypes */
static int _e_mod_log_init_atom(void);
static void _e_mod_log_handlers_add(void);
static void _e_mod_log_handlers_del(void);
static Eina_Bool _e_mod_log_cb_window_property(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_log_property_log_change(Ecore_X_Event_Window_Property *ev);

/* local variables */
static Ecore_X_Atom E_ILLUME_ATOM_ILLUME_LOG;
static Eina_List *_log_hdls = NULL;

/* external variables */
int _e_illume_logger_type = LT_NOTHING;


int
e_mod_log_init(void)
{
   if (!_e_mod_log_init_atom())
     {
        /* creating atom failed, bail out */
        printf ("Cannot create atom\n");
        return 0;
     }

   /* add our event handlers */
   _e_mod_log_handlers_add();

   return 1;
}

int
e_mod_log_shutdown(void)
{
   /* remove our event handlers */
   _e_mod_log_handlers_del();

   return 1;
}

static int
_e_mod_log_init_atom(void)
{
   E_ILLUME_ATOM_ILLUME_LOG = ecore_x_atom_get ("_E_ILLUME_LOG");
   if(!E_ILLUME_ATOM_ILLUME_LOG)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_LOG Atom...\n");
        return 0;
     }

   return 1;
}

static void
_e_mod_log_handlers_add(void)
{
   _log_hdls =
     eina_list_append(_log_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                              _e_mod_log_cb_window_property,
                                              NULL));
}

static void
_e_mod_log_handlers_del(void)
{
   Ecore_Event_Handler *hdl;

   /* remove the ecore event handlers */
   EINA_LIST_FREE(_log_hdls, hdl)
     ecore_event_handler_del(hdl);
}

static Eina_Bool
_e_mod_log_cb_window_property(void *data __UNUSED__,
                              int   type __UNUSED__,
                              void *event)
{
   Ecore_X_Event_Window_Property *ev;
   ev = event;

   if (ev->atom == E_ILLUME_ATOM_ILLUME_LOG)
     {
        _e_mod_log_property_log_change(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_log_property_log_change(Ecore_X_Event_Window_Property *ev)
{
   int ret, count;
   unsigned int info_type;
   unsigned char *prop_data = NULL;

   info_type = 0;
   ret = ecore_x_window_prop_property_get(ev->win, E_ILLUME_ATOM_ILLUME_LOG, ECORE_X_ATOM_CARDINAL, 32, &prop_data, &count);
   if (ret && prop_data)
     {
        fprintf(stdout, "[E17-ILLUME2] %s(%d) _e_illume_logger_type: ",  __func__, __LINE__);

        if      (_e_illume_logger_type == LT_NOTHING) fprintf(stdout, "LT_NOTHING ");
        else if (_e_illume_logger_type == LT_ALL    ) fprintf(stdout, "LT_ALL "    );
        else
          {
             if ((_e_illume_logger_type & LT_STACK) == LT_STACK) fprintf(stdout, "LT_STACK, ");
             if ((_e_illume_logger_type & LT_NOTIFICATION) == LT_NOTIFICATION) fprintf(stdout, "LT_NOTIFICATION, ");
             if ((_e_illume_logger_type & LT_NOTIFICATION_LEVEL) == LT_NOTIFICATION_LEVEL) fprintf(stdout, "LT_NOTIFICATION_LEVEL, ");
             if ((_e_illume_logger_type & LT_VISIBILITY) == LT_VISIBILITY) fprintf(stdout, "LT_VISIBILITY, ");
             if ((_e_illume_logger_type & LT_LOCK_SCREEN) == LT_LOCK_SCREEN) fprintf(stdout, "LT_LOCK_SCREEN, ");
             if ((_e_illume_logger_type & LT_ANGLE) == LT_ANGLE) fprintf(stdout, "LT_ANGLE, ");
             if ((_e_illume_logger_type & LT_TRANSIENT_FOR) == LT_TRANSIENT_FOR) fprintf(stdout, "LT_TRANSIENT_FOR, ");
             if ((_e_illume_logger_type & LT_QUICKPANEL) == LT_QUICKPANEL) fprintf(stdout, "LT_QUICKPANEL, ");
             if ((_e_illume_logger_type & LT_KEYBOARD) == LT_KEYBOARD) fprintf(stdout, "LT_KEYBOARD, ");
             if ((_e_illume_logger_type & LT_ICONIFY) == LT_ICONIFY) fprintf(stdout, "LT_ICONIFY, ");
             if ((_e_illume_logger_type & LT_DUAL_DISPLAY) == LT_DUAL_DISPLAY) fprintf(stdout, "LT_DUAL_DISPLAY, ");
             if ((_e_illume_logger_type & LT_AIA) == LT_AIA) fprintf(stdout, "LT_AIA, ");
             if ((_e_illume_logger_type & LT_INDICATOR) == LT_INDICATOR) fprintf(stdout, "LT_INDICATOR, ");
          }

        fprintf(stdout, "-> ");

        memcpy(&_e_illume_logger_type, prop_data, sizeof(unsigned int));

        if      (_e_illume_logger_type == LT_NOTHING ) fprintf(stdout, "LT_NOTHING\n");
        else if (_e_illume_logger_type == LT_ALL     ) fprintf(stdout, "LT_ALL\n"    );
        else
          {
             if ((_e_illume_logger_type & LT_STACK) == LT_STACK)fprintf (stdout, "LT_STACK, ");
             if ((_e_illume_logger_type & LT_NOTIFICATION) == LT_NOTIFICATION)fprintf (stdout, "LT_NOTIFICATION, ");
             if ((_e_illume_logger_type & LT_NOTIFICATION_LEVEL) == LT_NOTIFICATION_LEVEL) fprintf(stdout, "LT_NOTIFICATION_LEVEL, ");
             if ((_e_illume_logger_type & LT_VISIBILITY) == LT_VISIBILITY) fprintf(stdout, "LT_VISIBILITY, ");
             if ((_e_illume_logger_type & LT_LOCK_SCREEN) == LT_LOCK_SCREEN) fprintf(stdout, "LT_LOCK_SCREEN, ");
             if ((_e_illume_logger_type & LT_ANGLE) == LT_ANGLE) fprintf(stdout, "LT_ANGLE, ");
             if ((_e_illume_logger_type & LT_TRANSIENT_FOR) == LT_TRANSIENT_FOR) fprintf(stdout, "LT_TRANSIENT_FOR, ");
             if ((_e_illume_logger_type & LT_QUICKPANEL) == LT_QUICKPANEL) fprintf(stdout, "LT_QUICKPANEL, ");
             if ((_e_illume_logger_type & LT_KEYBOARD) == LT_KEYBOARD) fprintf(stdout, "LT_KEYBOARD, ");
             if ((_e_illume_logger_type & LT_ICONIFY) == LT_ICONIFY) fprintf(stdout, "LT_ICONIFY, ");
             if ((_e_illume_logger_type & LT_DUAL_DISPLAY) == LT_DUAL_DISPLAY) fprintf(stdout, "LT_DUAL_DISPLAY, ");
             if ((_e_illume_logger_type & LT_AIA) == LT_AIA) fprintf(stdout, "LT_AIA, ");
             if ((_e_illume_logger_type & LT_INDICATOR) == LT_INDICATOR) fprintf(stdout, "LT_INDICATOR, ");

             fprintf(stdout, "\n");
          }
     }

   if (prop_data) free(prop_data);

   return EINA_TRUE;
}

