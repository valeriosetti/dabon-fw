#include "sgtl5000.h"
#include "i2c.h"
#include "debug_printf.h"
#include "output_i2s.h"
#include "utils.h"

#define debug_msg(...)		debug_printf_with_tag("[sgtl5000] ", __VA_ARGS__)

#define SGTL5000_I2C_ADDRESS		0x0A

/*
 * Registers addresses
 */
#define SGTL5000_CHIP_ID				0x0000
#define SGTL5000_CHIP_DIG_POWER			0x0002
#define SGTL5000_CHIP_CLK_CTRL			0x0004
#define SGTL5000_CHIP_I2S_CTRL			0x0006
#define SGTL5000_CHIP_SSS_CTRL			0x000a
#define SGTL5000_CHIP_ADCDAC_CTRL		0x000e
#define SGTL5000_CHIP_DAC_VOL			0x0010
#define SGTL5000_CHIP_PAD_STRENGTH		0x0014
#define SGTL5000_CHIP_ANA_ADC_CTRL		0x0020
#define SGTL5000_CHIP_ANA_HP_CTRL		0x0022
#define SGTL5000_CHIP_ANA_CTRL			0x0024
#define SGTL5000_CHIP_LINREG_CTRL		0x0026
#define SGTL5000_CHIP_REF_CTRL			0x0028
#define SGTL5000_CHIP_MIC_CTRL			0x002a
#define SGTL5000_CHIP_LINE_OUT_CTRL		0x002c
#define SGTL5000_CHIP_LINE_OUT_VOL		0x002e
#define SGTL5000_CHIP_ANA_POWER			0x0030
#define SGTL5000_CHIP_PLL_CTRL			0x0032
#define SGTL5000_CHIP_CLK_TOP_CTRL		0x0034
#define SGTL5000_CHIP_ANA_STATUS		0x0036
#define SGTL5000_CHIP_SHORT_CTRL		0x003c
#define SGTL5000_CHIP_ANA_TEST2			0x003a
#define SGTL5000_DAP_CTRL				0x0100
#define SGTL5000_DAP_PEQ				0x0102
#define SGTL5000_DAP_BASS_ENHANCE		0x0104
#define SGTL5000_DAP_BASS_ENHANCE_CTRL	0x0106
#define SGTL5000_DAP_AUDIO_EQ			0x0108
#define SGTL5000_DAP_SURROUND			0x010a
#define SGTL5000_DAP_FLT_COEF_ACCESS	0x010c
#define SGTL5000_DAP_COEF_WR_B0_MSB		0x010e
#define SGTL5000_DAP_COEF_WR_B0_LSB		0x0110
#define SGTL5000_DAP_EQ_BASS_BAND0		0x0116
#define SGTL5000_DAP_EQ_BASS_BAND1		0x0118
#define SGTL5000_DAP_EQ_BASS_BAND2		0x011a
#define SGTL5000_DAP_EQ_BASS_BAND3		0x011c
#define SGTL5000_DAP_EQ_BASS_BAND4		0x011e
#define SGTL5000_DAP_MAIN_CHAN			0x0120
#define SGTL5000_DAP_MIX_CHAN			0x0122
#define SGTL5000_DAP_AVC_CTRL			0x0124
#define SGTL5000_DAP_AVC_THRESHOLD		0x0126
#define SGTL5000_DAP_AVC_ATTACK			0x0128
#define SGTL5000_DAP_AVC_DECAY			0x012a
#define SGTL5000_DAP_COEF_WR_B1_MSB		0x012c
#define SGTL5000_DAP_COEF_WR_B1_LSB		0x012e
#define SGTL5000_DAP_COEF_WR_B2_MSB		0x0130
#define SGTL5000_DAP_COEF_WR_B2_LSB		0x0132
#define SGTL5000_DAP_COEF_WR_A1_MSB		0x0134
#define SGTL5000_DAP_COEF_WR_A1_LSB		0x0136
#define SGTL5000_DAP_COEF_WR_A2_MSB		0x0138
#define SGTL5000_DAP_COEF_WR_A2_LSB		0x013a

/*
 * SGTL5000_CHIP_ID
 */
#define SGTL5000_PARTID_MASK			0xff00
#define SGTL5000_PARTID_SHIFT			8
#define SGTL5000_PARTID_WIDTH			8
#define SGTL5000_PARTID_PART_ID			0xa0
#define SGTL5000_REVID_MASK			0x00ff
#define SGTL5000_REVID_SHIFT			0
#define SGTL5000_REVID_WIDTH			8

/*
 * SGTL5000_CHIP_DIG_POWER
 */
#define SGTL5000_ADC_EN				0x0040
#define SGTL5000_DAC_EN				0x0020
#define SGTL5000_DAP_POWERUP			0x0010
#define SGTL5000_I2S_OUT_POWERUP		0x0002
#define SGTL5000_I2S_IN_POWERUP			0x0001

/*
 * SGTL5000_CHIP_CLK_CTRL
 */
