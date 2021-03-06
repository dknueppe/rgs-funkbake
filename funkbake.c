/**
 * @file funkbake.c
 * 
 * @author Daniel Knüppe
 * 
 * @brief Signalgenerator für eine Funkbake
 * 
 * Der Mikrokontroller ist ein ATtiny84 (Als Device für den flasher)
 * 
 * Fuse Bit settings für das Projekt Funkpbake:
 * 
 * SELFPRGEN    = [ ]
 * RSTDISBL     = [ ]
 * DWEN         = [ ]
 * SPIEN        = [X]
 * WDTON        = [ ]
 * EESAVE       = [ ]
 * BODLEVEL     = DISABLED
 * CKDIV8       = [X]
 * CKOUT        = [ ]
 * SUT_CKSEL    = EXTXOSC_8MHZ_XX_16KCK_14CK_65MS
 * 
 * EXTENDED     = 0xFF (valid)
 * HIGH         = 0xDF (valid)
 * LOW          = 0x7F (valid)
*/

#include <avr/io.h>
#define F_CPU 2000000UL
#include <util/delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include "avr/portpins.h"

/**
 * Diese Makros sind nur Shortcuts zum Ein- und Ausschalten des Relais
 * beziehungsweise des Open-Collector Ausgangs. Es ist nicht geeignet um den 
 * Aktuellen Zustand abzufragen.
 *     if (REED_RELAIS_ON)  (etc.)
 * prüft NICHT ob das Relais beziehungsweise der Open-Collector ein- / 
 * ausgeschaltet ist!
 */
#define REED_RELAIS_ON     (PORTA |=  (1 << PORTA7))
#define REED_RELAIS_OFF    (PORTA &= ~(1 << PORTA7))
#define OPEN_COLLECTOR_ON  (PORTB |=  (1 << PORTB2))
#define OPEN_COLLECTOR_OFF (PORTB &= ~(1 << PORTB2))
#define BOTH_ON REED_RELAIS_ON; OPEN_COLLECTOR_ON;
#define BOTH_OFF REED_RELAIS_OFF; OPEN_COLLECTOR_OFF;

/**
 * Jedes Symbol besteht aus 'dit' oder 'dah' und wird von einem
 * 'symbol_space' abgeschlossen, deshalb ist 'letter_space' nur
 * zwei dit lang, und da nach jedem Buchstaben bereits ein 
 * 'letter_space' kommt, ist 'word_space' auch nur 4 dit lang.
 * DIT_LEN ist die Dauer eines Dit in Millisekunden, das Makro
 * WPM_TO_DIT_LEN rechnet automatisch die Dauer eines Dit für
 * eine gewünschte Sendegeschwindigkeit aus. DIT_LEN kann jedoch
 * auch von Hand auf einen festen Wert gesetzt werden.
 */
#define WPM_TO_DIT_LEN(WPM) (1200 / WPM)
#define DIT_LEN WPM_TO_DIT_LEN(15)
#define delay_and_count(x) _delay_ms(x * DIT_LEN); t += x * DIT_LEN
#define symbol_space    BOTH_OFF;   delay_and_count(1)
#define letter_space                delay_and_count(2)
#define word_space                  delay_and_count(4)
#define dit             BOTH_ON;    delay_and_count(1); symbol_space;
#define dah             BOTH_ON;    delay_and_count(3); symbol_space;

/**
 * Eine Look-Up Tabelle, um ein Zeichen in seine Räpresentation
 * als Morsecode zu überführen. Braucht die obigen Makros.
 * gültig sind nur, 'A' bis 'Z', 'a' bis 'z' und '0' bis '9'
 * alles andere wird als Worttrennung aufgefasst und verursacht eine Pause.
 * Die Funktion gibt die Zeit die sie zum Morsen gebraucht hat in ms zurück.
 */
