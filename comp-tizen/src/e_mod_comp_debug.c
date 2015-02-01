#include "e_mod_comp_shared_types.h"
#include "e_mod_comp.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

typedef struct _E_Mod_Comp_Log_Info
{
   int  type;
   char file[256];
} E_Mod_Comp_Log_Info;

/* TODO: remove */
#define LT_NOTHING   0x0000
#define LT_CREATE    0x0004
#define LT_CONFIGURE 0x0008
#define LT_DRAW      0x0020
#define LT_DUMP      0x0100

/* local subsystem functions */
static void
_e_mod_comp_debug_wins_info_dump(E_Comp *c,
                                 FILE   *fs)
{
   E_Comp_Win *cw;
   Ecore_X_Window cid = 0;
   char *wname = NULL, *wclas = NULL;
   int pid = 0, i = 0;
   char buf[4096], buf2[4096], buf3[4096];
   Eina_Bool res;

   fprintf(fs, "B-----------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, "  NO WINDOW    (CLIENT)  V     x     y    w    h PIXMAP      pw   ph | v  VAL DMGs DONE | PID  WNAME  WCLASS\n");
   fprintf(fs, "------------------------------------------------------------------------------------------------------------------------------\n");

   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        cid = e_mod_comp_util_client_xid_get(cw);
        ecore_x_icccm_name_class_get(cid, &wname, &wclas);
        res = ecore_x_netwm_pid_get(cid, &pid);
        if (!res) pid = 0;

        memset(buf, 0, sizeof(buf));
        memset(buf2, 0, sizeof(buf2));
        memset(buf3, 0, sizeof(buf3));

        if (cw->bd)
          snprintf(buf, sizeof(buf), "0x%07x", cw->bd->client.win);

        if (cw->pixmap)
          snprintf(buf2, sizeof(buf2), "0x%07x", cw->pixmap);

        if (cw->sync_info.val)
          snprintf(buf3, sizeof(buf3), "%d %4d %4d %4d",
                   cw->sync_info.version,
                   cw->sync_info.val,
                   cw->dmg_updates,
                   cw->sync_info.done_count);

        fprintf(fs,
                " %3d 0x%07x %9s %s %5d %5d %4d %4d %9s %4d %4d | %16s | %d %d %d %d %d %d |%4d %s %s\n",
                i,
                cw->win,
                buf,
                cw->visible ? "v" : " ",
                cw->x, cw->y, cw->w, cw->h,
                buf2,
                cw->pw, cw->ph,
                buf3,
                cw->argb,
                cw->shaped,
                cw->show_ready,
                cw->show_done,
                cw->animating,
                cw->use_dri2,
                pid,
                wname ? wname : "",
                wclas ? wclas : "");

        if (wname) free(wname);
        if (wclas) free(wclas);
        wname = wclas = NULL;
        pid = cid = 0;
        i++;
     }
   fprintf(fs, "E-----------------------------------------------------------------------------------------------------------------------------\n");
}

