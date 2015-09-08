#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

#ifdef USE_HWC
#include "e_mod_comp_hwcomp.h"
#include <X11/extensions/hwc.h>

#define E_HWCOMP_COMP_COUNTDOWN  30
#define E_HWCOMP_SET_COUNTDOWN   1
#define E_HWCOMP_FULL_PENDING_COUNT 2

#if OPTIMIZED_HWC
#define KEYBOARD_MAGNIFIER_WIDTH_PORTRAIT  77
#define KEYBOARD_MAGNIFIER_WIDTH_LANDSCAPE  70
#endif

static Eina_Bool _hwc_available = EINA_FALSE;
static int _hwc_max_layer;
static int _hwc_major, _hwc_minor;
static Ecore_X_Display *_display = NULL;
static int _hwc_op_code;

static Eina_Bool _e_mod_comp_hwcomp_idle_timer_cb(const void *data);

#if DEBUG_HWC
static void
_hwcomp_dbg_print_update (E_Comp_HWComp_Update *hwc_update, const char *str)
{
   E_Comp_HWComp_Drawable *cur_d = NULL;
   int i;

   ELBF(ELBT_COMP, 0, 0, "%15.15s| mode: %d num_d: %d ",
        str, hwc_update->update_mode, hwc_update->num_drawable);
   for(i = 0; i < hwc_update->num_overlays; i++)
     {
        cur_d = hwc_update->hwc_drawable[i];
        ELBF(ELBT_COMP, 0, 0, "%15.15s|  cur {d[%d]:%p set:%d}",
             str,
             i, cur_d?cur_d->d:0x0,cur_d?cur_d->set_drawable:0);
     }
}

static void
_hwcomp_dbg_print_change_update (E_Comp_HWComp_Update *cur_hwc_update, E_Comp_HWComp_Update *new_hwc_update, const char *str)
{
    E_Comp_HWComp_Drawable *cur_d = NULL;
    E_Comp_HWComp_Drawable *new_d = NULL;
    int i;
    int diff;
    int count=0;

    ELBF(ELBT_COMP, 0, 0, "%15.15s| mode: %d ===> %d num_d: %d ==> %d",
         str,
         cur_hwc_update->update_mode, new_hwc_update->update_mode,
         cur_hwc_update->num_drawable, new_hwc_update->num_drawable);

    diff = cur_hwc_update->num_overlays - new_hwc_update->num_overlays;
    if (!diff) count = cur_hwc_update->num_overlays;
    else if (diff > 0) count = new_hwc_update->num_overlays;
    else count = cur_hwc_update->num_overlays;

    for (i = 0; i < count; i++)
      {
         cur_d = cur_hwc_update->hwc_drawable[i];
         new_d = new_hwc_update->hwc_drawable[i];
         ELBF(ELBT_COMP, 0, 0, "%15.15s|  cur {d[%d]:%p set:%d} new {d[%d]:%p set:%d}",
              str,
              i, cur_d?cur_d->d:0x0,cur_d?cur_d->set_drawable:0,
              i, new_d?new_d->d:0x0,new_d?new_d->set_drawable:0);
      }

    if (!diff) return;
    else if (diff > 0)
      {
         for (i; i < cur_hwc_update->num_overlays; i++)
           {
              cur_d = cur_hwc_update->hwc_drawable[i];
              ELBF(ELBT_COMP, 0, 0, "%15.15s|  cur {d[%d]:%p set:%d} new {d[%d]:%s set:%d}",
                   str,
                   i, cur_d?cur_d->d:0x0,cur_d?cur_d->set_drawable:0,
                   i, "HWOV",0);
           }
      }
    else
      {
         for (i; i < new_hwc_update->num_overlays; i++)
           {
              new_d = new_hwc_update->hwc_drawable[i];
              ELBF(ELBT_COMP, 0, 0, "%15.15s|  cur {d[%d]:%s set:%d} new {d[%d]:%p set:%d}",
                   str,
                   i, "HWOV",0,
                   i, new_d?new_d->d:0x0,new_d?new_d->set_drawable:0);
           }
}
}
#endif

#if OPTIMIZED_HWC

static const char*
_e_mod_comp_hwcomp_border_name_get(E_Border *bd)
{
    if (!bd) return NULL;

    if (bd->client.icccm.name && strcmp(bd->client.icccm.name, "NORMAL_WINDOW"))
        return bd->client.icccm.name;
    else if (bd->client.netwm.name)
        return bd->client.netwm.name;
    else if (bd->client.icccm.class)
        return bd->client.icccm.class;
    else
        return NULL;
}
#endif

static Eina_Bool
_e_mod_comp_hwcomp_idle_timer_cb(const void *data)
{
   /*
    * This call back means comp is entered real idle (nothing happened in 2 seconds)
    * Almost of this timer may expired in _e_mod_comp_cb_update()
    */
   E_Comp_HWComp *hwcomp = NULL;
   E_Comp_HWComp_Update *update = NULL;
   E_Comp *c = NULL;

   hwcomp = (E_Comp_HWComp *)data;
   if (!hwcomp) return ECORE_CALLBACK_DONE;

   update = hwcomp->hwc_update;
   c = hwcomp->c;

   if (!update || !c)
      return ECORE_CALLBACK_DONE;

   if (update->update_mode != E_HWCOMP_USE_HYBRIDCOMP_MODE)
      return ECORE_CALLBACK_DONE;

   /*1. expire self*/
   if (hwcomp->idle_timer)
     {
        ecore_timer_del(hwcomp->idle_timer);
        hwcomp->idle_timer = NULL;
        /*2.  set flag comp-tizen is idle now*/
        hwcomp->idle_status = EINA_TRUE;
        /*3. do compositing 1 time by GL*/
        e_mod_comp_hwcomp_set_full_composite(hwcomp);
#if DEBUG_HWC
        _hwcomp_dbg_print_update(update, "HWC:IdleTimerCB");
#endif
     }

   return ECORE_CALLBACK_DONE;
}

static void
_e_mod_comp_hwcomp_append_idle_timer(E_Comp_HWComp *hwcomp)
{
   E_Comp_HWComp_Update *update = NULL;
   E_Comp_HWComp_Drawable *drawable = NULL;
   int i;
   int num_set_drawable = 0;

   E_CHECK(hwcomp);

   update = hwcomp->hwc_update;
   E_CHECK(update);

   if (!hwcomp->idle_timer)
     {
        for (i = 0; i < update->num_drawable; i++)
          {
             drawable = update->hwc_drawable[i];
             if (drawable && drawable->set_drawable == EINA_TRUE)
                num_set_drawable++;
          }

        if (num_set_drawable > 1)
          {
             hwcomp->idle_timer = ecore_timer_add(2.0, (Ecore_Task_Cb)_e_mod_comp_hwcomp_idle_timer_cb, hwcomp);
             hwcomp->idle_status = EINA_FALSE;
          }
     }
}

static void
_e_mod_comp_hwcomp_dec_set_countdown (E_Comp_HWComp_Drawable *d)
{
   if(d->set_countdown <= 0) return;

   d->set_countdown--;
}

static void
_e_mod_comp_hwcomp_reset_set_countdown (E_Comp_HWComp_Drawable *d)
{
   d->set_countdown = E_HWCOMP_SET_COUNTDOWN;
}

static void
_e_mod_comp_hwcomp_dec_comp_countdown (E_Comp_HWComp_Drawable *d)
{
   if(d->comp_countdown <= 0) return;

   d->comp_countdown--;
}

static void
_e_mod_comp_hwcomp_reset_comp_countdown (E_Comp_HWComp_Drawable *d)
{
   d->comp_countdown = E_HWCOMP_COMP_COUNTDOWN;
}

static Eina_Bool
_e_mod_comp_hwcomp_update_check_region(E_Comp_HWComp_Update *hwc_update, E_Comp_Win *cw, int w, int h, int max_w, int max_h)
{
   Eina_Bool ret = EINA_FALSE;
   int area = 0;
   int max = max_w*max_h/2;

#if DEBUG_HWC
   ELBF(ELBT_COMP, 0, 0, "%15.15s| d:%p (%dx%d)", "HWC:WIN REGION",
      cw->bd->client.win, w, h);
#endif
   area = w * h;
   if (area > max)
      ret = EINA_TRUE;

#if OPTIMIZED_HWC
   return EINA_TRUE;
#else
   return ret;
#endif
}


static void
_e_mod_comp_hwcomp_update_countdown(E_Comp_HWComp_Update *hwc_update, E_Comp_Win *cw)
{
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;
   int i = 0;
   int num_drawable = 0;

   if (hwc_update->update_mode != E_HWCOMP_USE_HYBRIDCOMP_MODE) return;

   num_drawable = hwc_update->num_drawable;

   for (i = 0; i < num_drawable; i++)
    {
       hwc_drawable = hwc_update->hwc_drawable[i];

       if (hwc_drawable && hwc_drawable->cw == cw)
         {
            if(hwc_drawable->set_drawable)
              {
               _e_mod_comp_hwcomp_reset_comp_countdown (hwc_drawable);

                 if (i != 0 && hwc_drawable->first_update)
                   {
                      hwc_drawable->set_drawable = EINA_FALSE;
                      hwc_drawable->first_update = 0;
                   }
              }
            else
              {
                 if (hwc_drawable->region_update)
                   {
               _e_mod_comp_hwcomp_dec_set_countdown (hwc_drawable);
                      hwc_drawable->region_update = EINA_FALSE;
                   }
              }
#if DEBUG_HWC
            ELBF(ELBT_COMP, 0, 0, "%15.15s| client.win: %p set:%d set_cnt:%d comp_cnt:%d fu(%d)",
                 "HWC:CW Update", cw->bd->client.win, hwc_drawable->set_drawable, hwc_drawable->set_countdown, hwc_drawable->comp_countdown, hwc_drawable->first_update);
#endif
         }
    }
}

