#ifndef __EXTNDIALOG_H__
#define __EXTNDIALOG_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <utilX.h>
#include <utilX_ext.h>
#include <vconf.h>
#include <libintl.h>

#define DIALOG_ARGS_NUM 10
#define DIALOG_STR_LEN 128

#ifdef _
#undef _
#endif

#define _(s) extndialog_gettext(s)

#define _STRING_INIT(i, s) {                           \
   if (argc >= i) s = eina_stringshare_add(argv[i-1]); \
   else s = eina_stringshare_add("Not specified");     \
}

typedef struct
{
   char dialog_type[DIALOG_STR_LEN];
   char dialog_title[DIALOG_STR_LEN];
   char dialog_args[DIALOG_ARGS_NUM][DIALOG_STR_LEN];
   Evas_Object *dialog_win;
} ExtnDialogData;

char* extndialog_gettext(const char *s);
int extndialog_default_popup_add(ExtnDialogData *d_data);
int extndialog_hdmi_popup_add(ExtnDialogData *d_data);

#endif
