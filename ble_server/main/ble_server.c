#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "S3_NIMBLE";
static uint8_t own_addr_type;

static void ble_app_advertise(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connected");
            } else {
                ESP_LOGW(TAG, "Connection failed");
                ble_app_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected");
            ble_app_advertise();
            break;
    }
    return 0;
}

static void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    struct ble_gap_adv_params adv_params = {0};

    const char *name = "S3-BLE-CTRL";

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t*)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    ble_svc_gap_init();

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "set adv fields error: %d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(own_addr_type, NULL,
                           BLE_HS_FOREVER,
                           &adv_params,
                           ble_gap_event, NULL);

    if (rc != 0) {
        ESP_LOGE(TAG, "failed to start adv: %d", rc);
    } else {
        ESP_LOGI(TAG, "Advertising as \"%s\"", name);
    }
}

static void ble_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "infer addr type failed: %d", rc);
        return;
    }

    uint8_t addr[6];
    ble_hs_id_copy_addr(own_addr_type, addr, NULL);

    ESP_LOGI(TAG, "Device address: %02X:%02X:%02X:%02X:%02X:%02X",
        addr[5], addr[4], addr[3],
        addr[2], addr[1], addr[0]);

    ble_app_advertise();
}

static void host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void)
{
    nvs_flash_init();

    nimble_port_init();

    ble_hs_cfg.sync_cb = ble_on_sync;

    nimble_port_freertos_init(host_task);
}
