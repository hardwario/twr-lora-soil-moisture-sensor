#include <twr.h>

#define MEASURE_INTERVAL (5 * 60 * 1000)
#define SEND_DATA_INTERVAL (15 * 60 * 1000)

typedef struct
{
    uint8_t channel;
    float value;
    twr_tick_t next_pub;

} event_param_t;

TWR_DATA_STREAM_INT_BUFFER(sm_soil_moisture_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
TWR_DATA_STREAM_FLOAT_BUFFER(sm_soil_temperature_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))
TWR_DATA_STREAM_FLOAT_BUFFER(sm_core_temperature_buffer, (SEND_DATA_INTERVAL / MEASURE_INTERVAL))

twr_data_stream_t sm_soil_moisture;
twr_data_stream_t sm_soil_temperature;
twr_data_stream_t sm_core_temperature;

TWR_DATA_STREAM_FLOAT_BUFFER(sm_voltage_buffer, 8)
twr_data_stream_t sm_voltage;

// LED instance
twr_led_t led;
// Button instance
twr_button_t button;

twr_soil_sensor_t soil_sensor;
twr_tmp112_t tmp112;

// Lora instance
twr_cmwx1zzabz_t lora;

twr_scheduler_task_id_t battery_measure_task_id;

enum {
    HEADER_BOOT         = 0x00,
    HEADER_UPDATE       = 0x01,
    HEADER_BUTTON_PRESS = 0x02,

} header = HEADER_BOOT;

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    (void) event_param;

    if (event == TWR_BUTTON_EVENT_PRESS)
    {
        header = HEADER_BUTTON_PRESS;

        twr_scheduler_plan_now(0);
    }
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        float voltage = NAN;

        twr_module_battery_get_voltage(&voltage);

        twr_data_stream_feed(&sm_voltage, &voltage);
    }
}

void battery_measure_task(void *param)
{
    if (!twr_module_battery_measure())
    {
        twr_scheduler_plan_current_now();
    }
}

void lora_callback(twr_cmwx1zzabz_t *self, twr_cmwx1zzabz_event_t event, void *event_param)
{
    if (event == TWR_CMWX1ZZABZ_EVENT_ERROR)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_BLINK_FAST);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_START)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_ON);

        twr_scheduler_plan_relative(battery_measure_task_id, 20);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_SEND_MESSAGE_DONE)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_READY)
    {
        twr_led_set_mode(&led, TWR_LED_MODE_OFF);
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_SUCCESS)
    {
        twr_atci_printfln("$JOIN_OK");
    }
    else if (event == TWR_CMWX1ZZABZ_EVENT_JOIN_ERROR)
    {
        twr_atci_printfln("$JOIN_ERROR");
    }
}

bool at_send(void)
{
    twr_scheduler_plan_now(0);

    return true;
}

bool at_status(void)
{
    float value_avg = NAN;

    if (twr_data_stream_get_average(&sm_voltage, &value_avg))
    {
        twr_atci_printfln("$STATUS: \"Voltage\",%.1f", value_avg);
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Voltage\",");
    }

    float soil_temperature_avg = NAN;

    if (twr_data_stream_get_average(&sm_soil_temperature, &soil_temperature_avg))
    {
        twr_atci_printfln("$STATUS: \"Soil temperature\",%.1f", soil_temperature_avg);
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Soil temperature\",");
    }

    float soil_moisture_avg = NAN;

    if (twr_data_stream_get_average(&sm_soil_moisture, &soil_moisture_avg))
    {
        twr_atci_printfln("$STATUS: \"Soil Moisture\",%.1f", soil_moisture_avg);
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Soil Moisture\",");
    }

    float temperature_avg = NAN;

    if (twr_data_stream_get_average(&sm_core_temperature, &temperature_avg))
    {
        twr_atci_printfln("$STATUS: \"Core temperature\",%.1f", temperature_avg);
    }
    else
    {
        twr_atci_printfln("$STATUS: \"Core temperature\",");
    }

    return true;
}

