#include "platform.h"
#include "stm32f4xx_conf.h"
#include <stm32f4xx.h>
#include "ruuvi_errors.h"

/* Definitions */
I2C_TypeDef *const i2c[] = {I2C1, I2C2, I2C3};
static const u32 i2c_rcc[] = {RCC_APB1Periph_I2C1, RCC_APB1Periph_I2C2, RCC_APB1Periph_I2C3};
static const u8 i2c_af[] = {GPIO_AF_I2C1, GPIO_AF_I2C2, GPIO_AF_I2C3};

#if defined( ELUA_BOARD_RUUVIA )
static const u32 i2c_port_rcc[] = {RCC_AHB1Periph_GPIOB, 0, 0};
static GPIO_TypeDef *const i2c_port[] = {GPIOB, NULL, NULL};
static const u16 i2c_sda_pin[] = {GPIO_Pin_7, 0, 0};
static const u16 i2c_scl_pin[] = {GPIO_Pin_6, 0, 0};
static const u8 i2c_sda_pinsource[] = {GPIO_PinSource7, 0, 0};
static const u8 i2c_scl_pinsource[] = {GPIO_PinSource6, 0, 0};
#elif defined( ELUA_BOARD_RUUVIB1 )
static const u32 i2c_port_rcc[] = {RCC_AHB1Periph_GPIOB, 0, 0};
static GPIO_TypeDef *const i2c_port[] = {GPIOB, NULL, NULL};
static const u16 i2c_sda_pin[] = {GPIO_Pin_9, 0, 0};
static const u16 i2c_scl_pin[] = {GPIO_Pin_8, 0, 0};
static const u8 i2c_sda_pinsource[] = {GPIO_PinSource9, 0, 0};
static const u8 i2c_scl_pinsource[] = {GPIO_PinSource8, 0, 0};
#elif defined( ELUA_BOARD_RUUVIC1 )
static const u32 i2c_port_rcc[] = {RCC_AHB1Periph_GPIOB, 0, 0};
static GPIO_TypeDef *const i2c_port[] = {GPIOB, NULL, NULL};
static const u16 i2c_sda_pin[] = {GPIO_Pin_7, 0, 0};
static const u16 i2c_scl_pin[] = {GPIO_Pin_6, 0, 0};
static const u8 i2c_sda_pinsource[] = {GPIO_PinSource7, 0, 0};
static const u8 i2c_scl_pinsource[] = {GPIO_PinSource6, 0, 0};
#else
#error "Define I2C pins/ports for this board in platform_i2c.c"
#endif

#define TIMEOUT 4096

/* I2C Functions */
u32 platform_i2c_setup( unsigned id, u32 speed )
{
	GPIO_InitTypeDef GPIO_InitStruct;
	I2C_InitTypeDef I2C_InitStruct;

	// enable APB1 peripheral clock for I2C1
	RCC_APB1PeriphClockCmd(i2c_rcc[id], ENABLE);
	// enable clock for SCL and SDA pins
	RCC_AHB1PeriphClockCmd(i2c_port_rcc[id], ENABLE);

	I2C_DeInit(i2c[id]);

	/* setup SCL and SDA pins
	 * You can connect I2C1 to two different
	 * pairs of pins:
	 * 1. SCL on PB6 and SDA on PB7
	 * 2. SCL on PB8 and SDA on PB9
	 */
	GPIO_InitStruct.GPIO_Pin = i2c_scl_pin[id] | i2c_sda_pin[id]; // pins to use
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;		 // set pins to alternate function
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;		// set GPIO speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_OD;		// set output to open drain --> the line has to be only pulled low, not driven high
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;		// enable pull up resistors
	GPIO_Init(i2c_port[id], &GPIO_InitStruct);			// init GPIO

	// Connect I2C1 pins to AF
	GPIO_PinAFConfig(i2c_port[id], i2c_scl_pinsource[id], i2c_af[id]);	// SCL
	GPIO_PinAFConfig(i2c_port[id], i2c_sda_pinsource[id], i2c_af[id]); // SDA

	// configure I2C1
	I2C_StructInit(&I2C_InitStruct);
	I2C_InitStruct.I2C_ClockSpeed = speed; 		//set speed (100kHz or 400kHz)
	I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;			// I2C mode
	I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;	// 50% duty cycle --> standard
	I2C_InitStruct.I2C_OwnAddress1 = 0x00;			// own address, not relevant in master mode
	I2C_InitStruct.I2C_Ack = I2C_Ack_Disable;		// disable acknowledge when reading (can be changed later on)
	I2C_InitStruct.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
	I2C_Init(i2c[id], &I2C_InitStruct);				// init I2C1

	// enable I2C1
	I2C_Cmd(i2c[id], ENABLE);

	return speed;
}

rt_error platform_i2c_send_start( unsigned id )
{
	RT_TIMEOUT_INIT();
	// wait until I2C1 is not busy anymore
	while(I2C_GetFlagStatus(i2c[id], I2C_FLAG_BUSY))
	{
		RT_TIMEOUT_CHECK( RT_DEFAULT_TIMEOUT );
	}

	// Send I2C1 START condition
	I2C_GenerateSTART(i2c[id], ENABLE);
	return RT_ERR_OK;
}

