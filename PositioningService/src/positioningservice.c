#include <tizen.h>
#include <service_app.h>
#include "positioningservice.h"
//#include <net_connection.h>

#include <app_preference.h>

#include <data_control.h>

#include <sqlite3.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

#include <device/power.h>
//#include <pthread.h>

#include <app_manager.h>

char *app_id = "dk.safecall.backgrounduploaderservice";// hard code is enough and ok.

typedef struct dataControl_data {

	char URLServer[0x100];
	int SOS_number;
	int GPS_TimeOut;
	int interval;


	data_control_h *provider_service_h, *vault_provider_h;

} dataControl_data;

dataControl_data ad;

Ecore_Timer *timer, *GPSTimeOut, *peropdicFailedAttemptsUpdater;

int gpsFailed, TestFailedAttempts, startUploadFrom;
const char *integer_key = "FailedAttempts";
void savedata(char *str, int TestFailedAttempts);
bool shouldDelet, noInternet, canUploadFailedAttempts;

data_control_sql_response_cb sql_callback;


int launch_failedAttemptsUploader_service() {
	app_control_h app_control = NULL;
	if (app_control_create(&app_control) == APP_CONTROL_ERROR_NONE) {
		if ((app_control_set_app_id(app_control,
				"dk.safecall.backgrounduploaderservice") == APP_CONTROL_ERROR_NONE)
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

//DataControl works here...


void
app_check_and_request_permission(void *data);

void
sql_insert_response_cb(int request_id, data_control_h provider,
                       long long inserted_row_id, bool provider_result,
                       const char *error, void *user_data)
{
    if (provider_result) {
        dlog_print(DLOG_INFO, LOG_TAG, "The insert operation is successful");
    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The insert operation for the request %d failed. error message: %s",
                   request_id, error);
    }
}


void
sql_vault_insert_response_cb(int request_id, data_control_h provider,
                       long long inserted_row_id, bool provider_result,
                       const char *error, void *user_data)
{
    if (provider_result) {
        dlog_print(DLOG_INFO, LOG_TAG, "The insert operation is successful");

    	//we have failed attempts saved to storage, start service to upload those data.
    	bool running;
    			if (app_manager_is_running(app_id, &running) == APP_MANAGER_ERROR_NONE) {
    				if (!running) {
    					launch_failedAttemptsUploader_service();
    				}
    			}

    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The insert operation for the request %d failed. error message: %s",
                   request_id, error);
    }
}




static void stop_service() {
	app_control_h app_control = NULL;
	if (app_control_create(&app_control) == APP_CONTROL_ERROR_NONE) {
		if ((app_control_set_app_id(app_control,
				"dk.safecall.backgrounduploaderservice") == APP_CONTROL_ERROR_NONE)
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



//thread for network calls
static void
_thread_end_cb(void *data, Ecore_Thread *th)
{
  //App_Data *ad = data;

  dlog_print(DLOG_DEBUG, LOG_TAG,"Normal termination for thread %p.\n", th);
  if (th != NULL)
	  th = NULL;

}

static void
_thread_cancel_cb(void *data, Ecore_Thread *th)
{

  dlog_print(DLOG_DEBUG, LOG_TAG,"Thread %p got cancelled.\n", th);
  if (th != NULL)
	  th = NULL;

}
void upload_to_server_job(void *data EINA_UNUSED, Ecore_Thread *th)
{

	char *str = data;

	dlog_print(DLOG_INFO, LOG_TAG,"This is the thread for uploading data...");

	Ecore_Job *job;
	job = ecore_job_add(networkCall_cb, str);

	(void)job;

	//networkCall_cb(str);

}

//thread for network calls ends.

/* Callback for the select operation response */
void
sql_select_response_cb(int request_id, data_control_h provider,
                       result_set_cursor cursor, bool provider_result,
                       const char *error, void *user_data)
{

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

        data_control_sql_get_text_data(cursor, 0, ad.URLServer);
        data_control_sql_get_int_data(cursor, 1, &(ad.interval));
        data_control_sql_get_int_data(cursor, 2, &(ad.SOS_number));
        data_control_sql_get_int_data(cursor, 3, &(ad.GPS_TimeOut));

        dlog_print(DLOG_INFO, LOG_TAG, "URL: %s, INTERVAL: %d, SOS: %d, GPSTIMEOUT: %d ",
        		ad.URLServer, ad.interval, ad.SOS_number, ad.GPS_TimeOut);
    }
}

/* Callback for the select operation response from vault service*/
void sql_select_vault_response_cb(int request_id, data_control_h provider,
		result_set_cursor cursor, bool provider_result, const char *error,
		void *user_data) {


	if (provider_result) {
			dlog_print(DLOG_INFO, LOG_TAG, "The vault select operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "The select operation for the request %d failed. error message: %s",
					request_id, error);
		}


		//int totalUploadableRequests = 1;

		while (data_control_sql_step_next(cursor) == DATA_CONTROL_ERROR_NONE) {

			 char REQUEST[512] = {0,};
			 int ID = 0;

			data_control_sql_get_int_data(cursor, 0, &ID);
			data_control_sql_get_text_data(cursor, 1, REQUEST);

			dlog_print(DLOG_INFO, LOG_TAG, "ID: %d, REQUEST: %s.", ID, REQUEST );

			//make network call again... Its fine as it gets one id'ed call at a time.
			// then delete from database if succeed.

			// make network call thread.

			/*if (ID != 0) {

				ecore_thread_run(upload_to_server_job, _thread_end_cb, _thread_cancel_cb, REQUEST);
				totalUploadableRequests++;

			} else {
				if (totalUploadableRequests < startUploadFrom) {
									TestFailedAttempts = 1;

									dlog_print(DLOG_WARN, LOG_TAG,"restter check happening with UP: %d, su: %d", totalUploadableRequests, startUploadFrom);
								}

				if (totalUploadableRequests == 0) {
					TestFailedAttempts = 1;
					preference_set_int(integer_key, TestFailedAttempts);
					dlog_print(DLOG_WARN, LOG_TAG,"restter check happened. All failed attempts is uploaded.");

				}
			}

			if (shouldDelet) {
				int req_id = 0;
				char buf[8];
				snprintf(buf, sizeof(buf), "ID = %d",ID);
				//char *where_delete = "WORD = 'test'";
				int result = data_control_sql_delete(ad.vault_provider_h, buf, &req_id);
				startUploadFrom = ID;

			}
*/

		}

}


/* Callback for the update operation response */
void
sql_update_response_cb(int request_id, data_control_h provider, bool provider_result,
                       const char *error, void *user_data)
{
    if (provider_result) {
        dlog_print(DLOG_INFO, LOG_TAG, "The update operation is successful");
    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The update operation for the request %d failed. error message: %s",
                   request_id, error);
    }
}

/* Callback for the delete operation response */
void
sql_delete_response_cb(int request_id, data_control_h provider, bool provider_result,
                       const char *error, void *user_data)
{
    if (provider_result) {
        dlog_print(DLOG_INFO, LOG_TAG, "The delete operation is successful");
    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The delete operation for the request %d failed. error message: %s",
                   request_id, error);
    }
}





void
initialize_datacontrol_consumer()
{
    int ret;

    const char *provider_id = "http://datacontrolservice.com/datacontrol/provider/datacontrolservice";
    const char *data_id = "Dictionary";

    /* Create data control handler */
    ret = data_control_sql_create(&(ad.provider_service_h));
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "creating data control provider failed with error: %d", ret);

    ret = data_control_sql_set_provider_id(ad.provider_service_h, provider_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "setting provider id failed with error: %d", ret);

    ret = data_control_sql_set_data_id(ad.provider_service_h, data_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

    /* Set response callbacks */
    sql_callback.delete_cb = sql_delete_response_cb;
    sql_callback.insert_cb = sql_insert_response_cb;
    sql_callback.select_cb = sql_select_response_cb;
    sql_callback.update_cb = sql_update_response_cb;

    /* Register response callbacks */
    ret = data_control_sql_register_response_cb(ad.provider_service_h, &sql_callback, NULL);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "Registering the callback function failed with error: %d", ret);

    dlog_print(DLOG_INFO, LOG_TAG, "Init data control success");


    int req_id = 0;

    //Send a request to select a row
	char *column_list[4];
	column_list[0] = "URL";
	column_list[1] = "INTERVALTIME";
	column_list[2] = "SOS";
	column_list[3] = "GPSTIMEOUT";

	const char *where = "1";
	const char *order = "(SELECT NULL)";

	data_control_sql_select(ad.provider_service_h, column_list, 4, where, order, &req_id);


}

