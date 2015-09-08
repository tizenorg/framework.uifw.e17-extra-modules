#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

typedef enum _E_Comp_BG_Win_State
{
   E_COMP_BG_WIN_STATE_NONE = 0,
   E_COMP_BG_WIN_STATE_PARENT,
   E_COMP_BG_WIN_STATE_CHILD,
   E_COMP_BG_WIN_STATE_CHILD_WAIT_FOR_DAMAGE
} E_Comp_BG_Win_State;

struct _E_Comp_BG_Win
{
   E_Comp_BG_Win_State  state;
   Evas_Object         *obj;
   int                  x, y, w, h;
   Eina_Bool            hidden;
   struct {
     Ecore_X_Window     parent;
     Ecore_X_Window     child;
     Ecore_X_Window     self;
   } win;
};

/* local subsystem functions */
static void      _e_mod_comp_bg_win_fg_free(E_Comp_BG_Win *fg, E_Comp_Win *cw);
static void      _e_mod_comp_bg_win_bg_free(E_Comp_BG_Win *bg, E_Comp_Win *cw);
static Eina_Bool _e_mod_comp_bg_win_get_prop(Ecore_X_Window win, Ecore_X_Window *wins);
static Eina_Bool _e_mod_comp_bg_win_setup(E_Comp_Win *cw, E_Comp_Win *cw2);
static Eina_Bool _e_mod_comp_bg_win_obj_set(E_Comp_Win *cw);

/* externally accessible functions */
EAPI E_Comp_BG_Win *
e_mod_comp_bg_win_new(void)
{
   E_Comp_BG_Win *bg;
   bg = E_NEW(E_Comp_BG_Win, 1);
   return bg;
}

EAPI void
e_mod_comp_bg_win_free(E_Comp_BG_Win *bg)
{
   E_Comp_Win *cw;
   E_CHECK(bg);
   if (bg->obj) evas_object_del(bg->obj);
   bg->obj = NULL;

   cw = e_mod_comp_win_find(bg->win.self);
   switch (bg->state)
     {
      case E_COMP_BG_WIN_STATE_PARENT:
        _e_mod_comp_bg_win_fg_free(bg, cw);
        break;
      case E_COMP_BG_WIN_STATE_CHILD:
      case E_COMP_BG_WIN_STATE_CHILD_WAIT_FOR_DAMAGE:
        _e_mod_comp_bg_win_bg_free(bg, cw);
        break;
      default:
        break;
     }

   memset(bg, 0, sizeof(E_Comp_BG_Win));
   E_FREE(bg);
}

EAPI Eina_Bool
e_mod_comp_bg_win_handler_prop(Ecore_X_Event_Window_Property *ev)
{
   Eina_Bool res = EINA_FALSE;
   E_Comp_Win *cw[2];
   Ecore_X_Window wins[2];
   int i;

   res = _e_mod_comp_bg_win_get_prop(ev->win, wins);
   E_CHECK_RETURN(res, 0);

   for (i = 0; i < 2; i++)
     {
        cw[i] = e_mod_comp_win_find(wins[i]);
        if (!cw[i])
          {
             cw[i] = e_mod_comp_border_client_find(wins[i]);
             E_CHECK_RETURN(cw[i], 0);
          }
     }

   res = _e_mod_comp_bg_win_setup(cw[0], cw[1]);
   E_CHECK_RETURN(res, 0);

   return EINA_TRUE;
}

EAPI Eina_Bool
e_mod_comp_bg_win_handler_release(E_Comp_Win *cw)
{
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bgwin, 0);
   e_mod_comp_bg_win_free(cw->bgwin);
   cw->bgwin = NULL;
   return EINA_TRUE;
}

EAPI Eina_Bool
e_mod_comp_bg_win_handler_show(E_Comp_Win *cw)
{
   Eina_Bool res = EINA_FALSE;
   E_Comp_Win *cw2;
   Ecore_X_Window wins[2];

   res = _e_mod_comp_bg_win_get_prop
     (e_mod_comp_util_client_xid_get(cw), wins);
   E_CHECK_RETURN(res, 0);

   cw2 = e_mod_comp_win_find(wins[1]);
   if (!cw2)
     {
        cw2 = e_mod_comp_border_client_find(wins[1]);
        E_CHECK_RETURN(cw2, 0);
     }

   res = _e_mod_comp_bg_win_setup(cw, cw2);
   E_CHECK_RETURN(res, 0);

   return EINA_TRUE;
}

EAPI Eina_Bool
e_mod_comp_bg_win_handler_update(E_Comp_Win *cw)
{
   E_Comp_Win *fgcw;
   Eina_Bool res;
   E_CHECK_RETURN(cw, 0);
   E_CHECK_RETURN(cw->bgwin, 0);
   E_CHECK_RETURN(cw->bgwin->win.parent, 0);
   if (cw->bgwin->state != E_COMP_BG_WIN_STATE_CHILD_WAIT_FOR_DAMAGE)
     {
        return EINA_FALSE;
     }

   fgcw = e_mod_comp_win_find(cw->bgwin->win.parent);
   E_CHECK_RETURN(fgcw, 0);

   res = _e_mod_comp_bg_win_obj_set(fgcw);
   E_CHECK_RETURN(res, 0);

   cw->bgwin->state = E_COMP_BG_WIN_STATE_CHILD;

   return EINA_TRUE;
}

