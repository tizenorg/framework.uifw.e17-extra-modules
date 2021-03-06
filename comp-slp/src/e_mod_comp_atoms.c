#include "e.h"
#include "e_mod_comp_atoms.h"
#include "e_mod_comp_debug.h"

/* atoms */
EINTERN Ecore_X_Atom ATOM_CM_LOG                           = 0;
EINTERN Ecore_X_Atom ATOM_CM_LOG_DUMP_DONE                 = 0;
EINTERN Ecore_X_Atom ATOM_IMAGE_LAUNCH                     = 0;
EINTERN Ecore_X_Atom ATOM_IMAGE_LAUNCH_FILE                = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_ENABLE                    = 0;
EINTERN Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE             = 0;
EINTERN Ecore_X_Atom ATOM_WINDOW_EFFECT_CLIENT_STATE       = 0;
EINTERN Ecore_X_Atom ATOM_WINDOW_EFFECT_TYPE               = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_DEFAULT                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_NONE                      = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM0                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM1                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM2                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM3                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM4                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM5                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM6                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM7                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM8                   = 0;
EINTERN Ecore_X_Atom ATOM_EFFECT_CUSTOM9                   = 0;
EINTERN Ecore_X_Atom ATOM_OVERAY_WINDOW                    = 0;
EINTERN Ecore_X_Atom ATOM_X_HIBERNATION_STATE              = 0;
EINTERN Ecore_X_Atom ATOM_X_SCREEN_ROTATION                = 0;
EINTERN Ecore_X_Atom ATOM_X_WIN_USE_DRI2                   = 0;
EINTERN Ecore_X_Atom ATOM_X_WIN_HW_OV_SHOW                 = 0;
EINTERN Ecore_X_Atom ATOM_CM_LOCK_SCREEN                   = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_SUPPORTED     = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_STATE         = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_BEGIN         = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE    = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_END           = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_END_DONE      = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_REQUEST       = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE  = 0;
EINTERN Ecore_X_Atom ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP = 0;
EINTERN Ecore_X_Atom ATOM_CAPTURE_EFFECT                   = 0;
EINTERN Ecore_X_Atom ATOM_NET_CM_WINDOW_BACKGROUND         = 0;

/* local subsystem globals */
static Eina_Hash *atoms_hash = NULL;

static const char *atom_names[] = {
  "_E_COMP_LOG",
  "_E_COMP_LOG_DUMP_DONE",
  "_E_COMP_FAKE_LAUNCH",
  "_E_COMP_FAKE_LAUNCH_IMAGE",
  "_NET_CM_EFFECT_ENABLE",
  "_NET_CM_WINDOW_EFFECT_ENABLE",
  "_NET_CM_WINDOW_EFFECT_CLIENT_STATE",
  "_NET_CM_WINDOW_EFFECT_TYPE",
  "_NET_CM_EFFECT_DEFAULT",
  "_NET_CM_EFFECT_NONE",
  "_NET_CM_EFFECT_CUSTOM0",
  "_NET_CM_EFFECT_CUSTOM1",
  "_NET_CM_EFFECT_CUSTOM2",
  "_NET_CM_EFFECT_CUSTOM3",
  "_NET_CM_EFFECT_CUSTOM4",
  "_NET_CM_EFFECT_CUSTOM5",
  "_NET_CM_EFFECT_CUSTOM6",
  "_NET_CM_EFFECT_CUSTOM7",
  "_NET_CM_EFFECT_CUSTOM8",
  "_NET_CM_EFFECT_CUSTOM9",
  "_E_COMP_OVERAY_WINDOW",
  "X_HIBERNATION_STATE",
  "X_SCREEN_ROTATION",
  "X_WIN_USE_DRI2",
  "X_WIN_HW_OV_SHOW",
  "_E_COMP_LOCK_SCREEN",
  "_E_COMP_PIXMAP_ROTATION_SUPPORTED",
  "_E_COMP_PIXMAP_ROTATION_STATE",
  "_E_COMP_PIXMAP_ROTATION_BEGIN",
  "_E_COMP_PIXMAP_ROTATION_BEGIN_DONE",
  "_E_COMP_PIXMAP_ROTATION_END",
  "_E_COMP_PIXMAP_ROTATION_END_DONE",
  "_E_COMP_PIXMAP_ROTATION_REQUEST",
  "_E_COMP_PIXMAP_ROTATION_REQUEST_DONE",
  "_E_COMP_PIXMAP_ROTATION_RESIZE_PIXMAP",
  "_E_COMP_CAPTURE_EFFECT",
  "_NET_CM_WINDOW_BACKGROUND"
};