static Eina_Bool
_e_mod_comp_hwcomp_update_check_resized(E_Comp_HWComp_Update *hwc_update)
{
   int i;

   for (i = 0; i < hwc_update->num_drawable; i++)
     {
        if (hwc_update->hwc_drawable[i]->resized)
           return EINA_TRUE;
     }

   return EINA_FALSE;
}

static void
_e_mod_comp_hwcomp_update_unset_resized(E_Comp_HWComp_Update *hwc_update, E_Comp_Win *cw)
{
   int i;
   for (i = 0; i < hwc_update->num_drawable; i++)
     {
        if (cw == hwc_update->hwc_drawable[i]->cw)
          {
             if (hwc_update->hwc_drawable[i]->resized)
               {
                  hwc_update->hwc_drawable[i]->resized = EINA_FALSE;
                  break;
               }
          }
     }
}

static void
_e_mod_comp_hwcomp_update_set_resized(E_Comp_HWComp_Update *hwc_update, E_Comp_Win *cw)
{
   int i;

   for (i = 0; i < hwc_update->num_drawable; i++)
     {
        if (cw == hwc_update->hwc_drawable[i]->cw)
          {
             hwc_update->hwc_drawable[i]->resized = EINA_TRUE;
             break;
          }
     }
}



EAPI void
_e_mod_comp_hwcomp_update_set_drawables (E_Comp_HWComp_Update *hwc_update, Ecore_X_Window win)
{
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;
   Ecore_X_Drawable *d = NULL;
   int count = 0, idx = 0;
   int i;

#if OPTIMIZED_HWC
   XRectangle *src_rects = NULL;
   XRectangle *dst_rects = NULL;
   int zone_x = 0, zone_y = 0, zone_w = 0, zone_h = 0;
   int zone_rot = 0;
#endif

   if (!hwc_update) return;

   for (i = 0; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        if (hwc_drawable->set_drawable)
          count++;
     }

   if (count <= 0) return;

   d = E_NEW(Ecore_X_Drawable, count);
   if (!d)
      return;

#if OPTIMIZED_HWC
   src_rects = E_NEW(XRectangle, count);
   if (!src_rects)
       return;

   dst_rects = E_NEW(XRectangle, count);
   if (!dst_rects)
     {
        E_FREE(src_rects);
        return;
     }

   if (hwc_update->hwcomp->canvas && hwc_update->hwcomp->canvas->zone)
     {
        zone_x = hwc_update->hwcomp->canvas->zone->x;
        zone_y = hwc_update->hwcomp->canvas->zone->y;
        zone_w = hwc_update->hwcomp->canvas->zone->w;
        zone_h = hwc_update->hwcomp->canvas->zone->h;
     }
   else
     {
        zone_x = 0;
        zone_y = 0;
        zone_w = hwc_update->hwcomp->screen_width;
        zone_h = hwc_update->hwcomp->screen_height;
     }

   //check orientation
   if (hwc_update->hwcomp->canvas && hwc_update->hwcomp->canvas->zone)
     {
        zone_rot = e_zone_rotation_get(hwc_update->hwcomp->canvas->zone);
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, 0, "%15.15s|SET_DRAWABLES: Zone Rotation[%d]", "HWC:Zone",
            zone_rot);
#endif
     }
   else
     {
        //if zone is not present then what to do ?
        if (hwc_update->ime_present)
          {
             if (hwc_update->ime_rect.h == hwc_update->hwcomp->screen_height)
               {
                  if (hwc_update->ime_rect.x == 0)
                      zone_rot = 270;
                  else
                      zone_rot = 90;
               }
          }
     }
#endif

   for (i = 0; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        if (hwc_drawable->set_drawable)
          {

#if OPTIMIZED_HWC

#if DEBUG_HWC
             ELBF(ELBT_COMP, 0, 0, "%15.15s|SET_DRAWABLES: id (%d) , cnt(%d), fu(%d)", "HWC",
                 i, hwc_drawable->update_count, hwc_drawable->first_update);
             hwc_drawable->update_count++ ; //for debugging
#endif

             if (hwc_update->update_mode == E_HWCOMP_USE_FULLCOMP_MODE)
               {
                  src_rects[idx].x = zone_x;
                  src_rects[idx].y = zone_y;
                  src_rects[idx].width = zone_w;
                  src_rects[idx].height = zone_h;

                  hwc_drawable->first_update = 0;
               }
             else if (i == 0 && hwc_update->hwcomp->miniapp_present)
               {
                  src_rects[idx].x = zone_x;
                  src_rects[idx].y = zone_y;
                  src_rects[idx].width = zone_w;
                  src_rects[idx].height = zone_h;

                  hwc_drawable->first_update = 0;
               }
             else if (i == 0 && hwc_update->ime_present && hwc_update->split_launcher_rect_present)
               {
                  src_rects[idx].x = zone_x;
                  src_rects[idx].y = zone_y;
                  src_rects[idx].width = zone_w;
                  src_rects[idx].height = zone_h;

                  hwc_drawable->first_update = 0;
               }
             else if (i == 0 && hwc_update->ime_present)
               {
#if DEBUG_HWC
                  ELBF(ELBT_COMP, 0, 0, "%15.15s|SET_DRAWABLES: kbgeo[%d, %d, %d, %d] zRot[%d]", "HWC:Zone-KB",
                        hwc_update->ime_rect.x,hwc_update->ime_rect.y, hwc_update->ime_rect.w,
                        hwc_update->ime_rect.h, zone_rot);
#endif
                   if (zone_rot == 0 || (zone_rot != 0 && hwc_update->ime_rect.y != 0))
                     {
                        if (hwc_drawable->first_update == 0)
                          {
                             src_rects[idx].x = zone_x;
                             src_rects[idx].y = zone_y;
                             src_rects[idx].width = zone_w;
                             src_rects[idx].height = zone_h;

                             hwc_drawable->first_update = 1;
                          }
                        else
                          {
                             src_rects[idx].x = hwc_update->ime_rect.x;
                             src_rects[idx].y = hwc_update->ime_rect.y - KEYBOARD_MAGNIFIER_WIDTH_PORTRAIT; //includes magnifier
                             src_rects[idx].width = hwc_update->ime_rect.w;
                             src_rects[idx].height = hwc_update->ime_rect.h + KEYBOARD_MAGNIFIER_WIDTH_PORTRAIT; //includes magnifier
                          }
                     }
                   else
                     {
                        if (hwc_drawable->first_update == 0)
                          {
                             src_rects[idx].x = zone_x;
                             src_rects[idx].y = zone_y;
                             src_rects[idx].width = zone_w;
                             src_rects[idx].height = zone_h;

                             hwc_drawable->first_update = 1;
                          }
                        else
                          {
                             if (zone_rot == 90)
                               {
                                  src_rects[idx].x = hwc_update->ime_rect.x - KEYBOARD_MAGNIFIER_WIDTH_LANDSCAPE;
                               }
                             else //case (zone_rot == 270), as already zone angle 0 and 90 handled
                               {
                                  src_rects[idx].x = hwc_update->ime_rect.x;
                               }
                             src_rects[idx].y = hwc_update->ime_rect.y;
                             src_rects[idx].width = hwc_update->ime_rect.w + KEYBOARD_MAGNIFIER_WIDTH_LANDSCAPE;
                             src_rects[idx].height = hwc_update->ime_rect.h;
                          }
                     }
               }
             else if (i == 0 && hwc_update->split_launcher_rect_present)
               {
                  src_rects[idx].x = zone_x;
                  src_rects[idx].y = zone_y;
                  src_rects[idx].width = zone_w;
                  src_rects[idx].height = zone_h;

                  hwc_drawable->first_update = 0;
               }
             else
               {
                  if (hwc_drawable->cw)
                    {
                       src_rects[idx].x = hwc_drawable->cw->x;
                       src_rects[idx].y = hwc_drawable->cw->y;
                       src_rects[idx].width = hwc_drawable->cw->w;
                       src_rects[idx].height = hwc_drawable->cw->h;
                    }
                  else
                    {
                       src_rects[idx].x = zone_x;
                       src_rects[idx].y = zone_y;
                       src_rects[idx].width = zone_w;
                       src_rects[idx].height = zone_h;
                    }
               }
#endif

            //check for keyboard intersection
#if OPTIMIZED_HWC
            if (hwc_update->ime_present && i != 0 && hwc_drawable->cw &&
                    E_INTERSECTS(hwc_drawable->cw->x, hwc_drawable->cw->y,
                                 hwc_drawable->cw->w, hwc_drawable->cw->h,
                                 hwc_update->ime_rect.x, hwc_update->ime_rect.y,
                                 hwc_update->ime_rect.w, hwc_update->ime_rect.h)
                    &&
                    !E_CONTAINS(hwc_update->ime_rect.x, hwc_update->ime_rect.y,
                                hwc_update->ime_rect.w, hwc_update->ime_rect.h,
                                hwc_drawable->cw->x, hwc_drawable->cw->y,
                                hwc_drawable->cw->w, hwc_drawable->cw->h))
              {
#if DEBUG_HWC
                 ELBF(ELBT_COMP, 0, 0, "%15.15s|SET_DRAWABLES:Drawable[%d] size [%d, %d, %d, %d]", "HWC:Reduce", i, hwc_drawable->cw->x, hwc_drawable->cw->y,
                   hwc_drawable->cw->w, hwc_drawable->cw->h);
#endif

                 if (zone_rot == 0 || (zone_rot != 0 && hwc_update->ime_rect.y != 0))
                   {
                      src_rects[idx].x = hwc_drawable->cw->x;
                      src_rects[idx].y = hwc_drawable->cw->y;
                      src_rects[idx].width = hwc_drawable->cw->w;
                      src_rects[idx].height = hwc_drawable->cw->h - (hwc_drawable->cw->y +
                                              hwc_drawable->cw->h - hwc_update->ime_rect.y);
                   }
                 else
                   {
                      if (zone_rot == 90)
                        {
                           src_rects[idx].x = hwc_drawable->cw->x;
                           src_rects[idx].y = hwc_drawable->cw->y;
                           src_rects[idx].width = hwc_update->ime_rect.x - hwc_drawable->cw->x;
                           src_rects[idx].height = hwc_drawable->cw->h;
                        }
                      else // case (zone_rot == 270) as 0 and 90 zone angle already handled
                        {
                           int actualX = src_rects[idx].x;
                           src_rects[idx].x = hwc_update->ime_rect.x + hwc_update->ime_rect.w;
                           src_rects[idx].y = hwc_drawable->cw->y;
                           src_rects[idx].width = hwc_drawable->cw->w - (hwc_update->ime_rect.w - (actualX - hwc_update->ime_rect.x));
                           src_rects[idx].height = hwc_drawable->cw->h;
                        }
                   }
              }

             memcpy(&dst_rects[idx], &src_rects[idx], sizeof(XRectangle));

#if DEBUG_HWC
             ELBF(ELBT_COMP, 0, 0, "%15.15s|SET_DRAWABLES: id (%d) rect[%d, %d, %d, %d], cnt(%d), fu(%d)", "HWC",
                 i, dst_rects[i].x, dst_rects[i].y, dst_rects[i].width, dst_rects[i].height, hwc_drawable->update_count, hwc_drawable->first_update);
#endif

#endif

             d[idx] = hwc_drawable->d;
             idx++;
          }
     }