#define SGTL5000_CHIP_CLK_CTRL_DEFAULT		0x0008
#define SGTL5000_RATE_MODE_MASK			0x0030
#define SGTL5000_RATE_MODE_SHIFT		4
#define SGTL5000_RATE_MODE_WIDTH		2
#define SGTL5000_RATE_MODE_DIV_1		0
#define SGTL5000_RATE_MODE_DIV_2		1
#define SGTL5000_RATE_MODE_DIV_4		2
#define SGTL5000_RATE_MODE_DIV_6		3
#define SGTL5000_SYS_FS_MASK			0x000c
#define SGTL5000_SYS_FS_SHIFT			2
#define SGTL5000_SYS_FS_WIDTH			2
#define SGTL5000_SYS_FS_32k			0x0
#define SGTL5000_SYS_FS_44_1k			0x1
#define SGTL5000_SYS_FS_48k			0x2
#define SGTL5000_SYS_FS_96k			0x3
#define SGTL5000_MCLK_FREQ_MASK			0x0003
#define SGTL5000_MCLK_FREQ_SHIFT		0
#define SGTL5000_MCLK_FREQ_WIDTH		2
#define SGTL5000_MCLK_FREQ_256FS		0x0
#define SGTL5000_MCLK_FREQ_384FS		0x1
#define SGTL5000_MCLK_FREQ_512FS		0x2
#define SGTL5000_MCLK_FREQ_PLL			0x3

/*
 * SGTL5000_CHIP_I2S_CTRL
 */
#define SGTL5000_I2S_SCLKFREQ_MASK		0x0100
#define SGTL5000_I2S_SCLKFREQ_SHIFT		8
#define SGTL5000_I2S_SCLKFREQ_WIDTH		1
#define SGTL5000_I2S_SCLKFREQ_64FS		0x0
#define SGTL5000_I2S_SCLKFREQ_32FS		0x1	/* Not for RJ mode */
#define SGTL5000_I2S_MASTER			0x0080
#define SGTL5000_I2S_SCLK_INV			0x0040
#define SGTL5000_I2S_DLEN_MASK			0x0030
#define SGTL5000_I2S_DLEN_SHIFT			4
#define SGTL5000_I2S_DLEN_WIDTH			2
#define SGTL5000_I2S_DLEN_32			0x0
#define SGTL5000_I2S_DLEN_24			0x1
#define SGTL5000_I2S_DLEN_20			0x2
#define SGTL5000_I2S_DLEN_16			0x3
#define SGTL5000_I2S_MODE_MASK			0x000c
#define SGTL5000_I2S_MODE_SHIFT			2
#define SGTL5000_I2S_MODE_WIDTH			2
#define SGTL5000_I2S_MODE_I2S_LJ		0x0
#define SGTL5000_I2S_MODE_RJ			0x1
#define SGTL5000_I2S_MODE_PCM			0x2
#define SGTL5000_I2S_LRALIGN			0x0002
#define SGTL5000_I2S_LRPOL			0x0001	/* set for which mode */

/*
 * SGTL5000_CHIP_SSS_CTRL
 */
#define SGTL5000_DAP_MIX_LRSWAP			0x4000
#define SGTL5000_DAP_LRSWAP			0x2000
#define SGTL5000_DAC_LRSWAP			0x1000
#define SGTL5000_I2S_OUT_LRSWAP			0x0400
#define SGTL5000_DAP_MIX_SEL_MASK		0x0300
#define SGTL5000_DAP_MIX_SEL_SHIFT		8
#define SGTL5000_DAP_MIX_SEL_WIDTH		2
#define SGTL5000_DAP_MIX_SEL_ADC		0x0
#define SGTL5000_DAP_MIX_SEL_I2S_IN		0x1
#define SGTL5000_DAP_SEL_MASK			0x00c0
#define SGTL5000_DAP_SEL_SHIFT			6
#define SGTL5000_DAP_SEL_WIDTH			2
#define SGTL5000_DAP_SEL_ADC			0x0
#define SGTL5000_DAP_SEL_I2S_IN			0x1
#define SGTL5000_DAC_SEL_MASK			0x0030
#define SGTL5000_DAC_SEL_SHIFT			4
#define SGTL5000_DAC_SEL_WIDTH			2
#define SGTL5000_DAC_SEL_ADC			0x0
#define SGTL5000_DAC_SEL_I2S_IN			0x1
#define SGTL5000_DAC_SEL_DAP			0x3
#define SGTL5000_I2S_OUT_SEL_MASK		0x0003
#define SGTL5000_I2S_OUT_SEL_SHIFT		0
#define SGTL5000_I2S_OUT_SEL_WIDTH		2
#define SGTL5000_I2S_OUT_SEL_ADC		0x0
#define SGTL5000_I2S_OUT_SEL_I2S_IN		0x1
#define SGTL5000_I2S_OUT_SEL_DAP		0x3

/*
 * SGTL5000_CHIP_ADCDAC_CTRL
 */
#define SGTL5000_VOL_BUSY_DAC_RIGHT		0x2000
#define SGTL5000_VOL_BUSY_DAC_LEFT		0x1000
#define SGTL5000_DAC_VOL_RAMP_EN		0x0200
#define SGTL5000_DAC_VOL_RAMP_EXPO		0x0100
#define SGTL5000_DAC_MUTE_RIGHT			0x0008
#define SGTL5000_DAC_MUTE_LEFT			0x0004
#define SGTL5000_ADC_HPF_FREEZE			0x0002
#define SGTL5000_ADC_HPF_BYPASS			0x0001

/*
 * SGTL5000_CHIP_DAC_VOL
 */
