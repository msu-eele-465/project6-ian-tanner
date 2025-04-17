#include <msp430.h>
#include <stdint.h>
#include <math.h>

/**
 * main.c
 */

#define LM92_ADDRESS 0x48
#define LCD_ADDRESS 0x01   // Address of the LCD MSP430FR2310
#define TX_BYTES 6         // Number of bytes to transmit
#define LED1 BIT0
#define LED2 BIT1
#define LED3 BIT2
#define LED4 BIT3
#define LED5 BIT4
#define LED6 BIT0
#define LED7 BIT1
#define LED8 BIT2

// Temperature Data
volatile int window_size = 3;
int adc_results[10];
int lm92_results[10];
int lm19_sample_index = 0;
int lm19_samples_collected = 0;
volatile int lm19_temperature_integer = 0;
volatile int lm19_temperature_decimal = 0;
int lm92_samples[10];
int lm92_sample_index = 0;
int lm92_samples_collected = 0;
volatile int lm92_temperature_integer = 0;
volatile int lm92_temperature_decimal = 0;
volatile int timer = 0;
volatile int temp_match = 0;
int heat = 0;
int cool = 1;

// I2C Data
volatile int tx_index = 0;
char tx_buffer[TX_BYTES] = {0, 0, 0, 0, 0, 3};
unsigned char lm92_data[2];
unsigned int lm92_byte_count = 0;

// LED Data
volatile int step_pattern_heat;
volatile int step_pattern_cool;
volatile int pattern = 0;

// State Data
enum State {LOCKED, UNLOCKING, UNLOCKED, OFF, HEAT, COOL, MATCH, MATCH_SET, SET_TEMP, SET_WINDOW};
enum State state = LOCKED;
enum State sub_state = LOCKED;

void send_I2C_data()
{
    tx_index = 0; // Reset buffer index
    UCB0CTLW0 |= UCTR | UCTXSTT;  // Start condition, put master in transmit mode
    UCB0IE |= UCTXIE0; // Enable TX interrupt
}

void get_lm92_i2c()
{
    UCB1CTLW0 &= ~UCTR;          // Receiver mode
    UCB1CTLW0 |= UCTXSTT;        // Start condition
    UCB1IE |= UCRXIE1;           // Enable RX interrupt
}

void start_ADC_conversion()
{
    ADCCTL0 |= ADCENC | ADCSC;
}

void update_leds(int pattern)
   {
       P5OUT &= ~(LED1 | LED2 | LED3 | LED4 | LED5);
       P6OUT &= ~(LED6 | LED7 | LED8);

       // Set LEDs based on the pattern
       P5OUT |= (pattern & 0x01) ? LED1 : 0;
       P5OUT |= (pattern & 0x02) ? LED2 : 0;
       P5OUT |= (pattern & 0x04) ? LED3 : 0;
       P5OUT |= (pattern & 0x08) ? LED4 : 0;
       P5OUT |= (pattern & 0x10) ? LED5 : 0;
       P6OUT |= (pattern & 0x20) ? LED6 : 0;
       P6OUT |= (pattern & 0x40) ? LED7 : 0;
       P6OUT |= (pattern & 0x80) ? LED8 : 0;
   }

void get_temperature()
{
    int i;
    unsigned int total_adc_value = 0;
    for (i = 1; i <= window_size; i++)
    {
        total_adc_value += adc_results[i];
    }

    unsigned int average_adc_value = total_adc_value / window_size;
    float voltage = (average_adc_value / 4095.0);
    float temperature = -1481.96 + sqrt(2.1962e6 + ((1.8639 - voltage) / (3.88e-6)));
    lm19_temperature_integer = (int)temperature;
    temperature *= 10.0;
    lm19_temperature_decimal = (int)(temperature - lm19_temperature_integer);
    tx_buffer[1] = lm19_temperature_integer;
    tx_buffer[2] = lm19_temperature_decimal;

    if (state != LOCKED)
    {
        send_I2C_data();
    }
}

