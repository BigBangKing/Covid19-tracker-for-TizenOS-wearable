#ifndef __safecallhello_H__
#define __safecallhello_H__

#include <app.h>
#include <Elementary.h>
#include <system_settings.h>
#include <efl_extension.h>
#include <dlog.h>

#include <locations.h>
#include <device/battery.h>
#include <telephony.h>
#include <Ecore.h>
#include <app_control.h>
#include <sensor.h>
#include <runtime_info.h>
#include <system_info.h>
#include <privacy_privilege_manager.h>

#include <data_control.h>

#include <sqlite3.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#define ICON_DIR "/opt/usr/apps/no.safecall.hello/res/images"

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "safecallhello"

#if !defined(PACKAGE)
#define PACKAGE "no.safecall.hello"
#endif

#endif /* __safecallhello_H__ */
