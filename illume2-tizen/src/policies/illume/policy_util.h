#ifndef _POLICY_UTIL_H
#define _POLICY_UTIL_H

/* for malloc trim and stack trim */
void e_illume_util_mem_trim (void);

/* for HDMI rotation */
void e_illume_util_hdmi_rotation (Ecore_X_Window root, int angle);

#endif // _POLICY_UTIL_H
