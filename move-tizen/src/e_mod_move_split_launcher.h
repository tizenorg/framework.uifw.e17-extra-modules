#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_SPLIT_LAUNCHER_H
#define E_MOD_MOVE_SPLIT_LAUNCHER_H

struct _E_Move_Split_Launcher_Data
{
   int move_distance;
   Eina_Bool show;

   struct {
      Eina_List *list;
      Eina_Bool use;
   } stack_above_borders;
};

typedef struct _E_Move_Split_Launcher_Data E_Move_Split_Launcher_Data;
typedef struct _E_Move_Split_Launcher_Animation_Data E_Move_Split_Launcher_Animation_Data;

EINTERN E_Move_Border *e_mod_move_split_launcher_find(void);
EINTERN Eina_Bool      e_mod_move_split_launcher_show(E_Move_Border *mb);
EINTERN Eina_Bool      e_mod_move_split_launcher_hide(E_Move_Border *mb);
EINTERN void*          e_mod_move_split_launcher_internal_data_add(E_Move_Border *mb);
EINTERN Eina_Bool      e_mod_move_split_launcher_internal_data_del(E_Move_Border *mb);
EINTERN void           e_mod_move_split_launcher_above_border_process(E_Move_Border *mb,Eina_Bool size,Eina_Bool pos,Eina_Bool stack,Eina_Bool visible);

#endif
#endif
