#ifndef E_MOD_QUICKPANEL_H
# define E_MOD_QUICKPANEL_H

# define E_ILLUME_QP_TYPE 0xE1b0990

#define ILLUME_PREFIX     "/usr/lib/enlightenment/modules/illume2-slp/"
#define EDJ_FILE          ILLUME_PREFIX"/quickpanel.edj"

int e_mod_quickpanel_init(void);
int e_mod_quickpanel_shutdown(void);

E_Illume_Quickpanel *e_mod_quickpanel_new(E_Zone *zone);
void e_mod_quickpanel_show(E_Illume_Quickpanel *qp, int isAni);
void e_mod_quickpanel_hide(E_Illume_Quickpanel *qp, int isAni);

#endif
