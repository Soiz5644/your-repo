/*
 * main.c:
 * Copyright (c) 2014-2021 Rtrobot. <admin@rtrobot.org>
 *  <http://rtrobot.org>
 ***********************************************************************
 */

#include <stdio.h>
#include <nvs_flash.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "fs3000.h"

void i2c_fs3000_task(void *pvParameters)
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
	nvs_flash_init();
	xTaskCreate(i2c_fs3000_task, "i2c_fs3000_task", 4096, NULL, 5, NULL);
}
