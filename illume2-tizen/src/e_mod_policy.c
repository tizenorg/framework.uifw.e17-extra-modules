#include "e_illume_private.h"
#include "e_mod_policy.h"

/* local function prototypes */
static char *_e_mod_policy_find(void);
static int _e_mod_policy_load(char *file);
static void _e_mod_policy_handlers_add(void);
static void _e_mod_policy_hooks_add(void);
static void _e_mod_policy_cb_free(E_Illume_Policy *p);
static Eina_Bool _e_mod_policy_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_move(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_iconify(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_uniconify(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_zone_move_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_property(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_policy_change(void *data __UNUSED__, int type, void *event __UNUSED__);
static Eina_Bool _e_mod_policy_cb_window_configure_request (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_move_resize_request(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_state_request(void *data __UNUSED__, int type __UNUSED__, void *event);

static void _e_mod_policy_cb_hook_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_post_assign(void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_layout(void *data __UNUSED__, void *data2 __UNUSED__);
static void _e_mod_policy_cb_hook_post_new_border (void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_pre_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_policy_cb_hook_new_border(void *data __UNUSED__, void *data2);
#ifdef _F_BORDER_HOOK_PATCH_
static void _e_mod_policy_cb_hook_del_border(void *data __UNUSED__, void *data2);
#endif

static Eina_Bool _e_mod_policy_cb_window_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event);

static int _e_mod_policy_init_atom (void);
static Eina_Bool _e_mod_policy_cb_border_stack(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_border_zone_set(void *data __UNUSED__, int type __UNUSED__, void *event);

/* for visibility */
static Eina_Bool _e_mod_policy_cb_window_create (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_destroy (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_reparent (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_show (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_hide (void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_window_configure (void *data __UNUSED__, int type __UNUSED__, void *event);

static Eina_Bool _e_mod_policy_zonelist_update(void);
static Eina_Bool _e_mod_policy_cb_zone_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_policy_cb_zone_del(void *data __UNUSED__, int type __UNUSED__, void *event);

static Eina_Bool _e_mod_policy_cb_module_update(void *data __UNUSED__, int type __UNUSED__, void *event);

static Eina_Bool _e_mod_policy_cb_idle_enterer(void *data __UNUSED__);

/* local variables */
static E_Illume_Policy *_policy = NULL;
static Eina_List *_policy_hdls = NULL, *_policy_hooks = NULL;
Ecore_X_Atom E_ILLUME_BORDER_WIN_RESTACK;
static Ecore_X_Atom E_ILLUME_USER_CREATED_WINDOW = 0;
static Ecore_X_Atom E_ILLUME_PARENT_BORDER_WINDOW = 0;
static Ecore_X_Atom E_ILLUME_ZONE_GEOMETRY = 0;

static Ecore_X_Atom E_ILLUME_ATOM_DESK_TOP_WIN_DESK_CHANGE = 0;
static Ecore_X_Atom E_ILLUME_ATOM_DESK_NEXT_SHOW = 0;

static Ecore_Idle_Enterer *_idle_enterer = NULL;

/* external variables */
int E_ILLUME_POLICY_EVENT_CHANGE = 0;

int
e_mod_policy_init(void)
{
   Eina_List *ml;
   E_Manager *man;
   char *file;

   if (!_e_mod_policy_init_atom())
     {
        /* creating atom failed, bail out */
        printf ("Cannot create atom\n");
        return 0;
     }

   /* try to find the policy specified in config */
   if (!(file = _e_mod_policy_find()))
     {
        printf("Cannot find policy\n");
        return 0;
     }

   /* attempt to load policy */
   if (!_e_mod_policy_load(file))
     {
        /* loading policy failed, bail out */
        printf("Cannot load policy: %s\n", file);
        if (file) free(file);
        return 0;
     }

   /* create new event for policy changes */
   E_ILLUME_POLICY_EVENT_CHANGE = ecore_event_type_new();

   /* add our event handlers */
   _e_mod_policy_handlers_add();

   /* add our border hooks */
   _e_mod_policy_hooks_add();

   _idle_enterer = ecore_idle_enterer_add(_e_mod_policy_cb_idle_enterer, NULL);

   /* loop the root windows */
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        Eina_List *cl;
        E_Container *con;

        if (!man) continue;

        /* loop the containers */
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             Eina_List *zl;
             E_Zone *zone;

             if (!con) continue;

             /* loop the zones */
             EINA_LIST_FOREACH(con->zones, zl, zone)
               {
                  E_Illume_Config_Zone *cz;
                  Ecore_X_Illume_Mode mode = ECORE_X_ILLUME_MODE_SINGLE;

                  if (!zone) continue;

                  /* check for zone config */
                  if (!(cz = e_illume_zone_config_get(zone->id)))
                    continue;

                  /* set mode on this zone */
                  if (cz->mode.dual == 0)
                    mode = ECORE_X_ILLUME_MODE_SINGLE;
                  else
                    {
                       if ((cz->mode.dual == 1) && (cz->mode.side == 0))
                         mode = ECORE_X_ILLUME_MODE_DUAL_TOP;
                       else if ((cz->mode.dual == 1) && (cz->mode.side == 1))
                         mode = ECORE_X_ILLUME_MODE_DUAL_LEFT;
                    }
                  ecore_x_e_illume_mode_set(zone->black_win, mode);
               }
          }
     }

   if (file) free(file);
   return 1;
}

int
e_mod_policy_shutdown(void)
{
   Ecore_Event_Handler *hdl;
   E_Border_Hook *hook;

   /* remove the ecore event handlers */
   EINA_LIST_FREE(_policy_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* remove the border hooks */
   EINA_LIST_FREE(_policy_hooks, hook)
     e_border_hook_del(hook);

   if (_idle_enterer) ecore_idle_enterer_del(_idle_enterer);
   _idle_enterer = NULL;

   /* destroy the policy if it exists */
   if (_policy) e_object_del(E_OBJECT(_policy));

   /* reset event type */
   E_ILLUME_POLICY_EVENT_CHANGE = 0;

   return 1;
}

/* local functions */
static char *
_e_mod_policy_find(void)
{
   Eina_List *files;
   char buff[PATH_MAX], dir[PATH_MAX], *file;

   snprintf(buff, sizeof(buff), "%s.so", _e_illume_cfg->policy.name);
   snprintf(dir, sizeof(dir), "%s/policies", _e_illume_mod_dir);

   /* try to list all files in this directory */
   if (!(files = ecore_file_ls(dir))) return NULL;

   /* loop the returned files */
   EINA_LIST_FREE(files, file)
     {
        /* compare file with needed .so */
        if (!strcmp(file, buff))
          {
             snprintf(dir, sizeof(dir), "%s/policies/%s",
                      _e_illume_mod_dir, file);
             break;
          }
        free(file);
     }
   if (file) free(file);
   else
     {
        /* if we did not find the requested policy, use a fallback */
        snprintf(dir, sizeof(dir), "%s/policies/illume.so", _e_illume_mod_dir);
     }

   return strdup(dir);
}

static int
_e_mod_policy_load(char *file)
{
   /* safety check */
   if (!file) return 0;

   /* delete existing policy first */
   if (_policy) e_object_del(E_OBJECT(_policy));

   /* try to create our new policy object */
   _policy =
     E_OBJECT_ALLOC(E_Illume_Policy, E_ILLUME_POLICY_TYPE,
                    _e_mod_policy_cb_free);
   if (!_policy)
     {
        printf("Failed to allocate new policy object\n");
        return 0;
     }

   /* attempt to open the .so */
   if (!(_policy->handle = dlopen(file, (RTLD_NOW | RTLD_GLOBAL))))
     {
        /* cannot open the .so file, bail out */
        printf("Cannot open policy: %s\n", ecore_file_file_get(file));
        printf("\tError: %s\n", dlerror());
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* clear any existing errors in dynamic loader */
   dlerror();

   /* try to link to the needed policy api functions */
   _policy->api = dlsym(_policy->handle, "e_illume_policy_api");
   _policy->funcs.init = dlsym(_policy->handle, "e_illume_policy_init");
   _policy->funcs.shutdown = dlsym(_policy->handle, "e_illume_policy_shutdown");

   /* check that policy supports needed functions */
   if ((!_policy->api) || (!_policy->funcs.init) || (!_policy->funcs.shutdown))
     {
        /* policy doesn't support needed functions, bail out */
        printf("Policy does not support needed functions: %s\n",
               ecore_file_file_get(file));
        printf("\tError: %s\n", dlerror());
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* check policy api version */
   if (_policy->api->version < E_ILLUME_POLICY_API_VERSION)
     {
        /* policy is too old, bail out */
        printf("Policy is too old: %s\n", ecore_file_file_get(file));
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   /* try to initialize the policy */
   if (!_policy->funcs.init(_policy))
     {
        /* init failed, bail out */
        printf("Policy failed to initialize: %s\n", ecore_file_file_get(file));
        e_object_del(E_OBJECT(_policy));
        return 0;
     }

   return 1;
}

static void
_e_mod_policy_handlers_add(void)
{
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_ADD,
                                              _e_mod_policy_cb_border_add, NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
                                              _e_mod_policy_cb_border_del, NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN,
                                              _e_mod_policy_cb_border_focus_in,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT,
                                              _e_mod_policy_cb_border_focus_out,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_SHOW,
                                              _e_mod_policy_cb_border_show,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_MOVE,
                                              _e_mod_policy_cb_border_move,
                                              NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_STACK,
                                              _e_mod_policy_cb_border_stack,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_ZONE_SET,
                                              _e_mod_policy_cb_border_zone_set, NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
                                              _e_mod_policy_cb_zone_move_resize,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
                                              _e_mod_policy_cb_client_message,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,
                                              _e_mod_policy_cb_window_property,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_ILLUME_POLICY_EVENT_CHANGE,
                                              _e_mod_policy_cb_policy_change,
                                              NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN,
                                              _e_mod_policy_cb_window_focus_in,
                                              NULL));

   /* for visibility */
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,
                                              _e_mod_policy_cb_window_create,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,
                                              _e_mod_policy_cb_window_destroy,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,
                                              _e_mod_policy_cb_window_reparent,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,
                                              _e_mod_policy_cb_window_show,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE,
                                              _e_mod_policy_cb_window_hide,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE,
                                              _e_mod_policy_cb_window_configure,
                                              NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE_REQUEST,
                                              _e_mod_policy_cb_window_configure_request,
                                              NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_MOVE_RESIZE_REQUEST,
                                              _e_mod_policy_cb_window_move_resize_request, NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STATE_REQUEST,
                                              _e_mod_policy_cb_window_state_request,
                                              NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_ZONE_ADD,
                                              _e_mod_policy_cb_zone_add, NULL));
   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_ZONE_DEL,
                                              _e_mod_policy_cb_zone_del, NULL));


   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,
                                              _e_mod_policy_cb_border_iconify,
                                              NULL));

   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,
                                              _e_mod_policy_cb_border_uniconify,
                                              NULL));


   _policy_hdls =
     eina_list_append(_policy_hdls,
                      ecore_event_handler_add(E_EVENT_MODULE_UPDATE,
                                              _e_mod_policy_cb_module_update,
                                              NULL));
}

static void
_e_mod_policy_hooks_add(void)
{
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH,
                                        _e_mod_policy_cb_hook_post_fetch, NULL));
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
                                        _e_mod_policy_cb_hook_post_assign, NULL));
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT,
                                        _e_mod_policy_cb_hook_layout, NULL));
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_EVAL_POST_NEW_BORDER,
                                        _e_mod_policy_cb_hook_post_new_border, NULL));
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_FETCH,
                                        _e_mod_policy_cb_hook_pre_fetch, NULL));
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_NEW_BORDER,
                                        _e_mod_policy_cb_hook_new_border, NULL));
