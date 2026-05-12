/* ****************************************************************
 * (c) Copyright 2025                                             *
 * Romeo Perlstein                                                *
 * ENPM818J - Real Time Operating Systems                         *
 * FINAL PROJECT - THE MAIN THINGY                                *
 * College Park, MD 20742                                         *
 * https://terpconnect.umd.edu/~romeperl/                         *
 ******************************************************************/

// C LIBRARIES
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ESP32 LIBRARIES
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_err.h"
#include "esp_adc/adc_continuous.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// FreeRTOS LIBRARIES
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // unused, but keeping until we're done
#include "freertos/semphr.h"


// TODO: need to do the following
/*
    - create task that does position PID loop
        - this task will follow a ROS 2 control scheme of read() -> update() -> write(), inside the task
        - this task will read the encoder counts directly, not offloading it to another task
        - loop speed TBD - should be faster than velocity loop but how fast is too much for the thingy?
    - create task that does velocity PID loop
        - this task will follow a ROS 2 control scheme of read() -> update -> write(), inside the task
        - see above
    - create function to generate Command Signal
        - this takes in a control signal and converts it to a PWM single, and figures out what
            direction to command to the motor driver
    - create function to compute PID position loop
        - this function will take in the current error and the previous error and, kp, kd, and ki gains, and dt
    - create function to computer the PID velocity loop
        - this function will take in the current velocity error and the previous velocity error and kp, kd, ki and dt
    - what else?

*/

typedef struct 
{
    float Kp;
    float Kd;
    float Ki;
} pid_gains;

// Function prototypes
void ctrl_loop_read(int *encoder_counts_);
float compute_position_pid_standard(float err, float prev_err, pid_gains gains, float freq);
float compute_position_pid_velocity(float err, float vel, pid_gains gains);
void ctrl_loop_write(float *U_);
void position_pid_task(void *pvParameters);
void velocity_pid_task();
void setup_pcnt_stuff();
static void setup_ledc_stuff(void);

// Debug setup
static const char *TAG_position_PID = "[Position Ctrl]";
// static const char *TAG_velocity_PID = "[Velocity Ctrl]";
static const char *TAG_main = "[Main]";
static const char *TAG_pot = "[Potentiometer]";
static const char *TAG_display = "[DISPLAY]";

// CONSTANTS
#define PI 3.14159265359

// GPIO PIN SETTINGS
#define LEDC_OUTPUT_IO 2 // Define the PWM pin
#define DIRECTION_PIN 4
#define ENABLE_PIN 16
#define BRAKE_PIN 17
#define ENCODER_A_PIN 12
#define ENCODER_B_PIN 14

#define GPIO_OUTPUT_PINS ((1ULL<<DIRECTION_PIN) | (1ULL<<ENABLE_PIN) | (1ULL<<BRAKE_PIN))

// PWM LEDC setup
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_HIGH_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_11_BIT // Set duty resolution to 11 bits (calculated maximum duty cycle)
uint16_t ledc_duty_ctrl_loop    = 2047;
#define LEDC_CLK_SRC            LEDC_APB_CLK // want to use the APB (80 MHz) clock!
#define LEDC_FREQUENCY          20000 // Frequency in Hertz. Set frequency at 20 kHz

// Potentiometer setup
#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_12
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#define EXAMPLE_READ_LEN                    256

static adc_channel_t channel[] = {ADC_CHANNEL_6};

// control loop frequencies, to be used by each task
float ctrl_freq_position = 1000; // Hz
// float ctrl_freq_velocity = 500; // Hz
typedef enum {
  STANDARD,
  VELOCITY,
} pid_mode;
pid_mode mode = STANDARD;

//-------- POSITION LOOP PARAMETERS
float Kp_pos = 0.6;
float Kd_pos = 0.025;
float Ki_pos = 0;

// //-------- VELOCITY LOOP PARAMETERS
// float Kp_vel = 10;
// float Kd_vel = 0;
// float Ki_vel = 0;

//-------- CONTROL LOOP PARAMETERS
float Umax = 28; // Because out control loop outputs voltages (I think), our max operating voltage is 28V
float Umax_inv; // inverse of Umax to avoid dividing in the loop

