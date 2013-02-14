#ifndef VIRT_MONITOR_DEVICEMGR_H
#define VIRT_MONITOR_DEVICEMGR_H

/* atom string */
#define STR_ATOM_VIRT_MONITOR_REQUEST "_VIRT_MONITOR_REQUEST"

/* virtual monitor command */
typedef enum
{
    VM_REQ_NULL,         /* null */
    VM_REQ_PLUG,         /* virtual monitor plugged */
    VM_REQ_UNPLUG,       /* virtual monitor unplugged */
} VM_REQ;

#endif // VIRT_MONITOR_DEVICEMGR_H

