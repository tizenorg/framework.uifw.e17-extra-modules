#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include "e_mod_eom_dbus.h"

typedef struct _EomDBusServerInfo
{
   DBusConnection *conn;
   char            name[STR_LEN];
   char            rule[STR_LEN];
   EomDbusMethod  *methods;
   int             fd;
}EomDBusServerInfo;

static EomDBusServerInfo server_info;

static int
_eom_dbus_convert_list_to_message (Eina_List *list, DBusMessage *msg)
{
   DBusMessageIter iter;
   Eina_List *l;
   const Eina_Value *v;

   if (!list)
      return 1;

   dbus_message_iter_init_append (msg, &iter);

   EINA_LIST_FOREACH(list, l, v)
     {
        const Eina_Value_Type *type;
     
        if (!v) continue;
     
        type = eina_value_type_get (v);

        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] type: %p\n", type);

        if (type == EINA_VALUE_TYPE_INT)
          {
             int integer;
             eina_value_get (v, &integer);
             if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &integer))
               {
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: int append");
                  return 0;
               }
             SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] integer: %d\n", integer);
          }
        else if (type == EINA_VALUE_TYPE_UINT)
          {
             unsigned int uinteger;
             eina_value_get (v, &uinteger);
             if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_UINT32, &uinteger))
               {
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: uint append");
                  return 0;
               }
             SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] uinteger: %d\n", uinteger);
          }
        else if (type == EINA_VALUE_TYPE_STRING)
          {
             char *string;
             eina_value_get (v, &string);
             if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, (void*)&string))
               {
                  SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: string append");
                  return 0;
               }
             SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] string: %s\n", string);
          }
        else
          {
             SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] never get here");
             return 0;
          }
     }

   return 1;
}

static Eina_List*
_eom_dbus_convert_message_to_list (DBusMessage *msg)
{
   DBusMessageIter iter;
   Eina_List *list = NULL;
   Eina_Value *v;

   if (!dbus_message_iter_init (msg, &iter))
      return NULL;

   do
     {
        int type = dbus_message_iter_get_arg_type (&iter);

        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] type: %d\n", type);

        switch (type)
          {
          case DBUS_TYPE_INT32:
            {
               int integer = 0;
               dbus_message_iter_get_basic (&iter, &integer);
               v = eina_value_new (EINA_VALUE_TYPE_INT);
               eina_value_set (v, integer);
               list = eina_list_append (list, v);
               SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] integer: %d\n", integer);
               break;
            }
          case DBUS_TYPE_UINT32:
            {
               unsigned int uinteger = 0;
               dbus_message_iter_get_basic (&iter, &uinteger);
               v = eina_value_new (EINA_VALUE_TYPE_UINT);
               eina_value_set (v, uinteger);
               list = eina_list_append (list, v);
               SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] uinteger: %d\n", uinteger);
               break;
            }
          case DBUS_TYPE_STRING:
            {
               char *string = NULL;
               dbus_message_iter_get_basic (&iter, &string);
               v = eina_value_new (EINA_VALUE_TYPE_STRING);
               eina_value_set (v, string);
               list = eina_list_append (list, v);
               SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] string: %s\n", string);
               break;
            }
          case DBUS_TYPE_ARRAY:
            {
               DBusMessageIter sub;
               void *data = NULL;
               int data_size = 0;
               Eina_Value_Blob blob = {0,};

               dbus_message_iter_recurse (&iter, &sub);
               dbus_message_iter_get_fixed_array (&sub, &data, &data_size);

               blob.memory = data;
               blob.size = data_size;
            
               v = eina_value_new (EINA_VALUE_TYPE_BLOB);
               eina_value_set (v, blob);
               list = eina_list_append (list, v);
               SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] %p,%d\n", blob.memory, blob.size);
               break;
            }
          default:
            {
               SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] never get here");
               eina_list_free (list);
               return NULL;
            }
          }
     }
   while (dbus_message_iter_has_next (&iter) && dbus_message_iter_next (&iter));

   return list;
}

