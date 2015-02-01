#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_DEBUG_H
#define E_MOD_COMP_DEBUG_H

#define E_CHECK(x)           do {if (!(x)) return;    } while(0)
#define E_CHECK_RETURN(x, r) do {if (!(x)) return (r);} while(0)
#define E_CHECK_GOTO(x, l)   do {if (!(x)) goto l;    } while(0)

#define COMP_DEBUG_PIXMAP 0

#if COMP_DEBUG_PIXMAP
EAPI Ecore_X_Pixmap e_mod_comp_debug_name_window_pixmap_get(Ecore_X_Window w, const char *f, const int l);

#define ecore_x_composite_name_window_pixmap_get(w)                    \
   e_mod_comp_debug_name_window_pixmap_get(w, __func__, __LINE__)

#define ecore_x_pixmap_free(p) {                                       \
   ecore_x_pixmap_free(p);                                             \
   fprintf(stderr,                                                     \
           "[COMP] %30.30s|%04d 0x%08x FREE PIXMAP 0x%x\n",            \
           __func__, __LINE__, e_mod_comp_util_client_xid_get(cw), p); \
}
#endif /* COMP_DEBUG_PIXMAP */

/* TODO: clean up nocomp logic */
#define USE_NOCOMP_DISPOSE 0
#define USE_SHADOW 0
#define MOVE_IN_EFFECT 0
#define DEBUG_HWC_COLOR 0
#define DEBUG_HWC 0
#define OPTIMIZED_HWC 1

EAPI Eina_Bool e_mod_comp_debug_info_dump(Eina_Bool to_file, const char *name);
EAPI Eina_Bool e_mod_comp_debug_edje_error_get(Evas_Object *o, Ecore_X_Window win);
EAPI Eina_Bool e_mod_comp_debug_prop_handle(Ecore_X_Event_Window_Property *ev);

#endif
#endif
