#include "safecallhello.h"
#include "STRUCTS.h"
#include "PermissionManager.h"
#include <app_preference.h>
#include <app_manager.h>

//#include <net_connection.h>
//#include <wifi.h>


//data sharing

/* Callback for the insert operation response */
void sql_insert_response_cb(int request_id, data_control_h provider,
		long long inserted_row_id, bool provider_result, const char *error,
		void *user_data) {
	if (provider_result) {
		dlog_print(DLOG_INFO, LOG_TAG, "The insert operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"The insert operation for the request %d failed. error message: %s",
				request_id, error);
	}
}

/* Callback for the select operation response */
void sql_select_response_cb(int request_id, data_control_h provider,
		result_set_cursor cursor, bool provider_result, const char *error,
		void *user_data) {
	appdata_s *ad = user_data;

	if (provider_result) {
		dlog_print(DLOG_INFO, LOG_TAG, "The select operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"The select operation for the request %d failed. error message: %s",
				request_id, error);
	}

	while (data_control_sql_step_next(cursor) == DATA_CONTROL_ERROR_NONE) {

		/*

		 char URL[255] = {0,};
		 int INTERVALTIME = 10;
		 int SOS = 0;
		 int GPSTIMEOUT = 0;
		 */

		data_control_sql_get_text_data(cursor, 0, ad->Data_Settings.URLServer);
		data_control_sql_get_int_data(cursor, 1, &(ad->Data_Settings.interval));
		data_control_sql_get_int_data(cursor, 2,
				&(ad->Data_Settings.SOS_number));
		data_control_sql_get_int_data(cursor, 3,
				&(ad->Data_Settings.GPS_TimeOut));

		dlog_print(DLOG_INFO, LOG_TAG,
				"URL: %s, INTERVAL: %d, SOS: %d, GPSTIMEOUT: %d ",
				ad->Data_Settings.URLServer, ad->Data_Settings.interval,
				ad->Data_Settings.SOS_number, ad->Data_Settings.GPS_TimeOut);
	}
}

/* Callback for the select operation response from vault service*/
void sql_select_vault_response_cb(int request_id, data_control_h provider,
		result_set_cursor cursor, bool provider_result, const char *error,
		void *user_data) {

	appdata_s *ad = user_data;

	if (provider_result) {
			dlog_print(DLOG_INFO, LOG_TAG, "The vault select operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "The select operation for the request %d failed. error message: %s",
					request_id, error);
		}


		while (data_control_sql_step_next(cursor) == DATA_CONTROL_ERROR_NONE) {

			 char REQUEST[255] = {0,};
			 int ID = 10;

			data_control_sql_get_int_data(cursor, 0, &ID);
			data_control_sql_get_text_data(cursor, 1, REQUEST);

			dlog_print(DLOG_INFO, LOG_TAG,
					"REQUEST: %s, ID: %d .", REQUEST, ID);
		}

}


/* Callback for the update operation response */
void sql_update_response_cb(int request_id, data_control_h provider,
		bool provider_result, const char *error, void *user_data) {
	if (provider_result) {
		dlog_print(DLOG_INFO, LOG_TAG, "The update operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"The update operation for the request %d failed. error message: %s",
				request_id, error);
	}
}

/* Callback for the delete operation response */
void sql_delete_response_cb(int request_id, data_control_h provider,
		bool provider_result, const char *error, void *user_data) {
	if (provider_result) {
		dlog_print(DLOG_INFO, LOG_TAG, "The delete operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG,
				"The delete operation for the request %d failed. error message: %s",
				request_id, error);
	}
}

data_control_sql_response_cb sql_callback, sql_vault_callback;

void initialize_datacontrol_consumer(appdata_s *ad) {
	int ret;

	const char *provider_id =
			"http://datacontrolservice.com/datacontrol/provider/datacontrolservice";
	const char *data_id = "Dictionary";

	/* Create data control handler */
	ret = data_control_sql_create(&(ad->provider_h));
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG,
				"creating data control provider failed with error: %d", ret);

	ret = data_control_sql_set_provider_id(ad->provider_h, provider_id);
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG,
				"setting provider id failed with error: %d", ret);

	ret = data_control_sql_set_data_id(ad->provider_h, data_id);
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d",
				ret);

	/* Set response callbacks */
	sql_callback.delete_cb = sql_delete_response_cb;
	sql_callback.insert_cb = sql_insert_response_cb;
	sql_callback.select_cb = sql_select_response_cb;
	sql_callback.update_cb = sql_update_response_cb;

	/* Register response callbacks */
	ret = data_control_sql_register_response_cb(ad->provider_h, &sql_callback,
			ad);
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG,
				"Registering the callback function failed with error: %d", ret);

	dlog_print(DLOG_INFO, LOG_TAG, "Init data control success");

	const char * key = "FirstRun";
	const char *integer_key = "FailedAttempts";
	bool existing;

	preference_is_existing(key, &existing);

	int req_id = 0;

	if (existing == false) {

		preference_set_boolean(key, existing);

		dlog_print(DLOG_INFO, LOG_TAG, "it is first run");

		//Send a request to insert a row
		bundle *b = bundle_create();
		bundle_add_str(b, "URL",
				"'https://server.safecall.no/Webservices/PositioningTizen.ashx?'");
		bundle_add_str(b, "INTERVALTIME", "300");
		bundle_add_str(b, "SOS", "123");
		bundle_add_str(b, "GPSTIMEOUT", "30");

		data_control_sql_insert(ad->provider_h, b, &req_id);

		//Free memory
		bundle_free(b);
		/*
		 //Send a request to select a row
		 char *column_list[4];
		 column_list[0] = "URL";
		 column_list[1] = "INTERVALTIME";
		 column_list[2] = "SOS";
		 column_list[3] = "GPSTIMEOUT";

		 const char *where = "1";
		 const char *order = "(SELECT NULL)";

		 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);



		 //Send a request to add a row
		 bundle_add_str(b, "URL", "'www.money.com'");
		 bundle_add_str(b, "INTERVALTIME", "999");
		 data_control_sql_update(ad->provider_h, b, where, &req_id);


		 //Send a request to delete a row
		 const char *where_delete = "WORD = 'test'";
		 int result = data_control_sql_delete(ad->provider_h, where_delete, &req_id);

		//Free memory
		bundle_free(b);
		 */


		/*// Insert data to the Dictionary table
		    ret = data_control_sql_set_data_id(ad->provider_h, "LocationVault");
		    if (ret == DATA_CONTROL_ERROR_NONE){

		    b = bundle_create();
		    bundle_add_str(b, "0", "'test data from locv'");
		    data_control_sql_insert(ad->provider_h, b, &req_id);
		    bundle_free(b);


		    }else {
		    	dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);
			}*/

	} else {

		dlog_print(DLOG_INFO, LOG_TAG, "not first run");
	}

	//after checks of firstRun we will populate our variables from database controller.
	 ret = data_control_sql_set_data_id(ad->provider_h, data_id);
			    if (ret == DATA_CONTROL_ERROR_NONE){
			    	/* Send a request to select a row */
			    		char *column_list[4];
			    		column_list[0] = "URL";
			    		column_list[1] = "INTERVALTIME";
			    		column_list[2] = "SOS";
			    		column_list[3] = "GPSTIMEOUT";

			    		const char *where = "1";
			    		const char *order = "(SELECT NULL)";

			    		data_control_sql_select(ad->provider_h, column_list, 4, where, order,
			    				&req_id);

			    }else {
			    	dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);
				}




}

//vault service
void initialize_vault_datacontrol_consumer(appdata_s *ad) {
	int ret;
	const char *provider_id =
			"http://vaultservice.com/datacontrol/provider/vaultservice";

	const char *data_id = "Vault";

		/* Create data control handler */
		ret = data_control_sql_create(&(ad->vault_provider_h));
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"creating data control provider failed with error: %d", ret);

		ret = data_control_sql_set_provider_id(ad->vault_provider_h, provider_id);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"setting provider id failed with error: %d", ret);

		ret = data_control_sql_set_data_id(ad->vault_provider_h, data_id);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d",
					ret);

		/* Set response callbacks */
		sql_callback.delete_cb = sql_delete_response_cb;
		sql_callback.insert_cb = sql_insert_response_cb;
		sql_callback.select_cb = sql_select_vault_response_cb ;
		sql_callback.update_cb = sql_update_response_cb;

		/* Register response callbacks */
		ret = data_control_sql_register_response_cb(ad->vault_provider_h, &sql_callback,
				ad);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"Registering the callback function failed with error: %d", ret);

		dlog_print(DLOG_INFO, LOG_TAG, "Init data control success");

		//Send a request to insert a row

		int req_id = 0;
		bundle *b = bundle_create();
		bundle_add_str(b, "REQUEST",
							"'https://server.safecall.no/Webservices/PositioningTizen.ashx?'");
		bundle_add_str(b, "ID", "1");

		data_control_sql_insert(ad->vault_provider_h, b, &req_id);

		//Free memory
		bundle_free(b);

		//after checks of firstRun we will populate our variables from database controller.
		ret = data_control_sql_set_data_id(ad->vault_provider_h, data_id);
		if (ret == DATA_CONTROL_ERROR_NONE){
		 /* Send a request to select a row */
			char *column_list[2];
			column_list[0] = "ID";
			column_list[1] = "REQUEST";

			const char *where = "1";
			const char *order = "(SELECT NULL)";

			data_control_sql_select(ad->vault_provider_h, column_list, 2, where, order, &req_id);

			}
		else {

			dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

		}


}