#define SGTL5000_DAC_VOL_RIGHT_MASK		0xff00
#define SGTL5000_DAC_VOL_RIGHT_SHIFT		8
#define SGTL5000_DAC_VOL_RIGHT_WIDTH		8
#define SGTL5000_DAC_VOL_LEFT_MASK		0x00ff
#define SGTL5000_DAC_VOL_LEFT_SHIFT		0
#define SGTL5000_DAC_VOL_LEFT_WIDTH		8

/*
 * SGTL5000_CHIP_PAD_STRENGTH
 */
#define SGTL5000_PAD_I2S_LRCLK_MASK		0x0300
#define SGTL5000_PAD_I2S_LRCLK_SHIFT		8
#define SGTL5000_PAD_I2S_LRCLK_WIDTH		2
#define SGTL5000_PAD_I2S_SCLK_MASK		0x00c0
#define SGTL5000_PAD_I2S_SCLK_SHIFT		6
#define SGTL5000_PAD_I2S_SCLK_WIDTH		2
#define SGTL5000_PAD_I2S_DOUT_MASK		0x0030
#define SGTL5000_PAD_I2S_DOUT_SHIFT		4
#define SGTL5000_PAD_I2S_DOUT_WIDTH		2
#define SGTL5000_PAD_I2C_SDA_MASK		0x000c
#define SGTL5000_PAD_I2C_SDA_SHIFT		2
#define SGTL5000_PAD_I2C_SDA_WIDTH		2
#define SGTL5000_PAD_I2C_SCL_MASK		0x0003
#define SGTL5000_PAD_I2C_SCL_SHIFT		0
#define SGTL5000_PAD_I2C_SCL_WIDTH		2

/*
 * SGTL5000_CHIP_ANA_ADC_CTRL
 */
#define SGTL5000_ADC_VOL_M6DB			0x0100
#define SGTL5000_ADC_VOL_RIGHT_MASK		0x00f0
#define SGTL5000_ADC_VOL_RIGHT_SHIFT		4
#define SGTL5000_ADC_VOL_RIGHT_WIDTH		4
#define SGTL5000_ADC_VOL_LEFT_MASK		0x000f
#define SGTL5000_ADC_VOL_LEFT_SHIFT		0
#define SGTL5000_ADC_VOL_LEFT_WIDTH		4

/*
 * SGTL5000_CHIP_ANA_HP_CTRL
 */
#define SGTL5000_HP_VOL_RIGHT_MASK		0x7f00
#define SGTL5000_HP_VOL_RIGHT_SHIFT		8
#define SGTL5000_HP_VOL_RIGHT_WIDTH		7
#define SGTL5000_HP_VOL_LEFT_MASK		0x007f
#define SGTL5000_HP_VOL_LEFT_SHIFT		0
#define SGTL5000_HP_VOL_LEFT_WIDTH		7

/*
 * SGTL5000_CHIP_ANA_CTRL
 */
#define SGTL5000_LINE_OUT_MUTE			0x0100
#define SGTL5000_HP_SEL_MASK			0x0040
#define SGTL5000_HP_SEL_SHIFT			6
#define SGTL5000_HP_SEL_WIDTH			1
#define SGTL5000_HP_SEL_DAC			0x0
#define SGTL5000_HP_SEL_LINE_IN			0x1
#define SGTL5000_HP_ZCD_EN			0x0020
#define SGTL5000_HP_MUTE			0x0010
#define SGTL5000_ADC_SEL_MASK			0x0004
#define SGTL5000_ADC_SEL_SHIFT			2
#define SGTL5000_ADC_SEL_WIDTH			1
#define SGTL5000_ADC_SEL_MIC			0x0
#define SGTL5000_ADC_SEL_LINE_IN		0x1
#define SGTL5000_ADC_ZCD_EN			0x0002
#define SGTL5000_ADC_MUTE			0x0001

/*
 * SGTL5000_CHIP_LINREG_CTRL
 */
#define SGTL5000_VDDC_MAN_ASSN_MASK		0x0040
#define SGTL5000_VDDC_MAN_ASSN_SHIFT		6
#define SGTL5000_VDDC_MAN_ASSN_WIDTH		1
#define SGTL5000_VDDC_MAN_ASSN_VDDA		0x0
#define SGTL5000_VDDC_MAN_ASSN_VDDIO		0x1
#define SGTL5000_VDDC_ASSN_OVRD			0x0020
#define SGTL5000_LINREG_VDDD_MASK		0x000f
#define SGTL5000_LINREG_VDDD_SHIFT		0
#define SGTL5000_LINREG_VDDD_WIDTH		4

/*
 * SGTL5000_CHIP_REF_CTRL
 */
#define SGTL5000_ANA_GND_MASK			0x01f0
#define SGTL5000_ANA_GND_SHIFT			4
#define SGTL5000_ANA_GND_WIDTH			5
#define SGTL5000_ANA_GND_BASE			800	/* mv */
#define SGTL5000_ANA_GND_STP			25	/*mv */
#define SGTL5000_BIAS_CTRL_MASK			0x000e
#define SGTL5000_BIAS_CTRL_SHIFT		1
#define SGTL5000_BIAS_CTRL_WIDTH		3
#define SGTL5000_SMALL_POP			1

/*
 * SGTL5000_CHIP_MIC_CTRL
 */