void peltier_control()
{
    if (timer == 300)
    {
        heat = 0;
        cool = 0;
        P1OUT &= ~(BIT7 | BIT6);
        timer = 0;
        state = OFF;
        tx_buffer[0] = 2;
    }
    if (state == HEAT)
    {
        heat = 1;
        cool = 0;
        P1OUT &= ~BIT6;
        P1OUT |= BIT7;
    }
    else if (state == COOL)
    {
        cool = 1;
        heat = 0;
        P1OUT &= ~BIT7;
        P1OUT |= BIT6;
    }
    else if (state == MATCH)
    {
        if (lm92_temperature_integer > lm19_temperature_integer)
        {
            cool = 1;
            heat = 0;
            P1OUT &= ~BIT7;
            P1OUT |= BIT6;
        }
        else if (lm92_temperature_integer < lm19_temperature_integer)
        {
            heat = 1;
            cool = 0;
            P1OUT &= ~BIT6;
            P1OUT |= BIT7;
        }
    }
    else if (state == MATCH_SET)
    {
        if (lm92_temperature_integer > temp_match)
        {
            cool = 1;
            heat = 0;
            P1OUT &= ~BIT7;
            P1OUT |= BIT6;
        }
        else if (lm92_temperature_integer < temp_match)
        {
            heat = 1;
            cool = 0;
            P1OUT &= ~BIT6;
            P1OUT |= BIT7;
        }
    }
}

// Keypad data
// 2D Array, each array is a row, each item is a column.

char keyPad[][4] = {{'1', '2', '3', 'A'},  // Top Row
                    {'4', '5', '6', 'B'},
                    {'7', '8', '9', 'C'},
                    {'*', '0', '#', 'D'}}; // Bottom Row
/*                    ^              ^
 *                    |              |
 *                    Left Column    Right Column
 */

