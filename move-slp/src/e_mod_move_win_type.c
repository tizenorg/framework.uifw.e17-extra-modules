#include "e.h"
#include "e_mod_data.h"
#include "e_mod_move_win_type.h"

Eina_Bool _e_mod_move_is_app_tray_window(E_Border *bd);
Eina_Bool _e_mod_move_is_indicator_window(E_Border *bd);
Eina_Bool _e_mod_move_is_quickpanel_window(E_Border *bd);
Eina_Bool _e_mod_move_is_quickpanel_minicontroller_window(E_Border *bd);

E_Move_Win_Type
e_mod_move_win_type_get(E_Border *bd)
{
   if (!bd) return WIN_NORMAL;
   if (_e_mod_move_is_app_tray_window(bd)) return WIN_APP_TRAY;
   else if (_e_mod_move_is_indicator_window(bd)) return WIN_INDICATOR;
   else if (_e_mod_move_is_quickpanel_window(bd)) return WIN_QUICKPANEL_BASE;
   else if (_e_mod_move_is_quickpanel_minicontroller_window(bd)) return WIN_QUICKPANEL_MINICONTROLLER;
   else return WIN_NORMAL;
}

Eina_Bool
_e_mod_move_is_app_tray_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"APP_TRAY",strlen("APP_TRAY"))!= 0) return EINA_FALSE;
   if (name == NULL) return EINA_FALSE;
   if (strncmp(name,"APP_TRAY",strlen("APP_TRAY"))!= 0) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_move_is_indicator_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"INDICATOR",strlen("INDICATOR")) != 0) return EINA_FALSE;
   if (name == NULL) return EINA_FALSE;
   if (strncmp(name,"INDICATOR",strlen("INDICATOR")) != 0) return EINA_FALSE;
   if (bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_move_is_quickpanel_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;
   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"QUICKPANEL_BASE",strlen("QUICKPANEL_BASE")) != 0) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_move_is_quickpanel_minicontroller_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;
   if (clas == NULL) return EINA_FALSE;
   if (strncmp(clas,"QUICKPANEL",strlen("QUICKPANEL")) != 0) return EINA_FALSE;

   return EINA_TRUE;
}
