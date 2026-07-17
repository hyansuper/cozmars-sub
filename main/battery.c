#include "battery.h"
#include "sub_config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "battery";

#define BATTERY_ADC_UNIT               ADC_UNIT_1
#define BATTERY_ADC_ATTEN              ADC_ATTEN_DB_12
#define BATTERY_ADC_BITWIDTH           ADC_BITWIDTH_DEFAULT

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t adc_cali_handle;

static void battery_calibration_init(uint32_t channel)
{
    esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BATTERY_ADC_UNIT,
        .chan = channel,
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = BATTERY_ADC_BITWIDTH,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle);
#endif

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED) {
        adc_cali_handle = NULL;
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        adc_cali_handle = NULL;
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }
}

void battery_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = BATTERY_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = BATTERY_ADC_ATTEN,
        .bitwidth = BATTERY_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, BATTERY_ADC_CHANNEL, &chan_config));

    battery_calibration_init(BATTERY_ADC_CHANNEL);

    ESP_LOGI(TAG, "initialized on GPIO %d", BATTERY_ADC_GPIO);
}

int battery_read_adc_mv(void)
{
    int adc_raw, mv;
    adc_oneshot_read(adc_handle, BATTERY_ADC_CHANNEL, &adc_raw);
    
    if (adc_cali_handle && adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &mv)==ESP_OK)
        return mv;
    return adc_raw * 3100 / 4095; // fallback
}


typedef struct {
    int voltage; // mv
    int capacity;     // %
} battery_point_t;

/* data is from esp-iot-solution / adc_battery_estimation.h / CONFIG_OCV_SOC_MODEL_2 */
static const battery_point_t battery_points[] = {
    {4177, 100},
    {4129, 95},
    {4085, 90},
    {4045, 85},
    {4008, 80},
    {3974, 75},
    {3945, 70},
    {3917, 65},
    {3884, 60},
    {3841, 55},
    {3820, 50},
    {3805, 45},
    {3793, 40},
    {3783, 35},
    {3775, 30},
    {3762, 25},
    {3741, 20},
    {3709, 15},
    {3686, 10},
    {3674, 5},
    {3305, 0},
};

/* 
    the calulation has to do with hardware.
*/
battery_state_t battery_to_state(int mv)
{
    if (mv < 15) {
        return BATTERY_CHARGING;
    } else if (mv < 750) {
        return BATTERY_STANDBY;
    }
    mv *= 2; // battery voltage goes through a divider of ratio 1/2, now mv is battery voltage
    int cap = 0;
    const battery_point_t* p = battery_points, *p_end = p + sizeof(battery_points)/sizeof(battery_point_t);
    while(p < p_end) {
        if(mv > p->voltage) {
            cap = p->capacity;
            break;
        }
        p ++;
    }
    return cap/20 + BATTERY_TOO_LOW;
}