static void
_e_mod_comp_debug_canvas_info_dump(E_Comp        *c,
                                   E_Comp_Canvas *canvas,
                                   FILE          *fs)
{
   Eina_List *l, *ll;
   E_Comp_Object *co, *_co = NULL;
   E_Comp_Win *cw, *_cw = NULL;
   E_Comp_Layer *ly = NULL;
   Eina_List *lm = NULL;
   int x, y, w, h, i = 1;
   const char *file = NULL, *group = NULL;
   double val = 0.0;
   fprintf(fs, "B-----------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " canvas->zone:%p num:%d %d,%d %dx%d\n",
           canvas->zone, canvas->zone->num,
           canvas->zone->x, canvas->zone->y,
           canvas->zone->w, canvas->zone->h);
   fprintf(fs, " c->nocomp:%d use_hw_ov:%d\n", c->nocomp, c->use_hw_ov);
   fprintf(fs, " canvas->nocomp mode:%d force_composite:%d(ref:%d)\n", canvas->nocomp.mode, canvas->nocomp.force_composite, canvas->nocomp.comp_ref);
   fprintf(fs, " canvas->nocomp cw:0x%08x\n", e_mod_comp_util_client_xid_get(canvas->nocomp.cw));
   fprintf(fs, " canvas->animation run:%d num:%d\n", canvas->animation.run, canvas->animation.num);
   fprintf(fs, " H/W ov win:%p\n", canvas->ov);
   fprintf(fs, " Canvas Manual Render State: %d\n", ecore_evas_manual_render_get(canvas->ee));
   fprintf(fs, "------------------------------------------------------------------------------------------------------------------------------\n");

   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   fprintf(fs, " NO     WinID      shobj        obj    found_o   ex   ey   ew   eh | W S O | V SYNC  DMG DONE |\n");
   fprintf(fs, "--------------------------------------------------------------------------------------------------------------------------------------\n");
   Eina_Bool found = 0;

   ly = e_mod_comp_canvas_layer_get(canvas, "comp");
   E_CHECK(ly);

   lm = evas_object_smart_members_get(ly->layout);
   Evas_Object *o = NULL;

   EINA_LIST_REVERSE_FOREACH(lm, ll, o)
     {
        if (!evas_object_visible_get(o)) continue;

        EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
          {
             EINA_LIST_FOREACH(cw->objs, l, co)
               {
                  if (!co) continue;
                  if (co->canvas != canvas) continue;
                  if (co->shadow == o)
                    {
                       found = 1;
                       _co = co;
                       _cw = cw;
                       break;
                    }
               }
          }

        if (found && _co && _cw)
          {
             edje_object_file_get(_co->shadow, &file, &group);
             evas_object_geometry_get(o, &x, &y, &w, &h);

             fprintf(fs,
                     " %2d 0x%07x %p %p %p %4d %4d %4dx%4d %s %s|%s|%s %.1f|%s %.1f|%s %.1f\n",
                     i, _cw->win, _co->shadow, _co->img, o, x, y, w, h,
                     evas_object_visible_get(_co->shadow) ? "v" : "",
                     _cw->hidden_override ? "HIDDEN" : "",
                     group,
                     edje_object_part_state_get(_co->shadow, "clipper", &val), val,
                     edje_object_part_state_get(_co->shadow, "shower", &val), val,
                     edje_object_part_state_get(_co->shadow, "e.swallow.content", &val), val);
          }

        found = 0;
        _co = NULL;
        _cw = NULL;
        file = NULL;
        group = NULL;
        val = 0.0;
        i++;
     }
}

/* externally accessible globals */
EAPI int logtype = LT_NOTHING;

/* externally accessible functions */
EAPI Eina_Bool
e_mod_comp_debug_info_dump(Eina_Bool to_file,
                           const char *name)
{
   E_Comp *c;
   E_Comp_Canvas *canvas;
   Eina_List *l;
   FILE *fs = stderr;

   c = e_mod_comp_util_get();
   E_CHECK_RETURN(c, 0);

   if ((to_file) && (name))
     {
        fs = fopen(name, "w");
        if (!fs)
          {
             fprintf(stderr, "can't open %s file.\n", name);
             fs = stderr;
             to_file = EINA_FALSE;
          }
     }

   _e_mod_comp_debug_wins_info_dump(c, fs);

   EINA_LIST_FOREACH(c->canvases, l, canvas)
     {
        if (!canvas) continue;
        _e_mod_comp_debug_canvas_info_dump(c, canvas, fs);
     }

   if (to_file)
     {
        fflush(fs);
        fclose(fs);
     }
   return EINA_TRUE;
}

EAPI Eina_Bool
e_mod_comp_debug_edje_error_get(Evas_Object *o,
                                Ecore_X_Window win)
{
   Edje_Load_Error err = edje_object_load_error_get(o);
   char *_err_msg = NULL;
   Eina_Bool res = EINA_TRUE;

   if (err != EDJE_LOAD_ERROR_NONE)
     {
        switch (err)
          {
           case EDJE_LOAD_ERROR_GENERIC:                    _err_msg = strdup("ERR_GENERIC");                    break;
           case EDJE_LOAD_ERROR_DOES_NOT_EXIST:             _err_msg = strdup("ERR_DOES_NOT_EXIST");             break;
           case EDJE_LOAD_ERROR_PERMISSION_DENIED:          _err_msg = strdup("ERR_PERMISSION_DENIED");          break;
           case EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: _err_msg = strdup("ERR_RESOURCE_ALLOCATION_FAILED"); break;
           case EDJE_LOAD_ERROR_CORRUPT_FILE:               _err_msg = strdup("ERR_CORRUPT_FILE");               break;
           case EDJE_LOAD_ERROR_UNKNOWN_FORMAT:             _err_msg = strdup("ERR_UNKNOWN_FORMAT");             break;
           case EDJE_LOAD_ERROR_INCOMPATIBLE_FILE:          _err_msg = strdup("ERR_INCOMPATIBLE_FILE");          break;
           case EDJE_LOAD_ERROR_UNKNOWN_COLLECTION:         _err_msg = strdup("ERR_UNKNOWN_COLLECTION");         break;
           case EDJE_LOAD_ERROR_RECURSIVE_REFERENCE:        _err_msg = strdup("ERR_RECURSIVE_REFERENCE");        break;
           default:                                         _err_msg = strdup("ERR_UNKNOWN_ERROR");              break;
          }
        res = EINA_FALSE;
     }

   if (res) _err_msg = strdup("SUCCESS");
   ELBF(ELBT_COMP, 0, win, "%15.15s|o:%p %s", "EDC", o, _err_msg);
   free(_err_msg);

   return res;
}