int column, row = 0;
char key_pressed = '\0';
char pass_code[] = "2659";
char input_code[] = "0000";
int mili_seconds_surpassed = 0;
int index = 0;  // Which index of the above input_code array we're in

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //---------------- Configure ADC ---------------
    // Set P1.0 as ADC input
    P1SEL0 |= BIT1;
    P1SEL1 |= BIT1;
    ADCCTL0 &= ~ADCSHT;
    ADCCTL0 |= ADCSHT_2;
    ADCCTL0 |= ADCON;
    ADCCTL1 |= ADCSSEL_2;
    ADCCTL1 |= ADCSHP;
    ADCCTL2 &= ~ADCRES;
    ADCCTL2 |= ADCRES_2;
    ADCMCTL0 |= ADCINCH_1;
    ADCIE |= ADCIE0;
    //---------------- End Configure ADC --------------

    //---------------- Configure TB0 ------------------
    TB0CTL |= TBCLR;            // Clear TB0 timer and dividers
    TB0CTL |= TBSSEL__SMCLK;    // Select SMCLK as clock source
    TB0CTL |= MC__UP;            // Choose UP counting
    TB0CCR0 = 1000;             // TTB0CCR0 = 1000, since 1/MHz * 1000 = 1 ms
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 interrupt flag
    TB0CCTL0 |= CCIE;           // Enable interrupt vector for CCR0
    //---------------- End Configure TB0 --------------

    //---------------- Configure P3 -------------------
    P3SEL0 &= 0x00;
    P3SEL1 &= 0x00;
    P3DIR &= 0x0F;  // CLEARING bits 7 - 4, that way they are set to INPUT mode
    P3DIR |= 0X0F;  // SETTING bits 0 - 3, that way they are set to OUTPUT mode
    P3REN |= 0xF0;  // ENABLING the resistors for bits 7 - 4
    P3OUT &= 0x00;  // CLEARING output register. This both clears our outputs on bits 0 - 3, and sets pull-down resistors
                    // for bits 7 - 4
    //---------------- End Configure P3 ----------------

    //---------------- Configure LEDs ------------------
    //Heartbeat LEDs
    P1DIR |= BIT0;            //Config P1.0 (LED1) as output
    P1OUT |= BIT0;            //LED1 = 1 to start
    P6DIR |= BIT6;            //Config P6.6 (LED2) as output
    P6OUT &= ~BIT6;           //LED2 = 0 to start
    // LED box configuration
    P5DIR |= LED1 | LED2 | LED3 | LED4 | LED5;
    P6DIR |= LED6 | LED7 | LED8;
    P5OUT &= ~(LED1 | LED2 | LED3 | LED4 | LED5);
    P6OUT &= ~(LED6 | LED7 | LED8);
    //---------------- End Configure LEDs ---------------
    //---------------- Configure Heat/Cool pins ---------
    P1DIR |= BIT7; //Heat
    P1DIR |= BIT6; //Cool
    P1OUT &= ~BIT7;
    P1OUT &= ~BIT6;
    //---------------- End Configure Heat/Cool ----------
    //---------------- Configure Timers -----------------
    //LED Timer
    TB1CTL |= TBCLR;
    TB1CTL |= TBSSEL__ACLK;
    TB1CTL |= MC__UP;
    TB1CCR0 = 32768;
    TB1CCTL0 |= CCIE;
    TB1CCTL0 &= ~CCIFG;

    //Temperature Sample Timer
    TB2CTL |= TBCLR;
    TB2CTL |= TBSSEL__ACLK;
    TB2CTL |= MC__UP;
    TB2CCR0 = 16384;
    TB2CCTL0 |= CCIE;         //enable TB2 CCR0 Overflow IRQ
    TB2CCTL0 &= ~CCIFG;       //clear CCR0 flag
    //---------------- End Timer Configure --------------

    //---------------- Configure UCB0 I2C ---------------

    // Configure P1.2 (SDA) and P1.3 (SCL) for I2C
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    // Put eUSCI_B0 into reset mode
    UCB0CTLW0 = UCSWRST;

    // Set as I2C master, synchronous mode, SMCLK source
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;

    // Manually adjusting baud rate to 100 kHz  (1MHz / 10 = 100 kHz)
    UCB0BRW = 10;

    // Set slave address
    UCB0I2CSA = LCD_ADDRESS;

    // Release reset state
    UCB0CTLW0 &= ~UCSWRST;

    // Enable transmit interrupt
    UCB0IE |= UCTXIE0;
    //---------------- End Configure UCB0 I2C -----------

    //---------------- Configure UCB1 I2C ---------------

    // Configure P4.6 (SDA) and P4.7 (SCL) for I2C
    P4SEL0 |= BIT6 | BIT7;
    P4SEL1 &= ~(BIT6 | BIT7);

    // Put eUSCI_B0 into reset mode
    UCB1CTLW0 = UCSWRST;

    // Set as I2C master, synchronous mode, SMCLK source
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;

    // Manually adjusting baud rate to 100 kHz  (1MHz / 10 = 100 kHz)
    UCB1BRW = 10;

    // Set slave address
    UCB1I2CSA = LM92_ADDRESS;

    // Release reset state
    UCB1CTLW0 &= ~UCSWRST;

    // Enable receive interrupt
    UCB1IE |= UCRXIE1;
    //---------------- End Configure UCB0 I2C -----------
    send_I2C_data();

    __enable_interrupt();       // Enable Global Interrupts
    PM5CTL0 &= ~LOCKLPM5;       // Clear lock bit

    while (1)
    {

    }
    return 0;
}

//-------------------------------------------------------
// Interrupt Service Routines
//-------------------------------------------------------

