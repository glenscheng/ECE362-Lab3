/**
  ******************************************************************************
  * @file    main.c
  * @author  Weili An
  * @version V1.0
  * @date    Oct 24, 2022
  * @brief   ECE 362 Lab 7 template
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include <stdint.h>

// Global data structure
char* login          = "xyz"; // Replace with your login.
char disp[9]         = "Hello...";
uint8_t col          = 0;
uint8_t mode         = 'A';
uint8_t thrust       = 0;
int16_t fuel         = 800;
int16_t alt          = 4500;
int16_t velo         = 0;

// Make them visible to autotest.o
extern char* login;
// Keymap is in `font.S` to match up what autotester expected
extern char keymap;
extern char disp[9];
extern uint8_t col;
extern uint8_t mode;
extern uint8_t thrust;
extern int16_t fuel;
extern int16_t alt;
extern int16_t velo;

char* keymap_arr = &keymap;

// Font array in assembly file
// as I am too lazy to convert it into C array
extern uint8_t font[];

// The functions we should implement
void enable_ports();
void setup_tim6();
void show_char(int n, char c);
void drive_column(int c);
int read_rows();
char rows_to_key(int rows);
void handle_key(char key);
void setup_tim7();
void write_display();
void update_variables();
void setup_tim14();

// Auotest functions
extern void check_wiring();
extern void autotest();
extern void fill_alpha();

int main(void) {
    // check_wiring();
    // autotest();
    // fill_alpha();
    enable_ports();
    setup_tim6();
    setup_tim7();
    setup_tim14();

    for(;;) {
        asm("wfi");
    }
}

/**
 * @brief Enable the ports and configure pins as described
 *        in lab handout
 * 
 */
void enable_ports(){
    RCC->AHBENR |= (RCC_AHBENR_GPIOBEN | RCC_AHBENR_GPIOCEN); // enable clocks
    // port B
    GPIOB->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5 |
            GPIO_MODER_MODER6 | GPIO_MODER_MODER7 | GPIO_MODER_MODER8 | GPIO_MODER_MODER9 | GPIO_MODER_MODER10); // clear for outputs
    GPIOB->MODER |= (GPIO_MODER_MODER0_0 | GPIO_MODER_MODER1_0 | GPIO_MODER_MODER2_0 | GPIO_MODER_MODER3_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 |
            GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0 | GPIO_MODER_MODER9_0 | GPIO_MODER_MODER10_0); // set outputs
    // port C
    GPIOC->MODER &= ~(GPIO_MODER_MODER4 | GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7 | GPIO_MODER_MODER8); // clear for outputs
    GPIOC->MODER |= (GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0 | GPIO_MODER_MODER7_0 | GPIO_MODER_MODER8_0); // set outputs
    GPIOC->MODER &= ~(GPIO_MODER_MODER0 | GPIO_MODER_MODER1 | GPIO_MODER_MODER2 | GPIO_MODER_MODER3); // set inputs
    GPIOC->PUPDR &= ~(GPIO_PUPDR_PUPDR0 | GPIO_PUPDR_PUPDR1 | GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3); // clear pins for PD resistors
    GPIOC->PUPDR |= (GPIO_PUPDR_PUPDR0_1 | GPIO_PUPDR_PUPDR1_1 | GPIO_PUPDR_PUPDR2_1 | GPIO_PUPDR_PUPDR3_1); // enable PD resistors on inputs
}

//-------------------------------
// Timer 6 ISR goes here
//-------------------------------
void TIM6_DAC_IRQHandler() {
       TIM6->SR &= ~TIM_SR_UIF; // acknowledge interrupt
       // toggle PC8 output
       int32_t bit = (GPIOC->ODR & GPIO_ODR_8) >> 8;
       if (bit == 0) {
           GPIOC->BSRR |= 1 << 8; // toggle on
       } else {
           GPIOC->BRR |= 1 << 8; // toggle off
       }
}

/**
 * @brief Set up timer 6 as described in handout
 * 
 */
void setup_tim6() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN; // enable clock
    TIM6->PSC = 48000 - 1; // set prescale value to 48000
    TIM6->ARR = 500 - 1; // set auto-reload register to 500
    TIM6->DIER |= TIM_DIER_UIE; // DMA/interrupt enable register; enables interrupt to occur when counter resets from 0
    TIM6->CR1 |= TIM_CR1_CEN; // start counter
    NVIC->ISER[0] |= 1 << TIM6_DAC_IRQn;
}

/**
 * @brief Show a character `c` on column `n`
 *        of the segment LED display
 * 
 * @param n 
 * @param c 
 */
void show_char(int n, char c) {
    if (n < 0 || n > 7) { // check range for digits
        return;
    }
    GPIOB->ODR &= ~(0x00000FFF); // clearing outputs
    int32_t bits;
    if (n == 0) {
        bits = 0x00000000;
    } else if (n == 1) {
        bits = 0x00000100;
    } else if (n == 2) {
        bits = 0x00000200;
    } else if (n == 3) {
        bits = 0x00000300;
    } else if (n == 4) {
        bits = 0x00000400;
    } else if (n == 5) {
        bits = 0x00000500;
    } else if (n == 6) {
        bits = 0x00000600;
    } else {
        bits = 0x00000700;
    }
    GPIOB->ODR |= (bits | font[c]); // setting selected digit and pattern to display
}

