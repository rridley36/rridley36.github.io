/* --------------------------------------------------------------
   Application 6
   Class: Real Time Systems - Spring 2026
   Author: Russell Ridley
   AI Use: Integration of buzzer if BPM is above 140 BPM
---------------------------------------------------------------*/

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/ledc.h"

// Healthcare monitoring theme:
// Green LED = patient monitor heartbeat/status
// Red LED = emergency alert
// Button = nurse call button
// Potentiometer = simulated heart rate sensor
// Buzzer = continuous patient alarm tone

#define LED_GREEN GPIO_NUM_5
#define LED_RED   GPIO_NUM_4
#define BUTTON_PIN GPIO_NUM_18
#define BUZZER_PIN GPIO_NUM_23
#define POT_ADC_CHANNEL ADC1_CHANNEL_6   // GPIO34

#define MAX_COUNT_SEM 300
#define BPM_ALERT_THRESHOLD 140
#define BPM_MAX 200

#define BUZZER_FREQ 2000                 // 2 kHz tone // AI Generated
#define BUZZER_RES LEDC_TIMER_10_BIT      // AI Generated
#define BUZZER_DUTY 512                  // 50% duty for 10-bit resolution // AI Generated

SemaphoreHandle_t sem_button;
SemaphoreHandle_t sem_sensor;
SemaphoreHandle_t print_mutex;

volatile int SEMCNT = 0;
volatile bool high_bpm_alert = false;

// Initialize PWM hardware for buzzer output      (AI generated)
void buzzer_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = BUZZER_RES,
        .freq_hz = BUZZER_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Attach PWM channel to buzzer GPIO
    ledc_channel_config_t ledc_channel = {
        .gpio_num = BUZZER_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,   // start OFF
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

// Turn buzzer ON (50% duty → audible tone)         (AI generated)
void buzzer_on(void) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, BUZZER_DUTY); 
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Turn buzzer OFF                                  (AI generated)
void buzzer_off(void) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Task: Simulates device "heartbeat" using green LED
// Period: 1000 ms (1 Hz blink)
void medical_device_active_task(void *pvParameters) {
    bool led_status = false;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        gpio_set_level(LED_GREEN, led_status);
        led_status = !led_status;

        xSemaphoreTake(print_mutex, portMAX_DELAY);
        printf("[%lu ms] Patient Monitor\n",
               (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
        xSemaphoreGive(print_mutex);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

// Task: Reads ADC (potentiometer) and converts to BPM
// Period: 100 ms
// Also detects threshold crossing (>140 BPM) and signals alert
void heart_rate_sensor_task(void *pvParameters) {
    static int prevAboveThreshold = 0;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        int val = adc1_get_raw(POT_ADC_CHANNEL);
        int BPM = (val * BPM_MAX) / 4095;

        // Protected print (mutex prevents interleaving)
        xSemaphoreTake(print_mutex, portMAX_DELAY);
        printf("[%lu ms] Heart Rate: %d BPM\n", (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS), BPM);
        xSemaphoreGive(print_mutex);

        // Detect high BPM condition
        if (BPM > BPM_ALERT_THRESHOLD) {
            high_bpm_alert = true;

            // Only trigger semaphore on rising edge
            if (prevAboveThreshold == 0) {
                if (SEMCNT < MAX_COUNT_SEM) {
                    SEMCNT++;
                    xSemaphoreGive(sem_sensor);
                }
                prevAboveThreshold = 1;
            }
        } else {
            high_bpm_alert = false;
            prevAboveThreshold = 0;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

// Task: Polls nurse button with software debounce
// Period: 10 ms
// Gives semaphore when button press is detected
void nurse_call_button_task(void *pvParameters) {
    int lastState = 1;
    TickType_t lastDebounceTime = 0;
    const TickType_t debounceDelay = pdMS_TO_TICKS(50);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        int state = gpio_get_level(BUTTON_PIN);

        // Detect falling edge + debounce check
        if (state == 0 && lastState == 1 &&
            (xTaskGetTickCount() - lastDebounceTime) > debounceDelay) {
            xSemaphoreGive(sem_button);
            lastDebounceTime = xTaskGetTickCount();
        }

        lastState = state;
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

// Task: Handles alerts from sensor and nurse button
// Period: 10 ms (event-driven via non-blocking semaphore checks)
void medical_alert_handler_task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        // Handle high BPM alert event
        if (xSemaphoreTake(sem_sensor, 0)) {
            SEMCNT--;

            xSemaphoreTake(print_mutex, portMAX_DELAY);
            printf("[%lu ms] ALERT: Patient heart rate exceeded 140 BPM! Notify nurse immediately!\n",
                   (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            xSemaphoreGive(print_mutex);
        }

        // Handle nurse button press
        if (xSemaphoreTake(sem_button, 0)) {
            xSemaphoreTake(print_mutex, portMAX_DELAY);
            printf("[%lu ms] Nurse Button Pressed!\n",
                   (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            printf("[%lu ms] ALERT: Nurse assistance requested!\n",
                   (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            xSemaphoreGive(print_mutex);

            // Flash red LED briefly (blocking delay)
            gpio_set_level(LED_RED, 1);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_set_level(LED_RED, 0);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

// Task: Controls alarm outputs (buzzer + red LED)
// Period: 200 ms
// Activates continuously while BPM alert is active
void alarm_output_task(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bool red_led_state = false;

    while (1) {
        if (high_bpm_alert) {
            buzzer_on();

            // Blink red LED during alert
            red_led_state = !red_led_state;
            gpio_set_level(LED_RED, red_led_state);
        } else {
            buzzer_off();
            gpio_set_level(LED_RED, 0);
            red_led_state = false;
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(200));
    }
}

void app_main(void) {
    // Configure LEDs as outputs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_GREEN) | (1ULL << LED_RED),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    // Configure button as input with pull-up
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&btn_conf);

    // Configure ADC for potentiometer input
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_ADC_CHANNEL, ADC_ATTEN_DB_11);

    // Initialize buzzer PWM
    buzzer_init();

    // Create synchronization primitives
    sem_button = xSemaphoreCreateBinary();             // button event
    sem_sensor = xSemaphoreCreateCounting(MAX_COUNT_SEM, 0); // BPM events
    print_mutex = xSemaphoreCreateMutex();             // protect printf

    // Create tasks with priorities
    xTaskCreate(medical_device_active_task, "Patient Monitor", 2048, NULL, 1, NULL);
    xTaskCreate(heart_rate_sensor_task, "Heart Rate Sensor", 2048, NULL, 2, NULL);
    xTaskCreate(nurse_call_button_task, "Nurse Call Button", 2048, NULL, 3, NULL);
    xTaskCreate(medical_alert_handler_task, "Medical Alert Handler", 2048, NULL, 2, NULL);
    xTaskCreate(alarm_output_task, "Alarm Output", 2048, NULL, 2, NULL);
}