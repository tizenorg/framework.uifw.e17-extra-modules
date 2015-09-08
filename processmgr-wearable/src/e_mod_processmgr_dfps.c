#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xatom.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <Eina.h>
#include "e_mod_processmgr_dfps.h"
#include "e_mod_processmgr.h"

#define LOG_TAG "DFPS"
#include "dlog.h"

#define TRM_DEFAULT_FPS 30 /* default fps */
#define MAX_LCD_REFRESH_RATE 60.0 /* max fps */
#define TRM_DFPS_OBJ_PATH    "/Org/Tizen/Trm/Siop"
#define TRM_DFPS_INTERFACE  "org.tizen.trm.siop"
#define TRM_DFPS_NAME           "SetDynamicFps"

static Eina_List * g_dfps_list = NULL;
const char DPFS_config_file[] = {DYNAMICFPS_CFG_FILE};
static dfpsConfig dfpsInfo;
static int current_fps = TRM_DEFAULT_FPS;
typedef struct _dfps_app_info
{
   char* package_name;
   char* app_name;
} dfps_app_info;

static void _dfps_set_dfps_rate(int pid, int dfps_rate);

static void _tree_walk_dfpslist(xmlNodePtr cur)
{
   xmlNode *cur_node = NULL;
   int index = 0;
   for (cur_node = cur; cur_node; cur_node = cur_node->next)
     {
        if (cur_node->type == XML_ELEMENT_NODE)
          {
             dfps_entry * pitem = (dfps_entry *)calloc(1, sizeof(dfps_entry));
             if (pitem == NULL) continue;

             pitem->pid = 0;

             // if we use xmlGetProp(), we should keep the pointer and call xmlFree()
             xmlChar *xml_dfps = NULL;
             xmlChar *xml_package = NULL;
             xmlChar *xml_name = NULL;

             xml_dfps = xmlGetProp(cur_node, (const xmlChar *)"dfps");
             if (xml_dfps != NULL)
                pitem->dfps = atoi((char*)xml_dfps);

             xml_package = xmlGetProp(cur_node, (const xmlChar *)"package");
             if (xml_package != NULL)
                pitem->package  = (char *)strdup((char*)xml_package);

             xml_name = xmlGetProp(cur_node, (const xmlChar *)"name");
             if (xml_name != NULL)
                pitem->name  = (char *)strdup((char*)xml_name);

             pitem->index = index;
             index++;

             if (xml_dfps != NULL)
                xmlFree(xml_dfps);

             if (xml_package != NULL)
                xmlFree(xml_package);

             if (xml_name != NULL)
                xmlFree(xml_name);

             LOGD("[DFPS] Index=%d\n\tPackage name:%s\n\tApplication name:%s\n\tDFPS:%d\n",
                  pitem->index, pitem->package, pitem->name, pitem->dfps);

             g_dfps_list = eina_list_append(g_dfps_list, pitem);
          }
     }
}

static void _dfps_xml_parse(const char* docname)
{
   xmlDocPtr doc;
   xmlNodePtr cur;

   doc = xmlParseFile(docname);
   if( doc == NULL )
     {
        LOGE("[DFPS] Setting - Documentation is not parsed successfully\n");
        return;
     }

   cur = xmlDocGetRootElement(doc);
   if( cur == NULL )
     {
        LOGE("[DFPS] Setting - Empty documentation\n");
        xmlFreeDoc(doc);
        return;
     }

   if (xmlStrcmp(cur->name, (const xmlChar *) "DynamicFPS"))
     {
        LOGE("[DFPS] Setting - Documentation of the wrong type, root node != settings\n");
        xmlFreeDoc(doc);
        return;
     }

   cur = cur->xmlChildrenNode;
   _tree_walk_dfpslist(cur);

   if( doc )
     {
        xmlFreeDoc(doc);
     }

   // need to call this one for clean internal memory in xml.
   xmlCleanupParser();

   return;
}

static int _dfps_config_get(void)
{
   //eina_init();
   _dfps_xml_parse(DPFS_config_file);

   return 1;
}