static void DATA_updater(void *data, char *what, char *value) {

	appdata_s *ad = data;

	int req_id = 0;
	bundle *b;
	int ret;
	char *where = "1";

	//dlog_print(DLOG_DEBUG, LOG_TAG, "we got, what: %s, and value: %s", what, value);

	//Insert data to the Dictionary table
	ret = data_control_sql_set_data_id(ad->provider_h, "Dictionary");
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d",
				ret);

	b = bundle_create();
	bundle_add_str(b, what, value);
	data_control_sql_update(ad->provider_h, b, where, &req_id);
	bundle_free(b);

}

static void UPDATE_GPSTIMEOUT_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {

	int req_id = 0;
	int ret;
	char *where = "1";

	dlog_print(DLOG_INFO, LOG_TAG, "current text is: %s",
			elm_object_text_get(ad->DataToPass.textbox));

	DATA_updater(ad, "'GPSTIMEOUT'",
			elm_object_text_get(ad->DataToPass.textbox));

	/* char *column_list[4];
	 column_list[0] = "URL";
	 column_list[1] = "INTERVALTIME";
	 column_list[2] = "SOS";
	 column_list[3] = "GPSTIMEOUT";


	 char *order = "(SELECT NULL)";

	 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);
	 */
}

static void UPDATE_SOSNumber_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {

	int req_id = 0;
	int ret;
	char *where = "1";

	dlog_print(DLOG_INFO, LOG_TAG, "current text is: %s",
			elm_object_text_get(ad->DataToPass.textbox));

	DATA_updater(ad, "'SOS'", elm_object_text_get(ad->DataToPass.textbox));

	/*

	 char *column_list[4];
	 column_list[0] = "URL";
	 column_list[1] = "INTERVALTIME";
	 column_list[2] = "SOS";
	 column_list[3] = "GPSTIMEOUT";


	 char *order = "(SELECT NULL)";

	 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);
	 */

}

static void UPDATE_INTERVAL_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {

	int req_id = 0;
	int ret;
	char *where = "1";

	dlog_print(DLOG_INFO, LOG_TAG, "current text is: %s",
			elm_object_text_get(ad->DataToPass.textbox));

	DATA_updater(ad, "'INTERVALTIME'",
			elm_object_text_get(ad->DataToPass.textbox));

	/*char *column_list[4];
	 column_list[0] = "URL";
	 column_list[1] = "INTERVALTIME";
	 column_list[2] = "SOS";
	 column_list[3] = "GPSTIMEOUT";


	 char *order = "(SELECT NULL)";

	 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);
	 */
}

static void UPDATE_URL_cb(appdata_s *ad, Evas_Object *obj, void *event_info) {

	int req_id = 0;
	int ret;
	char *where = "1";
	char *sub[0x100];
	char *str = elm_object_text_get(ad->DataToPass.textbox);
	/*
	 //<font_size=15> & </font_size>
	 int position = 15, length =  0;
	 while (length < 255) {
	 if (str[length] == '\0') {
	 break;
	 }
	 length++;
	 }
	 length = length - (11+position);*/
	//dlog_print(DLOG_INFO, LOG_TAG, "current text is: %s, size is: %d", str, length);
	//substring(str, sub, position, length);

	snprintf(sub, sizeof(sub), "'%s'", elm_entry_markup_to_utf8(str));

	//DATA_updater(ad, "'URL'", str);
	DATA_updater(ad, "'URL'", sub);

	//dlog_print(DLOG_INFO, LOG_TAG, "current text is: %s", sub);

	/*char *column_list[4];
	 column_list[0] = "URL";
	 column_list[1] = "INTERVALTIME";
	 column_list[2] = "SOS";
	 column_list[3] = "GPSTIMEOUT";


	 char *order = "(SELECT NULL)";

	 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);
	 */
}

static void addData_cb(void *data, Evas_Object *obj, void *event_info) {

	appdata_s *ad = data;

	int req_id = 0;
	bundle *b;
	int ret;
	char *where = "1";

	dlog_print(DLOG_INFO, LOG_TAG,
			"URL: %s, INTERVAL: %d, SOS: %d, GPSTIMEOUT: %d ",
			ad->Data_Settings.URLServer, ad->Data_Settings.interval,
			ad->Data_Settings.SOS_number, ad->Data_Settings.GPS_TimeOut);

	dlog_print(DLOG_INFO, LOG_TAG, "calling that func");

	/*
	 //Insert data to the Dictionary table
	 ret = data_control_sql_set_data_id(ad->provider_h, "Dictionary");
	 if (ret != DATA_CONTROL_ERROR_NONE)
	 dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

	 b = bundle_create();
	 bundle_add_str(b, "URL", "'www.money.com'");
	 //data_control_sql_insert(ad->provider_h, b, &req_id);
	 data_control_sql_update(ad->provider_h, b, where, &req_id);
	 bundle_free(b);



	 char *column_list[4];
	 column_list[0] = "URL";
	 column_list[1] = "INTERVALTIME";
	 column_list[2] = "SOS";
	 column_list[3] = "GPSTIMEOUT";


	 char *order = "(SELECT NULL)";

	 data_control_sql_select(ad->provider_h, column_list, 4, where, order, &req_id);
	 */

}

//data sharing code ends

//DATA CHANGE NOTI

/* Triggered when the data change notification arrives */
void data_change_cb(data_control_h provider,
		data_control_data_change_type_e type, bundle *data, void *user_data) {
	appdata_s *ad = user_data;

	char *column_list[4];
	column_list[0] = "URL";
	column_list[1] = "INTERVALTIME";
	column_list[2] = "SOS";
	column_list[3] = "GPSTIMEOUT";

	char *where = "1";
	int req_id = 0;
	char *order = "(SELECT NULL)";

	data_control_sql_select(ad->provider_h, column_list, 4, where, order,
			&req_id);

	/* char *word;
	 char *word_desc;
	 char *word_num;

	 bundle_get_str(data, "WORD", &word);
	 bundle_get_str(data, "WORD_DESC", &word_desc);
	 bundle_get_str(data, "WORD_NUM", &word_num);
	 dlog_print(DLOG_INFO, LOG_TAG, "%d type noti, changed data: %s, %s, %s",
	 type, word, word_desc, word_num);*/
	//dlog_print(DLOG_INFO, LOG_TAG, "data change / update callback...%d type noti....... %s %s %s %s", type, URL,INTERVALTIME ,SOS , GPSTIMEOUT);
}

/* Triggered when the provider has accepted the callback registration */
void result_cb(data_control_h provider, data_control_error_e result,
		int callback_id, void *user_data) {
	dlog_print(DLOG_INFO, LOG_TAG, "Add data change callback RESULT: %d",
			result);
}

/* Register the callback //, Evas_Object *obj, void *event_info */
int cb_id;
void add_data_change_cb_func(void *data) {
	appdata_s *ad = (appdata_s *) data;
	int ret = data_control_add_data_change_cb(ad->provider_h, data_change_cb,
			ad, result_cb, NULL, &cb_id);
	if (ret != DATA_CONTROL_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG,
				"add data change callback failed with error: %d", ret);
	dlog_print(DLOG_INFO, LOG_TAG, "add data change callback done: %d", cb_id);
}

/* Remove the callback */
void remove_data_change_cb_func(void *data) {
	appdata_s *ad = (appdata_s *) data;
	data_control_remove_data_change_cb(ad->provider_h, cb_id);
	dlog_print(DLOG_INFO, LOG_TAG, "remove data change callback done: %d",
			cb_id);
}

//DATA CHANGE NOTI ENDS

//base works...........

#define EXPANDFILL(x) { \
	   evas_object_size_hint_weight_set(x, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND); \
	   evas_object_size_hint_align_set(x, EVAS_HINT_FILL, EVAS_HINT_FILL); }

static void win_delete_request_cb(void *data, Evas_Object *obj,
		void *event_info) {
	ui_app_exit();
}

