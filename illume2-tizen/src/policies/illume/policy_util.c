#include "e_illume_private.h"
#include "policy_util.h"

/* for malloc trim and stack trim */
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <malloc.h>

#define GETSP() ({ unsigned int sp; asm volatile ("mov %0,sp " : "=r"(sp) ); sp;})
#define BUF_SIZE 256
#define PAGE_SIZE (1 << 12)
#define _ALIGN_UP(addr,size) (((addr)+((size)-1))&(~((size)-1)))
#define _ALIGN_DOWN(addr,size) ((addr)&(~((size)-1)))
#define PAGE_ALIGN(addr) _ALIGN_DOWN(addr, PAGE_SIZE)


/* for HDMI rotation */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#define HDMI_DEV_NODE "/dev/video14"
#define HDMI_BUF_SIZE 128

static int _e_illume_util_marshalize_string (char* buf, int num, char* srcs[]);

static Ecore_X_Atom _atom_hdmi = 0;


/* for malloc trim and stack trim */
void e_illume_util_mem_trim (void)
{
   malloc_trim(0);
}


/* for HDMI rotation */
void e_illume_util_hdmi_rotation (Ecore_X_Window root, int angle)
{
   Ecore_X_Display *dpy;
   RROutput output;
   char hdmi_commands[HDMI_BUF_SIZE];
   char buf[5];
   int buf_len;
   int i;
   char* x_control[] = {"illume2", "rotation", buf, NULL };

   if (access(HDMI_DEV_NODE, F_OK) == 0)
     {
        if (!_atom_hdmi)
           _atom_hdmi = ecore_x_atom_get ("X_RR_PROPERTY_REMOTE_CONTROLLER");

        if(!_atom_hdmi)
          {
             fprintf (stderr, "[ILLUME2] Critical Error!!! Cannot create X_RR_PROPERTY_REMOTE_CONTROLLER Atom...\n");
             return;
          }

        dpy = ecore_x_display_get();

        memset (hdmi_commands, 0, sizeof(hdmi_commands));
        memset (buf, 0, sizeof(buf));

        output = 0;
        XRRScreenResources* res = XRRGetScreenResources (dpy, root);
        if (res && (res->noutput != 0))
          {
             for (i=0; i<res->noutput; i++)
               {
                  output = res->outputs[i];
               }

             snprintf (buf, sizeof(buf)-1, "%d", angle);
             buf_len = _e_illume_util_marshalize_string (hdmi_commands, 3, x_control);

             XRRChangeOutputProperty(dpy, output, _atom_hdmi, XA_CARDINAL, 8, PropModeReplace, (unsigned char *)hdmi_commands, buf_len);
          }
        else
          {
             printf("[ILLUME2]Error.. Cannot get screen resource.\n");
          }
     }
}

static int _e_illume_util_marshalize_string (char* buf, int num, char* srcs[])
{
   int i;
   char * p = buf;

   for (i=0; i<num; i++)
     {
        p += snprintf (p, HDMI_BUF_SIZE-1, srcs[i]);
        *p = '\0';
        p++;
     }

   *p = '\0';
   p++;

   return (p - buf);
}

