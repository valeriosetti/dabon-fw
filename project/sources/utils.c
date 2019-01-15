#include "utils.h"
#include "stm32f407xx.h"
#include "core_cm4.h"

/*
 * Perform a software reset of the STM32
 */
int reset(int argc, char *argv[])
{
	NVIC_SystemReset();
}

/*
 * Return whether or not the specified FW is embedded in the image
 */
int8_t utils_is_fw_embedded(Tuner_FW_type type)
{
	#if defined(DAB_RADIO) 
		if ((type == SI468X_BOOT_FW) || (type == SI468X_DAB_FW))
			return TRUE;
		else
			return FALSE;
	#elif defined(FM_RADIO)
		if ((type == SI468X_BOOT_FW) || (type == SI468X_FM_FW))
			return TRUE;
		else
			return FALSE;
	#elif defined(NO_EXT_FIRMWARES)
		return FALSE;
	#endif
}

/*
 * Returns the starting address for the specified FW
 */
uint8_t* utils_get_embedded_FW_start_address(Tuner_FW_type type)
{
	if (type == SI468X_DAB_FW) {
		#if defined(DAB_RADIO) 
			extern uint8_t _binary___external_firmwares_dab_radio_5_0_5_bin_start;
			return &_binary___external_firmwares_dab_radio_5_0_5_bin_start;
		#else
			return 0;
		#endif
	} else if (type == SI468X_FM_FW) {
		#if defined(FM_RADIO) 
			extern uint8_t _binary___external_firmwares_fmhd_radio_5_0_4_bin_start;
			return &_binary___external_firmwares_fmhd_radio_5_0_4_bin_start;
		#else
			return 0;
		#endif
	} else if (type == SI468X_BOOT_FW) {
		#if defined(DAB_RADIO) || defined(FM_RADIO)
			extern uint8_t _binary___external_firmwares_rom00_patch_016_bin_start;
			return &_binary___external_firmwares_rom00_patch_016_bin_start;
		#else
			return 0;
		#endif
	} else 
		return 0;
}

/*
 * 
 */
uint32_t utils_get_embedded_FW_size(Tuner_FW_type type)
{
	if (type == SI468X_DAB_FW) {
		#if defined(DAB_RADIO)
			extern uint8_t _binary___external_firmwares_dab_radio_5_0_5_bin_end;
			extern uint8_t _binary___external_firmwares_dab_radio_5_0_5_bin_start;
			return (&_binary___external_firmwares_dab_radio_5_0_5_bin_end - &_binary___external_firmwares_dab_radio_5_0_5_bin_start);
		#else
			return 0;
		#endif
	} else if (type == SI468X_FM_FW) {
		#if defined(FM_RADIO) 
			extern uint8_t _binary___external_firmwares_fmhd_radio_5_0_4_bin_end;
			extern uint8_t _binary___external_firmwares_fmhd_radio_5_0_4_bin_start;
			return (&_binary___external_firmwares_fmhd_radio_5_0_4_bin_end - &_binary___external_firmwares_fmhd_radio_5_0_4_bin_start);
		#else
			return 0;
		#endif
	} else if (type == SI468X_BOOT_FW) {
		#if defined(DAB_RADIO) || defined(FM_RADIO)
			extern uint8_t _binary___external_firmwares_rom00_patch_016_bin_end;
			extern uint8_t _binary___external_firmwares_rom00_patch_016_bin_start;
			return (&_binary___external_firmwares_rom00_patch_016_bin_end - &_binary___external_firmwares_rom00_patch_016_bin_start);
		#else
			return 0;
		#endif
	} else 
		return 0;
} 
