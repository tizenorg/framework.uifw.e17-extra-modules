#ifndef _E_MOD_FLOATING_WINDOWS_H
# define _E_MOD_FLOATING_WINDOWS_H

typedef struct _E_Illume_Floating_Border E_Illume_Floating_Border;

struct _E_Illume_Floating_Border
{
   E_Border    *bd;
   Eina_List   *handlers;

   struct{
        struct{
             Eina_Rectangle prev, curr;
        } geo;
        unsigned char resized :1;
   } rot;

   struct{
        E_Win *win;
        Evas_Object *edje_icon_layout;
        int x, y, w, h;

        struct{
             struct{
                  int x, y;
             } last_down;
             struct{
                  Ecore_Event_Handler *move;
                  Ecore_Event_Handler *down;
                  Ecore_Event_Handler *up;
             } handlers;
             unsigned char pressed :1;
        } mouse;
        unsigned char moving :1;
   } icon;

   struct{
        unsigned char close :1;
   } defer;
   unsigned char changed :1;
   unsigned char rot_done_wait :1;
   unsigned char placed :1;
   unsigned char pinon :1;
   unsigned char hide :1;
};

EINTERN Eina_Bool    policy_floating_init(void);
EINTERN void         policy_floating_shutdown(void);
EINTERN void         policy_floating_idle_enterer(void);
EINTERN Eina_List*   policy_floating_get_window_list(void);
EINTERN void         policy_zone_layout_floating(E_Border *bd);
EINTERN void         policy_floating_border_add(E_Border *bd);
EINTERN void         policy_floating_border_del(E_Border *bd);
EINTERN void         policy_floating_configure_request(Ecore_X_Event_Window_Configure_Request *ev);
EINTERN void         policy_floating_window_state_change(E_Border *bd, unsigned int state);
EINTERN void         policy_floating_window_state_update(E_Border *bd, Ecore_X_Window_State state, Ecore_X_Window_State_Action action);
EINTERN void         policy_floating_icon_rotation(E_Border *bd, int rotation);
#endif
