#include <tizen.h>
#include <service_app.h>
#include "datacontrolservice.h"


#include <data_control.h>

#include <sqlite3.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>

//data sharing

data_control_provider_sql_cb *sql_callback;
static sqlite3* db;

/* Callback for handling the insert operation request */
void
insert_request_cb(int request_id, data_control_h provider, bundle *insert_data,
                  void *user_data)
{
    char* command = data_control_provider_create_insert_statement(provider,
                                                                  insert_data);
    int ret = sqlite3_exec(db, command, NULL, NULL, NULL);

    if (ret != SQLITE_OK) {
        data_control_provider_send_error(request_id, sqlite3_errmsg(db));
        free(command);

        return;
    }
    dlog_print(DLOG_INFO, LOG_TAG, "[insert_request_cb] insert success");

    long long inserted_row_id = sqlite3_last_insert_rowid(db);
    ret = data_control_provider_send_insert_result(request_id, inserted_row_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "insert_send_result failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "[insert_request_cb] send result success");

    free(command);
}

/* Callback for handling the select operation request */
void
select_request_cb(int request_id, data_control_h provider, const char **column_list,
                  int column_count, const char *where, const char *order,
                  void *user_data)
{
    sqlite3_stmt* sql_stmt = NULL;

    char* command = data_control_provider_create_select_statement(provider,
                                                                  column_list,
                                                                  column_count, where,
                                                                  order);
    int ret = sqlite3_prepare_v2(db, command, strlen(command), &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        data_control_provider_send_error(request_id, sqlite3_errmsg(db));
        free(command);

        return;
    }

    ret = data_control_provider_send_select_result(request_id, (void *)sql_stmt);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "select_send_result failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "[select_request_cb] send result success");

    sqlite3_finalize(sql_stmt);
    free(command);
}

/* Callback for handling the update operation request */
/*
void
update_request_cb(int request_id, data_control_h provider, bundle *update_data,
                  const char *where, void *user_data)
{
    char* command = data_control_provider_create_update_statement(provider,
                                                                  update_data, where);
    int ret = sqlite3_exec(db, command, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        data_control_provider_send_error(request_id, sqlite3_errmsg(db));
        free(command);

        return;
    }

    ret = data_control_provider_send_update_result(request_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "update_send_result failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "[update_request_cb] send result success");

    free(command);
}
*/

void
update_request_cb(int request_id, data_control_h provider, bundle *update_data,
                  const char *where, void *user_data)
{
    char* command = data_control_provider_create_update_statement(provider,
                                                                  update_data, where);
    int ret = sqlite3_exec(db, command, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        data_control_provider_send_error(request_id, sqlite3_errmsg(db));
        free(command);

        return;
    }

    ret = data_control_provider_send_update_result(request_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "update_send_result failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "[update_request_cb] send result success");

    data_control_provider_send_data_change_noti(provider,
                                                DATA_CONTROL_DATA_CHANGE_SQL_UPDATE,
                                                update_data);
    dlog_print(DLOG_INFO, LOG_TAG,
               "[send data change notification] Notify data change");

    free(command);
}


/* Callback for handling the delete operation request */
void
delete_request_cb(int request_id, data_control_h provider, const char *where,
                  void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG, "[delete_request_cb] request_id(%d)", request_id);
    char* command = data_control_provider_create_delete_statement(provider, where);
    int ret = sqlite3_exec(db, command, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        data_control_provider_send_error(request_id, sqlite3_errmsg(db));
        free(command);

        return;
    }

    ret = data_control_provider_send_delete_result(request_id);
    if (ret != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "delete_send_result failed with error: %d", ret);
    dlog_print(DLOG_INFO, LOG_TAG, "[delete_request_cb] delete success");

    free(command);
}

int
create_database()
{
	char db_path[128];
	char *res_path = app_get_data_path();
	snprintf(db_path, sizeof(db_path), "%s%s", res_path, "settings.db");

    dlog_print(DLOG_INFO, LOG_TAG, "%s%s", app_get_data_path(), "settings.db");

    int open_flags = (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    int ret = sqlite3_open_v2(db_path, &db, open_flags, NULL);
    if (ret != SQLITE_OK) {
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "database creation failed with error: %d", ret);

        return ret;
    }

    //char* sql_command = "CREATE TABLE IF NOT EXISTS Dictionary (WORD VARCHAR(30), WORD_DESC TEXT, WORD_NUM INT, Point INT)";
    char* sql_command = "CREATE TABLE IF NOT EXISTS Dictionary (URL VARCHAR(255), INTERVALTIME INT, SOS INT, GPSTIMEOUT INT)";
    ret = sqlite3_exec(db, sql_command, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "database table creation failed with error: %d", ret);

   /* sql_command = "CREATE TABLE IF NOT EXISTS Note (TITLE VARCHAR(30), CONTENTS TEXT)";
    ret = sqlite3_exec(db, sql_command, NULL, NULL, NULL);
    if (ret != SQLITE_OK)
        dlog_print(DLOG_ERROR, LOG_TAG,
                   "database table creation failed with error: %d", ret);
    */
    dlog_print(DLOG_INFO, LOG_TAG, "DB init Success.");

    return ret;
}

void
initialize_datacontrol_provider()
{
    dlog_print(DLOG_INFO, LOG_TAG, "initialize_datacontrol_provider");

    int result = create_database();
    if (result != SQLITE_OK)
        return;

    sql_callback = (data_control_provider_sql_cb *)malloc(sizeof(data_control_provider_sql_cb));
    sql_callback->select_cb = select_request_cb;
    sql_callback->insert_cb = insert_request_cb;
    sql_callback->delete_cb = delete_request_cb;
    sql_callback->update_cb = update_request_cb;
    result = data_control_provider_sql_register_cb(sql_callback, NULL);
    if (result != DATA_CONTROL_ERROR_NONE)
        dlog_print(DLOG_ERROR,
                   "data_control_sql_response_c failed with error: %d", result);
    else
        dlog_print(DLOG_INFO, LOG_TAG, "Provider SQL register success");
}




//data change stuff



bool
change_noti_consumer_list_cb(data_control_h provider, char *consumer_appid,
                             void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG,
               "Added change noti consumer appid: %s", consumer_appid);

    return true;
}