// TODO overflow value for the encoder - used in conjunction with overlow catch ability in the PCNT
int EXAMPLE_PCNT_HIGH_LIMIT = 24000; // can do 10 positive turns starting from 0 without overflowing
int EXAMPLE_PCNT_LOW_LIMIT = -24000; // can do 10 negative turns starting from 0 without overflowing
int encoder_counts_per_rev = 2400; // encoder counts per revolution. Since encoder is 600 P/R, the quadrature encoder counts are 4x the P/R

// channel definitions for PCNT counter - global variables because the position and velocity tasks are sharing the resources and need to claim them when running
pcnt_unit_config_t unit_config;
pcnt_unit_handle_t pcnt_unit;
pcnt_glitch_filter_config_t filter_config;
pcnt_chan_config_t chan_a_config;
pcnt_chan_config_t chan_b_config;
pcnt_channel_handle_t pcnt_chan_a;
pcnt_channel_handle_t pcnt_chan_b;

// TASK HANDLES
TaskHandle_t control_loop_task;
TaskHandle_t read_pot_task;
TaskHandle_t display_data_task;

// DATA BUFFERS
float COMMAND_POS = 0.0;
int POT_POSITION = 0;
float CONTROL_SIGNAL = 0.0;
int ENCODER_COUNTS = 0.0;
float ACTUAL_POS = 0.0;
float ERROR = 0.0;

// functionalizing to reduce clutter, but this does nothing of importance
void setup_pcnt_stuff()
{
    // copy the setup from the ESP32 example
    ESP_LOGI(TAG_main, "[%i] install pcnt unit", 0);
    unit_config.high_limit = EXAMPLE_PCNT_HIGH_LIMIT;
    unit_config.low_limit = EXAMPLE_PCNT_LOW_LIMIT;

    pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    ESP_LOGI(TAG_main, "[%i] set glitch filter", 0);
    filter_config.max_glitch_ns = 1000;
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    ESP_LOGI(TAG_main, "[%i] install pcnt channels", 0);
    chan_a_config.edge_gpio_num = ENCODER_A_PIN;
    chan_a_config.level_gpio_num = ENCODER_B_PIN;
    pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    chan_b_config.edge_gpio_num = ENCODER_B_PIN;
    chan_b_config.level_gpio_num = ENCODER_A_PIN;
    pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_LOGI(TAG_main, "[%i] set edge and level actions for pcnt channels", 0);
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG_main, "[%i] enable pcnt unit", 0);
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG_main, "[%i] clear pcnt unit", 0);
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG_main, "[%i] start pcnt unit", 0);
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}

void setup_gpio_stuff()
{
    //zero-initialize the config structure.
    gpio_config_t io_conf = {};
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set
    io_conf.pin_bit_mask = GPIO_OUTPUT_PINS;
    //disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    //disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
}

static void setup_ledc_stuff(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 20 kHz
        .clk_cfg          = LEDC_CLK_SRC,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void ctrl_loop_read(int *encoder_counts_)
{
    // just get the encoder counts and pass them by reference
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, encoder_counts_));
}

// control loop implementation borrow from: https://geekeeceebee.com/DCMotor%20Position%20Tracking.html
void ctrl_loop_write(float *U_)
{
    // enforce that our control input is within bounds
    if (*U_ > Umax)
    {
        *U_ = Umax;
    }
    else if (*U_ < -Umax)
    {
        *U_ = -Umax;
    }

    // generate the PWM signal based on the duty count and the current control signal
    int cycle = (int)((ledc_duty_ctrl_loop) * fabsf(*U_) * Umax_inv);
    if(cycle > (ledc_duty_ctrl_loop)) // should never be the case, but lets check anyway
    {
        cycle = (ledc_duty_ctrl_loop);
    }

    if(*U_ < 0)
    {
        // set the direction pin LOW 
        // Low is negative! so this should make sense
        gpio_set_level(DIRECTION_PIN, 0);
    }
    else if(*U_ > 0)
    {
        // set the direction pin HIGH
        // high is positive! so this is good
        gpio_set_level(DIRECTION_PIN, 1);
    }
    //
    // update the PWM generator, and then write it
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, cycle));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}

