/*
 * STRUCTS.h
 *
 *  Created on: Apr 2, 2020
 *      Author: Rifat
 */

#include <safecallhello.h>

typedef struct dataControl_data {

	char URLServer[0x100];
	int SOS_number;
	int GPS_TimeOut;
	int interval;

} dataControl_data;

typedef struct data_to_pass {

	Evas_Object *textbox;

} data_to_pass;

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *label;
	Evas_Object *layout;
	Evas_Object *RUN;
	Evas_Object *Settings;
	Evas_Object *SOSButton;
	Evas_Object *SendButton;
	Evas_Object *nf;
	Evas_Object *gps;
	Evas_Object *image;
	location_manager_h manager;
	data_control_h *provider_h, *vault_provider_h;

	dataControl_data Data_Settings;
	data_to_pass DataToPass;

} appdata_s;

struct _telephony_handle_list_s {
	unsigned int count;
	telephony_h *handle;
};
typedef struct _telephony_handle_list_s telephony_handle_list_ss;
telephony_handle_list_ss handle_list;