#ifdef _F_BORDER_HOOK_PATCH_
   _policy_hooks =
     eina_list_append(_policy_hooks,
                      e_border_hook_add(E_BORDER_HOOK_DEL_BORDER,
                                        _e_mod_policy_cb_hook_del_border, NULL));
#endif
}

static void
_e_mod_policy_cb_free(E_Illume_Policy *p)
{
   if (!p) return;

   /* tell the policy to shutdown */
   if (p->funcs.shutdown) p->funcs.shutdown(p);
   p->funcs.shutdown = NULL;

   p->funcs.init = NULL;
   p->api = NULL;

   /* close the linked .so */
   if (p->handle) dlclose(p->handle);
   p->handle = NULL;

   E_FREE(p);
}

static Eina_Bool
_e_mod_policy_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Add *ev;

   ev = event;

   if ((_policy) && (_policy->funcs.border_add))
     _policy->funcs.border_add(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Remove *ev;

   ev = event;

   ecore_x_window_prop_property_del(ev->border->client.win, E_ILLUME_PARENT_BORDER_WINDOW);

   if ((_policy) && (_policy->funcs.border_del))
     _policy->funcs.border_del(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_focus_in))
     _policy->funcs.border_focus_in(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_focus_out))
     _policy->funcs.border_focus_out(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_show(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Show *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_show))
     _policy->funcs.border_show(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_move(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Move *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_move))
     _policy->funcs.border_move(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_iconify(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Iconify *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_iconify_cb))
     _policy->funcs.border_iconify_cb(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_uniconify(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Uniconify *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_uniconify_cb))
     _policy->funcs.border_uniconify_cb(ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_zone_move_resize(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Zone_Move_Resize *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.zone_move_resize))
     _policy->funcs.zone_move_resize(ev->zone);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW)
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.border_activate))
          _policy->funcs.border_activate(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_MODE)
     {
        E_Zone *zone;

        if (!(zone = e_util_zone_window_find(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.zone_mode_change))
          _policy->funcs.zone_mode_change(zone, ev->data.l[0]);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_CLOSE)
     {
        E_Zone *zone;

        if (!(zone = e_util_zone_window_find(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.zone_close))
          _policy->funcs.zone_close(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_FOCUS_BACK)
     {
        E_Zone *zone;

        if (!(zone = e_util_zone_window_find(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.focus_back))
          _policy->funcs.focus_back(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_FOCUS_FORWARD)
     {
        E_Zone *zone;

        if (!(zone = e_util_zone_window_find(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.focus_forward))
          _policy->funcs.focus_forward(zone);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_START)
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.drag_start))
          _policy->funcs.drag_start(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_END)
     {
        E_Border *bd;

        if (!(bd = e_border_find_by_client_window(ev->win))) return ECORE_CALLBACK_PASS_ON;
        if ((_policy) && (_policy->funcs.drag_end))
          _policy->funcs.drag_end(bd);
     }
   else if (ev->message_type == E_ILLUME_BORDER_WIN_RESTACK)
     {
        E_Border *bd;
        E_Border *bd_sibling;

        if (!(bd = e_border_find_by_client_window(ev->win))) return 1;
        if (!(bd_sibling = e_border_find_by_client_window((Ecore_X_Window) ev->data.l[0]))) return 1;

        if ((_policy) && (_policy->funcs.border_restack_request))
          _policy->funcs.border_restack_request(bd, bd_sibling, ev->data.l[1]);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE)
     {
        if ((_policy) && (_policy->funcs.window_sync_draw_done))
          _policy->funcs.window_sync_draw_done(ev);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE)
     {
        if ((_policy) && (_policy->funcs.quickpanel_state_change))
          _policy->funcs.quickpanel_state_change(ev);
     }
   else if (ev->message_type == E_ILLUME_ATOM_DESK_TOP_WIN_DESK_CHANGE)
     {
        if ((_policy) && (_policy->funcs.window_desk_set))
          _policy->funcs.window_desk_set(ev);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_WINDOW_STATE)
     {
        if ((_policy) && (_policy->funcs.illume_win_state_change_request))
          _policy->funcs.illume_win_state_change_request(ev);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_property(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Property *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.property_change))
     _policy->funcs.property_change(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_policy_change(void *data __UNUSED__, int type, void *event __UNUSED__)
{
   char *file;

   if (type != E_ILLUME_POLICY_EVENT_CHANGE) return ECORE_CALLBACK_PASS_ON;

   /* find policy specified in config */
   if (!(file = _e_mod_policy_find())) return ECORE_CALLBACK_PASS_ON;

   /* try to load the policy */
   _e_mod_policy_load(file);

   if (file) free(file);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_policy_cb_hook_post_fetch(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_post_fetch))
     _policy->funcs.border_post_fetch(bd);
}

static void
_e_mod_policy_cb_hook_post_assign(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_post_assign))
     _policy->funcs.border_post_assign(bd);
}

static void
_e_mod_policy_cb_hook_post_new_border (void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_post_new_border))
     _policy->funcs.border_post_new_border(bd);
}