static void win_back_cb(void *data, Evas_Object *obj, void *event_info) {
	appdata_s *ad = data;
	dlog_print(DLOG_INFO, LOG_TAG,
			"Wearable Back Button Event Callback Triggered.");
	/* Let window go to hide state. ✓ */
	elm_win_lower(ad->win);
}

static void prev_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info) {
	Evas_Object *nf = data;
	elm_naviframe_item_pop(nf);
}

//service related

static int launch_service() {
	app_control_h app_control = NULL;
	if (app_control_create(&app_control) == APP_CONTROL_ERROR_NONE) {
		if ((app_control_set_app_id(app_control,
				"no.safecall.positioningservice") == APP_CONTROL_ERROR_NONE)
				&& (app_control_send_launch_request(app_control, NULL, NULL)
						== APP_CONTROL_ERROR_NONE)) {
			dlog_print(DLOG_INFO, LOG_TAG, "App launch request sent!");
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG,
					"App launch request sending failed!");
			return 0;
		}
		if (app_control_destroy(app_control) == APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_INFO, LOG_TAG, "App control destroyed.");
		}
		// We exit our launcher app, there is no point in keeping it open.
		//ui_app_exit();
		return 1;
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "App control creation failed!");
		return 0;
	}
}

static void stop_service() {
	app_control_h app_control = NULL;
	if (app_control_create(&app_control) == APP_CONTROL_ERROR_NONE) {
		if ((app_control_set_app_id(app_control,
				"no.safecall.positioningservice") == APP_CONTROL_ERROR_NONE)
				&& (app_control_add_extra_data(app_control, "service_action",
						"stop") == APP_CONTROL_ERROR_NONE)
				&& (app_control_send_launch_request(app_control, NULL, NULL)
						== APP_CONTROL_ERROR_NONE)) {
			dlog_print(DLOG_INFO, LOG_TAG, "App launch request sent!");
		} else {
			dlog_print(DLOG_ERROR, LOG_TAG,
					"App launch request sending failed!");
		}
		if (app_control_destroy(app_control) == APP_CONTROL_ERROR_NONE) {
			dlog_print(DLOG_INFO, LOG_TAG, "App control destroyed.");
		}
		//ui_app_exit();
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "App control creation failed!");
	}
}

//service stuff ends

static void stop_and_back_cb(void *data, Evas_Object *obj, void *event_info) {

	stop_service();

	Evas_Object *nf = data;
	elm_naviframe_item_pop(nf);
}

//splash screen

static Eina_Bool SplashPopper(void *data EINA_UNUSED) {
	Evas_Object *nf = data;

	elm_naviframe_item_pop(nf);

	return ECORE_CALLBACK_CANCEL;
}

static void ShowSplashScreen(void *data) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = data;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 51, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	Evas_Object *image = elm_image_add(bg);
	evas_object_move(image, 0, 0);
	evas_object_resize(image, 400, 300);
	evas_object_show(image);

	char img_path[128];
	char *res_path = app_get_resource_path();
	snprintf(img_path, sizeof(img_path), "%s%s%s", res_path, "images/",
			"safecall_pos_logo_app_trans.png");

	elm_image_file_set(image, img_path, NULL);

	//elm_box_pack_end(box, image);

	//show the app version.

		label = elm_label_add(box);
		elm_object_text_set(label,
				"<align=center> <font_size=25>V: 0.3</font_size></align>");
		//evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
		//evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(180), ELM_SCALE_SIZE(100));
		//EXPANDFILL(label);
		evas_object_size_hint_weight_set(label, 1.0, 1.0);
		evas_object_size_hint_align_set(label, -1.0, 1.0);
		elm_label_line_wrap_set(label, ELM_WRAP_WORD);
		//elm_object_content_set(box, ad->label);
		evas_object_show(label);
		elm_box_pack_end(box, label);

		/*Evas_Object *spacer1 = evas_object_rectangle_add(box);
		evas_object_size_hint_weight_set(spacer1, 1.0, 1.0);
		evas_object_size_hint_align_set(spacer1, -1.0, 1.0);
		evas_object_size_hint_min_set(spacer1, 0, 10);
		elm_box_pack_end(box, spacer1);*/

	elm_naviframe_item_push(nf, "Background", NULL, NULL, bg, "empty");
	//----END

	double d = 2.0;

	Ecore_Timer *timer1;

	if (!ecore_init()) {
		printf("ERROR: Cannot init Ecore!\n");
	}

	else {

		timer1 = ecore_timer_add(d, SplashPopper, nf);

	}

}

//splash screen end

//Window Run.

static void btn_RunningInBackground_cb(void *data, Evas_Object *obj,
		void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = data;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label,
			"<align=center>The background service is starting.</align>");
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(180),
			ELM_SCALE_SIZE(100));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	//elm_object_content_set(box, ad->label);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	/*	Evas_Object *button;
	 button = elm_button_add(box);
	 elm_object_text_set(button, "OK");
	 evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
	 EVAS_HINT_EXPAND);
	 evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(80),
	 ELM_SCALE_SIZE(40));
	 evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, nf);
	 elm_object_style_set(button, "nextdepth");
	 evas_object_show(button);
	 elm_box_pack_end(box, button);
	 */

	Evas_Object *image = elm_image_add(bg);
	evas_object_size_hint_weight_set(image, 1.0, 0.0);
	evas_object_size_hint_align_set(image, 0.0, -1.0);
	evas_object_smart_callback_add(image, "clicked", prev_btn_clicked_cb, nf);
	EXPANDFILL(image);
	evas_object_show(image);
	elm_image_file_set(image, ICON_DIR"/okfin.png", NULL);
	elm_box_pack_end(box, image);

	elm_naviframe_item_pop(nf);

	elm_naviframe_item_push(nf, "Background", NULL, NULL, bg, "empty");
	//----END

	int x = launch_service();
	if (x == 1) {
		elm_object_text_set(label,
				"<align=center>The service application now is running.</align>");
	} else {

		elm_object_text_set(label,
				"<align=center>Could not start The background service.</align>");
	}

}

