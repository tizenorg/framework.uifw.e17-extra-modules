#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_policy.h"

/* local subsystem functions */
static E_Comp_Win *_transient_parent_find(E_Comp_Win *cw);

/* local subsystem globals */
static Eina_Hash *shadow_hash = NULL;

/* externally accessible functions */
EINTERN int
e_mod_comp_policy_init(void)
{
   if (!shadow_hash) shadow_hash = eina_hash_string_superfast_new(NULL);

   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UNKNOWN),        "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DESKTOP),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DOCK),           "shadow_fade"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLBAR),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENU),           "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_UTILITY),        "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_SPLASH),         "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DIALOG),         "dialog"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NORMAL),         "shadow_fade"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DROPDOWN_MENU),  "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_POPUP_MENU),     "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TOOLTIP),        "shadow"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_NOTIFICATION),   "dialog"            );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_COMBO),          "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DND),            "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MENUSCREEN),     "home_screen"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL_BASE),"quickpanel"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_QUICKPANEL),     "quickpanel"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TASKMANAGER),    "taskmgr"           );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LIVEMAGAZINE),   "home_screen"       );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_LOCKSCREEN),     "lockscreen"        );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_INDICATOR),      "indicator"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_TICKERNOTI),     "dialog_without_dim");
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_DEBUGGING_INFO), "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_APPTRAY),        "app_tray"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_MINI_APPTRAY),   "app_tray"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_VOLUME),         "dialog_without_dim");
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_BACKGROUND),     "no-effect"         );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_KEYBOARD),   "keyboard"          );
   eina_hash_add(shadow_hash, e_util_winid_str_get(E_COMP_WIN_TYPE_ISF_SUB),        "no-effect"         );

   return 1;
}

EINTERN int
e_mod_comp_policy_shutdown(void)
{
   if (shadow_hash) eina_hash_free(shadow_hash);
   shadow_hash = NULL;
   return 1;
}

EINTERN Eina_Bool
e_mod_comp_policy_app_launch_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   if (TYPE_NORMAL_CHECK(cw) && SIZE_EQUAL_TO_ROOT(cw))
     return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_policy_app_close_check(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   if (TYPE_NORMAL_CHECK(cw) && SIZE_EQUAL_TO_ROOT(cw))
     return EINA_TRUE;
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_policy_win_restack_check(E_Comp_Win *cw,
                                    E_Comp_Win *cw2)
{
   E_Comp_Effect_Style st;
   Eina_Bool animatable;
   E_Comp_Win_Type type;
   E_Comp_Win *tp, *tp2;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw2, 0);
   E_CHECK_RETURN(cw->c, 0);

   type = e_mod_comp_win_type_get(cw);
   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_RESTACK);

   if ((!cw->c->animatable) ||
       (!cw->visible) ||
       (!animatable) ||
       (st == E_COMP_EFFECT_STYLE_NONE) ||
       (!REGION_EQUAL_TO_ROOT(cw)))
     {
        return EINA_FALSE;
     }

   tp  = _transient_parent_find(cw);
   tp2 = _transient_parent_find(cw2);
   if ((tp) && (tp2) && (tp->win == tp2->win))
     {
        return EINA_FALSE;
     }

   if ((type == E_COMP_WIN_TYPE_NORMAL) ||
       (type == E_COMP_WIN_TYPE_MENUSCREEN) ||
       (type == E_COMP_WIN_TYPE_TASKMANAGER) ||
       (type == E_COMP_WIN_TYPE_LIVEMAGAZINE))
     {
        return EINA_TRUE;
     }

   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_policy_win_lower_check(E_Comp_Win *cw,
                                  E_Comp_Win *cw2)
{
   E_Comp_Effect_Style st1, st2;
   Eina_Bool animatable1, animatable2;
   E_Comp_Win *tp, *tp2;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw2, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->animatable, 0);
   E_CHECK_RETURN(cw->visible, 0);
   E_CHECK_RETURN(cw2->visible, 0);
   if (!REGION_EQUAL_TO_ROOT(cw)) return EINA_FALSE;
   if (!REGION_EQUAL_TO_ROOT(cw2)) return EINA_FALSE;

   animatable1 = e_mod_comp_effect_state_get(cw->eff_type);
   animatable2 = e_mod_comp_effect_state_get(cw2->eff_type);
   E_CHECK_RETURN(animatable1, 0);
   E_CHECK_RETURN(animatable2, 0);

   if ((!TYPE_NORMAL_CHECK(cw)) &&
       (!TYPE_NORMAL_CHECK(cw2)))
     {
        return EINA_FALSE;
     }

   if ((!TYPE_NORMAL_CHECK(cw)) &&
       (!TYPE_HOME_CHECK(cw2)))
     {
        return EINA_FALSE;
     }

   st1 = e_mod_comp_effect_style_get
           (cw->eff_type, E_COMP_EFFECT_KIND_RESTACK);
   st2 = e_mod_comp_effect_style_get
           (cw2->eff_type, E_COMP_EFFECT_KIND_RESTACK);
   E_CHECK_RETURN((st1 != E_COMP_EFFECT_STYLE_NONE), 0);
   E_CHECK_RETURN((st2 != E_COMP_EFFECT_STYLE_NONE), 0);

   tp  = _transient_parent_find(cw);
   tp2 = _transient_parent_find(cw2);
   if ((tp) && (tp2) && (tp->win == tp2->win))
     {
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_comp_policy_win_rotation_effect_check(E_Comp_Win *cw)
{
   E_Comp_Effect_Style st;
   Eina_Bool animatable;
   const char *file, *group;
   E_Comp_Object *co;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->animatable, 0);
   E_CHECK_RETURN(cw->visible, 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->objs, 0);

   animatable = e_mod_comp_effect_state_get(cw->eff_type);
   E_CHECK_RETURN(animatable, 0);

   co = eina_list_data_get(cw->objs);
   E_CHECK_RETURN(co, 0);

   edje_object_file_get(co->shadow, &file, &group);
   if ((strcmp(group, "shadow_fade") != 0) &&
       (strcmp(group, "shadow_twist") !=0))
     {
        return EINA_FALSE;
     }

   st = e_mod_comp_effect_style_get
          (cw->eff_type,
          E_COMP_EFFECT_KIND_ROTATION);
   if (st == E_COMP_EFFECT_STYLE_NONE)
     return EINA_FALSE;

   if ((e_mod_comp_util_win_visible_get(cw)) &&
       (evas_object_visible_get(co->shadow)) &&
       TYPE_NORMAL_CHECK(cw))
     {
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EINTERN char *
e_mod_comp_policy_win_shadow_group_get(E_Comp_Win *cw)
{
   E_Comp_Win_Type type;
   E_CHECK_RETURN(cw, 0);
   type = e_mod_comp_win_type_get(cw);
   return eina_hash_find(shadow_hash, e_util_winid_str_get(type));
}

/* local subsystem functions */
static E_Comp_Win *
_transient_parent_find(E_Comp_Win *cw)
{
   // if Border is not existed then, return itself.
   // otherwise, return itself or parent.
   Ecore_X_Window transient_parent;
   E_Comp_Win *parent_cw = NULL;
   E_Border *bd = NULL;
   if (cw->bd)
     {
        bd = cw->bd;
        do {
             transient_parent = bd->win;
             bd = bd->parent;
        } while (bd);

        parent_cw = e_mod_comp_win_find(transient_parent);
        return parent_cw;
     }
   return cw;
}
