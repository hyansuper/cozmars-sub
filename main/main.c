#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2c_slave.h"
#include "driver/gpio.h"
#include "hal/i2c_types.h"
#include "sub_config.h"
#include "sub_i2c_msg.h"
#include "components.h"
#include "battery.h"
#include "state_monitor.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "kv.h"

static const char *TAG = "cozmars-sub";

static i2c_slave_dev_handle_t i2c_dev_handle;
static TaskHandle_t i2c_write_task_handle;
static sub_msg_t pending_request;

static void config_to_settings() 
{
    settings.left_motor.swap = motors.left.config.swap;
    settings.right_motor.swap = motors.right.config.swap;
    settings.lift = lift_servo.cfg;
    settings.head = head_servo.cfg;
}

static void poweroff_all_components_from_isr(BaseType_t* xTaskWoken)
{
    motors.cmd.type = MOTOR_CMD_POWEROFF;
    xTaskNotifyFromISR(motors.task_handle, MOTOR_NOTIFY_NEW_CMD_MASK, eSetBits, xTaskWoken);

    head_servo.cmd.type = SERVO_CMD_POWEROFF;
    vTaskNotifyGiveFromISR(head_servo.task_handle, xTaskWoken);

    lift_servo.cmd.type = SERVO_CMD_POWEROFF;
    vTaskNotifyGiveFromISR(lift_servo.task_handle, xTaskWoken);

    wled.cmd.type = WLED_CMD_SET_COLOR;
    wled.cmd.color_arg = (wled_color_t){0, 0, 0, 0};
    vTaskNotifyGiveFromISR(wled.task_handle, xTaskWoken);
}

static void prepare_for_deep_sleep(void)
{
    ESP_ERROR_CHECK(i2c_del_slave_device(i2c_dev_handle));

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << DSL_WAKE_UP_IO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(esp_sleep_enable_gpio_wakeup_on_hp_periph_powerdown(
        1ULL << DSL_WAKE_UP_IO, ESP_GPIO_WAKEUP_GPIO_LOW));
}

static void i2c_write_task(void *arg)
{
    for (;;) {
        uint32_t write_len;
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        switch (pending_request.type) {
        case SUB_MSG_RD_STATE:
            sub_state.idle_flags = motors.idle | head_servo.idle | lift_servo.idle | wled.idle;
            i2c_slave_write(i2c_dev_handle, (const uint8_t *)&sub_state, sizeof(sub_state), &write_len, 0);
            break;
        case SUB_MSG_RD_VERSION: {
            sub_msg_version_resp_t ver = GET_SUB_MSG_VER_RESP();
            i2c_slave_write(i2c_dev_handle, (const uint8_t *)&ver, sizeof(ver), &write_len, 0);
            break;
        }
        case SUB_MSG_RD_SETTINGS:
            config_to_settings();
            i2c_slave_write(i2c_dev_handle, (const uint8_t *)&settings, sizeof(settings), &write_len, 0);
            break;
        case SUB_MSG_WR_POWER:
            vTaskDelay(pdMS_TO_TICKS(500));
            if (pending_request.power_cmd.type == POWER_CMD_SLEEP) {
                ESP_LOGI(TAG, "Got cmd, deep sleep");
                prepare_for_deep_sleep();
                esp_deep_sleep_start();
            } else if(pending_request.power_cmd.type == POWER_CMD_REBOOT) {
                ESP_LOGI(TAG, "Got cmd, restart");
                esp_restart();
            }
            break;
        default:
            break;
        }
    }
}

