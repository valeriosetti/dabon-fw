#include "i2c.h"
#include "stm32f407xx.h"
#include "clock_configuration.h"
#include "gpio.h"
#include "systick.h"
#include "debug_printf.h"

#define debug_msg(format, ...)		debug_printf("[i2c] " format, ##__VA_ARGS__)

// Interface properties
#define I2C_CLOCK_FREQ				100000		// in Hz
#define I2C_TIMEOUT					100			// in ms

// Read/write LSb
#define I2C_WRITE_MASK				0x00
#define I2C_READ_MASK				0x01

#define I2C_DUTYCYCLE_DEFAULT		0x00
#define I2C_DUTYCYCLE_1_2			0x01
#define I2C_DUTYCYCLE_16_9			0x02

#define I2C_FREQRANGE(__PCLK__)                            ((__PCLK__)/1000000U)
#define I2C_RISE_TIME(__FREQRANGE__, __SPEED__)            (((__SPEED__) <= 100000U) ? ((__FREQRANGE__) + 1U) : ((((__FREQRANGE__) * 300U) / 1000U) + 1U))
#define I2C_SPEED_STANDARD(__PCLK__, __SPEED__)            (((((__PCLK__)/((__SPEED__) << 1U)) & I2C_CCR_CCR) < 4U)? 4U:((__PCLK__) / ((__SPEED__) << 1U)))
#define I2C_SPEED_FAST(__PCLK__, __SPEED__, __DUTYCYCLE__) (((__DUTYCYCLE__) == I2C_DUTYCYCLE_1_2)? ((__PCLK__) / ((__SPEED__) * 3U)) : (((__PCLK__) / ((__SPEED__) * 25U)) | I2C_CCR_DUTY))
#define I2C_SPEED(__PCLK__, __SPEED__, __DUTYCYCLE__)      (((__SPEED__) <= 100000U)? (I2C_SPEED_STANDARD((__PCLK__), (__SPEED__))) : \
                                                                  ((I2C_SPEED_FAST((__PCLK__), (__SPEED__), (__DUTYCYCLE__)) & I2C_CCR_CCR) == 0U)? 1U : \
                                                                  ((I2C_SPEED_FAST((__PCLK__), (__SPEED__), (__DUTYCYCLE__))) | I2C_CCR_FS))

// Private functions' prototypes
static int32_t i2c_send_start(void);
static int32_t i2c_send_address(uint8_t address, uint8_t operation_mask);
static int32_t i2c_send_byte(uint8_t data, uint8_t wait_for_tx_to_complete);
static int32_t i2c_receive_byte(uint8_t* data, uint8_t bytes_left);

/*
 * Initialize the i2c1 peripheral
 */
int32_t i2c_init()
{
	// Enable the GPIOB's peripheral clock
	RCC_GPIOB_CLK_ENABLE();
	// Configure PB8 and PB9 for i2c alternate function (AF4) and open drain
	GPIOB->MODER |= ((MODER_ALTERNATE << GPIO_MODER_MODE8_Pos) | (MODER_ALTERNATE << GPIO_MODER_MODE9_Pos));
	GPIOB->OTYPER |= (GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);
	GPIOB->AFR[1] |= ((4UL << GPIO_AFRH_AFSEL8_Pos) | (4UL << GPIO_AFRH_AFSEL9_Pos));
	// Enable also the I2C1's peripheral clock
	RCC_I2C1_CLK_ENABLE();
	// Disable the peripheral
	I2C1->CR1 &= ~I2C_CR1_PE;
	// Set the peripheral clock to 10MHz (a multiple of 10MHz is required
	// in order to support 400kHz fast mode)
	I2C1->CR2 |= (I2C_FREQRANGE(APB1_freq) << I2C_CR2_FREQ_Pos);
	// Configure the High/Low time duration of the clock
	I2C1->CCR |= (I2C_SPEED(APB1_freq, I2C_CLOCK_FREQ, I2C_DUTYCYCLE_DEFAULT)) << I2C_CCR_CCR_Pos;
	// In standard (100kHz) mode the max rise time is 1000ns, whereas
	// in fast mode (400kHz) is 300ns
	I2C1->TRISE = (I2C_RISE_TIME(APB1_freq, I2C_CLOCK_FREQ) << I2C_TRISE_TRISE_Pos);
	// Enable the peripheral
	I2C1->CR1 |= I2C_CR1_PE;
	return 0;
}

/*
 * Send the start condition in order to begin the communication
 */
static int32_t i2c_send_start()
{
	uint32_t start_tick = systick_get_tick_count();
	// Set the START bit and wait for the start condition to be sent
	I2C1->CR1 |= I2C_CR1_START;
	while (!(I2C1->SR1 & I2C_SR1_SB)) {
		if ((systick_get_tick_count() - start_tick) > I2C_TIMEOUT) {
			debug_msg("Start bit timeout\n");
			return -1;
		}
	}
	return 0;
}

/*
 * Send the address byte and wait for it to be transmitted
 */