// we're just gonna do a PD loop for right now
// this function can be used for either position control or velocity control, how the output "u" is used just changes
float compute_position_pid_standard(float err, float prev_err, pid_gains gains, float freq)
{
    float u = gains.Kp*err + gains.Kd*(err-prev_err)*freq;
    return u;
}

float compute_position_pid_velocity(float err, float vel, pid_gains gains)
{
    float u = gains.Kp*err + gains.Kd*vel;
    return u;
}

void position_pid_task(void *pvParameters)
{
    ESP_LOGI(TAG_position_PID, "[%i] Entered position control setup", 0);
    //-------- SETUP
    ESP_LOGI(TAG_position_PID, "[%i] Setting up parameters...", 0);
    // compute the period of the control loop - once at runtime! so that we don't need to be constantly dividing
    Umax_inv = 1/Umax;
    float counts_to_radians = 2*PI / encoder_counts_per_rev; // output in radians
    float radians_to_counts = 1/counts_to_radians;
    float radians_to_degrees = 180/PI;
    float degrees_to_radians = PI/180;

    // Setup control gains
    pid_gains pos_gains;
    pos_gains.Kp = Kp_pos;
    pos_gains.Kd = Kd_pos;
    pos_gains.Ki = Ki_pos;
    ESP_LOGI(TAG_position_PID, "[%i] Parameters:\nFrequency: %f\nEncoder Counts: %i\nKp: %f\nKd: %f\nKi: %f", 0, ctrl_freq_position, encoder_counts_per_rev, Kp_pos, Kd_pos, Ki_pos);


    // initial values
    float des_pos = 0.0; // the desired position of the motor
    float curr_pos = 0.0; // the measured position - always will be set to 0
    float curr_vel = 0.0; // the measured velocity - always will be set to 0
    float prev_pos = 0.0; // the previous measured position - always will start at 0
    int curr_pos_enc_counts = 0; // the current encoder counts of the system

    // initial error values
    float err = 0.0; // the position error
    float prev_err = 0.0; // the position error

    // control parameters
    float  U = 0.0; // control output

    //-------- MAIN CONTROL LOOP
    TickType_t last_wake_time = xTaskGetTickCount(); // get the beginning time loop
    while (1) 
    {
         // timing stuff

        //-------- READ MEASUREMENT
        ctrl_loop_read(&curr_pos_enc_counts);

        //-------- UPDATE CONTROL WITH USER INPUT
        // update() <- get user inputs on desired position changes
        des_pos = COMMAND_POS;
        // des_pos = get_desired_pos_somehow();

        curr_pos = curr_pos_enc_counts*counts_to_radians; // convert encoder counts to radians
        err = des_pos - curr_pos; //get the position error
        curr_vel = (curr_pos - prev_pos)*ctrl_freq_position; // computer the current velocity
        switch(mode) // switch based on the set PID mode
        {
            case STANDARD:
                U = compute_position_pid_standard(err, prev_err, pos_gains, ctrl_freq_position);
                break;
            case VELOCITY:
                U = compute_position_pid_velocity(err, curr_vel, pos_gains);
                break;
        }
        // ESP_LOGI(TAG_position_PID, "Position: %f, Counts: %i, Error: %f, U: %f", curr_pos, curr_pos_enc_counts, err, U);

        //-------- WRITE OUPUTS TO MOTOR DRIVER
        ctrl_loop_write(&U);

        // store data in the buffers
        ACTUAL_POS = curr_pos;
        ENCODER_COUNTS = curr_pos_enc_counts;
        ERROR = err;
        CONTROL_SIGNAL = U;

        // Set the previous variables for our next loop
        prev_err = err;
        prev_pos = curr_pos;
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1));
    }
}

// control potentiometer task function
static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data)
{
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    vTaskNotifyGiveFromISR(read_pot_task, &mustYield);

    return (mustYield == pdTRUE);
}
static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle)
{
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 1024,
        .conv_frame_size = EXAMPLE_READ_LEN,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = 20 * 1000,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i] & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(TAG_pot, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG_pot, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG_pot, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}