//vault service
void initialize_vault_datacontrol_consumer() {
	int ret;
	const char *provider_id =
			"http://vaultservice.com/datacontrol/provider/vaultservice";

	const char *data_id = "Vault";

		/* Create data control handler */
		ret = data_control_sql_create(&(ad.vault_provider_h));
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"creating data control provider failed with error: %d", ret);

		ret = data_control_sql_set_provider_id(ad.vault_provider_h, provider_id);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"setting provider id failed with error: %d", ret);

		ret = data_control_sql_set_data_id(ad.vault_provider_h, data_id);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d",
					ret);

		/* Set response callbacks */
		sql_callback.delete_cb = sql_delete_response_cb;
		sql_callback.insert_cb = sql_vault_insert_response_cb;
		sql_callback.select_cb = sql_select_vault_response_cb ;
		sql_callback.update_cb = sql_update_response_cb;

		/* Register response callbacks */
		ret = data_control_sql_register_response_cb(ad.vault_provider_h, &sql_callback, NULL);
		if (ret != DATA_CONTROL_ERROR_NONE)
			dlog_print(DLOG_ERROR, LOG_TAG,
					"Registering the callback function failed with error: %d", ret);

		dlog_print(DLOG_INFO, LOG_TAG, "Init data control success");

		//Send a request to insert a row

		int req_id = 0;
		bundle *b = bundle_create();
		bundle_add_str(b, "REQUEST",
							"'https://server.safecall.no/Webservices/PositioningTizen.ashx?'");
		bundle_add_str(b, "ID", "0");

		data_control_sql_insert(ad.vault_provider_h, b, &req_id);

		//Free memory
		bundle_free(b);

		/*b = bundle_create();
				bundle_add_str(b, "REQUEST",
									"'This is the 100th one test?'");
				bundle_add_str(b, "ID", "100");

				data_control_sql_insert(ad.vault_provider_h, b, &req_id);

				//Free memory
				bundle_free(b);
		 */

		/*
		//after checks of firstRun we will populate our variables from database controller.
		ret = data_control_sql_set_data_id(ad.vault_provider_h, data_id);
		if (ret == DATA_CONTROL_ERROR_NONE){
		  Send a request to select a row
			char *column_list[2];
			column_list[0] = "ID";
			column_list[1] = "REQUEST";

			const char *where = "1";
			const char *order = "(SELECT NULL)";

			data_control_sql_select(ad.vault_provider_h, column_list, 2, where, order, &req_id);

			}
		else {

			dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

		}*/


}