#if DEBUG_HWC
   for (i = 0; i < count; i++)
      ELBF(ELBT_COMP, 0, 0, "%15.15s|Sree : total:%d, count(%d) win:0x%08x: rect[%d, %d, %d, %d]", "HWC",
      count, i + 1, d[i], dst_rects[i].x, dst_rects[i].y, dst_rects[i].width, dst_rects[i].height);
#endif

#if OPTIMIZED_HWC
   if (count == 1)
     {
        dst_rects[0].x = src_rects[0].x = zone_x;
        dst_rects[0].y = src_rects[0].y = zone_y;
        dst_rects[0].width = src_rects[0].width = zone_w;
        dst_rects[0].height = src_rects[0].height = zone_h;
     }

   HWCSetDrawables(_display, win, (Drawable *)d, (XRectangle *)src_rects, (XRectangle *)dst_rects, count);
#else
   HWCSetDrawables(_display, win, (Drawable *)d, count);
#endif

   E_FREE(d);

#if OPTIMIZED_HWC
   E_FREE(src_rects);
   E_FREE(dst_rects);
#endif
}

static Eina_Bool
_e_mod_comp_hwcomp_update_check_comp (E_Comp_HWComp_Update *hwc_update)
{
    return hwc_update->comp_update;
}

static void
_e_mod_comp_hwcomp_update_set_comp (E_Comp_HWComp_Update *hwc_update, Eina_Bool comp_update)
{
   if (hwc_update->comp_update == comp_update) return;

   hwc_update->comp_update = comp_update;
}

static Eina_Bool
_e_mod_comp_hwcomp_update_compare_drawables (E_Comp_HWComp_Update *cur_hwc_update, E_Comp_HWComp_Update *new_hwc_update)
{
   E_Comp_HWComp_Drawable *cur_d = NULL;
   E_Comp_HWComp_Drawable *new_d = NULL;
   int i;

   if (cur_hwc_update->num_drawable != new_hwc_update->num_drawable)
     return EINA_FALSE;

   for (i = 0; i < cur_hwc_update->num_drawable; i++)
     {
        cur_d = cur_hwc_update->hwc_drawable[i];
        new_d = new_hwc_update->hwc_drawable[i];

        if (cur_d->d != new_d->d)
          return EINA_FALSE;

#if OPTIMIZED_HWC
        if (cur_hwc_update->ime_present && new_hwc_update->ime_present)
          {
#if DEBUG_HWC
             ELBF(ELBT_COMP, 0, 0, "%15.15s|ime_old(%d,%d,%d,%d)->ime_new(%d,%d,%d,%d)", "HWC:IME",
                cur_hwc_update->ime_rect.x, cur_hwc_update->ime_rect.y, cur_hwc_update->ime_rect.w, cur_hwc_update->ime_rect.h,
                new_hwc_update->ime_rect.x, new_hwc_update->ime_rect.y, new_hwc_update->ime_rect.w, new_hwc_update->ime_rect.h);
#endif
             if (cur_hwc_update->ime_rect.x != new_hwc_update->ime_rect.x)
               return EINA_FALSE;

             if (cur_hwc_update->ime_rect.y != new_hwc_update->ime_rect.y)
               return EINA_FALSE;

             if (cur_hwc_update->ime_rect.w != new_hwc_update->ime_rect.w)
               return EINA_FALSE;

             if (cur_hwc_update->ime_rect.h != new_hwc_update->ime_rect.h)
               return EINA_FALSE;
          }
     }
#endif

   return EINA_TRUE;
}

static void
_e_mod_comp_hwcomp_update_migrate_drawables (E_Comp_HWComp_Update *cur_hwc_update, E_Comp_HWComp_Update *new_hwc_update)
{
   E_Comp_HWComp_Drawable *cur_d = NULL;
   E_Comp_HWComp_Drawable *new_d = NULL;
   int i, j;

   for (i = 0; i < new_hwc_update->num_drawable; i++)
     {
        new_d = new_hwc_update->hwc_drawable[i];
        for (j = 0; j < cur_hwc_update->num_drawable; j++)
          {
             cur_d = cur_hwc_update->hwc_drawable[i];
             if (cur_d->d == new_d->d)
               {
                  new_d->comp_countdown = cur_d->comp_countdown;
                  new_d->set_countdown = cur_d->set_countdown;
                  new_d->set_drawable = cur_d->set_drawable;

#if OPTIMIZED_HWC
                  new_d->first_update = cur_d->first_update;
#if DEBUG_HWC
                  new_d->update_count = cur_d->update_count;
#endif
#endif
                  break;
               }
          }
     }
}


static void
_e_mod_comp_hwcomp_update_destroy_drawable (E_Comp_HWComp_Update *hwc_update)
{
   int i;
   E_Comp_Win *hide_cw = NULL;
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;

   for (i = 1; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        hide_cw = hwc_drawable->cw;
        if (hwc_update->update_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE &&
            hide_cw->hwc.set_drawable == EINA_FALSE)
           e_mod_comp_win_hwcomp_mask_objs_hide(hide_cw);
     }

   if(hwc_update->hwc_drawable)
     {
        for(i = 0; i < hwc_update->num_overlays; i++)
          {
             if (!hwc_update->hwc_drawable[i])
               {
                  E_FREE(hwc_update->hwc_drawable[i]);
                  hwc_update->hwc_drawable[i] = NULL;
               }
          }
        E_FREE(hwc_update->hwc_drawable);
        hwc_update->hwc_drawable = NULL;
     }
}

static Eina_Bool
_e_mod_comp_hwcomp_update_create_drawable (E_Comp_HWComp_Update *hwc_update)
{
   int i;
   int num = hwc_update->num_overlays;

   hwc_update->hwc_drawable = E_NEW(E_Comp_HWComp_Drawable *, num);
   E_CHECK_RETURN(hwc_update->hwc_drawable, 0);
   memset(hwc_update->hwc_drawable, 0x0, num * sizeof(E_Comp_HWComp_Drawable *));

   for(i = 0; i < num; i++)
     {
        hwc_update->hwc_drawable[i] = E_NEW(E_Comp_HWComp_Drawable, 1);
        if (!hwc_update->hwc_drawable[i]) goto fail;
     }

   return EINA_TRUE;
fail:
   _e_mod_comp_hwcomp_update_destroy_drawable (hwc_update);

   return EINA_FALSE;
}

static void
_e_mod_comp_hwcomp_update_clear_drawable (E_Comp_HWComp_Update *hwc_update)
{
   int i;

   for(i = 0; i < hwc_update->num_overlays; i++)
     {
        memset (hwc_update->hwc_drawable[i], 0x0, sizeof(E_Comp_HWComp_Drawable));
     }
}


