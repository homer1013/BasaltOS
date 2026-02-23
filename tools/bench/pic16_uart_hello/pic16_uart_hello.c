#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 8000000UL

static void uart_delay_bit(void) {
    __delay_us(833);  // ~1200 baud at 8 MHz
}

static void tx_drive(uint8_t level) {
    LATAbits.LATA0 = level; LATAbits.LATA1 = level; LATAbits.LATA2 = level;
    LATAbits.LATA4 = level; LATAbits.LATA5 = level;
    LATBbits.LATB4 = level; LATBbits.LATB5 = level; LATBbits.LATB6 = level; LATBbits.LATB7 = level;
    LATCbits.LATC0 = level; LATCbits.LATC1 = level; LATCbits.LATC2 = level; LATCbits.LATC3 = level;
    LATCbits.LATC4 = level; LATCbits.LATC5 = level; LATCbits.LATC6 = level; LATCbits.LATC7 = level;
}

static void uart_tx_byte(uint8_t b) {
    tx_drive(0);  // start bit
    uart_delay_bit();

    for (uint8_t i = 0; i < 8; ++i) {
        tx_drive((uint8_t)((b >> i) & 0x01u));
        uart_delay_bit();
    }

    tx_drive(1);  // stop bit
    uart_delay_bit();
}

static void uart_tx_text(const char *s) {
    while (*s) {
        uart_tx_byte((uint8_t)*s++);
    }
}

void main(void) {
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x00;
    tx_drive(1);

    __delay_ms(120);
    uart_tx_text("BASALT PIC16 UART HELLO\r\n");

    while (1) {
        uart_tx_text("pic16 heartbeat\r\n");
        __delay_ms(900);
    }
}