static void DATA_updater(void *data, char *what, char *value){


	int req_id = 0;
	bundle *b;
	int ret;
	char *where = "1";

	//dlog_print(DLOG_DEBUG, LOG_TAG, "we got, what: %s, and value: %s", what, value);


	//Insert data to the Dictionary table
	ret = data_control_sql_set_data_id(ad.provider_service_h, "Dictionary");
	if (ret != DATA_CONTROL_ERROR_NONE)
	dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

	b = bundle_create();
	bundle_add_str(b, what, value);
	data_control_sql_update(ad.provider_service_h, b, where, &req_id);
	bundle_free(b);



}





//DataControl work ends.


//Data change CB



/* Triggered when the data change notification arrives */
void
data_change_cb(data_control_h provider, data_control_data_change_type_e type,
               bundle *data, void *user_data)
{

	char *column_list[4];
	column_list[0] = "URL";
	column_list[1] = "INTERVALTIME";
	column_list[2] = "SOS";
	column_list[3] = "GPSTIMEOUT";

	char *where = "1";
	int req_id = 0;
	char *order = "(SELECT NULL)";


	data_control_sql_select(ad.provider_service_h, column_list, 4, where, order, &req_id);

}

/* Triggered when the provider has accepted the callback registration */
void
result_cb(data_control_h provider, data_control_error_e result, int callback_id,
          void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG, "Add data change callback RESULT: %d", result);
    if (result == 0) {

    	app_check_and_request_permission(user_data);
	}
}