static void
_e_mod_comp_hwcomp_update_destroy (E_Comp_HWComp_Update *hwc_update)
{
   if (!hwc_update) return;

   _e_mod_comp_hwcomp_update_destroy_drawable (hwc_update);

   E_FREE(hwc_update);
   hwc_update = NULL;
}

static E_Comp_HWComp_Update *
_e_mod_comp_hwcomp_update_create (E_Comp_HWComp *hwcomp, int num)
{
   E_Comp_HWComp_Update *hwc_update = NULL;
   Eina_Bool ret = EINA_FALSE;

   hwc_update = E_NEW(E_Comp_HWComp_Update, 1);
   E_CHECK_RETURN(hwc_update, 0);

   hwc_update->update_mode = E_HWCOMP_USE_INVALID;
   hwc_update->num_overlays = num;
   ret = _e_mod_comp_hwcomp_update_create_drawable (hwc_update);
   if (ret == EINA_FALSE)
     {
        _e_mod_comp_hwcomp_update_destroy(hwc_update);
        return NULL;
     }
   hwc_update->num_drawable = 0;
   hwc_update->comp_update = EINA_FALSE;

#if OPTIMIZED_HWC
   hwc_update->hwcomp = hwcomp;
#endif

   return hwc_update;
}

static void
_e_mod_comp_hwcomp_update_clear (E_Comp_HWComp_Update *hwc_update)
{
   int i;
   E_Comp_Win *hide_cw = NULL;
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;

   for (i = 1; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        hide_cw = hwc_drawable->cw;
        e_mod_comp_win_hwcomp_mask_objs_hide(hide_cw);
     }

    hwc_update->update_mode = E_HWCOMP_USE_INVALID;
    hwc_update->num_drawable = 0;
    hwc_update->comp_update = EINA_FALSE;
    _e_mod_comp_hwcomp_update_clear_drawable (hwc_update);
}

EAPI void
_e_mod_comp_hwcomp_enable(Ecore_X_Window win)
{
   int hwc_event, hwc_error;
   _hwc_major = 1;
   _hwc_minor = 0;

   if (!_display) _display = ecore_x_display_get();

   if (XQueryExtension(_display, HWC_NAME, &_hwc_op_code, &hwc_event, &hwc_error))
     {
        HWCQueryVersion(_display, &_hwc_major, &_hwc_minor);
        if (!HWCOpen(_display, win, &_hwc_max_layer))
          {
             _hwc_available = EINA_FALSE;
             return;
          }
        HWCSelectInput(_display, win, HWCAllEvents);
        _hwc_available = EINA_TRUE;
     }
   else
     _hwc_available = EINA_FALSE;
}

EAPI Eina_Bool
_e_mod_comp_hwcomp_query(void)
{
   return _hwc_available;
}

EAPI int
_e_mod_comp_hwcomp_max_layer(void)
{
   return _hwc_max_layer;
}

static int
_e_mod_comp_hwcomp_get_event_data(Ecore_X_Event_Generic *e)
{
   if (!e) return -1;

   if (e->extension == _hwc_op_code && e->evtype == HWCConfigureNotify)
     {
        HWCConfigureNotifyCookie *data = (HWCConfigureNotifyCookie *)e->data;
        return data->maxLayer;
     }
   return -1;
}


static void
_e_mod_comp_hwcomp_turn_composite(E_Comp_HWComp *hwcomp, Eina_Bool on)
{

   E_Comp_Canvas *canvas = hwcomp->canvas;
   Eina_Bool manual_render_set = !on;

   if (ecore_evas_manual_render_get(canvas->ee) != manual_render_set)
      ecore_evas_manual_render_set(canvas->ee, manual_render_set);
}

static Eina_Bool
_e_mod_comp_hwcomp_check_fullcomp_mode(E_Comp_Win *cw)
{
   Eina_Bool ret = EINA_FALSE;

   /* Mini app */
   if (STATE_INSET_CHECK(cw) && cw->bd->lock_user_location)
        ret = EINA_TRUE;

   /* Resizing of window */
   if (cw->needpix)
       ret= EINA_TRUE;

   /* Moving of window */
   if ((STATE_INSET_CHECK(cw) || CLASS_ICONIC_CHECK(cw)) &&
         cw->hwc.geo_changed)
     {
       cw->hwc.geo_changed--;
       ret= EINA_TRUE;
     }

      /* split */
      if (cw->bd->client.e.state.ly.curr_ly)
          ret= EINA_TRUE;

   return ret;
}

