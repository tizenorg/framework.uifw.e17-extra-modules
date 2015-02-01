#ifndef _POLICY_UTIL_H
#define _POLICY_UTIL_H

/* for malloc trim and stack trim */
void e_illume_util_mem_trim (void);

/* for HDMI rotation */
void e_illume_util_hdmi_rotation (Ecore_X_Window root, int angle);


#define CALCULATE_RESIZE_BR(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                             \
   dx = ev_x - x2;                                            \
   dy = ev_y - y2;                                            \
   cx = base_x;                                               \
   cy = base_y;                                               \
   if (dx >= 0 || dy >= 0)                                    \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x2 = ev_x;                                       \
             y2 = (double)(y2 + (double)(dx * w_base_ratio)); \
          }                                                   \
        else                                                  \
          {                                                   \
             x2 = (double)(x2 + (double)(dy * h_base_ratio)); \
             y2 = ev_y;                                       \
          }                                                   \
     }                                                        \
   else                                                       \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x2 = x2 + dx;                                    \
             y2 = y2 + (double)(dx * w_base_ratio);           \
          }                                                   \
        else                                                  \
          {                                                   \
             x2 = x2 + (double)(dy * h_base_ratio);           \
             y2 = y2 + dy;                                    \
          }                                                   \
     }                                                        \
}

#define CALCULATE_RESIZE_TR(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                             \
   dx = ev_x - x2;                                            \
   dy = y1 - ev_y;                                            \
   cx = base_x;                                               \
   cy = base_y + base_h;                                      \
   if (dx >= 0 || dy >= 0)                                    \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x2 = ev_x;                                       \
             y1 = (double)(y1 - (double)(dx * w_base_ratio)); \
          }                                                   \
        else                                                  \
          {                                                   \
             x2 = (double)(x2 + (double)(dy * h_base_ratio)); \
             y1 = ev_y;                                       \
          }                                                   \
     }                                                        \
   else                                                       \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x2 = x2 + dx;                                    \
             y1 = y1 - (double)(dx * w_base_ratio);           \
          }                                                   \
        else                                                  \
          {                                                   \
             x2 = x2 + (double)(dy * h_base_ratio);           \
             y1 = y1 - dy;                                    \
          }                                                   \
     }                                                        \
}


#define CALCULATE_RESIZE_TL(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                             \
   dx = x1 - ev_x;                                            \
   dy = y1 - ev_y;                                            \
   cx = base_x + base_w;                                      \
   cy = base_y + base_h;                                      \
   if (dx >= 0 || dy >= 0)                                    \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x1 = ev_x;                                       \
             y1 = (double)(y1 - (double)(dx * w_base_ratio)); \
          }                                                   \
        else                                                  \
          {                                                   \
             x1 = (double)(x1 - (double)(dy * h_base_ratio)); \
             y1 = ev_y;                                       \
          }                                                   \
     }                                                        \
   else                                                       \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x1 = ev_x;                                       \
             y1 = y1 - (double)(dx * w_base_ratio);           \
          }                                                   \
        else                                                  \
          {                                                   \
             x1 = x1 - (double)(dy * h_base_ratio);           \
             y1 = ev_y;                                       \
          }                                                   \
     }                                                        \
}


#define CALCULATE_RESIZE_BL(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                             \
   dx = x1 - ev_x;                                            \
   dy = ev_y - y2;                                            \
   cx = base_x + base_w;                                      \
   cy = base_y;                                               \
   if (dx >= 0 || dy >= 0)                                    \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x1 = ev_x;                                       \
             y2 = (double)(y2 + (double)(dx * w_base_ratio)); \
          }                                                   \
        else                                                  \
          {                                                   \
             x1 = (double)(x1 - (double)(dy * h_base_ratio)); \
             y2 = ev_y;                                       \
          }                                                   \
     }                                                        \
   else                                                       \
     {                                                        \
        if (dx > dy)                                          \
          {                                                   \
             x1 = ev_x;                                       \
             y2 = y2 + (double)(dx * w_base_ratio);           \
          }                                                   \
        else                                                  \
          {                                                   \
             x1 = x1 - (double)(dy * h_base_ratio);           \
             y2 = ev_y;                                       \
          }                                                   \
     }                                                        \
}


#define CALCULATE_RESIZE_T(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                        \
   dy = y1 - ev_y;                                       \
   cx = base_x;                                          \
   cy = base_y + base_h;                                 \
   if (dy >= 0)                                          \
     {                                                   \
        x2 = (double)(x2 + (double)(dy * h_base_ratio)); \
        y1 = ev_y;                                       \
     }                                                   \
   else                                                  \
     {                                                   \
        x2 = x2 + (double)(dy * h_base_ratio);           \
        y1 = ev_y;                                       \
     }                                                   \
}


#define CALCULATE_RESIZE_L(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                        \
   dx = x1 - ev_x;                                       \
   cx = base_x + base_w;                                 \
   cy = base_y + base_h;                                 \
   if (dx >= 0)                                          \
     {                                                   \
        x1 = ev_x;                                       \
        y1 = (double)(y1 - (double)(dx * w_base_ratio)); \
     }                                                   \
   else                                                  \
     {                                                   \
        x1 = ev_x;                                       \
        y1 = y1 - (double)(dx * w_base_ratio);           \
     }                                                   \
}


#define CALCULATE_RESIZE_B(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                        \
   dy = ev_y - y2;                                       \
   cx = base_x + base_w;                                 \
   cy = base_y;                                          \
   if (dy >= 0)                                          \
     {                                                   \
        x1 = (double)(x1 - (double)(dy * h_base_ratio)); \
        y2 = ev_y;                                       \
     }                                                   \
   else                                                  \
     {                                                   \
        x1 = x1 - (double)(dy * h_base_ratio);           \
        y2 = ev_y;                                       \
     }                                                   \
}


#define CALCULATE_RESIZE_R(ev_x, ev_y, base_x, base_y, base_w, base_h, w_base_ratio, h_base_ratio, dx, dy, cx, cy, x1, y1, x2, y2) \
{                                                        \
   dx = ev_x - x2;                                       \
   cx = base_x;                                          \
   cy = base_y;                                          \
   if (dx >= 0)                                          \
     {                                                   \
        x2 = ev_x;                                       \
        y2 = (double)(y2 + (double)(dx * w_base_ratio)); \
     }                                                   \
   else                                                  \
     {                                                   \
        x2 = ev_x;                                       \
        y2 = y2 + (double)(dx * w_base_ratio);           \
     }                                                   \
}


#endif // _POLICY_UTIL_H