/* Register the callback //, Evas_Object *obj, void *event_info */
int cb_id;
void
add_data_change_cb_func(void *data)
{

    int ret = data_control_add_data_change_cb(ad.provider_service_h, data_change_cb, NULL,
                                              result_cb, NULL, &cb_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "add data change callback failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "add data change callback done: %d", cb_id);



}

/* Remove the callback */
void
remove_data_change_cb_func(void *data)
{
    data_control_remove_data_change_cb(ad.provider_service_h, cb_id);
    dlog_print(DLOG_INFO, LOG_TAG, "remove data change callback done: %d", cb_id);
}






//Data Change NOTI

/*
//test with ecore job
void
_job_print_cb(void *data)
{
   char *str = data;

   dlog_print(DLOG_INFO, LOG_TAG,"%s\n", str);
}

static void
_job_quit_cb(void *data EINA_UNUSED)
{
   //ecore_main_loop_quit();

	   dlog_print(DLOG_INFO, LOG_TAG,"test quit");
}


//test with ecore job ends
*/


//base works ...............

/*

void upload_failed_attempts_again_job(void *data EINA_UNUSED, Ecore_Thread *th)
{

	int i;
	int check = (int)data;
	char buf[512];
	int req_id = 0;

	dlog_print(DLOG_INFO, LOG_TAG,"This is the thread Running... %d", check);
	for (i = startUploadFrom; i <= check; i++)
      {
         //dlog_print(DLOG_DEBUG, LOG_TAG, "Thread %p: String number %d", th, i);
         //sleep(0.5);

         char *column_list[2];
         column_list[0] = "ID";
         column_list[1] = "REQUEST";


         snprintf(buf, sizeof(buf),"ID=%d", (i));
         const char *where = buf;
         const char *order = "(SELECT NULL)";


     	 dlog_print(DLOG_INFO, LOG_TAG,"This thread is calling... %d times out of: %d to id: %d", i, check, (i));
         data_control_sql_select(ad.vault_provider_h, column_list, 2, where, order, &req_id);
         //sleep(1);

         shouldDelet = true;

      }


	 char *column_list[2];
	 column_list[0] = "ID";
	 column_list[1] = "REQUEST";


	 snprintf(buf, sizeof(buf),"ID=%d", (i));
	 const char *where = buf;
	 const char *order = "(SELECT NULL)";


	 dlog_print(DLOG_INFO, LOG_TAG,"This thread is calling... %d times out of: %d to id: %d", i, check, (i));
	 data_control_sql_select(ad.vault_provider_h, column_list, 2, where, order, &req_id);
	 //sleep(1);

	 shouldDelet = true;
	//free buf
	snprintf(buf, sizeof(buf),"");

}
*/

static Eina_Bool PeriodicGPSTimeOut(void *data EINA_UNUSED) {


	 /*  Ecore_Job *job1, *job2, *job3, *job_quit;
	   char *str1 = "Job 1 started.";
	   char *str2 = "Job 2 started.";
	   char *str3 = "Job 3 started.";

	   if (!ecore_init())
	     {
		    dlog_print(DLOG_INFO, LOG_TAG,"ERROR: Cannot init Ecore!\n");
	        return -1;
	     }


	   //job1 = ecore_job_add(_job_print_cb, str1);
	   job2 = ecore_job_add(_job_print_cb, str2);
	   job3 = ecore_job_add(_job_print_cb, str3);

	   //job_quit = ecore_job_add(_job_quit_cb, NULL);

	   //(void)job2;
	   (void)job3;
	   //(void)job_quit;

	   dlog_print(DLOG_INFO, LOG_TAG,"Created jobs and quit.\n");

	   if (job2)
	     {
	        char *str;
	        str = ecore_job_del(job2);
	        job2 = NULL;
	        dlog_print(DLOG_INFO, LOG_TAG,"Deleted job 2. Its data was: \"%s\"\n", str);
	     }

	   //ecore_main_loop_begin();
	   //ecore_shutdown();

	   dlog_print(DLOG_INFO, LOG_TAG,"this callback exectured reagrdless.\n");*/


	location_manager_stop(manager);

	dlog_print(DLOG_INFO, LOG_TAG, "GPSTimeOut... shutting down gps manager.");

	gpsFailed++;
	return ECORE_CALLBACK_RENEW;//EINA_TRUE
}

void resetGPSTimer(){


	ecore_timer_del(GPSTimeOut);
	GPSTimeOut = ecore_timer_add(ad.GPS_TimeOut, PeriodicGPSTimeOut, NULL);

}
/*

Eina_Bool PeriodicFailedAttemptsUpdater(void *data EINA_UNUSED) {

	bool existing;
	preference_is_existing(integer_key, &existing);
	int FailedAttemptsSecondary;
	if (existing == true) {
				preference_get_int(integer_key, &FailedAttemptsSecondary);

				if (FailedAttemptsSecondary > 1) {

					dlog_print(DLOG_INFO, LOG_TAG,"FailedAttempts retrieved and proceeding with upload for this many times: %d", FailedAttemptsSecondary);


					ecore_thread_run(upload_failed_attempts_again_job, _thread_end_cb, _thread_cancel_cb, FailedAttemptsSecondary);

				}else {
					dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts logged yet. All is good.");
					shouldDelet = false;
				}

			}else {
				dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts. Canceling uploader. ");

				return ECORE_CALLBACK_CANCEL;
			}

	return ECORE_CALLBACK_RENEW;//EINA_TRUE

		bool existing;
		preference_is_existing(integer_key, &existing);
		int FailedAttemptsSecondary;
		if (existing == true) {
					preference_get_int(integer_key, &FailedAttemptsSecondary);

					if (FailedAttemptsSecondary > 1) {

						dlog_print(DLOG_INFO, LOG_TAG,"FailedAttempts retrieved and proceeding with upload for this many times: %d", FailedAttemptsSecondary);


						ecore_thread_run(upload_failed_attempts_again_job, _thread_end_cb, _thread_cancel_cb, FailedAttemptsSecondary);

					}else {
						dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts logged yet. All is good.");
						shouldDelet = false;
					}

				}else {
					dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts. Canceling uploader. ");

				}
		return ECORE_CALLBACK_CANCEL;
}
*/

static Eina_Bool PeriodicUpdater(void *data EINA_UNUSED) {


	resetGPSTimer();

	location_manager_start(manager);

	if (TestFailedAttempts > 0) {
		if (!noInternet) {
			dlog_print(DLOG_INFO, LOG_TAG,"Internet is probably available again. Try to check for saved data.");
			bool running;
			if (app_manager_is_running(app_id, &running) == APP_MANAGER_ERROR_NONE) {
						if (!running) {
									launch_failedAttemptsUploader_service();
								}
							}
			}
	}

	//dlog_print(DLOG_INFO, LOG_TAG, "yay, Im called...");

	return ECORE_CALLBACK_RENEW;//EINA_TRUE
}


void getCoreInformation(){
						/*

https://server.safecall.no/Webservices/PositioningTizen.ashx?lat=-17.369296&lon=-66.193078&des=&alt=2650.50&spd=0.000000&src=GPS&dat=2020-04-17T15%3a53%3a44&bat=50&vol=3&aid=NUwEIz%2fUQmaGr9wJ3EAOt1R6SUQ%3d&imei=z2wVZUYFRiiMzOYLbcecsVR6SUQ%3d11&ver=0.3
https://dev.safecall.no/Webservices/PositioningTizen.ashx?lat=-17.369296&lon=-66.193078&des=&alt=2650.50&spd=0.000000&src=GPS&dat=2020-03-30T16%3a53%3a44&bat=50&vol=3&aid=NUwEIz%2fUQmaGr9wJ3EAOt1R6SUQ%3d&imei=z2wVZUYFRiiMzOYLbcecsVR6SUQ%3d11&a=&ver=0.3

						"&tmp=%f"
						"&ver=0.0.3"
						"&aid=c13d1d948ea2f352"
						"&imei=352842073942434"
						"&wifi=0,&towers=0"

						 */

		char *chars[0x80] = { 0 };

		int battery_percent = 0;


		//add time
		//"&dat=2020-03-30T16:53:44"
		time_t rawtime = time(NULL);

		    if (rawtime == -1) {

		    	snprintf(chars, sizeof(chars), "&dat=2020-03-30T16:53:44");
		    	strcat(str, chars);

		    }else {

		    //struct tm *ptm = localtime(&rawtime);
		    struct tm *ptm = gmtime(&rawtime);

		    if (ptm != NULL) {

		    	snprintf(chars, sizeof(chars), "&dat=%02d-%02d-%02dT%02d:%02d:%02d", ptm->tm_year + 1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour,
		    		           ptm->tm_min, ptm->tm_sec);

		    	strcat(str, chars);

		    	}
		    }




		//bat info
		if (device_battery_get_percent(&battery_percent) == DEVICE_ERROR_NONE) {

				snprintf(chars, sizeof(chars), "&bat=%d", battery_percent);

				strcat(str, chars);

			}
			device_battery_property_e battery_voltage =
					DEVICE_BATTERY_PROPERTY_CURRENT_NOW; // there should be voltage.

			if (device_battery_get_level_status(&battery_voltage)
					== DEVICE_ERROR_NONE) {
				snprintf(chars, sizeof(chars), "&vol=%d", battery_voltage);

				strcat(str, chars);

			}


		char* value = NULL;
		system_info_get_platform_string("tizen.org/system/tizenid", &value);
		//snprintf(chars, sizeof(chars), "<br>DUID: %s", value);
		if (value == NULL) {
			value = "NULL_tizenid";
		}
		snprintf(chars, sizeof(chars), "&aid=%s&imei=%s", value, value);
		strcat(str, chars);

		//app ver.
		snprintf(chars, sizeof(chars), "&ver=0.3&TestTimeouts=%d&TestFailedAttempts=%d",gpsFailed,TestFailedAttempts);

		strcat(str, chars);


		/*

		telephony_error_e ret;

		ret = telephony_init(&handle_list);

		int cell_id;
		ret = telephony_network_get_cell_id(handle_list.handle[0], &cell_id);
		if (ret == TELEPHONY_ERROR_NONE) {
			dlog_print(DLOG_INFO, LOG_TAG, "Cell Id: %d", cell_id);
			//snprintf(chars, sizeof(chars), "&wifi=0,&towers=%d", cell_id);
			//strcat(str, chars);
			//dlog_print(DLOG_DEBUG, LOG_TAG, chars);
			} else {
					dlog_print(DLOG_INFO, LOG_TAG, "Cell Id getting error.Code: %d", ret);
					//snprintf(chars, sizeof(chars), "<br>Cell Id getting error.Code: %d", ret);
					//strcat(str, chars);
					}

			char *mnc;
			ret = telephony_network_get_mnc(handle_list.handle[0], &mnc);
			if (ret == TELEPHONY_ERROR_NONE) {

						dlog_print(DLOG_INFO, LOG_TAG, "mnc: %s", mnc);
						//snprintf(chars, sizeof(chars), "%s", mnc);
						//strcat(str, chars);
						//dlog_print(DLOG_DEBUG, LOG_TAG, chars);

						free(mnc);
					} else {
						//snprintf(chars, sizeof(chars), "<br>mnc getting error. Code: %d", ret);
						//strcat(str, chars);
					}





			char *meid;

			ret = telephony_modem_get_meid(handle_list.handle[0], &meid);

			if (ret == TELEPHONY_ERROR_NONE) {
						snprintf(chars, sizeof(chars), "meid: %s ", meid);
						strcat(str, chars);
						dlog_print(DLOG_DEBUG, LOG_TAG, chars);
						free(meid);
					} else {
						snprintf(chars, sizeof(chars), "meid getting error.Code: %d", ret);
						dlog_print(DLOG_DEBUG, LOG_TAG, chars);

						//strcat(str, chars);
					}



			ret = telephony_deinit(&handle_list);

*/

			dlog_print(DLOG_DEBUG, LOG_TAG, str);


}


void
app_request_response_cb(ppm_call_cause_e cause, ppm_request_result_e result,
                             const char *privilege, void *user_data)
{

	//appdata_s *ad = user_data;

    if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
        /* Log and handle errors */
    	dlog_print(DLOG_DEBUG, LOG_TAG, "Log and handle errors");

        return;
    }

    switch (result) {
        case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
            /* Update UI and start accessing protected functionality */
        	dlog_print(DLOG_DEBUG, LOG_TAG, "allow forever, Update UI and start accessing protected functionality.");
        	enter_gps();
        	break;
        case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
            /* Show a message and terminate the application */
        	dlog_print(DLOG_DEBUG, LOG_TAG, "Deny forever, Show a message and terminate the application.");
            break;
        case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
            /* Show a message with explanation */
        	dlog_print(DLOG_DEBUG, LOG_TAG, "Deny once, Show a message with explanation");
            break;
    }
}