void read_potentiometer_task(void *pvParameters)
{
    esp_err_t ret;
    uint32_t ret_num = 0;
    uint8_t result[EXAMPLE_READ_LEN] = {0};
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    adc_continuous_handle_t handle = NULL;
    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    ESP_ERROR_CHECK(adc_continuous_start(handle));
    while (1) 
    {
        while (1) 
        {
            ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
            if (ret == ESP_OK) {
                // ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);

                adc_continuous_data_t parsed_data[ret_num / SOC_ADC_DIGI_RESULT_BYTES];
                uint32_t num_parsed_samples = 0;

                esp_err_t parse_ret = adc_continuous_parse_data(handle, result, ret_num, parsed_data, &num_parsed_samples);
                if (parse_ret == ESP_OK) {
                    if (parsed_data[num_parsed_samples-1].valid) {
                        // ESP_LOGI(TAG_pot, "ADC%d, Channel: %d, Value: %"PRIu32,
                        //             parsed_data[num_parsed_samples-1].unit + 1,
                        //             parsed_data[num_parsed_samples-1].channel,
                        //             parsed_data[num_parsed_samples-1].raw_data);
                        // COMMAND_POS = ((float)(parsed_data[num_parsed_samples-1].raw_data)/2048.0 - 1)*2*PI;
                        COMMAND_POS = ((float)(parsed_data[num_parsed_samples-1].raw_data)/2048.0 - 1)*PI;
                        POT_POSITION = parsed_data[num_parsed_samples-1].raw_data;
                        // ESP_LOGI(TAG_pot, "Pot Position: %i, Desired Position: %f", parsed_data[num_parsed_samples-1].raw_data, COMMAND_POS);
                    }
                }

                /**
                 * Because printing is slow, so every time you call `ulTaskNotifyTake`, it will immediately return.
                 * To avoid a task watchdog timeout, add a delay here. When you replace the way you process the data,
                 * usually you don't need this delay (as this task will block for a while).
                 */
                vTaskDelay(pdMS_TO_TICKS(10)); // run fast, if you can!
            }
            else if (ret == ESP_ERR_TIMEOUT)
            {
                //We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
                break;
            }
        }
    }
    ESP_ERROR_CHECK(adc_continuous_stop(handle));
    ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}

void display_control_data_task(void *pvParameters)
{
    while(1)
    {
        ESP_LOGI(TAG_display, "CONTROL DATA:\t\ndes_pos: %f,\ncurr_pos: %f,\npot_pos: %i,\nerr: %f,\nu(t): %f,\nenc_counts: %i\n\n", 
                    COMMAND_POS,
                    ACTUAL_POS,
                    POT_POSITION,
                    ERROR,
                    CONTROL_SIGNAL,
                    ENCODER_COUNTS);

        // display data 10 times a second
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    // setup the counter first
    ESP_LOGI(TAG_main, "[%i] Setting up pulse counter for quadrature decoder...", 0);
    setup_pcnt_stuff();

    // setup the PWM generator next
    ESP_LOGI(TAG_main, "[%i] Setting up PWM generator for control signal...", 0);
    setup_ledc_stuff();

    // setup GPIO pins
    ESP_LOGI(TAG_main, "[%i] Setting up GPIO pins for signal outputs...", 0);
    setup_gpio_stuff();

    // setup RTOS Tasks
    ESP_LOGI(TAG_main, "[%i] Setting up RTOS tasks...", 0);
    ESP_LOGI(TAG_main, "[%i] SETTING UP POSITION CONTROL TASK", 0);
    xTaskCreatePinnedToCore(position_pid_task, "position_pid", 8192, NULL, 1, &control_loop_task, 1);
    ESP_LOGI(TAG_main, "[%i] SETTING UP POTENTIOMETER TASK", 0);
    xTaskCreatePinnedToCore(read_potentiometer_task, "poteniometer", 10000, NULL, 1, &read_pot_task, 0);
    ESP_LOGI(TAG_main, "[%i] SETTING UP DISPLAY TASK", 0);
    xTaskCreatePinnedToCore(display_control_data_task, "display", 8192, NULL, 2, &display_data_task, 0);
    return;
}
