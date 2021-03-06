#include "includes.h"

HOST   infoHost;   // Information interaction with Marlin
MENU   infoMenu;   // Menu structure
CLOCKS mcuClocks;  // system clocks: SYSCLK, AHB, APB1, APB2, APB1_Timer, APB2_Timer2

void mcu_GetClocksFreq(CLOCKS *clk)
{
  RCC_GetClocksFreq(&clk->rccClocks);

  if (clk->rccClocks.PCLK1_Frequency < clk->rccClocks.HCLK_Frequency)  // if (APBx presc = 1) x1 else x2
    clk->PCLK1_Timer_Frequency = clk->rccClocks.PCLK1_Frequency * 2;
  else
    clk->PCLK1_Timer_Frequency = clk->rccClocks.PCLK1_Frequency;

  if (clk->rccClocks.PCLK2_Frequency < clk->rccClocks.HCLK_Frequency)
    clk->PCLK2_Timer_Frequency = clk->rccClocks.PCLK2_Frequency * 2;
  else
    clk->PCLK2_Timer_Frequency = clk->rccClocks.PCLK2_Frequency;
}

void Hardware_GenericInit(void)
{
  mcu_GetClocksFreq(&mcuClocks);
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
  Delay_init();

  #ifdef DISABLE_JTAG
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  // disable JTAG, enable SWD
  #endif

  #ifdef DISABLE_DEBUG
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);  //disable JTAG & SWD
  #endif

  #if defined(MKS_TFT)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);
  #endif

  XPT2046_Init();
  OS_TimerInitMs();  // System clock timer, cycle 1ms, called after XPT2046_Init()
  W25Qxx_Init();
  LCD_Init();
  readStoredPara();  // Read settings parameter

  #if defined(SERIAL_DEBUG_PORT) && defined(SERIAL_DEBUG_ENABLED)
    Serial_ReSourceInit();  // Initialize serial ports first if debugging is enabled
  #endif

  LCD_RefreshDirection(infoSettings.rotate_ui);  // refresh display direction after reading settings
  scanUpdates();                                 // scan icon, fonts and config files
  checkflashSign();                              // check font/icon/config signature in SPI flash for update
  initMachineSetting();                          // load default machine settings

  #ifdef LED_COLOR_PIN
    knob_LED_Init();
    Knob_LED_SetColor(led_colors[infoSettings.knob_led_color], infoSettings.neopixel_pixels);  // set last saved color after initialization
  #endif

  #ifdef BUZZER_PIN
    Buzzer_Config();
  #endif

  #if !defined(MKS_TFT)
    //causes hang if we deinit spi1
    SD_DeInit();
  #endif

  #if LCD_ENCODER_SUPPORT
    HW_EncoderInit();
  #endif

  #if ENC_ACTIVE_SIGNAL
    HW_EncActiveSignalInit();
  #endif

  #ifdef PS_ON_PIN
    PS_ON_Init();
  #endif

  #ifdef FIL_RUNOUT_PIN
    FIL_Runout_Init();
  #endif

  #ifdef U_DISK_SUPPORT
    USBH_Init(&USB_OTG_Core, USB_OTG_FS_CORE_ID, &USB_Host, &USBH_MSC_cb, &USR_cb);
  #endif

  if (readIsTSCExist() == false)  // Read settings parameter
  {
    TSC_Calibration();
    storePara();
  }
  else if (readIsRestored())
  {
    storePara();
  }

  #ifdef LCD_LED_PWM_CHANNEL
    LCD_SetBrightness(lcd_brightness[infoSettings.lcd_brightness]);
  #endif

  switchMode();
}

int main(void)
{
  SystemClockInit();

  SCB->VTOR = VECT_TAB_FLASH;

  Hardware_GenericInit();

  #if defined(SERIAL_DEBUG_PORT) && defined(SERIAL_DEBUG_ENABLED)
    dbg_print("Main Startup: Generic debug output is enabled.\n");
  #endif

  for (; ;)
  {
    (*infoMenu.menu[infoMenu.cur])();
  }
}
