#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_ATOMS_H
#define E_MOD_MOVE_ATOMS_H

extern EINTERN Ecore_X_Atom ATOM_CM_LOG;
extern EINTERN Ecore_X_Atom ATOM_MV_LOG;
extern EINTERN Ecore_X_Atom ATOM_MV_LOG_DUMP_DONE;
extern EINTERN Ecore_X_Atom ATOM_EFFECT_ENABLE;
extern EINTERN Ecore_X_Atom ATOM_CM_WINDOW_INPUT_REGION;
extern EINTERN Ecore_X_Atom ATOM_CM_WINDOW_CONTENTS_REGION;
extern EINTERN Ecore_X_Atom ATOM_WM_WINDOW_SHOW;
extern EINTERN Ecore_X_Atom ATOM_ROTATION_LOCK;

EINTERN int         e_mod_move_atoms_init(void);
EINTERN int         e_mod_move_atoms_shutdown(void);
EINTERN const char *e_mod_move_atoms_name_get(Ecore_X_Atom a);

#endif
#endif