#define SGTL5000_BIAS_R_MASK			0x0300
#define SGTL5000_BIAS_R_SHIFT			8
#define SGTL5000_BIAS_R_WIDTH			2
#define SGTL5000_BIAS_R_off			0x0
#define SGTL5000_BIAS_R_2K			0x1
#define SGTL5000_BIAS_R_4k			0x2
#define SGTL5000_BIAS_R_8k			0x3
#define SGTL5000_BIAS_VOLT_MASK			0x0070
#define SGTL5000_BIAS_VOLT_SHIFT		4
#define SGTL5000_BIAS_VOLT_WIDTH		3
#define SGTL5000_MIC_GAIN_MASK			0x0003
#define SGTL5000_MIC_GAIN_SHIFT			0
#define SGTL5000_MIC_GAIN_WIDTH			2

/*
 * SGTL5000_CHIP_LINE_OUT_CTRL
 */
#define SGTL5000_LINE_OUT_CURRENT_MASK		0x0f00
#define SGTL5000_LINE_OUT_CURRENT_SHIFT		8
#define SGTL5000_LINE_OUT_CURRENT_WIDTH		4
#define SGTL5000_LINE_OUT_CURRENT_180u		0x0
#define SGTL5000_LINE_OUT_CURRENT_270u		0x1
#define SGTL5000_LINE_OUT_CURRENT_360u		0x3
#define SGTL5000_LINE_OUT_CURRENT_450u		0x7
#define SGTL5000_LINE_OUT_CURRENT_540u		0xf
#define SGTL5000_LINE_OUT_GND_MASK		0x003f
#define SGTL5000_LINE_OUT_GND_SHIFT		0
#define SGTL5000_LINE_OUT_GND_WIDTH		6
#define SGTL5000_LINE_OUT_GND_BASE		800	/* mv */
#define SGTL5000_LINE_OUT_GND_STP		25
#define SGTL5000_LINE_OUT_GND_MAX		0x23

/*
 * SGTL5000_CHIP_LINE_OUT_VOL
 */
#define SGTL5000_LINE_OUT_VOL_RIGHT_MASK	0x1f00
#define SGTL5000_LINE_OUT_VOL_RIGHT_SHIFT	8
#define SGTL5000_LINE_OUT_VOL_RIGHT_WIDTH	5
#define SGTL5000_LINE_OUT_VOL_LEFT_MASK		0x001f
#define SGTL5000_LINE_OUT_VOL_LEFT_SHIFT	0
#define SGTL5000_LINE_OUT_VOL_LEFT_WIDTH	5

/*
 * SGTL5000_CHIP_ANA_POWER
 */
#define SGTL5000_ANA_POWER_DEFAULT		0x7060
#define SGTL5000_DAC_STEREO			0x4000
#define SGTL5000_LINREG_SIMPLE_POWERUP		0x2000
#define SGTL5000_STARTUP_POWERUP		0x1000
#define SGTL5000_VDDC_CHRGPMP_POWERUP		0x0800
#define SGTL5000_PLL_POWERUP			0x0400
#define SGTL5000_LINEREG_D_POWERUP		0x0200
#define SGTL5000_VCOAMP_POWERUP			0x0100
#define SGTL5000_VAG_POWERUP			0x0080
#define SGTL5000_ADC_STEREO			0x0040
#define SGTL5000_REFTOP_POWERUP			0x0020
#define SGTL5000_HP_POWERUP			0x0010
#define SGTL5000_DAC_POWERUP			0x0008
#define SGTL5000_CAPLESS_HP_POWERUP		0x0004
#define SGTL5000_ADC_POWERUP			0x0002
#define SGTL5000_LINE_OUT_POWERUP		0x0001

/*
 * SGTL5000_CHIP_PLL_CTRL
 */
#define SGTL5000_PLL_INT_DIV_MASK		0xf800
#define SGTL5000_PLL_INT_DIV_SHIFT		11
#define SGTL5000_PLL_INT_DIV_WIDTH		5
#define SGTL5000_PLL_FRAC_DIV_MASK		0x07ff
#define SGTL5000_PLL_FRAC_DIV_SHIFT		0
#define SGTL5000_PLL_FRAC_DIV_WIDTH		11

/*
 * SGTL5000_CHIP_CLK_TOP_CTRL
 */
#define SGTL5000_INT_OSC_EN			0x0800
#define SGTL5000_INPUT_FREQ_DIV2		0x0008

/*
 * SGTL5000_CHIP_ANA_STATUS
 */
#define SGTL5000_HP_LRSHORT			0x0200
#define SGTL5000_CAPLESS_SHORT			0x0100
#define SGTL5000_PLL_LOCKED			0x0010

/*
 * SGTL5000_CHIP_SHORT_CTRL
 */
#define SGTL5000_LVLADJR_MASK			0x7000
#define SGTL5000_LVLADJR_SHIFT			12
#define SGTL5000_LVLADJR_WIDTH			3
#define SGTL5000_LVLADJL_MASK			0x0700
#define SGTL5000_LVLADJL_SHIFT			8
#define SGTL5000_LVLADJL_WIDTH			3
#define SGTL5000_LVLADJC_MASK			0x0070
#define SGTL5000_LVLADJC_SHIFT			4
#define SGTL5000_LVLADJC_WIDTH			3
#define SGTL5000_LR_SHORT_MOD_MASK		0x000c
#define SGTL5000_LR_SHORT_MOD_SHIFT		2
#define SGTL5000_LR_SHORT_MOD_WIDTH		2
#define SGTL5000_CM_SHORT_MOD_MASK		0x0003
#define SGTL5000_CM_SHORT_MOD_SHIFT		0
#define SGTL5000_CM_SHORT_MOD_WIDTH		2

