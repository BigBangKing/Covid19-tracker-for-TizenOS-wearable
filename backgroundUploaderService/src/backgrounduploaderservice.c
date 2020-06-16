#include <tizen.h>
#include <service_app.h>
#include "backgrounduploaderservice.h"

#include <stdlib.h>
#include <data_control.h>
#define STRNCMP_LIMIT 256
#define MYSERVICELAUNCHER_APP_ID "no.safecall.positioningservice"

void savedata(char *str );
void networkCall_For_FailedAttempts_cb(void *data);
void failedAttemptsDecrementer();

data_control_h *vault_service_h;
const char *integer_key = "FailedAttempts";
const char *integer_lastKnown = "lastKnownFailedAttempts";
data_control_sql_response_cb sql_callback;


int FailedAttempts, lastKnownFailedAttempts;
bool canDecrement, noInternet = false;
char STR[512] = {0,};



void
_thread_end_cb(void *data, Ecore_Thread *th)
{
  //App_Data *ad = data;

  dlog_print(DLOG_DEBUG, LOG_TAG,"(b)Normal termination for thread %p.\n", th);
  if (th != NULL)
	  th = NULL;

}

//network related


void upload_failed_attempts_again_job(void *data EINA_UNUSED, Ecore_Thread *th)
{

	char buf[512];
	int req_id = 0;

	 char *column_list[2];
	 column_list[0] = "ID";
	 column_list[1] = "REQUEST";


	 snprintf(buf, sizeof(buf),"ID=%d", FailedAttempts);
	 const char *where = buf;
	 const char *order = "(SELECT NULL)";


	 dlog_print(DLOG_INFO, LOG_TAG,"Selecting next stored request with ID: %d", FailedAttempts);

	 data_control_sql_select(vault_service_h, column_list, 2, where, order, &req_id);


	//free buf
	snprintf(buf, sizeof(buf),"");

}


Eina_Bool FailedAttemptsUploader(void *data EINA_UNUSED) {

					if (FailedAttempts > 0) {

						ecore_thread_run(upload_failed_attempts_again_job, _thread_end_cb, _thread_end_cb, NULL);

					}else {
						dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts logged Now. All is good.");
						preference_set_int(integer_lastKnown, FailedAttempts);
					}

					//dlog_print(DLOG_INFO, LOG_TAG,"No FailedAttempts logged Now. All is good.");


		return ECORE_CALLBACK_CANCEL;

}



void again_upload_to_server_job(void *data EINA_UNUSED, Ecore_Thread *th)
{

	ecore_timer_del(FailedAttemptsUploader);

	char *str = data;

	dlog_print(DLOG_INFO, LOG_TAG,"This is the thread for uploading data...");

	Ecore_Job *job;
	job = ecore_job_add(networkCall_For_FailedAttempts_cb, str);

	(void)job;

	//networkCall_cb(str);

}




/* Callback for the select operation response from vault service*/
void sql_select_vault_response_cb(int request_id, data_control_h provider,
		result_set_cursor cursor, bool provider_result, const char *error,
		void *user_data) {


	if (provider_result) {
			dlog_print(DLOG_INFO, LOG_TAG, "(back)The vault select operation is successful");
	} else {
		dlog_print(DLOG_ERROR, LOG_TAG, "The select operation for the request %d failed. error message: %s",
					request_id, error);
		//nothing work on. Exiting.
		service_app_exit();

		}


		//int totalUploadableRequests = 1;

		while (data_control_sql_step_next(cursor) == DATA_CONTROL_ERROR_NONE) {

			 char REQUEST[512] = {0,};
			 int ID = 0;

			data_control_sql_get_int_data(cursor, 0, &ID);
			data_control_sql_get_text_data(cursor, 1, REQUEST);

			dlog_print(DLOG_INFO, LOG_TAG, "(back)ID: %d, REQUEST: %s.", ID, REQUEST );

			//make network call again... Its fine as it gets one id'ed call at a time.
			//then delete from database if succeed.

			// make network call thread.

			if (ID != 0 ) {

				dlog_print(DLOG_INFO, LOG_TAG, "(back)Continuing with normal operation.");
				snprintf(STR, sizeof(REQUEST), REQUEST);

				ecore_thread_run(again_upload_to_server_job, _thread_end_cb, _thread_end_cb, STR);
			}else {

				if (FailedAttempts > 0) {

									dlog_print(DLOG_INFO, LOG_TAG, "(back)decrementing after getting a 0th id.");
									failedAttemptsDecrementer();
									ecore_timer_add(3, FailedAttemptsUploader, NULL);
								}else {
									//nothing to work on. exit.
									service_app_exit();
								}
			}



		}

}

