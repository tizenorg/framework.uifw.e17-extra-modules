#include "e_mod_comp_shared_types.h"
#include "e_mod_comp_debug.h"
#include "e_mod_comp_effect_pos_animation.h"


/* local subsystem globals */

/* local subsystem functions */
static E_Comp_Effect_Ani_Style
_angle_dep_animation_open_get(int angle, E_Comp_Effect_Ani_Style ani)
{
   E_Comp_Effect_Ani_Style ani_dep_angle;
   ani_dep_angle = ani;

   switch (angle)
     {
      case 90:
         if(ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if((ani == E_COMP_EFFECT_ANIMATION_SLIDEUP) ||
                 (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         break;

      case 180:
         if(ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         else if((ani == E_COMP_EFFECT_ANIMATION_SLIDEUP) ||
                 (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         break;

      case 270:
         if(ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         else if((ani == E_COMP_EFFECT_ANIMATION_SLIDEUP) ||
                 (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         break;
     }

   return ani_dep_angle;
}

static E_Comp_Effect_Ani_Style
_angle_dep_animation_close_get(int angle, E_Comp_Effect_Ani_Style ani)
{
   E_Comp_Effect_Ani_Style ani_dep_angle;
   ani_dep_angle = ani;

   switch (angle)
     {
      case 90:
         if((ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN) ||
            (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDEUP)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         break;

      case 180:
         if((ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN) ||
            (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDEUP)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         break;

      case 270:
         if((ani == E_COMP_EFFECT_ANIMATION_SLIDEDOWN) ||
            (ani == E_COMP_EFFECT_ANIMATION_DEFAULT))
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDELEFT;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDELEFT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEUP;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDERIGHT)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
         else if(ani == E_COMP_EFFECT_ANIMATION_SLIDEUP)
           ani_dep_angle = E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
         break;
     }

   return ani_dep_angle;
}


/* externally accessible functions */
EINTERN E_Comp_Effect_Ani_Style
e_mod_comp_effect_pos_launch_type_get(const char *source)
{
   Ecore_X_Window xwin;
   int ang,x,y,w,h;

   E_CHECK_RETURN(source, E_COMP_EFFECT_ANIMATION_DEFAULT);

   // source format is WIN_ID.ROTATION_ANGLE.X.Y.W.H
   if (sscanf(source, "%x.%d.%d.%d.%d.%d",&xwin, &ang, &x, &y, &w, &h) == 6)
     {
        if (y < 0) return E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
        else if (y >= h) return E_COMP_EFFECT_ANIMATION_SLIDEUP;
        else if (x < 0) return E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
        else if (x >= w) return E_COMP_EFFECT_ANIMATION_SLIDELEFT;
     }
   else if(!strncmp(source , "noeffect", 8))
     {
        return E_COMP_EFFECT_ANIMATION_NONE;
     }

   return E_COMP_EFFECT_ANIMATION_DEFAULT;
}

EINTERN E_Comp_Effect_Ani_Style
e_mod_comp_effect_pos_close_type_get(const char *source)
{
   Ecore_X_Window xwin;
   int ang,x,y,w,h;

   E_CHECK_RETURN(source, E_COMP_EFFECT_ANIMATION_DEFAULT);

   // source format is WIN_ID.ROTATION_ANGLE.X.Y.W.H
   if (sscanf(source, "%x.%d.%d.%d.%d.%d",&xwin, &ang, &x, &y, &w, &h) == 6)
     {
	   if (y < 0) return E_COMP_EFFECT_ANIMATION_SLIDEUP;
	   else if (y >= h) return E_COMP_EFFECT_ANIMATION_SLIDEDOWN;
	   else if (x < 0) return E_COMP_EFFECT_ANIMATION_SLIDELEFT;
	   else if (x >= w) return E_COMP_EFFECT_ANIMATION_SLIDERIGHT;
     }

   return E_COMP_EFFECT_ANIMATION_DEFAULT;
}

EINTERN void
e_mod_comp_effect_pos_launch_make_emission(E_Comp_Win *cw, char *emission, int size)
{
   const char *sig = NULL;
   E_Zone *zone = NULL;
   E_Comp_Effect_Ani_Style ani_dep_angle;
   int sig_len = 0;

   ani_dep_angle = cw->eff_launch_style;

   if ((cw->bd) && (cw->bd->zone))
      zone = cw->bd->zone;

   if ((zone) && (zone->rot.curr))
      ani_dep_angle = _angle_dep_animation_open_get(zone->rot.curr, cw->eff_launch_style);

   switch (ani_dep_angle)
     {
      case E_COMP_EFFECT_ANIMATION_NONE:	   sig = "e,state,visible,on,noeffect";  break;
      case E_COMP_EFFECT_ANIMATION_SLIDEDOWN: sig = "e,state,visible,on,from0"; break; //from 0
      case E_COMP_EFFECT_ANIMATION_SLIDELEFT: sig = "e,state,visible,on,from270"; break; //from 270
      case E_COMP_EFFECT_ANIMATION_SLIDERIGHT: sig = "e,state,visible,on,from90";	 break; //from 90
      case E_COMP_EFFECT_ANIMATION_SLIDEUP:
      case E_COMP_EFFECT_ANIMATION_DEFAULT:
      default: sig = "e,state,visible,on";  break;//from 180
     }

   E_CHECK(sig);
   memset(emission, 0x0, size);
   sig_len = strlen(sig);

   if (sig_len >= size) memcpy(emission, sig, size-1);
   else memcpy(emission, sig, sig_len);

}

EINTERN void
e_mod_comp_effect_pos_close_make_emission(E_Comp_Win *cw, char *emission, int size)
{
   const char *sig = NULL;
   E_Zone *zone = NULL;
   E_Comp_Effect_Ani_Style ani_dep_angle;
   int sig_len = 0;

   ani_dep_angle = cw->eff_close_style;

   if ((cw->bd) && (cw->bd->zone))
      zone = cw->bd->zone;

   if ((zone) && (zone->rot.curr))
      ani_dep_angle = _angle_dep_animation_close_get(zone->rot.curr, cw->eff_close_style);

   switch (ani_dep_angle)
     {
      case E_COMP_EFFECT_ANIMATION_NONE:	  sig = "e,state,visible,off,noeffect";	break;
      case E_COMP_EFFECT_ANIMATION_SLIDEUP: sig = "e,state,visible,off,to0";	break;
      case E_COMP_EFFECT_ANIMATION_SLIDELEFT: sig = "e,state,visible,off,to90";	break;
      case E_COMP_EFFECT_ANIMATION_SLIDERIGHT: sig = "e,state,visible,off,to270";	break;
      case E_COMP_EFFECT_ANIMATION_SLIDEDOWN:
      case E_COMP_EFFECT_ANIMATION_DEFAULT:
      default: sig = "e,state,visible,off";	break; // to180
     }

   E_CHECK(sig);
   memset(emission, 0x0, size);
   sig_len = strlen(sig);

   if (sig_len >= size) memcpy(emission, sig, size-1);
   else memcpy(emission, sig, sig_len);
}