static int32_t i2c_send_address(uint8_t address, uint8_t operation_mask)
{
	uint32_t start_tick = systick_get_tick_count();
	
	I2C1->DR = ((address << 1U) | operation_mask);
	while (!(I2C1->SR1 & I2C_SR1_ADDR)) {
		// If no ACK is received for the specified address, then send the stop condition and return an error
		if (I2C1->SR1 & I2C_SR1_AF) {
			I2C1->SR1 &= ~I2C_SR1_AF;
			debug_msg("ACK bit not received for address 0x%x\n", address);
			return -1;
		}
		if ((systick_get_tick_count() - start_tick) > I2C_TIMEOUT) {
			debug_msg("Send address timed out\n");
			return -2;
		}
	}
	// Also SR2 must be read at this point as suggested by the datasheet
	volatile uint32_t tmp = I2C1->SR2;

	return 0;
}

/*
 * Send a single byte of data to the peripheral:
 * 	- if this is the last byte of the transmission, then wait for the transmission
 * 		to be completed
 * 	- otherwise, just wait for the data register to be empty in order to move
 * 		to the next byte
 */
static int32_t i2c_send_byte(uint8_t data, uint8_t wait_for_tx_to_complete)
{
	uint32_t start_tick = systick_get_tick_count();
	
	I2C1->DR = data;
	if (wait_for_tx_to_complete){
		// Wait for the last byte to be completely transmitted
		while(!(I2C1->SR1 & (I2C_SR1_TXE | I2C_SR1_BTF))) {
			if ((systick_get_tick_count() - start_tick) > I2C_TIMEOUT) {
				debug_msg("Send byte timed out\n");
				return -1;
			}
		}
	} else {
		// Wait for the data to be moved to the shift register
		while(!(I2C1->SR1 & I2C_SR1_TXE)) {
			if ((systick_get_tick_count() - start_tick) > I2C_TIMEOUT) {
				debug_msg("Send byte timed out\n");
				return -2;
			}
		}
	}

	return 0;
}

/*
 * Receive a single byte of data.
 * NOTE = The last byte must be NACKed. Additionally, the STOP bit must also be set before receiving the last byte.
 */
static int32_t i2c_receive_byte(uint8_t* data, uint8_t bytes_left)
{
	uint32_t start_tick = systick_get_tick_count();
	
	if (bytes_left == 1) {
		I2C1->CR1 = (I2C1->CR1 & (~I2C_CR1_ACK));
	} else {
		I2C1->CR1 |= I2C_CR1_ACK;
	}
	// Wait until new data is received
	while(!(I2C1->SR1 & I2C_SR1_RXNE)) {
		if ((systick_get_tick_count() - start_tick) > I2C_TIMEOUT) {
			debug_msg("Receive byte timed out\n");
			return -2;
		}
	}
	*data = I2C1->DR;

	return 0;
}

/*
 * Send the stop condition on the bus to terminate the communication
 */
#define i2c_send_stop()		do { I2C1->CR1 |= I2C_CR1_STOP; } while(0)

/*
 * Write an array of bytes to the specified peripheral
 */
int32_t i2c_write_buffer(uint8_t addr, uint8_t* data, uint8_t length)
{
	int ret_val = 0;

	ret_val = i2c_send_start();
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	ret_val = i2c_send_address(addr, I2C_WRITE_MASK);
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	while (length > 0) {
		ret_val = i2c_send_byte(*data, (length==1));
		if (ret_val != 0) {
			i2c_send_stop();
			return ret_val;
		}
		data++;
		length--;
	}

	i2c_send_stop();

	return ret_val;
}

/*
 * Read an array of bytes from the specified peripheral
 */
int32_t i2c_read_buffer(uint8_t addr, uint8_t* data, uint8_t length)
{
	int ret_val;

	ret_val = i2c_send_start();
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	ret_val = i2c_send_address(addr, I2C_READ_MASK);
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	while (length > 0) {
		ret_val = i2c_receive_byte(data, length);
		if (ret_val != 0) {
			i2c_send_stop();
			return ret_val;
		}
		data++;
		length--;
	}

	i2c_send_stop();

	return 0;
}

/*
 * Write a single byte
 */
int32_t i2c_write_byte(uint8_t addr, uint8_t data)
{
	return i2c_write_buffer(addr, &data, 1);
}

/*
 * Read a single byte
 */
int32_t i2c_read_byte(uint8_t addr, uint8_t* data)
{
	return i2c_read_buffer(addr, data, 1);
}

/*
 * Check if there's any peripheral at the specified address
 */
int32_t i2c_scan_address(uint8_t addr)
{
	int ret_val;

	ret_val = i2c_send_start();
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	ret_val = i2c_send_address(addr, I2C_READ_MASK);
	if (ret_val != 0) {
		i2c_send_stop();
		return ret_val;
	}

	// Send the STOP condition
	i2c_send_stop();
	return 0;
}

/*
 *	Scan all the allowed I2C addresses
 */
int32_t i2c_scan_peripherals()
{
	uint8_t addr;
	int32_t ret_val;
	for (addr = 0; addr < 0x7F; addr++) {
		ret_val = i2c_scan_address(addr);
		if (ret_val == 0) {
			debug_msg(">>> peripheral found at address 0x%x <<<\n", addr);
		}
	}
}
