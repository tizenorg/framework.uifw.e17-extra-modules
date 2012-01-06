#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "e_mod_comp_data.h"
#include "config.h"

#if COMP_LOGGER_BUILD_ENABLE
extern int comp_logger_type                        ;
extern Ecore_X_Atom ATOM_CM_LOG                    ;
#endif

extern Ecore_X_Atom ATOM_EFFECT_ENABLE             ;
extern Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE      ;
extern Ecore_X_Atom ATOM_WINDOW_EFFECT_CLIENT_STATE;
extern Ecore_X_Atom ATOM_WINDOW_EFFECT_STATE       ;
extern Ecore_X_Atom ATOM_WINDOW_EFFECT_TYPE        ;
extern Ecore_X_Atom ATOM_EFFECT_DEFAULT            ;
extern Ecore_X_Atom ATOM_EFFECT_NONE               ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM0            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM1            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM2            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM3            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM4            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM5            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM6            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM7            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM8            ;
extern Ecore_X_Atom ATOM_EFFECT_CUSTOM9            ;

extern E_Comp_Win       * _e_mod_comp_win_transient_parent_find    (E_Comp_Win * cw);
extern E_Comp_Effect_Type _e_mod_comp_get_effect_type              (Ecore_X_Atom *atom);
extern Eina_Bool          _e_mod_comp_win_check_visible            (E_Comp_Win *cw);

Eina_Bool    _e_mod_comp_is_quickpanel_window         (E_Border *bd);
Eina_Bool    _e_mod_comp_is_task_manager_window       (E_Border *bd);
Eina_Bool    _e_mod_comp_is_live_magazine_window       (E_Border * bd);
Eina_Bool    _e_mod_comp_is_lock_screen_window        (E_Border *bd);
Eina_Bool    _e_mod_comp_is_indicator_window          (E_Border *bd);
Eina_Bool    _e_mod_comp_is_isf_main_window           (E_Border *bd);
Eina_Bool    _e_mod_comp_is_isf_sub_window            (E_Border *bd);
Eina_Bool    _e_mod_comp_is_normal_window             (E_Border *bd);
Eina_Bool    _e_mod_comp_is_tooltip_window            (E_Border *bd);
Eina_Bool    _e_mod_comp_is_combo_window              (E_Border *bd);
Eina_Bool    _e_mod_comp_is_dnd_window                (E_Border *bd);
Eina_Bool    _e_mod_comp_is_desktop_window            (E_Border *bd);
Eina_Bool    _e_mod_comp_is_toolbar_window            (E_Border *bd);
Eina_Bool    _e_mod_comp_is_menu_window               (E_Border *bd);
Eina_Bool    _e_mod_comp_is_splash_window             (E_Border *bd);
Eina_Bool    _e_mod_comp_is_drop_down_menu_window     (E_Border *bd);
Eina_Bool    _e_mod_comp_is_notification_window       (E_Border *bd);
Eina_Bool    _e_mod_comp_is_utility_window            (E_Border *bd);
Eina_Bool    _e_mod_comp_is_popup_menu_window         (E_Border *bd);
Eina_Bool    _e_mod_comp_is_dialog_window             (E_Border *bd);

Eina_Bool    _e_mod_comp_is_application_launch        (E_Border *bd);
Eina_Bool    _e_mod_comp_is_application_close         (E_Border *bd);

void         _e_mod_comp_window_effect_policy         (E_Comp_Win *cw);
Eina_Bool    _e_mod_comp_window_restack_policy        (E_Comp_Win *cw, E_Comp_Win *cw2);
Eina_Bool    _e_mod_comp_window_rotation_policy       (E_Comp_Win *cw);
int          _e_mod_comp_shadow_policy                (E_Comp_Win *cw);

