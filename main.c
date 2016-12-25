#define BUZZER_PIN      RA0
#define BUZZER_TRIS     TRISA0

#define _XTAL_FREQ      4000000L

// CONFIG
#pragma config FOSC = INTRCIO   // Oscillator Selection bits (INTOSC oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RA3/MCLR pin function select (RA3/MCLR pin function is MCLR)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD enabled)
#pragma config CP = OFF         // Code Protection bit (Program Memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)

#include <xc.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

uint32_t seed = 7;  // 100% random seed value

uint32_t random()
{
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

volatile uint16_t   combo_task              = 0;
volatile uint16_t   lights_counter          = 0;

const uint16_t combo1[3] = {    0b100000000 + (3 << 9),
                                0b011100000 + (3 << 9),
                                0b000011111 + (3 << 9) };

const uint16_t combo2[4] = {    0b110010000 + (5 << 9),
                                0b100100001 + (5 << 9),
                                0b101000100 + (5 << 9),
                                0b111111111 + (10 << 9) };

const uint16_t combo3[8] = {    0b100000000 + (3 << 9),
                                0b110100000 + (3 << 9),
                                0b110110001 + (3 << 9),
                                0b110111011 + (3 << 9),
                                0b110111111 + (3 << 9),
                                0b111111111 + (10 << 9),
                                0b000000000 + (8 << 9),
                                0b111111111 + (8 << 9),
                            };

static uint8_t          i_repeat        = 0;
static uint8_t          i_subcombo      = 0;

#define GET_BIT_VALUE(bits, x) ( (bits >> x) & 0x1 )

void set_lights( uint16_t input )
{
    RA4 = GET_BIT_VALUE( input, 0 );
    RA5 = GET_BIT_VALUE( input, 1 );
    RC5 = GET_BIT_VALUE( input, 2 );
    RA2 = GET_BIT_VALUE( input, 3 );
    RC0 = GET_BIT_VALUE( input, 4 );
    RC4 = GET_BIT_VALUE( input, 5 );
    RC3 = GET_BIT_VALUE( input, 6 );
    RC1 = GET_BIT_VALUE( input, 7 );
    RC2 = GET_BIT_VALUE( input, 8 );
}

void set_next_lights( void )
{
    static uint16_t *combo  = NULL;
    static uint8_t  size    = 0;
    static uint8_t  repeats = 0;
        
    if ( combo == NULL )
    {
        uint8_t random_number = random() % 3;
        if ( random_number == 0 ) {
            combo   = combo1;
            size    = 3;
            repeats = 1;
        } else if ( random_number == 1 ) {
            combo   = combo2;
            size    = 4;
            repeats = 1;
        } else if ( random_number == 2 ) {
            combo   = combo3;
            size    = 8;
            repeats = 1;
        }
    }
    
    set_lights( combo[i_subcombo] );
    combo_task = (combo[i_subcombo] >> 9) * (2000L);
    
    if ( ++i_subcombo >= size )
    {
        i_subcombo = 0;
        if ( ++i_repeat >= repeats )
        {
            i_repeat = 0;
            combo = NULL;
        }
    }

    lights_counter = 0;
}

// We wish
const char notes1[]         = "dggagfeceaabagfdfbbCbageddeafg,,";
const uint8_t durs1[]       = { 4, 4, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2, 4, 4, 4, 4, 2, 2, 2, 2, 4, 4, 2, 2, 4, 4, 4, 8, 8, 20 };

// Jingle
const char notes2[]         = "bbbbbbbDgab,CCCCCbbbbbaabaDbbbbbbbDgab,,CCCCCbbbbDDCag,,";
const uint8_t durs2[]       = { 4, 4, 8, 4, 4, 8, 4, 4, 6, 2, 12, 4, 4, 4, 6, 2, 4, 4, 4, 2, 2, 4, 4, 4, 4, 8, 8, 4, 4, 8, 4, 4, 8, 4, 4, 6, 2, 12, 4, 4, 4, 6, 2, 4, 4, 4, 2, 2, 4, 4, 4, 4, 12, 8, 20 };

static char     *current_note     = NULL;
static uint16_t beat_length; 

volatile bool       playing             = false;

volatile uint16_t   frequency_task      = 0;
volatile uint16_t   duration_task       = 0;
volatile uint16_t   delay_after_task    = 0;

const char      names[] = { 'c', 'd', 'e', 'f', 'g', 'a', 'b', 'C', 'D', ',' };
const uint16_t  tones[] = { 1915, 1700, 1519, 1432, 1275, 1136, 1014, 956, 851, 0  };

void play_note ( char note, uint16_t duration_ms ) 
{
    for ( uint8_t i = 0; i < sizeof(names); i++ ) 
    {
        if (note == names[i]) 
        {
            frequency_task      = tones[i] / 50;
            
            duration_task       = duration_ms * 20;
            delay_after_task    = beat_length * 10;
            
            playing = true;
        }
    }
}

void play_next_note ( void )
{
    static uint8_t  i_note      = 0;
    static uint8_t  *durs       = NULL;
    static uint8_t  length      = 0;
    
    if ( current_note == NULL ) {
        uint8_t random_number = random() % ( 2 );
        if ( random_number == 0 ) {
            current_note    = notes1;
            durs            = durs1;
            length          = sizeof( notes1 );
            beat_length     = 75;
        } else if ( random_number == 1 ) {
            current_note    = notes2;
            durs            = durs2;
            length          = sizeof( notes2 );
            beat_length     = 50;
        }
    }

    play_note( current_note[i_note], beat_length * durs[i_note] );
    
    if ( ++i_note >= length )
    {
        i_note = 0;
        current_note = NULL;
    }
}


int main(void) {
    
    CMCONbits.CM = 0b111;
    
    BUZZER_TRIS = 0;
    TRISC       = 0;
    TRISA       &= ~(1 << 2 | 1 << 4 | 1 << 5);
    
    OPTION_REGbits.PS   = 0b000;
    OPTION_REGbits.T0CS = 0;
    OPTION_REGbits.PSA  = 0;
    
    GIE     = 1;
    T0IF    = 0;
    T0IE    = 1;
    
    __delay_ms( 2000 );
    
    while ( 1 )
    {
        if ( lights_counter >= combo_task )
            set_next_lights();

        if ( !playing )
            play_next_note();
    }
    return 0;
}

void interrupt timer_isr (void)
{
    static uint16_t   frequency_counter     = 0;
    static bool       start_after_delay     = false;
    static uint16_t   duration_counter      = 0;
    
    if ( T0IF )
    {
        TMR0 = 230U;                // Each 50 us checking
        
        lights_counter++;
        
        if ( !playing )
        {
            BUZZER_PIN = 0;
            T0IF = 0;
            return;
        }
            
        if ( start_after_delay )
        {
            BUZZER_PIN = 0;
            
            if ( duration_counter++ >= delay_after_task )
            {
                start_after_delay = false;
                playing = false;
                duration_counter = 0;
            }
            
            T0IF = 0;
            return;
        }
        
        if ( duration_counter++ >= duration_task )
        {
            start_after_delay = true;
            duration_counter = 0;
            BUZZER_PIN = 0;
            T0IF = 0;
            return;
        } 
        else 
        {
            if ( frequency_task == 0 )
            {
                BUZZER_PIN = 0;
                T0IF = 0;
                return;
            }

            if ( frequency_counter++ >= frequency_task )
            {
                BUZZER_PIN = ~BUZZER_PIN;
                frequency_counter = 0;
            }
        }
        
        T0IF = 0;
    }
}