static Eina_Bool
_e_mod_comp_hwcomp_verify_update_mode(E_Comp_HWComp *hwcomp, E_Comp_HWComp_Update *hwc_update)
{
    E_Comp *c = NULL;
    E_Comp_Canvas *canvas = NULL;
    E_Zone *zone = NULL;
    E_Comp_Layer *ly = NULL;
    Eina_Bool effect_status = EINA_FALSE, move_status = EINA_FALSE, rotation_status = EINA_FALSE;

    E_Comp_Win *cw;
    E_Comp_Object *co = NULL, *_co = NULL;
    Eina_List *l;
    Eina_Bool check_nocomp = EINA_TRUE;
    int hybrid_idx = 0;

#if OPTIMIZED_HWC
    Eina_Rectangle total_rect = {0,};
    Eina_Bool miniapp = EINA_FALSE;
    Eina_Bool keyboard = EINA_FALSE;
    Eina_Bool magnifier = EINA_FALSE;
    Eina_Bool clipboard = EINA_FALSE;
    Eina_Bool split_launcher = EINA_FALSE;
    const char *name = NULL;
    Eina_Rectangle clipboard_rect = {0,};
    Eina_Rectangle zone_rect = {0,};
#endif

    c = hwcomp->c;
    E_CHECK_RETURN(c, 0);

    canvas = hwcomp->canvas;
    E_CHECK_RETURN(canvas, 0);

    zone = canvas->zone;
    E_CHECK_RETURN(zone, 0);

#if OPTIMIZED_HWC
    zone_rect.x = zone->x;
    zone_rect.y = zone->y;
    zone_rect.w = zone->h;
    zone_rect.h = zone->h;
#endif

    /* pre-check full composite condition */
    ly = e_mod_comp_canvas_layer_get(canvas, "effect");
    if (ly)
    effect_status = evas_object_visible_get(ly->layout);
    ly = e_mod_comp_canvas_layer_get(canvas, "move");
    if (ly)
    move_status = evas_object_visible_get(ly->layout);
    if ((canvas->zone) && (canvas->zone->rot.block_count))
      rotation_status = EINA_TRUE;

     /*
      * basically canvas->ee_win always will be set at top layer0
      * this drawable drawn by GLES
      */
      hwc_update->hwc_drawable[0]->cw = NULL;
      hwc_update->hwc_drawable[0]->d = canvas->ee_win;
      hwc_update->hwc_drawable[0]->set_drawable = EINA_TRUE;

#if OPTIMIZED_HWC
    hwc_update->hwc_drawable[0]->first_update = 0;

#if DEBUG_HWC
    hwc_update->hwc_drawable[0]->update_count = 0;  //for debugging
#endif

#endif

    if (_hwc_max_layer == 1)
      goto full_comp;

    if (hwcomp->force_composite)
      goto full_comp;

    /* full composite */
    if (effect_status == EINA_TRUE)
        goto full_comp;
    if (move_status == EINA_TRUE)
        goto full_comp;
    if (rotation_status == EINA_TRUE)
        goto full_comp;

     /*
       * I have 2 special cases
       *
       * CASE 1 : 1 full screen app overlapes full screen
       *
       * CASE 2: Some windows are can be set at HW layer
       *   - If num of windows are bigger than "hwcomp->num_overlays-1" additional algorithm should be used.
       */
      EINA_INLIST_REVERSE_FOREACH (c->wins, cw)
       {
          EINA_LIST_FOREACH (cw->objs, l, co)
            {
               if (!co) continue;
               if (co->canvas != canvas) continue;

               _co = co;
               break;
            }

          if ((_co) && (cw->bd) && (cw->bd->client.win) &&
              (!cw->shaped) && (!cw->rects) && (cw->hwc.set_drawable))
            {
               int abs_x, abs_y, abs_w, abs_h;
#if OPTIMIZED_HWC
               abs_x = cw->x;
               abs_y = cw->y;
               abs_w = cw->w;
               abs_h = cw->h;

               name = _e_mod_comp_hwcomp_border_name_get(cw->bd);

#else
               ecore_x_window_geometry_get(cw->win, &abs_x, &abs_y, &abs_w, &abs_h);
#endif

               if (E_CONTAINS(zone->x, zone->y, zone->w, zone->h, abs_x, abs_y, abs_w, abs_h) || (name && !strcmp(name, "SPLIT_LAUNCHER")))
                 {
                    /* check nocomp mode with the first cw once */
                    if (check_nocomp && REGION_EQUAL_TO_ZONE(cw, zone) && cw->use_dri2 && (!cw->argb)
                        && !cw->hwc.resize_pending)
                      {
                         hwc_update->update_mode = E_HWCOMP_USE_NOCOMP_MODE;
                         hwc_update->num_drawable = 1;
                         hwc_update->hwc_drawable[0]->cw = cw;
                         hwc_update->hwc_drawable[0]->d = cw->bd->client.win;
                         hwc_update->hwc_drawable[0]->set_drawable = EINA_TRUE;

#if OPTIMIZED_HWC
                         hwc_update->hwc_drawable[0]->first_update = 0;
#if DEBUG_HWC
                         hwc_update->hwc_drawable[0]->update_count = 0;  //for debugging
#endif

#endif
                         goto done;
                      }
                    else
                      {
                         if (check_nocomp) check_nocomp = EINA_FALSE;
                      }

#if DEBUG_HWC
                    ELBF(ELBT_COMP, 0, 0, "%15.15s|VERIFY_UPDATE:win id:0x%08x name:%s", "HWC:NAME", cw->win, cw->bd->client.icccm.name);
                    ELBF(ELBT_COMP, 0, 0, "%15.15s|VERIFY_UPDATE:win id:0x%08x class:%s", "HWC:NAME", cw->win, cw->bd->client.icccm.class);
                    ELBF(ELBT_COMP, 0, 0, "%15.15s|VERIFY_UPDATE:win id:0x%08x title:%s", "HWC:NAME", cw->win, cw->bd->client.icccm.title);
                    ELBF(ELBT_COMP, 0, 0, "%15.15s|VERIFY_UPDATE:win id:0x%08x iconname:%s", "HWC:NAME", cw->win, cw->bd->client.icccm.icon_name);
#endif
                    /* skip the cw if the cw is not the use_dri2 */
                    if (!cw->use_dri2 && !cw->argb)
                        continue;

                    /* skip the cw is the Key Magnifier */
#if OPTIMIZED_HWC
                    if (name && !strcmp(name, "Key Magnifier"))
                      {

                         magnifier = EINA_TRUE;
                         hwc_update->keymag_rect.x = cw->x;
                         hwc_update->keymag_rect.y = cw->y;
                         hwc_update->keymag_rect.w = cw->w;
                         hwc_update->keymag_rect.h = cw->h;
#if DEBUG_HWC
                         ELBF(ELBT_COMP, 0, 0, "%15.15s|KM_RECT rect(%d,%d,%d,%d)", "HWC:RECT", cw->x, cw->y, cw->w, cw->h);
#endif
                         continue;
                      }
#else
                    if (cw->bd->client.icccm.name && !strcmp(cw->bd->client.icccm.name, "Key Magnifier"))
                        continue;
#endif

                    /* skip the cw is the Prediction Window */
#if OPTIMIZED_HWC
                    if (cw->argb && name && !strcmp(name, "Prediction Window"))
                        continue;
#else
                    if ( cw->argb && (cw->bd->client.icccm.name && !strcmp(cw->bd->client.icccm.name, "Prediction Window")))
                        continue;
#endif

#if OPTIMIZED_HWC
                    /* skip the cw is the Key Board / Clipboard */
                    if (name && !strcmp(name, "Virtual Keyboard"))
                      {
                         keyboard = EINA_TRUE;
                         hwc_update->ime_rect.x = cw->x;
                         hwc_update->ime_rect.y = cw->y;
                         hwc_update->ime_rect.w = cw->w;
                         hwc_update->ime_rect.h = cw->h;
                         continue;
                      }

                    if (name && !strcmp(name, "Clipboard History Manager"))
                      {
                         clipboard = EINA_TRUE;
                         clipboard_rect.x = cw->x;
                         clipboard_rect.y = cw->y;
                         clipboard_rect.w = cw->w;
                         clipboard_rect.h = cw->h;
                         continue;
                      }

                    if (name && !strcmp(name, "SPLIT_LAUNCHER"))
                      {
                         split_launcher = EINA_TRUE;
                         hwc_update->split_launcher_rect.x = cw->x;
                         hwc_update->split_launcher_rect.y = cw->y;
                         hwc_update->split_launcher_rect.w = cw->w;
                         hwc_update->split_launcher_rect.h = cw->h;
                         eina_rectangle_intersection(&(hwc_update->split_launcher_rect), &zone_rect);
                         continue;
                      }

                    if (STATE_INSET_CHECK(cw) || (!cw->argb && (cw->w < zone->w || cw->h < zone->h)))
                      {
                        if (_e_mod_comp_hwcomp_check_fullcomp_mode(cw) != EINA_TRUE)
                        {
                           e_mod_comp_win_hwcomp_mask_objs_hide(cw);
                           miniapp = EINA_TRUE;
                           continue;
                        }
                        else
                           goto full_comp;
                      }
#endif
                    /* argb */
                    if (cw->argb)
                        goto full_comp;

                    if (hybrid_idx < hwc_update->num_overlays-1)
                      {
                         hybrid_idx++;

                         /* I think there are some windows which is no full screen */
                         /* Mini app || Splited window || No magnifier of keypad */
                         hwc_update->update_mode = E_HWCOMP_USE_HYBRIDCOMP_MODE;
                         hwc_update->num_drawable = hybrid_idx + 1;
                         hwc_update->hwc_drawable[hybrid_idx]->cw = cw;
                         hwc_update->hwc_drawable[hybrid_idx]->d = cw->bd->client.win;
#if OPTIMIZED_HWC

#if DEBUG_HWC
                         hwc_update->hwc_drawable[hybrid_idx]->update_count = 0;  //for debugging
#endif

                         if (hwc_update->keymag_present == EINA_FALSE &&
                             hwc_update->ime_present == EINA_FALSE)
                         /* Calculate all rects with union */
                           {
                              Eina_Rectangle this_rect;
                              this_rect.x = cw->x;
                              this_rect.y = cw->y;
                              this_rect.w = cw->w;
                              this_rect.h = cw->h;

                              eina_rectangle_union(&total_rect, &this_rect);
                           }
#endif

                         if ((REGION_EQUAL_TO_ZONE(cw, zone) && (!cw->argb))) break;
                      }
                 }
               /* full composite */
               else if (E_INTERSECTS(zone->x, zone->y, zone->w, zone->h, abs_x, abs_y, abs_w, abs_h))
                  goto full_comp;

               /* full composite */
               if (_e_mod_comp_hwcomp_check_fullcomp_mode(cw) == EINA_TRUE)
                  goto full_comp;

            }
          else if ((_co) && (cw->redirected) && (!cw->bd) &&
                   (!cw->shaped) && (!cw->rects) && (cw->hwc.set_drawable))
             goto full_comp;
       }
      if (hwc_update->num_drawable == 0)
         goto full_comp;

#if OPTIMIZED_HWC

      if (!magnifier && !keyboard && !clipboard
          && !miniapp && !split_launcher && hwc_update->update_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE
          && total_rect.x == zone->x && total_rect.y == zone->y
          && total_rect.w == zone->w && total_rect.h == zone->h)
        {
           //update the drawables
           int idx = 0;
           for (idx = 0; idx < hwc_update->num_drawable - 1; idx++)
             {
                hwc_update->hwc_drawable[idx]->cw = hwc_update->hwc_drawable[idx+1]->cw;
                hwc_update->hwc_drawable[idx]->d = hwc_update->hwc_drawable[idx+1]->d;
                hwc_update->hwc_drawable[idx]->set_drawable = EINA_TRUE;

#if DEBUG_HWC
                hwc_update->hwc_drawable[idx]->update_count = hwc_update->hwc_drawable[idx+1]->update_count;  //for debugging
#endif
            }
           hwc_update->num_drawable--;
       }

#endif

done:
#if OPTIMIZED_HWC
   hwc_update->hwcomp->miniapp_present = miniapp;
   hwc_update->ime_present = keyboard;
   hwc_update->keymag_present = magnifier;
   hwc_update->split_launcher_rect_present = split_launcher;

   if (keyboard && clipboard)
     {
        if ((hwc_update->ime_rect.w * hwc_update->ime_rect.h) < (clipboard_rect.w * clipboard_rect.h))
          {
             hwc_update->ime_rect.x = clipboard_rect.x;
             hwc_update->ime_rect.y = clipboard_rect.y;
             hwc_update->ime_rect.w = clipboard_rect.w;
             hwc_update->ime_rect.h = clipboard_rect.h;
          }
     }
#endif

   return EINA_TRUE;

full_comp:
    _e_mod_comp_hwcomp_update_clear (hwc_update);

    hwc_update->update_mode = E_HWCOMP_USE_FULLCOMP_MODE;
    hwc_update->num_drawable = 1;
    hwc_update->hwc_drawable[0]->cw = NULL;
    hwc_update->hwc_drawable[0]->d = canvas->ee_win;
    hwc_update->hwc_drawable[0]->set_drawable = EINA_TRUE;

    return EINA_TRUE;
}

static void
_e_mod_comp_hwcomp_update_reset_maskobj(E_Comp_HWComp_Update *hwc_update)
{
   E_CHECK(hwc_update);
   int i;
   E_Comp_Win *hide_cw = NULL;
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;

   for (i = 1; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        hide_cw = hwc_drawable->cw;
        if (hide_cw)
          e_mod_comp_win_hwcomp_mask_objs_hide(hide_cw);
     }
}