static void btn_RUN_clicked_cb(void *data, Evas_Object *obj, void *event_info) {
	appdata_s *ad = data;

	Evas_Object *bg;
	bg = elm_bg_add(ad->nf);
	elm_bg_color_set(bg, 0, 0, 0);

	eext_object_event_callback_add(ad->nf, EEXT_CALLBACK_BACK,
			prev_btn_clicked_cb, ad);

	Evas_Object *box = elm_box_add(bg);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_horizontal_set(box, EINA_TRUE);
	//elm_box_padding_set(box, 0, 5 * elm_config_scale_get());

	elm_object_content_set(ad->conform, box);
	evas_object_show(box);


		Evas_Object *tab = elm_table_add(ad->nf);
		evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
		EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.5);
		evas_object_show(tab);
		elm_box_pack_end(box, tab);


		Evas_Object *button;
		button = elm_button_add(tab);
		//evas_object_size_hint_weight_set(button, EVAS_HINT_FILL, EVAS_HINT_FILL);
		//evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(70),ELM_SCALE_SIZE(40));
		EXPANDFILL(button);
		elm_object_style_set(button, "nextdepth");
		evas_object_show(button);
		elm_table_pack(tab, button, 0, 0, 1, 1);



	/*Evas_Object *image = elm_image_add(bg);
	evas_object_size_hint_weight_set(image, 1.0, 0.0);
	evas_object_size_hint_align_set(image, 0.0, -1.0);
	//evas_object_size_hint_min_set(image, ELM_SCALE_SIZE(60),0);
	EXPANDFILL(image);
	evas_object_show(image);
	elm_box_pack_end(box, image);*/

	//char img_path[128];
	//char *res_path = app_get_resource_path();
	//snprintf(img_path, sizeof(img_path), "%s%s%s", res_path, "images/", "ok.png");


	Evas_Object *label = elm_label_add(bg);

	//evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	//evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(150), EVAS_HINT_EXPAND);
	EXPANDFILL(label);
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(150), EVAS_HINT_EXPAND);
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	//elm_box_pack_end(box, label);
	elm_table_pack(tab, label, 1, 0, 1, 1);

	/*
	 Evas_Object *button1 = elm_button_add(bg);
	 evas_object_size_hint_weight_set(button1, 1.0,0.0);
	 evas_object_size_hint_align_set(button1, 1.0,-1.0);
	 evas_object_smart_callback_add(button1, "clicked", stop_and_back_cb,
	 ad->nf);
	 //evas_object_size_hint_min_set(button1, ELM_SCALE_SIZE(60),0);
	 EXPANDFILL(button1);
	 evas_object_show(button1);
	 elm_box_pack_end(box,button1);

	 Evas_Object *ic1;
	 ic1 = elm_icon_add(button1);
	 elm_image_file_set(ic1,ICON_DIR"/no.png",NULL);
	 elm_object_part_content_set(button1,"icon",ic1);
	 evas_object_show(ic1);
	 */

	/*Evas_Object *image1 = elm_image_add(bg);
	evas_object_size_hint_weight_set(image1, 1.0, 0.0);
	evas_object_size_hint_align_set(image1, 1.0, -1.0);
	EXPANDFILL(image1);
	elm_box_pack_end(box, image1);*/

	Evas_Object *button1;
	button1 = elm_button_add(box);
	elm_object_text_set(button1, "X");
	//evas_object_size_hint_weight_set(button1, EVAS_HINT_FILL, EVAS_HINT_FILL);
	//evas_object_size_hint_min_set(button1, ELM_SCALE_SIZE(70),ELM_SCALE_SIZE(40));
	EXPANDFILL(button1);
	evas_object_smart_callback_add(button1, "clicked", prev_btn_clicked_cb, ad->nf);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);


	Evas_Object *ic;
	ic = elm_icon_add(tab);

	bool running;
	char *app_id = "no.safecall.positioningservice";// hard code is enough and ok.
	if (app_manager_is_running(app_id, &running) == APP_MANAGER_ERROR_NONE) {
		if (running) {
			dlog_print(DLOG_DEBUG, LOG_TAG, "it is running...");
			elm_object_text_set(label,
					"<align=center>Service is running in background.</align>");

			evas_object_smart_callback_add(button1, "clicked",
								prev_btn_clicked_cb, ad->nf);
			evas_object_smart_callback_add(button, "clicked", stop_and_back_cb,
								ad->nf);


			elm_object_text_set(button, "<font_size=12>STOP</font_size>");
			elm_object_text_set(button1, "✓");

			/*
			evas_object_smart_callback_add(image1, "clicked",
					prev_btn_clicked_cb, ad->nf);
			evas_object_smart_callback_add(image, "clicked", stop_and_back_cb,
					ad->nf);

			evas_object_show(image);
			elm_image_file_set(image, ICON_DIR"/stop.png", NULL);
			elm_box_pack_end(box, image);

			evas_object_show(image1);
			elm_image_file_set(image1, ICON_DIR"/okfin.png", NULL);
			 */

			/*elm_image_file_set(ic, ICON_DIR"/nofin.png", NULL);
			elm_object_part_content_set(button, "icon", ic);
			evas_object_show(ic);
			*/

		} else {

			evas_object_smart_callback_add(button1, "clicked",
					btn_RunningInBackground_cb, ad->nf);
			evas_object_smart_callback_add(button, "clicked",
					prev_btn_clicked_cb, ad->nf);
			dlog_print(DLOG_DEBUG, LOG_TAG, "it is not running...");
			elm_object_text_set(label,
					"<align=center>Run service in background?</align>");
			/*
			elm_image_file_set(image, ICON_DIR"/nofin.png", NULL);


			evas_object_show(image1);
			elm_image_file_set(image1, ICON_DIR"/okfin.png", NULL);
			 */

			elm_object_text_set(button, "X");

			elm_object_text_set(button1, "✓");


		}
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "... Error");

	}

	//----END

	elm_object_content_set(bg, box);
	elm_naviframe_item_push(ad->nf, NULL, NULL, NULL, bg, "empty");
	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}
//Window Run End.

//Window SOS

//void Make_SOS_Call(char* number) {

static void Make_SOS_Call(void *data, Evas_Object *obj, void *event_info) {

	app_check_and_request_call_permission(data);

	appdata_s *ad = data;

	char tel[0x18];
	snprintf(tel, sizeof(tel), "tel:%d", ad->Data_Settings.SOS_number);

	//dlog_print(DLOG_INFO, LOG_TAG, "URL: %s, INTERVAL: %d, SOS: %d, GPSTIMEOUT: %d ", ad->Data_Settings.URLServer, ad->Data_Settings.interval, ad->Data_Settings.SOS_number, ad->Data_Settings.GPS_TimeOut);

	app_control_h request = NULL;
	app_control_create(&request);

	app_control_set_operation(request, APP_CONTROL_OPERATION_CALL);
	app_control_set_uri(request, tel);

	//app_control_send_launch_request(request, lauchCallCallBack, data);
	if (app_control_send_launch_request(request, NULL, NULL)
			== APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "starting call");
		//elm_object_text_set(ad->label, "<align=center>starting call</align>");
	}

	app_control_destroy(request);
}

static void btn_URLSERVERSettings_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = ad->nf;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label, "<align=center>URL SERVER</align>");
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(90),
			ELM_SCALE_SIZE(40));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *tab = elm_table_add(bg);
	evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.3);
	evas_object_show(tab);
	elm_box_pack_end(box, tab);

	Evas_Object *button;
	button = elm_button_add(box);
	elm_object_text_set(button, "X");
	/* evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
	 EVAS_HINT_EXPAND);
	 evas_object_size_hint_min_set(button, EVAS_HINT_FILL,
	 ELM_SCALE_SIZE(60));*/
	EXPANDFILL(button);
	//elm_object_style_set(button, "editfield_clear");
	evas_object_show(button);
	elm_table_pack(tab, button, 0, 0, 1, 1);

	/*Evas_Object *ic1;
	ic1 = elm_icon_add(button);
	elm_image_file_set(ic1, ICON_DIR"/ok3.png", NULL);
	elm_object_part_content_set(button, "icon", ic1);
	evas_object_show(ic1);
*/
	Evas_Object *bg1 = evas_object_rectangle_add(tab);
	EXPANDFILL(bg1);
	evas_object_color_set(bg1, 255, 255, 255, 255);
	elm_table_pack(tab, bg1, 1, 0, 1, 1);
	evas_object_show(bg1);

	Evas_Object *entry;
	entry = elm_entry_add(tab);
	EXPANDFILL(entry);
	evas_object_show(entry);
	elm_table_pack(tab, entry, 1, 0, 1, 1);
	evas_object_color_set(entry, 100, 100, 200, 255);

	char setOnEntry[0x100] = { 0 };
	snprintf(setOnEntry, sizeof(setOnEntry), "<font_size=15>%s</font_size>",
			ad->Data_Settings.URLServer);
	elm_entry_entry_set(entry, setOnEntry);
	dlog_print(DLOG_DEBUG, LOG_TAG, setOnEntry);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	elm_object_text_set(button1, "✓");
	EXPANDFILL(button1);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);

	/*Evas_Object *ic;
	ic = elm_icon_add(button1);
	elm_image_file_set(ic, ICON_DIR"/no.png", NULL);
	elm_object_part_content_set(button1, "icon", ic);
	evas_object_show(ic);
*/
	elm_naviframe_item_push(nf, "URL SERVER", NULL, NULL, bg, "empty");

	ad->DataToPass.textbox = entry;

	evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, nf);
	evas_object_smart_callback_add(button1, "clicked", UPDATE_URL_cb, ad);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}

static void btn_GPSTIMEOUTSettings_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;

	Evas_Object *nf = ad->nf;

	//Evas_Object *conform = elm_conformant_add(nf);
	//elm_win_resize_object_add(nf, conform);
	//Evas_Object *layout = elm_layout_add(conform);
	//elm_object_content_set(conform, layout);

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label,
			"<align=center><font_size=25>GPS TIMEOUT</font_size></align>");
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(90),
			ELM_SCALE_SIZE(40));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *tab = elm_table_add(nf);
	evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.4);
	evas_object_show(tab);
	elm_box_pack_end(box, tab);

	Evas_Object *button;
	button = elm_button_add(box);
	//elm_object_text_set(button, "OK");
	/*evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
	 EVAS_HINT_EXPAND);
	 evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(60),
	 ELM_SCALE_SIZE(40));*/
	EXPANDFILL(button);
	elm_object_style_set(button, "nextdepth");
	evas_object_show(button);
	elm_table_pack(tab, button, 0, 0, 1, 1);

	Evas_Object *ic;
	ic = elm_icon_add(button);
	elm_image_file_set(ic, ICON_DIR"/nofin.png", NULL);
	elm_object_part_content_set(button, "icon", ic);
	evas_object_show(ic);

	Evas_Object *bg1 = evas_object_rectangle_add(tab);
	EXPANDFILL(bg1);
	evas_object_color_set(bg1, 255, 255, 255, 255);
	elm_table_pack(tab, bg1, 1, 0, 1, 1);
	evas_object_show(bg1);

	Evas_Object *entry;
	entry = elm_entry_add(tab);
	EXPANDFILL(entry);
	evas_object_show(entry);
	elm_table_pack(tab, entry, 1, 0, 1, 1);
	evas_object_color_set(entry, 100, 100, 200, 255);

	char setOnEntry[0x14] = { 0 };
	snprintf(setOnEntry, sizeof(setOnEntry), "%d",
			ad->Data_Settings.GPS_TimeOut);
	elm_entry_entry_set(entry, setOnEntry);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	//elm_object_text_set(button1, "NO");
	EXPANDFILL(button1);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);

	Evas_Object *ic1;
	ic1 = elm_icon_add(button1);
	elm_image_file_set(ic1, ICON_DIR"/okfin.png", NULL);
	elm_object_part_content_set(button1, "icon", ic1);
	evas_object_show(ic1);

	elm_naviframe_item_push(nf, "SETTINGS", NULL, NULL, bg, "empty");

	ad->DataToPass.textbox = entry;

	evas_object_smart_callback_add(button1, "clicked", UPDATE_GPSTIMEOUT_cb, ad);
	evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, nf);
	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);

}