bool
consumer_filter_cb_1(data_control_h provider, char *consumer_appid, void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG,
               "consumer appid %s try to add data change callback", consumer_appid);
    if (strcmp(consumer_appid, "org.tizen.helloworld_consumer2") == 0) {
        dlog_print(DLOG_INFO, LOG_TAG, "Invalid appid: %s", consumer_appid);

        return false;
    }
    data_control_provider_foreach_data_change_consumer(provider,
                                                       &change_noti_consumer_list_cb,
                                                       NULL);

    return true;
}

bool
consumer_filter_cb_2(data_control_h provider, char *consumer_appid, void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG,
               "consumer appid %s try to add data change callback", consumer_appid);
    if (strcmp(consumer_appid, "org.tizen.helloworld_consumer3") == 0) {
        dlog_print(DLOG_INFO, LOG_TAG, "Invalid appid: %s", consumer_appid);

        return false;
    }

    return true;
}

/* Add the filter for the accepted callback registration */
int filter_callback_id_1;
int filter_callback_id_2;
void
add_consumer_filter_cb_func(void *data, Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
    data_control_provider_add_data_change_consumer_filter_cb(consumer_filter_cb_1,
                                                             NULL,
                                                             &filter_callback_id_1);
    dlog_print(DLOG_INFO, LOG_TAG,
               "filter_callback_id_1 id: %d", filter_callback_id_1);

    data_control_provider_add_data_change_consumer_filter_cb(consumer_filter_cb_2,
                                                             NULL,
                                                             &filter_callback_id_2);
    dlog_print(DLOG_INFO, LOG_TAG,
               "filter_callback_id_2 id: %d", filter_callback_id_2);
}

/* Remove the filter */
void
remove_consumer_filter_cb_func(void *data, Evas_Object *obj EINA_UNUSED,
                               void *event_info EINA_UNUSED)
{
    data_control_provider_remove_data_change_consumer_filter_cb(filter_callback_id_1);
    dlog_print(DLOG_INFO, LOG_TAG, "remove callback %d", filter_callback_id_1);

    data_control_provider_remove_data_change_consumer_filter_cb(filter_callback_id_2);
    dlog_print(DLOG_INFO, LOG_TAG, "remove callback %d", filter_callback_id_2);
}







bool service_app_create(void *data)
{
	initialize_datacontrol_provider();

    // Todo: add your code here.
    return true;
}

void service_app_terminate(void *data)
{
    // Todo: add your code here.
    return;
}

void service_app_control(app_control_h app_control, void *data)
{
    // Todo: add your code here.
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
