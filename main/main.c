
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/* Enable this to show verbose logging for this file only. */
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"

#include "my_N2K_lib.h"

static const char *TAG = "main";

// Main routine. 
void app_main(void)
{

	// testing!!!
	my_N2K_lib_init();
	setSwitchState(1,1);

	while (1)
	{
		vTaskDelay(1000UL / portTICK_PERIOD_MS);
		//ESP_LOGD(TAG, "free stack: %d", uxTaskGetStackHighWaterMark(NULL));
		bool switchState = getSwitchState(1);
		ESP_LOGI(TAG, "switch 1: %d", switchState);
	}
}
