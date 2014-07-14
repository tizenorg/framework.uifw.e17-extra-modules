#include "e_mod_eom_privates.h"
#include "e_mod_eom_ownership.h"
#include "e_mod_eom_dbus.h"

typedef struct
{
   int   sc_output_id;
   char *sc_owner;
} EOMOwnerInfo;

static Eina_List *owner_list;

static Eina_List*
_eom_ownership_owner_find (int sc_output_id)
{
   Eina_List *l = NULL;
   void *data;

   if (!owner_list)
     return NULL;

   EINA_LIST_FOREACH(owner_list, l, data)
     {
        EOMOwnerInfo *owner_info = (EOMOwnerInfo*)data;

        if (!owner_info)
          continue;

        if (owner_info->sc_output_id == sc_output_id)
          return l;
     }
 
   return NULL;
}

static int
_eom_ownership_owner_check_alive (char *sc_owner)
{
   int ret;

   EOM_CHK_RETV (sc_owner != NULL, 0);

   ret = eom_dbus_check_destination (sc_owner);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] %s is %s\n", sc_owner, (ret)?"alive":"dead");

   return ret;
}

int
eom_ownership_can_set (int sc_output_id, char *sc_owner)
{
   Eina_List *l;
   EOMOwnerInfo *owner_info;

   if (!owner_list)
     return 1;

   l = _eom_ownership_owner_find (sc_output_id);
   if (!l || !l->data)
     return 1;

   owner_info = (EOMOwnerInfo*)l->data;

   if (!_eom_ownership_owner_check_alive (owner_info->sc_owner))
     {
        eom_ownership_release (owner_info->sc_output_id, owner_info->sc_owner);
        return 1;
     }

   if (!strncmp (owner_info->sc_owner, sc_owner, STR_LEN))
     return 1;
 
   return 0;
}

int
eom_ownership_take (int sc_output_id, char *sc_owner)
{
   Eina_List *l;
   EOMOwnerInfo *owner_info;

   l = _eom_ownership_owner_find (sc_output_id);
   if (l && l->data)
     {
        owner_info = (EOMOwnerInfo*)l->data;

        if (_eom_ownership_owner_check_alive (owner_info->sc_owner))
          return 0;
        else
          eom_ownership_release (owner_info->sc_output_id, owner_info->sc_owner);
     }

   owner_info = calloc (1, sizeof (EOMOwnerInfo));
   EOM_CHK_RETV (owner_info != NULL, 0);

   owner_info->sc_output_id = sc_output_id;
   owner_info->sc_owner = strndup (sc_owner, STR_LEN);

   owner_list = eina_list_append (owner_list, owner_info);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] %d's owner: %s\n", sc_output_id, sc_owner);

   return 1;
}

int
eom_ownership_release (int sc_output_id, char *sc_owner)
{
   Eina_List *l;
   EOMOwnerInfo *owner_info;

   l = _eom_ownership_owner_find (sc_output_id);
   if (!l || !l->data)
      return 1;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][svr] %d's old owner: %s\n", sc_output_id, sc_owner);

   owner_info = (EOMOwnerInfo*)l->data;

   owner_list = eina_list_remove (owner_list, owner_info);

   if (owner_info->sc_owner)
      free (owner_info->sc_owner);

   free (owner_info);

   return 1;
}

int
eom_ownership_has (int sc_output_id, char *sc_owner)
{
   Eina_List *l;
   EOMOwnerInfo *owner_info;

   if (!owner_list)
     return 0;

   l = _eom_ownership_owner_find (sc_output_id);
   if (!l || !l->data)
     return 0;

   owner_info = (EOMOwnerInfo*)l->data;
   if (strncmp (owner_info->sc_owner, sc_owner, STR_LEN))
     return 0;

   return 1;
}

void
eom_ownership_deinit (void)
{
   Eina_List *l = NULL;
   void *data;

   if (!owner_list)
      return;
 
   EINA_LIST_FOREACH(owner_list, l, data)
     {
        EOMOwnerInfo *owner_info = (EOMOwnerInfo*)data;
 
        if (!owner_info)
          continue;
 
        if (owner_info->sc_owner)
           free (owner_info->sc_owner);
        free (owner_info);
     }
 
   eina_list_free (owner_list);
   owner_list = NULL;
}