static int
_eom_dbus_send_reply(DBusMessage *msg, DBusConnection *conn, Eina_List *list)
{
   DBusMessage *reply;

   EOM_CHK_RETV(conn != NULL, 0);

   reply = dbus_message_new_method_return(msg);
   EOM_CHK_RETV(reply != NULL, 0);
 
   if (!_eom_dbus_convert_list_to_message (list, reply))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] fail: list_to_message");
        dbus_message_unref(reply);
        return 0;
     }

   if (!dbus_connection_send(conn, reply, NULL))
     {
        dbus_message_unref(reply);
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] Out Of Memory!\n");
        return 0;
     }

   dbus_connection_flush(conn);
   dbus_message_unref(reply);
   return 1;
}

static void
_eom_dbus_process_message(EomDBusServerInfo *info, DBusMessage *msg)
{
   EomDbusMethod **prev;
   DBusError err;

   dbus_error_init(&err);

   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] Process a message (%s.%s)\n",
        dbus_message_get_interface(msg), dbus_message_get_member(msg));

   EOM_CHK_RET(info->conn != NULL);

   for (prev = &info->methods; *prev; prev = &(*prev)->next)
     {
        EomDbusMethod *method = *prev;
        Eina_List *ret_list = NULL;

        if (strcmp(dbus_message_get_member(msg), method->name))
           continue;

        if (method->func)
          {
             Eina_List *list = _eom_dbus_convert_message_to_list (msg);
             const char *client = dbus_message_get_sender (msg);
             client = (client != NULL) ? client : "Unknow";
             ret_list = method->func (client, list);
             if (list)
                eom_dbus_free_list (list);
          }
 
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] method func go\n");
        _eom_dbus_send_reply(msg, info->conn, ret_list);
 
        if (ret_list)
           eom_dbus_free_list (ret_list);
 
        dbus_error_free(&err);
 
        return;
     }
}

Eina_Bool
_eom_dbus_cb(void *data, Ecore_Fd_Handler *handler)
{
   EomDBusServerInfo *info = (EomDBusServerInfo *)data;

   EOM_CHK_RETV(info && info->conn && info->fd > 0, EINA_TRUE);

   do
     {
        dbus_connection_read_write_dispatch(info->conn, 0);
     } while (info->conn &&
              dbus_connection_get_is_connected(info->conn) &&
              dbus_connection_get_dispatch_status(info->conn) ==
              DBUS_DISPATCH_DATA_REMAINS);

   return EINA_TRUE;
}

static void
_eom_dbus_deinit(EomDBusServerInfo *info)
{
   if (info->conn)
     {
        DBusError err;
        dbus_error_init(&err);
        dbus_bus_remove_match(info->conn, info->rule, &err);
        dbus_error_free(&err);
        dbus_bus_release_name(info->conn, EOM_DBUS_SERVER, &err);
        dbus_error_free(&err);
        dbus_connection_unref(info->conn);
        info->conn = NULL;
     }
   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] disconnected\n");
}

static DBusHandlerResult
_eom_dbus_msg_handler(DBusConnection *connection, DBusMessage *msg, void *data)
{
   EomDBusServerInfo *info = (EomDBusServerInfo *)data;

   if (!info || !info->conn || !msg)
     return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] interface: %s\n", dbus_message_get_interface(msg));
   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] member: %s\n", dbus_message_get_member(msg));
   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] sender: %s\n", dbus_message_get_sender(msg));
   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] destination: %s\n", dbus_message_get_destination(msg));
   SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] path: %s\n", dbus_message_get_path(msg));

   _eom_dbus_process_message(info, msg);

   return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
_eom_dbus_msg_filter(DBusConnection *conn, DBusMessage *msg, void *data)
{
   EomDBusServerInfo *info = (EomDBusServerInfo *)data;

   if (!info)
     return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

   if (dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] disconnected by signal\n");
        _eom_dbus_deinit(info);
        return DBUS_HANDLER_RESULT_HANDLED;
     }
   return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

