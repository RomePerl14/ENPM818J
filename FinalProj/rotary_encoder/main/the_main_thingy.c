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
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

// FreeRTOS LIBRARIES
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h" // unused, but keeping until we're done


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
void ctrl_loop_write(float U_);
void position_pid_task(void *pvParameters);
void velocity_pid_task();
void setup_pcnt_stuff();
static void setup_ledc_stuff(void);

// Debug setup
static const char *TAG_position_PID = "[Position Ctrl]";
// static const char *TAG_velocity_PID = "[Velocity Ctrl]";
static const char *TAG_main = "[Main]";

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

// control loop frequencies, to be used by each task
float ctrl_freq_position = 1000; // Hz
float ctrl_freq_velocity = 500; // Hz
typedef enum {
  STANDARD,
  VELOCITY,
} pid_mode;
pid_mode mode = STANDARD;

//-------- POSITION LOOP PARAMETERS
float Kp_pos = 1;
float Kd_pos = 0.00;
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
void ctrl_loop_write(float U_)
{
    // enforce that our control input is within bounds
    if (U_ > Umax)
    {
        U_ = Umax;
    }
    else if (U_ < -Umax)
    {
        U_ = -Umax;
    }

    // generate the PWM signal based on the duty count and the current control signal
    int cycle = (int)((ledc_duty_ctrl_loop) * fabsf(U_) * Umax_inv);
    if(cycle > (ledc_duty_ctrl_loop)) // should never be the case, but lets check anyway
    {
        cycle = (ledc_duty_ctrl_loop);
    }

    if(U_ < 0)
    {
        // set the direction pin LOW 
        // Low is negative! so this should make sense
        gpio_set_level(DIRECTION_PIN, 0);
    }
    else if(U_ > 0)
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
    float des_pos = 3.141592; // the desired position of the motor
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
        ctrl_loop_write(U);

        // Set the previous variables for our next loop
        prev_err = err;
        prev_pos = curr_pos;
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(1));
    }
}

// void velocity_pid_task()
// {

// }

// void command_interface_task()
// {

// }

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
    xTaskCreate(position_pid_task, "position_pid", 8192, NULL, 1, NULL);
    // xTaskCreate(blink_task_2, "Blink2", 2048, NULL, 1, NULL);
    return;
}