/*
 *SGTL5000_CHIP_ANA_TEST2
 */
#define SGTL5000_MONO_DAC			0x1000

/*
 * SGTL5000_DAP_CTRL
 */
#define SGTL5000_DAP_MIX_EN			0x0010
#define SGTL5000_DAP_EN				0x0001

#define SGTL5000_SYSCLK				0x00
#define SGTL5000_LRCLK	0x01

/****************************************************************/
/*      PRIVATE FUNCTIONS
/****************************************************************/
/*
 * Read a register
 */
int32_t sgtl5000_read_reg(uint16_t reg, uint16_t* value)
{
	uint8_t reg_in_bytes[2] = { (reg >> 8) & 0xFF, reg & 0xFF };
	uint8_t read_bytes[2];
	int32_t ret_val = 0;

	ret_val = i2c_write_buffer(SGTL5000_I2C_ADDRESS, reg_in_bytes, sizeof(reg_in_bytes));
	if (ret_val == 0) {
		ret_val = i2c_read_buffer(SGTL5000_I2C_ADDRESS, read_bytes, sizeof(read_bytes));
		if (ret_val == 0) {
			*value = (((uint16_t)read_bytes[0]) << 8) | ((uint16_t)read_bytes[1]);
			return 0;
		}
	}
	debug_msg("Error in: %s\n", __func__);
	return ret_val;
}

/*
 * Write a register
 */
int32_t sgtl5000_write_reg(uint16_t reg, uint16_t value)
{
	uint8_t write_bytes[4] = { 	(reg >> 8) & 0xFF, reg & 0xFF,
								(value >> 8) & 0xFF, value & 0xFF };
	int32_t ret_val = 0;

	ret_val = i2c_write_buffer(SGTL5000_I2C_ADDRESS, write_bytes, sizeof(write_bytes));
	if (ret_val == 0) {
		return 0;
	}
	debug_msg("Error in: %s\n", __func__);
	return ret_val;
}

/*
 * Modify a single register
 */
int32_t sgtl5000_modify_reg(uint16_t reg, uint16_t mask, uint16_t new_value)
{
	uint16_t reg_data;
	int32_t ret_val;

	ret_val = sgtl5000_read_reg(reg, &reg_data);
	if (ret_val == 0) {
		reg_data = (reg_data & ~mask) | (new_value & mask);
		ret_val = sgtl5000_write_reg(reg, reg_data);
		if (ret_val == 0) {
			return 0;
		}
	}
	debug_msg("Error in: %s\n", __func__);
	return ret_val;
}

/*
 * 	Power up sequence (taken from AN3663)
 */
