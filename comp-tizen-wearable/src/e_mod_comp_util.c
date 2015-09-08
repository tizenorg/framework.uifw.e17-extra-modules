#include <utilX.h>
#include <utilX_ext.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xatom.h>
#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"

/* local subsystem globals */
static E_Comp *_c = NULL;

/* externally accessible functions */
EAPI void
e_mod_comp_util_set(E_Comp *c,
                    E_Manager *man __UNUSED__)
{
   _c = c;
}

EAPI E_Comp *
e_mod_comp_util_get(void)
{
   return _c;
}

EAPI Eina_Bool
e_mod_comp_util_grab_key_set(Eina_Bool grab)
{
#ifdef _F_USE_GRAB_KEY_SET_
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
#endif

   return EINA_TRUE;
}

EAPI Eina_Bool
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

EAPI E_Comp_Win *
e_mod_comp_util_win_nocomp_get(E_Comp *c,
                               E_Zone *zone)
{
   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Win *cw = NULL;

   if (_comp_mod->conf->nocomp_fs)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone == zone)
               {
                  if (canvas->nocomp.mode == E_NOCOMP_MODE_RUN)
                    {
                       if (canvas->nocomp.cw)
                         {
                            cw = canvas->nocomp.cw;

                            ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                                 "%15.15s| FOUND NOCOMP RUN WIN", "NOCOMP");
                         }
                    }
                  break;
               }
          }
     }

   return cw;
}

/* TODO: clean up nocomp logic */
#if USE_NOCOMP_DISPOSE
EAPI E_Comp_Win *
e_mod_comp_util_win_nocomp_end_get(E_Comp *c,
                                   E_Zone *zone)
{
   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Win *cw = NULL;

   if (_comp_mod->conf->nocomp_fs)
     {
        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             if (!canvas) continue;
             if (canvas->zone == zone)
               {
                  if (canvas->nocomp.mode == E_NOCOMP_MODE_END)
                    {
                       if (canvas->nocomp.end.cw)
                         {
                            cw = canvas->nocomp.end.cw;

                            ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
                                 "%15.15s| FOUND NOCOMP END WIN", "NOCOMP");
                         }
                    }
                  break;
               }
          }
     }

   return cw;
}
#endif

EAPI E_Comp_Win *
e_mod_comp_util_win_normal_get(E_Comp_Win *cw,
                                       Eina_Bool opaque)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw2 = NULL, *nocomp_cw = NULL;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(c, 0);

   if ((cw) && (cw->bd))
     zone = cw->bd->zone;

   if (!zone)
     zone = e_util_zone_current_get(c->man);

   E_CHECK_RETURN(zone, NULL);

   /* find nocomp window first because nocomp window doesn't have pixmap */
   nocomp_cw = e_mod_comp_util_win_nocomp_get(c, zone);

   /* look for visible normal window except for given window */
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw2)
     {
        if ((cw2->invalid) || (cw2->input_only)) continue;
        if ((cw) && (cw == cw2)) continue; /* except for given window */

        /* return nocomp window */
        if ((nocomp_cw) && (nocomp_cw == cw2))
          return cw2;

        /* check pixmap and compare size with zone */
        if (!cw2->bd) continue;
        if (!cw2->bd->zone) continue;
        if (cw2->bd->zone != zone) continue;
        if (!E_INTERSECTS(zone->x, zone->y, zone->w, zone->h,
                          cw2->x, cw2->y, cw2->w, cw2->h))
          continue;

        if (!((cw2->pixmap) &&
              (cw2->pw > 0) && (cw2->ph > 0) &&
              (cw2->dmg_updates >= 1)))
          continue;

        if (REGION_EQUAL_TO_ZONE(cw2, zone) &&
            ((!opaque) || (!cw2->argb)))
          return cw2;
     }
   return NULL;
}

