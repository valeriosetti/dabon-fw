#include "i2c.h"
#include "stm32f407xx.h"

// Read/write LSb
#define I2C_WRITE_MASK		0x00
#define I2C_READ_MASK		0x01

/*
 * Initialize the i2c1 peripheral
 */
int i2c_init()
{
	// Enable the GPIOB's peripheral clock
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
	// Configure PB8 and PB9 for i2c alternate function (AF4)
	GPIOB->MODER |= ((2UL << GPIO_MODER_MODE8_Pos) | (2UL << GPIO_MODER_MODE9_Pos));
	GPIOB->AFR[1] |= ((4UL << GPIO_AFRH_AFSEL8_Pos) | (4UL << GPIO_AFRH_AFSEL9_Pos));
	// Enable also the I2C1's peripheral clock
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
	// Disable the peripheral
	I2C1->CR1 &= ~I2C_CR1_PE;
	// Set the peripheral clock to 10MHz (a multiple of 10MHz is required
	// in order to support 400kHz fast mode)
	I2C1->CR2 |= (10U << I2C_CR2_FREQ_Pos);
	// Enable fast mode and Tlow/Thigh=16/9
	I2C1->CCR |= (I2C_CCR_FS | I2C_CCR_DUTY);
	// Since we want a 400kHz clock rate, we must set CCR to:
	//		1/CCR = 400e3/((9+16)*10e6) --> CCR = 625
	I2C1->CCR |= (625U << I2C_CCR_CCR_Pos);
	// In standard (100kHz) mode the max rise time is 1000ns, whereas
	// in fast mode (400kHz) is 300ns. As a consequence:
	//		TREISE = 200ns/(1/10e6) = 3
	I2C1->TRISE = ((3U + 1U) << I2C_TRISE_TRISE_Pos);
	// Enable the peripheral
	I2C1->CR1 |= I2C_CR1_PE;
	return I2C_SUCCESS;
}

/*
 * Write an array of bytes to the specified peripheral
 */
int i2c_write_buffer(uint8_t addr, uint8_t* data, uint8_t length)
{
	// Set the START bit and wait for the start condition to be sent
	I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB));
	// Write the slave address and wait for it to be transmitted. The address's LSb
	// is reset because we want to transmit data
	I2C1->DR = ((addr << 1U) | I2C_WRITE_MASK);
	while (!(I2C1->SR1 & I2C_SR1_ADDR));
	// Also SR2 must be read at this point as suggested by the datasheet
	volatile uint32_t tmp = I2C1->SR2;
	// Now send each data byte out
	while (length > 0) {
		I2C1->DR = *data;
		data++;
		length--;
		// Wait for the data to be moved to the shift register
		while(!(I2C1->SR1 & I2C_SR1_TXE));
	}
	// Wait for the last byte to be completely transmitted
	while(!(I2C1->SR1 & (I2C_SR1_TXE | I2C_SR1_BTF)));
	// Send the STOP condition
	I2C1->CR1 |= I2C_CR1_STOP;
	return I2C_SUCCESS;
}

/*
 * Read an array of bytes from the specified peripheral
 */
int i2c_read_buffer(uint8_t addr, uint8_t* data, uint8_t length)
{
	// Set the START bit and wait for the start condition to be sent. The
	// ACK bit is set only if we're going to receive more than 1 byte, otherwise
	// it will not be set.
	if (length > 1)
		I2C1->CR1 |= (I2C_CR1_START | I2C_CR1_ACK);
	else
		I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB));
	// Write the slave address and wait for it to be transmitted. The address's LSb
	// is reset because we want to transmit data
	I2C1->DR = ((addr << 1U) | I2C_READ_MASK);
	while (!(I2C1->SR1 & I2C_SR1_ADDR));
	// Also SR2 must be read at this point as suggested by the datasheet
	volatile uint32_t tmp = I2C1->SR2;
	// Read each byte
	while (length > 0) {
		// The last byte must be NACKed. The ACK bit is already cleared in case of a single
		// byte communication (see above), but it must be changed properly for multi-bytes ones.
		// Additionally, the STOP bit must also be set before receiving the last byte.
		if (length == 1) {
			I2C1->CR1 &= ~(I2C_CR1_ACK | I2C_CR1_STOP);
		}
		// Wait until new data is received
		while(!(I2C1->SR1 & I2C_SR1_RXNE));
		*data = I2C1->DR;
		data++;
		length--;
	}
	return I2C_SUCCESS;
}

/*
 * Write a single byte
 */
int i2c_write_byte(uint8_t addr, uint8_t data)
{
	return i2c_write_buffer(addr, &data, 1);
}

/*
 * Read a single byte
 */
int i2c_read_byte(uint8_t addr, uint8_t* data)
{
	return i2c_read_buffer(addr, data, 1);
}

/*
 * Check if there's any peripheral at the specified address
 */
int i2c_scan_address(uint8_t addr)
{
	// Set the START bit and wait for the start condition to be sent
	I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB));
	// Write the slave address and wait for it to be transmitted. The address's LSb
	// is reset because we want to transmit data
	I2C1->DR = ((addr << 1U) | I2C_WRITE_MASK);
	while (!(I2C1->SR1 & I2C_SR1_ADDR)) {
		// If no ACK is received for the specified address, then return an error
		if (I2C1->SR1 & I2C_SR1_AF)
			return I2C_ERROR;
	}
	// Send the STOP condition
	I2C1->CR1 |= I2C_CR1_STOP;
}