//---------------- START ISR_TB0_SwitchColumn -----------
//-- TB0 CCR0 interrupt, read row data from column, shift roll read column right
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_SwitchColumn(void)
{
    if (state == UNLOCKING)
    { // If in unlocking state
        if (mili_seconds_surpassed >= 5000)
        {
            state = LOCKED; // Set to lock state
            index = 0; // Reset position on input_code
            mili_seconds_surpassed = 0; // Reset timeout counter
            send_I2C_data();
        }
        else
        {
            mili_seconds_surpassed++;
        }
    }

    switch (column)
    {
        case 0:
            P3OUT = 0b00001000; //Enable reading far left column
            break;
        case 1:
            P3OUT = 0b00000100; // Enable reading center left column
            break;
        case 2:
            P3OUT = 0b00000010; // Enable reading center right column
            break;
        default: // Case 3
            P3OUT = 0b00000001; // Enable reading far right column
    }

    if (P3IN > 15){  // If a button is being pressed

        if (state == LOCKED)
        {
            state = UNLOCKING;
        }

        if (P3IN & BIT4)
        {    // If bit 4 is receiving input, we're at row 3, so on and so forth
            row = 3;
        }
        else if (P3IN & BIT5)
        {
            row = 2;
        }
        else if (P3IN & BIT6)
        {
            row = 1;
        }
        else if (P3IN & BIT7)
        {
            row = 0;
        }

        key_pressed = keyPad[row][column];

        switch (state)
        {
            case UNLOCKING: // If unlocking, we populate our input code with each pressed key
                input_code[index] = key_pressed; // Set the input code at index to what is pressed.
                if (index >= 3)
                { // If we've entered all four digits of input code:
                    index = 0;
                    state = UNLOCKED; // Initially set state to free
                    mili_seconds_surpassed = 0; // Stop lockout counter
                    int i;
                    for (i = 0; i < 4; i++)
                    { // Iterate through the pass_code and input_code
                        if (input_code[i] != pass_code[i])
                        { // If an element in pass_code and input_code doesn't match
                            state = LOCKED;                   // Set state back to locked.
                            break;
                        }
                    }
                    send_I2C_data();
                }
                else
                {
                    index++; // Shift to next index of input code
                }

                break;
            default:     // If unlocked, we check the individual key press.
                switch (key_pressed)
                {
                    case ('A'):
                        state = HEAT;
                        if (state != sub_state)
                        {
                            timer = 0;
                        }
                        sub_state = state;
                        tx_buffer[0] = 0;
                        break;
                    case ('B'):
                        state = COOL;
                        if (state != sub_state)
                        {
                            timer = 0;
                        }
                        sub_state = state;
                        tx_buffer[0] = 1;
                        break;
                    case ('C'):
                        state = MATCH;
                        if (state != sub_state)
                        {
                            timer = 0;
                        }
                        sub_state = state;
                        tx_buffer[0] = 3;
                        break;
                    case ('D'):
                        state = OFF;
                        if (state != sub_state)
                        {
                            timer = 0;
                        }
                        sub_state = state;
                        tx_buffer[0] = 2;
                        break;
                    case ('0'):
                         state = SET_WINDOW;
                        break;
                    case ('1'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 1;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 1;
                            state = sub_state;
                        }
                        break;
                    case ('2'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 2;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 2;
                            state = sub_state;
                        }
                        break;
                    case ('3'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 3;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 3;
                            state = sub_state;
                        }
                        break;
                    case ('4'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 4;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 4;
                            state = sub_state;
                        }
                        break;
                    case ('5'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 5;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 5;
                            state = sub_state;
                        }
                        break;
                    case ('6'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 6;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 6;
                            state = sub_state;
                        }
                        break;
                    case ('7'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 7;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 7;
                            state = sub_state;
                        }
                        break;
                    case ('8'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 8;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 8;
                            state = sub_state;
                        }
                        break;
                    case ('9'):
                        if (state == SET_WINDOW)
                        {
                            window_size = 9;
                            state = sub_state;
                            tx_buffer[5] = window_size;
                        }
                        else if (state == SET_TEMP)
                        {
                            temp_match = 9;
                            state = sub_state;
                        }
                        break;
                    case ('*'):
                        state = SET_TEMP;
                        break;
                    case ('#'):
                        state = MATCH_SET;
                        if (state != sub_state)
                        {
                            timer = 0;
                        }
                        sub_state = state;
                        tx_buffer[0] = 4;
                        break;
                    default:
                        break;
                }
                break;
        }
        while (P3IN > 15)
        {

        } // Wait until button is released
        send_I2C_data();
    }
    if (P3IN < 16)
    { // Checks if pins 7 - 4 are on, that means a button is being held down; don't shift columns
        if (++column >= 4)
        {
            column = 0;
        } // Add one to column, if it's 4 reset back to 0.
    }
    TB0CCTL0 &= ~TBIFG;
}
//---------------- End ISR_TB0_SwitchColumn -------------