/**
 * @brief Drive the column pins of the keypad
 *        First clear the keypad column output
 *        Then drive the column represented by `c`
 * 
 * @param c 
 */
void drive_column(int c) {
    int32_t bits;
    int column = c & 0b11; // least 2 bits of c is the column we are driving
    GPIOC->ODR &= ~(0x00F0); // clearing outputs (PC4-7)
    GPIOC->BSRR |= 1 << (column + 4); // drive PC4-7 for column 0-3 respectively
}

/**
 * @brief Read the rows value of the keypad
 * 
 * @return int 
 */
int read_rows() {
    return GPIOC->IDR &= 0xF;
}

/**
 * @brief Convert the pressed key to character
 *        Use the rows value and the current `col`
 *        being scanning to compute an offset into
 *        the character map array
 * 
 * @param rows 
 * @return char 
 */
char rows_to_key(int rows) {
    int column = col & 0b11;
    int row;
    if ((rows & 0b0001) == 0b0001) {
        row = 0;
    } else if ((rows & 0b0010) == 0b0010) {
        row = 1;
    } else if ((rows & 0b0100) == 0b0100) {
        row = 2;
    } else {
        row = 3;
    }
    int offset = column * 4 + row;
    return keymap_arr[offset];
}

/**
 * @brief Handle key pressed in the game
 * 
 * @param key 
 */
void handle_key(char key) {
    if (key == 'A' || key == 'B' || key == 'D') { // set mode
        mode = key;
        return;
    }
    if (key == '0' || key == '1' || key == '2' || key == '3' || key == '4' || key == '5' ||
            key == '6' || key == '7' || key == '8' || key == '9') { // set thrust
        thrust = key - '0';
    }
}

//-------------------------------
// Timer 7 ISR goes here
//-------------------------------
void TIM7_IRQHandler() {
    TIM7->SR &= ~TIM_SR_UIF; // acknowledge interrupt
    int row_values = read_rows(); // read the row values
    char key;
    if (row_values != 0) {
        key = rows_to_key(row_values); // convert rows to keys
        handle_key(key); // handle the key by changing the mode or thrust
    }
    show_char(col, disp[col]); // display char at column `col` (put into GPIOB->ODR)
    col++;
    if (col > 7) {
        col = 0;
    }
    drive_column(col); // drive the next column
}

/**
 * @brief Setup timer 7 as described in lab handout
 * 
 */
void setup_tim7() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM7EN; // turn on clock
    TIM7->PSC = 4800 - 1; // set prescale value to 4800
    TIM7->ARR = 10 - 1; // set auto-reload register to 10
    TIM7->DIER |= TIM_DIER_UIE; // DMA/interrupt enable register; enables interrupt to occur when counter resets from 0
    TIM7->CR1 |= TIM_CR1_CEN; // start counter
    NVIC->ISER[0] |= 1 << TIM7_IRQn;
}

/**
 * @brief Write the display based on game's mode
 * 
 */
void write_display() { // change what is in the
    if (mode == 'C') {
        snprintf(disp, sizeof(disp), "Crashed");
    } else if (mode == 'L') {
        snprintf(disp, sizeof(disp), "Landed ");
    } else if (mode == 'A') {
        snprintf(disp, sizeof(disp), "ALt%5d", alt);
    } else if (mode == 'B') {
        snprintf(disp, sizeof(disp), "FUEL %3d", fuel);
    } else if (mode == 'D') {
        snprintf(disp, sizeof(disp), "Spd %4d", velo);
    }
}

/**
 * @brief Game logic
 * 
 */
void update_variables() {
    fuel -= thrust;
    if (fuel <= 0) { // out of fuel
        thrust = 0;
        fuel = 0;
    }
    alt += velo;
    if (alt <= 0) { // reached surface
        if ((-1 * velo) < 10) {
            mode = 'L'; // landed!
        } else {
            mode = 'C'; // crashed :(
        }
        return;
    }
    velo += thrust - 5;
}

//-------------------------------
// Timer 14 ISR goes here
//-------------------------------
void TIM14_IRQHandler() {
    TIM14->SR &= ~TIM_SR_UIF; // acknowledge interrupt
    update_variables();
    write_display();
}

/**
 * @brief Setup timer 14 as described in lab
 *        handout
 * 
 */
void setup_tim14() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM14EN; // turn on clock
    TIM14->PSC = 48000 - 1; // set prescale value to 48000
    TIM14->ARR = 500 - 1; // set auto-reload register to 500
    TIM14->DIER |= TIM_DIER_UIE; // DMA/interrupt enable register; enables interrupt to occur when counter resets from 0
    TIM14->CR1 |= TIM_CR1_CEN; // start counter
    NVIC->ISER[0] |= 1 << TIM14_IRQn;
}
