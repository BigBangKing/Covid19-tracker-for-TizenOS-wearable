#ifndef __positioningservice_H__
#define __positioningservice_H__

#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "positioningservice"



#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <locations.h>
#include <device/battery.h>
#include <telephony.h>
#include <time.h>
#include <system_info.h>
#include <privacy_privilege_manager.h>
#define STRNCMP_LIMIT 256
#define MYSERVICELAUNCHER_APP_ID "no.safecall.hello"

#include <Ecore.h>

#include <curl/curl.h>
#include <net_connection.h>


struct _telephony_handle_list_s {
	unsigned int count;
	telephony_h *handle;
};
typedef struct _telephony_handle_list_s telephony_handle_lists_s;
telephony_handle_lists_s handle_list;


location_manager_h manager;
void enter_gps();
void networkCall_cb();
void networkCall_For_FailedAttempts_cb();

struct string {
  char *ptr;
  size_t len;
};

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(s->len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}



char str[2048] = { 0 };



#endif /* __positioningservice_H__ */