EAPI E_Comp_Win *
e_mod_comp_util_win_home_get(E_Comp_Win *cw)
{
   E_Comp *c = e_mod_comp_util_get();
   E_Comp_Win *cw2 = NULL;
   E_Zone *zone = NULL;
   E_CHECK_RETURN(c, 0);

   if ((cw) && (cw->bd))
     zone = cw->bd->zone;

   if (!zone)
     zone = e_util_zone_current_get(c->man);

   E_CHECK_RETURN(zone, NULL);

   /* look for visible normal window except for given window */
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw2)
     {
        if (TYPE_HOME_CHECK(cw2))
          return cw2;
     }
   return NULL;
}
EAPI E_Comp_Win *
e_mod_comp_util_win_below_get(E_Comp_Win *cw,
                              Eina_Bool normal_check,
                              Eina_Bool visible_check)
{
   Eina_Inlist *l;
   E_Comp_Win *_cw = cw;
   E_CHECK_RETURN(_cw, NULL);

   while ((l = EINA_INLIST_GET(_cw)->prev))
     {
        _cw = _EINA_INLIST_CONTAINER(_cw, l);
        if ((_cw) &&
            (!_cw->invalid) &&
            (!_cw->input_only) &&
            (!TYPE_APPTRAY_CHECK(_cw)) &&
            (!TYPE_MINI_APPTRAY_CHECK(_cw)) &&
            REGION_EQUAL_TO_ROOT(_cw))
          {
             if (visible_check && !_cw->visible)
               continue;
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

EAPI Eina_Bool
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

EAPI Eina_Bool
e_mod_comp_util_win_visible_get(E_Comp_Win *cw,
                                      Eina_Bool alpha_check)
{
   E_Comp_Win *cw2 = NULL, *nocomp_cw = NULL;
   E_Zone *zone = NULL;
   int count = 0;

   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->visible, 0);
   E_CHECK_RETURN(!(cw->invalid), 0);
   E_CHECK_RETURN(!(cw->input_only), 0);
   E_CHECK_RETURN(cw->bd, 0);
   E_CHECK_RETURN(cw->bd->zone, 0);

   zone = cw->bd->zone;
   E_CHECK_RETURN(zone, 0);

   /* find nocomp window first because nocomp window doesn't have pixmap */
   nocomp_cw = e_mod_comp_util_win_nocomp_get(cw->c, zone);

   if (!E_INTERSECTS
       (zone->x, zone->y, zone->w, zone->h,
        cw->x, cw->y, cw->w, cw->h))
     {
        return EINA_FALSE;
     }

   EINA_INLIST_REVERSE_FOREACH(cw->c->wins, cw2)
     {
        if (cw2->invalid) continue;
        if (cw2->input_only) continue;
        if (!cw2->bd) continue;

        ELBF(ELBT_COMP, 0, e_mod_comp_util_client_xid_get(cw),
             "%15.15s| CHECK | %02d| %p %p 0x%08x pixmap:0x%x %dx%d %d", "EFFECT",
             count++,
             cw, cw2,
             e_mod_comp_util_client_xid_get(cw2),
             cw2->pixmap, cw2->pw, cw2->ph,
             cw2->dmg_updates);

        if (!cw2->bd->zone) continue;
        if (!cw2->visible) continue;
        if (cw2->bd->zone != zone) continue;
        if (!E_INTERSECTS(zone->x, zone->y, zone->w, zone->h,
                          cw2->x, cw2->y, cw2->w, cw2->h))
          continue;

        if (!((cw2->pixmap) &&
              (cw2->pw > 0) && (cw2->ph > 0) &&
              (cw2->dmg_updates >= 1)))
          {
             if ((nocomp_cw) && (nocomp_cw == cw2))
               {
                  if (cw2 == cw)
                    return EINA_TRUE;
                  else
                    return EINA_FALSE;
               }
             else
               continue;
          }

        if (cw2 == cw) return EINA_TRUE;

        if (REGION_EQUAL_TO_ZONE(cw2, zone) &&
            ((!alpha_check) || (!cw2->argb)))
          return EINA_FALSE;
     }

   return EINA_FALSE;
}

/* check visibility of cw before changing c->wins.
 * this utility function assumes that cw is positioned behind cw2.
 */
EAPI Eina_Bool
e_mod_comp_util_win_visible_below_get(E_Comp_Win *cw,
                                      E_Comp_Win *cw2)
{
   /* TODO: */
   return EINA_TRUE;
}

EAPI Ecore_X_Window
e_mod_comp_util_client_xid_get(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   if (cw->bd) return cw->bd->client.win;
   else return cw->win;
}

EAPI void
e_mod_comp_util_fb_visible_set(Eina_Bool set)
{
   Ecore_X_Display *d = ecore_x_display_get();
   E_CHECK(d);

   utilx_set_fb_visible(d, UTILX_FB_TYPE_OVERLAY,
                        set ? True : False);
}

EAPI void
e_mod_comp_util_rr_prop_set(Ecore_X_Atom  atom,
                            const char   *state)
{
   E_Comp *c = e_mod_comp_util_get();
   Ecore_X_Display *dpy;
   Ecore_X_Window root;
   static RROutput output = 0;
   int i;

   root = c->man->root;
   dpy = ecore_x_display_get();
   if (!output)
     {
        XRRScreenResources *res = XRRGetScreenResources(dpy, root);
        XRROutputInfo *output_info = NULL;

        if (res)
          {
             if (res->noutput != 0)
               {
                  for (i = 0; i < res->noutput; i++)
                    {
                       output_info = XRRGetOutputInfo(dpy, res, res->outputs[i]);

                       if ((output_info) &&
                           (output_info->name) &&
                           (!strncmp(output_info->name, "LVDS1", 5)))
                         {
                            output = res->outputs[i];
                            XRRFreeOutputInfo(output_info);
                            break;
                         }

                       if (output_info) XRRFreeOutputInfo(output_info);
                    }
               }
             XRRFreeScreenResources(res);
          }
     }

   if (output)
     XRRChangeOutputProperty(dpy, output, atom, XA_STRING, 8, PropModeReplace, (unsigned char*)state, strlen(state));
}

EAPI Ecore_X_Pixmap
e_mod_comp_util_copied_pixmap_get(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->win, 0);
   E_CHECK_RETURN(cw->pixmap, 0);

   Ecore_X_Pixmap pixmap = 0;
   int depth = ecore_x_pixmap_depth_get(cw->pixmap);
   Ecore_X_GC gc = ecore_x_gc_new(cw->win, 0, NULL);
   E_CHECK_RETURN(gc, 0);

   pixmap = ecore_x_pixmap_new(cw->win, cw->pw, cw->ph, depth);
   E_CHECK_GOTO(pixmap, error);

   ecore_x_pixmap_paste(cw->pixmap, pixmap, gc, 0, 0, cw->pw, cw->ph, 0, 0);
   ecore_x_gc_free(gc);

   return pixmap;
error:
   if (gc) ecore_x_gc_free(gc);
   return 0;
}