void
app_check_and_request_permission(void *data)
{
	//appdata_s *ad = data;

    ppm_check_result_e result;
    const char *privilege = "http://tizen.org/privilege/location";

    int ret = ppm_check_permission(privilege, &result);
    if (ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
            switch (result) {
                case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
                     //Update UI and start accessing protected functionality
                	enter_gps();
                	dlog_print(DLOG_DEBUG, LOG_TAG, "allow, Update UI and start accessing protected functionality");
                	break;
                case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
                     //Show a message and terminate the application
                	dlog_print(DLOG_DEBUG, LOG_TAG, "Deny , Show a message and terminate the application");

                                break;
                case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
                                ret = ppm_request_permission(privilege, app_request_response_cb, NULL);
                                 //Log and handle errors
                                dlog_print(DLOG_DEBUG, LOG_TAG, "Asking.");

                                break;
                        }
                    }
                    else {
                         //ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE
                         //Handle errors
                    	dlog_print(DLOG_DEBUG, LOG_TAG, "errors.");

                    }

}



/* Callback for whenever gps state is changed */
void standalone_gps_state_changed_cb(location_service_state_e state,
		void *user_data) {


	int ret;
	/* Check if the service is enabled - can only read location data if the service is enabled */
	if (LOCATIONS_SERVICE_ENABLED == state) {

		//appdata_s* ad = user_data;
		//elm_object_text_set(ad->label, "GPS Location service started. Waiting for location.<br/><br/>");
		dlog_print(DLOG_DEBUG, LOG_TAG,  "GPS Location service started. Waiting for location.");

		/* Receive the current information about position, velocity, or location accuracy: */
		/*double altitude, latitude, longitude;
		time_t timestamp;
		int ret = location_manager_get_position(manager, &altitude, &latitude,
				&longitude, &timestamp);
		*/
		double altitude, latitude, longitude, climb, direction, speed, horizontal,
							vertical;
		location_accuracy_level_e level;
		time_t timestamp;
		location_method_e *method;

		//int ret = location_manager_get_last_location(manager, &altitude, &latitude, &longitude, &climb, &direction, &speed, &level, &horizontal, &vertical, &timestamp);
		int ret = location_manager_get_location(manager, &altitude, &latitude, &longitude, &climb, &direction, &speed, &level, &horizontal, &vertical, &timestamp);

		//int ret = location_manager_get_last_position(manager, &altitude, &latitude, &longitude, &timestamp);

		if (LOCATIONS_ERROR_NONE == ret) {

			//Process the location information

			/* Sample code to display the data */
			//dlog_print(DLOG_INFO, LOG_TAG, "location got altitude %f, latitude %f, longitude %f", altitude, latitude, longitude);

			char buf[256];
			//char* timebuf;
			//timebuf = asctime(localtime(&timestamp));

			//snprintf(buf, sizeof(buf),"In GPS: altitude %f, latitude %f, longitude %f, climb %f, direction %f, speed %f, horizontal %f, vertical %f, Timestamp - %s", altitude, latitude, longitude, climb, direction, speed, horizontal, vertical, timebuf);

			snprintf(buf, sizeof(buf),
					"lat=%f"
					"&lon=%f"
					"&des="
					//"&sat=%f"
					"&alt=%f"
					"&spd=%f",
					//"&rdo=%f"
					//"&ang=%f"
					latitude,
					longitude,
					altitude,
					speed);

			//http://server.tecnologica.com.bo/OldSmith/Webservices/PositioningApp.ashx?lat=-17.3762692&lon=-66.1560879&des=&sat=0&alt=0.0&spd=0.0&rdo=28.77&ang=0.0&src=network&dat=2015-09-30T15:05:02Z&bat=92.0&vol=4.222&tmp=34.0&ver=0.2.3&aid=c13d1d948ea2f352&imei=352842073942434&wifi=00:1c:f0:fd:ef:d6|TECNOLOGICA|-55,1c:7e:e5:dc:50:08|IMEXCOMED|-52,34:08:04:0c:77:90|COPORBO|-64,00:13:33:ac:97:27|identigene|-74,28:28:5d:fb:31:fc|COMTECO-95220945|-74,00:13:33:c4:c4:98|NOELIARIOS|-78,e8:94:f6:a0:ce:90|Stephi|-78,3c:ce:73:8f:08:f1|aitwnet1|-79,c8:3a:35:2a:e1:e1|COMTECO-272537|-83,&towers=736,02,40401,31133,-51
			//new demo url:
			//https://dev.safecall.no/Webservices/PositioningTizen.ashx?lat=-17.369296&lon=-66.193078&des=&alt=2650.50&spd=0.000000&src=GPS&dat=2020-03-30T16%3a53%3a44&bat=50&vol=3&aid=NUwEIz%2fUQmaGr9wJ3EAOt1R6SUQ%3d&imei=z2wVZUYFRiiMzOYLbcecsVR6SUQ%3d11&a=&ver=0.3

			//setting server prefixes.
			//strcat(str, "http://server.tecnologica.com.bo/OldSmith/Webservices/PositioningApp.ashx?");
			snprintf(str, sizeof(str), ad.URLServer);

			strcat(str, buf);


			if (location_manager_get_method ( manager, &method) == LOCATIONS_ERROR_NONE) {
				if (method == LOCATIONS_METHOD_HYBRID) {
						snprintf(buf, sizeof(buf), "&src=HYBRID");
						dlog_print(DLOG_DEBUG, LOG_TAG, "method: is hyrbid");
						strcat(str, buf);
					}
				else if (method == LOCATIONS_METHOD_GPS) {
									snprintf(buf, sizeof(buf), "&src=GPS");
									dlog_print(DLOG_DEBUG, LOG_TAG, "method: is GPS");
									strcat(str, buf);
					}
				else if (method == LOCATIONS_METHOD_WPS) {
									snprintf(buf, sizeof(buf), "&src=WPS");
									dlog_print(DLOG_DEBUG, LOG_TAG, "method: is WPS");
									strcat(str, buf);
								}
				else{
					snprintf(buf, sizeof(buf), "&src=unknown");
					dlog_print(DLOG_DEBUG, LOG_TAG, "method: is unknown");
					strcat(str, buf);
				}
			}


			ecore_timer_del(GPSTimeOut);

			location_manager_stop(manager);

			getCoreInformation();


			//pthread_create(&thread_id, NULL, networkCall_cb, thread_id);
			//pthread_t thread_net;
			//pthread_create(&thread_net, NULL, networkCall_cb, NULL);


			/*
			Ecore_Job *job;
			 //don't need following, it comes here if it had success on following from before code
				   if (!ecore_init())
				     {
					    dlog_print(DLOG_INFO, LOG_TAG,"ERROR: Cannot init Ecore!\n");
				        return -1;
				     }


			 job = ecore_job_add(networkCall_cb, str);

			 //job_quit = ecore_job_add(_job_quit_cb, NULL);

			 (void)job;
			 */


			ecore_thread_run(upload_to_server_job, _thread_end_cb, _thread_cancel_cb, str);


			dlog_print(DLOG_INFO, LOG_TAG,"Create job done.\n");



		}
	}
}

