#include "stdint.h"
#include "stdbool.h"
#include "inc/tm4c123gh6pm.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/hibernate.h"
#include "inc/hw_memmap.h"

void UART_Init(void);
void UART_WriteChar(char c);
char UART_ReadChar(void);
uint32_t ConvertToSeconds(char *buffer);
void HibernateMode(uint32_t seconds);
void SetInitSettings(void);
void HibernateConfig(void);
void haricikesme(void);

int main(void)
{
    char buffer[10];
    int index = 0;
    char receivedChar;
    uint32_t seconds;

    UART_Init();
    SetInitSettings();
    HibernateConfig();

    while (1)
    {
        receivedChar = UART_ReadChar();

        if (receivedChar == '\n')
        {
            buffer[index] = '\0'; // Null-terminate string
            seconds = ConvertToSeconds(buffer);

            if (seconds >= 1 && seconds <= 60)
            {
                HibernateMode(seconds); // Hibernate for the given seconds
            }

            index = 0; // Reset buffer after processing
        }
        else
        {
            buffer[index++] = receivedChar; // Collect incoming characters
        }
    }
}

void SetInitSettings()
{
    SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
}

void HibernateConfig(void)
{
    HibernateEnableExpClk(SysCtlClockGet());
    HibernateGPIORetentionEnable();
    HibernateRTCSet(0);
    HibernateRTCEnable();
    HibernateRTCMatchSet(0, 4);
    HibernateWakeSet(HIBERNATE_WAKE_PIN | HIBERNATE_WAKE_RTC);

    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);

    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_STRENGTH_4MA, GPIO_PIN_TYPE_STD_WPU);

    IntMasterEnable();
    GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4, GPIO_FALLING_EDGE);
    GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);
    GPIOIntRegister(GPIO_PORTF_BASE, haricikesme);  // Kesme iþleyicisini kaydet
}

void haricikesme(void)
{
    int a = GPIOIntStatus(GPIO_PORTF_BASE, true);

    if (a == 16)  // PF4'e basýlmýþtýr
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 2);  // Kýrmýzý LED (PF1) yanacak
    }
    else if (a == 1)  // PF0'a basýlmýþtýr
    {
        GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 4);  // Yeþil LED (PF3) yanacak
    }

    GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0 | GPIO_PIN_4);  // Kesme bayraðýný temizle
}

void HibernateMode(uint32_t seconds)
{
    HibernateRTCMatchSet(0, seconds * 4);  // RTC eþleþmesini verilen saniyeye göre ayarla
    HibernateRequest();
    __asm("wfi");
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3, 0);  // LED'leri kapat
}

void UART_Init(void)
{
    SYSCTL_RCGCUART_R |= 0x01;
    SYSCTL_RCGCGPIO_R |= 0x01;

    UART0_CTL_R &= ~0x01;
    UART0_IBRD_R = 104;
    UART0_FBRD_R = 11;
    UART0_LCRH_R = 0x70;
    UART0_CTL_R = 0x301;

    GPIO_PORTA_AFSEL_R |= 0x03;
    GPIO_PORTA_DEN_R |= 0x03;
}

void UART_WriteChar(char c)
{
    while ((UART0_FR_R & 0x20) != 0);
    UART0_DR_R = c;
}

char UART_ReadChar(void)
{
    while ((UART0_FR_R & 0x10) != 0);
    return (char)(UART0_DR_R & 0xFF);
}

uint32_t ConvertToSeconds(char *buffer)
{
    uint32_t value = 0;
    while (*buffer)
    {
        value = value * 10 + (*buffer - '0');
        buffer++;
    }
    return value;
}
