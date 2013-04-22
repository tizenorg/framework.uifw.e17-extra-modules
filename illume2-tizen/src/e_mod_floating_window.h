#ifndef _E_MOD_FLOATING_WINDOWS_H
# define _E_MOD_FLOATING_WINDOWS_H

typedef struct _E_Illume_Floating_Border E_Illume_Floating_Border;

typedef enum _E_Illume_Maximize
{
   E_ILLUME_MAXIMIZE_NONE = 0x00000000,
   E_ILLUME_MAXIMIZE_FULLSCREEN = 0x00000001,
   E_ILLUME_MAXIMIZE_SMART = 0x00000002,
   E_ILLUME_MAXIMIZE_EXPAND = 0x00000003,
   E_ILLUME_MAXIMIZE_FILL = 0x00000004,
   E_ILLUME_MAXIMIZE_TYPE = 0x0000000f,
   E_ILLUME_MAXIMIZE_VERTICAL = 0x00000010,
   E_ILLUME_MAXIMIZE_HORIZONTAL = 0x00000020,
   E_ILLUME_MAXIMIZE_BOTH = 0x00000030,
   E_ILLUME_MAXIMIZE_BOTTOM = 0x00000070,
   E_ILLUME_MAXIMIZE_TOP = 0x000000b0,
   E_ILLUME_MAXIMIZE_DIRECTION = 0x000000f0
} E_Illume_Maximize;

struct _E_Illume_Floating_Border
{
   E_Border    *bd;
   Eina_List   *handlers;

   unsigned char moving :1;

   struct{
        unsigned char maximize_top :1;
        unsigned char maximize_bottom :1;
        unsigned char close :1;
   } defer;
   unsigned char changed :1;

   struct{
        unsigned char maximize_by_illume :1;
   } state;
};

int e_mod_floating_init(void);
int e_mod_floating_shutdown(void);
EINTERN Eina_Bool e_mod_floating_border_is_floating(E_Border *bd);
EINTERN Eina_List* e_mod_floating_get_window_list(void);

#endif
