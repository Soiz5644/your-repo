/*
 * main.c:
 * Copyright (c) 2014-2020 Rtrobot. <admin@rtrobot.org>
 *  <http://rtrobot.org>
 ***********************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "fs3000.h"

void i2c_FS3000_task(void *pvParameters)
{
	FS3000_Init();
	while (1)
	{
		float speed;
		speed = FS3000_ReadData();
		printf("%f m/s\r\n", speed);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}


void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    xTaskCreate(i2c_FS3000_task, "i2c_fs3000_task", 4096 *2, NULL, 2, NULL);
}
