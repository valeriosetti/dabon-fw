FWS =
ifeq ($(TUNER_CONFIG),DAB_RADIO)
	FWS += ./external_firmwares/dab_radio_5_0_5.bin
else
	FWS += ./external_firmwares/fmhd_radio_5_0_4.bin
endif
FWS += ./external_firmwares/rom00_patch.016.bin
#FWS += ./external_firmwares/rom00_patch_mini.bin
