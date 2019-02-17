/*
Copyright (c) 2017 Tony Pottier

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

@file main.c
@author Tony Pottier
@brief Entry point for the ESP32 application.
@see https://idyl.io
@see https://github.com/tonyp7/esp32-wifi-manager

*/



#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "mdns.h"
#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/netdb.h"

#include "http_server_wm.h"
#include "wifi_manager.h"
#include "hap.h"
#include "driver/touch_pad.h"

#define TAG "SWITCH"
#define ACCESSORY_NAME  "SWITCH"
#define MANUFACTURER_NAME   "YOUNGHYUN"
#define MODEL_NAME  "ESP32_ACC"
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

// homekit pairing code
#define PAIRING_KEY "053-58-197" 
// needed for homekit qr-code pairing. This field is 4 positions long and can contain capital letters or digits
#define SETUP_ID "Q007"

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (90)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)



static bool s_pad_activated;
static uint16_t s_pad_init_val;

static gpio_num_t LED_PORT = GPIO_NUM_2;

static void* a;
static void* _ev_handle;
static bool led = 0;

static TaskHandle_t task_http_server = NULL;
static TaskHandle_t task_wifi_manager = NULL;

void* led_read(void* arg)
{
    printf("[MAIN] LED READ\n");
    return (void*)led;
}

void led_write(void* arg, void* value, int len)
{
    printf("[MAIN] LED WRITE. %d\n", (int)value);

    led = (bool)value;
	gpio_set_level(LED_PORT, led);

    if (_ev_handle)
        hap_event_response(a, _ev_handle, (void*)led);

    return;
}

void led_notify(void* arg, void* ev_handle, bool enable)
{
    if (enable) {
        _ev_handle = ev_handle;
    }
    else {
        _ev_handle = NULL;
    }
}

void* identify_read(void* arg)
{
    return (void*)true;
}

void hap_object_init(void* arg)
{
    void* accessory_object = hap_accessory_add(a);
    struct hap_characteristic cs[] = {
        {HAP_CHARACTER_IDENTIFY, (void*)true, NULL, identify_read, NULL, NULL},
        {HAP_CHARACTER_MANUFACTURER, (void*)MANUFACTURER_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_MODEL, (void*)MODEL_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_NAME, (void*)ACCESSORY_NAME, NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_SERIAL_NUMBER, (void*)"0123456789", NULL, NULL, NULL, NULL},
        {HAP_CHARACTER_FIRMWARE_REVISION, (void*)"1.0", NULL, NULL, NULL, NULL},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_ACCESSORY_INFORMATION, cs, ARRAY_SIZE(cs));

    struct hap_characteristic cc[] = {
        {HAP_CHARACTER_ON, (void*)led, NULL, led_read, led_write, led_notify},
    };
    hap_service_and_characteristics_add(a, accessory_object, HAP_SERVICE_SWITCHS, cc, ARRAY_SIZE(cc));
}

void initHK()
{
	printf("start hap\n");
	{
		hap_init();

		uint8_t mac[6];
		esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
		char accessory_id[32] = {0,};
		sprintf(accessory_id, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		hap_accessory_callback_t callback;
		callback.hap_object_init = hap_object_init;
		a = hap_accessory_register((char*)ACCESSORY_NAME, accessory_id, (char*)PAIRING_KEY, (char*)MANUFACTURER_NAME, HAP_ACCESSORY_CATEGORY_OTHER, 811, 1, NULL, &callback, (char*)SETUP_ID); 
	}
}

static void touch_pad_read_task(void *pvParameter)
{
	uint16_t value = 0;
    while (1){
		
		touch_pad_read_filtered(0, &value);
	
		if (value < s_pad_init_val * TOUCH_THRESH_PERCENT / 100) {

			if (s_pad_activated == false) {
				led_write(NULL,(void*)!led,0);
				printf("[MAIN] Touched. %d\n", (int)led);
			}
			s_pad_activated = true;
			vTaskDelay(200 / portTICK_PERIOD_MS);
		}
		else {
			s_pad_activated = false;
		}		
		vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


/**
 * @brief RTOS task that periodically prints the heap memory available.
 * @note Pure debug information, should not be ever started on production code!
 */
void monitoring_task(void *pvParameter)
{
	for(;;){
		printf("free heap: %d\n",esp_get_free_heap_size());
		vTaskDelay(5000 / portTICK_PERIOD_MS);
	}
}

void app_main()
{

	/* disable the default wifi logging */
	esp_log_level_set("wifi", ESP_LOG_NONE);

	/* initialize flash memory */
	nvs_flash_init();

	/* start the HTTP Server task */
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &task_http_server);

	/* start the wifi manager task */
	xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 4, &task_wifi_manager);

	/* your code should go here. In debug mode we create a simple task on core 2 that monitors free heap memory */
	
    gpio_pad_select_gpio(LED_PORT);
    gpio_set_direction(LED_PORT, GPIO_MODE_OUTPUT);
		
	touch_pad_init();
	touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
	touch_pad_config(0, TOUCH_THRESH_NO_USE);
	touch_pad_filter_start(10);
	touch_pad_read_filtered(0, &s_pad_init_val);
	
	xTaskCreate(&touch_pad_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL);
	
#if WIFI_MANAGER_DEBUG
	xTaskCreatePinnedToCore(&monitoring_task, "monitoring_task", 2048, NULL, 1, NULL, 1);
#endif
}