Eina_Bool
_e_mod_comp_is_quickpanel_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;
   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"QUICKPANEL",strlen("QUICKPANEL")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_task_manager_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"TASK_MANAGER",strlen("TASK_MANAGER")) != 0 ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( strncmp(name,"TASK_MANAGER",strlen("TASK_MANAGER")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_live_magazine_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"live-magazine",strlen("live-magazine")) != 0 ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( strncmp(name,"Live magazine",strlen("Live magazine")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_lock_screen_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"LOCK_SCREEN",strlen("LOCK_SCREEN")) != 0 ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( strncmp(name,"LOCK_SCREEN",strlen("LOCK_SCREEN")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_indicator_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"INDICATOR",strlen("INDICATOR")) != 0 ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( strncmp(name,"INDICATOR",strlen("INDICATOR")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_isf_main_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"ISF",strlen("ISF")) != 0 ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( strncmp(name,"Virtual Keyboard",strlen("Virtual Keyboard")) != 0  ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL ) return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_isf_sub_window(E_Border *bd)
{
   const char *name = NULL;
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   name = bd->client.icccm.name;
   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"ISF",strlen("ISF")) != 0 ) return EINA_FALSE;
   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NORMAL ) return EINA_FALSE;
   if ( name == NULL ) return EINA_FALSE;
   if ( (strncmp(name,"Key Magnifier",strlen("Key Magnifier")) == 0 )
        || (strncmp(name,"Prediction Window",strlen("Prediction Window")) == 0 )
        || (strncmp(name,"Setting Window",strlen("Setting Window")) == 0 )
        || (strncmp(name,"ISF Popup",strlen("ISF Popup")) == 0 ) ) return EINA_TRUE;
   return EINA_FALSE;
}

Eina_Bool
_e_mod_comp_is_normal_window(E_Border *bd)
{
   const char *clas = NULL;

   if (!bd) return EINA_FALSE;

   clas = bd->client.icccm.class;

   if ( clas == NULL ) return EINA_FALSE;
   if ( strncmp(clas,"NORMAL_WINDOW",strlen("NORMAL_WINDOW")) != 0 ) return EINA_FALSE;

   if ( ( bd->client.netwm.type == ECORE_X_WINDOW_TYPE_NORMAL)
        || ( bd->client.netwm.type == ECORE_X_WINDOW_TYPE_UNKNOWN ) ) return EINA_TRUE;

   return EINA_FALSE;
}

Eina_Bool
_e_mod_comp_is_tooltip_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLTIP )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_combo_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_COMBO )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_dnd_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DND )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_desktop_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DESKTOP )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_toolbar_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_TOOLBAR )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_menu_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_MENU )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_splash_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_SPLASH )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_drop_down_menu_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DROPDOWN_MENU )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_notification_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_NOTIFICATION )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_utility_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_UTILITY )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_popup_menu_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_POPUP_MENU )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_dialog_window(E_Border *bd)
{

   if (!bd) return EINA_FALSE;

   if ( bd->client.netwm.type != ECORE_X_WINDOW_TYPE_DIALOG )  return EINA_FALSE;

   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_application_launch(E_Border *bd)
{
   if (!bd) return EINA_FALSE;
   if ( _e_mod_comp_is_quickpanel_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_task_manager_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_lock_screen_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_indicator_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_isf_main_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_isf_sub_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_normal_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_tooltip_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_combo_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_dnd_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_desktop_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_toolbar_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_splash_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_drop_down_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_notification_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_utility_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_popup_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_dialog_window(bd) ) return EINA_FALSE;
   if (_e_mod_comp_is_live_magazine_window(bd)) return EINA_FALSE;
   return EINA_TRUE;
}

Eina_Bool
_e_mod_comp_is_application_close(E_Border *bd)
{
   if (!bd) return EINA_FALSE;
   if ( _e_mod_comp_is_quickpanel_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_task_manager_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_lock_screen_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_indicator_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_isf_main_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_isf_sub_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_normal_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_tooltip_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_combo_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_dnd_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_desktop_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_toolbar_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_splash_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_drop_down_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_notification_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_utility_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_popup_menu_window(bd) ) return EINA_FALSE;
   if ( _e_mod_comp_is_dialog_window(bd) ) return EINA_FALSE;
   if (_e_mod_comp_is_live_magazine_window(bd)) return EINA_FALSE;
   return EINA_TRUE;
}

void _e_mod_comp_window_effect_policy( E_Comp_Win *cw )
{
   unsigned int    effect_enable = 0;
   int             num = 0;
   Ecore_X_Atom   *atoms = NULL;
   Ecore_X_Window  win;

   unsigned int val = 0;
   int          ret = -1;

   if ( cw->bd)
     win = cw->bd->client.win;
   else
     win = cw->win;

   ret =  ecore_x_window_prop_card32_get(win,  ATOM_WINDOW_EFFECT_ENABLE, &val, 1);
   if ( ret != -1 )
     {
        cw->animatable = val;
     }
   else
     {
        effect_enable = 1;
        cw->animatable = effect_enable;
        ecore_x_window_prop_card32_set(win,  ATOM_WINDOW_EFFECT_ENABLE, &effect_enable, 1);
     }

   num = ecore_x_window_prop_atom_list_get(win, ATOM_WINDOW_EFFECT_TYPE, &atoms);
   if ( num != 6 )
     {
        Ecore_X_Atom window_effect_type_list[] = {ATOM_EFFECT_DEFAULT,ATOM_EFFECT_DEFAULT,ATOM_EFFECT_DEFAULT,ATOM_EFFECT_DEFAULT,ATOM_EFFECT_DEFAULT,ATOM_EFFECT_DEFAULT};

        cw->show_effect = COMP_EFFECT_DEFAULT;
        cw->hide_effect = COMP_EFFECT_DEFAULT;
        cw->restack_effect = COMP_EFFECT_DEFAULT;
        cw->rotation_effect = COMP_EFFECT_DEFAULT;
        cw->focusin_effect = COMP_EFFECT_DEFAULT;
        cw->focusout_effect = COMP_EFFECT_DEFAULT;

        ecore_x_window_prop_atom_set(win,ATOM_WINDOW_EFFECT_TYPE, window_effect_type_list , 6 );
     }
   else
     {
        cw->show_effect = _e_mod_comp_get_effect_type(&atoms[0]);
        cw->hide_effect = _e_mod_comp_get_effect_type(&atoms[1]);
        cw->restack_effect = _e_mod_comp_get_effect_type(&atoms[2]);
        cw->rotation_effect = _e_mod_comp_get_effect_type(&atoms[3]);
        cw->focusin_effect = _e_mod_comp_get_effect_type(&atoms[4]);
        cw->focusout_effect = _e_mod_comp_get_effect_type(&atoms[5]);
     }

   if ( num > 0 ) free(atoms);
}

Eina_Bool
_e_mod_comp_window_restack_policy(E_Comp_Win *cw,
                                  E_Comp_Win *cw2)
{
   if ( cw == NULL || cw2 == NULL ) return EINA_FALSE;

   if (!(cw->c->animatable)
       || !(cw->visible)
       || !(cw2->visible)
       || !(cw->animatable)
       || (cw->restack_effect == COMP_EFFECT_NONE)
       || !(cw->x == 0 && cw->y == 0
            && cw->w == cw->c->man->w
            && cw->h == cw->c->man->h)
      )
     {
        return EINA_FALSE;
     }

   E_Comp_Win *tp = _e_mod_comp_win_transient_parent_find(cw);
   E_Comp_Win *tp2 = _e_mod_comp_win_transient_parent_find(cw2);
   if ((tp) && (tp2) && (tp->win == tp2->win))
     {
        return EINA_FALSE;
     }

   if (!_e_mod_comp_is_isf_main_window(cw->bd)
       && !_e_mod_comp_is_isf_sub_window(cw->bd)
       && !_e_mod_comp_is_utility_window(cw->bd)
       && !_e_mod_comp_is_indicator_window(cw->bd)
       && !_e_mod_comp_is_indicator_window(cw2->bd)
       && !_e_mod_comp_is_tooltip_window(cw->bd)
       && !_e_mod_comp_is_combo_window(cw->bd)
       && !_e_mod_comp_is_dnd_window(cw->bd)
       && !_e_mod_comp_is_popup_menu_window(cw->bd)
       && !_e_mod_comp_is_normal_window(cw->bd)
       && !_e_mod_comp_is_quickpanel_window(cw->bd))
     {
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

Eina_Bool _e_mod_comp_window_rotation_policy( E_Comp_Win *cw )
{

   Eina_Bool supported_group = EINA_TRUE;
   const char *file = NULL;
   const char *group = NULL;

   if (!cw) return EINA_FALSE;

   edje_object_file_get(cw->shobj, &file, &group);
   if ( ( strcmp(group, "shadow_fade") != 0 )
       && ( strcmp(group, "shadow_twist") !=0 ) )
     {
        supported_group = EINA_FALSE;
     }

   if ( ( cw->visible ) && ( cw->bd ) && ( supported_group == EINA_TRUE )
       && ( _e_mod_comp_win_check_visible(cw) == EINA_TRUE )

       && ( cw->animatable == EINA_TRUE )
       && ( cw->rotation_effect !=  COMP_EFFECT_NONE )

       && ( evas_object_visible_get(cw->shobj) == EINA_TRUE )
       && ( _e_mod_comp_is_indicator_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_isf_main_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_isf_sub_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_quickpanel_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_normal_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_task_manager_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_lock_screen_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_tooltip_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_combo_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_dnd_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_popup_menu_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_utility_window(cw->bd) == EINA_FALSE )
       && ( _e_mod_comp_is_dialog_window(cw->bd)  == EINA_FALSE )
       && ( cw->c->animatable == EINA_TRUE) )
     {
        return EINA_TRUE;
     }
   else
     {
        return EINA_FALSE;
     }
}

int
_e_mod_comp_shadow_policy( E_Comp_Win *cw )
{
   int ok = 0;

   if ( cw == NULL ) return ok;
   else
     {
        if      (_e_mod_comp_is_quickpanel_window(cw->bd))                                   ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_task_manager_window(cw->bd))                                 ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "taskmgr"    );
        else if (_e_mod_comp_is_lock_screen_window(cw->bd))                                  ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "lockscreen" );
        else if (_e_mod_comp_is_indicator_window(cw->bd))                                    ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "indicator"  );
        else if (_e_mod_comp_is_isf_main_window(cw->bd))                                     ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow"     );
        else if (_e_mod_comp_is_isf_sub_window(cw->bd))                                      ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow"     );
        else if (_e_mod_comp_is_normal_window(cw->bd))                                       ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow"     );
        else if (_e_mod_comp_is_tooltip_window(cw->bd))                                      ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow"     );
        else if (_e_mod_comp_is_combo_window(cw->bd))                                        ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_dnd_window(cw->bd))                                          ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_desktop_window(cw->bd))                                      ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_toolbar_window(cw->bd))                                      ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_menu_window(cw->bd))                                         ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_splash_window(cw->bd))                                       ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_drop_down_menu_window(cw->bd))                               ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_notification_window(cw->bd))                                 ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "dialog"     );
        else if (_e_mod_comp_is_utility_window(cw->bd))                                      ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "no-effect"  );
        else if (_e_mod_comp_is_popup_menu_window(cw->bd))                                   ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow"     );
        else if (_e_mod_comp_is_dialog_window(cw->bd))                                       ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "dialog"     );
        else                                                                                 ok = edje_object_file_set(cw->shobj, _comp_mod->conf->shadow_file, "shadow_twist");
     }
   return ok;
}