static void btn_SOSSettings_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = ad->nf;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label, "<align=center>SOS</align>");

	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(90),
			ELM_SCALE_SIZE(10));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *tab = elm_table_add(bg);
	evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.4);
	evas_object_show(tab);
	elm_box_pack_end(box, tab);

	Evas_Object *button;
	button = elm_button_add(box);
	//elm_object_text_set(button, "OK");
	/*evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
	 EVAS_HINT_EXPAND);
	 evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(60),
	 ELM_SCALE_SIZE(40));*/
	EXPANDFILL(button);

	elm_object_style_set(button, "nextdepth");
	evas_object_show(button);
	elm_table_pack(tab, button, 0, 0, 1, 1);

	Evas_Object *ic;
	ic = elm_icon_add(button);
	elm_image_file_set(ic, ICON_DIR"/nofin.png", NULL);
	elm_object_part_content_set(button, "icon", ic);
	evas_object_show(ic);

	Evas_Object *bg1 = evas_object_rectangle_add(tab);
	EXPANDFILL(bg1);
	evas_object_color_set(bg1, 255, 255, 255, 255);
	elm_table_pack(tab, bg1, 1, 0, 1, 1);
	evas_object_show(bg1);

	Evas_Object *entry;
	entry = elm_entry_add(tab);
	EXPANDFILL(entry);
	evas_object_show(entry);
	elm_table_pack(tab, entry, 1, 0, 1, 1);
	evas_object_color_set(entry, 100, 100, 200, 255);

	char setOnEntry[0x14] = { 0 };
	snprintf(setOnEntry, sizeof(setOnEntry), "%d",
			ad->Data_Settings.SOS_number);
	elm_entry_entry_set(entry, setOnEntry);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	//elm_object_text_set(button1, "NO");
	EXPANDFILL(button1);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);

	Evas_Object *ic1;
	ic1 = elm_icon_add(button1);
	elm_image_file_set(ic1, ICON_DIR"/okfin.png", NULL);
	elm_object_part_content_set(button1, "icon", ic1);
	evas_object_show(ic1);

	elm_naviframe_item_push(nf, "SOS SETTINGS", NULL, NULL, bg, "empty");

	ad->DataToPass.textbox = entry;


	evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, nf);
	evas_object_smart_callback_add(button1, "clicked", UPDATE_SOSNumber_cb, ad);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}

//
//Winodw settings/Interval

static void btn_SETTINGSInterval_cb(appdata_s *ad, Evas_Object *obj,
		void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = ad->nf;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label, "<align=center>Interval</align>");

	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(90),
			ELM_SCALE_SIZE(10));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *tab = elm_table_add(bg);
	evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.4);
	evas_object_show(tab);
	elm_box_pack_end(box, tab);

	Evas_Object *button;
	button = elm_button_add(box);
	//elm_object_text_set(button, "OK");
	evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(60),
			ELM_SCALE_SIZE(40));
	EXPANDFILL(button);
	elm_object_style_set(button, "nextdepth");
	evas_object_show(button);
	elm_table_pack(tab, button, 0, 0, 1, 1);

	Evas_Object *ic;
	ic = elm_icon_add(button);
	elm_image_file_set(ic, ICON_DIR"/nofin.png", NULL);
	elm_object_part_content_set(button, "icon", ic);
	evas_object_show(ic);

	Evas_Object *bg1 = evas_object_rectangle_add(tab);
	EXPANDFILL(bg1);
	evas_object_color_set(bg1, 255, 255, 255, 255);
	elm_table_pack(tab, bg1, 1, 0, 1, 1);
	evas_object_show(bg1);

	Evas_Object *entry;
	entry = elm_entry_add(tab);
	EXPANDFILL(entry);
	evas_object_show(entry);
	elm_table_pack(tab, entry, 1, 0, 1, 1);
	evas_object_color_set(entry, 100, 100, 200, 255);

	char setOnEntry[0x14] = { 0 };
	snprintf(setOnEntry, sizeof(setOnEntry), "%d", ad->Data_Settings.interval);
	elm_entry_entry_set(entry, setOnEntry);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	//elm_object_text_set(button1, "NO");
	evas_object_size_hint_weight_set(button1, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(button1, ELM_SCALE_SIZE(60),
			ELM_SCALE_SIZE(40));
	EXPANDFILL(button1);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);

	Evas_Object *ic1;
	ic1 = elm_icon_add(button1);
	elm_image_file_set(ic1, ICON_DIR"/okfin.png", NULL);
	elm_object_part_content_set(button1, "icon", ic1);
	evas_object_show(ic1);

	elm_naviframe_item_push(nf, "Interval", NULL, NULL, bg, "empty");
	//----END

	ad->DataToPass.textbox = entry;

	evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, nf);
	evas_object_smart_callback_add(button1, "clicked", UPDATE_INTERVAL_cb, ad);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}

//Winodw settings/Interval Ends

static void btn_SOSButton_cb(void *data, Evas_Object *obj, void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;

	appdata_s *ad = data;

	bg = elm_bg_add(ad->nf);

	elm_bg_color_set(bg, 0, 0, 0);

	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_FALSE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	label = elm_label_add(box);
	elm_object_text_set(label, "<align=center>SOS</align>");

	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(90),
			ELM_SCALE_SIZE(30));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *tab = elm_table_add(bg);
	evas_object_size_hint_weight_set(tab, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(tab, EVAS_HINT_FILL, 0.4);
	evas_object_show(tab);
	elm_box_pack_end(box, tab);

	Evas_Object *button;
	button = elm_button_add(box);
	//elm_object_text_set(button, "OK");
	/*evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND,
	 EVAS_HINT_EXPAND);
	 evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(60),
	 ELM_SCALE_SIZE(40));*/
	EXPANDFILL(button);

	elm_object_style_set(button, "nextdepth");
	evas_object_show(button);
	elm_table_pack(tab, button, 0, 0, 1, 1);

	Evas_Object *ic;
	ic = elm_icon_add(button);
	elm_image_file_set(ic, ICON_DIR"/nofin.png", NULL);
	elm_object_part_content_set(button, "icon", ic);
	evas_object_show(ic);

	/*Evas_Object *bg1 = evas_object_rectangle_add(tab);
	 EXPANDFILL(bg1);
	 evas_object_color_set(bg1, 255, 255, 255, 255);
	 elm_table_pack(tab, bg1, 1, 0, 1, 1);
	 evas_object_show(bg1);
	 */
	label = elm_label_add(box);
	elm_object_text_set(label,
			"<align=center>Do you want to call to emergency number?</align>");
	//evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(130),
			ELM_SCALE_SIZE(180));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	EXPANDFILL(label);
	evas_object_show(label);
	elm_table_pack(tab, label, 1, 0, 1, 1);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	//elm_object_text_set(button1, "NO");
	EXPANDFILL(button1);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_table_pack(tab, button1, 2, 0, 1, 1);

	Evas_Object *ic1;
	ic1 = elm_icon_add(button1);
	elm_image_file_set(ic1, ICON_DIR"/okfin.png", NULL);
	elm_object_part_content_set(button1, "icon", ic1);
	evas_object_show(ic1);

	evas_object_smart_callback_add(button, "clicked", prev_btn_clicked_cb, ad->nf);
	evas_object_smart_callback_add(button1, "clicked", Make_SOS_Call, ad);

	elm_naviframe_item_push(ad->nf, "Background", NULL, NULL, bg, "empty");
	//----END

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}

//Window SOS END

//Window Settings.