void
sql_insert_response_cb(int request_id, data_control_h provider,
                       long long inserted_row_id, bool provider_result,
                       const char *error, void *user_data)
{
    if (provider_result) {
        dlog_print(DLOG_INFO, LOG_TAG, "The insert operation is successful");
        if (noInternet) {
        	dlog_print(DLOG_ERROR, LOG_TAG, "noInternet. Exiting.");
        	service_app_exit();
        }
    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The insert operation for the request %d failed. error message: %s",
                   request_id, error);

		service_app_exit();
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

        if (FailedAttempts > 0) {

            		    dlog_print(DLOG_INFO, LOG_TAG, "Deleted last attempts for backgroundUploader. With id: %d", (FailedAttempts+1));

            			ecore_timer_add(1, FailedAttemptsUploader, NULL);

            			int temp;
            			bool existing,existing1;
            			preference_is_existing(integer_key, &existing);
            			preference_is_existing(integer_lastKnown, &existing1);
            				if (existing == true) {
            						preference_get_int(integer_key, &temp);
            						if (temp == lastKnownFailedAttempts) {
            							preference_set_int(integer_key, FailedAttempts);
            							dlog_print(DLOG_DEBUG, LOG_TAG,"FailedAttempts has been decremented by 1, All is good.");
            						}

        			}else {

            		    dlog_print(DLOG_INFO, LOG_TAG, "No FailedAttempts, exiting backgroundUploader.");
        				service_app_exit();
        			}


    } else {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "The delete operation for the request %d failed. error message: %s",
                   request_id, error);
    }
}

}

void
initialize_datacontrol_consumer()
{
    int ret;

    const char *provider_id = "http://vaultservice.com/datacontrol/provider/vaultservice";
    const char *data_id = "Vault";

    /* Create data control handler */
    ret = data_control_sql_create(&(vault_service_h));
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "creating data control provider failed with error: %d", ret);

    ret = data_control_sql_set_provider_id(vault_service_h, provider_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "setting provider id failed with error: %d", ret);

    ret = data_control_sql_set_data_id(vault_service_h, data_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG, "setting data id failed with error: %d", ret);

    /* Set response callbacks */
    sql_callback.delete_cb = sql_delete_response_cb;
    sql_callback.insert_cb = sql_insert_response_cb;
    sql_callback.select_cb = sql_select_vault_response_cb;
    sql_callback.update_cb = sql_update_response_cb;

    /* Register response callbacks */
    ret = data_control_sql_register_response_cb(vault_service_h, &sql_callback, NULL);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "Registering the callback function failed with error: %d", ret);

    dlog_print(DLOG_INFO, LOG_TAG, "Init data control success for backgroundUploader.");


    bool existing,existing1;
    preference_is_existing(integer_key, &existing);
    preference_is_existing(integer_lastKnown, &existing1);

    if (existing == true) {
    		preference_get_int(integer_key, &FailedAttempts);
    		if (existing1 != true) {
    				preference_set_int(integer_lastKnown, FailedAttempts);
    			}else {
    				preference_get_int(integer_lastKnown, &lastKnownFailedAttempts);
				}
    		if (FailedAttempts > 0) {
    			lastKnownFailedAttempts = FailedAttempts;
				preference_set_int(integer_lastKnown, FailedAttempts);
    		    dlog_print(DLOG_ERROR, LOG_TAG, "started service back.");
    		    noInternet = false;
    			ecore_timer_add(1, FailedAttemptsUploader, NULL);

			}else {

    		    dlog_print(DLOG_ERROR, LOG_TAG, "No FailedAttempts, exiting backgroundUploader.");
				service_app_exit();
			}


    	}else {
    	    dlog_print(DLOG_ERROR, LOG_TAG, "this preference doesn't exist, backgroundUploader.");

		}


}








//app related