static void _dfps_output_init(void)
{
   int i;

   XRRScreenResources* res = XRRGetScreenResources (dfpsInfo.disp, dfpsInfo.rootWin);
   dfpsInfo.output = 0;

   XRROutputInfo *output_info = NULL;

   if( res && (res->noutput != 0) )
     {
        for( i = 0; i < res->noutput; i++ )
          {
             output_info = XRRGetOutputInfo(dfpsInfo.disp, res, res->outputs[i]);

             if ((output_info) && !(strcmp(output_info->name, "LVDS1")))
               {
                  dfpsInfo.output = res->outputs[i];

                  if (output_info)
                     XRRFreeOutputInfo(output_info);
                  break;
               }

             if (output_info)
                XRRFreeOutputInfo(output_info);
          }
     }

   if(!dfpsInfo.output)
     {
        LOGE("[DFPS] Failed to init RR output !\n");
     }
}


static void
_dfps_dbus_signal_changed_cb(E_DBus_Object *obj, DBusMessage   *msg)
{
   DBusError err;
   int ret;
   int val;
   int pid = -1;
   int refresh_rate;

   ret = dbus_message_is_signal(msg, TRM_DFPS_INTERFACE, TRM_DFPS_NAME);
   if (!ret)
     {
        LOGE("[DFPS] dbus_message_is_signal error! ret(%d)", ret);
        return;
     }

   dbus_error_init(&err);
   ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
   if (!ret)
     {
        LOGE("[DFPS] dbus_message_get_args error! ret(%d)", ret);
        return;
     }

   /*
   *  val is FPS percentage.
   *   i.e> LCD refresh rate is 60.  val is 50(%). That means  60 * 50/100 -> 30 fps.
   *          PID -1 is special case.
   *          All rendering will use specific refresh_rate.
   */
   refresh_rate = MAX_LCD_REFRESH_RATE * (val /(float)100);
   _dfps_set_dfps_rate(pid, refresh_rate);

}

static  void _e_mod_processmgr_dfps_register_dbus_signal()
{
   E_DBus_Connection *conn = NULL;
   static E_DBus_Signal_Handler *edbus_dfps_handler = NULL;
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn)
     {
        LOGE("[DFPS] e_dbus_bus_get failed!");
        return ;
     }

   dfpsInfo.conn = conn;

   edbus_dfps_handler = e_dbus_signal_handler_add(conn, NULL, TRM_DFPS_OBJ_PATH,
                                                      TRM_DFPS_INTERFACE, TRM_DFPS_NAME,
                                                      (E_DBus_Signal_Cb)_dfps_dbus_signal_changed_cb,  NULL);

   if (!edbus_dfps_handler)
     {
        LOGE("[DFPS] e_dbus_signal_handler_add failed!");
        return ;
     }

   dfpsInfo.edbus_dfps_handler = edbus_dfps_handler;
}

EINTERN int _e_mod_processmgr_dfps_init(void)
{
   int res, ret = 1;

   memset(&dfpsInfo, 0, sizeof(dfpsInfo));

   dfpsInfo.disp = ecore_x_display_get();

   if(!dfpsInfo.disp)
     {
        LOGE("[DFPS] Failed to open X dis_dfps_app_search_dfps_rateplay..!\n");

        ret = 0;
        goto out;
     }

   dfpsInfo.rootWin = ecore_x_window_root_first_get();

   dfpsInfo.atomRROutput = ecore_x_atom_get(E_PROP_XRROUTPUT);

   _dfps_output_init();

   res = _dfps_config_get();

   if( !res )
     {
        LOGE("[DFPS] Failed to get configureation from %s.cfg file!\n", DPFS_config_file);
        ret = 0;
        goto out;
     }

   /* (B3 disable) register dbus signal handler for fps control with overheating. */
   //_e_mod_processmgr_dfps_register_dbus_signal();

out:
   return ret;
}

EINTERN int _e_mod_processmgr_dfps_close(void)
{
   // clean up g_dfps_list
   Eina_List* cur;
   Eina_List* next;
   dfps_entry *data;
   EINA_LIST_FOREACH_SAFE(g_dfps_list, cur, next, data)
     {
        if (data->package)
           free(data->package);
        if (data->name)
           free(data->name);
        free(data);
        g_dfps_list = eina_list_remove_list(g_dfps_list, cur);
     }

   if (dfpsInfo.edbus_dfps_handler != NULL)
     {
        e_dbus_signal_handler_del(dfpsInfo.conn, dfpsInfo.edbus_dfps_handler);
        dfpsInfo.edbus_dfps_handler = NULL;
     }

   if (dfpsInfo.conn != NULL)
     {
        e_dbus_connection_close(dfpsInfo.conn);
        dfpsInfo.conn = NULL;
     }

   return 1;
}

