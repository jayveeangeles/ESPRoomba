#include "coap_task.h"

static const char *TAG = "CoAP_server";

// static const char safeOp[] = { SAFE_OP };
// static const char fullOp[] = { FULL_OP };
// static const char spotOp[] = { SPOT_OP };
// static const char cleanOp[] = { CLEAN_OP };
// static const char maxOp[] = { MAX_OP };
// static const char seekDockOp[] = { SEEK_DOCK_OP };

struct coap_resource_t *globalResource = NULL;
PRIVILEGED_DATA portMUX_TYPE xTaskQueueMutex = portMUX_INITIALIZER_UNLOCKED;

int str_splitter(       const unsigned char *s,
                                                            size_t length,
                                                            unsigned char **name,
                                                            unsigned char **val,
                  unsigned char token) {

  size_t counter = 0;
  const unsigned char *p;

  p = s;

  while (counter < length) {
    if (*p == token) {
      *name = (unsigned char* )pvPortMalloc(counter + 1);
      memset(*name, '\0', counter + 1);
      memcpy(*name, s, counter);

      *val = (unsigned char* )pvPortMalloc(length - counter);
      memset(*val, '\0', length - counter);
      memcpy(*val, p + 1, length - counter - 1);
      break;
    }

    ++p;
    ++counter;
  }
  return 0;
};

/*
 * Schedule Cleaning handler
 */
