#include "e.h"
#include "e_mod_move_atoms.h"
#include "e_mod_move_debug.h"

/* atoms */
EINTERN Ecore_X_Atom ATOM_CM_LOG                           = 0;
EINTERN Ecore_X_Atom ATOM_MV_LOG                           = 0;
EINTERN Ecore_X_Atom ATOM_MV_LOG_DUMP_DONE                 = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_ENABLE                    = 0;
EINTERN Ecore_X_Atom ATOM_CM_WINDOW_INPUT_REGION           = 0;
EINTERN Ecore_X_Atom ATOM_CM_WINDOW_CONTENTS_REGION        = 0;
EINTERN Ecore_X_Atom ATOM_WM_WINDOW_SHOW                   = 0;
EINTERN Ecore_X_Atom ATOM_ROTATION_LOCK                    = 0;
EINTERN Ecore_X_Atom ATOM_MV_FULLSCREEN_INDICATOR_SHOW     = 0;
EINTERN Ecore_X_Atom ATOM_MV_APPTRAY_STATE                 = 0;
EINTERN Ecore_X_Atom ATOM_MV_MINI_APPTRAY_STATE            = 0;
EINTERN Ecore_X_Atom ATOM_MV_QUICKPANEL_STATE              = 0;
EINTERN Ecore_X_Atom ATOM_CLIENT_PANEL                     = 0;
EINTERN Ecore_X_Atom ATOM_MV_PANEL_SCROLLABLE_STATE        = 0;
EINTERN Ecore_X_Atom ATOM_MV_INDICATOR_GEOMETRY            = 0;
EINTERN Ecore_X_Atom ATOM_INDICATOR_CMD_WIN                = 0;
EINTERN Ecore_X_Atom ATOM_ACTIVE_INDICATOR_WIN             = 0;
EINTERN Ecore_X_Atom ATOM_INDICATOR_FLICK_DONE             = 0;

/* local subsystem globals */
static Eina_Hash *atoms_hash = NULL;

static const char *atom_names[] = {
  "_E_COMP_LOG",
  "_E_MOVE_LOG",
  "_E_MOVE_LOG_DUMP_DONE",
  "_NET_CM_EFFECT_ENABLE",
  "_E_COMP_WINDOW_INPUT_REGION",
  "_E_COMP_WINDOW_CONTENTS_REGION",
  "_NET_WM_WINDOW_SHOW",
  "_E_ROTATION_LOCK",
  "_E_MOVE_FULLSCREEN_INDICATOR_SHOW",
  "_E_MOVE_APPTRAY_STATE",
  "_E_MOVE_MINI_APPTRAY_STATE",
  "_E_MOVE_QUICKPANEL_STATE",
  "_NET_CLIENT_PANEL",
  "_E_MOVE_PANEL_SCROLLABLE_STATE",
  "_E_MOVE_INDICATOR_GEOMETRY",
  "_E_INDICATOR_CMD_WIN",
  "_E_ACTIVE_INDICATOR_WIN",
  "_E_INDICATOR_FLICK_DONE"
};

static const char *external_atom_names[] = {
  "WIN_ROT_ANGLE",
  "WM_CLASS",
  "_NET_ACTIVE_WINDOW",
  "_E_ILLUME_INDICATOR_STATE",
  "_E_ILLUME_QUICKPANEL_STATE",
  "ROOT_ROT_ANGLE",
  "_E_ILLUME_INDICATOR_TYPE_MODE"
};

/* externally accessible functions */
EINTERN int
e_mod_move_atoms_init(void)
{
   Ecore_X_Atom *atoms = NULL;
   int n = 0, i = 0, res = 0;

   if (!atoms_hash) atoms_hash = eina_hash_string_superfast_new(NULL);
   E_CHECK_RETURN(atoms_hash, 0);

   n = (sizeof(atom_names) / sizeof(char *));

   atoms = E_NEW(Ecore_X_Atom, n);
   E_CHECK_GOTO(atoms, cleanup);

   ecore_x_atoms_get(atom_names, n, atoms);

   ATOM_CM_LOG                           = atoms[i++];
   ATOM_MV_LOG                           = atoms[i++];
   ATOM_MV_LOG_DUMP_DONE                 = atoms[i++];
   ATOM_EFFECT_ENABLE                    = atoms[i++];
   ATOM_CM_WINDOW_INPUT_REGION           = atoms[i++];
   ATOM_CM_WINDOW_CONTENTS_REGION        = atoms[i++];
   ATOM_WM_WINDOW_SHOW                   = atoms[i++];
   ATOM_ROTATION_LOCK                    = atoms[i++];
   ATOM_MV_FULLSCREEN_INDICATOR_SHOW     = atoms[i++];
   ATOM_MV_APPTRAY_STATE                 = atoms[i++];
   ATOM_MV_MINI_APPTRAY_STATE            = atoms[i++];
   ATOM_MV_QUICKPANEL_STATE              = atoms[i++];
   ATOM_CLIENT_PANEL                     = atoms[i++];
   ATOM_MV_PANEL_SCROLLABLE_STATE        = atoms[i++];
   ATOM_MV_INDICATOR_GEOMETRY            = atoms[i++];
   ATOM_INDICATOR_CMD_WIN                = atoms[i++];
   ATOM_ACTIVE_INDICATOR_WIN             = atoms[i++];
   ATOM_INDICATOR_FLICK_DONE             = atoms[i++];

   for (i = 0; i < n; i++)
     {
        E_CHECK_GOTO(atoms[i], cleanup);
        eina_hash_add(atoms_hash,
                      e_util_winid_str_get(atoms[i]),
                      atom_names[i]);
     }

   i = 0;
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE), external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_WM_CLASS),                     external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_NET_ACTIVE_WINDOW),            external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_INDICATOR_STATE),     external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE),    external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_ROTATE_ROOT_ANGLE),   external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_INDICATOR_TYPE_MODE), external_atom_names[i++]);

   res = 1;

cleanup:
   if (atoms) E_FREE(atoms);
   return res;
}

EINTERN int
e_mod_move_atoms_shutdown(void)
{
   if (atoms_hash) eina_hash_free(atoms_hash);
   atoms_hash = NULL;
   return 1;
}

EINTERN const char *
e_mod_move_atoms_name_get(Ecore_X_Atom a)
{
   E_CHECK_RETURN(a, NULL);
   return eina_hash_find(atoms_hash,
                         e_util_winid_str_get(a));
}

