#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <Ecore.h>
#include <Ecore_Getopt.h>
#include "E_DBus.h"
#include "e.h"

static void
_cb_lw_request_name(void        *data,
                    DBusMessage *msg,
                    DBusError   *err)
{
   /* TODO: handle error */
   return;
}

static DBusMessage *
_cb_lw_log_show(E_DBus_Object *obj,
                DBusMessage   *msg)
{
   DBusMessageIter from;
   char *log;

   dbus_message_iter_init(msg, &from);
   dbus_message_iter_get_basic(&from, &log);

   fprintf(stdout, "%s\n", log);
   fflush(stdout);

   return dbus_message_new_method_return(msg);
}

static int
_lw_init(E_DBus_Connection *conn)
{
   E_DBus_Object *obj;
   E_DBus_Interface *iface;

   e_dbus_request_name(conn, "org.enlightenment.elogwatcher.service",
                       0, _cb_lw_request_name, NULL);

   obj = e_dbus_object_add(conn, "/org/enlightenment/elogwatcher/RemoteObject", NULL);
   iface = e_dbus_interface_new("org.enlightenment.elogwatcher.Log");
   e_dbus_interface_method_add(iface, "Show", "s", "", _cb_lw_log_show);
   e_dbus_object_interface_attach(obj, iface);

   return 1;
}

static int
_lw_dump_mode_set(E_DBus_Connection *conn,
                  unsigned int       type)
{
   DBusMessage *msg;
   const char *str = "watcher";

   msg = dbus_message_new_method_call("org.enlightenment.wm.service",
                                      "/org/enlightenment/wm/RemoteObject",
                                      "org.enlightenment.wm.Log",
                                      "Dump");
   dbus_message_append_args(msg,
                            DBUS_TYPE_STRING, &str,
                            DBUS_TYPE_INVALID);
   e_dbus_message_send(conn, msg, NULL, 0, NULL);
   dbus_message_unref(msg);

   msg = dbus_message_new_method_call("org.enlightenment.wm.service",
                                      "/org/enlightenment/wm/RemoteObject",
                                      "org.enlightenment.wm.Log",
                                      "SetType");
   dbus_message_append_args(msg,
                            DBUS_TYPE_INT32, &type,
                            DBUS_TYPE_INVALID);
   e_dbus_message_send(conn, msg, NULL, 0, NULL);
   dbus_message_unref(msg);

   return 1;
}

static unsigned int
_lw_log_type_get(const char *str)
{
   unsigned int val = 0, t_val = 0;
   char *buf = strdup(str);
   char tok[64];
   char p = 0;
   int i = 0, j = 0;
   char op = 0;

   do
     {
        p = buf[i++];

        switch (p)
          {
           case '+':
           case '-':
           case '\0':
              tok[j] = '\0';
              j = 0;

              if      (!strcmp(tok, "DFT"    )) t_val = ELBT_DFT;
              else if (!strcmp(tok, "MNG"    )) t_val = ELBT_MNG;
              else if (!strcmp(tok, "BD"     )) t_val = ELBT_BD;
              else if (!strcmp(tok, "ROT"    )) t_val = ELBT_ROT;
              else if (!strcmp(tok, "ILLUME" )) t_val = ELBT_ILLUME;
              else if (!strcmp(tok, "COMP"   )) t_val = ELBT_COMP;
              else if (!strcmp(tok, "MOVE"   )) t_val = ELBT_MOVE;
              else if (!strcmp(tok, "TRACE"  )) t_val = ELBT_TRACE;
              else if (!strcmp(tok, "DESK_LY")) t_val = ELBT_DESK_LY;
              else if (!strcmp(tok, "RENDER" )) t_val = ELBT_COMP_RENDER;
              else if (!strcmp(tok, "ALL"    )) t_val = ELBT_ALL;
              else
                {
                   fprintf(stdout, "[ERR] Invalid type:%s\n", str);
                   goto finish;
                }

              if (op)
                {
                   switch (op)
                     {
                      case '+': val |=  t_val; break;
                      case '-': val &= ~t_val; break;
                     }
                }
              else
                val = t_val;
              op = p;
              break;

           default:
              tok[j++] = p;
              break;
          }
     }
   while ((p) != ('\0'));

finish:
   free(buf);
   return val;
}

static const Ecore_Getopt optdesc =
{
   "elogwatcher",
   "%prog [options]",
   "0.1.0",
   "(C) 2013 Enlightenment",
   "BSD 2-Clause",
   "Log watcher of the Enlightenment",
   EINA_FALSE,
   {
      ECORE_GETOPT_STORE_STR('t', "type",
                             "Set log type: "
                             "NONE ALL DFT MNG BD ROT ILLUME COMP MOVE "
                             "TRACE DESK_LY RENDER "
                             "(eg BD+ROT+DFT or ALL-RENDER etc.)"),
      ECORE_GETOPT_VERSION('v', "version"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

int
main(int argc, char **argv)
{
   E_DBus_Connection *conn = NULL;
   int ret = 1, res;
   Eina_Bool quit = EINA_FALSE;
   unsigned int type = ELBT_NONE;
   char *opt_types = NULL;
   Ecore_Getopt_Value values[] =
     {
        ECORE_GETOPT_VALUE_STR(opt_types),
        ECORE_GETOPT_VALUE_BOOL(quit),
        ECORE_GETOPT_VALUE_NONE
     };

   ecore_init();
   e_dbus_init();

   if (ecore_getopt_parse(&optdesc, values, argc, argv) < 0)
     {
        goto finish;
     }

   if (quit) goto finish;

   if (!opt_types)
     type = (ELBT_ALL) & (~ELBT_COMP_RENDER);
   else
     type = _lw_log_type_get(opt_types);

   conn = e_dbus_bus_get(DBUS_BUS_SESSION);
   if (conn)
     {
        res = _lw_init(conn);
        if (!res) goto finish;

        res = _lw_dump_mode_set(conn, type);
        if (!res) goto finish;

        ecore_main_loop_begin();
     }

   ret = 0;

finish:
   if (conn) e_dbus_connection_close(conn);
   e_dbus_shutdown();
   ecore_shutdown();

   return ret;
}
