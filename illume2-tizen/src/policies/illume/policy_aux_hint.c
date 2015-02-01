#include "e_illume_private.h"
#include "policy.h"
#include "policy_aux_hint.h"

typedef struct _E_Illume_Aux_Hint_Item E_Illume_Aux_Hint_Item;

struct _E_Illume_Aux_Hint_Item
{
   /* val[0] previous value, val[1] current value */
   union
     {
        Eina_Bool b;
        int       i;
     } val[2];
   Eina_Bool changed;
};

static const char *hint_names[] =
{
   "wm.policy.win.startup.by",
   "wm.policy.win.zone.desk.layout.pos",
   "wm.policy.win.user.geometry",
   "wm.policy.win.fixed.resize",
};

/* externally accessible functions */
int
_policy_aux_hint_init(void)
{
   int i, n;
   n = (sizeof(hint_names) / sizeof(char *));

   Eina_List *l;
   E_Manager *man;
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        for (i = 0; i < n; i++)
          {
             e_hints_aux_hint_supported_add(man->root, hint_names[i]);
          }
     }

   return 1;
}

int
_policy_aux_hint_shutdown(void)
{
   return 1;
}

#define _STR_CMP(a, b) (strncmp(a, b, strlen(b)) == 0)

#define _VAL_CHECK(a, b, c, d, e) \
   else if (_STR_CMP(a, b))       \
     {                            \
        c = d;                    \
        e = EINA_TRUE;            \
        send = EINA_TRUE;         \
     }

void
_policy_border_hook_aux_hint_eval(E_Border *bd)
{
   E_CHECK(bd);

   E_Illume_Border_Info* bd_info = NULL;
   E_Illume_Aux_Hint_Item startup;

   startup.val[0].i = bd->launch_info.launch_by;
   startup.val[1].i = -1;
   startup.changed = EINA_FALSE;

   E_Illume_Aux_Hint_Item ly_pos;
   ly_pos.val[0].i =  bd->launch_info.launch_to;
   ly_pos.val[1].i =  -2;
   ly_pos.changed = EINA_FALSE;

   E_Border_Aux_Hint *hint;
   Eina_List *l;
   EINA_LIST_FOREACH(bd->client.e.state.aux_hint.hints, l, hint)
     {
        Eina_Bool send = EINA_FALSE;
        if (!hint->deleted)
          {
             if (_STR_CMP(hint->hint, hint_names[0]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "0", startup.val[1].i, 0, startup.changed)
                  _VAL_CHECK(hint->val, "1", startup.val[1].i, 1, startup.changed)
                  _VAL_CHECK(hint->val, "2", startup.val[1].i, 2, startup.changed)
                  _VAL_CHECK(hint->val, "normal", startup.val[1].i, 0,  startup.changed)
                  _VAL_CHECK(hint->val, "task-manager", startup.val[1].i, 1,  startup.changed)
                  _VAL_CHECK(hint->val, "split-launcher", startup.val[1].i, 2, startup.changed)
               }
             else if (_STR_CMP(hint->hint, hint_names[1]))
               {
                  if (0) ;
                  _VAL_CHECK(hint->val, "-2", ly_pos.val[1].i, -2,  ly_pos.changed)
                  _VAL_CHECK(hint->val, "-1", ly_pos.val[1].i, -1, ly_pos.changed)
                  _VAL_CHECK(hint->val, "0", ly_pos.val[1].i, 0, ly_pos.changed)
                  _VAL_CHECK(hint->val, "1", ly_pos.val[1].i, 1,  ly_pos.changed)
                  _VAL_CHECK(hint->val, "0.0.0.-2", ly_pos.val[1].i, -2,  ly_pos.changed)
                  _VAL_CHECK(hint->val, "0.0.0.0", ly_pos.val[1].i, 0, ly_pos.changed)
                  _VAL_CHECK(hint->val, "0.0.0.1", ly_pos.val[1].i, 1,  ly_pos.changed)
               }
          }

        if (hint->changed)
          {
             if (_STR_CMP(hint->hint, hint_names[2]))
               {
                  bd_info = policy_get_border_info(bd);
                  if (bd_info)
                    {
                       if (_STR_CMP(hint->val, "1"))
                          bd_info->allow_user_geometry = EINA_TRUE;
                       else
                          bd_info->allow_user_geometry = EINA_FALSE;

                       send = EINA_TRUE;
                    }
               }
             else if (_STR_CMP(hint->hint, hint_names[3]))
               {
                  bd_info = policy_get_border_info(bd);
                  if (bd_info)
                    {
                       if (_STR_CMP(hint->val, "1"))
                         bd_info->resize_req.use_fixed_ratio = EINA_TRUE;
                       else
                         bd_info->resize_req.use_fixed_ratio = EINA_FALSE;

                       send = EINA_TRUE;
                    }
               }

             if (send)
               e_border_aux_hint_reply_send(bd, hint->id);
          }
     }

   if ((startup.changed) && (startup.val[0].i != startup.val[1].i))
     bd->launch_info.launch_by = startup.val[1].i;

   if ((ly_pos.changed) && (ly_pos.val[0].i != ly_pos.val[1].i))
     {
        bd->launch_info.launch_to = ly_pos.val[1].i;
     }
}