int
_eom_dbus_init(EomDBusServerInfo *info)
{
   DBusError err;
   int ret;
   DBusObjectPathVTable vtable = {.message_function = _eom_dbus_msg_handler, };

   dbus_error_init(&err);
   ecore_init();

   info->conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
   if (dbus_error_is_set(&err))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: connection (%s)\n", err.message);
        goto free_err;
     }
   if (!info->conn)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: connection NULL\n");
        goto free_err;
     }

   ret = dbus_bus_request_name(info->conn, EOM_DBUS_SERVER,
                               DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
   if (dbus_error_is_set(&err))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: request name (%s)\n", err.message);
        goto free_conn;
     }
   if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: Not Primary Owner (%d)\n", ret);
        goto free_conn;
     }

   snprintf(info->rule, sizeof (info->rule), "type='method_call',interface='%s'",
            EOM_DBUS_INTERFACE);

   dbus_bus_add_match(info->conn, info->rule, &err);

   if (dbus_error_is_set(&err))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: add match (%s)\n", err.message);
        goto free_name;
     }

   if (!dbus_connection_register_object_path(info->conn,
                                             EOM_DBUS_PATH, &vtable,
                                             info))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: register object path\n");
        goto free_match;
     }

   dbus_connection_set_exit_on_disconnect(info->conn, FALSE);

   if (!dbus_connection_add_filter(info->conn, _eom_dbus_msg_filter, info, NULL))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: add filter (%s)\n", err.message);
        goto free_register;
     }

   if (!dbus_connection_get_unix_fd(info->conn, &info->fd) || info->fd < 0)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: get fd\n");
        goto free_filter;
     }

   if (!ecore_main_fd_handler_add(info->fd, ECORE_FD_READ, (Ecore_Fd_Cb)_eom_dbus_cb, info, NULL, NULL))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] error add fd handler\n");
        goto free_filter;
     }

   dbus_error_free(&err);

   return 1;

free_filter:
   dbus_connection_remove_filter(info->conn, _eom_dbus_msg_filter, info);
free_register:
   dbus_connection_unregister_object_path(info->conn, EOM_DBUS_PATH);
free_match:
   dbus_bus_remove_match(info->conn, info->rule, &err);
   dbus_error_free(&err);
free_name:
   dbus_bus_release_name(info->conn, EOM_DBUS_SERVER, &err);
   dbus_error_free(&err);
free_conn:
   dbus_connection_close(info->conn);
free_err:
   dbus_error_free(&err);
   info->conn = NULL;
   info->fd = -1;
   return 0;
}

int
eom_dbus_connect(void)
{
   snprintf(server_info.name, sizeof(server_info.name), "%d", getpid());
   server_info.fd = -1;

   if (!_eom_dbus_init(&server_info))
     return 0;

   return 1;
}

void
eom_dbus_disconnect(void)
{
   _eom_dbus_deinit(&server_info);
}

int
eom_dbus_add_method(EomDbusMethod *method)
{
   EomDbusMethod **prev;

   for (prev = &server_info.methods; *prev; prev = &(*prev)->next) ;

   method->next = NULL;
   *prev = method;

   return 1;
}

void
eom_dbus_remove_method(EomDbusMethod *method)
{
   EomDbusMethod **prev;

   for (prev = &server_info.methods; *prev; prev = &(*prev)->next)
     if (*prev == method)
       {
          *prev = method->next;
          break;
       }
}

int
eom_dbus_send_signal(const char *signal, Eina_List *list)
{
   DBusMessage *msg = NULL;

   EOM_CHK_RETV(server_info.conn, 0);

   msg = dbus_message_new_signal(EOM_DBUS_PATH,
                                 EOM_DBUS_INTERFACE,
                                 signal);
   if (NULL == msg)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] Message Null\n");
        return 0;
     }

   if (!_eom_dbus_convert_list_to_message (list, msg))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] fail: list_to_message");
        goto fail;
     }

   if (!dbus_connection_send(server_info.conn, msg, NULL))
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] Out Of Memory!\n");

        goto fail;
     }

   dbus_connection_flush(server_info.conn);

   dbus_message_unref(msg);

   return 1;

fail:
   if (msg)
      dbus_message_unref(msg);

   return 0;
}

int
eom_dbus_check_destination(const char *dest)
{
   DBusError err;
   unsigned long ret;

   EOM_CHK_RETV(server_info.conn != NULL, 0);
   EOM_CHK_RETV(dest != NULL, 0);

   dbus_error_init(&err);

   ret = dbus_bus_get_unix_user (server_info.conn, dest, &err);

   if (ret == (unsigned long)-1)
     {
        SLOG(LOG_DEBUG, "EOM", "[e_eom][bus] failed: get_unix_user(%s)\n", err.message);
        return 0;
     }

   return 1;
}

void
eom_dbus_free_list (Eina_List *list)
{
   Eina_List *l;
   Eina_Value *v;

   if (!list)
      return;

   EINA_LIST_FOREACH(list, l, v)
     {
        eina_value_free (v);
     }

   eina_list_free (list);
}