EAPI E_Comp_HWComp *
e_mod_comp_hwcomp_new (E_Comp_Canvas *canvas)
{
   E_Comp_HWComp *hwcomp = NULL;
   E_CHECK_RETURN(canvas, 0);

   hwcomp = E_NEW(E_Comp_HWComp, 1);
   E_CHECK_RETURN(hwcomp, 0);

   _e_mod_comp_hwcomp_enable(canvas->comp->man->root);

   hwcomp->c = canvas->comp;
   hwcomp->canvas = canvas;

   if (_e_mod_comp_hwcomp_query() == EINA_TRUE)
     {
        hwcomp->num_overlays = _e_mod_comp_hwcomp_max_layer();
        if (hwcomp->num_overlays <= 0) goto fail;
     }

   hwcomp->hwc_update = _e_mod_comp_hwcomp_update_create (hwcomp, hwcomp->num_overlays);
   if (!hwcomp->hwc_update) goto fail;

#if OPTIMIZED_HWC
   //get the screen size
   ecore_x_screen_size_get(ecore_x_default_screen_get(), &(hwcomp->screen_width), &(hwcomp->screen_height));

#if DEBUG_HWC
   ELBF(ELBT_COMP, 0, 0, "%15.15s|Default Screen Size : (%d, %d)", "HWC:SCREEN_SIZE", hwcomp->screen_width, hwcomp->screen_height);
#endif
#endif


   return hwcomp;

fail:
    if (hwcomp)
      {
         if (hwcomp->hwc_update)
           _e_mod_comp_hwcomp_update_destroy (hwcomp->hwc_update);
         E_FREE(hwcomp);
      }
    return 0;
}

EAPI void
e_mod_comp_hwcomp_free (E_Comp_HWComp *hwcomp)
{
    if (!hwcomp) return;

    _e_mod_comp_hwcomp_update_destroy(hwcomp->hwc_update);

    if (hwcomp->idle_enterer) ecore_idle_enterer_del(hwcomp->idle_enterer);
    if (hwcomp->idle_timer) ecore_timer_del(hwcomp->idle_timer);

    E_FREE(hwcomp);
}

EAPI void
e_mod_comp_hwcomp_update_composite (E_Comp_HWComp *hwcomp)
{
   E_CHECK(hwcomp);
   E_CHECK(hwcomp->hwc_update);

   E_Comp_HWComp_Update *hwc_update = hwcomp->hwc_update;
   Ecore_X_Window root_win = hwcomp->canvas->comp->man->root;

   if (_e_mod_comp_hwcomp_update_check_resized (hwc_update))
     {
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, 0, "%15.15s|canvas->ee_win : %p", "HWC:SKIP(RESIZE)", hwcomp->canvas->ee_win);
        _hwcomp_dbg_print_update (hwc_update, "HWC:SKIP(RESIZE)");
#endif
        return;
     }

   if (hwcomp->fullcomp_pending)
     {
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, 0, "%15.15s|canvas->ee_win : %p", "HWC:PENDING(FULL)", hwcomp->canvas->ee_win);
        _hwcomp_dbg_print_update(hwc_update, "HWC:PENDING(FULL)");
#endif
        return;
     }

   if (_e_mod_comp_hwcomp_update_check_comp(hwc_update))
     {
#if DEBUG_HWC
        ELBF(ELBT_COMP, 0, 0, "%15.15s|canvas->ee_win : %p", "HWC:DRAW_DONE", hwcomp->canvas->ee_win);
        _hwcomp_dbg_print_update (hwc_update, "HWC:Comp");
#endif
         _e_mod_comp_hwcomp_update_set_comp (hwc_update, EINA_FALSE);

         _e_mod_comp_hwcomp_update_set_drawables (hwc_update, root_win);
        if (hwcomp->force_swap)
           {
              E_Comp_Canvas *canvas = hwcomp->canvas;
              if ((canvas) && (canvas->evas) && (canvas->ee))
                {
                   evas_obscured_clear(canvas->evas);
                   evas_damage_rectangle_add(canvas->evas, 0, 0, canvas->w, canvas->h);
                   ecore_evas_manual_render(canvas->ee);
                   hwcomp->force_swap = 0;
                }
           }
      }
}

EAPI void
e_mod_comp_hwcomp_set_full_composite (E_Comp_HWComp *hwcomp)
{
   E_CHECK(hwcomp);
   E_CHECK(hwcomp->hwc_update);

   E_Comp_HWComp_Update *hwc_update = hwcomp->hwc_update;
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;
   E_Comp_Win *hide_cw = NULL;
   int i;

   /* do not change the update until ee_win has done the update */
   if (_e_mod_comp_hwcomp_update_check_comp(hwcomp->hwc_update))
     return;

#if DEBUG_HWC
   _hwcomp_dbg_print_update (hwc_update, "HWC:Set Comp");
#endif

    /* draw all hwc_drawables at the next ee_win update.
       1. hide mask objs
       2. trigger the compositing ee_win
     */

    /* hide mask objs */
    for (i = 1; i < hwc_update->num_drawable; i++)
      {
         hwc_drawable = hwc_update->hwc_drawable[i];
         hide_cw = hwc_drawable->cw;
         e_mod_comp_win_hwcomp_mask_objs_hide(hide_cw);
         _e_mod_comp_hwcomp_reset_comp_countdown(hwc_drawable);
         _e_mod_comp_hwcomp_reset_set_countdown(hwc_drawable);
         hwc_drawable->set_drawable = EINA_FALSE;
      }

    _e_mod_comp_hwcomp_update_set_comp(hwc_update, EINA_TRUE);
    _e_mod_comp_hwcomp_turn_composite(hwcomp, EINA_TRUE);

    hwc_update->hwc_drawable[0]->cw = NULL;
    hwc_update->hwc_drawable[0]->d = hwcomp->canvas->ee_win;
    hwc_update->hwc_drawable[0]->set_drawable = EINA_TRUE;

//    ecore_evas_manual_render(hwcomp->canvas->ee);
}

#if OPTIMIZED_HWC
static void
_e_mod_comp_hwcomp_set_hybrid_composite(E_Comp_HWComp_Update *hwc_update)
{
   E_CHECK(hwc_update);

   E_Comp_HWComp_Drawable *hwc_drawable = NULL;
   int i;

   for (i = 1; i < hwc_update->num_drawable; i++)
     {
        hwc_drawable = hwc_update->hwc_drawable[i];
        hwc_drawable->set_drawable = EINA_TRUE;
        hwc_drawable->first_update = 1;
        _e_mod_comp_hwcomp_reset_comp_countdown(hwc_drawable);
        _e_mod_comp_hwcomp_reset_set_countdown(hwc_drawable);
     }
}
#endif

