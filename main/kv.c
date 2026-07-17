#include "kv.h"
#include "nvs_flash.h"
#include "nvs.h"

kv_settings_t settings;

esp_err_t kv_read_settings()
{
	nvs_handle_t handle;
	esp_err_t err = nvs_open(KV_SETTINGS, NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return err;
	}
	size_t len = sizeof(settings);
	err = nvs_get_blob(handle, KV_SETTINGS, &settings, &len);
	nvs_close(handle);
	return err;
}

esp_err_t kv_save_settings()
{
	nvs_handle_t handle;
	esp_err_t err = nvs_open(KV_SETTINGS, NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		return err;
	}
	err = nvs_set_blob(handle, KV_SETTINGS, &settings, sizeof(settings));
	if (err == ESP_OK) {
		err = nvs_commit(handle);
	}
	nvs_close(handle);
	return err;
}

esp_err_t kv_init()
{
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	return err;
}
