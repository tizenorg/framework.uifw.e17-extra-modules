#ifndef E_ILLUME_LOG_H
#define E_ILLUME_LOG_H

/* for logger */
#define ILLUME_LOGGER_BUILD_ENABLE 1

/* print type */
#define PT_NOTHING      0x0000
#define PT_STACK        0x0001
#define PT_VISIBILITY   0x0002
#define PT_ALL          0xFFFF

/* log type */
#define LT_NOTHING               0x00000000
#define LT_STACK                 0x00000001
#define LT_NOTIFICATION          0x00000002
#define LT_NOTIFICATION_LEVEL    0x00000004
#define LT_VISIBILITY            0x00000008
#define LT_SCREEN_LOCK           0x00000010
#define LT_XWIN                  0x00000020
#define LT_ANGLE                 0x00000040
#define LT_TRANSIENT_FOR         0x00000080
#define LT_QUICKPANEL            0x00000100
#define LT_KEYBOARD              0x00000200
#define LT_ICONIFY               0x00000400
#define LT_DUAL_DISPLAY          0x00000800
#define LT_AIA                   0x00001000
#define LT_INDICATOR             0x00002000
#define LT_VISIBILITY_DETAIL     0x00004000
#define LT_FLOATING              0x00008000
#define LT_ASSISTANT_MENU        0x00010000
#define LT_ALL                   0xFFFFFFFF

#if ILLUME_LOGGER_BUILD_ENABLE
extern int _e_illume_logger_type;
#include <stdarg.h>
#define L( type, fmt, args... ) { \
     if( _e_illume_logger_type & type ) printf( fmt, ##args ); \
}
#else
#define L( ... ) { ; }
#endif /* ILLUME_LOGGER_BUILD_ENABLE */

#endif