EAPI Eina_Bool
e_mod_comp_hwcomp_cb_update (E_Comp_HWComp *hwcomp)
{
   E_CHECK_RETURN(hwcomp, 0);

   E_Comp *c = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_Win *show_cw = NULL;
   E_Comp_Win *hide_cw = NULL;
   E_Comp_HWComp_Update *cur_hwc_update = NULL;
   E_Comp_HWComp_Update *new_hwc_update = NULL;
   E_Comp_HWComp_Drawable *hwc_drawable = NULL;
   E_HWComp_Mode cur_mode = E_HWCOMP_USE_INVALID;
   E_HWComp_Mode new_mode = E_HWCOMP_USE_INVALID;
   int i = 0;

   c = hwcomp->c;
   E_CHECK_RETURN(c, EINA_FALSE);
   E_CHECK_RETURN(!(c->lock.locked), EINA_FALSE);

   canvas = hwcomp->canvas;
   E_CHECK_RETURN(canvas, EINA_FALSE);

   cur_hwc_update = hwcomp->hwc_update;
   if (!cur_hwc_update)
     return EINA_FALSE;

   if (hwcomp->idle_timer)
     {
        ecore_timer_del(hwcomp->idle_timer);
        hwcomp->idle_timer = NULL;
     }

   new_hwc_update = _e_mod_comp_hwcomp_update_create (hwcomp, hwcomp->num_overlays);
   if (!new_hwc_update)
     return EINA_FALSE;

   if (!_e_mod_comp_hwcomp_verify_update_mode(hwcomp, new_hwc_update))
     {
        _e_mod_comp_hwcomp_update_destroy (new_hwc_update);
        return EINA_FALSE;
     }

   cur_mode = hwcomp->hwc_update->update_mode;
   new_mode = new_hwc_update->update_mode;

   if (cur_mode != new_mode)
     {
          /* do not change the update until resized is done. */
          if (_e_mod_comp_hwcomp_update_check_resized(cur_hwc_update))
          {
               if (cur_hwc_update->update_mode != E_HWCOMP_USE_NOCOMP_MODE)
               {
#if DEBUG_HWC
                    _hwcomp_dbg_print_update(cur_hwc_update, "HWC:SKIP(RESIZE)");
#endif
                    _e_mod_comp_hwcomp_update_destroy(new_hwc_update);
                  return EINA_TRUE;
               }
            }

        if ((cur_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE) &&
            (new_mode == E_HWCOMP_USE_FULLCOMP_MODE))
               {
             hwcomp->fullcomp_pending = E_HWCOMP_FULL_PENDING_COUNT;
               }

        if ((cur_mode == E_HWCOMP_USE_NOCOMP_MODE) &&
            (new_mode == E_HWCOMP_USE_FULLCOMP_MODE))
          {
             hwcomp->force_swap = 1;
          }

#if DEBUG_HWC
        _hwcomp_dbg_print_change_update (cur_hwc_update, new_hwc_update, "HWC:Mode Change");
#endif

        if (cur_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE)
          {
        /* hide mask_objs */
        for (i = 1; i < cur_hwc_update->num_drawable; i++)
          {
             if (cur_hwc_update->hwc_drawable[i]->cw)
               {
                  hide_cw = cur_hwc_update->hwc_drawable[i]->cw;
                  e_mod_comp_win_hwcomp_mask_objs_hide (hide_cw);
               }
          }
          }

        /* change the new hwc_update */
        _e_mod_comp_hwcomp_update_destroy (hwcomp->hwc_update);
        hwcomp->hwc_update = new_hwc_update;

        /* set drawable is 1 at this case */
        hwcomp->hwc_update->hwc_drawable[0]->set_drawable = EINA_TRUE;

        /* reset count */
        for (i = 0; i < hwcomp->hwc_update->num_drawable; i++)
          {
             _e_mod_comp_hwcomp_reset_comp_countdown(hwcomp->hwc_update->hwc_drawable[i]);
             _e_mod_comp_hwcomp_reset_set_countdown(hwcomp->hwc_update->hwc_drawable[i]);
          }

        /* check ee_win update */
        if (hwcomp->hwc_update->update_mode == E_HWCOMP_USE_NOCOMP_MODE)
          {
             /* update client win. set_drawable */
             _e_mod_comp_hwcomp_update_set_drawables(hwcomp->hwc_update, hwcomp->canvas->comp->man->root);
             _e_mod_comp_hwcomp_turn_composite(hwcomp, EINA_FALSE);
          }
        else if (hwcomp->hwc_update->update_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE)
          {
#if OPTIMIZED_HWC
             _e_mod_comp_hwcomp_set_hybrid_composite(hwcomp->hwc_update);
#endif
             /* update ee_win at the next draw_done of the ee_win */
             _e_mod_comp_hwcomp_update_set_comp(hwcomp->hwc_update, EINA_TRUE);
             _e_mod_comp_hwcomp_turn_composite(hwcomp, EINA_TRUE);
          }
        else if (hwcomp->hwc_update->update_mode == E_HWCOMP_USE_FULLCOMP_MODE)
          {
             /* update ee_win at the next draw_done of the ee_win */
             _e_mod_comp_hwcomp_update_set_comp(hwcomp->hwc_update, EINA_TRUE);
             _e_mod_comp_hwcomp_turn_composite(hwcomp, EINA_TRUE);
          }
     }
   else
     {
        if (cur_hwc_update->update_mode == E_HWCOMP_USE_NOCOMP_MODE)
          {
             /* compare the hwc_drawables with the new hwc_drawbles */
             if (!_e_mod_comp_hwcomp_update_compare_drawables (cur_hwc_update, new_hwc_update))
               {
                  _e_mod_comp_hwcomp_update_migrate_drawables (cur_hwc_update, new_hwc_update);

#if DEBUG_HWC
                  _hwcomp_dbg_print_change_update (cur_hwc_update, new_hwc_update, "HWC:NOCOMP Draw Change");
#endif

                  /* change the new hwc_update */
                  _e_mod_comp_hwcomp_update_destroy (hwcomp->hwc_update);
                  hwcomp->hwc_update = new_hwc_update;

#if DEBUG_HWC
                  _hwcomp_dbg_print_update (hwcomp->hwc_update, "HWC:NOCOMP Draw Change");
#endif
                  /* update client win. set_drawable */
                  _e_mod_comp_hwcomp_update_set_drawables(hwcomp->hwc_update, canvas->comp->man->root);
               }
             else
               {
                  _e_mod_comp_hwcomp_update_destroy(new_hwc_update);
               }
          }
        else if (cur_hwc_update->update_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE)
          {
             Eina_Bool set_flag = EINA_FALSE;

             /* compare the hwc_drawables with the new hwc_drawbles */
             if (!_e_mod_comp_hwcomp_update_compare_drawables (cur_hwc_update, new_hwc_update))
               {
                  _e_mod_comp_hwcomp_update_migrate_drawables (cur_hwc_update, new_hwc_update);

#if DEBUG_HWC
                  _hwcomp_dbg_print_change_update (cur_hwc_update, new_hwc_update, "HWC:HIB Draw Change");
#endif

                  /* change the new hwc_update */
                  _e_mod_comp_hwcomp_update_destroy (hwcomp->hwc_update);
                  hwcomp->hwc_update = new_hwc_update;

#if OPTIMIZED_HWC
                  /* update client win. set_drawable */
                  _e_mod_comp_hwcomp_update_set_drawables(new_hwc_update, canvas->comp->man->root);
#endif
               }
             else
               {
                  /* destory new hwc_update */
                  _e_mod_comp_hwcomp_update_destroy (new_hwc_update);

                  /* do not set/comp until the first update of ee_win has done. */
                  if (_e_mod_comp_hwcomp_update_check_comp (cur_hwc_update))
                    {
                       for (i = 1; i < cur_hwc_update->num_drawable; i++)
                         {
                            hwc_drawable = cur_hwc_update->hwc_drawable[i];
                            if (!hwc_drawable) break;
                            _e_mod_comp_hwcomp_reset_comp_countdown (hwc_drawable);
                            _e_mod_comp_hwcomp_reset_set_countdown (hwc_drawable);
                         }
                    }
                  else
                    {
                       Eina_Bool partial_update = EINA_TRUE;

                       /* index 0 is composite overlay window */
                       for (i = 1; i < cur_hwc_update->num_drawable; i++)
                         {
                            hwc_drawable = cur_hwc_update->hwc_drawable[i];
                            if (!hwc_drawable) break;

                            if (hwc_drawable->set_drawable)
                              {
                                 _e_mod_comp_hwcomp_dec_comp_countdown (hwc_drawable);
                                 /* composite */
                                 if (hwc_drawable->comp_countdown <= 0)
                                   {
                                      hwc_drawable->set_drawable = EINA_FALSE;

                                      /* hide mask obj */
                                      hide_cw = hwc_drawable->cw;
                                      e_mod_comp_win_hwcomp_mask_objs_hide(hide_cw);
                                   }
                                 partial_update = EINA_FALSE;

                              }
                            else
                              {
                                 /* set drawables */
                                 if (hwc_drawable->set_countdown <= 0)
                                   {
                                      ELBF(ELBT_COMP, 0, 0, "%15.15s|### [%d]: %p, %d, %d, %d", "HWC:BEFORE",
                                           i, hwc_drawable->d, hwc_drawable->set_drawable, hwc_drawable->set_countdown, hwc_drawable->comp_countdown);
                                      hwc_drawable->set_drawable = EINA_TRUE;
                                      _e_mod_comp_hwcomp_reset_set_countdown (hwc_drawable);

                                      /* show mask obj */
                                      show_cw = hwc_drawable->cw;
                                      e_mod_comp_win_hwcomp_mask_objs_show(show_cw);

                                      /* set flags */
                                      set_flag = EINA_TRUE;
                                      ELBF(ELBT_COMP, 0, 0, "%15.15s|### [%d]: %p, %d, %d, %d", "HWC:AFTER",
                                           i, hwc_drawable->d, hwc_drawable->set_drawable, hwc_drawable->set_countdown, hwc_drawable->comp_countdown);
                                      partial_update = EINA_FALSE;

                                   }
                              }
                           }

                        if (set_flag)
                          {
#if DEBUG_HWC
                             _hwcomp_dbg_print_update (cur_hwc_update, "HWC:HIB State Change");
#endif
                             /* update client win. set_drawable */
                             _e_mod_comp_hwcomp_update_set_drawables(cur_hwc_update, canvas->comp->man->root);
                           }

                        if (partial_update) return partial_update;
                    }
                 }
          }
        else
          _e_mod_comp_hwcomp_update_destroy(new_hwc_update);

     }
   if (hwcomp && hwcomp->hwc_update && new_mode == E_HWCOMP_USE_HYBRIDCOMP_MODE)
      _e_mod_comp_hwcomp_append_idle_timer(hwcomp);

   return EINA_TRUE;
}

EAPI void
e_mod_comp_hwcomp_process_event (E_Comp *c, Ecore_X_Event_Generic *e)
{
    Eina_List *l = NULL;
    E_Comp_Canvas *canvas = NULL;
    E_Comp_HWComp *hwcomp = NULL;
    int num_overlays = 0;
    E_HWComp_Mode mode = E_HWCOMP_USE_INVALID;
    int i;

    if (!c || !e) return;
    if (_e_mod_comp_hwcomp_query() == EINA_FALSE) return;

    if ((num_overlays = _e_mod_comp_hwcomp_get_event_data(e)) != -1)
      {
         EINA_LIST_FOREACH(c->canvases, l, canvas)
           {
              if (!canvas->hwcomp) continue;

              hwcomp = canvas->hwcomp;
              if (hwcomp && hwcomp->hwc_update)
                 mode = hwcomp->hwc_update->update_mode;

              if (mode != E_HWCOMP_USE_FULLCOMP_MODE)
                {
                  _e_mod_comp_hwcomp_update_reset_maskobj(hwcomp->hwc_update);
                  _e_mod_comp_hwcomp_update_destroy(hwcomp->hwc_update);

                   /* recreate the hwc update with the updated overlays */
                   hwcomp->hwc_update = _e_mod_comp_hwcomp_update_create (hwcomp, num_overlays);
#if DEBUG_HWC
                  ELBF(ELBT_COMP, 0, 0, "%15.15s| maxLayers = %d", "HWC:MAXLAYER CHANGE", num_overlays);
#endif
                  e_mod_comp_hwcomp_set_full_composite(hwcomp);
                  e_mod_comp_render_queue(c);
               }
                   hwcomp->num_overlays = num_overlays;
                   _hwc_max_layer = num_overlays;

#if DEBUG_HWC
              _hwcomp_dbg_print_update(hwcomp->hwc_update, "HWC:Change MAX Layers");
#endif
           }
      }
}