int32_t sgtl5000_power_up()
{
	int32_t ret_val = 0;
	
	// Turn off startup power supplies to save power
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_POWER, SGTL5000_STARTUP_POWERUP | SGTL5000_LINREG_SIMPLE_POWERUP, 0x0);
	if (ret_val != 0)
		goto Exit;
	// Configure the charge pump to use the VDDIO rail (set bit 5 and bit 6) 
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_LINREG_CTRL, SGTL5000_VDDC_MAN_ASSN_MASK | SGTL5000_VDDC_ASSN_OVRD,
									SGTL5000_VDDC_MAN_ASSN_MASK | SGTL5000_VDDC_ASSN_OVRD);
	if (ret_val != 0)
		goto Exit;
	//------ Reference Voltage and Bias Current Configuration----------
	// NOTE: The value written in the next 2 Write calls is dependent on the VDDA voltage value.
	// Set ground, ADC, DAC reference voltage (bits 8:4). The value should be set to VDDA/2. This example assumes VDDA = 1.8V. VDDA/2 = 0.9V.
	// The bias current should be set to 50% of the nominal value (bits 3:1)
	ret_val = sgtl5000_write_reg(SGTL5000_CHIP_REF_CTRL, (0x1F << SGTL5000_ANA_GND_SHIFT) | (0x7 << SGTL5000_BIAS_CTRL_SHIFT));
	if (ret_val != 0)
		goto Exit;
	// Set LINEOUT reference voltage to VDDIO/2 (1.65V) (bits 5:0) and bias current (bits 11:8) to the recommended value of 0.36mA for 10kOhm load with 1nF capacitance 
	//ret_val = sgtl5000_write_reg(SGTL5000_CHIP_LINE_OUT_CTRL, (SGTL5000_LINE_OUT_CURRENT_360u << SGTL5000_LINE_OUT_CURRENT_SHIFT) |
	//														  (((3300/2)-SGTL5000_LINE_OUT_GND_BASE)/SGTL5000_LINE_OUT_GND_STP));
	//if (ret_val != 0)
	//	goto Exit;
	//----------------Other Analog Block Configurations------------------
	// Configure slow ramp up rate to minimize pop (bit 0) 
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_REF_CTRL, SGTL5000_SMALL_POP, SGTL5000_SMALL_POP);
	if (ret_val != 0)
		goto Exit;
	// Enable short detect mode for headphone left/right and center channel and set short detect current trip level to 75mA
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_SHORT_CTRL, SGTL5000_LVLADJR_MASK | SGTL5000_LVLADJL_MASK | SGTL5000_LVLADJC_MASK | 
									SGTL5000_LR_SHORT_MOD_MASK | SGTL5000_CM_SHORT_MOD_MASK, 
									(1<<SGTL5000_LVLADJR_SHIFT) | (1<<SGTL5000_LVLADJL_SHIFT) | (0<<SGTL5000_LVLADJC_SHIFT) | 
									(1<<SGTL5000_LR_SHORT_MOD_SHIFT) | (2<<SGTL5000_CM_SHORT_MOD_SHIFT) );
	if (ret_val != 0)
		goto Exit;
	// Enable Zero-cross detect if needed for HP_OUT (bit 5) and ADC (bit 1) 
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_CTRL, SGTL5000_HP_ZCD_EN | SGTL5000_ADC_ZCD_EN, SGTL5000_HP_ZCD_EN | SGTL5000_ADC_ZCD_EN);
	if (ret_val != 0)
		goto Exit;
	//----------------Power up Inputs/Outputs/Digital Blocks-------------
	// Power up LINEOUT, HP, ADC, DAC 
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_POWER,  SGTL5000_LINE_OUT_POWERUP | SGTL5000_ADC_POWERUP | SGTL5000_CAPLESS_HP_POWERUP | SGTL5000_DAC_POWERUP |
															SGTL5000_HP_POWERUP | SGTL5000_REFTOP_POWERUP | SGTL5000_ADC_STEREO | SGTL5000_VAG_POWERUP | 
															SGTL5000_LINEREG_D_POWERUP | SGTL5000_VDDC_CHRGPMP_POWERUP | SGTL5000_DAC_STEREO,
															SGTL5000_LINE_OUT_POWERUP | SGTL5000_ADC_POWERUP | SGTL5000_CAPLESS_HP_POWERUP | SGTL5000_DAC_POWERUP |
															SGTL5000_HP_POWERUP | SGTL5000_REFTOP_POWERUP | SGTL5000_ADC_STEREO | SGTL5000_VAG_POWERUP | 
															SGTL5000_LINEREG_D_POWERUP | SGTL5000_VDDC_CHRGPMP_POWERUP | SGTL5000_DAC_STEREO);
	if (ret_val != 0)
		goto Exit;
	// Power up desired digital blocks
	// I2S_IN (bit 0), I2S_OUT (bit 1), DAP (bit 4), DAC (bit 5), ADC (bit 6) are powered on
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_DIG_POWER, SGTL5000_ADC_EN | SGTL5000_DAC_EN | SGTL5000_DAP_POWERUP | SGTL5000_I2S_OUT_POWERUP | SGTL5000_I2S_IN_POWERUP,
															SGTL5000_ADC_EN | SGTL5000_DAC_EN | SGTL5000_DAP_POWERUP | SGTL5000_I2S_OUT_POWERUP | SGTL5000_I2S_IN_POWERUP);
	if (ret_val != 0)
		goto Exit;
	//--------------------Set LINEOUT Volume Level-----------------------
	// Set the LINEOUT volume level based on voltage reference (VAG) values using this formula
	// Value = (int)(40*log(VAG_VAL/LO_VAGCNTRL) + 15)
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_LINE_OUT_VOL, SGTL5000_LINE_OUT_VOL_RIGHT_MASK | SGTL5000_LINE_OUT_VOL_LEFT_MASK,
									(0xF << SGTL5000_LINE_OUT_VOL_RIGHT_SHIFT) | (0xF << SGTL5000_LINE_OUT_VOL_LEFT_SHIFT));
	
Exit:
	if (ret_val != 0) 
		debug_msg("Error in: %s\n", __func__);
	return ret_val;
}

/*
 * Poweroff all the internal supplies
 */
int32_t sgtl5000_power_down()
{
	int32_t ret_val = 0;

	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_DIG_POWER, 0xFFFF, 0x0000);
	if (ret_val != 0)
			goto Exit;

	ret_val = sgtl5000_write_reg(SGTL5000_CHIP_ANA_POWER, 0x7060);
	if (ret_val != 0)
			goto Exit;

	Exit:
		if (ret_val != 0)
			debug_msg("Error in: %s\n", __func__);
		return ret_val;
}

/*
 * Configure the internal I2S properties:
 * - data length = 16 bits
 * Other default values are fine, therefore they are not changed
 * - slave
 * - SCLK freq = 64*Fs
 * - I2S mode = I2S format
 */
int32_t sgtl5000_configure_i2s()
{
	int32_t ret_val = 0;
	
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_I2S_CTRL, SGTL5000_I2S_DLEN_MASK, SGTL5000_I2S_DLEN_16 << SGTL5000_I2S_DLEN_SHIFT);
	if (ret_val != 0)  
		debug_msg("Error in: %s\n", __func__);

	return ret_val;
}

/*
 * Set the ADC gain (in 0.1*dB):
 * - steps are 1.5dB
 * - range is between [0dB, 22.5dB]
 * Note: the gain is supposed to be symmetric between right and left channels
 */