void setup_location_manager()
{
    if (location_manager_create(LOCATIONS_METHOD_GPS, &manager) != LOCATIONS_ERROR_NONE)
    {
        dlog_print(DLOG_DEBUG, LOG_TAG, "setup_location_manager: Failed to setup the Location Manager.");
        service_app_exit();
    }

    if(location_manager_set_service_state_changed_cb(manager, standalone_gps_state_changed_cb, NULL) != LOCATIONS_ERROR_NONE)
    {
        dlog_print(DLOG_DEBUG, LOG_TAG, "setup_location_manager: Failed to register service_state_changed callback for the Location Manager.");
        service_app_exit();
    }

    if(location_manager_set_position_updated_cb(manager, standalone_gps_state_changed_cb, 1, NULL) != LOCATIONS_ERROR_NONE)
    {
        dlog_print(DLOG_DEBUG, LOG_TAG, "setup_location_manager: Failed to register location_updated callback for the Location Manager.");
        service_app_exit();
    }

        //THE LOGGER SHOWS THIS ON THE SCREEN
    dlog_print(DLOG_DEBUG, LOG_TAG, "setup_location_manager: Location Manager has been initialized successfully.");
}

/* Function which handles all the work of setting up and starting the location manager */
void enter_gps() {

	//int ret;

	dlog_print(DLOG_DEBUG, LOG_TAG, "Enter gps called....................");

	/* Create a location manager handle */
	//ret = location_manager_create(LOCATIONS_METHOD_HYBRID, &manager);

	/* Register a callback function for location service state changes */
	//ret = location_manager_set_service_state_changed_cb(manager, standalone_gps_state_changed_cb, NULL);


	//ret = location_manager_request_single_location(manager, 5, standalone_gps_state_changed_cb, NULL);

	/*  Start the Location Manager */
	//ret = location_manager_start(manager);

	bool existing;

	preference_is_existing(integer_key, &existing);
	if (existing == true) {
				preference_get_int(integer_key, &TestFailedAttempts);
				//preference_set_int(integer_key, FailedAttempts);
				dlog_print(DLOG_INFO, LOG_TAG,"TestFailedAttempts pref exists.");

		}else {
				preference_set_int(integer_key, 0);
				dlog_print(DLOG_INFO, LOG_TAG,"TestFailedAttempts saved first time: %d",TestFailedAttempts);

			}


	setup_location_manager();

	/*
	double d = ad.interval; // interval...
    */

	dlog_print(DLOG_DEBUG, LOG_TAG, "currently ad.interval is: %d\n", ad.interval);


	//
	if (!ecore_init()) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "ERROR: Cannot init Ecore!\n");
	}

	else {
		location_manager_start(manager);
		timer = ecore_timer_add(ad.interval, PeriodicUpdater, NULL);
		GPSTimeOut = ecore_timer_add(ad.GPS_TimeOut, PeriodicGPSTimeOut, NULL);
		//peropdicFailedAttemptsUpdater = ecore_timer_add((ad.interval + 1), PeriodicFailedAttemptsUpdater, NULL);
	}

	//dlog_print(DLOG_DEBUG, LOG_TAG, "GPS enter called...with ret: %d",ret);

}