static void btn_SETTINGSWindow_cb(void *data, Evas_Object *obj,
		void *event_info) {
	appdata_s *ad = data;

	Evas_Object *bg;
	bg = elm_bg_add(ad->nf);
	elm_bg_color_set(bg, 0, 0, 0);

	Evas_Object *scroller;

	scroller = elm_scroller_add(bg);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF,
			ELM_SCROLLER_POLICY_AUTO);
	evas_object_show(scroller);

	//----------
	Evas_Object *box;

	box = elm_box_add(scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_padding_set(box, 0, 5 * elm_config_scale_get());
	evas_object_show(box);

	Evas_Object *box1;

	box1 = elm_box_add(scroller);
	elm_box_padding_set(box1, 0, 5 * elm_config_scale_get());
	elm_box_horizontal_set(box1, EINA_TRUE);
	evas_object_show(box1);
	elm_box_pack_end(box, box1);

	Evas_Object *label = elm_label_add(box1);
	elm_object_text_set(label,
			"<align=center><font_size=25>SETTINGS</font_size></align>");
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	evas_object_smart_callback_add(label, "clicked", prev_btn_clicked_cb, bg);
	evas_object_show(label);
	elm_box_pack_end(box1, label);

	/*
	 Evas_Object *buttonx = elm_button_add(box1);
	 elm_object_text_set(buttonx,
	 "<align=left><font_size=15>BACK</font_size></align>");
	 evas_object_size_hint_min_set(buttonx, ELM_SCALE_SIZE(60),
	 ELM_SCALE_SIZE(20));
	 evas_object_smart_callback_add(buttonx, "clicked", prev_btn_clicked_cb,
	 ad->nf);
	 evas_object_show(buttonx);
	 elm_box_pack_end(box1, buttonx);
	 */

	Evas_Object *button = elm_button_add(box);
	elm_object_text_set(button, "INTERVAL");
	evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(180),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(button, "clicked", btn_SETTINGSInterval_cb,
			ad);
	evas_object_show(button);
	elm_box_pack_end(box, button);

	Evas_Object *button1 = elm_button_add(box);
	elm_object_text_set(button1, "SOS NUMBER");
	evas_object_size_hint_min_set(button1, ELM_SCALE_SIZE(240),
			ELM_SCALE_SIZE(40));
	evas_object_show(button1);
	evas_object_smart_callback_add(button1, "clicked", btn_SOSSettings_cb, ad);
	elm_box_pack_end(box, button1);

	Evas_Object *button2 = elm_button_add(box);
	elm_object_text_set(button2, "GPS TIME_OUT");
	evas_object_size_hint_min_set(button2, ELM_SCALE_SIZE(240),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(button2, "clicked",
			btn_GPSTIMEOUTSettings_cb, ad);
	evas_object_show(button2);
	elm_box_pack_end(box, button2);

	Evas_Object *button3 = elm_button_add(box);
	elm_object_text_set(button3, "URL_SERVER");
	evas_object_size_hint_min_set(button3, ELM_SCALE_SIZE(160),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(button3, "clicked", btn_URLSERVERSettings_cb,
			ad);
	evas_object_show(button3);
	elm_box_pack_end(box, button3);

	//----END

	elm_object_content_set(bg, box);
	elm_naviframe_item_push(ad->nf, NULL, NULL, NULL, bg, "empty");

	//elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
			ad->nf);
}

//WIndow Settings END.

//Window SEND POSITION

//Window SEND POSITION INFO
static void btn_SendPositionINFO_cb(void *data, Evas_Object *obj,
		void *event_info) {

	char str[2048] = { 0 };

	char *chars[256] = { 0 };

	int i = 0;

	int battery_percent = 0;

	Evas_Object *nf = data;

	Evas_Object *scroller;

	scroller = elm_scroller_add(nf);
	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_TRUE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF,
			ELM_SCROLLER_POLICY_AUTO);
	evas_object_show(scroller);

	//----------
	Evas_Object *box;

	box = elm_box_add(scroller);
	evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_padding_set(box, 0, 5 * elm_config_scale_get());
	evas_object_show(box);

	Evas_Object *label = elm_label_add(box);
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(120),
			ELM_SCALE_SIZE(100));
	elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object * OK = elm_button_add(box);
	elm_object_text_set(OK, "OK");
	evas_object_size_hint_min_set(OK, ELM_SCALE_SIZE(180), ELM_SCALE_SIZE(40));
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_smart_callback_add(OK, "clicked", prev_btn_clicked_cb, nf);
	evas_object_show(OK);
	elm_box_pack_end(box, OK);

	strcat(str, "<align=center><font_size=20><br>You will be sending:<br>");

	if (device_battery_get_percent(&battery_percent) == DEVICE_ERROR_NONE) {

		snprintf(chars, sizeof(chars), "bat: %d", battery_percent);

		strcat(str, chars);

	}
	device_battery_property_e battery_voltage =
			DEVICE_BATTERY_PROPERTY_CURRENT_NOW;

	if (device_battery_get_level_status(&battery_voltage)
			== DEVICE_ERROR_NONE) {
		snprintf(chars, sizeof(chars), "<br>volt: %d", battery_voltage);

		strcat(str, chars);

	}

	strcat(str, "<br>_");
	 //telephony stuff
	 telephony_error_e ret;


	 char* value = NULL;
	 system_info_get_platform_string("tizen.org/system/tizenid", &value);
	 snprintf(chars, sizeof(chars),
	 "<br>DUID: %s", value);
	 strcat(str, chars);


	 strcat(str, " <br>_ ");

	 //IMEI retrieval not for third party app.

	/* if (wifi_initialize() == WIFI_ERROR_NONE)
	     {
	         wifi_connection_state_e state = WIFI_CONNECTION_STATE_FAILURE;
	         if (wifi_get_connection_state(&state) == WIFI_ERROR_NONE)
	         {
	             switch (state)
	             {
	                 case WIFI_CONNECTION_STATE_FAILURE:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "WiFi connection state: FAILURE");
	                	 	 strcat(str, chars);
	                     break;
	                 case WIFI_CONNECTION_STATE_DISCONNECTED:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "WiFi connection state: DISCONNECTED");
	                	 	 strcat(str, chars);
	                     break;
	                 case WIFI_CONNECTION_STATE_ASSOCIATION:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "WiFi connection state: ASSOCIATION");
	                	 	 strcat(str, chars);
	                     break;
	                 case WIFI_CONNECTION_STATE_CONFIGURATION:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "WiFi connection state: CONFIGURATION");
	                	 	 strcat(str, chars);
	                     break;
	                 case WIFI_CONNECTION_STATE_CONNECTED:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "WiFi connection state: CONNECTED");
	                	 	 strcat(str, chars);
	                     break;
	                 default:
	                	 snprintf(chars, sizeof(chars),
	                	 	 "Invalid status!");
	                	 	 strcat(str, chars);
	                     break;
	             };
	         }
	         else
	         {
	        	 snprintf(chars, sizeof(chars),
            	 	 "Cannot get WiFi connection state!");
            	 	 strcat(str, chars);
	         }
	     }
	     else
	     {
	    	 snprintf(chars, sizeof(chars),
        	 	 "WiFi initialization error!");
        	 	 strcat(str, chars);
	     }
	 if (wifi_deinitialize() == WIFI_ERROR_NONE)
	     { snprintf(chars, sizeof(chars),
        	 	 " WiFi deinitialized ");
        	 	 strcat(str, chars);
	     }


	 strcat(str, "<br>_");

	 //bool internet_available = false;
	 connection_h connection;
	 connection_type_e type;

	 if (connection_create(&connection) == CONNECTION_ERROR_NONE)
	 {
	     if(connection_get_type(connection, &type) == CONNECTION_ERROR_NONE){
	         if(type == CONNECTION_TYPE_DISCONNECTED){
	        	 snprintf(chars, sizeof(chars),
	        	 	        	         	 	 " connection is disconnected. " );
	        	 	        	 strcat(str, chars);
	         }else if(type == CONNECTION_TYPE_WIFI || type == CONNECTION_TYPE_CELLULAR || type == CONNECTION_TYPE_ETHERNET || type == CONNECTION_TYPE_BT || type == CONNECTION_TYPE_NET_PROXY){
	        	 snprintf(chars, sizeof(chars),
	        	         	 	 " connection is available %d ", type );
	        	 strcat(str, chars);
	             //internet_available = true;
	         } else {
	        	 snprintf(chars, sizeof(chars),
	         	 	 " connection is not available ");
	         	 	 strcat(str, chars);
	             //internet_available = false;
	         }
	     }
	     connection_destroy(connection);
	 }


	 strcat(str, "<br>_");*/
