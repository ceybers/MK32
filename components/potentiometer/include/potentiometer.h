/*
 * potentiometer.h
 *
 *  Created on: 28 Apr 2022
 *      Author: cae
 * 
 * Based off Gal Zaidenstein's r_encoder plugin
 */

#ifndef POTENTIOMETER_H_
#define POTENTIOMETER_H_

#ifdef __cplusplus
extern "C" {
#endif

void potentiometer_setup(void); // Called on init
void potentiometer_reset(void); // Called when pressing Key #3
void potentiometer_update(void); // Called every tick
void potentiometer_command(int8_t delta);
void potentiometer_get_sample(void);

#ifdef __cplusplus
}
#endif

#endif /* POTENTIOMETER_H_ */