EAPI void
e_mod_comp_hwcomp_check_win_update(E_Comp_Win *cw, int w, int h)
{
    E_CHECK(cw);
    E_CHECK(w > 0);
    E_CHECK(h > 0);

    Eina_List *l = NULL;
    E_Comp_Canvas *canvas = NULL;
    E_Comp_HWComp *hwcomp = NULL;
    E_Comp_HWComp_Update *hwc_update = NULL;
    E_Comp *c = cw->c;
    E_Comp_HWComp_Drawable *hwc_drawable = NULL;
    int i = 0;
    int num_drawable = 0;
    int max_w = 0, max_h = 0;
#if OPTIMIZED_HWC
    const char *name=NULL;

    name = _e_mod_comp_hwcomp_border_name_get(cw->bd);
#endif

    EINA_LIST_FOREACH(c->canvases, l, canvas)
      {
         if (!canvas->hwcomp) continue;

         hwcomp = canvas->hwcomp;
         hwc_update = hwcomp->hwc_update;

         if (hwc_update->update_mode != E_HWCOMP_USE_HYBRIDCOMP_MODE) continue;

         max_w = canvas->zone->w;
         max_h = canvas->zone->h;
         num_drawable = hwc_update->num_drawable;

         for (i = 0; i < num_drawable; i++)
                     {
             hwc_drawable = hwc_update->hwc_drawable[i];

             if (hwc_drawable && hwc_drawable->cw == cw)
                          {
                  /* Mini app */
#if OPTIMIZED_HWC
                  if (STATE_INSET_CHECK(cw) && name && !strcmp(name, "video-player"))
#else
                  if (STATE_INSET_CHECK(cw) &&
                      cw->bd->client.icccm.name &&
                      !strcmp(cw->bd->client.icccm.name, "video-player"))
#endif
                    hwc_drawable->region_update = EINA_TRUE;
                   else
                     {
                       if (_e_mod_comp_hwcomp_update_check_region(hwc_update, cw, w, h, max_w, max_h))
                         hwc_drawable->region_update = EINA_TRUE;
                       else
                         hwc_drawable->region_update = EINA_FALSE;
                     }
                }
           }
      }

}


EAPI void
e_mod_comp_hwcomp_win_update (E_Comp_Win *cw)
{
   E_CHECK(cw);
   E_CHECK(cw->bd);

   Eina_List *l = NULL;
   E_Comp_Canvas *canvas = NULL;
   E_Comp_HWComp *hwcomp = NULL;
   E_Comp_HWComp_Update *hwc_update = NULL;
   E_Comp *c = cw->c;

   EINA_LIST_FOREACH(c->canvases, l, canvas)
    {
       if (!canvas->hwcomp) continue;

       hwcomp = canvas->hwcomp;
       hwc_update = hwcomp->hwc_update;

        /* unset the resized flag here */
       _e_mod_comp_hwcomp_update_unset_resized(hwc_update, cw);

       /* do not update the countdown when there are the resized drawables */
       if (!_e_mod_comp_hwcomp_update_check_resized(hwc_update))
         _e_mod_comp_hwcomp_update_countdown(hwc_update, cw);
    }
}

EAPI void
e_mod_comp_hwcomp_set_resize (E_Comp_Win *cw)
{
    E_CHECK(cw);
    E_CHECK(cw->bd);

    Eina_List *l = NULL;
    E_Comp_Canvas *canvas = NULL;
    E_Comp_HWComp *hwcomp = NULL;
    E_Comp_HWComp_Update *hwc_update = NULL;
    E_Comp *c = cw->c;

    EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas->hwcomp) continue;

        hwcomp = canvas->hwcomp;
        hwc_update = hwcomp->hwc_update;

        _e_mod_comp_hwcomp_update_set_resized(hwc_update, cw);
     }
}
EAPI void
e_mod_comp_hwcomp_force_composite_set(E_Comp_HWComp *hwcomp, Eina_Bool set)
{
   E_CHECK(hwcomp);

   if (set)
     {
        hwcomp->comp_ref++;
        if (!hwcomp->force_composite)
          {
             hwcomp->force_composite = set;
          }
     }
   else
     {
        hwcomp->comp_ref--;
        if (hwcomp->comp_ref < 0)
          hwcomp->comp_ref = 0;

        if ((hwcomp->comp_ref == 0) &&
            (hwcomp->force_composite))
          {
             hwcomp->force_composite = EINA_FALSE;
          }
     }
}

EAPI Eina_Bool
e_mod_comp_hwcomp_force_composite_get(E_Comp_HWComp *hwcomp)
{
   E_CHECK_RETURN(hwcomp, 0);
   return hwcomp->force_composite;
}

EAPI void
e_mod_comp_hwcomp_reset_idle_timer(E_Comp_Canvas *canvas)
{

   E_Comp_HWComp *hwcomp = NULL;
   E_CHECK(canvas);

   hwcomp = canvas->hwcomp;
   E_CHECK(hwcomp);

   if (hwcomp->idle_timer)
     {
        ecore_timer_del(hwcomp->idle_timer);
        hwcomp->idle_timer = NULL;
     }

   return;
}

EAPI void
e_mod_comp_hwcomp_win_del(E_Comp_Win *cw)
{

    E_CHECK(cw);

    Eina_List *l = NULL;
    E_Comp_Canvas *canvas = NULL;
    E_Comp_HWComp *hwcomp = NULL;
    E_Comp_HWComp_Update *hwc_update = NULL;
    E_Comp_HWComp_Drawable **drawables = NULL;
    E_Comp *c = cw->c;
    int i;

    EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas || !canvas->hwcomp) continue;

        hwcomp = canvas->hwcomp;
        hwc_update = hwcomp->hwc_update;
        drawables = hwc_update->hwc_drawable;

        for (i=0; i < hwc_update->num_drawable; i++)
          {
             if (drawables[i]->cw && (cw == drawables[i]->cw))
             {
#if DEBUG_HWC
                ELBF(ELBT_COMP, 0, 0, "%15.15s| cw : %p deleted", "HWC:CW DEL", cw);
#endif
                _e_mod_comp_hwcomp_update_clear(hwc_update);
                return;
             }
          }
     }

   return;
}

EAPI E_HWComp_Mode
e_mod_comp_hwcomp_mode_get(E_Comp_HWComp *hwcomp)
{
   E_CHECK_RETURN(hwcomp, E_HWCOMP_USE_INVALID);
   E_CHECK_RETURN(hwcomp->hwc_update, E_HWCOMP_USE_INVALID);

   return hwcomp->hwc_update->update_mode;
}

EAPI void
e_mod_comp_hwcomp_update_null_set_drawables(E_Comp_HWComp *hwcomp)
{
   E_CHECK(hwcomp);
   Ecore_X_Window root_win = hwcomp->canvas->comp->man->root;
   E_CHECK(root_win);

#if OPTIMIZED_HWC
   HWCSetDrawables(_display, root_win, NULL, NULL, NULL, 0);
#else
   HWCSetDrawables(_display, root_win, NULL, 0);
#endif
}
#else
EAPI E_Comp_HWComp *
e_mod_comp_hwcomp_new (E_Comp_Canvas *canvas)
{
   return 0;
}

EAPI void
e_mod_comp_hwcomp_free (E_Comp_HWComp *hwcomp)
{
   return ;
}

EAPI void
e_mod_comp_hwcomp_update_composite (E_Comp_HWComp *hwcomp)
{
    return ;
}

EAPI void
e_mod_comp_hwcomp_set_full_composite (E_Comp_HWComp *hwcomp)
{
   return ;
}

EAPI Eina_Bool
e_mod_comp_hwcomp_cb_update (E_Comp_HWComp *hwcomp)
{
   return EINA_FALSE;
}

EAPI void
e_mod_comp_hwcomp_process_event (E_Comp *c, Ecore_X_Event_Generic *e)
{
   return ;
}

EAPI void
e_mod_comp_hwcomp_check_win_update(E_Comp_Win *cw, int w, int h)
{
   return ;
}

EAPI void
e_mod_comp_hwcomp_win_update (E_Comp_Win *cw)
{
    return ;
}

EAPI void
e_mod_comp_hwcomp_set_resize (E_Comp_Win *cw)
{
    return ;
}

EAPI void
e_mod_comp_hwcomp_force_composite_set(E_Comp_HWComp *hwcomp, Eina_Bool set)
{
    return ;
}

EAPI Eina_Bool
e_mod_comp_hwcomp_force_composite_get(E_Comp_HWComp *hwcomp)
{
    return EINA_FALSE;
}

EAPI void
e_mod_comp_hwcomp_reset_idle_timer(E_Comp_Canvas *canvas)
{
   return ;
}

EAPI void
e_mod_comp_hwcomp_win_del(E_Comp_Win *cw)
{
   return;
}

EAPI E_HWComp_Mode
e_mod_comp_hwcomp_mode_get(E_Comp_HWComp *hwcomp)
{
   return -1;
}

EAPI void
e_mod_comp_hwcomp_update_null_set_drawables(E_Comp_HWComp *hwcomp)
{
   return;
}
#endif