void soil_sensor_event_handler(twr_soil_sensor_t *self, uint64_t device_address, twr_soil_sensor_event_t event, void *event_param)
{
    if (event == TWR_SOIL_SENSOR_EVENT_UPDATE)
    {
        float temperature;

        if (twr_soil_sensor_get_temperature_celsius(self, device_address, &temperature))
        {
            twr_data_stream_feed(&sm_soil_temperature, &temperature);
        }

        uint16_t raw_cap_u16;

        if (twr_soil_sensor_get_cap_raw(self, device_address, &raw_cap_u16))
        {
            int raw_cap = (int)raw_cap_u16;
            twr_data_stream_feed(&sm_soil_moisture, &raw_cap);
        }
    }
    else if (event == TWR_SOIL_SENSOR_EVENT_ERROR)
    {
        twr_atci_printfln("$STATUS: \"Sensor Error\",");
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        float temperature;

        if (twr_tmp112_get_temperature_celsius(self, &temperature))
        {
            twr_data_stream_feed(&sm_core_temperature, &temperature);
        }
    }
}

void application_init(void)
{
    //twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    battery_measure_task_id = twr_scheduler_register(battery_measure_task, NULL, 2020);

    // Initialize soil sensor
    twr_soil_sensor_init(&soil_sensor);
    twr_soil_sensor_set_event_handler(&soil_sensor, soil_sensor_event_handler, NULL);
    twr_soil_sensor_set_update_interval(&soil_sensor, MEASURE_INTERVAL);

    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, MEASURE_INTERVAL);

    // Init stream buffers for averaging
    twr_data_stream_init(&sm_voltage, 1, &sm_voltage_buffer);
    twr_data_stream_init(&sm_soil_moisture, 1, &sm_soil_moisture_buffer);
    twr_data_stream_init(&sm_soil_temperature, 1, &sm_soil_temperature_buffer);
    twr_data_stream_init(&sm_core_temperature, 1, &sm_core_temperature_buffer);

    // Initialize lora module
    twr_cmwx1zzabz_init(&lora, TWR_UART_UART1);
    twr_cmwx1zzabz_set_event_handler(&lora, lora_callback, NULL);
    twr_cmwx1zzabz_set_class(&lora, TWR_CMWX1ZZABZ_CONFIG_CLASS_A);
    //twr_cmwx1zzabz_set_debug(&lora, debug); // Enable debug output of LoRa Module commands to Core Module console

    // Initialize AT command interface
    twr_at_lora_init(&lora);
    static const twr_atci_command_t commands[] = {
            TWR_AT_LORA_COMMANDS,
            {"$SEND", at_send, NULL, NULL, NULL, "Immediately send packet"},
            {"$STATUS", at_status, NULL, NULL, NULL, "Show status"},
            TWR_ATCI_COMMAND_CLAC,
            TWR_ATCI_COMMAND_HELP
    };
    twr_atci_init(commands, TWR_ATCI_COMMANDS_LENGTH(commands));

    // Plan task 0 (application_task) to be run after 10 seconds
    twr_scheduler_plan_relative(0, 10 * 1000);
    
    twr_led_pulse(&led, 2000);
}

void application_task(void)
{
    if (!twr_cmwx1zzabz_is_ready(&lora))
    {
        twr_scheduler_plan_current_relative(100);

        return;
    }

    static uint8_t buffer[8];

    memset(buffer, 0xff, sizeof(buffer));

    buffer[0] = header;

    float voltage_avg = NAN;

    twr_data_stream_get_average(&sm_voltage, &voltage_avg);

    if (!isnan(voltage_avg))
    {
        buffer[1] = ceil(voltage_avg * 10.f);
    }

    float temperature_avg = NAN;

    twr_data_stream_get_average(&sm_soil_temperature, &temperature_avg);

    if (!isnan(temperature_avg))
    {
        int16_t temperature_i16 = (int16_t) (temperature_avg * 10.f);

        buffer[2] = temperature_i16 >> 8;
        buffer[3] = temperature_i16;
    }

    int soil_avg = -10;

    twr_data_stream_get_average(&sm_soil_moisture, &soil_avg);

    if (soil_avg != -10)
    {
        int16_t soil_i16 = (int16_t) (soil_avg);

        buffer[4] = soil_i16 >> 8;
        buffer[5] = soil_i16;
    }

    float core_temperature_avg = NAN;

    twr_data_stream_get_average(&sm_core_temperature, &core_temperature_avg);

    if (!isnan(core_temperature_avg))
    {
        int16_t temperature_i16 = (int16_t) (core_temperature_avg * 10.f);

        buffer[6] = temperature_i16 >> 8;
        buffer[7] = temperature_i16;
    }

    twr_cmwx1zzabz_send_message(&lora, buffer, sizeof(buffer));

    twr_scheduler_plan_current_relative(SEND_DATA_INTERVAL);
}