int32_t sgtl5000_set_ADC_gain(int16_t gain_db)
{
	// Input parameter check
	if (gain_db > SGTL5000_MAX_ADC_GAIN) {
		gain_db = SGTL5000_MAX_ADC_GAIN;
		debug_msg("ADC gain limited to %d\n", gain_db);
	} else if (gain_db < SGTL5000_MIN_ADC_GAIN) {
		gain_db = SGTL5000_MIN_ADC_GAIN;
		debug_msg("ADC gain limited to %d\n", gain_db);
	}
	// Convert the input value to a valid register value
	uint16_t gain_db_reg_val = ((uint16_t)gain_db)/SGTL5000_ADC_GAIN_STEP;
	// Write the new value to the register
	int32_t ret_val = 0;
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_ADC_CTRL, SGTL5000_ADC_VOL_RIGHT_MASK | SGTL5000_ADC_VOL_LEFT_MASK,
									(gain_db_reg_val << SGTL5000_ADC_VOL_RIGHT_SHIFT) | (gain_db_reg_val << SGTL5000_ADC_VOL_LEFT_SHIFT));
	if (ret_val != 0) goto Exit;
	// Unmute DAC
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ADCDAC_CTRL, SGTL5000_DAC_MUTE_RIGHT | SGTL5000_DAC_MUTE_LEFT, 0x00);
	if (ret_val != 0) goto Exit;

Exit:
	if (ret_val != 0)  
		debug_msg("Error in: %s\n", __func__);
	return ret_val;	
}

/****************************************************************/
/*      PUBLIC FUNCTIONS
/****************************************************************/
/*
 * Configure MCLK to 256*Fs and the user selected sample rate
 */
int32_t sgtl5000_config_clocks(uint32_t sample_rate)
{	
	switch (sample_rate) {
		case 32000:
			return sgtl5000_modify_reg(SGTL5000_CHIP_CLK_CTRL, SGTL5000_SYS_FS_MASK | SGTL5000_MCLK_FREQ_MASK,
										(SGTL5000_SYS_FS_32k << SGTL5000_SYS_FS_SHIFT) | SGTL5000_MCLK_FREQ_256FS);
		case 44100:
			return sgtl5000_modify_reg(SGTL5000_CHIP_CLK_CTRL, SGTL5000_SYS_FS_MASK | SGTL5000_MCLK_FREQ_MASK,
										(SGTL5000_SYS_FS_44_1k << SGTL5000_SYS_FS_SHIFT) | SGTL5000_MCLK_FREQ_256FS);
		case 48000:
			return sgtl5000_modify_reg(SGTL5000_CHIP_CLK_CTRL, SGTL5000_SYS_FS_MASK | SGTL5000_MCLK_FREQ_MASK,
										(SGTL5000_SYS_FS_48k << SGTL5000_SYS_FS_SHIFT) | SGTL5000_MCLK_FREQ_256FS);
		case 96000:
			return sgtl5000_modify_reg(SGTL5000_CHIP_CLK_CTRL, SGTL5000_SYS_FS_MASK | SGTL5000_MCLK_FREQ_MASK,
										(SGTL5000_SYS_FS_96k << SGTL5000_SYS_FS_SHIFT) | SGTL5000_MCLK_FREQ_256FS);
		default:
			debug_msg("Wrong sample rate %d\n", sample_rate);
			return -1;
	}
}

/*
 * Select the internal routing of the SGTL5000. 
 * Note: for now the routing is fixed
 * 		I2S_IN --> DAC --> HP_OUT
 * This should be changed in the future in case DAP (digital audio processor) becomes necessary
 */
int32_t sgtl5000_set_audio_routing(uint8_t use_dap)
{
	int32_t ret_val = 0;
	
	if (use_dap == 0) {	// no DAP
		// set DAC input to I2S_IN
		ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_SSS_CTRL, SGTL5000_DAC_SEL_MASK, SGTL5000_DAC_SEL_I2S_IN << SGTL5000_DAC_SEL_SHIFT);
		if (ret_val != 0) goto Exit;
		// set HP_OUT input to DAC
		ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_CTRL, SGTL5000_HP_SEL_MASK, SGTL5000_HP_SEL_DAC << SGTL5000_HP_SEL_SHIFT);
		if (ret_val != 0) goto Exit;
	} else {
		// set DAP input to I2S_IN
		ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_SSS_CTRL, SGTL5000_DAP_SEL_MASK, SGTL5000_DAP_SEL_I2S_IN << SGTL5000_DAP_SEL_SHIFT);
		if (ret_val != 0) goto Exit;
		// set DAC input to DAP
		ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_SSS_CTRL, SGTL5000_DAC_SEL_MASK, SGTL5000_DAC_SEL_DAP << SGTL5000_DAC_SEL_SHIFT);
		if (ret_val != 0) goto Exit;
		// set HP_OUT input to DAC
		ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_CTRL, SGTL5000_HP_SEL_MASK, SGTL5000_HP_SEL_DAC << SGTL5000_HP_SEL_SHIFT);
		if (ret_val != 0) goto Exit;
	}
	
Exit:
	if (ret_val != 0)  
		debug_msg("Error in: %s\n", __func__);
	return ret_val;
}

/*
 * Set the headphone output volume (in 0.1*dB)
 * - each step is 0.5dB
 * - allowed range is [-51.5dB, +12dB]
 * Out of range input values will be automatically limted to the allowed ones
 */
