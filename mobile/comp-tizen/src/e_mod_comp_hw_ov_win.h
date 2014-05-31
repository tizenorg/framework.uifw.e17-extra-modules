#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_HW_OV_WIN_H
#define E_MOD_COMP_HW_OV_WIN_H

typedef enum   _E_Comp_Log_Type  E_Comp_Log_Type;
typedef struct _E_Comp_HW_Ov_Win E_Comp_HW_Ov_Win;

enum _E_Comp_Log_Type
{
   E_COMP_LOG_TYPE_DEFAULT = 0,
   E_COMP_LOG_TYPE_NOCOMP,
   E_COMP_LOG_TYPE_SWAP,
   E_COMP_LOG_TYPE_EFFECT,
   E_COMP_LOG_TYPE_MAX
};

/* HW overlay window setup and handler functions */
EINTERN E_Comp_HW_Ov_Win *e_mod_comp_hw_ov_win_new(Ecore_X_Window parent,
                                                   int            x,
                                                   int            y,
                                                   int            w,
                                                   int            h);
EINTERN void              e_mod_comp_hw_ov_win_free(E_Comp_HW_Ov_Win *ov);
EINTERN Eina_Bool         e_mod_comp_hw_ov_win_update(E_Comp_HW_Ov_Win *ov,
                                                      E_Comp_Win       *cw);
EINTERN void              e_mod_comp_hw_ov_win_show(E_Comp_HW_Ov_Win *ov,
                                                    E_Comp_Win       *cw);
EINTERN void              e_mod_comp_hw_ov_win_hide(E_Comp_HW_Ov_Win *ov,
                                                    E_Comp_Win       *cw);
EINTERN void              e_mod_comp_hw_ov_win_msg_config_update(void);
EINTERN void              e_mod_comp_hw_ov_win_msg_show(E_Comp_Log_Type  type,
                                                        const char      *f,
                                                        ...);
EINTERN void              e_mod_comp_hw_ov_win_root_set(E_Comp_HW_Ov_Win *ov,
                                                        Ecore_X_Window    root);
EINTERN void              e_mod_comp_hw_ov_win_obj_show(E_Comp_HW_Ov_Win *ov,
                                                        E_Comp_Win       *cw);
EINTERN void              e_mod_comp_hw_ov_win_obj_hide(E_Comp_HW_Ov_Win *ov,
                                                        E_Comp_Win       *cw);
#endif
#endif