//---------------- START ISR_TB1_Heartbeat --------------
// Heartbeat function
#pragma vector = TIMER1_B0_VECTOR
__interrupt void ISR_TB1_Heartbeat(void)
{
    P1OUT ^= BIT0;               //Toggle P1.0(LED1)
    P6OUT ^= BIT6;               //Toggle P6.6(LED2)
    if (heat == 1)
    {
        pattern = (1 << (step_pattern_heat + 1)) - 1;
        step_pattern_heat = (step_pattern_heat + 1) % 8;
    }
    if (cool == 1)
    {
        pattern = 0xFF << (7 - step_pattern_cool);
        step_pattern_cool = (step_pattern_cool + 1) % 8;
    }
    timer++;
    TB1CCTL0 &= ~CCIFG;          //clear CCR0 flag
}
//---------------- END ISR_TB1_Heartbeat ----------------

//---------------- START ISR_TB2_CCR0 -------------------
// Sample LM19 Temperature
#pragma vector = TIMER2_B0_VECTOR
__interrupt void ISR_TB2_CCR0(void)
{
    if (state != LOCKED)
    {
        start_ADC_conversion();   // LM19 analog read
        get_lm92_i2c();           // Start LM92 I2C read
        peltier_control();        // Initiate Peltier Control
    }
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    if (UCB0IV == 0x18)
    { // TXIFG0 triggered
        if (tx_index < TX_BYTES)
        {
            UCB0TXBUF = tx_buffer[tx_index++]; // Load next byte
        }
        else
        {
            UCB0CTLW0 |= UCTXSTP; // Send stop condition
            UCB0IE &= ~UCTXIE0;   // Disable TX interrupt after completion
            tx_index = 0;
        }
    }
}

#pragma vector = USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void)
{
    switch (__even_in_range(UCB1IV, USCI_I2C_UCBIT9IFG))
    {
        case 0x16:
            lm92_data[lm92_byte_count++] = UCB1RXBUF;
            if (lm92_byte_count == 1)
            {
                UCB1CTLW0 |= UCTXSTP;  // Send stop after 2nd byte
            }
            else if (lm92_byte_count == 2)
            {
                // Convert to temperature
                int raw_temp = ((lm92_data[0] << 8) | lm92_data[1]) >> 3;
                float temp_C = raw_temp * 0.0625;  // LM92 resolution
                lm92_samples[lm92_sample_index++] = (unsigned int)(temp_C * 10); // Store in tenths of a degree
                if (lm92_sample_index >= window_size)
                {
                    lm92_samples_collected = 1;
                    lm92_sample_index = 0;
                }
                if (lm92_samples_collected == 1)
                {
                    // Average and store in tx_buffer
                    int i = 0;
                    unsigned int sum = 0;
                    for (i = 0; i < window_size; i++) sum += lm92_samples[i];
                    unsigned int avg = sum / window_size;
                    lm92_temperature_integer = avg / 10;
                    lm92_temperature_decimal = avg % 10;
                    tx_buffer[3] = lm92_temperature_integer;     // Integer part
                    tx_buffer[4] = lm92_temperature_decimal;     // Decimal part
                }
                UCB1IE &= ~UCRXIE1;  // Disable RX interrupt
            }
            break;
        default:
            break;
    }
}

#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void)
{
    // Store the ADC result in the array
    adc_results[lm19_sample_index] = ADCMEM0;
    lm19_sample_index++;

    // If we have collected window_size, calculate the average and reset the counter
    if (lm19_samples_collected == 1)
        {
            get_temperature();
        }
    if (lm19_sample_index >= window_size)
    {
        lm19_samples_collected = 1;
        lm19_sample_index = 0;
    }
}
