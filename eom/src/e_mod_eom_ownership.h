#ifndef _E_MOD_EOM_OWNERSHIP_H_
#define _E_MOD_EOM_OWNERSHIP_H_

int eom_ownership_take (int sc_output_id, char *sc_owner);
int eom_ownership_release (int sc_output_id, char *sc_owner);
int eom_ownership_has (int sc_output_id, char *sc_owner);
int eom_ownership_can_set (int sc_output_id, char *sc_owner);

void eom_ownership_deinit (void);

#endif /* _E_MOD_EOM_OWNERSHIP_H_ */
