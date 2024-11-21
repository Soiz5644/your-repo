/*
 * @author: rtrobot<admin@rtrobot.org>
 * @website:rtrobot.org
 * @licence: GPL v3
 */

#include "fs3000.h"
#include <stdio.h>
#include <driver/i2c.h>
#include <driver/spi_master.h>

#define ESP_MASTER_ADDR FS3000_I2CADDR
#define I2C_MASTER_SDA_IO (gpio_num_t)18
#define I2C_MASTER_SCL_IO (gpio_num_t)19
#define I2C_MASTER_FREQ_HZ 50000

/***************************************************************************************************************
i2c master initialization
****************************************************************************************************************/
esp_err_t rtrobot_i2c_init(void)
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(I2C_NUM_0, &conf);
    return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

/***************************************************************************************************************
FS3000 Read Command
****************************************************************************************************************/
uint16_t FS3000_ReadCommand()
{
    uint8_t rev_data[5];
    uint16_t sum = 0x00;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_MASTER_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, rev_data, 4, (i2c_ack_type_t)0x0);
    i2c_master_read_byte(cmd, &rev_data[4], (i2c_ack_type_t)0x01);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_NUM_0, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    for (int i = 0; i < 5; i++)
        sum += rev_data[i];
    if ((sum & 0xff) != 0x00)
        return 0x0000; 
    else
        //The flow data is a 12-bit integer. Only the least significant four bits in the high byte are valid.
        return (uint16_t)((((uint16_t)rev_data[1] << 8) + rev_data[2]) & 0xfff);
}

/***************************************************************************************************************
FS3000 Write Command
****************************************************************************************************************/
esp_err_t FS3000_WriteCommand(uint8_t reg_addr, uint16_t send_data)
{
    uint8_t pdata[2];
    pdata[0] = send_data & 0xff;
    pdata[1] = send_data >> 8;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ESP_MASTER_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, pdata, 2, true);

    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/***************************************************************************************************************
FS3000 Read Data
****************************************************************************************************************/
float FS3000_ReadData(void)
{
    //1015
    float air_velocity_table[13] = {0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 15.0};
    int adc_table[13] = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};
    //1005
    //float air_velocity_table[9] = {0, 1.07, 2.01, 3.00, 3.97, 4.96, 5.98, 6.99, 7.23};
    //int adc_table[9] = {409, 915, 1522, 2066, 2523, 2908, 3256, 3572, 3686};
    uint16_t fm_raw = FS3000_ReadCommand();
    uint8_t fm_level = 0;
    float fm_percentage = 0;
    if (fm_raw < adc_table[0] || fm_raw > adc_table[12])
        return 0;
    for (int i = 0; i < 13; i++)
    {
        if (fm_raw > adc_table[i])
            fm_level = i;
    }
    fm_percentage = (float)(fm_raw - adc_table[fm_level]) / (adc_table[fm_level + 1] - adc_table[fm_level]);
    return (air_velocity_table[fm_level + 1] - air_velocity_table[fm_level]) * fm_percentage + air_velocity_table[fm_level];
}

/***************************************************************************************************************
FS3000 Init
****************************************************************************************************************/
void FS3000_Init(void)
{
    rtrobot_i2c_init();
}
