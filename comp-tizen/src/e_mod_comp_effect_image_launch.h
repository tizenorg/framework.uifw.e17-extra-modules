#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_EFFECT_IMAGE_LAUNCH_H
#define E_MOD_COMP_EFFECT_IMAGE_LAUNCH_H

#ifdef _E_USE_EFL_ASSIST_
#include <efl_assist.h>
#endif

typedef enum _E_Fake_Effect_File_type
{
   E_FAKE_EFFECT_FILE_TYPE_EDJ = 0,
   E_FAKE_EFFECT_FILE_TYPE_IMAGE
} E_Fake_Effect_File_type;

typedef enum _E_Fake_Effect_Theme_type
{
   E_FAKE_EFFECT_THEME_DARK = 0,
   E_FAKE_EFFECT_THEME_LIGHT,
   E_FAKE_EFFECT_THEME_DEFAULT
} E_Fake_Effect_Theme_type;


typedef struct _E_Comp_Effect_Image_Launch E_Comp_Effect_Image_Launch;

struct _E_Comp_Effect_Image_Launch
{
   Eina_Bool       running : 1;
   Eina_Bool       fake_image_show_done : 1; // image launch edje object got effect done or not.
   Evas_Object    *obj;             // image object
   Evas_Object    *shobj;           // image shadow object
   Ecore_Timer    *timeout;         // max time between show, hide image launch
   Ecore_X_Window  win;             // this represent image launch effect's real window id.
   int             w, h;            // width and height of image object
   int             rot;             // rotation angle
   int             indicator_show;  // indicator enable / disable flag
   Evas_Object*    indicator_obj;   // plugin indicator object
   Evas           *evas;             // pointer for saving evas of canvas
   Ecore_Evas     *ecore_evas;      // pointer for saving ecore_evas of canvas

   //Changable UI variable
   E_Fake_Effect_File_type  file_type; //file type for fake effect
   E_Fake_Effect_Theme_type theme_type; //file type for fake effect
#ifdef _E_USE_EFL_ASSIST_
   Ea_Theme_Color_Table    *theme_table;//Default theme table
   Ea_Theme_Color_Table    *app_table;//Extra table that made by application
#endif
};

#endif
#endif
