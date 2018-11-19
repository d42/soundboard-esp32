#include "esp_system.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

static const char *TAG = "buttons :^)";

static xQueueHandle gpio_evt_queue = NULL;

static int pin_values[40];
static int *PINS;
static int PIN_COUNT;
#define ESP_INTR_FLAG_DEFAULT 0

static void (*CALLBACK)(int, int);

int get_pin_num(int io_num) {
    for (int i = 0; i < PIN_COUNT; ++i) {
        if(io_num == PINS[i]) {
            return i;
        }
    }
    return -1;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void buttons_task(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            int pin_level = gpio_get_level(io_num);
            pin_level = pin_level ? 0 : 1;
            if(pin_values[io_num] == pin_level) {
                continue;
            }
            pin_values[io_num] = pin_level;
            int pin_num = get_pin_num(io_num);
            (*CALLBACK)(pin_num, pin_level);
        }
    }
}

void setup_pins() {
    printf("Setup pins...\n");
    unsigned long long pin_mask = 0;
    for (int i = 0; i < PIN_COUNT; ++i) {
        pin_mask |= (1ULL << PINS[i]);
    }

    ESP_LOGI(TAG, "pin mask = 0x%.8llX\n", pin_mask);
    gpio_config_t io_conf;
    //interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    //bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = pin_mask;
    //set as input mode    
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(buttons_task, "buttons_task", 1024*8, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    //hook isr handler for specific gpio pin
    for (int i = 0; i < PIN_COUNT; ++i) {
        gpio_isr_handler_add(PINS[i], gpio_isr_handler, (void*) PINS[i]);
    }
}

void setup_buttons(int *pins, int pin_count, void(*callback)(int, int)) {
    PINS = pins;
    PIN_COUNT = pin_count;
    ESP_LOGI(TAG, "registering %d pins", pin_count);
    CALLBACK = callback;
    setup_pins();
}

