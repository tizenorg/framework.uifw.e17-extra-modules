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
#define LT_NOTHING               0x0000
#define LT_STACK                 0x0001
#define LT_NOTIFICATION          0x0002
#define LT_NOTIFICATION_LEVEL    0x0004
#define LT_VISIBILITY            0x0008
#define LT_LOCK_SCREEN           0x0010
#define LT_XWIN                  0x0020
#define LT_ANGLE                 0x0040
#define LT_TRANSIENT_FOR         0x0080
#define LT_QUICKPANEL            0x0100
#define LT_KEYBOARD              0x0200
#define LT_ICONIFY               0x0400
#define LT_DUAL_DISPLAY          0x0800
#define LT_AIA                   0x1000
#define LT_INDICATOR             0x2000
#define LT_VISIBILITY_DETAIL     0x4000
#define LT_FLOATING              0x8000
#define LT_ALL                   0xFFFF

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
