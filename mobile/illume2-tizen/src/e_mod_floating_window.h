#ifndef _E_MOD_FLOATING_WINDOWS_H
# define _E_MOD_FLOATING_WINDOWS_H

typedef struct _E_Illume_Floating_Border E_Illume_Floating_Border;

struct _E_Illume_Floating_Border
{
   E_Border    *bd;
   Eina_List   *handlers;

   struct{
        unsigned char close :1;
   } defer;
   unsigned char changed :1;
};

int e_mod_floating_init(void);
int e_mod_floating_shutdown(void);
EINTERN Eina_Bool e_mod_floating_border_is_floating(E_Border *bd);
EINTERN Eina_List* e_mod_floating_get_window_list(void);

#endif
