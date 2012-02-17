#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef HAVE_GETTEXT
#define _(str) gettext(str)
#define d_(str, dom) dgettext(PACKAGE dom, str)
#else
#define _(str) (str)
#define d_(str, dom) (str)
#endif

typedef struct _Config        Config;
typedef struct _Mod           Mod;

typedef struct _Match         Match;

struct _Config
{
   unsigned char    use_shadow;
   const char      *shadow_file;
   const char      *shadow_style;
   int              engine;
   unsigned char    indirect;
   unsigned char    texture_from_pixmap;
   unsigned char    lock_fps;
   unsigned char    loose_sync;
   unsigned char    efl_sync;
   unsigned char    grab;
   unsigned char    vsync;
   unsigned char    keep_unmapped;
   unsigned char    send_flush;
   unsigned char    send_dump;
   unsigned char    nocomp_fs;
   unsigned char    smooth_windows;
   int              max_unmapped_pixels;
   int              max_unmapped_time;
   int              min_unmapped_time;
   unsigned char    use_lock_screen;
   float            max_lock_screen_time;
   unsigned char    dri_buff_flip;
   unsigned char    default_window_effect;
   int              fps_average_range;
   unsigned char    fps_corner;
   unsigned char    fps_show;
   float            damage_timeout;

   struct {
      Eina_List    *popups; // used for e popups
      Eina_List    *borders; // used for borders
      Eina_List    *overrides; // used for client menus, tooltips etc.
      Eina_List    *menus; // used for e menus
   } match;
};

struct _Mod
{
   E_Module        *module;

   E_Config_DD     *conf_edd;
   E_Config_DD     *conf_match_edd;
   Config          *conf;

   E_Config_Dialog *config_dialog;
};

struct _Match
{
   const char *title; // glob - used for borders, NULL if not to be used
   const char *name; // glob - used for borders, overrides, popups, NULL if not to be used
   const char *clas; // glob - used for borders, overrides, NULL if not to be used
   const char *role; // glob - used for borders

   int         primary_type; // Ecore_X_Window_Type - used for borders, overrides, first one found - ECORE_X_WINDOW_TYPE_UNKNOWN if not to be used
   char        borderless; // used for borders, 0 == dont use, 1 == borderless, -1 == not borderless
   char        dialog; // used for borders, 0 == don't use, 1 == dialog, -1 == not dialog
   char        accepts_focus; // used for borders, 0 == don't use, 1 == accepts focus, -1 == does not accept focus
   char        vkbd; // used for borders, 0 == don't use, 1 == is vkbd, -1 == not vkbd
   char        quickpanel; // used for borders, 0 == don't use, 1 == is quickpanel, -1 == not quickpanel
   char        argb; // used for borders, overrides, popups, menus, 0 == don't use, 1 == is argb, -1 == not argb
   char        fullscreen; // used for borders, 0 == don't use, 1 == is fullscreen, -1 == not fullscreen
   char        modal; // used for borders, 0 == don't use, 1 == is modal, -1 == not modal

   const char *shadow_style; // shadow style to use
};

extern Mod *_comp_mod;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);

void _e_mod_config_new(E_Module *m);
void _e_mod_config_free(E_Module *m);

#endif