EAPI Eina_Bool
e_mod_comp_debug_prop_handle(Ecore_X_Event_Window_Property *ev)
{
   Eina_Bool res = EINA_FALSE;
   E_Mod_Comp_Log_Info info = {LT_NOTHING, {0,}};
   unsigned char* data = NULL;
   int ret, n;

   E_CHECK_RETURN(ev, 0);
   E_CHECK_RETURN(ev->win, 0);

   ret = ecore_x_window_prop_property_get
           (ev->win, ATOM_CM_LOG, ECORE_X_ATOM_CARDINAL,
           32, &data, &n);
   E_CHECK_GOTO((ret != -1), cleanup);
   E_CHECK_GOTO(((ret > 0) && (data)), cleanup);

   memcpy(&info, data, sizeof(E_Mod_Comp_Log_Info));
   logtype = info.type;

   if (data) free(data);

   fprintf(stdout, "[COMP] logtupe:0x%08x\n", logtype);

   if (logtype == LT_CREATE)
     {
        e_mod_comp_debug_info_dump(EINA_FALSE, NULL);
     }
   else if (logtype == LT_CONFIGURE)
     {
        int enable = e_configure_registry_exists("appearance/comp");
        if (enable)
          {
             E_Comp *c = e_mod_comp_util_get();
             E_CHECK_RETURN(c, 0);
             e_scale_update();
             e_configure_registry_call("appearance/comp",
                                       e_container_current_get(c->man),
                                       NULL);
          }
     }
   else if (logtype == LT_DRAW)
     {
        E_Comp *c = e_mod_comp_util_get();
        E_Comp_Canvas *canvas;
        Eina_List *l = NULL;
        char msg[4096], buf[2048];

        snprintf(msg, sizeof(msg), "This is a test dialog for debugging purpose.");

        EINA_LIST_FOREACH(c->canvases, l, canvas)
          {
             snprintf(buf, sizeof(buf),
               "<br>"
               "Zone[%d] %d,%d %dx%d<br>"
               "%s 0x%x F:%d(%d) A:%d(%d)",
               canvas->zone->num,
               canvas->zone->x,
               canvas->zone->y,
               canvas->zone->w,
               canvas->zone->h,
               (canvas->nocomp.mode == E_NOCOMP_MODE_RUN) ? "No composite" : "Composite",
               e_mod_comp_util_client_xid_get(canvas->nocomp.cw),
               canvas->nocomp.force_composite,
			   canvas->nocomp.comp_ref,
               canvas->animation.run,
               canvas->animation.num);

             strcat(msg, buf);
          }
        e_util_dialog_internal("Enlightenment Rendering Mode", msg);
     }

   if ((logtype == LT_DUMP) &&
       (strlen(info.file) > 0))
     {
        e_mod_comp_debug_info_dump(EINA_TRUE, info.file);
        ecore_x_client_message32_send
          (ev->win, ATOM_CM_LOG_DUMP_DONE,
          ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
          0, 0, 0, 0, 0);
     }

   res = EINA_TRUE;

cleanup:
   if (data) E_FREE(data);
   return res;
}

#if COMP_DEBUG_PIXMAP
# ifdef ecore_x_composite_name_window_pixmap_get
#  undef ecore_x_composite_name_window_pixmap_get
# endif
EAPI Ecore_X_Pixmap
e_mod_comp_debug_name_window_pixmap_get(Ecore_X_Window w,
                                        const char    *f,
                                        const int      l)
{
   Ecore_X_Pixmap p = 0;
   Ecore_X_Window win = 0;
   E_Border *bd = NULL;

   p = ecore_x_composite_name_window_pixmap_get(w);
   bd = e_border_find_by_window(w);
   if (bd) win = bd->client.win;
   fprintf(stderr,
           "[COMP] %30.30s|%04d 0x%08x NEW  PIXMAP 0x%x\n",
           f, l, win, p);
   return p;
}
#endif /* COMP_DEBUG_PIXMAP */