static void _dfps_app_dump_list(void)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             LOGD("[DFPS] list dump: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                  data->package, data->name, data->pid, data->dfps);
          }
   }
}

static int _dfps_app_pid_exists(int pid)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (data->pid && pid == data->pid)
               {
                  LOGD("[DFPS] pid [%d] exists: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);
                  return 1;
               }
          }
   }
   return 0;
}

static int _dfps_app_clear_pid(int pid)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (pid == data->pid)
               {
                  data->pid = 0;
                  LOGD("[DFPS] clear pid [%d]: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);
                  return 0;
               }
          }
   }
   return -1;
}

static int _dfps_app_set_pid(dfps_app_info * app_info, int pid )
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (app_info->package_name && app_info->app_name &&
                 data->package && data->name &&
                 0 == strcmp(app_info->package_name, data->package) &&
                 0 == strcmp(app_info->app_name, data->name))
               {
                  data->pid = pid;
                  LOGD("[DFPS] set pid [%d]: package_name [%s], app_name [%s], pid [ %d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);
                  return pid;
               }
          }
   }
   return -1;
}

static int _dfps_app_search_dfps_rate(int pid)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (data->pid && pid == data->pid)
               {
                  LOGD("[DFPS] pid [%d] search dfps: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);
                  return data->dfps;
               }
          }
   }
   return -1;
}

static char* _dfps_app_search_package(int pid)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (data->pid && pid == data->pid)
               {
                  LOGD("[DFPS] pid [%d] search package: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);

                  return data->package;
               }
          }
   }
   return NULL;
}

static char* _dfps_app_search_name(int pid)
{
   Eina_List * tmp;
   dfps_entry* data;

   if (g_dfps_list) {
        EINA_LIST_FOREACH(g_dfps_list, tmp, data)
          {
             if (data->pid && pid == data->pid)
               {
                  LOGD("[DFPS] pid [%d] search name: package_name [%s], app_name [%s], pid [%d], rate [%d]\n",
                       pid, data->package, data->name, data->pid, data->dfps);

                  return data->name;
               }
          }
   }
   return NULL;
}

static int _dfps_marshalize_string (char* buf, int num, char* srcs[])
{
   int i;
   char *p = buf;

   for (i=0; i<num; i++)
     {
        p += sprintf (p, "%s", srcs[i]);
        *p = '\0';
        p++;
     }

   *p = '\0';
   p++;

   return (p - buf);
}

static void _dfps_set_dfps_rate(int pid, int dfps_rate)
{
   if( current_fps == dfps_rate)
      return;

   char* cmdsFixed[2] = {"e_dynamic_fps", "dynamic_fps"};
   char cmdDynamic[32] = {0, };
   char* cmdCombined[] = {cmdsFixed[0], cmdsFixed[1], &cmdDynamic[0], NULL };

   int dfps_rate_and_pid = pid << 16 | dfps_rate;

   LOGD("[DFPS] Set DFPS refresh_rate=[%d] pid=[%d]: package_name [%s], app_name [%s]\n",
        dfps_rate, pid, _dfps_app_search_package(pid), _dfps_app_search_name(pid));

   sprintf(&cmdDynamic[0], "%d", dfps_rate_and_pid);

   dfpsInfo.rroutput_buf_len = _dfps_marshalize_string(dfpsInfo.rroutput_buf, 3, cmdCombined);

   XRRChangeOutputProperty (dfpsInfo.disp, dfpsInfo.output, dfpsInfo.atomRROutput, XA_CARDINAL, 8,
                            PropModeReplace, (unsigned char *) dfpsInfo.rroutput_buf, dfpsInfo.rroutput_buf_len);

   current_fps = dfps_rate;
}

