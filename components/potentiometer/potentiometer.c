/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "key_definitions.h"
#include "keyboard_config.h"
#include "key_definitions.h"
#include "driver/pcnt.h"
#include "hal_ble.h"
#include "esp_adc_cal.h"

#ifndef minmax
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif // minmax

static bool initialized = false;
uint8_t PastPotentiometerValue = 0; // Range 0 - 100
float smoothedStatistic = 0.0f; // Range 0 - 4095

bool isHostVolumeKnown = false;
uint8_t hostVolume = 0;
uint8_t targetVolume = 0;

uint8_t potState = 0x00;

// Set up potentiometer switch at 0%
void potentiometer_setup(void)
{	
	// No need to calibrate if we aren't converting to voltage
	// Potentiometer Switch (click at 0%)
#ifdef POTENTIOMETER_PIN
	gpio_pad_select_gpio(POTENTIOMETER_PIN);
	gpio_set_direction(POTENTIOMETER_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(POTENTIOMETER_PIN, GPIO_PULLDOWN_ONLY);
#endif
}

// How to process potentiometer activity
void potentiometer_reset(void) {
	ESP_LOGI("Potentiometer", "Resetting to zero");
	potState = 0x01;
	hostVolume = 100;
	targetVolume = 0;
	//PastPotentiometerValue = 0;
	//smoothedStatistic = 0.0f;
}

void potentiometer_command(int8_t delta)
{
	//ESP_LOGI("Potentiometer", "command %d", delta);
	uint8_t media_state[2] = {0};

	if (delta > 2) {
		SET_BIT(media_state[0], 6); // VOL_UP
	} else if (delta < -2) {
		SET_BIT(media_state[0], 7); // VOL_DN
	} else {
		return;	
	}

	// Don't change the volume while we are debugging
	// xQueueSend(media_q, (void *)&media_state, (TickType_t)0);

	vTaskDelay(5 / portTICK_PERIOD_MS);
}

uint8_t smoothedStatisticToVolume() {
	const float cutoffLower  =  204.8f; // 4096 * 0.05
	const float cutoffHigher = 3891.2f; // 4096 * 0.95
	const float divisor = 40.96f; // 4096 / 100
	float a = min(max(smoothedStatistic, cutoffLower), cutoffHigher) / divisor; // New range is 5.0-95.0
	return (((int)(((a - 5.0f) / (95.0f - 5.0f)) * 50.0f) * 2) - 100) * -1;	
}

// Get sample from ADC and apply low cut-off and exponential smoothing
void potentiometer_get_sample(void)
{
	const float smoothingFactor = 0.1f;
	const uint16_t cutOff = 500;
	int thisSample = 0;
	uint16_t delta; // Cut-off small changes

	adc2_get_raw(ADC2_CHANNEL_3, ADC_WIDTH_BIT_DEFAULT, &thisSample);

    if (initialized == false) {
		ESP_LOGI("Potentiometer", "Initializing...");
		initialized = true;
		smoothedStatistic = thisSample;
	}

	delta = thisSample - smoothedStatistic;

	// Do not update smoothedStatistic or Volume if change is below cut-off
	if (delta > -cutOff && delta < cutOff) return;

	smoothedStatistic = smoothedStatistic + smoothingFactor * (thisSample - smoothedStatistic);
	//ESP_LOGI("Potentiometer", "SmStat: %f", smoothedStatistic);
}

void potentiometer_update(void) {
	potentiometer_get_sample();

	if (potState == 0x02) {
		uint8_t newVolume = smoothedStatisticToVolume();
		if (newVolume != targetVolume) {
			targetVolume = newVolume;
			//ESP_LOGI("Potentiometer", "Target volume changed to: %d", targetVolume);
		}
	}
	
	uint8_t media_state[2] = {0};
	if (hostVolume > targetVolume) {
		hostVolume = hostVolume - 2;
		SET_BIT(media_state[0], 7); // VOL_DN
	} else if (hostVolume < targetVolume) {
		hostVolume = hostVolume + 2;
		SET_BIT(media_state[0], 6); // VOL_UP
	} else {
		if (potState == 0x01) {
			potState = 0x02;
			//ESP_LOGI("Potentiometer", "Host Volume should now be 0");
			targetVolume = smoothedStatisticToVolume();
		} else if (potState == 0x02) {
			//ESP_LOGI("Potentiometer", "Host Volume should now be == targetVolume");
		}
		return;
	}
	xQueueSend(media_q, (void *)&media_state, (TickType_t)0);
	vTaskDelay(5 / portTICK_PERIOD_MS);	
}