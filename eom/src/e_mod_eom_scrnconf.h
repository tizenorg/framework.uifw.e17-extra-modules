#ifndef _E_MOD_EOM_SCRNCONF_H_
#define _E_MOD_EOM_SCRNCONF_H_

#include "e_mod_eom_privates.h"

void eom_scrnconf_init();
void eom_scrnconf_deinit();

void eom_scrnconf_dialog_new(int sc_output_id);
void eom_scrnconf_dialog_free(void);

int  eom_scrnconf_get_output_id_from_xid(Ecore_X_Randr_Output output_xid);
int  eom_scrnconf_get_default_res(int sc_output_id, int preferred_w, int preferred_h);

int  eom_scrnconf_get_output_id(void);
int  eom_scrnconf_set_output_id(int sc_output_id);

int  eom_scrnconf_get_res(void);
char* eom_scrnconf_get_res_str(void);
int  eom_scrnconf_set_res(int sc_res);

int  eom_scrnconf_get_status(void);
int  eom_scrnconf_set_status(int sc_stat);

int  eom_scrnconf_get_output_type(int sc_output_id);

int  eom_scrnconf_set_dispmode(int sc_output_id, int sc_dispmode, int sc_res);
int  eom_scrnconf_get_dispmode(void);

int  eom_scrnconf_reset(int sc_output_id);

void eom_scrnconf_container_bg_canvas_visible_set(Eina_Bool visible);

#endif /* _E_MOD_EOM_SCRNCONF_H_ */
