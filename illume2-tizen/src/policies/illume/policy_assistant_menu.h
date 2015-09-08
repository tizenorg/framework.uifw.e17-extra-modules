#ifndef _E_MOD_ASSISTANT_MENU_WINDOWS_H
# define _E_MOD_ASSISTANT_MENU_WINDOWS_H

typedef struct _E_Illume_Assistant_Menu_Border E_Illume_Assistant_Menu_Border;
typedef struct _E_Illume_Assistant_Menu_Animator E_Illume_Assistant_Menu_Animator;

struct _E_Illume_Assistant_Menu_Border
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
        int prev_x, prev_y;
        int kbd_x, kbd_y, kbd_w, kbd_h;
        unsigned char keyboard_show :1;
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
             unsigned char released :1;
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

struct _E_Illume_Assistant_Menu_Animator
{
   void           *obj;
   int             sx;// start x
   int             sy;// start y
   int             ex;// end x
   int             ey;// end y
   int             dx;// distance x
   int             dy;// distance y
   Ecore_Animator *animator;
};

EINTERN Eina_Bool    policy_assistant_menu_init(void);
EINTERN void         policy_assistant_menu_shutdown(void);
EINTERN void         policy_assistant_menu_idle_enterer(void);
EINTERN Eina_List*   policy_assistant_menu_get_window_list(void);
EINTERN void         policy_assistant_menu_zone_layout_floating(E_Border *bd);
EINTERN void         policy_assistant_menu_border_add(E_Border *bd);
EINTERN void         policy_assistant_menu_border_del(E_Border *bd);
EINTERN void         policy_assistant_menu_configure_request(Ecore_X_Event_Window_Configure_Request *ev);
EINTERN void         policy_assistant_menu_window_state_change(E_Border *bd, unsigned int state);
EINTERN void         policy_assistant_menu_window_state_update(E_Border *bd, Ecore_X_Window_State state, Ecore_X_Window_State_Action action);
EINTERN void         policy_assistant_menu_icon_rotation(E_Border *bd, int rotation);
EINTERN void         policy_assistant_menu_icon_relayout_by_keyboard(E_Border *kbd_bd);
#endif
