#include QMK_KEYBOARD_H
#include <print.h>

// Include YOUR secret file here... from outside the code tree
// #include "~/.secrets.h"
#include "./secrets.h"

enum uno_keycode
{
  UNO = SAFE_RANGE,
};

enum interations
{
  SINGLE_TAP,
  DOUBLE_TAP,
  LONG_PRESS, // > LONG_PRESS_LENGTH
  RESET_PRESS, // > RESET_LENGTH
  TIMEOUT,
};

uint16_t pressTimer = 0xFFFF;
uint8_t secret_index = 0;
uint8_t secret_stage = 0;
uint8_t pc_index = 0;
uint8_t current_code = 0;
uint8_t good_code = 0;
bool locked = 1;
bool pressed = 0;
bool double_tap = false;
bool tap_wait = false;
bool interaction_pending = false;

#define LONG_PRESS_LENGTH 250
#define RESET_LENGTH 3000
#define TAP_TERM 175

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
      [0] = LAYOUT(
            UNO
          )
};

void reset_device(void) {

    locked = 1;
    pc_index = 0;
    current_code = 0;
    good_code = 0;
    secret_index = 0;
    secret_stage = 0;
    rgblight_sethsv_noeeprom(0, 255, 0);
    wait_ms(200);
    rgblight_sethsv_noeeprom(0, 255, 255);
    wait_ms(100);
    rgblight_sethsv_noeeprom(0, 255, 0);
    wait_ms(200);
    rgblight_sethsv_noeeprom(0, 255, 255);
}


void interaction_handler(uint8_t interaction_type) {
    if(interaction_type == TIMEOUT || interaction_type == RESET_PRESS) {
	reset_device();
	return;
    }
    if(locked) {
	if(interaction_type == LONG_PRESS) {
	    rgblight_sethsv_noeeprom(0, 255, 255);
	    if(current_code == passcode[pc_index])
		good_code++;
	    if(pc_index == 3) {
		if(good_code == 4) {
		    locked = 0;
		    rgblight_sethsv_noeeprom(82,255,255);
		    wait_ms(100);
		    rgblight_sethsv_noeeprom(0,0,0);
		    wait_ms(200);
		    rgblight_sethsv_noeeprom(82,255,255);
		    wait_ms(100);
		    rgblight_sethsv_noeeprom(0,0,0);
		    wait_ms(300);
		    rgblight_sethsv_noeeprom(pw_colors[secret_index],255,128);
		}
		else
		{
		    rgblight_sethsv_noeeprom(0,0,0);
		    wait_ms(200);
		    rgblight_sethsv_noeeprom(0,255,255);
		    wait_ms(100);
		    rgblight_sethsv_noeeprom(0,0,0);
		    wait_ms(200);
		    rgblight_sethsv_noeeprom(0,255,255);
		    wait_ms(100);
		    rgblight_sethsv_noeeprom(0,0,0);
		    wait_ms(200);
		    reset_device();
		}
	    }
	    else {
		pc_index++;
		current_code = 0;
	    }
	}
	else {
	    rgblight_sethsv_noeeprom(0, 255, 255);
	    current_code++;
	    if(current_code > 9)
		current_code = 0;
	    dprintf("Current Index: %d - Code: %d\n", pc_index,current_code);
	}
	return;
    } // End locked / code handling

    // If unlocked
    switch(secret_stage) {
	case 0:
            // Selecting password color
	    if(interaction_type == LONG_PRESS) {
		// Select!
		secret_stage = 1;
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 128);
		wait_ms(100);
		rgblight_sethsv_noeeprom(0,0,0);
		wait_ms(200);
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 255);
	    }
	    else {
		// Cycle
	      	secret_index++;
	        if(secret_index == secret_count)
		    secret_index = 0;
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 128);
	    }
	    break;

	case 1:
	    if(interaction_type == LONG_PRESS) {
		// Back to selecting
		secret_stage = 0;
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 128);
		wait_ms(100);
		rgblight_sethsv_noeeprom(0,0,0);
		wait_ms(300);
		secret_index++;
		if(secret_index == secret_count)
		    secret_index=0;
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 128);
	    }
	    else {
		// Send Selected password or username
		rgblight_sethsv_noeeprom(pw_colors[secret_index], 255, 255);
		if(interaction_type == DOUBLE_TAP)
		    send_string(pw_passwords[secret_index]);
		else
		    send_string(pw_usernames[secret_index]);
	    }
	    break;
    }

}


void matrix_scan_user(void) {

    uint16_t timeElapsed = timer_elapsed(pressTimer);

    // If 5 minutes have elapsed with no new presses... lock it up!
    if(timeElapsed > 300000) {
	interaction_handler(TIMEOUT);
    }
    if(pressed) {
	// If pressed for more than 3 seconds at any point... lock it up!
	if(timeElapsed > RESET_LENGTH) {
	    interaction_handler(RESET_PRESS);
	}
        if(timeElapsed > LONG_PRESS_LENGTH) {
	    if(!interaction_pending) {
		interaction_pending = true;
	        interaction_handler(LONG_PRESS);
	    }
    	    //dprintf("MSU: Longpres\n");
        }
    }

    if(tap_wait && timeElapsed > TAP_TERM) {
	tap_wait = false;
	interaction_handler(SINGLE_TAP);
    }

}


bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
	if(timer_elapsed(pressTimer) < TAP_TERM) {
	    dprint("DT1\n");
	    double_tap = true;
	    tap_wait = false;
	}
	else
	{
	    double_tap = false;
	}
	pressTimer = timer_read();
	pressed = 1;
        rgblight_sethsv_noeeprom(0,0,0);
    } else {
	pressed = 0;
	uint16_t timeElapsed = timer_elapsed(pressTimer);
	if(timeElapsed < LONG_PRESS_LENGTH) {
	    if(locked)
		interaction_handler(SINGLE_TAP);
	    else {
		if(double_tap)
		    interaction_handler(DOUBLE_TAP);
		else
		    tap_wait = true;
	    }
	}
    }
    interaction_pending = false;
    return false;
}

void keyboard_post_init_user(void) {
    //debug_enable=true;
    rgblight_enable_noeeprom();
    rgblight_sethsv_noeeprom(0, 255, 255);
    rgblight_mode(1);
}
