#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_MOVE_DEBUG_H
#define E_MOD_MOVE_DEBUG_H

#define E_CHECK(x)           do {if (!(x)) return;    } while(0)
#define E_CHECK_RETURN(x, r) do {if (!(x)) return (r);} while(0)
#define E_CHECK_GOTO(x, l)   do {if (!(x)) goto l;    } while(0)

#define MOVE_LOG_BUILD_ENABLE 1
#if MOVE_LOG_BUILD_ENABLE
# define LT_NOTHING   0x0000
# define LT_EVENT_X   0x0001
# define LT_EVENT_BD  0x0002
# define LT_CREATE    0x0004
# define LT_EVENT_OBJ 0x0008
# define LT_INFO      0x0010
# define LT_INFO_SHOW 0x0080
# define LT_DUMP      0x0100
# define LT_ALL       0xFFFF

extern EINTERN int logtype;

# include <stdarg.h>
# define L(t, f, x...) { \
   if (logtype & t)      \
     printf(f, ##x);     \
}
#else
# define L(...) { ; }
#endif /* MOVE_LOG_BUILD_ENABLE */

typedef enum _E_Move_Event_Log_Type
{
   E_MOVE_EVENT_LOG_UNKOWN = 0,
   E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_DOWN,
   E_MOVE_EVENT_LOG_ECORE_SINGLE_MOUSE_UP,
   E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_DOWN,
   E_MOVE_EVENT_LOG_ECORE_MULTI_MOUSE_UP,
   E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_DOWN,
   E_MOVE_EVENT_LOG_EVAS_OBJECT_MOUSE_UP,
} E_Move_Event_Log_Type;

typedef enum _E_Move_Event_Log_Evas_Object_Type
{
   E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_UNKNOWN = 0,
   E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_QUICKPANEL,
   E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_INDICATOR,
   E_MOVE_EVENT_LOG_EVAS_OBJECT_TYPE_APPTRAY
} E_Move_Event_Log_Evas_Object_Type;

typedef struct _E_Move_Event_Log
{
   E_Move_Event_Log_Type t; // type

   union {
     struct {
        int x; // single touch x geomety position
        int y; // single touch y geomety position
        int btn; // Mouse button number
        Ecore_X_Window win;
     } ec_sm; // ecore sigle mouse data

     struct {
        double x; // multi touch x geomety position type is double
        double y; // multi touch y geomety position type is double
        int btn; // Mouse button number
        int dev; // 0: normal mouse  1+: other mouse, multi touch
        Ecore_X_Window win;
     } ec_mm; // ecore multi mouse data

     struct {
        int x; // evas object mouse down x geomety position
        int y;  // evas object mouse down y geomety position
        int btn; // Mouse button number
        int ox; // evas object geometry x
        int oy; // evas object geometry y
        int ow; // evas object geometry w
        int oh; // evas object geometry h
        Evas_Object *obj;
        E_Move_Event_Log_Evas_Object_Type t; // evas object type
     } eo_m; // evas object mouse data
   } d; // data
} E_Move_Event_Log;

EINTERN Eina_Bool e_mod_move_debug_info_dump(Eina_Bool to_file, const char *name);
EINTERN Eina_Bool e_mod_move_debug_prop_handle(Ecore_X_Event_Window_Property *ev);

#endif
#endif