static bool i2c_slave_receive_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_rx_done_event_data_t *evt_data, void *arg)
{
    BaseType_t xTaskWoken = 0;
    if (evt_data->length < sizeof(sub_msg_type_t)) return xTaskWoken;

    pending_request.type = *(sub_msg_type_t*) evt_data->buffer;
    
    if (evt_data->length < sizeof(sub_msg_t)) return xTaskWoken;

    sub_msg_t *msg = (sub_msg_t *)evt_data->buffer;
    switch (msg->type) {
    case SUB_MSG_WR_STOP: 
        poweroff_all_components_from_isr(&xTaskWoken);
        break;
    case SUB_MSG_WR_HEAD:
        head_servo.cmd = msg->servo_cmd;
        vTaskNotifyGiveFromISR(head_servo.task_handle, &xTaskWoken);
        break;
    case SUB_MSG_WR_LIFT:
        lift_servo.cmd = msg->servo_cmd;
        vTaskNotifyGiveFromISR(lift_servo.task_handle, &xTaskWoken);
        break;
    case SUB_MSG_WR_MOTOR:
        motors.cmd = msg->motor_cmd;
        xTaskNotifyFromISR(motors.task_handle, MOTOR_NOTIFY_NEW_CMD_MASK, eSetBits, &xTaskWoken);
        break;
    case SUB_MSG_WR_LED:
        wled.cmd = msg->wled_cmd;
        vTaskNotifyGiveFromISR(wled.task_handle, &xTaskWoken);
        break;
    case SUB_MSG_WR_SAVE_SETTINGS:
        config_to_settings();
        kv_save_settings();
        break;
    case SUB_MSG_WR_CONFIG:
        switch(msg->config_cmd.type) {
        case CONFIG_CMD_HEAD_DUTY_RANGE:
            servo_config(&head_servo, &msg->config_cmd.servo_cfg);
            break;
        case CONFIG_CMD_LIFT_DUTY_RANGE:
            servo_config(&lift_servo, &msg->config_cmd.servo_cfg);
            break;
        case CONFIG_CMD_DISABLE_CLIFF_DETECTION:
            state_monitor_disable_cliff_detection(msg->config_cmd.disable_cliff_detection);
            break;
        case CONFIG_CMD_MOTOR_ACCEL:
            motors_set_accel(&motors, msg->config_cmd.accel_throttle_per_sec);
            break;
        case CONFIG_CMD_MOTOR_SWAP:
            motors_set_swap(&motors, msg->config_cmd.swap_motors[0], msg->config_cmd.swap_motors[1]);
            break;
        default:
            break;
        }
        break;
    case SUB_MSG_WR_POWER:
        poweroff_all_components_from_isr(&xTaskWoken);
        pending_request.power_cmd.type = msg->power_cmd.type;
        // reuse the i2c_write_task to perform a delay to allow all components power off,
        // you cann't take a delay in this isr function.
        vTaskNotifyGiveFromISR(i2c_write_task_handle, &xTaskWoken);
        break;
    default:
        break;
    }
    return xTaskWoken;
}

static bool i2c_slave_request_cb(i2c_slave_dev_handle_t i2c_slave, const i2c_slave_request_event_data_t *evt_data, void *arg)
{
    BaseType_t xTaskWoken = 0;
    vTaskNotifyGiveFromISR(i2c_write_task_handle, &xTaskWoken);
    return xTaskWoken;
}

void app_main(void)
{
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Wake up caused by: %d (%s)", wakeup_cause,
             wakeup_cause == ESP_SLEEP_WAKEUP_GPIO ? "GPIO" :
             wakeup_cause == ESP_SLEEP_WAKEUP_UNDEFINED ? "reset" : "other");

    kv_init();
    components_init();
    if (ESP_OK == kv_read_settings()) {
        motors_set_swap(&motors, settings.left_motor.swap, settings.right_motor.swap);
        servo_config(&head_servo, &settings.head);
        servo_config(&lift_servo, &settings.lift);
    }
    state_monitor_init();

    i2c_slave_config_t i2c_slv_config = {
        .i2c_port = 0,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .scl_io_num = I2C_SLAVE_SCL_IO,
        .sda_io_num = I2C_SLAVE_SDA_IO,
        .slave_addr = SUB_I2C_ADDR,
        .send_buf_depth = 128,
        .receive_buf_depth = sizeof(sub_msg_t),
    };

    ESP_ERROR_CHECK(i2c_new_slave_device(&i2c_slv_config, &i2c_dev_handle));

    xTaskCreate(i2c_write_task, "i2c_write", 2048, NULL, I2C_RESPONSE_TASK_PRIORITY, &i2c_write_task_handle);

    i2c_slave_event_callbacks_t cbs = {
        .on_receive = i2c_slave_receive_cb,
        .on_request = i2c_slave_request_cb,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(i2c_dev_handle, &cbs, NULL));

    ESP_LOGI(TAG, "I2C slave initialized, address: 0x%02x", SUB_I2C_ADDR);

    vTaskDelete(NULL);
}