rt_error platform_i2c_send_stop( unsigned id )
{
	RT_TIMEOUT_INIT();
	// wait until I2C1 is not busy anymore
	while(I2C_GetFlagStatus(i2c[id], I2C_FLAG_BUSY))
	{
		RT_TIMEOUT_CHECK( RT_DEFAULT_TIMEOUT );
	}

	// Send I2C1 STOP Condition
	I2C_GenerateSTOP(i2c[id], ENABLE);

	return RT_ERR_OK;
}

/* Send 7bit address to I2C buss */
/* Adds R/W bit to end of address.(Should not be included in address) */
rt_error platform_i2c_send_address( unsigned id, u16 address, int direction )
{
	RT_TIMEOUT_INIT();
	// wait for I2C1 EV5 --> Master has acknowledged start condition
	while(SUCCESS != I2C_CheckEvent(i2c[id], I2C_EVENT_MASTER_MODE_SELECT))
	{
		RT_TIMEOUT_CHECK( RT_DEFAULT_TIMEOUT );
	}

	address<<=1; //Shift 7bit address to left by one to leave room for R/W bit
	RT_TIMEOUT_REINIT();

	/* wait for I2C1 EV6, check if
	 * either Slave has acknowledged Master transmitter or
	 * Master receiver mode, depending on the transmission
	 * direction
	 */
	if(direction == PLATFORM_I2C_DIRECTION_TRANSMITTER) {
		I2C_Send7bitAddress(i2c[id], address, I2C_Direction_Transmitter);
		while(SUCCESS != I2C_CheckEvent(i2c[id], I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
		{
			RT_TIMEOUT_CHECK( RT_DEFAULT_TIMEOUT );
		}
	} else if(direction == PLATFORM_I2C_DIRECTION_RECEIVER) {
		I2C_Send7bitAddress(i2c[id], address, I2C_Direction_Receiver);
		while(SUCCESS != I2C_CheckEvent(i2c[id], I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
		{
			RT_TIMEOUT_CHECK( RT_DEFAULT_TIMEOUT );
		}
	}
	return RT_ERR_OK;
}

/* Send one byte to I2C bus */
int platform_i2c_send_byte( unsigned id, u8 data )
{
	I2C_SendData(i2c[id], data);
	// wait for I2C1 EV8_2 --> byte has been transmitted
	while(!I2C_CheckEvent(i2c[id], I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	return 1;
}

/* This function reads one byte from the slave device */
int platform_i2c_recv_byte( unsigned id, int ack )
{
	if(ack) // enable acknowledge of recieved data
		I2C_AcknowledgeConfig(i2c[id], ENABLE);
	else // disabe acknowledge of received data
		I2C_AcknowledgeConfig(i2c[id], DISABLE);
	// wait until one byte has been received
	while( !I2C_CheckEvent(i2c[id], I2C_EVENT_MASTER_BYTE_RECEIVED) );
	// read data from I2C data register and return data byte
	uint8_t data = I2C_ReceiveData(i2c[id]);
	return data;
}

static int _i2c_recv_buf(unsigned id, u8 *buff, int len)
{
	int i, ack;
	for (i=0;i<len;i++) {
		if (i<(len-1))
			ack = 1;
		else
			ack = 0;
		buff[i] = platform_i2c_recv_byte(id, ack);
	}
	return i;
}

static int _i2c_send_buf(unsigned id, u8 *buff, int len)
{
	int i;
	for (i=0;i<len;i++) {
		platform_i2c_send_byte(id, buff[i]);
	}
	return i;
}

/* Read from device using 8 bit offset addressing */
int platform_i2c_read8(unsigned id, u8 device, u8 offset, u8 *buff, int len)
{
	int rc;
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_TRANSMITTER);
	if (1 != rc)
		goto END;
	platform_i2c_send_byte(id, offset);
	platform_i2c_send_stop(id);
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_RECEIVER);
	if (1 != rc)
		goto END;
	rc = _i2c_recv_buf(id, buff, len);
END:
	platform_i2c_send_stop(id);
	return rc;
}

/* Write to device using 8bit addressing */
int platform_i2c_write8(unsigned id, u8 device, u8 offset, u8 *buff, int len)
{
	int rc;
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_TRANSMITTER);
	if (1 != rc)
		goto END;
	platform_i2c_send_byte(id, offset);
	rc = _i2c_send_buf(id, buff, len);
END:
	platform_i2c_send_stop(id);
	return rc;
}

static void _i2c_send_offset16(unsigned id, u8 offset)
{
	platform_i2c_send_byte(id, (offset>>8)&0xff);
	platform_i2c_send_byte(id, (offset&0xff));
}

/* Read from device using 16bit offset */
int platform_i2c_read16(unsigned id, u8 device, u16 offset, u8 *buff, int len)
{
	int rc;
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_TRANSMITTER);
	if (1 != rc)
		goto END;
	_i2c_send_offset16(id, offset);
	platform_i2c_send_stop(id);
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_RECEIVER);
	if (1 != rc)
		goto END;
	rc = _i2c_recv_buf(id, buff, len);
END:
	platform_i2c_send_stop(id);
	return rc;
}

/* Write to device using 16bit addressing */
int platform_i2c_write16(unsigned id, u8 device, u16 offset, u8 *buff, int len)
{
	int rc;
	platform_i2c_send_start(id);
	rc = platform_i2c_send_address(id, device, PLATFORM_I2C_DIRECTION_TRANSMITTER);
	if (1 != rc)
		goto END;
	_i2c_send_offset16(id, offset);
	rc = _i2c_send_buf(id, buff, len);
END:
	platform_i2c_send_stop(id);
	return rc;
}
