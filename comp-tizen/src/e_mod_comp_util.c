#include <utilX.h>
#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"

/* local subsystem globals */
static E_Comp *_c = NULL;

/* externally accessible functions */
EINTERN void
e_mod_comp_util_set(E_Comp *c,
                    E_Manager *man __UNUSED__)
{
   _c = c;
}

EINTERN E_Comp *
e_mod_comp_util_get(void)
{
   return _c;
}

EINTERN Eina_Bool
e_mod_comp_util_grab_key_set(Eina_Bool grab)
{
   E_Comp *c = NULL;
   Ecore_X_Display *d = NULL;
   Ecore_X_Window w;
   int r[4] = { -1, };
   int i = 0;

   c = e_mod_comp_util_get();
   E_CHECK_RETURN(c, 0);

   d = ecore_x_display_get();
   E_CHECK_RETURN(d, 0);

   w = c->win;
   E_CHECK_RETURN(w, 0);

   if (grab)
     {
        r[0] = utilx_grab_key(d, w, KEY_SELECT, EXCLUSIVE_GRAB);
        r[1] = utilx_grab_key(d, w, KEY_VOLUMEUP, EXCLUSIVE_GRAB);
        r[2] = utilx_grab_key(d, w, KEY_VOLUMEDOWN, EXCLUSIVE_GRAB);
        r[3] = utilx_grab_key(d, w, KEY_CAMERA, EXCLUSIVE_GRAB);

        for (i = 0; i < 4; i++)
          {
             if (r[i] != 0) goto grab_failed;
          }
     }
   else
     {
        utilx_ungrab_key(d, w, KEY_SELECT);
        utilx_ungrab_key(d, w, KEY_VOLUMEUP);
        utilx_ungrab_key(d, w, KEY_VOLUMEDOWN);
        utilx_ungrab_key(d, w, KEY_CAMERA);
     }

   return EINA_TRUE;

grab_failed:
   utilx_ungrab_key(d, w, KEY_SELECT);
   utilx_ungrab_key(d, w, KEY_VOLUMEUP);
   utilx_ungrab_key(d, w, KEY_VOLUMEDOWN);
   utilx_ungrab_key(d, w, KEY_CAMERA);
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_util_screen_input_region_set(Eina_Bool set)
{
   E_Comp *c = e_mod_comp_util_get();
   int x = -1, y = -1, w = 1, h = 1;
   E_CHECK_RETURN(c, 0);
   
   if (set)
     {
        x = y = 0;
        w = c->man->w;
        h = c->man->h;
     }

   ecore_x_window_shape_input_rectangle_set(c->win,
                                            x, y, w, h);

   return EINA_TRUE;
}

EINTERN E_Comp_Win *
e_mod_comp_util_win_normal_get(void)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw = NULL;
   E_CHECK_RETURN(c, 0);

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if (!cw) continue;
        if ((cw->visible) &&
            (REGION_EQUAL_TO_ROOT(cw)) &&
            (!cw->invalid) && (!cw->input_only) &&
            TYPE_NORMAL_CHECK(cw))
          {
             return cw;
          }
     }
   return NULL;
}

EINTERN E_Comp_Win *
e_mod_comp_util_win_below_get(E_Comp_Win *cw,
                              Eina_Bool normal_check)
{
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw;
   E_CHECK_RETURN(_cw, NULL);

   while ((l = EINA_INLIST_GET(_cw)->prev))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        if ((_cw) && (_cw->visible) &&
            (!_cw->invalid) &&
            (!_cw->input_only) &&
            (!TYPE_APPTRAY_CHECK(_cw)) &&
            REGION_EQUAL_TO_ROOT(_cw))
          {
             if (normal_check)
               {
                  if (TYPE_NORMAL_CHECK(_cw))
                    return _cw;
                  else
                    return NULL;
               }
             else
               return _cw;
          }
     }
   return NULL;
}

EINTERN Eina_Bool
e_mod_comp_util_win_below_check(E_Comp_Win *cw,
                                E_Comp_Win *cw2)
{
   Eina_Inlist *wins_list;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw2, 0);

   while ((wins_list = EINA_INLIST_GET(cw)->prev) != NULL)
     {
        cw = _EINA_INLIST_CONTAINER(cw, wins_list);
        if (cw == cw2)
          {
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

EINTERN Eina_Bool
e_mod_comp_util_win_visible_get(E_Comp_Win *cw)
{
   Eina_Bool v = EINA_FALSE;
   E_Comp_Win *_cw = NULL;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->visible, 0);
   E_CHECK_RETURN(!(cw->invalid), 0);
   E_CHECK_RETURN(!(cw->input_only), 0);
   E_CHECK_RETURN(!(cw->defer_hide), 0);
   E_CHECK_RETURN(cw->c, 0);
   E_CHECK_RETURN(cw->c->man, 0);

   if (!E_INTERSECTS
       (0, 0, cw->c->man->w, cw->c->man->h,
        cw->x, cw->y, cw->w, cw->h))
     {
        return v;
     }

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, _cw)
     {
        if (!_cw) continue;
        if ((_cw->visible) && REGION_EQUAL_TO_ROOT(_cw) &&
            !(_cw->input_only) && !(_cw->invalid))
          {
             if (_cw == cw) return EINA_TRUE;
             else if (!TYPE_HOME_CHECK(_cw) && _cw->argb) continue;
             else return EINA_FALSE;
          }
     }
   return v;
}

EINTERN Ecore_X_Window
e_mod_comp_util_client_xid_get(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   if (cw->bd) return cw->bd->client.win;
   else return cw->win;
}

EINTERN void
e_mod_comp_util_fb_visible_set(Eina_Bool set)
{
   Ecore_X_Display *d = ecore_x_display_get();
   E_CHECK(d);

   utilx_set_fb_visible(d, UTILX_FB_TYPE_OVERLAY,
                        set ? True : False);
}