static int32_t string_to_morse(const char* msg)
{
    unsigned int t = 0;
    for (;*msg; msg++) {
        /* Kleinbuchstaben zu Großbuchstaben konvertieren */
        char c = ((*msg >= 'a') && (*msg <= 'z')) ? *msg + ('A' - 'a') : *msg ;
        switch (c) {
        case 'A': dit dah               break;
        case 'B': dah dit dit dit       break;
        case 'C': dah dit dah dit       break;
        case 'D': dah dit dit           break;
        case 'E': dit                   break;
        case 'F': dit dit dah dit       break;
        case 'G': dah dah dit           break;
        case 'H': dit dit dit dit       break;
        case 'I': dit dit               break;
        case 'J': dit dah dah dah       break;
        case 'K': dah dit dah           break;
        case 'L': dit dah dit dit       break;
        case 'M': dah dah               break;
        case 'N': dah dit               break;
        case 'O': dah dah dah           break;
        case 'P': dit dah dah dit       break;
        case 'Q': dah dah dit dah       break;
        case 'R': dit dah dit           break;
        case 'S': dit dit dit           break;
        case 'T': dah                   break;
        case 'U': dit dit dah           break;
        case 'V': dit dit dit dah       break;
        case 'W': dit dah dah           break;
        case 'X': dah dit dit dah       break;
        case 'Y': dah dit dah dah       break;
        case 'Z': dah dah dit dit       break;
        case '1': dit dah dah dah dah   break;
        case '2': dit dit dah dah dah   break;
        case '3': dit dit dit dah dah   break;
        case '4': dit dit dit dit dah   break;
        case '5': dit dit dit dit dit   break;
        case '6': dah dit dit dit dit   break;
        case '7': dah dah dit dit dit   break;
        case '8': dah dah dah dit dit   break;
        case '9': dah dah dah dah dit   break;
        case '0': dah dah dah dah dah   break;
        default : word_space;
        }
        letter_space;
    }
    return t;
}

/* 
 * Pin PA7 wird als Ausgang gesetzt und High geschrieben.
 * Alle anderen Pins an PortA sind als Eingang gesetzt,
 * mit ausgeschaltetem Pull-Up
 */
static void init()
{
    PORTA = 0x80;
    DDRA  = 0x80;
    PORTB = 0x04;
    DDRB  = 0x04;
}

/*
 * Sortiert die Bits vom Eingang am PortA so um, dass sie
 * mit den Positionen des Dip-Schalters übereinstimmen.
 */
static uint8_t dip_to_index()
{
    uint8_t pos = ((~PINA & 0x1) << 3) | \
                  ((~PINA & 0x2) << 1) | \
                  ((~PINA & 0x4) >> 1) | \
                  ((~PINA & 0x8) >> 3);
    return pos;
}

/**
 * Hier sind die über den Dipschalter wählbaren Texte hinterlegt.
 * Die Position im Array korrespondiert zu der Position des Dipschalters.
 */
static const char *text[16] = {
    "DF0MU ",
    "DJ8EN",
    "DK2FD",
    "DL8YEH",
    "DH8AF",
};

/**
 * Die main Routine initialisiert zuerst die Ausgänge entsprechend der init Routine.
 * Anschließend wird in einer Endlosschleife die Position des Dipschalters
 * abgefragt. Anschließend wird ein Textstring aus dem obigen Array gewählt
 * und gemorst. Schließlich wird noch das Restintervall zu einer Minute (60000ms)
 * gewartet, bevor die Schleife erneut beginnt.
 */
int main(void)
{
    init();
    while (1) {
        uint8_t pos = dip_to_index();
        int32_t t = string_to_morse(text[pos]);
        REED_RELAIS_ON;
        for (int32_t i = 60000 - 7 * DIT_LEN - t; i > 0; i -= DIT_LEN)
            _delay_ms(DIT_LEN);
        symbol_space; letter_space; word_space;
    }
    return 0;
}
