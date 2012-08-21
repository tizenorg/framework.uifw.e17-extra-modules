#ifndef HIB_DEVICEMGR_H
#define HIB_DEVICEMGR_H

/* atom string */
#define STR_ATOM_HIB_REQUEST "_HIB_REQUEST"

/* hibernation command */
typedef enum
{
    HIB_REQ_NULL,        /* null */
    HIB_REQ_SET,         /* set hibernation */
    HIB_REQ_UNSET,      /* unset hibernation */
} HIB_REQ;

#endif // HIB_DEVICEMGR_H

