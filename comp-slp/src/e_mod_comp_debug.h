#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_DEBUG_H
#define E_MOD_COMP_DEBUG_H

#define E_CHECK(x)           do {if (!(x)) return;    } while(0)
#define E_CHECK_RETURN(x, r) do {if (!(x)) return (r);} while(0)
#define E_CHECK_GOTO(x, l)   do {if (!(x)) goto l;    } while(0)

#define COMP_LOG_BUILD_ENABLE 1

#if COMP_LOG_BUILD_ENABLE
# define LT_NOTHING   0x0000
# define LT_EVENT_X   0x0001
# define LT_EVENT_BD  0x0002
# define LT_CREATE    0x0004
# define LT_CONFIGURE 0x0008
# define LT_DAMAGE    0x0010
# define LT_DRAW      0x0020
# define LT_SYNC      0x0040
# define LT_EFFECT    0x0080
# define LT_DUMP      0x0100
# define LT_ALL       0xFFFF

extern EINTERN int logtype;

# include <stdarg.h>
# define L(t, f, x...) { \
   if (logtype & t)      \
     printf(f, ##x);     \
}
#else
# define L(...) { ; }
#endif /* COMP_LOG_BUILD_ENABLE */

#define COMP_DEBUG_PIXMAP 0

#if COMP_DEBUG_PIXMAP
EINTERN Ecore_X_Pixmap e_mod_comp_debug_name_window_pixmap_get(Ecore_X_Window w, const char *f, const int l);

#define ecore_x_composite_name_window_pixmap_get(w)                    \
   e_mod_comp_debug_name_window_pixmap_get(w, __func__, __LINE__)

#define ecore_x_pixmap_free(p) {                                       \
   ecore_x_pixmap_free(p);                                             \
   fprintf(stderr,                                                     \
           "[COMP] %30.30s|%04d 0x%08x FREE PIXMAP 0x%x\n",            \
           __func__, __LINE__, e_mod_comp_util_client_xid_get(cw), p); \
}
#endif /* COMP_DEBUG_PIXMAP */

EINTERN Eina_Bool e_mod_comp_debug_info_dump(Eina_Bool to_file, const char *name);
EINTERN Eina_Bool e_mod_comp_debug_edje_error_get(Evas_Object *o, Ecore_X_Window win);
EINTERN Eina_Bool e_mod_comp_debug_prop_handle(Ecore_X_Event_Window_Property *ev);

#endif
#endif
