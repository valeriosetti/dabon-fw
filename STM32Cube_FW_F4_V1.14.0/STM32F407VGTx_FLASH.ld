/* Entry Point */
ENTRY(Reset_Handler)

/* Physical definitions */
FLASH_START_ADDRESS = 0x8000000;
FLASH_SIZE_KB = 1024K;
RAM_START_ADDRESS = 0x20000000;
RAM_SIZE_KB = 128K;
CCRAM_START_ADDRESS = 0x10000000;
CCRAM_SIZE_KB = 64K;

/* RAM splitting between stack and other data  */
STACK_START_ADDRESS = RAM_START_ADDRESS;
STACK_SIZE_KB = 10K;
GENERAL_PURPOSE_RAM_START_ADDRESS = STACK_START_ADDRESS + STACK_SIZE_KB + 1;
GENERAL_PURPOSE_RAM_SIZE = RAM_SIZE_KB - STACK_SIZE_KB;

/* Specify the memory areas */
MEMORY {
	FLASH (rx)                  : ORIGIN = FLASH_START_ADDRESS, LENGTH = FLASH_SIZE_KB
    STACK (xrw)                 : ORIGIN = STACK_START_ADDRESS, LENGTH = STACK_SIZE_KB
	GENERAL_PURPOSE_RAM (xrw)   : ORIGIN = GENERAL_PURPOSE_RAM_START_ADDRESS, LENGTH = GENERAL_PURPOSE_RAM_SIZE
	CCMRAM (rw)                 : ORIGIN = CCRAM_START_ADDRESS, LENGTH = CCRAM_SIZE_KB
}

/* Define output sections */
SECTIONS {
    /* The startup code goes first into FLASH */
    .isr_vector FLASH_START_ADDRESS : {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */
        . = ALIGN(4);
    } >FLASH

    /* The program code and other data goes into FLASH */
    .text : {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)
        . = ALIGN(4);
        _etext = .;        /* define a global symbols at end of code */
    } >FLASH

    /* Constant data goes into FLASH */
    .rodata : {
        . = ALIGN(4);
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */	
        . = ALIGN(4);
    } >FLASH

    /* Initialized data sections goes into RAM, load LMA copy after code */
    _sidata = LOADADDR(.data);
    .data : {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start */
        *(.data)           
        *(.data*)          
        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end */
    } >GENERAL_PURPOSE_RAM AT> FLASH
  
    /* Uninitialized data section */
    . = ALIGN(4);
    .bss : {
        _sbss = .;         /* define a global symbol at bss start */
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end */
    } >GENERAL_PURPOSE_RAM
    
    /* CCM-RAM section */
    _siccmram = LOADADDR(.ccmram);
    .ccmram : {
        . = ALIGN(4);
        _sccmram = .;       /* create a global symbol at ccmram start */
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;       /* create a global symbol at ccmram end */
    } >CCMRAM AT> FLASH

    _estack = ORIGIN(STACK) + LENGTH(STACK);
  
    /* Remove information from the standard libraries */
    /DISCARD/ : {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }
}


