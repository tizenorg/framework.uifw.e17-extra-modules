#ifndef _E_MOD_EOM_DBUS_H_
#define _E_MOD_EOM_DBUS_H_

#include "e_mod_eom_privates.h"

#define EOM_DBUS_SERVER    "org.eom.server"
#define EOM_DBUS_INTERFACE "org.eom.interface"
#define EOM_DBUS_PATH      "/org/eom/path"

#define REPLY_TIME         4000
#define ARGV_NUM           64

typedef Eina_List* (*MethodFunc)(const char *client, Eina_List *list);

typedef struct _EomDbusMethod
{
   char                  *name;
   MethodFunc             func;

   struct _EomDbusMethod *next;
} EomDbusMethod;

int  eom_dbus_connect(void);
void eom_dbus_disconnect(void);
int  eom_dbus_add_method(EomDbusMethod *method);
void eom_dbus_remove_method(EomDbusMethod *method);

int eom_dbus_send_signal(const char *signal, Eina_List *list);
int eom_dbus_check_destination(const char *dest);
void eom_dbus_free_list (Eina_List *list);

#endif /* _E_MOD_EOM_DBUS_H_ */