void exit_gps() {

	/*
	 * At the end of the application, destroy all used resources, such as the location manager
	 * If you destroy the handle, there is no need to call the location_manager_stop() function to stop the service.
	 * The service is automatically stopped. Also, you do not have to unset previously set callbacks.
	 */
	location_manager_stop(manager);
	location_manager_destroy(manager);
	manager = NULL;
}



bool service_app_create(void *data)
{


	initialize_datacontrol_consumer();

	add_data_change_cb_func(NULL);

	initialize_vault_datacontrol_consumer();

	device_power_request_lock(POWER_LOCK_CPU, 5);

	bool running;
			if (app_manager_is_running(app_id, &running) == APP_MANAGER_ERROR_NONE) {
				if (!running) {
					launch_failedAttemptsUploader_service();
				}
			}

    return true;
}

void service_app_terminate(void *data)
{
	snprintf(str, sizeof(str), "");
	exit_gps();
	remove_data_change_cb_func(NULL);
	data_control_sql_destroy(ad.provider_service_h);

	device_power_release_lock(POWER_LOCK_CPU);

	bool running;
	if (app_manager_is_running(app_id, &running) == APP_MANAGER_ERROR_NONE) {
	if (running) {
					stop_service();
				}
			}

    return;
}

void service_app_control(app_control_h app_control, void *data)
{
    // Todo: add your code here.

	char *caller_id = NULL, *action_value = NULL;

		if ((app_control_get_caller(app_control, &caller_id) == APP_CONTROL_ERROR_NONE)
			&& (app_control_get_extra_data(app_control, "service_action", &action_value) == APP_CONTROL_ERROR_NONE))
		{
			if((caller_id != NULL) && (action_value != NULL)
				&& (!strncmp(caller_id, MYSERVICELAUNCHER_APP_ID, STRNCMP_LIMIT))
				&& (!strncmp(action_value,"stop", STRNCMP_LIMIT)))
			{
				ecore_thread_main_loop_end();
				ecore_shutdown();
				dlog_print(DLOG_INFO, LOG_TAG, "Stopping MyService!");
				free(caller_id);
				free(action_value);
				service_app_exit();
				return;
			}
			else
			{
				dlog_print(DLOG_INFO, LOG_TAG, "Unsupported action! Doing nothing...");
				free(caller_id);
				free(action_value);
				caller_id = NULL;
				action_value = NULL;
			}
		}
}

static void
service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	return;
}

static void
service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
    char ad[50] = {0,};
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}





