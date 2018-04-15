#include <stdio.h>
#include <bcm2835.h>

// Define Input Pins
#define TOGGLE_SWITCH     RPI_V2_GPIO_P1_32   //GPIO12
#define FOOT_SWITCH       RPI_GPIO_P1_10      //GPIO15
#define LED               RPI_V2_GPIO_P1_36   //GPIO16

uint32_t read_timer=0;
uint32_t input_signal=0;

uint8_t FOOT_SWITCH_val;
uint8_t TOGGLE_SWITCH_val;


//main program
int main(int argc, char **argv)
{
    // Start the BCM2835 Library to access GPIO.
    if (!bcm2835_init())
    {printf("bcm2835_init failed. Are you running as root??\n"); return 1;}


    // Start the SPI BUS.
    if (!bcm2835_spi_begin())
    {printf("bcm2835_spi_begin failed. Are you running as root??\n"); return 1;}

    //define PWM mode
    bcm2835_gpio_fsel(18,BCM2835_GPIO_FSEL_ALT5 ); //PWM0 signal on GPIO18
    bcm2835_gpio_fsel(13,BCM2835_GPIO_FSEL_ALT0 ); //PWM1 signal on GPIO13
    bcm2835_pwm_set_clock(2);                      // Max clk frequency (19.2MHz/2 = 9.6MHz)
    bcm2835_pwm_set_mode(0,1 , 1);                 //channel 0, markspace mode, PWM enabled.
    bcm2835_pwm_set_range(0,64);                   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz PWM freq.
    bcm2835_pwm_set_mode(1, 1, 1);                 //channel 1, markspace mode, PWM enabled.
    bcm2835_pwm_set_range(1,64);                   //channel 0, 64 is max range (6bits): 9.6MHz/64=150KHz PWM freq.

    //define SPI bus configuration
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);      // The default
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // The default
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_64);    // 4MHz clock with _64
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                      // The default
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);      // the default

    uint8_t mosi[10] = { 0x01, 0x00, 0x00 }; //12 bit ADC read channel 0.
    uint8_t miso[10] = { 0 };

    //Define GPIO pins configuration
    bcm2835_gpio_fsel(TOGGLE_SWITCH, BCM2835_GPIO_FSEL_INPT);   //TOGGLE_SWITCH as input
    bcm2835_gpio_fsel(FOOT_SWITCH, BCM2835_GPIO_FSEL_INPT);     //FOOT_SWITCH as input
    bcm2835_gpio_fsel(LED, BCM2835_GPIO_FSEL_OUTP);             //LED as output

    bcm2835_gpio_set_pud(TOGGLE_SWITCH, BCM2835_GPIO_PUD_UP);   //TOGGLE_SWITCH pull-up enabled
    bcm2835_gpio_set_pud(FOOT_SWITCH, BCM2835_GPIO_PUD_UP);     //FOOT_SWITCH pull-up enabled

    while(1) //Main Loop
    {
    //Read the PUSH buttons every 50000 times (0.25s) to save resources.
    read_timer++;
    if (read_timer==50000)
    {
    read_timer=0;
    TOGGLE_SWITCH_val = bcm2835_gpio_lev(TOGGLE_SWITCH);
    uint8_t FOOT_SWITCH_val = bcm2835_gpio_lev(FOOT_SWITCH);
    bcm2835_gpio_write(LED,!FOOT_SWITCH_val); //light the effect when the footswitch is activated.
    }

    //read 12 bits ADC
    bcm2835_spi_transfernb(mosi, miso, 3);

    //print raw input before calculation
    printf("miso[2]: " + miso[2] + "miso[1]: " + miso[1]);
    input_signal = miso[2] + ((miso[1] & 0x0F) << 8);

    //Print input for testing. input_signal should be right around 2047,
    //with min voltage returning 0 and max voltage returning 4095.
    printf("input: " + input_signal);

    //**** CLEAN EFFECT ***///
    //Nothing to do, the input_signal goes directly to the PWM output.

    //generate output PWM signal 6 bits
    bcm2835_pwm_set_data(1,input_signal & 0x3F);
    bcm2835_pwm_set_data(0,input_signal >> 6);
    }

    //close all and exit
    bcm2835_spi_end();
    bcm2835_close();
    return 0;
}