int32_t sgtl5000_set_hp_out_volume(int16_t value)
{
	int32_t ret_val = 0;
	
	if (value > SGTL5000_MAX_HP_OUT_VOLUME) {
		value = SGTL5000_MAX_HP_OUT_VOLUME;
		debug_msg("ADC gain limited to %d\n", value);
	} else if (value < SGTL5000_MIN_HP_OUT_VOLUME) {
		value = SGTL5000_MIN_HP_OUT_VOLUME;
		debug_msg("ADC gain limited to %d\n", value);
	} 
	// Convert the input value to a valid register value
	uint16_t vol_reg_val = ((uint16_t)(SGTL5000_MAX_HP_OUT_VOLUME-value))/SGTL5000_HP_OUT_VOLUME_STEP;
	// Write the new value to the register
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_ADC_CTRL, SGTL5000_ADC_VOL_RIGHT_MASK | SGTL5000_ADC_VOL_LEFT_MASK,
									(vol_reg_val << SGTL5000_ADC_VOL_RIGHT_SHIFT) | (vol_reg_val << SGTL5000_ADC_VOL_LEFT_SHIFT));
	if (ret_val != 0) goto Exit;
	// Unmute headphones
	ret_val = sgtl5000_modify_reg(SGTL5000_CHIP_ANA_CTRL, SGTL5000_HP_MUTE, 0x00);
	if (ret_val != 0) goto Exit;

Exit:
	if (ret_val != 0)  
		debug_msg("Error in: %s\n", __func__);
	return ret_val;	
}

/*
 * Retrun the current hp_out volume (in 0.1*dB)
 */
int32_t sgtl5000_get_hp_out_volume(int16_t* value)
{
	uint16_t register_value;
	int32_t ret_val = sgtl5000_read_reg(SGTL5000_CHIP_ANA_ADC_CTRL, &register_value);
	if (ret_val != 0)  {
		debug_msg("Error in: %s\n", __func__);
		return ret_val;	
	}
	
	*value = SGTL5000_MAX_HP_OUT_VOLUME - ((int16_t)(register_value*SGTL5000_HP_OUT_VOLUME_STEP));
	
	return ret_val;	
}

/*
 * Initialize the codec
 */
int32_t sgtl5000_init()
{
	int32_t ret_val = 0;
	
	// Power cycle the device the device
	ret_val = sgtl5000_power_down();
	if (ret_val != 0)
		return ret_val;
	ret_val = sgtl5000_power_up();
	if (ret_val != 0)
		return ret_val;
	// wait some time in order to complete powerup (this is not documented on the datasheet,
	// but it seems to be necessary to have following I2C communications to work)
	systick_wait_for_ms(5);
	// Configure the sample frequency
	ret_val = output_i2s_ConfigurePLL(48000);
	if (ret_val != 0)
		return ret_val;
	ret_val = sgtl5000_config_clocks(48000);
	if (ret_val != 0)
		return ret_val;
	// Configure the internal audio routing (no DAP)
	ret_val = sgtl5000_set_audio_routing(FALSE);
	if (ret_val != 0)
		return ret_val;
	// Configure the I2S data format
	ret_val = sgtl5000_configure_i2s();
	if (ret_val != 0)
		return ret_val;	
	// Set the ADC gain to 0db
	ret_val = sgtl5000_set_ADC_gain(SGTL5000_MIN_ADC_GAIN);
	if (ret_val != 0)
		return ret_val;	
	// Set the HP_OUT volume to 0dB at boot
	ret_val = sgtl5000_set_hp_out_volume(0);
	if (ret_val != 0)
		return ret_val;
		
	return ret_val;
}

/*
 * Dump all the registers for debug
 */
#define dump_single_reg(reg)	\
	do { \
		if (sgtl5000_read_reg(reg, &val) == 0)	{ \
			debug_msg(#reg " = 0x%x\n", val);	\
		} else {	\
			debug_msg("Error in function %s\n", __func__);	\
			return ; \
		}	\
	} while(0);

void sgtl5000_dump_registers()
{
	uint16_t val;

	dump_single_reg(SGTL5000_CHIP_ID);
	dump_single_reg(SGTL5000_CHIP_DIG_POWER);
	dump_single_reg(SGTL5000_CHIP_CLK_CTRL);
	dump_single_reg(SGTL5000_CHIP_I2S_CTRL);
	dump_single_reg(SGTL5000_CHIP_SSS_CTRL);
	dump_single_reg(SGTL5000_CHIP_ADCDAC_CTRL);
	dump_single_reg(SGTL5000_CHIP_DAC_VOL);
	dump_single_reg(SGTL5000_CHIP_PAD_STRENGTH);
	dump_single_reg(SGTL5000_CHIP_ANA_ADC_CTRL);
	dump_single_reg(SGTL5000_CHIP_ANA_HP_CTRL);
	dump_single_reg(SGTL5000_CHIP_ANA_CTRL);
	dump_single_reg(SGTL5000_CHIP_LINREG_CTRL);
	dump_single_reg(SGTL5000_CHIP_REF_CTRL);
	dump_single_reg(SGTL5000_CHIP_MIC_CTRL);
	dump_single_reg(SGTL5000_CHIP_LINE_OUT_CTRL);
	dump_single_reg(SGTL5000_CHIP_LINE_OUT_VOL);
	dump_single_reg(SGTL5000_CHIP_ANA_POWER);
	dump_single_reg(SGTL5000_CHIP_PLL_CTRL);
	dump_single_reg(SGTL5000_CHIP_CLK_TOP_CTRL);
	dump_single_reg(SGTL5000_CHIP_ANA_STATUS);
	dump_single_reg(SGTL5000_CHIP_SHORT_CTRL);
	dump_single_reg(SGTL5000_CHIP_ANA_TEST2);
}

#undef dump_single_reg
