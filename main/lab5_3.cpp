#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/uart.h"


// Define the GPIO pin number
#define GPIO_INPUT_PIN GPIO_NUM_1  // GPIO2 (can be any available GPIO pin)
#define POLLING_PERIOD 10000
#define UNIT_DURATION 10  // Unit duration in milliseconds

// Function prototypes
void interpret(uint16_t morse_code);
char lookup_character(uint16_t morse_code);

// Global variables
static bool listening = false;
static uint16_t morse_code = 0;

// Morse Code Lookup Table
char lookup_character(uint16_t morse_code) {
    switch (morse_code) {
	case 0b0110: return 'A';
	case 0b10010101: return 'B';
	case 0b10011001: return 'C';
	case 0b100101: return 'D';
        case 0b01: return 'E';  // Example for dot
	case 0b01011001: return 'F';
	case 0b101001: return 'G';
	case 0b01010101: return 'H';
	case 0b0101: return 'I';
	case 0b01101010: return 'J';
	case 0b100110: return 'K';
	case 0b01100101: return 'L';
	case 0b1010: return 'M';
	case 0b1001: return 'N';
	case 0b101010: return 'O';
	case 0b01101001: return 'P';
	case 0b10100110: return 'Q';
	case 0b011001: return 'R';
	case 0b010101: return 'S';
        case 0b10: return 'T';  // Example for dash
	case 0b010110: return 'U';
	case 0b01010110: return 'V';
	case 0b011010: return 'W';
	case 0b10010110: return 'X';
	case 0b10011010: return 'Y';
	case 0b10100101: return 'Z';
	case 0b0110101010: return '1';
	case 0b0101101010: return '2';
	case 0b0101011010: return '3';
	case 0b0101010110: return '4';
	case 0b0101010101: return '5';
	case 0b1001010101: return '6';
	case 0b1010010101: return '7';
	case 0b1010100101: return '8';
	case 0b1010101001: return '9';
	case 0b1010101010: return '0';
        // Add all other Morse code mappings here
        default: return '?';    // Unknown character
    }
}

void interpret(uint16_t morse_code) {
    char decoded_char = lookup_character(morse_code);
    printf("%c", decoded_char);
}


void morse_code_handler(void* arg) {
    static bool signal_high = false;
    static uint64_t last_signal_change_time = 0;
	static uint64_t start_time = 0;
    

    uint64_t current_time = esp_timer_get_time() / 1000; // Get current time in milliseconds
    bool current_signal = gpio_get_level(GPIO_INPUT_PIN);

    if (current_signal != signal_high) {  // State change detected
        signal_high = current_signal;
        last_signal_change_time = current_time;

        if (signal_high) {  // LOW to HIGH
            if (!listening) {
                listening = true;
		printf("now listening\n");
            } else{
		    uint64_t duration = current_time - start_time;
		    //printf("low duration: %lld \n", duration);
		    if(duration == UNIT_DURATION){//space inside letters 
						  //do nothing
		    }else if (duration == 4*UNIT_DURATION){ //space between letters
			interpret(morse_code);
			morse_code = 0;
		    }else {
			interpret(morse_code);
			morse_code = 0;
			printf(" ");
		    }
		}
	    
        } else {  // HIGH to LOW 
		    uint64_t duration = current_time - start_time;
		    //printf("high duration: %lld \n", duration);
		    if (duration <= UNIT_DURATION) {
			morse_code = (morse_code << 2) | 0b01;  // Dot
		    } else  {
			morse_code = (morse_code << 2) | 0b10;  // Dash
		    }
	}
                start_time = current_time;
    }

    // Check for timeouts (idle state)
    if (!signal_high && listening) {
        uint64_t idle_time = current_time - last_signal_change_time;
	if(idle_time > 15 * UNIT_DURATION){
		if(morse_code != 0){
			interpret(morse_code);
			morse_code = 0;
		}
		printf("\nnot listening, %lld \n", idle_time);
		listening = false;
	}
	/*
        if (idle_time >= 3 * UNIT_DURATION && idle_time < 7 * UNIT_DURATION) {
            // End of a letter
            interpret(morse_code);
            morse_code = 0;
            listening = false;
        } else if (idle_time >= 7 * UNIT_DURATION) {
            // End of a word
            if (morse_code != 0) {
                interpret(morse_code);
                morse_code = 0;
            }
            printf(" ");  // Space between words
            listening = false;
        }
	*/
    }
}

// Main task to read GPIO input and log the state
//void read_gpio_task(void *arg) {
    // Configure the GPIO pin as input
//}

extern "C" void app_main() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE; // Disable interrupts
    io_conf.mode = GPIO_MODE_INPUT;        // Set as input mode
    io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_PIN); // Select GPIO pin
    io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;    // Disable pull-down
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;         // Enable pull-up
    gpio_config(&io_conf);

    const esp_timer_create_args_t periodic_timer_args = {
	.callback = &morse_code_handler,
	.arg = NULL, 
	.name = "morse_code_timer",
	.skip_unhandled_events = false
    };

    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);

    printf("Listening for GPIO input on pin %d...\n", GPIO_INPUT_PIN);

    esp_timer_start_periodic(periodic_timer, POLLING_PERIOD);
    
    /*
    while (1) {

        // Log the GPIO state
        if (state == 1) {
            printf("GPIO %d: HIGH\n", GPIO_INPUT_PIN);
        } else {
            printf("GPIO %d: LOW\n", GPIO_INPUT_PIN);
        }

        // Delay to avoid spamming the console
        vTaskDelay(pdMS_TO_TICKS(100));  // 100 ms delay
    }
    */
}
