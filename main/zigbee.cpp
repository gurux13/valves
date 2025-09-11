#include "zigbee.h"
#include "esp_zb_light.h"
#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "driver/gpio.h"
#include "pinout.h"
#include "driver/gptimer_etm.h"
#include "driver/gptimer.h"
#include "esp_timer.h"
#include <thread>
#include <esp_zigbee_core.h>
#include "esp_zb_light.h"
#include "zboss_api.h"
#include "valves.h"
char mfg[128];
char model[128];

static const char *TAG = "Zigbee";
volatile bool is_connected_anew = false;
static bool should_factory_reset = false;

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    ESP_LOGI(TAG, "Restart top level commissioning with mode 0x%x", mode_mask);
    ESP_ERROR_CHECK(esp_zb_bdb_start_top_level_commissioning(mode_mask));
}


static esp_err_t zb_attribute_handler(const esp_zb_zcl_set_attr_value_message_t *message)
{
    esp_err_t ret = ESP_OK;

    ESP_RETURN_ON_FALSE(message, ESP_FAIL, TAG, "Empty message");
    ESP_RETURN_ON_FALSE(message->info.status == ESP_ZB_ZCL_STATUS_SUCCESS, ESP_ERR_INVALID_ARG, TAG, "Received message: error status(%d)",
                        message->info.status);
    ESP_LOGI(TAG, "Received message: endpoint(%d), cluster(0x%x), attribute(0x%x), data size(%d), data type(%d)", message->info.dst_endpoint,
        message->info.cluster, message->attribute.id, message->attribute.data.size, message->attribute.data.type);

    if (message->info.dst_endpoint > 0 && message->info.dst_endpoint < 4) {
        switch (message->info.cluster) {
        case ESP_ZB_ZCL_CLUSTER_ID_IDENTIFY:
            ESP_LOGI(TAG, "Identify pressed");
            break;

        case ESP_ZB_ZCL_CLUSTER_ID_ON_OFF:
            if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_ON_OFF_ID
                && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_BOOL
                && message->attribute.data.value != NULL) {
                    auto value = *(bool *)message->attribute.data.value;
                    auto& valve = Valves::get_instance().get_valve(static_cast<Valves::ValveId>(message->info.dst_endpoint - 1));
                    if (value) {
                        ESP_LOGI(TAG, "Valve %d: OPEN", message->info.dst_endpoint);
                        valve.open();
                    }
                    else {
                        ESP_LOGI(TAG, "Valve %d: CLOSED", message->info.dst_endpoint);
                        valve.close();
                    }
                    
                }

            // if (message->attribute.id == ESP_ZB_ZCL_ATTR_ON_OFF_START_UP_ON_OFF
            //     && message->attribute.data.type == ESP_ZB_ZCL_ATTR_TYPE_8BIT_ENUM
            //     && message->attribute.data.value != NULL)
            //         light_set_startup_on_off(*(uint8_t *)message->attribute.data.value);
            break;

        }
    }

    return ret;
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = (esp_zb_app_signal_type_t)*p_sg_p;
    switch (sig_type)
    {
    case ESP_ZB_NLME_STATUS_INDICATION:
    {
        if (err_status == ESP_OK)
        {
            is_connected_anew = true;
            ESP_LOGI(TAG, "Rejoined network successfully (PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        }
        else
        {
            ESP_LOGW(TAG, "Failed to get network status: %s", esp_err_to_name(err_status));
        }
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        ESP_LOGI(TAG, "Initialize Zigbee stack");
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK)
        {
            // ESP_LOGI(TAG, "Deferred driver initialization %s", deferred_driver_init() ? "failed" : "successful");
            ESP_LOGI(TAG, "Device started up in %s factory-reset mode", esp_zb_bdb_is_factory_new() ? "" : "non");
            if (esp_zb_bdb_is_factory_new())
            {
                ESP_LOGI(TAG, "Start network steering");
                esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
                // pthread_t t2;

                // esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
                // cfg.stack_size = (4 * 1024);
                // esp_pthread_set_cfg(&cfg);

                // pthread_create(&t2, NULL, blinker_thread, NULL);
            }
            else
            {
                if (should_factory_reset)
                {
                    ESP_LOGI(TAG, "Factory reset Zigbee stack");
                    esp_zb_factory_reset();
                }
                ESP_LOGI(TAG, "Device rebooted");
            }
        }
        else
        {
            ESP_LOGW(TAG, "Failed to initialize Zigbee stack (status: %s)", esp_err_to_name(err_status));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK)
        {
            is_connected_anew = true;
            esp_zb_ieee_addr_t extended_pan_id;
            esp_zb_get_extended_pan_id(extended_pan_id);
            ESP_LOGI(TAG, "Joined network successfully (Extended PAN ID: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x, PAN ID: 0x%04hx, Channel:%d, Short Address: 0x%04hx)",
                     extended_pan_id[7], extended_pan_id[6], extended_pan_id[5], extended_pan_id[4],
                     extended_pan_id[3], extended_pan_id[2], extended_pan_id[1], extended_pan_id[0],
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
        }
        else
        {
            ESP_LOGI(TAG, "Network steering was not successful (status: %s)", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm((esp_zb_callback_t)bdb_start_top_level_commissioning_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 1000);
        }
        break;
    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS:
        if (err_status == ESP_OK)
        {
            if (*(uint8_t *)esp_zb_app_signal_get_params(p_sg_p))
            {
                ESP_LOGI(TAG, "Network(0x%04hx) is open for %d seconds", esp_zb_get_pan_id(), *(uint8_t *)esp_zb_app_signal_get_params(p_sg_p));
            }
            else
            {
                ESP_LOGW(TAG, "Network(0x%04hx) closed, devices joining not allowed.", esp_zb_get_pan_id());
            }
        }
        break;
    default:
        ESP_LOGI(TAG, "ZDO signal: %s (0x%x), status: %s", esp_zb_zdo_signal_to_string(sig_type), sig_type, esp_err_to_name(err_status));
        break;
    }
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id, const void *message)
{
    esp_err_t ret = ESP_OK;
    switch (callback_id)
    {
    case ESP_ZB_CORE_SET_ATTR_VALUE_CB_ID:
        ret = zb_attribute_handler((esp_zb_zcl_set_attr_value_message_t *)message);
        break;
    default:
        ESP_LOGW(TAG, "Receive Zigbee action(0x%x) callback", callback_id);
        break;
    }
    return ret;
}

static void set_zcl_string(char *buffer, const char *value) {
    buffer[0] = (char) strlen(value);
    memcpy(buffer + 1, value, buffer[0]);
}
static char model_id[16];
static char manufacturer_name[16];
static char firmware_version[16];
#define FIRMWARE_VERSION                "v1.0"

static void make_valve_endpoint(uint8_t id, esp_zb_ep_list_t *ep_list) {
    esp_zb_basic_cluster_cfg_t basic_cfg = 
        {
            .zcl_version = 3,
            .power_source = ZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE
        };
    esp_zb_attribute_list_t *esp_zb_basic_cluster = esp_zb_basic_cluster_create(&basic_cfg);
    set_zcl_string(model_id, ESP_MODEL_IDENTIFIER);
    set_zcl_string(manufacturer_name, ESP_MANUFACTURER_NAME);
    set_zcl_string(firmware_version, FIRMWARE_VERSION);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model_id);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer_name);
    esp_zb_basic_cluster_add_attr(esp_zb_basic_cluster, ESP_ZB_ZCL_ATTR_BASIC_SW_BUILD_ID, firmware_version);

    esp_zb_identify_cluster_cfg_t identify_cfg = {
        .identify_time = 0
    };
    esp_zb_attribute_list_t *esp_zb_identify_cluster = esp_zb_identify_cluster_create(&identify_cfg);
    esp_zb_groups_cluster_cfg_t groups_cfg = {};
    esp_zb_attribute_list_t *esp_zb_ep_groups_cluster = esp_zb_groups_cluster_create(&groups_cfg);
    esp_zb_scenes_cluster_cfg_t scenes_cfg = {};
    esp_zb_attribute_list_t *esp_zb_ep_scenes_cluster = esp_zb_scenes_cluster_create(&scenes_cfg);

    uint8_t default_on_off = ZB_ZCL_ON_OFF_START_UP_ON_OFF_IS_OFF;
    esp_zb_on_off_cluster_cfg_t on_off_cfg = {
        .on_off = false
    };
    esp_zb_attribute_list_t *esp_zb_ep_on_off_cluster = esp_zb_on_off_cluster_create(&on_off_cfg);
    esp_zb_on_off_cluster_add_attr(esp_zb_ep_on_off_cluster, ESP_ZB_ZCL_ATTR_ON_OFF_START_UP_ON_OFF, &default_on_off);

    esp_zb_cluster_list_t *esp_zb_zcl_cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_basic_cluster(esp_zb_zcl_cluster_list, esp_zb_basic_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_identify_cluster(esp_zb_zcl_cluster_list, esp_zb_identify_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_groups_cluster(esp_zb_zcl_cluster_list, esp_zb_ep_groups_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_scenes_cluster(esp_zb_zcl_cluster_list, esp_zb_ep_scenes_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_on_off_cluster(esp_zb_zcl_cluster_list, esp_zb_ep_on_off_cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);


    esp_zb_endpoint_config_t endpoint_config = {
        .endpoint = id,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_COMBINED_INTERFACE_DEVICE_ID,
    };
    esp_zb_ep_list_add_ep(ep_list, esp_zb_zcl_cluster_list, endpoint_config);
}

static void esp_zb_task(void *pvParameters)
{
    esp_zb_cfg_t zb_nwk_cfg; // = ESP_ZB_ZR_CONFIG();
    zb_nwk_cfg.esp_zb_role = ESP_ZB_DEVICE_TYPE_ROUTER;
    zb_nwk_cfg.install_code_policy = INSTALLCODE_POLICY_ENABLE;
    zb_nwk_cfg.nwk_cfg.zczr_cfg = {
        .max_children = MAX_CHILDREN};
    esp_zb_init(&zb_nwk_cfg);


    esp_zb_ep_list_t *esp_zb_ep_list = esp_zb_ep_list_create();
    for (uint8_t i = 1; i < 4; i++) {
        make_valve_endpoint(i, esp_zb_ep_list);
    }

    esp_zb_device_register(esp_zb_ep_list);
    esp_zb_core_action_handler_register(zb_action_handler);
    esp_zb_set_primary_network_channel_set(ESP_ZB_PRIMARY_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}

void init_zigbee()
{
    esp_zb_platform_config_t config = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_zb_platform_config(&config));
    xTaskCreate(esp_zb_task, "Zigbee_main", 4096, NULL, 5, NULL);
}