static void
get_sked_handler(coap_context_t *ctx, 
								struct coap_resource_t *resource,
	              const coap_endpoint_t *local_interface, 
	              coap_address_t *peer,
	              coap_pdu_t *request, 
	              str *token, 
	              coap_pdu_t *response) {
	(void)request;
	unsigned char buf[40];
	response->hdr->code = COAP_RESPONSE_CODE(205);

	cJSON *json = cJSON_Parse((const char *)request->data);
	if (json == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    response->hdr->code = COAP_RESPONSE_CODE(500);
    if (error_ptr != NULL) {
      ESP_LOGW(TAG, "Error before: %s", error_ptr);
    }
    goto end;
  }

	char *commandString = (char* )pvPortMalloc(20);
	memset(commandString, 0, 20);
  commandString[0] = SCHEDULE_OP;

  const cJSON *hr = NULL;
  const cJSON *mn = NULL;

  const cJSON *sunSked = cJSON_GetObjectItemCaseSensitive(json, "sun");
  if (cJSON_IsObject(sunSked)) {
  	commandString[1] = commandString[1] | BIT0;
  	hr = cJSON_GetObjectItemCaseSensitive(sunSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(sunSked, "mn");
    commandString[2] = hr->valuedouble;
    commandString[3] = mn->valuedouble;
  }
  const cJSON *monSked = cJSON_GetObjectItemCaseSensitive(json, "mon");
  if (cJSON_IsObject(monSked)) {
  	commandString[1] = commandString[1] | BIT1;
  	hr = cJSON_GetObjectItemCaseSensitive(monSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(monSked, "mn");
    commandString[4] = hr->valuedouble;
    commandString[5] = mn->valuedouble;
  }
  const cJSON *tueSked = cJSON_GetObjectItemCaseSensitive(json, "tue");
  if (cJSON_IsObject(tueSked)) {
  	commandString[1] = commandString[1] | BIT2;
  	hr = cJSON_GetObjectItemCaseSensitive(tueSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(tueSked, "mn");
    commandString[6] = hr->valuedouble;
    commandString[7] = mn->valuedouble;
  }
  const cJSON *wedSked = cJSON_GetObjectItemCaseSensitive(json, "wed");
  if (cJSON_IsObject(wedSked)) {
  	commandString[1] = commandString[1] | BIT3;
  	hr = cJSON_GetObjectItemCaseSensitive(wedSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(wedSked, "mn");
    commandString[8] = hr->valuedouble;
    commandString[9] = mn->valuedouble;
  }
  const cJSON *thuSked = cJSON_GetObjectItemCaseSensitive(json, "thu");
  if (cJSON_IsObject(thuSked)) {
  	commandString[1] = commandString[1] | BIT4;
  	hr = cJSON_GetObjectItemCaseSensitive(thuSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(thuSked, "mn");
    commandString[10] = hr->valuedouble;
    commandString[11] = mn->valuedouble;
  }
  const cJSON *friSked = cJSON_GetObjectItemCaseSensitive(json, "fri");
  if (cJSON_IsObject(friSked)) {
  	commandString[1] = commandString[1] | BIT5;
  	hr = cJSON_GetObjectItemCaseSensitive(friSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(friSked, "mn");
    commandString[12] = hr->valuedouble;
    commandString[13] = mn->valuedouble;
  }
  const cJSON *satSked = cJSON_GetObjectItemCaseSensitive(json, "sat");
  if (cJSON_IsObject(satSked)) {
  	commandString[1] = commandString[1] | BIT6;
  	hr = cJSON_GetObjectItemCaseSensitive(satSked, "hr");
    mn = cJSON_GetObjectItemCaseSensitive(satSked, "mn");
    commandString[14] = hr->valuedouble;
    commandString[15] = mn->valuedouble;
  }

  xQueueSend( xCommandsQueue, ( void * ) commandString, pdMS_TO_TICKS(1000) );

end:
  cJSON_Delete(json);

  coap_add_option(response,
                  COAP_OPTION_CONTENT_FORMAT,
                  coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  coap_encode_var_bytes(buf, 0x01), buf);

  coap_add_data(response, 4, (const unsigned char*)"done");
};

/*
 * Simple commands handler
 */
static void
get_simple_cmd_handler(coap_context_t *ctx, 
								struct coap_resource_t *resource,
	              const coap_endpoint_t *local_interface, 
	              coap_address_t *peer,
	              coap_pdu_t *request, 
	              str *token, 
	              coap_pdu_t *response) {
  (void)request;
  unsigned char buf[40];

  coap_opt_iterator_t opt_iter;
  coap_opt_filter_t f = {};
  coap_opt_t *q;

  unsigned char *name = NULL;
  unsigned char *val = NULL;

  response->hdr->code = COAP_RESPONSE_CODE(205);

  coap_add_option(response,
                  COAP_OPTION_CONTENT_FORMAT,
                  coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  coap_encode_var_bytes(buf, 0x01), buf);

  coap_option_filter_clear(f);
  coap_option_setb(f, COAP_OPTION_URI_QUERY);
  coap_option_iterator_init(request, &opt_iter, f);

  while ((q = coap_option_next(&opt_iter)) != NULL) {
    unsigned char *value = coap_opt_value(q);
    size_t size = coap_opt_length(q);
    str_splitter(value, size, &name, &val, '=');
  }

	char *pxMessage = (char* )pvPortMalloc(20);
	memset(pxMessage, 0, 20);

  if ( memcmp(name, "mode", 4) == 0 ) {
  	if ( memcmp(val, "safe", 4) == 0 ) {
  		pxMessage[0] = SAFE_OP;
  	} else if ( memcmp(val, "full", 4) == 0 ) {
  		pxMessage[0] = FULL_OP;
  	} else if ( memcmp(val, "start", 4) == 0 ) {
      pxMessage[0] = START_OP;
    }
  } else if ( memcmp(name, "clean", 5) == 0 ) {
  	if ( memcmp(val, "spot", 4) == 0 ) {
  		pxMessage[0] = SPOT_OP;
  	} else if ( memcmp(val, "clean", 5) == 0 ) {
  		pxMessage[0] = CLEAN_OP;
  	} else if ( memcmp(val, "max", 3) == 0 ) { 
  		pxMessage[0] = MAX_OP;
  	} else if ( memcmp(val, "dock", 4) == 0 ) { 
  		pxMessage[0] = SEEK_DOCK_OP;
  	}
  }

  // ESP_LOGI(TAG, "name->%s val->%s pxMessage->%d", name, val, pxMessage[0]);

  xQueueSend( xCommandsQueue, ( void * ) pxMessage, pdMS_TO_TICKS(1000) );

  vPortFree(name);
  vPortFree(val);
  vPortFree(pxMessage);

  coap_add_data(response, 4, (const unsigned char*)"done");
};

/*
 * Drive Roomba
 */
static void
get_drive_handler(coap_context_t *ctx,
                  struct coap_resource_t *resource,
                  const coap_endpoint_t *local_interface,
                  coap_address_t *peer,
                  coap_pdu_t *request,
                  str *token,
                  coap_pdu_t *response) {
  (void)request;
  unsigned char buf[40];
  response->hdr->code = COAP_RESPONSE_CODE(205);

  unsigned char *left = NULL;
  unsigned char *right = NULL;

  str_splitter(request->data, strlen((const char*)request->data), &left, &right, ',');

  char *ptrLeft;
  char *ptrRight;
  uint16_t leftSpeed = (uint16_t) strtol((const char*)left, &ptrLeft, 10);
  uint16_t rightSpeed = (uint16_t) strtol((const char*)right, &ptrRight, 10);
  uint16_t hiRight = (rightSpeed & 0xFF00) >> 8;
  uint16_t loRight = (rightSpeed & 0xFF);
  uint16_t hiLeft = (leftSpeed & 0xFF00) >> 8;
  uint16_t loLeft = (leftSpeed & 0xFF);

  char *pxMessage = (char* )pvPortMalloc(20);
  memset(pxMessage, 0, 20);

  pxMessage[0] = DRIVE_PWM_OP;
  pxMessage[1] = hiRight;
  pxMessage[2] = loRight;
  pxMessage[3] = hiLeft;
  pxMessage[4] = loLeft;

  xQueueSend( xCommandsQueue, ( void * ) pxMessage, pdMS_TO_TICKS(1000) );

  coap_add_option(response,
                  COAP_OPTION_CONTENT_FORMAT,
                  coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  coap_encode_var_bytes(buf, 0x01), buf);

  coap_add_data(response, 4, (const unsigned char*)"done");

  vPortFree(left);
  vPortFree(right);
  vPortFree(pxMessage);
};

/*
 * Observer handler
 */
static void
get_obs_handler(coap_context_t *ctx, 
								struct coap_resource_t *resource,
	              const coap_endpoint_t *local_interface, 
	              coap_address_t *peer,
	              coap_pdu_t *request, 
	              str *token, 
	              coap_pdu_t *response) {
	(void)request;
	unsigned char buf[40];

	response->hdr->code = COAP_RESPONSE_CODE(205);

  coap_add_option(response,
	                COAP_OPTION_OBSERVE,
	                coap_encode_var_bytes(buf, ctx->observe), buf);

	coap_add_option(response,
	                COAP_OPTION_CONTENT_FORMAT,
	                coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

	coap_add_option(response,
	                COAP_OPTION_MAXAGE,
	                coap_encode_var_bytes(buf, 0x01), buf);


	// ESP_LOGI(TAG, "observer");

	vTaskEnterCritical(&xTaskQueueMutex);
	if (sensorData != NULL) {
		coap_add_data(response, RD_BUF_SIZE, (const unsigned char*)sensorData);
	} else {
		coap_add_data(response, 4, (const unsigned char*)"NULL");
	}
	// globalResource->dirty = pdFALSE;
	vTaskExitCritical(&xTaskQueueMutex);
};

static void resourceInit(coap_context_t* ctx) {
	struct coap_resource_t *r;

  r = coap_resource_init((unsigned char *)"sc", 2, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, get_simple_cmd_handler);
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Simple Commands Handler\"", 23, 0);
  coap_add_resource(ctx, r);

  r = coap_resource_init((unsigned char *)"sked", 4, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, get_sked_handler);
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Cleaning Schedule Handler\"", 25, 0);
  coap_add_resource(ctx, r);

  r = coap_resource_init((unsigned char *)"drive", 5, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, get_drive_handler);
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Drive Handler\"", 13, 0);
  coap_add_resource(ctx, r);

  r = coap_resource_init((unsigned char *)"obs", 3, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  coap_register_handler(r, COAP_REQUEST_GET, get_obs_handler);
  coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Observer Handler\"", 16, 0);
  r->observable = 1;
  coap_add_resource(ctx, r);

	vTaskEnterCritical(&xTaskQueueMutex);
  globalResource = r;
	vTaskExitCritical(&xTaskQueueMutex);
  // return resource;
};

void vCoapServer(void *p) {
	coap_context_t*  ctx = NULL;
  coap_address_t   serv_addr;
  // coap_resource_t* resource = NULL;
  fd_set           readfds;
  struct timeval tv;
  int flags = 0;
  coap_queue_t *nextpdu;
  coap_tick_t now;

	while (1) {
		uint32_t ulNotifiedValue;
		xTaskNotifyWait( 0x00,      			 /* Don't clear any notification bits on entry. */
		                 0x00,				 /* Reset the notification value to 0 on exit. */
		                 &ulNotifiedValue, /* Notified value pass out in
		                                      ulNotifiedValue. */
		                 pdMS_TO_TICKS(10000) );  /* Block for 10 seconds. */

		if (( ulNotifiedValue & NOT_CONNECTED_INET ) == 1) {
	    ESP_LOGE(TAG, "WiFi: Not connected!");
	    vTaskDelete(NULL);
	    return;
		}

	  /* Prepare the CoAP server socket */
	  coap_address_init(&serv_addr);
	  serv_addr.addr.sin.sin_family      = AF_INET;
	  serv_addr.addr.sin.sin_addr.s_addr = INADDR_ANY;
	  serv_addr.addr.sin.sin_port        = htons(COAP_DEFAULT_PORT);
	  ctx                                = coap_new_context(&serv_addr);

	  if (ctx == NULL) {
	    ESP_LOGE(TAG, "Cannot create new coap context!");
	    vTaskDelete(NULL);
	    return;
	  }

	  flags = fcntl(ctx->sockfd, F_GETFL, 0);
	  fcntl(ctx->sockfd, F_SETFL, flags|O_NONBLOCK);

	  tv.tv_usec = COAP_DEFAULT_TIME_USEC;
	  tv.tv_sec = COAP_DEFAULT_TIME_SEC;
	  /* Initialize the resource */
	  // struct coap_resource_t *sensor_resource = NULL;
	  // sensor_resource = resourceInit(ctx);
	  resourceInit(ctx);

	  /*For incoming connections*/
	  for (;;) {
	    FD_ZERO(&readfds);
	    FD_CLR( ctx->sockfd, &readfds);
	    FD_SET( ctx->sockfd, &readfds);

	    nextpdu = coap_peek_next( ctx );

	    coap_ticks(&now);
	    while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
	      coap_retransmit( ctx, coap_pop_next( ctx ) );
	      nextpdu = coap_peek_next( ctx );
	    }

	    int result = select( ctx->sockfd+1, &readfds, 0, 0, &tv );
	    if (result > 0){
	      if (FD_ISSET( ctx->sockfd, &readfds )){
	        coap_read(ctx);
	      }
	    } else if (result < 0){
	      break;
	    } else {
	      // ESP_LOGE(TAG, "select timeout");
	    }

	    coap_check_notify(ctx);

	  }

	  coap_free_context(ctx);
	}
	vTaskDelete(NULL);
};

