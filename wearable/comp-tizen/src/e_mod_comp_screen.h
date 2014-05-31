#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_SCREEN_H
#define E_MOD_COMP_SCREEN_H

typedef struct _E_Comp_Screen_Rotation E_Comp_Screen_Rotation;
typedef struct _E_Comp_Screen_Lock     E_Comp_Screen_Lock;

struct _E_Comp_Screen_Rotation
{
   Eina_Bool    enabled : 1;
   int          angle;
   int          scr_w;
   int          scr_h;
};

struct _E_Comp_Screen_Lock
{
   Eina_Bool    locked : 1;
   Ecore_Timer *timeout;
};

EINTERN Eina_Bool e_mod_comp_screen_rotation_init(E_Comp_Screen_Rotation *r, Ecore_X_Window root, int w, int h);
EINTERN Eina_Bool e_mod_comp_screen_lock_init(E_Comp_Screen_Lock *l);
EINTERN Eina_Bool e_mod_comp_screen_lock_handler_message(Ecore_X_Event_Client_Message *ev);
EINTERN void      e_mod_comp_screen_lock_func(void *data, E_Manager *man);
EINTERN void      e_mod_comp_screen_unlock_func(void *data, E_Manager *man);

#endif
#endif
