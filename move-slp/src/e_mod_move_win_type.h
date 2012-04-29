#ifndef E_MOD_WIN_TYPE_H
#define E_MOD_WIN_TYPE_H

typedef enum _E_Move_Win_Type E_Move_Win_Type;

enum _E_Move_Win_Type
{
   WIN_NORMAL,
   WIN_QUICKPANEL_BASE,
   WIN_QUICKPANEL_MINICONTROLLER,
   WIN_INDICATOR,
   WIN_APP_TRAY
};

E_Move_Win_Type e_mod_move_win_type_get(E_Border *bd);

#endif // E_MOD_WIN_TYPE_H
