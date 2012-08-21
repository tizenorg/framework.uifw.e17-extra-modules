
#ifndef E_MOD_SCRNCONF_H
#define E_MOD_SCRNCONF_H

void e_mod_scrnconf_external_init ();
void e_mod_scrnconf_external_deinit ();

void e_mod_scrnconf_external_dialog_new (int sc_output);
void e_mod_scrnconf_external_dialog_free (void);

int e_mod_scrnconf_external_get_output_from_xid (Ecore_X_Randr_Output output_xid);
int e_mod_scrnconf_external_get_default_res     (int sc_output, int preferred_w, int preferred_h);

int e_mod_scrnconf_external_get_output (void);
int e_mod_scrnconf_external_set_output (int sc_output);

int e_mod_scrnconf_external_get_res (void);
int e_mod_scrnconf_external_set_res (int sc_res);

int e_mod_scrnconf_external_get_status (void);
int e_mod_scrnconf_external_set_status (int sc_stat);

int e_mod_scrnconf_external_set_dispmode (int sc_output, int sc_dispmode, int sc_res);
int e_mod_scrnconf_external_get_dispmode (void);

int e_mod_scrnconf_external_send_current_status (void);

int e_mod_scrnconf_external_reset (int sc_output);

#endif // E_MOD_SCRNCONF_H

