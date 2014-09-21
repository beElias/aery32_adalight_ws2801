#include "board.h"
#include <aery32/all.h>
#include <aery32/delay.h>
#include <stdlib.h>

#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>
#include "Descriptors.h"

#define SPI0_PINMASK ((1 << 10) | (1 << 11) | (1 << 12) | (1 << 13))

using namespace aery;

/** LUFA CDC Class driver interface configuration and state information. This structure is
*  passed to all CDC Class driver functions, so that multiple instances of the same class
*  within a device can be differentiated from one another.
*/
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
    { /* Config */
        0, /* ControlInterfaceNumber */
        { /* DataINEndpoint */
            CDC_TX_EPADDR, /* Address */
            CPU_TO_LE16(CDC_TXRX_EPSIZE), /* Size */
            EP_TYPE_CONTROL, /* Endpoint type */
            1, /* Banks */
        },
        { /* DataOUTEndpoint */
            CDC_RX_EPADDR, /* Address */
            CPU_TO_LE16(CDC_TXRX_EPSIZE), /* Size */
            EP_TYPE_CONTROL, /* Endpoint type */
            1, /* Banks */
        },
        { /* NotificationEndpoint */
            CDC_NOTIFICATION_EPADDR, /* Address */
            CPU_TO_LE16(CDC_NOTIFICATION_EPSIZE), /* Size */
            EP_TYPE_CONTROL, /* Endpoint type */
            1, /* Banks */
        }
    }
};

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);	
}

int main(void)
{
    board::init();
    gpio_init_pin(LED, GPIO_OUTPUT|GPIO_HIGH);

    gpio_init_pins(porta, SPI0_PINMASK, GPIO_FUNCTION_A);

    /*
    * Init SPI0 as master. The chip select pin 0 of SPI0 is set to work in
    * spi mode 0 and 24 bits wide shift register.
    */
    spi_init_master(spi0);
    spi_setup_npcs(spi0, 0, SPI_MODE0, 24);
    spi_enable(spi0);
    spi0->CSR0.scbr = 64; /// 1Mhz

    /* Set up the USB generic clock. That's f_pll1 / 2 == 48MHz. */
    pm_init_gclk(GCLK_USBB, GCLK_SOURCE_PLL1, 1);
    pm_enable_gclk(GCLK_USBB);

    /* Register USB isr handler, from LUFA library */
    INTC_Init();
    INTC_RegisterGroupHandler(INTC_IRQ_GROUP(AVR32_USBB_IRQ), 1, &USB_GEN_vect);
    GlobalInterruptEnable();

    /* Initiliaze the USB, LUFA magic */
    USB_Init();

    uint8_t prefix = 0;
    uint8_t magic = 0;
    uint8_t magicWord[3] = {'A', 'd', 'a'};
    uint8_t magicIn[3] = { 0 };
    uint8_t ledInfo[3] = { 0 };
    uint8_t ledInfoCount = 0;
    uint8_t hi, lo, chk;
    int32_t ledChannles = 1000;
    uint8_t colors[1500] = { 0 };
    int32_t dataCount = 0;
    uint8_t latch = 0;
    uint8_t latchCounter = 0;

    CDC_Device_SendString(&VirtualSerial_CDC_Interface,"Zup?\n"); // Zup?

    /* Fancy stuff when power is connected */
    for (int j = 0; j < 50; j++)
    {
        for (int i = 0; i < 50; i++)
        {
            if (j == i)
            { 
                spi_transmit(spi0, 0, 0xff);
                spi_transmit(spi0, 0, 0xff);
                spi_transmit(spi0, 0, 0xff);
            }
            else 
            {
                spi_transmit(spi0, 0, 0);
                spi_transmit(spi0, 0, 0);
                spi_transmit(spi0, 0, 0);
            }
        }
        delay_ms(8);
    }
    for (int i = 0; i < 255; ++i)
    {
        for (int j = 0; j < 50; ++j)
        {
            spi_transmit(spi0, 0, i);
            spi_transmit(spi0, 0, 0x00);
            spi_transmit(spi0, 0, 0x00);
        }
        delay_ms(1);
    }
    for (int i = 0; i < 255; ++i)
    {
        for (int j = 0; j < 50; ++j)
        {
            spi_transmit(spi0, 0, 255 - i);
            spi_transmit(spi0, 0, i);
            spi_transmit(spi0, 0, 0x00);
        }
        delay_ms(1);
    }
    for (int i = 0; i < 255; ++i)
    {
        for (int j = 0; j < 50; ++j)
        {
            spi_transmit(spi0, 0, 0x00);
            spi_transmit(spi0, 0, 255 - i);
            spi_transmit(spi0, 0, i);
        }
        delay_ms(1);
    }
    for (int i = 0; i < 255; ++i)
    {
        for (int j = 0; j < 50; ++j)
        {
            spi_transmit(spi0, 0, 0x00);
            spi_transmit(spi0, 0, 0x00);
            spi_transmit(spi0, 0, 254 - i);
        }
        delay_ms(1);
    }

    /* Ok, time for work! */
    for (;;) 
    {
        if (CDC_Device_BytesReceived(&VirtualSerial_CDC_Interface))
        {
            /* Data has been received */
            /* Turn on the onboard led so we can see that data is being processed */
            gpio_set_pin_high(LED);

            if (!prefix)
            {
                if (!magic)
                {
                    /* Look for the magic word */
                    for (int i = 2; i > 0; i--) magicIn[i] = magicIn[i - 1];
                    magicIn[0] = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
                    for (int i = 0; (i < 3) && (magicIn[i] == magicWord[2 - i]); i++)
                    {
                        if (i == 2) magic = 1; // Magic word matched
                    }
                }
                else
                {
                    /* Magic word has been received, read led info */
                    for (int i = 2; i >= 0; i--) ledInfo[i] = ledInfo[i - 1];
                    ledInfo[0] = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
                    if (++ledInfoCount > 2)
                    {
                        hi = ledInfo[2];
                        lo = ledInfo[1];
                        chk = ledInfo[0];
                        if (chk == hi ^ lo ^ 0x56) // Checksum test
                        {
                            /* Checksum is valid */
                            ledChannles = 3 * ((hi<<8) + lo + 1);
                            ledInfoCount = 0;
                            prefix = 1;
                            magic = 0;
                            dataCount = 0;
                        }
                        else
                        {
                            /* Cecksum invalid */
                            ledInfoCount = 0;
                            magic = 0;
                        }
                    }
                }
            }
            else // Prefix is set, read the color data
            {
                if (dataCount < ledChannles) colors[dataCount++] = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
            }
        }

        if (dataCount >= ledChannles && !latch && prefix)
        {
            /* The color data is received, set the led colors */
            for (int i = dataCount; i > 0; i--) spi_transmit(spi0, 0, colors[ledChannles - i]);
            latch = 1;
        }

        if (latch)
        {
            /* Allow the WS2801 to latch, latch time is 500uS */
            delay_us(100);
            if (latch++ >= 5)
            {
                latch = 0;
                prefix = 0;
            }
        }

        /* let LUFA do its thing */
        CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
        USB_USBTask();

        /* turn off the led */
        gpio_set_pin_low(LED);
    }

    return 0;
}
