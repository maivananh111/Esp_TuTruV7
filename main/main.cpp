
#include <string.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "sdkconfig.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/ledc.h"
#include "driver/touch_pad.h"
#include "hal/touch_sensor_types.h"
#include "hal/ledc_types.h"

#include "WIFI.h"
#include "FireBase.h"


#define SSID "FREE"
#define PASS "0986382835"

using namespace std;


FireBase fb;

const char *TAG = "HTTP CLIENT";
extern const char server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");

void Control_Task(void* pvparameters);
void Post_Get_Task(void *pvparameters);


extern "C"{
	void app_main(void){
		ESP_ERROR_CHECK(nvs_flash_init() );
		ESP_ERROR_CHECK(esp_netif_init());
		ESP_ERROR_CHECK(esp_event_loop_create_default());

		while(WiFi_GetState() == WIFI_CONNECT_FAILED){
			WiFi_STA_Disconnect();
			WiFi_STA_Connect((char *)SSID, (char *)PASS);
		} ESP_LOGW("LOCAL IP", "%s", LocalIP(WiFi_STA_get_netif()));

		FireBase_Auth auth_conf = {};
		auth_conf.Api_Key = "AIzaSyBm32b6Vm3_EoNtFiH-DF7aIgHiRY4UQEI";
		auth_conf.Username = "anh.iot1708@gmail.com";
		auth_conf.Password = "159852dm";
		auth_conf.Auth_Secrets = "OjRy1CXIU6cIiSZ3Dk1y91kNPZTdhPe7RPVZhJb2";
		fb.Config(&auth_conf);
		fb.Init("https://tutruv7-default-rtdb.firebaseio.com/", (const char *)server_root_cert_pem_start);

		/* **********MAIN LOOP*********** */
		xTaskCreate(&Post_Get_Task, "Post get data", 8192, NULL, 5, NULL);
		xTaskCreate(&Control_Task, "Control", 2048, NULL, 5, NULL);

	}
}

void Control_Task(void *pvparameters){

	while(1){
		ESP_LOGI(TAG, "Free heap size: %d", esp_get_free_heap_size());
		vTaskDelay(500/portTICK_PERIOD_MS);
	}
}

void Post_Get_Task(void* pvparameters){
	while(1){
		char *buf;
		asprintf(&buf, "{\"Temperature\":\"%.01f\",\"Humidity\":\"%.01f\"}", (float)((rand()%200 + 200)/10.0), (float)((rand()%300 + 700)/10.0));
		fb.SetJson("Value", buf);
		vTaskDelay(500/portTICK_PERIOD_MS);
		free(buf);
	}
}





