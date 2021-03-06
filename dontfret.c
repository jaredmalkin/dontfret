#include <bcm2835.h>
#include <stdio.h>

// Define Input Pins
#define TOGGLE_SWITCH 	RPI_V2_GPIO_P1_32 	//GPIO12
#define FOOT_SWITCH 	RPI_GPIO_P1_10 		//GPIO15
#define LED   			RPI_V2_GPIO_P1_07 	//GPIO4
#define DELAY_MAX 800000

uint32_t Delay_Buffer[DELAY_MAX];
uint32_t DelayCounter = 0;
uint32_t Delay_Depth = 100000;

uint32_t read_timer=0;
uint32_t input_signal=0;
uint32_t output_signal=0;
uint32_t distortion_value=100;
uint32_t delay;

uint8_t FOOT_SWITCH_val;
uint8_t TOGGLE_SWITCH_val;

int main(int argc, char **argv)
{
  // Start the BCM2835 Library to access GPIO.
  if (!bcm2835_init())
  { printf("bcm2835_init failed. Are you running as root??\n");
  return 1;}
  // Start the SPI BUS.
  if (!bcm2835_spi_begin())
  {printf("bcm2835_spi_begin failed. Are you running as root??\n");
  return 1;}

  //define PWM
  bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_ALT5 ); //PWM0 signal on GPIO18
  bcm2835_gpio_fsel(13,BCM2835_GPIO_FSEL_ALT0 ); //PWM1 signal on GPIO13
  bcm2835_pwm_set_clock(2); // Max clk frequency (19.2MHz/2 = 9.6MHz)
  bcm2835_pwm_set_mode(0,1 , 1); //channel 0, markspace mode, PWM enabled.
  bcm2835_pwm_set_range(0,64);   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz switching PWM freq.
  bcm2835_pwm_set_mode(1, 1, 1); //channel 1, markspace mode, PWM enabled.
  bcm2835_pwm_set_range(1,64);   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz switching PWM freq.

  //define SPI bus configuration
  bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
  bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
  bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64); 	  // 4MHz clock with _64
  bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
  bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

  uint8_t mosi[10] = { 0x01, 0x00, 0x00 }; //12 bit ADC read 0x08 ch0, - 0c for ch1
  uint8_t miso[10] = { 0 };

  //Define GPIO pins configuration
  bcm2835_gpio_fsel(TOGGLE_SWITCH, BCM2835_GPIO_FSEL_INPT);	//TOGGLE_SWITCH as input
  bcm2835_gpio_fsel(FOOT_SWITCH, BCM2835_GPIO_FSEL_INPT); 	//FOOT_SWITCH as input
  bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);				//LED as output

  bcm2835_gpio_set_pud(TOGGLE_SWITCH, BCM2835_GPIO_PUD_UP);   //TOGGLE_SWITCH pull-up enabled
  bcm2835_gpio_set_pud(FOOT_SWITCH, BCM2835_GPIO_PUD_UP);     //FOOT_SWITCH pull-up enabled

  while(1) //Main Loop
  {
    //read 12 bits ADC
    bcm2835_spi_transfernb(mosi, miso, 3);
    input_signal = miso[2] + ((miso[1] & 0x0F) << 8);

    //Read buttons every 50000 times (0.25s)
    read_timer++;
    if (read_timer==50000)
    {
      read_timer=0;
      TOGGLE_SWITCH_val = bcm2835_gpio_lev(TOGGLE_SWITCH);
      uint8_t FOOT_SWITCH_val = bcm2835_gpio_lev(FOOT_SWITCH);
      //light the effect when the footswitch is activated.
      bcm2835_gpio_write(LED,!FOOT_SWITCH_val);


    }

    //DISTORTION if toggle switch off
    if(TOGGLE_SWITCH_val == 0){
      //////// This code comes from distortion.c on
      //////// https://www.electrosmash.com/forum/pedal-pi/216-distortion-effect-guitar-pedal?lang=en
      //**** DISTORTION EFFECT ***///
      //The input_signal is clipped to a threshold when it reaches the distortion_value threshold.
      //The guitar signal fluctuates above and under 2047.
      if (input_signal > 2047 + distortion_value) input_signal= 2047 + distortion_value;
      if (input_signal < 2047 - distortion_value) input_signal= 2047 - distortion_value;

      bcm2835_pwm_set_data(1,input_signal & 0x3F);
      bcm2835_pwm_set_data(0,input_signal >> 6);
    }

    //DELAY if toggle switch on
    else{
      /////// This code comes from delay.c on
      /////// https://www.electrosmash.com/forum/pedal-pi/219-delay-guitar-effect-pedal?lang=en
      //**** DELAY EFFECT ****///
      //The input_signal is saved in a "circular" buffer (Delay_Buffer) to be recovered later.
      //The delayed signal is added again to the current guitar input so you can hear the original and delayed at the
      //same time
      Delay_Buffer[DelayCounter] = input_signal;
      DelayCounter++;
      if(DelayCounter >= Delay_Depth) DelayCounter = 0;
      output_signal = (Delay_Buffer[DelayCounter]+input_signal)>>1;

      bcm2835_pwm_set_data(1,output_signal & 0x3F);
      bcm2835_pwm_set_data(0,output_signal >> 6);
    }

  }

  //close all and exit
  bcm2835_spi_end();
  bcm2835_close();
  return 0;
}