void networkCall_cb(void *data)
{
	char *str = data;

	struct string s;
	init_string(&s);

	//char *temp[512];

	/*bool internet_available = false;
	connection_h connection;
	connection_type_e type;
	if (connection_create(&connection) == CONNECTION_ERROR_NONE){
		dlog_print(DLOG_DEBUG, LOG_TAG,"First seg...");
		if(connection_get_type(connection, &type) != CONNECTION_ERROR_NONE){
    		dlog_print(DLOG_DEBUG, LOG_TAG,"Second seg...");
			if(type == CONNECTION_TYPE_WIFI || type == CONNECTION_TYPE_CELLULAR){
				 internet_available = true;
		    		dlog_print(DLOG_DEBUG, LOG_TAG,"network is available. Proceeding...");
			}else {
	            internet_available = false;
				TestFailedAttempts++;
	    		dlog_print(DLOG_DEBUG, LOG_TAG,"network not available. Exiting...");
	            return 0;
	        }
		}
	}else {
		dlog_print(DLOG_DEBUG, LOG_TAG,"CONNECTION_ERROR Exists. Exiting... %s",connection);
        return 0;
    }*/


	//dlog_print(DLOG_INFO, LOG_TAG,"network call method exiting successfully with data \n %s\n", str);

	/*//if error insert and save data.
	char buf[512];

	snprintf(buf, sizeof(buf),"'%s'",str);

	int req_id = 0;
	bundle *b = bundle_create();

	bundle_add_str(b, "REQUEST", buf);

	snprintf(buf, sizeof(buf),"%d", TestFailedAttempts);
	bundle_add_str(b, "ID", buf);

	//data_control_sql_insert(ad.vault_provider_h, b, &req_id);
	bundle_free(b);
*/



	/*char *column_list[2];
	column_list[0] = "ID";
	column_list[1] = "REQUEST";


	snprintf(buf, sizeof(buf),"ID=%d", TestFailedAttempts);
	const char *where = buf;
	const char *order = "(SELECT NULL)";
	 */
	//data_control_sql_select(ad.vault_provider_h, column_list, 2, where, order, &req_id);

	/*int FailedAttempts;

	const char *integer_key = "FailedAttempts";
	bool existing;
	//FailedAttempts = TestFailedAttempts++;

	preference_is_existing(integer_key, &existing);
	if (existing == true) {
			//preference_get_int(integer_key, &FailedAttempts);
			//preference_set_int(integer_key, FailedAttempts);
			dlog_print(DLOG_INFO, LOG_TAG,"failedATT saved was: %d",TestFailedAttempts);

		}else {

			preference_set_int(integer_key, 0);
			dlog_print(DLOG_INFO, LOG_TAG,"failedATT saved first: %d",TestFailedAttempts);

		}
	 */
	//free buff
	//snprintf(buf, sizeof(buf),"");
	/*
	TestFailedAttempts++;
	dlog_print(DLOG_DEBUG, LOG_TAG, "error... emulation: %d", TestFailedAttempts);

	savedata(str, TestFailedAttempts);

	return;
	 */

	//dlog_print(DLOG_INFO, LOG_TAG,"This should not print...");


	//curl area
	CURL* curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
	curl_easy_setopt(curl, CURLOPT_URL, str);
	//curl_easy_setopt(curl, CURLOPT_URL, "https://dev.safecall.no/Webservices/PositioningTizen.ashx?lat=-17.369296&lon=-66.193078&des=&alt=2650.50&spd=0.000000&src=GPS&dat=2020-03-30T16%3a53%3a44&bat=50&vol=3&aid=NUwEIz%2fUQmaGr9wJ3EAOt1R6SUQ%3d&imei=z2wVZUYFRiiMzOYLbcecsVR6SUQ%3d11&a=&ver=0.3");
	//curl_easy_setopt(curl, CURLOPT_URL, "https://server.safecall.no/Webservices/PositioningTizen.ashx?lat=-17.369296&lon=-66.193078&des=&alt=2650.50&spd=0.000000&src=GPS&dat=2020-04-17T15%3a53%3a44&bat=50&vol=3&aid=NUwEIz%2fUQmaGr9wJ3EAOt1R6SUQ%3d&imei=z2wVZUYFRiiMzOYLbcecsVR6SUQ%3d11&ver=0.3");
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (ad.interval));
//	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	// 2020-04-17T15%3a53%3a44


	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (CURLE_OK == res) {

		dlog_print(DLOG_INFO, LOG_TAG, "Following is the server response:");

		dlog_print(DLOG_DEBUG, LOG_TAG, s.ptr);


		noInternet = false;

		}else {
			TestFailedAttempts++;
			dlog_print(DLOG_DEBUG, LOG_TAG, "error... code: %d", res);

			savedata(str, TestFailedAttempts);

			noInternet = true;
		}


		//free all other network data
		free(s.ptr);

				}

	 else {
			TestFailedAttempts++;
			dlog_print(DLOG_DEBUG, LOG_TAG, "Download failed");
			savedata(str, TestFailedAttempts);
			noInternet = true;
	 	 }


		curl_global_cleanup();

}

void savedata(char *str, int TestFailedAttempts){

	dlog_print(DLOG_DEBUG, LOG_TAG, "saving data after failed attempt. id ");

	bool existing;
	preference_is_existing(integer_key, &existing);
	if (existing == true) {
		preference_set_int(integer_key, TestFailedAttempts);
		//dlog_print(DLOG_INFO, LOG_TAG,"failedATT saved was: %d",TestFailedAttempts);

	}

	//if error insert and save data.
	char buf[512];

	snprintf(buf, sizeof(buf),"'%s'",str);

	int req_id = 0;
	bundle *b = bundle_create();

	bundle_add_str(b, "REQUEST", buf);

	snprintf(buf, sizeof(buf),"%d", TestFailedAttempts);
	bundle_add_str(b, "ID", buf);

	data_control_sql_insert(ad.vault_provider_h, b, &req_id);
	bundle_free(b);




	return;


}

/*


void networkCall_For_FailedAttempts_cb(void *data)
{
	char *str = data;

	struct string s;
	init_string(&s);

	//curl area
	CURL* curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
	curl_easy_setopt(curl, CURLOPT_URL, str);
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, (ad.interval));


	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (CURLE_OK == res) {
		canUploadFailedAttempts = true;
		dlog_print(DLOG_INFO, LOG_TAG, "Following is the server response:");

		dlog_print(DLOG_DEBUG, LOG_TAG, s.ptr);

		}else {
			canUploadFailedAttempts = false;
			TestFailedAttempts++;
			dlog_print(DLOG_DEBUG, LOG_TAG, "error... code: %d", res);

			savedata(str, TestFailedAttempts);
		}


		//free all other network data
		free(s.ptr);

				}

	 else {
			TestFailedAttempts++;
			dlog_print(DLOG_DEBUG, LOG_TAG, "Download failed");
			savedata(str, TestFailedAttempts);
	 	 }


		curl_global_cleanup();

		if (canUploadFailedAttempts) {
			peropdicFailedAttemptsUpdater = ecore_timer_add(0, PeriodicFailedAttemptsUpdater, NULL);
		}
}
*/