static const char *external_atom_names[] = {
  "SYNC_DRAW_DONE",
  "SYNC_COUNTER",
  "WIN_ROT_ANGLE",
  "ILLUME_WIN_STATE",
  "WM_WINDOW_OPACITY",
  "WM_CLASS"
};

/* externally accessible functions */
EINTERN int
e_mod_comp_atoms_init(void)
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
   ATOM_CM_LOG_DUMP_DONE                 = atoms[i++];
   ATOM_IMAGE_LAUNCH                     = atoms[i++];
   ATOM_IMAGE_LAUNCH_FILE                = atoms[i++];
   ATOM_EFFECT_ENABLE                    = atoms[i++];
   ATOM_WINDOW_EFFECT_ENABLE             = atoms[i++];
   ATOM_WINDOW_EFFECT_CLIENT_STATE       = atoms[i++];
   ATOM_WINDOW_EFFECT_TYPE               = atoms[i++];
   ATOM_EFFECT_DEFAULT                   = atoms[i++];
   ATOM_EFFECT_NONE                      = atoms[i++];
   ATOM_EFFECT_CUSTOM0                   = atoms[i++];
   ATOM_EFFECT_CUSTOM1                   = atoms[i++];
   ATOM_EFFECT_CUSTOM2                   = atoms[i++];
   ATOM_EFFECT_CUSTOM3                   = atoms[i++];
   ATOM_EFFECT_CUSTOM4                   = atoms[i++];
   ATOM_EFFECT_CUSTOM5                   = atoms[i++];
   ATOM_EFFECT_CUSTOM6                   = atoms[i++];
   ATOM_EFFECT_CUSTOM7                   = atoms[i++];
   ATOM_EFFECT_CUSTOM8                   = atoms[i++];
   ATOM_EFFECT_CUSTOM9                   = atoms[i++];
   ATOM_OVERAY_WINDOW                    = atoms[i++];
   ATOM_X_HIBERNATION_STATE              = atoms[i++];
   ATOM_X_SCREEN_ROTATION                = atoms[i++];
   ATOM_X_WIN_USE_DRI2                   = atoms[i++];
   ATOM_X_WIN_HW_OV_SHOW                 = atoms[i++];
   ATOM_CM_LOCK_SCREEN                   = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_SUPPORTED     = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_STATE         = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_BEGIN         = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_BEGIN_DONE    = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_END           = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_END_DONE      = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_REQUEST       = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_REQUEST_DONE  = atoms[i++];
   ATOM_CM_PIXMAP_ROTATION_RESIZE_PIXMAP = atoms[i++];
   ATOM_CAPTURE_EFFECT                   = atoms[i++];
   ATOM_NET_CM_WINDOW_BACKGROUND         = atoms[i++];

   for (i = 0; i < n; i++)
     {
        E_CHECK_GOTO(atoms[i], cleanup);
        eina_hash_add(atoms_hash,
                      e_util_winid_str_get(atoms[i]),
                      atom_names[i]);
     }

   i = 0;
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE),        external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_COMP_SYNC_COUNTER),          external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_ROTATE_WINDOW_ANGLE), external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_E_ILLUME_WINDOW_STATE),        external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_NET_WM_WINDOW_OPACITY),        external_atom_names[i++]);
   eina_hash_add(atoms_hash, e_util_winid_str_get(ECORE_X_ATOM_WM_CLASS),                     external_atom_names[i++]);

   res = 1;

cleanup:
   if (atoms) E_FREE(atoms);
   return res;
}

EINTERN int
e_mod_comp_atoms_shutdown(void)
{
   if (atoms_hash) eina_hash_free(atoms_hash);
   atoms_hash = NULL;
   return 1;
}

EINTERN const char *
e_mod_comp_atoms_name_get(Ecore_X_Atom a)
{
   E_CHECK_RETURN(a, NULL);
   return eina_hash_find(atoms_hash,
                         e_util_winid_str_get(a));
}