/*

	 char *imei;
	 //In the case of a single SIM, you get only one handle

	 ret = telephony_init(&handle_list);
	 if (ret == TELEPHONY_ERROR_NONE) {

	 for (i = 0; i < handle_list.count; i++) {

	 snprintf(chars, sizeof(chars),
	 "<br>telephony handle[%p] for subscription[%d]",
	 handle_list.handle[i], i);
	 strcat(str, chars);

	 strcat(str, "<br>_");

	 ret = telephony_modem_get_imei(handle_list.handle[i], &imei);
	 if (ret == TELEPHONY_ERROR_NONE) {
	 //dlog_print(DLOG_INFO, LOG_TAG, "imei: %s", imei);
	 snprintf(chars, sizeof(chars), "<br>imei: %s", imei);
	 strcat(str, chars);
	 //free(imei);
	 } else {
	 snprintf(chars, sizeof(chars),
	 "<br>imei: getting error.Code: %d", ret);
	 strcat(str, chars);
	 }

	 }
	 }

	 strcat(str, "<br>_");

	 int cell_id;
	 ret = telephony_network_get_cell_id(handle_list.handle[0], &cell_id);
	 if (ret == TELEPHONY_ERROR_NONE) {
	 //dlog_print(DLOG_INFO, LOG_TAG, "Cell Id: %d", cell_id);
	 snprintf(chars, sizeof(chars), "<br>Cell Id: %d", ret);
	 strcat(str, chars);

	 } else {
	 snprintf(chars, sizeof(chars), "<br>Cell Id getting error.Code: %d",
	 ret);
	 strcat(str, chars);
	 }

	 strcat(str, "<br>_");

	 char *mnc;
	 ret = telephony_network_get_mnc(handle_list.handle[0], &mnc);
	 if (ret == TELEPHONY_ERROR_NONE) {

	 //dlog_print(DLOG_INFO, LOG_TAG, "mnc: %s", mnc);
	 snprintf(chars, sizeof(chars), "<br>mnc: %s ", mnc);
	 strcat(str, chars);

	 free(mnc);
	 } else {
	 snprintf(chars, sizeof(chars), "<br>mnc getting error. Code: %d", ret);
	 strcat(str, chars);
	 }

	 strcat(str, "<br>_");

	 char *meid;

	 ret = telephony_modem_get_meid(handle_list.handle[0], &meid);

	 if (ret == TELEPHONY_ERROR_NONE) {
	 snprintf(chars, sizeof(chars), "<br>meid: %s ", meid);
	 strcat(str, chars);
	 free(meid);
	 } else {
	 snprintf(chars, sizeof(chars), "<br>meid getting error.Code: %d", ret);
	 strcat(str, chars);
	 }

	 strcat(str, "<br>_");

	 strcat(str, "<br>APP version is: 0.001<br>");

	 strcat(str, "<br>_");

	 //double seconds = ecore_time_unix_get();

	 time_t t = time(NULL);
	 struct tm tm = *localtime(&t);

	 sprintf(chars, "<br>time now: %d-%02d-%02d %02d:%02d:%02d\n",
	 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
	 tm.tm_sec);
	 strcat(str, chars);

	 strcat(str, "<br>_");

	 ret = telephony_deinit(&handle_list);
*/

	//telephony work end
	//GPS work Starts
	// Start gps service
	Evas_Object *gps;
	Evas_Object *wps;

	location_manager_create(LOCATIONS_METHOD_GPS, &gps);
	//location_manager_create(LOCATIONS_METHOD_WPS, &wps);
	//location_manager_set_service_state_changed_cb(ad->gps, state_changed_cb, (void*)ad);
	location_manager_start(gps);

	double altitude, latitude, longitude, climb, direction, speed, horizontal,
			vertical;
	location_accuracy_level_e level;
	time_t timestamp;

	location_manager_get_location(gps, &altitude, &latitude, &longitude, &climb,
			&direction, &speed, &level, &horizontal, &vertical, &timestamp);

	snprintf(chars, sizeof(chars),
			"<br>In GPS: altitude %f, latitude %f, longitude %f, climb %f, direction %f, speed %f, horizontal %f, vertical %f",
			altitude, latitude, longitude, climb, direction, speed, horizontal,
			vertical);

	strcat(str, chars);

	strcat(str, "<br>_");

	location_manager_get_last_location(gps, &altitude, &latitude, &longitude,
			&climb, &direction, &speed, &level, &horizontal, &vertical,
			&timestamp);

	snprintf(chars, sizeof(chars),
			"<br>OR<br>In GPS: altitude %f, latitude %f, longitude %f, climb %f, direction %f, speed %f, horizontal %f, vertical %f",
			altitude, latitude, longitude, climb, direction, speed, horizontal,
			vertical);

	strcat(str, chars);
	/*
	 snprintf(chars, sizeof(chars),
	 "<br>In WPS: altitude %f, latitude %f, longitude %f, climb %f, direction %f, speed %f, horizontal %f, vertical %f",
	 altitude, latitude, longitude, climb, direction, speed, horizontal,
	 vertical);

	 strcat(str, chars);*/

	strcat(str, "<br>_");
//runtime info

	int value_int;
	int retCheck;

	retCheck = runtime_info_get_value_int(RUNTIME_INFO_KEY_GPS_STATUS,
			&value_int);
	if (retCheck != RUNTIME_INFO_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "runtime_info_get_value_int error: %d",
				retCheck);

		snprintf(chars, sizeof(chars),
				"<br>runtime_info_get_value_int error: %d", retCheck);

		strcat(str, chars);
		return;
	} else {
		switch (value_int) {
		case RUNTIME_INFO_GPS_STATUS_DISABLED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "GPS status: DISABLED.");
			snprintf(chars, sizeof(chars), "<br>GPS status: DISABLED.");
			strcat(str, chars);
			break;

		case RUNTIME_INFO_GPS_STATUS_SEARCHING:
			dlog_print(DLOG_DEBUG, LOG_TAG, "GPS status: SEARCHING.");
			snprintf(chars, sizeof(chars), "<br>GPS status: SEARCHING.");
			strcat(str, chars);
			break;

		case RUNTIME_INFO_GPS_STATUS_CONNECTED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "GPS status: CONNECTED.");
			snprintf(chars, sizeof(chars), "<br>GPS status: CONNECTED.");
			strcat(str, chars);
			break;

		default:
			dlog_print(DLOG_DEBUG, LOG_TAG, "GPS status: Unknown.");
			snprintf(chars, sizeof(chars), "<br>GPS status: Unknown.");
			strcat(str, chars);
			break;
		}
	}
	strcat(str, "<br>_");

	//GPS work ends

	// print on screen
	strcat(str, "<font_size></align>");
	elm_object_text_set(label, str);
	dlog_print(DLOG_DEBUG, LOG_TAG, str);
	//----END

	elm_object_content_set(scroller, box);
	elm_naviframe_item_push(nf, NULL, NULL, NULL, scroller, "empty");

	location_manager_stop(gps);

	dlog_print(DLOG_DEBUG, LOG_TAG, str);

}

//Window SEND POSITION INFO END

static void btn_SendPosition_cb(void *data, Evas_Object *obj, void *event_info) {
	Evas_Object *bg;
	Evas_Object *box;
	Evas_Object *label;
	Evas_Object *nf = data;

	bg = elm_bg_add(nf);

	elm_bg_color_set(bg, 0, 0, 0);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, prev_btn_clicked_cb,
				nf);
	/* Add a box and set it as the background content */
	box = elm_box_add(bg);
	//evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
	elm_box_horizontal_set(box, EINA_TRUE);
	elm_box_padding_set(box, 1, 5 * elm_config_scale_get());
	elm_object_content_set(bg, box);

	Evas_Object *button;
	button = elm_button_add(box);
	elm_object_text_set(button, "OK");
	//evas_object_size_hint_weight_set(button, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	//evas_object_size_hint_min_set(button, ELM_SCALE_SIZE(60), ELM_SCALE_SIZE(40));
	EXPANDFILL(button);
	evas_object_smart_callback_add(button, "clicked", btn_SendPositionINFO_cb,
			nf);
	elm_object_style_set(button, "nextdepth");
	evas_object_show(button);
	elm_box_pack_end(box, button);

	label = elm_label_add(box);
	elm_object_text_set(label,
			"<align=center><font_size=20>Do you want to send a GPS/WIFI position now?</font_size></align>");
	evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(80),
			ELM_SCALE_SIZE(140));
	elm_label_line_wrap_set(label, ELM_WRAP_WORD);
	//elm_object_content_set(box, ad->label);
	evas_object_show(label);
	elm_box_pack_end(box, label);

	Evas_Object *button1;
	button1 = elm_button_add(box);
	elm_object_text_set(button1, "NO");
	//evas_object_size_hint_weight_set(button1, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	//evas_object_size_hint_min_set(button1, ELM_SCALE_SIZE(60), ELM_SCALE_SIZE(40));
	EXPANDFILL(button1);
	evas_object_smart_callback_add(button1, "clicked", prev_btn_clicked_cb, nf);
	elm_object_style_set(button1, "nextdepth");
	evas_object_show(button1);
	elm_box_pack_end(box, button1);

	elm_naviframe_item_push(nf, "Background", NULL, NULL, bg, "empty");
	//----END

}

//Window SEND POSITION END