/* local subsystem functions */
static void
_e_mod_comp_bg_win_fg_free(E_Comp_BG_Win *fg,
                           E_Comp_Win    *cw)
{
   E_Comp_BG_Win *bg;
   E_Comp_Win *bgcw, *fgcw;
   E_CHECK(fg->win.child);

   bgcw = e_mod_comp_win_find(fg->win.child);
   E_CHECK(bgcw);

   bg = bgcw->bgwin;
   E_CHECK(bg);
   E_CHECK(bg->win.parent);

   fgcw = e_mod_comp_win_find(bg->win.parent);
   E_CHECK(fgcw);

   E_CHECK(fgcw == cw);

   bg->win.parent = 0;
}

static void
_e_mod_comp_bg_win_bg_free(E_Comp_BG_Win *bg,
                           E_Comp_Win    *cw)
{
   E_Comp_BG_Win *fg;
   E_Comp_Win *fgcw, *bgcw;
   E_CHECK(bg->win.parent);

   fgcw = e_mod_comp_win_find(bg->win.parent);
   E_CHECK(fgcw);

   fg = fgcw->bgwin;
   E_CHECK(fg);
   E_CHECK(fg->win.child);

   bgcw = e_mod_comp_win_find(fg->win.child);
   E_CHECK(bgcw);

   E_CHECK(bgcw == cw);

   fg->win.child = 0;

   e_mod_comp_bg_win_free(fg);
}

static Eina_Bool
_e_mod_comp_bg_win_get_prop(Ecore_X_Window  win,
                            Ecore_X_Window *wins)
{
   Eina_Bool res = EINA_FALSE;
   int ret, cnt;
   unsigned char *data = NULL;
   ret = ecore_x_window_prop_property_get
           (win, ATOM_NET_CM_WINDOW_BACKGROUND,
           ECORE_X_ATOM_CARDINAL, 32, &data, &cnt);
   E_CHECK_GOTO((ret > 0), cleanup);
   E_CHECK_GOTO((data && (cnt == 2)), cleanup);

   memcpy(wins, data, sizeof(Ecore_X_Window) * cnt);

   res = EINA_TRUE;

cleanup:
   if (data) E_FREE(data);
   return res;
}

static Eina_Bool
_e_mod_comp_bg_win_setup(E_Comp_Win *cw,
                         E_Comp_Win *cw2)
{
   Eina_Bool res = EINA_FALSE;

   if (TYPE_BG_CHECK(cw) ||
       !TYPE_BG_CHECK(cw2) ||
       (cw == cw2))
     {
        return EINA_FALSE;
     }

   if (cw->bgwin)
     e_mod_comp_bg_win_free(cw->bgwin);

   cw->bgwin = e_mod_comp_bg_win_new();
   E_CHECK_GOTO(cw->bgwin, cleanup);
   cw->bgwin->state = E_COMP_BG_WIN_STATE_PARENT;

   if (!cw2->bgwin)
     {
        cw2->bgwin = e_mod_comp_bg_win_new();
        E_CHECK_GOTO(cw2->bgwin, cleanup);

        e_mod_comp_src_hidden_set_func
          (NULL, NULL, (E_Manager_Comp_Source *)cw2, 1);

        cw2->bgwin->hidden = EINA_TRUE;
     }

   cw->bgwin->win.child = cw2->win;
   cw2->bgwin->win.parent = cw->win;
   if (cw2->dmg_updates <= 1)
     {
        cw2->bgwin->state = E_COMP_BG_WIN_STATE_CHILD_WAIT_FOR_DAMAGE;
        res = EINA_TRUE;
        goto cleanup;
     }

   cw2->bgwin->state = E_COMP_BG_WIN_STATE_CHILD;

   res = _e_mod_comp_bg_win_obj_set(cw);
   E_CHECK_GOTO(res, cleanup);

   res = EINA_TRUE;

cleanup:
   if (!res && cw->bgwin)
     {
        e_mod_comp_bg_win_free(cw->bgwin);
        cw->bgwin = NULL;
     }
   return res;
}

static Eina_Bool
_e_mod_comp_bg_win_obj_set(E_Comp_Win *cw)
{
   Eina_List *l;
   E_Comp_Object *co;
   E_Comp_Win *cw2;
   Evas *evas;
   Eina_Bool res = EINA_FALSE, set = EINA_FALSE;
   E_CHECK_RETURN(cw->bgwin, 0);

   cw2 = e_mod_comp_win_find(cw->bgwin->win.child);
   E_CHECK_RETURN(cw2, 0);

   cw->bgwin->x = 0;
   cw->bgwin->y = 0;
   cw->bgwin->w = cw2->pw;
   cw->bgwin->h = cw2->ph;
   cw->bgwin->obj = e_mod_comp_win_mirror_add(cw2);
   E_CHECK_GOTO(cw->bgwin->obj, finish);

   evas = evas_object_evas_get(cw->bgwin->obj);
   E_CHECK_GOTO(evas, finish);
   EINA_LIST_FOREACH(cw->objs, l, co)
     {
        if (!co) continue;
        if (evas == evas_object_evas_get(co->shadow))
          {
             res = edje_object_part_swallow
                (co->shadow, "e.swallow.bgcontent",
                cw->bgwin->obj);
             E_CHECK_GOTO(res, finish);
             set = EINA_TRUE;
          }
     }
   E_CHECK_GOTO(set, finish);

   if (cw2->bgwin &&
       !cw2->bgwin->hidden)
     {
        e_mod_comp_src_hidden_set_func
          (NULL, NULL, (E_Manager_Comp_Source *)cw2, 1);
        cw2->bgwin->hidden = EINA_TRUE;
     }

   e_mod_comp_win_render_queue(cw);

   res = EINA_TRUE;

finish:
   return res;
}
