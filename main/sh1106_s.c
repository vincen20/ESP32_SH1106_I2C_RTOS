#include <stdio.h>
#include "driver/i2c.h"

#define I2C_MASTER_NUM    I2C_NUM_0    
#define I2C_MASTER_SCL_IO    26    /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO    25    /*!< gpio number for I2C master data  */
#define I2C_MASTER_TX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0   /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ    100000     /*!< I2C master clock frequency,<400 khz*/
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define SH1106_I2C_ADDRESS   0x78 

esp_err_t i2c_cmd(i2c_port_t i2c_num,uint8_t cmdx){
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SH1106_I2C_ADDRESS, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x00, ACK_CHECK_EN);//CMD START...
	i2c_master_write_byte(cmd, cmdx, ACK_CHECK_EN);//CMD
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
		printf("cmd err:%d\n",ret);
        return ret;
    }
    return ESP_OK;
}
esp_err_t i2c_data(i2c_port_t i2c_num,uint8_t datas){
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, SH1106_I2C_ADDRESS,ACK_CHECK_EN);
	i2c_master_write_byte(cmd, 0x40, ACK_CHECK_EN);//data cmd contra data
	i2c_master_write_byte(cmd, datas, ACK_CHECK_EN);
	i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, 200 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    if (ret == ESP_FAIL) {
		printf("data err:%d\n",ret);
        return ret;
    }
    return ESP_OK;
}
esp_err_t oled_setxy(i2c_port_t i2c_num,uint8_t x,uint8_t y)
{
	uint8_t h=(0xf0&x)>>4;
	uint8_t l=(0x0f&x)>>4;
	esp_err_t ret=i2c_cmd(i2c_num,0xb0|y);  //0154 3210
	ret=i2c_cmd(i2c_num,l|0x01);            //0000 3210
	ret=i2c_cmd(i2c_num,h|0x10);            //0001 7654
	return ret;
}
esp_err_t oled_cls(i2c_port_t i2c_num)
{
    uint8_t x,y,ret;
    for(y=0;y<64;y++)
    {
		if(oled_setxy(i2c_num,0,y))
		  return ESP_OK;
        for(x=0;x<132;x++)
        {
         if(i2c_data(i2c_num,0x00))
		  return ESP_OK; 
        } 
    }
    return ret;
}

void sh1106_init(i2c_port_t i2c_num){
	i2c_cmd(i2c_num,0xAE);//--turn off oled panel
	i2c_cmd(i2c_num,0x00);//---set low column address
	i2c_cmd(i2c_num,0x10);//---set high column address0-131 X
	i2c_cmd(i2c_num,0x40);//--set start line address 0-63  Y
	i2c_cmd(i2c_num,0x81);//--set contrast control register
	i2c_cmd(i2c_num,0xFF);
	i2c_cmd(i2c_num,0xA1);//--Set SEG/Column Mapping     a1/a0
	i2c_cmd(i2c_num,0xC8);//Set COM/Row Scan Direction   c8/c0
	i2c_cmd(i2c_num,0xA6);//--set normal display
	i2c_cmd(i2c_num,0xA8);//--set multiplex ratio(1 to 64)
	i2c_cmd(i2c_num,0x3F);//--1/64 duty
	i2c_cmd(i2c_num,0xD3);//-set display offset	Shift Mapping RAM Counter 
	i2c_cmd(i2c_num,0x00);//-not offset
	i2c_cmd(i2c_num,0xD5);//--set display clock divide ratio/oscillator frequency
	i2c_cmd(i2c_num,0x50);//--set divide ratio, Set Clock as 100 Frames/Sec
	i2c_cmd(i2c_num,0xD9);//--set pre-charge period
	i2c_cmd(i2c_num,0x22);//Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
	i2c_cmd(i2c_num,0xDA);//--set com pins hardware configuration
	i2c_cmd(i2c_num,0x12);
	i2c_cmd(i2c_num,0xDB);//--set vcomh
	i2c_cmd(i2c_num,0x40);//Set VCOM Deselect Level
	i2c_cmd(i2c_num,0xA4);// Disable Entire Display On (0xa4/0xa5)
	i2c_cmd(i2c_num,0xA6);// Disable Inverse Display On (0xa6/a7) 
	i2c_cmd(i2c_num,0xAF);//--turn on oled panel
} 

esp_err_t i2c_testx(i2c_port_t i2c_num){
	sh1106_init(I2C_MASTER_NUM);
	oled_cls(I2C_MASTER_NUM);
    oled_setxy(I2C_MASTER_NUM,3,3);
	i2c_data(I2C_MASTER_NUM,0x00); 
	i2c_data(I2C_MASTER_NUM,0x76); 
	i2c_data(I2C_MASTER_NUM,0x89); 
	i2c_data(I2C_MASTER_NUM,0x89); 
	i2c_data(I2C_MASTER_NUM,0x89); 
	i2c_data(I2C_MASTER_NUM,0x72); 
      vTaskDelay(50000 / portTICK_RATE_MS);
	return ESP_OK;
}

void i2c_test_task()
{
    esp_err_t ret;
    while (1) {
       ret =i2c_testx(I2C_MASTER_NUM);
	 if (ret == ESP_OK) {
            printf("espok\n");
        } else {
            printf("No ack, sensor not connected...skip...\n");
        }

	}
}

esp_err_t i2c_master_init()
{
    i2c_port_t i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER; 
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    esp_err_t ret=i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    return ret;
}
void app_main()
{
	esp_err_t ret=i2c_master_init();
	if(ret!=ESP_OK){
	printf("i2c_init err%d",ret);
	}
	xTaskCreate(i2c_test_task, "i2c_test_task_0", 1024 * 2,0, 10, NULL);
}

