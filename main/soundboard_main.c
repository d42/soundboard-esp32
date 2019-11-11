/* Hello World Example
 * 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "mqtt_client.h"
#include "soc/soc.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "freertos/portmacro.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "buttons.h"
#include "wifi.h"

static const char *TAG = "main";
static esp_mqtt_client_handle_t mqtt_client;

static int PINS[] = {
    13, 26, 33, 15,
    12, 27, 19, 4,
    14, 25, 32, 22,
};


void button_callback(int btn_num, int btn_val) {
    printf("BTN %d : %d\n", btn_num, btn_val);
    char data[32];
    snprintf(data, 32, "%d=%d", btn_num, btn_val);
    esp_mqtt_client_publish(mqtt_client, CONFIG_MQTT_TOPIC, data, 0, 0, 0);

}
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

static esp_mqtt_client_handle_t setup_mqtt(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_MQTT_SERVER,
        .event_handle = mqtt_event_handler,
        .username = CONFIG_MQTT_LOGIN,
        .password = CONFIG_MQTT_PASSWORD
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    return client;
}

void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    setup_wifi();
    mqtt_client = setup_mqtt();
    setup_buttons(PINS, sizeof(PINS)/sizeof(int), &button_callback);
}