static void _e_mod_policy_cb_hook_pre_fetch(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_pre_fetch))
     _policy->funcs.border_pre_fetch(bd);
}

static void
_e_mod_policy_cb_hook_new_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;

   ecore_x_window_prop_window_set(bd->win, E_ILLUME_USER_CREATED_WINDOW, &(bd->client.win), 1);
   ecore_x_window_prop_window_set(bd->client.win, E_ILLUME_PARENT_BORDER_WINDOW, &(bd->win), 1);

   if ((_policy) && (_policy->funcs.border_new_border))
     _policy->funcs.border_new_border(bd);
}

#ifdef _F_BORDER_HOOK_PATCH_
static void
_e_mod_policy_cb_hook_del_border(void *data __UNUSED__, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((_policy) && (_policy->funcs.border_del_border))
     _policy->funcs.border_del_border(bd);
}

#endif

static Eina_Bool
_e_mod_policy_cb_window_configure_request (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Configure_Request* ev = event;

   if ((_policy) && (_policy->funcs.window_configure_request))
     _policy->funcs.window_configure_request(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_move_resize_request(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Move_Resize_Request *ev = event;

   if ((_policy) && (_policy->funcs.window_move_resize_request))
     _policy->funcs.window_move_resize_request(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_state_request(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_State_Request *ev = event;

   if ((_policy) && (_policy->funcs.window_state_request))
     _policy->funcs.window_state_request(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_policy_cb_hook_layout(void *data __UNUSED__, void *data2 __UNUSED__)
{
   E_Zone *zone;
   E_Border *bd;
   Eina_List *zl = NULL, *l;

   /* loop through border list and find what changed */
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd) continue;
        if ((bd->new_client) || (bd->pending_move_resize) ||
            (bd->changes.pos) || (bd->changes.size) || (bd->changes.visible) ||
            (bd->need_shape_export) || (bd->need_shape_merge))
          {
             /* NB: this border changed. add it's zone to list of what needs
              * updating. This is done so we do not waste cpu cycles
              * updating zones where nothing changed */
             if (!eina_list_data_find(zl, bd->zone))
               zl = eina_list_append(zl, bd->zone);
          }
     }

   /* loop the zones that need updating and call the policy update function */
   EINA_LIST_FREE(zl, zone)
     {
        if ((_policy) && (_policy->funcs.zone_layout))
          _policy->funcs.zone_layout(zone);
     }
}

static Eina_Bool
_e_mod_policy_cb_window_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Focus_In *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.window_focus_in))
     _policy->funcs.window_focus_in(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static int
_e_mod_policy_init_atom (void)
{
   E_ILLUME_BORDER_WIN_RESTACK = ecore_x_atom_get ("_E_ILLUME_RESTACK_WINDOW");
   if (!E_ILLUME_BORDER_WIN_RESTACK)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_ILLUME_RESTACK_WINDOW Atom...\n");
        return 0;
     }

   E_ILLUME_USER_CREATED_WINDOW = ecore_x_atom_get("_E_USER_CREATED_WINDOW");
   if (!E_ILLUME_USER_CREATED_WINDOW)
     {
        fprintf(stderr,
                "[ILLUME2] cannot create _E_USER_CREATED_WINDOW atom.\n");
        return 0;
     }

   E_ILLUME_PARENT_BORDER_WINDOW = ecore_x_atom_get("_E_PARENT_BORDER_WINDOW");
   if (!E_ILLUME_PARENT_BORDER_WINDOW)
     {
        fprintf(stderr,
                "[ILLUME2] cannot create _E_PARENT_BORDER_WINDOW atom.\n");
        return 0;
     }

   E_ILLUME_ZONE_GEOMETRY = ecore_x_atom_get("_E_ILLUME_ZONE_GEOMETRY");
   if (!E_ILLUME_ZONE_GEOMETRY)
     {
        fprintf(stderr,
                "[ILLUME2] cannot create _E_ILLUME_ZONE_GEOMETRY atom.\n");
        return 0;
     }

   E_ILLUME_ATOM_DESK_TOP_WIN_DESK_CHANGE =
      ecore_x_atom_get ("_E_ILLUME_ATOM_DESK_TOP_WIN_DESK_CHANGE");
   if(!E_ILLUME_ATOM_DESK_TOP_WIN_DESK_CHANGE)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_COMP_ENABLE Atom...\n");
        return 0;
     }

   E_ILLUME_ATOM_DESK_NEXT_SHOW =
      ecore_x_atom_get ("_E_ILLUME_ATOM_DESK_NEXT_SHOW");
   if(!E_ILLUME_ATOM_DESK_NEXT_SHOW)
     {
        fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create _E_COMP_ENABLE Atom...\n");
        return 0;
     }

   return 1;
}

static Eina_Bool
_e_mod_policy_cb_border_stack(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Stack *ev;

   ev = event;
   if ((_policy) && (_policy->funcs.border_stack))
     _policy->funcs.border_stack(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_border_zone_set(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Zone_Set *ev;
   ev = event;

   if ((_policy) && (_policy->funcs.border_zone_set))
     _policy->funcs.border_zone_set(ev);

   return ECORE_CALLBACK_PASS_ON;
}

/* for visibility */
static Eina_Bool
_e_mod_policy_cb_window_create (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Create* ev = event;

   if ((_policy) && (_policy->funcs.window_create))
     _policy->funcs.window_create(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_destroy (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Destroy* ev = event;

   if ((_policy) && (_policy->funcs.window_destroy))
     _policy->funcs.window_destroy(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_reparent (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Reparent* ev = event;

   if ((_policy) && (_policy->funcs.window_reparent))
     _policy->funcs.window_reparent(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_show (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Show* ev = event;

   if ((_policy) && (_policy->funcs.window_show))
     _policy->funcs.window_show(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_hide (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Hide* ev = event;

   if ((_policy) && (_policy->funcs.window_hide))
     _policy->funcs.window_hide(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_window_configure (void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Window_Configure* ev = event;

   if ((_policy) && (_policy->funcs.window_configure))
     _policy->funcs.window_configure(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_zonelist_update(void)
{
   Eina_List *ml, *cl, *zl;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   Ecore_X_Window *zones;
   int zcount = 0;

   /* loop zones and get count */
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        if (!man) continue;
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             if (!con) continue;
             EINA_LIST_FOREACH(con->zones, zl, zone)
                zcount++;
          }
     }

   /* allocate enough zones */
   zones = calloc(zcount, sizeof(Ecore_X_Window));
   if (!zones) return EINA_FALSE;

   zcount = 0;

   /* loop the zones and set zone list */
   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
        if (!man) continue;
        EINA_LIST_FOREACH(man->containers, cl, con)
          {
             if (!con) continue;
             EINA_LIST_FOREACH(con->zones, zl, zone)
               {
                  if (!zone) continue;

                  zones[zcount] = zone->black_win;
                  zcount++;
               }
          }

        ecore_x_e_illume_zone_list_set(man->root, zones, zcount);
     }

   free(zones);
   return EINA_TRUE;
}

static Eina_Bool
_e_mod_policy_cb_zone_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Zone_Add *ev;
   E_Zone *zone;
   unsigned int geom[4];

   ev = event;
   zone = ev->zone;

   geom[0] = zone->x;
   geom[1] = zone->y;
   geom[2] = zone->w;
   geom[3] = zone->h;
   ecore_x_window_prop_card32_set(zone->black_win, E_ILLUME_ZONE_GEOMETRY,
                                  geom, 4);

   _e_mod_policy_zonelist_update();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_zone_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Zone_Del *ev;
   E_Zone *zone;

   ev = event;
   zone = ev->zone;

   _e_mod_policy_zonelist_update();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_module_update(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Module_Update *ev;
   ev = event;

   if ((_policy) && (_policy->funcs.module_update))
     _policy->funcs.module_update(ev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_policy_cb_idle_enterer(void *data __UNUSED__)
{
   if ((_policy) && (_policy->funcs.idle_enterer))
     _policy->funcs.idle_enterer();

   return ECORE_CALLBACK_RENEW;
}