static int _dfps_get_application_pkgname(Ecore_X_Window win, int pid, dfps_app_info* app_info)
{
   const char* tagAppPath[] = {"/usr/apps/", "/opt/apps/"};
   const int sizebuf = 256;
   char *buf = 0;
   char *buf_ptr1 = 0;
   char *buf_ptr2 = 0;
   char *pkg_ptr = 0;
   char *name_ptr = 0;
   int retval = 0;
   int argc = 0;
   char **argv = 0;
   int i;

   memset(app_info, 0, sizeof(dfps_app_info));

   ecore_x_icccm_command_get(win, &argc, &argv);

   if (argc < 1)
     {
        if (argv) free(argv);
        return retval;
     }

   if (argv == NULL)
     {
        return retval;
     }

   buf = argv[0];

   if (buf != strstr(buf, tagAppPath[0]) && buf != strstr(buf, tagAppPath[1]))
     {
        for (i = 0; i < argc; i++)
          {
             if (argv[i]) free(argv[i]);
          }
        free(argv);

        return retval;
     }
   buf_ptr1 = strdup(buf);
   buf_ptr2 = strdup(buf);

   name_ptr = basename(buf_ptr2);
   pkg_ptr = dirname(buf_ptr1);

   if (   pkg_ptr[strlen(pkg_ptr) - 3] == 'b'
       && pkg_ptr[strlen(pkg_ptr) - 2] == 'i'
       && pkg_ptr[strlen(pkg_ptr) - 1] == 'n')
     {
        pkg_ptr = dirname(pkg_ptr);
        pkg_ptr = basename(pkg_ptr);

        if (*pkg_ptr && *name_ptr)
          {
             app_info->package_name = strdup(pkg_ptr);
             app_info->app_name = strdup(name_ptr);
             retval = 1;
          }
     }

   if (buf_ptr1) free(buf_ptr1);
   if (buf_ptr2) free(buf_ptr2);
   for (i = 0; i < argc; i++)
     {
        if (argv[i]) free(argv[i]);
     }
   free(argv);

   return retval;
}

static void _dfps_action(dfps_app_info* app_info, int pid,  Ecore_X_Window win, dfps_app_state state)
{
   int dfps_rate = 0;
   Eina_List *l;
   E_Border *bd = NULL;
   E_Manager *man = NULL;
   E_Container *con = NULL;
   E_Border_List *bl;

   _dfps_app_dump_list();

   switch (state) {
      case DFPS_LAUNCH_STATE:
      case DFPS_RESUME_STATE:
         if (!_dfps_app_pid_exists(pid)) {
              if (-1 == _dfps_app_set_pid(app_info, pid))
              {
                 dfps_rate = TRM_DEFAULT_FPS;
                 _dfps_set_dfps_rate(pid, dfps_rate);
                 break;
              }
         }
         ; // fall through

         if (_dfps_app_pid_exists(pid)) {
              dfps_rate = _dfps_app_search_dfps_rate(pid);
              if ( -1 != dfps_rate )
              {
                 _dfps_set_dfps_rate(pid, dfps_rate);
              }
         }

         break;
      case DFPS_PAUSE_STATE:
      case DFPS_TERMINATED_STATE:
         man = e_manager_current_get();
         if(!man) return;
         con = e_container_current_get(man);
         if(!con) return;

         bl = e_container_border_list_last(con);
         if (bl)
           {
              while ((bd = e_container_border_list_prev(bl)))
                {
                   if(bd->client.win == win)
                     continue;
                   if (!E_INTERSECTS(bd->x, bd->y, bd->w, bd->h, bd->zone->x, bd->zone->y, bd->zone->w, bd->zone->h))
                     continue;

                   break;
                }
              e_container_border_list_free(bl);
           }
         if(!bd) return;

         dfps_rate = _dfps_app_search_dfps_rate(bd->client.netwm.pid);
         if ( -1 != dfps_rate )
         {
            _dfps_set_dfps_rate(bd->client.netwm.pid, dfps_rate);
         }
         else
         {
            _dfps_set_dfps_rate(bd->client.netwm.pid, TRM_DEFAULT_FPS );
         }
         break;
      default:
         ;//
   };
}


static int _dfps_get_window_pid(Ecore_X_Window win)
{
   int pid = 0;

   ecore_x_netwm_pid_get(win, &pid);

   return pid;
}

EINTERN void _e_mod_processmgr_dfps_process_state_change(dfps_app_state state, Ecore_X_Window win)
{
   dfps_app_info app_info;
   int pid = _dfps_get_window_pid(win);

   _dfps_get_application_pkgname(win, pid, &app_info);
   _dfps_action(&app_info,pid,win,state);

   if (app_info.package_name)
     free(app_info.package_name);
   if (app_info.app_name)
     free(app_info.app_name);
}
