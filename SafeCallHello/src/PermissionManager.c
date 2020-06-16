/*
 * PermissionManager.c
 *
 *  Created on: Apr 2, 2020
 *      Author: Rifat
 */

#include "PermissionManager.h"
#include "STRUCTS.h"

//location_manager_h manager;

void app_request_response_cb(ppm_call_cause_e cause,
		ppm_request_result_e result, const char *privilege, void *user_data) {

	//appdata_s *ad = user_data;

	if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
		/* Log and handle errors */

		return;
	}

	switch (result) {
	case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
		/* Update UI and start accessing protected functionality */

		break;
	case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
		/* Show a message and terminate the application */

		break;
	case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
		/* Show a message with explanation */

		break;
	}
}

void app_check_and_request_call_permission(void *data) {
	appdata_s *ad = data;

	ppm_check_result_e result;
	const char *privilege = "http://tizen.org/privilege/call";

	int ret = ppm_check_permission(privilege, &result);
	if (ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
		switch (result) {
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
			//Update UI and start accessing protected functionality

			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
			//Show a message and terminate the application

			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
			ret = ppm_request_permission(privilege, app_request_response_cb,
					ad);
			//Log and handle errors

			break;
		}
	} else {
		//ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE
		//Handle errors

	}

}

void app_check_and_request_location_permission(void *data) {
	appdata_s *ad = data;

	ppm_check_result_e result;
	const char *privilege = "http://tizen.org/privilege/location";

	int ret = ppm_check_permission(privilege, &result);
	if (ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
		switch (result) {
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
			//Update UI and start accessing protected functionality

			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
			//Show a message and terminate the application

			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
			ret = ppm_request_permission(privilege, app_request_response_cb,
					ad);
			//Log and handle errors

			break;
		}
	} else {
		//ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE
		//Handle errors

	}

}

/*

 void
 app_request_multiple_response_cb(ppm_call_cause_e cause, ppm_request_result_e* results,
 const char **privileges, size_t privileges_count, void *user_data)
 {
 if (cause == PRIVACY_PRIVILEGE_MANAGER_CALL_CAUSE_ERROR) {
 //Log and handle errors

 return;
 }
 for (int it = 0; it < privileges_count; ++it) {
 switch (results[it]) {
 case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_ALLOW_FOREVER:
 //Update UI and start accessing protected functionality
 break;
 case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_FOREVER:
 //Show a message and terminate the application
 break;
 case PRIVACY_PRIVILEGE_MANAGER_REQUEST_RESULT_DENY_ONCE:
 // Show a message with explanation
 break;
 }
 }
 }






 //launch on first run
 void
 app_check_and_request_permissions(void *data)
 {

 appdata_s *ad = data;

 ppm_check_result_e results[2];
 const char* privileges [] = {"http://tizen.org/privilege/call",
 "http://tizen.org/privilege/location"};
 char* askable_privileges[2];
 int askable_privileges_count = 0;

 int ret = PPM ppm_check_permissions(privileges, sizeof(privileges) / sizeof(privileges[0]), results);
 if (ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
 for (int it = 0; it < sizeof(privileges) / sizeof(privileges[0]); ++it)
 {
 switch (results[it]) {
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
 //Update UI and start accessing protected functionality
 break;
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
 // Show a message and terminate the application
 break;
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
 askable_privileges[askable_privileges_count++] = privileges[it];
 //Log and handle errors
 break;
 }
 }

 ret = ppm_request_permissions(askable_privileges, askable_privileges_count, app_request_multiple_response_cb, NULL);

 } else {
 //ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE
 //Handle errors
 }
 }





 void
 check_app_permission(void *data)
 {
 appdata_s *ad = data;
 ppm_check_result_e result;
 const char *app_id = "org.example.positioningappdemo";
 const char *privilege = "http://tizen.org/privilege/call";

 int ret = ppm_check_app_permission(app_id, privilege, &result);
 if (ret == PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE) {
 switch (result) {
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
 //Update UI and start accessing protected functionality
 elm_object_text_set(ad->label, "<align=center>Update UI and start accessing protected functionality. In XXX.</align>");


 break;
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
 // Show a message and terminate the application
 elm_object_text_set(ad->label, "<align=center>Show a message and terminate the application.</align>");

 break;
 case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
 //Log and handle errors
 elm_object_text_set(ad->label, "<align=center>Log and handle its errors.</align>");
 app_check_and_request_permission(ad);
 break;
 }


 } else {
 //ret != PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE
 // Handle errors
 elm_object_text_set(ad->label, "<align=center>PRIVACY_PRIVILEGE_MANAGER_ERROR_NONE.</align>");
 }

 }


 */