bool service_app_create(void *data)
{

	initialize_datacontrol_consumer();
	dlog_print(DLOG_INFO, LOG_TAG, "started backgroundUploaderService!");

	//ecore_timer_add(3, FailedAttemptsUploader, NULL);

    return true;
}

void service_app_terminate(void *data)
{
	snprintf(STR, sizeof(STR), "");
	data_control_sql_destroy(vault_service_h);
	dlog_print(DLOG_ERROR, LOG_TAG, "exiting backgroundUploaderService!");
    return;
}

void service_app_control(app_control_h app_control, void *data)
{
		char *caller_id = NULL, *action_value = NULL;

			if ((app_control_get_caller(app_control, &caller_id) == APP_CONTROL_ERROR_NONE)
				&& (app_control_get_extra_data(app_control, "service_action", &action_value) == APP_CONTROL_ERROR_NONE))
			{
				if((caller_id != NULL) && (action_value != NULL)
					&& (!strncmp(caller_id, MYSERVICELAUNCHER_APP_ID, STRNCMP_LIMIT))
					&& (!strncmp(action_value,"stop", STRNCMP_LIMIT)))
				{

					dlog_print(DLOG_INFO, LOG_TAG, "Stopping backgroundUploaderService!");
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

    return;
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







void failedAttemptsDecrementer(){
	FailedAttempts--;



	if (FailedAttempts == 0) {
		int temp;
		bool existing,existing1;
		 preference_is_existing(integer_key, &existing);
		 preference_is_existing(integer_lastKnown, &existing1);
		if (existing == true) {
			preference_get_int(integer_key, &temp);
			if (existing1 == true)
			preference_get_int(integer_lastKnown, &lastKnownFailedAttempts);
			if (temp == lastKnownFailedAttempts) {
				preference_set_int(integer_lastKnown, FailedAttempts);
				preference_set_int(integer_key, FailedAttempts);
				dlog_print(DLOG_DEBUG, LOG_TAG,"FailedAttempts has been reset to 0, All is good.");
				service_app_exit();
			}else {
				service_app_exit();
			}
		}

		//if 0 = no failed attempts -> exit.
		service_app_exit();
	}else{
		int req_id = 0;
		char buf[10];
		snprintf(buf, sizeof(buf), "ID = %d", (FailedAttempts+1));
		data_control_sql_delete(vault_service_h, buf, &req_id);
	}
}





void networkCall_For_FailedAttempts_cb(void *data)
{

	dlog_print(DLOG_DEBUG, LOG_TAG, "Failed attempts uploading now. This is good.");

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
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);


	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	if (CURLE_OK == res) {
		dlog_print(DLOG_INFO, LOG_TAG, "Printing str: %s", str);

		dlog_print(DLOG_INFO, LOG_TAG, "Following is the server response: (back)");

		dlog_print(DLOG_DEBUG, LOG_TAG, s.ptr);

		canDecrement = true;

		}else {
			canDecrement = false;
			dlog_print(DLOG_DEBUG, LOG_TAG, "(back)error... code: %d", res);

			savedata(str);

			dlog_print(DLOG_INFO, LOG_TAG, "Printing str: %s", str);
		}


		//free all other network data
		free(s.ptr);

				}

	 else {
			canDecrement = false;
			dlog_print(DLOG_DEBUG, LOG_TAG, "(back)Download failed");
			savedata(str);
			noInternet = true;
	 	 }


		curl_global_cleanup();

		if (canDecrement) {
			noInternet = false;
			failedAttemptsDecrementer();
			ecore_timer_add(0, FailedAttemptsUploader, NULL);
		}
}


void savedata(char *str){

	dlog_print(DLOG_DEBUG, LOG_TAG, "saving data after another failed attempt. id : %d",FailedAttempts);


	//if error insert and save data.
	char buf[512];

	snprintf(buf, sizeof(buf),"'%s'",str);

	int req_id = 0;
	bundle *b = bundle_create();

	bundle_add_str(b, "REQUEST", buf);

	snprintf(buf, sizeof(buf),"%d", FailedAttempts);
	bundle_add_str(b, "ID", buf);

	data_control_sql_insert(vault_service_h, b, &req_id);
	bundle_free(b);

	//probably theres no internet. So we had to save it again in here. So its better to exit.
	noInternet = true;

	return;


}
