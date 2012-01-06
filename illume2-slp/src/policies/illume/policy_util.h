#ifndef _POLICY_UTIL_H
#define _POLICY_UTIL_H

/* for malloc trim and stack trim */
void e_illume_util_mem_trim (void);

/* for HDMI rotation */
void e_illume_util_hdmi_rotation (Ecore_X_Window root, int angle);

/* for logger */
#define ILLUME_LOGGER_BUILD_ENABLE 1
#if ILLUME_LOGGER_BUILD_ENABLE
/* print type */
#define PT_NOTHING      0x0000
#define PT_STACK        0x0001
#define PT_VISIBILITY   0x0002
#define PT_ALL          0xFFFF

/* log type */
#define LT_NOTHING               0x0000
#define LT_STACK                 0x0001
#define LT_NOTIFICATION          0x0002
#define LT_NOTIFICATION_LEVEL    0x0004
#define LT_VISIBILITY            0x0008
#define LT_LOCK_SCREEN           0x0010
#define LT_XWIN                  0x0020
#define LT_ANGLE                 0x0040
#define LT_ALL                   0xFFFF

#include <stdarg.h>
#define L( type, fmt, args... ) { \
     if( _illume_logger_type & type ) printf( fmt, ##args ); \
}
#else
#define L( ... ) { ; }
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

#endif // _POLICY_UTIL_H