static void create_base_gui(appdata_s *ad) {
	/* Window */
	/* Create and initialize elm_win.
	 elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win,
				(const int *) (&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request",
			win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb,
			ad);

	/* Create an actual view of the base gui.
	 Modify this part to change the view. */
	//	/* Base Layout */
	ad->layout = elm_layout_add(ad->win);
	evas_object_size_hint_weight_set(ad->layout, EVAS_HINT_EXPAND,
	EVAS_HINT_EXPAND);
	elm_layout_theme_set(ad->layout, "layout", "application", "default");
	elm_win_resize_object_add(ad->win, ad->layout);
	evas_object_show(ad->layout);

	elm_object_content_set(ad->win, ad->layout);
	evas_object_show(ad->win);

	ad->nf = elm_naviframe_add(ad->layout);

	//-------------------------------MOVE TO PROCEDURE

	Evas_Object *bg;
	bg = elm_bg_add(ad->nf);
	elm_bg_color_set(bg, 27, 25, 27);

	eext_object_event_callback_add(bg, EEXT_CALLBACK_BACK, win_back_cb,
					ad);

	//---------- 0, 153, 51
	Evas_Object *box;

	box = elm_box_add(bg);
	EXPANDFILL(box);
	//elm_box_padding_set(box, 0, 5 * elm_config_scale_get());
	evas_object_show(box);

	Evas_Object *label = elm_label_add(box);
			elm_object_text_set(label,
					"<align=center> <font_size=20>V: 0.3</font_size></align>");
			//evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
			//evas_object_size_hint_min_set(label, ELM_SCALE_SIZE(180), ELM_SCALE_SIZE(100));
			//EXPANDFILL(label);
			//evas_object_size_hint_weight_set(label, 1.0, 1.0);
			//evas_object_size_hint_align_set(label, -1.0, 1.0);
			elm_label_line_wrap_set(label, ELM_WRAP_WORD);
			//elm_object_content_set(box, ad->label);
			evas_object_show(label);
			elm_box_pack_end(box, label);


	Evas_Object *tab = elm_table_add(bg);
	EXPANDFILL(tab);
	evas_object_show(tab);
 	elm_box_pack_end(box, tab);
	/*
	 Evas_Object *bg1 = evas_object_rectangle_add(tab);
	 EXPANDFILL(bg1);
	 evas_object_color_set(bg1, 255, 255, 255, 255);
	 elm_table_pack(tab, bg1, 0, 0, 1, 1);
	 evas_object_show(bg1);
	 */


	ad->RUN = elm_button_add(box);
	elm_object_text_set(ad->RUN, "RUN");
	evas_object_size_hint_min_set(ad->RUN, ELM_SCALE_SIZE(180),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(ad->RUN, "clicked", btn_RUN_clicked_cb, ad);
	evas_object_show(ad->RUN);
	//elm_box_pack_end(box, ad->RUN);
	elm_table_pack(tab, ad->RUN, 0, 0, 1, 1);

	Evas_Object *top_spacer = evas_object_rectangle_add(tab);
	evas_object_size_hint_min_set(top_spacer, 0, 10);
	evas_object_color_set(top_spacer, 0, 0, 0, 0);
	elm_table_pack(tab, top_spacer, 0, 1, 1, 1);

	/*Evas_Object *bg2 = evas_object_rectangle_add(tab);
	 EXPANDFILL(bg2);
	 evas_object_color_set(bg2, 255, 255, 255, 255);
	 elm_table_pack(tab, bg2, 0, 2, 1, 1);
	 evas_object_show(bg2);
	 */
	ad->Settings = elm_button_add(box);
	elm_object_text_set(ad->Settings, "SETTINGS");
	evas_object_size_hint_min_set(ad->Settings, ELM_SCALE_SIZE(240),
			ELM_SCALE_SIZE(40));
	//evas_object_smart_callback_add(ad->Settings, "clicked", btn_parseArray_clicked_cb, ad->nf);
	evas_object_show(ad->Settings);
	//evas_object_color_set(ad->Settings, 100, 100, 200, 255);
	evas_object_smart_callback_add(ad->Settings, "clicked",
			btn_SETTINGSWindow_cb, ad);
	//elm_box_pack_end(box, ad->Settings);
	elm_table_pack(tab, ad->Settings, 0, 2, 1, 1);

	/*
	 ad->label = elm_label_add(box);
	 elm_object_text_set(ad->label, "<align=center>SETTINGS</align>");
	 elm_label_line_wrap_set(ad->label, ELM_WRAP_WORD);
	 evas_object_show(ad->label);
	 elm_table_pack(tab, ad->label, 0, 2, 1, 1);
	 evas_object_smart_callback_add(ad->label, "clicked",
	 btn_SETTINGSWindow_cb, ad);
	 evas_object_color_set(ad->label, 0, 0, 10, 255);
	 */

	Evas_Object *top_spacer1 = evas_object_rectangle_add(tab);
	evas_object_size_hint_min_set(top_spacer1, 0, 10);
	elm_table_pack(tab, top_spacer1, 0, 3, 1, 1);

	ad->SOSButton = elm_button_add(box);
	elm_object_text_set(ad->SOSButton, "SOS BUTTON");
	evas_object_size_hint_min_set(ad->SOSButton, ELM_SCALE_SIZE(240),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(ad->SOSButton, "clicked", btn_SOSButton_cb,
			ad);
	evas_object_show(ad->SOSButton);
	//elm_box_pack_end(box, ad->SOSButton);
	elm_table_pack(tab, ad->SOSButton, 0, 4, 1, 1);

	Evas_Object *top_spacer2 = evas_object_rectangle_add(tab);
	evas_object_size_hint_min_set(top_spacer2, 0, 10);
	//elm_table_pack(tab, top_spacer2, 0, 7, 1, 1);
	elm_table_pack(tab, top_spacer2, 0, 5, 1, 1);

	ad->SendButton = elm_button_add(box);
	elm_object_text_set(ad->SendButton, "SEND POSITION");
	evas_object_size_hint_min_set(ad->SendButton, ELM_SCALE_SIZE(180),
			ELM_SCALE_SIZE(40));
	evas_object_smart_callback_add(ad->SendButton, "clicked",
			btn_SendPosition_cb, ad->nf);
	//evas_object_smart_callback_add(ad->SendButton, "clicked", addData_cb, ad);
	//evas_object_smart_callback_add(ad->SendButton, "clicked", btn_SendPosition_cb, ad);
	evas_object_show(ad->SendButton);
	//elm_box_pack_end(box, ad->SendButton);
	elm_table_pack(tab, ad->SendButton, 0, 6, 1, 1);

	Evas_Object *top_spacer4 = evas_object_rectangle_add(tab);
	evas_object_size_hint_min_set(top_spacer4, 0, 10);
	elm_table_pack(tab, top_spacer4, 0, 7, 1, 1);

	//----END

	elm_object_content_set(bg, box);
	elm_naviframe_item_push(ad->nf, NULL, NULL, NULL, bg, "empty");

	elm_object_part_content_set(ad->layout, "elm.swallow.content", ad->nf);

	ShowSplashScreen(ad->nf);


}

static bool app_create(void *data) {
	/* Hook to take necessary actions before main event loop starts
	 Initialize UI resources and application's data
	 If this function returns true, the main loop of application starts
	 If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	initialize_datacontrol_consumer(ad);

	add_data_change_cb_func(ad);

	//initialize_vault_datacontrol_consumer(ad);

	app_check_and_request_call_permission(ad);
	app_check_and_request_location_permission(ad);

	const char *integer_key = "FailedAttempts";
		bool existing;
		int FailedAttempts = 0;

		preference_is_existing(integer_key, &existing);
		if (existing == true) {
				preference_get_int(integer_key, &FailedAttempts);

				dlog_print(DLOG_INFO, LOG_TAG,"FailedAttempts saved was: %d",FailedAttempts);

			}else {
				//FailedAttempts++;
				preference_set_int(integer_key, FailedAttempts);
				dlog_print(DLOG_INFO, LOG_TAG,"FailedAttemptsdoesnot exist");

			}

	return true;
}

static void app_control(app_control_h app_control, void *data) {
	/* Handle the launch request. */
}

static void app_pause(void *data) {
	/* Take necessary actions when application becomes invisible. */
}

static void app_resume(void *data) {
	/* Take necessary actions when application becomes visible. */
}

static void app_terminate(void *data) {
	/* Release all resources. */
	appdata_s *ad = data;
	remove_data_change_cb_func(ad);
	data_control_sql_destroy(ad->provider_h);
}

static void ui_app_lang_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE,
			&locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void ui_app_orient_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void ui_app_region_changed(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void ui_app_low_battery(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LOW_BATTERY*/
}

static void ui_app_low_memory(app_event_info_h event_info, void *user_data) {
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char *argv[]) {
	appdata_s ad = { 0, };
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = { 0, };
	app_event_handler_h handlers[5] = { NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY],
			APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY],
			APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED],
			APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
