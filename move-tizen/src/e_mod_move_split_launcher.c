#include "e_mod_move_shared_types.h"
#include "e_mod_move_debug.h"
#include "e_mod_move.h"
#include "e_mod_move_atoms.h"

#define ANIMATION_TIME 0.3
struct _E_Move_Split_Launcher_Animation_Data
{
   Eina_Bool       animating;
   int             sx;// start x
   int             sy;// start y
   int             ex;// end x
   int             ey;// end y
   int             dx;// distance x
   int             dy;// distance y
   Ecore_Animator *animator;
};

static Eina_Bool _e_mod_move_split_launcher_objs_add(E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_objs_del(E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_objs_move(E_Move_Border *mb,int x,int y);

static Eina_Bool _e_mod_move_split_launcher_objs_animation_move(E_Move_Border *mb, int x, int y);
static Eina_Bool _e_mod_move_split_launcher_objs_animation_move_with_time(E_Move_Border *mb, int x, int y, double time);
static Eina_Bool _e_mod_move_split_launcher_objs_animation_frame(void  *data, double pos);
static Eina_Bool _e_mod_move_split_launcher_anim_state_send(E_Move_Border *mb,Eina_Bool state);
static Eina_Bool _e_mod_move_split_launcher_e_border_move(E_Move_Border *mb, int x, int y);
static Eina_Bool _e_mod_move_split_launcher_comp_layer_obj_move_intern(int x,int y);

static Eina_Bool _e_mod_move_split_launcher_above_borders_list_init(E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_borders_list_deinit(E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_borders_list_search(E_Move_Border *sl_mb, E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_border_add(E_Move_Border *sl_mb, E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_border_del(E_Move_Border *sl_mb, E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_border_check(E_Move_Border *sl_mb, E_Move_Border *mb);
static Eina_Bool _e_mod_move_split_launcher_above_borders_stack_align(E_Move_Border *sl_mb);

EINTERN E_Move_Border *
e_mod_move_split_launcher_find(void)
{
   E_Move *m;
   E_Move_Border *mb;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, 0);

   EINA_INLIST_REVERSE_FOREACH(m->borders, mb)
     {
        if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return mb;
     }
   return NULL;
}

EINTERN void*
e_mod_move_split_launcher_internal_data_add(E_Move_Border *mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_CHECK_RETURN(mb, NULL);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(mb), NULL);
   E_CHECK_RETURN(mb->m, NULL);
   sl_data = (E_Move_Split_Launcher_Data *)mb->data;
   if (!sl_data)
     {
        sl_data = E_NEW(E_Move_Split_Launcher_Data, 1);
        E_CHECK_RETURN(sl_data, NULL);
        mb->data = sl_data;
     }
   return mb->data;
}

EINTERN Eina_Bool
e_mod_move_split_launcher_internal_data_del(E_Move_Border *mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   sl_data = (E_Move_Split_Launcher_Data *)mb->data;

   E_FREE(sl_data);
   mb->data = NULL;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_split_launcher_show(E_Move_Border *mb)
{
   int dest_x,dest_y;
   E_Zone *zone = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   sl_data = (E_Move_Split_Launcher_Data *)mb->data;

   if (sl_data->show == EINA_TRUE)
      return EINA_FALSE;

   _e_mod_move_split_launcher_objs_add(mb);
   zone = mb->bd->zone;
   switch (mb->angle)
     {
      case 90:
         dest_x = mb->x;
         dest_y = zone->h - mb->h;
         sl_data->move_distance = mb->h - (zone->h - mb->y);
         break;
      case 270:
         dest_x = mb->x;
         dest_y = zone->y;
         sl_data->move_distance = -mb->y;
         break;
      case 0:
      case 180:
      default:
         dest_x = zone->x;
         dest_y = mb->y;
         sl_data->move_distance = -mb->x;
         break;
     }

   _e_mod_move_split_launcher_e_border_move(mb, dest_x, dest_y);
   _e_mod_move_split_launcher_objs_animation_move(mb, dest_x, dest_y);
   sl_data->show = EINA_TRUE;
   return EINA_TRUE;
}

EINTERN Eina_Bool
e_mod_move_split_launcher_hide(E_Move_Border *mb)
{
   int dest_x,dest_y;
   int move_distance;
   E_Zone *zone = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(mb), EINA_FALSE);
   E_CHECK_RETURN(mb->data, EINA_FALSE);
   sl_data = (E_Move_Split_Launcher_Data *)mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (sl_data->show == EINA_FALSE)
      return EINA_FALSE;
   _e_mod_move_split_launcher_objs_add(mb);

   zone = mb->bd->zone;
   move_distance = 156;
   switch (mb->angle)
     {
      case 90:
         dest_x = mb->x;
         dest_y = mb->y + move_distance;
         break;
      case 270:
         dest_x = mb->x;
         dest_y = mb->y - move_distance;
         break;
      case 0:
      case 180:
      default:
         dest_x = mb->x - move_distance;
         dest_y = mb->y;
         break;
     }

   _e_mod_move_split_launcher_e_border_move(mb, dest_x, dest_y);
   _e_mod_move_split_launcher_objs_animation_move(mb, dest_x, dest_y);
   sl_data->show = EINA_FALSE;
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_objs_add(E_Move_Border *mb)
{
   E_Move *m = NULL;
   Eina_Bool mirror = EINA_TRUE;
   Evas_Object *move_layer = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   sl_data = (E_Move_Split_Launcher_Data *)mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   m = mb->m;

   move_layer = e_mod_move_util_comp_layer_get(m, "move");
   E_CHECK_RETURN(move_layer, EINA_FALSE);

   if (!(mb->objs))
     {
        mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
        e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
        e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
        e_mod_move_bd_move_objs_show(mb);

        e_mod_move_util_screen_input_block(m);

        // Move Layer Stack & Position control
        if (!evas_object_visible_get(move_layer))
           evas_object_show(move_layer);

        if (mb->objs) e_mod_move_util_rotation_lock(mb->m);

        _e_mod_move_split_launcher_above_borders_list_init(mb);
     }

   return EINA_TRUE;
}

EINTERN void e_mod_move_split_launcher_above_border_process(E_Move_Border *mb,
                                                        Eina_Bool      size,
                                                        Eina_Bool      pos,
                                                        Eina_Bool      stack,
                                                        Eina_Bool      visible)
{
   E_Move_Border *sl_mb = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   Eina_Bool is_above_border = EINA_FALSE;
   Eina_Bool is_contain_above_border_list = EINA_FALSE;
   Eina_Bool mirror = EINA_TRUE;
   E_CHECK(mb);

   if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return;

   sl_mb = e_mod_move_split_launcher_find();
   E_CHECK(sl_mb);

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK(sl_data);

   if (!(sl_data->stack_above_borders.use)) return;

   is_above_border = _e_mod_move_split_launcher_above_border_check(sl_mb, mb);
   is_contain_above_border_list = _e_mod_move_split_launcher_above_borders_list_search(sl_mb, mb);

   if (!(is_above_border) && is_contain_above_border_list)
     {
        _e_mod_move_split_launcher_above_border_del(sl_mb, mb);
        return;
     }

   if ( is_above_border && !(is_contain_above_border_list))
     {
        _e_mod_move_split_launcher_above_border_add(sl_mb, mb);
        _e_mod_move_split_launcher_above_borders_stack_align(sl_mb);
     }
   else if ( is_above_border && is_contain_above_border_list)
     {
        if (!mb->objs)
          {
             mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
             e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
             e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
             e_mod_move_bd_move_objs_show(mb);

             _e_mod_move_split_launcher_above_borders_stack_align(sl_mb);
          }

        if (size)
          {
             e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
          }
        if (pos)
          {
             e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
          }
        if (stack)
          {
             _e_mod_move_split_launcher_above_borders_stack_align(sl_mb);
          }
        if (visible)
          {
             ;// do nothing
          }
     }
}

static Eina_Bool
_e_mod_move_split_launcher_objs_del(E_Move_Border *mb)
{
   E_Move        *m = NULL;
   E_Manager     *man = NULL;
   E_Zone        *zone = NULL;
   Evas_Object   *move_layer = NULL;

   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);
   man = m->man;
   E_CHECK_RETURN(man,EINA_FALSE);
   zone = e_util_zone_current_get(man);
   // restore comp layer's position
   if (zone) _e_mod_move_split_launcher_comp_layer_obj_move_intern(zone->x, zone->y);

   move_layer = e_mod_move_util_comp_layer_get(m, "move");
   E_CHECK_RETURN(move_layer, EINA_FALSE);

   if (evas_object_visible_get(move_layer))
     evas_object_hide(move_layer);

   e_mod_move_bd_move_objs_del(mb, mb->objs);

   _e_mod_move_split_launcher_above_borders_list_deinit(mb);

   e_mod_move_util_rotation_unlock(mb->m);
   e_mod_move_util_screen_input_unblock(mb->m);

   mb->objs = NULL;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_objs_move(E_Move_Border *mb,int x,int y)
{
   E_Move *m = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);

   e_mod_move_bd_move_objs_move(mb, x, y);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_comp_layer_obj_move_intern(int x,int y)
{
   E_Move *m = NULL;
   Evas_Object *comp_layer = NULL;
   Evas_Object *effect_layer = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   comp_layer = e_mod_move_util_comp_layer_get(m, "comp");
   if (comp_layer)
     {
        evas_object_move(comp_layer, x, y);
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s comp_layer_obj:0x%x %s()  (%d,%d)\n",
          "EVAS_OBJ", comp_layer, __func__, x, y);
     }


   effect_layer = e_mod_move_util_comp_layer_get(m, "effect");
   if (effect_layer)
     {
        evas_object_move(effect_layer, x, y);
        L(LT_EVENT_OBJ,
          "[MOVE] ev:%15.15s effect_layer_obj:0x%x %s()  (%d,%d)\n",
          "EVAS_OBJ", effect_layer, __func__, x, y);
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_objs_animation_move(E_Move_Border *mb, int x, int y)
{
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);
   return _e_mod_move_split_launcher_objs_animation_move_with_time(mb, x, y, ANIMATION_TIME);
}

static Eina_Bool _e_mod_move_split_launcher_objs_animation_move_with_time(E_Move_Border *mb, int x, int y, double anim_time)
{
   E_Move_Split_Launcher_Animation_Data *anim_data = NULL;
   Ecore_Animator *animator = NULL;
   int sx, sy; //start x, start y
   int angle = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->m, EINA_FALSE);

   if (mb->anim_data)
     {
        return EINA_FALSE;
     }

   anim_data = E_NEW(E_Move_Split_Launcher_Animation_Data, 1);
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   anim_data->sx = mb->x;
   anim_data->sy = mb->y;
   anim_data->ex = x;
   anim_data->ey = y;
   anim_data->dx = anim_data->ex - anim_data->sx;
   anim_data->dy = anim_data->ey - anim_data->sy;

   animator = ecore_animator_timeline_add(anim_time,
                                          _e_mod_move_split_launcher_objs_animation_frame,
                                          anim_data);
   if (!animator)
     {
        memset(anim_data, 0, sizeof(E_Move_Split_Launcher_Animation_Data));
        E_FREE(anim_data);
        return EINA_FALSE;
     }

   anim_data->animator = animator;
   anim_data->animating = EINA_TRUE;
   mb->anim_data = anim_data;

   return EINA_TRUE;
}

static Eina_Bool _e_mod_move_split_launcher_objs_animation_frame(void  *data, double pos)
{
   E_Move_Split_Launcher_Animation_Data *anim_data = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_Move_Border                    *mb = NULL;
   double                            frame = pos;
   int                               x, y;

   anim_data = (E_Move_Split_Launcher_Animation_Data *)data;
   E_CHECK_RETURN(anim_data, EINA_FALSE);

   mb = e_mod_move_split_launcher_find();
   E_CHECK_RETURN(mb, EINA_FALSE);

   sl_data = (E_Move_Split_Launcher_Data *)mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   frame = ecore_animator_pos_map(pos, ECORE_POS_MAP_ACCELERATE, 0.0, 0.0);
   x = anim_data->sx + anim_data->dx * frame;
   y = anim_data->sy + anim_data->dy * frame;
   _e_mod_move_split_launcher_objs_move(mb, x, y);

   if (pos >= 1.0)
     {
        _e_mod_move_split_launcher_objs_move(mb, x, y);
        _e_mod_move_split_launcher_objs_del(mb);
        // send split launcher to "animation state message".
        _e_mod_move_split_launcher_anim_state_send(mb,sl_data->show);

        memset(anim_data, 0, sizeof(E_Move_Split_Launcher_Animation_Data));
        E_FREE(anim_data);
        mb->anim_data = NULL;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_anim_state_send(E_Move_Border *mb,Eina_Bool state)
{
   long d[5] = {0L, 0L, 0L, 0L, 0L};
   Ecore_X_Window win;
   E_Move *m;
   E_Zone *zone = NULL;
   E_Border *bd = NULL;
   int on_screen_state = 0;

   E_CHECK_RETURN(mb, EINA_FALSE);
   m = mb->m;
   E_CHECK_RETURN(m, EINA_FALSE);

   win = e_mod_move_util_client_xid_get(mb);
   E_CHECK_RETURN(win, 0);

   bd = mb->bd;
   E_CHECK_RETURN(bd, EINA_FALSE);
   zone = bd->zone;
   E_CHECK_RETURN(zone, EINA_FALSE);

   if (state) d[0] = 1L;
   else d[0] = 0L;

   ecore_x_client_message32_send
      (win, ATOM_SPLIT_LAUNCHER_SHOW_STATE,
       ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
       d[0], d[1], d[2], d[3], d[4]);

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_e_border_move(E_Move_Border *mb, int x, int y)
{
   E_Border      *bd = NULL;
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(mb->bd, EINA_FALSE);
   bd = mb->bd;

   e_border_move(bd, x, y);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_above_borders_list_init(E_Move_Border *sl_mb)
{
   E_Move *m = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_Move_Border *find_mb = NULL;
   E_Move_Border *stack_base_mb = NULL;
   Eina_Bool mirror = EINA_TRUE;

   E_CHECK_RETURN(sl_mb, EINA_FALSE);

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   if (!sl_data)
      sl_data = (E_Move_Split_Launcher_Data *)e_mod_move_split_launcher_internal_data_add(sl_mb);
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (sl_data->stack_above_borders.list)
      _e_mod_move_split_launcher_above_borders_list_deinit(sl_mb);

   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (TYPE_SPLIT_LAUNCHER_CHECK(find_mb)) break;
        if (find_mb->visible
            && REGION_INTERSECTS_WITH_ZONE(find_mb, sl_mb->bd->zone))
          {
             sl_data->stack_above_borders.list = eina_list_append(sl_data->stack_above_borders.list, find_mb);
             if (!(find_mb->objs))
               {
                  find_mb->objs = e_mod_move_bd_move_objs_add(find_mb, mirror);

                  if (stack_base_mb)
                    {
                       e_mod_move_bd_move_objs_stack_below(find_mb, stack_base_mb);
                    }
                  stack_base_mb = find_mb;

                  e_mod_move_bd_move_objs_move(find_mb, find_mb->x, find_mb->y);
                  e_mod_move_bd_move_objs_resize(find_mb, find_mb->w, find_mb->h);
                  e_mod_move_bd_move_objs_show(find_mb);
               }
          }
     }
   sl_data->stack_above_borders.use = EINA_TRUE;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_above_borders_list_deinit(E_Move_Border *sl_mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_List *l = NULL;

   E_CHECK_RETURN(sl_mb, EINA_FALSE);

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (sl_data->stack_above_borders.list)
     {
        EINA_LIST_REVERSE_FOREACH(sl_data->stack_above_borders.list, l, find_mb)
          {
             e_mod_move_bd_move_objs_del(find_mb, find_mb->objs);
             find_mb->objs = NULL;
             sl_data->stack_above_borders.list = eina_list_remove(sl_data->stack_above_borders.list, find_mb);
          }
     }

   sl_data->stack_above_borders.list = NULL;
   sl_data->stack_above_borders.use = EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_e_mod_move_split_launcher_above_borders_list_search(E_Move_Border *sl_mb,
                                                 E_Move_Border *mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool ret = EINA_FALSE;

   E_CHECK_RETURN(sl_mb, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(sl_mb), EINA_FALSE);
   if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return EINA_FALSE;

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if ((sl_data->stack_above_borders.use) && (sl_data->stack_above_borders.list))
     {
        find_mb = (E_Move_Border *)eina_list_data_find(sl_data->stack_above_borders.list, mb);
        if (find_mb) ret = EINA_TRUE;
     }

   return ret;
}


static Eina_Bool
_e_mod_move_split_launcher_above_border_add(E_Move_Border *sl_mb,
                                        E_Move_Border *mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   Eina_Bool ret = EINA_FALSE;
   Eina_Bool mirror = EINA_TRUE;

   E_CHECK_RETURN(sl_mb, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(sl_mb), EINA_FALSE);
   if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return EINA_FALSE;

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (sl_data->stack_above_borders.use)
     {
        sl_data->stack_above_borders.list = eina_list_append(sl_data->stack_above_borders.list, mb);
        if (!(mb->objs))
          {
             mb->objs = e_mod_move_bd_move_objs_add(mb, mirror);
             e_mod_move_bd_move_objs_move(mb, mb->x, mb->y);
             e_mod_move_bd_move_objs_resize(mb, mb->w, mb->h);
             e_mod_move_bd_move_objs_show(mb);
             ret = EINA_TRUE;
          }
     }
   return ret;
}

static Eina_Bool
_e_mod_move_split_launcher_above_border_del(E_Move_Border *sl_mb,
                                        E_Move_Border *mb)
{
   E_Move_Split_Launcher_Data *sl_data = NULL;
   Eina_Bool ret = EINA_FALSE;

   E_CHECK_RETURN(sl_mb, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(sl_mb), EINA_FALSE);
   if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return EINA_FALSE;

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (sl_data->stack_above_borders.list)
     {
        e_mod_move_bd_move_objs_del(mb, mb->objs);
        mb->objs = NULL;
        sl_data->stack_above_borders.list = eina_list_remove(sl_data->stack_above_borders.list, mb);
        if (eina_list_count(sl_data->stack_above_borders.list) == 0)
           sl_data->stack_above_borders.list = NULL;

        ret = EINA_TRUE;
     }
   return ret;
}

static Eina_Bool
_e_mod_move_split_launcher_above_border_check(E_Move_Border *sl_mb,
                                          E_Move_Border *mb)
{
   E_Move *m = NULL;
   E_Move_Border *find_mb = NULL;
   Eina_Bool ret = EINA_FALSE;
   E_Move_Split_Launcher_Data *sl_data = NULL;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(sl_mb, EINA_FALSE);
   E_CHECK_RETURN(mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(sl_mb), EINA_FALSE);
   if (TYPE_SPLIT_LAUNCHER_CHECK(mb)) return EINA_FALSE;

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (!sl_data->stack_above_borders.use)
      return EINA_FALSE;

   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (find_mb == sl_mb) break;
        if ((find_mb->visible)
            && (REGION_INTERSECTS_WITH_ZONE(find_mb, sl_mb->bd->zone))
            && (find_mb == mb))
          {
             ret = EINA_TRUE;
             break;
          }
     }

   return ret;
}

static Eina_Bool
_e_mod_move_split_launcher_above_borders_stack_align(E_Move_Border *sl_mb)
{
   E_Move *m = NULL;
   E_Move_Border *find_mb = NULL;
   E_Move_Border *sl_above_mb = NULL;
   E_Move_Border *stack_base_mb = NULL;
   E_Move_Split_Launcher_Data *sl_data = NULL;
   Eina_List *l = NULL;
   Eina_Bool mirror = EINA_TRUE;

   m = e_mod_move_util_get();
   E_CHECK_RETURN(m, EINA_FALSE);
   E_CHECK_RETURN(sl_mb, EINA_FALSE);
   E_CHECK_RETURN(TYPE_SPLIT_LAUNCHER_CHECK(sl_mb), EINA_FALSE);

   sl_data = (E_Move_Split_Launcher_Data *)sl_mb->data;
   E_CHECK_RETURN(sl_data, EINA_FALSE);

   if (!(sl_data->stack_above_borders.use)
       || !(sl_data->stack_above_borders.list))
      return EINA_FALSE;

   EINA_INLIST_REVERSE_FOREACH(m->borders, find_mb)
     {
        if (TYPE_SPLIT_LAUNCHER_CHECK(find_mb)) break;

        EINA_LIST_REVERSE_FOREACH(sl_data->stack_above_borders.list, l, sl_above_mb)
          {
             if (find_mb == sl_above_mb)
               {
                  sl_data->stack_above_borders.list = eina_list_remove(sl_data->stack_above_borders.list, sl_above_mb);
                  sl_data->stack_above_borders.list = eina_list_prepend(sl_data->stack_above_borders.list, sl_above_mb);

                  if (!sl_above_mb->objs)
                    {
                       sl_above_mb->objs = e_mod_move_bd_move_objs_add(sl_above_mb, mirror);
                       e_mod_move_bd_move_objs_move(sl_above_mb, sl_above_mb->x, sl_above_mb->y);
                       e_mod_move_bd_move_objs_resize(sl_above_mb, sl_above_mb->w, sl_above_mb->h);
                       e_mod_move_bd_move_objs_show(sl_above_mb);
                    }

                  if (!stack_base_mb)
                    {
                       e_mod_move_bd_move_objs_raise(sl_above_mb);
                       stack_base_mb = sl_above_mb;
                    }
                  else
                    {
                       e_mod_move_bd_move_objs_stack_below(sl_above_mb, stack_base_mb);
                       stack_base_mb = find_mb;
                    }

                  break;
               }
          }
     }

   return EINA_TRUE;
}
