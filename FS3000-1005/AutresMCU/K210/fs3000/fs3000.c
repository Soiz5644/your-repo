/*
 * @author: rtrobot<admin@rtrobot.org>
 * @website:rtrobot.org
 * @licence: GPL v3
 */

#include "fs3000.h"
#include <fpioa.h>
#include <bsp.h>
#include <gpiohs.h>
#include <i2c.h>

#define I2C_Slave_Address FS3000_I2CADDR
#define i2c0_sda_io 7
#define i2c0_scl_io 8

/***************************************************************************************************************
i2c master initialization
****************************************************************************************************************/
void rtrobot_i2c_init(void)
{
    fpioa_set_function(i2c0_scl_io, FUNC_I2C0_SCLK);
	fpioa_set_function(i2c0_sda_io, FUNC_I2C0_SDA);
	i2c_init(I2C_DEVICE_0, I2C_Slave_Address, 7, 20000);
}

/***************************************************************************************************************
FS3000 Read Command
****************************************************************************************************************/
uint16_t FS3000_ReadCommand()
{
    uint8_t rev_data[5];
    uint16_t sum = 0x00;

    i2c_recv_data(I2C_DEVICE_0, NULL, 0, rev_data, 5);
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
void FS3000_WriteCommand(uint8_t reg_addr, uint16_t send_data)
{
	uint8_t* buf[3];
	buf[0] = reg_addr;
	buf[0] = send_data & 0xff;
	buf[1] = send_data >> 8;

	//if you used core1,don't use dma.
	//i2c_send_data_dma(DMAC_CHANNEL0, I2C_DEVICE_0, buf, length + 1);
	i2c_send_data(I2C_DEVICE_0, buf, 3);
}

/***************************************************************************************************************
FS3000 Read Data
****************************************************************************************************************/
float FS3000_ReadData(void)
{
    float air_velocity_table[13] = {0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 13.0, 15.0};
    int adc_table[13] = {409, 1203, 1597, 1908, 2187, 2400, 2629, 2801, 3006, 3178, 3309, 3563, 3686};
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
