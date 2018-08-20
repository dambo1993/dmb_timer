/*
 * dmb_timer.h
 *
 *  Created on: 02.04.2018
 *      Author: Przemek
 */

#ifndef DMB_TIMER_DMB_TIMER_H_
#define DMB_TIMER_DMB_TIMER_H_

#include <inttypes.h>
#include <libs_config/dmb_timer_settings.h>

//! typ callbacka wywolywanego przez timer - funkcje musza byc z nim zgodne
//! jako parametr przyjmowany jest id aktualnego timera
typedef void (*dmb_timer_callback)(uint8_t);

//! enum okreslajacy typ timera/taska
typedef enum dmb_timer_type
{
	dmb_timer_type_not_initialized, // nie byl zainicjalizowany, uzywany wewnetrznie
	dmb_timer_type_single,			// pojedyncze odpalenie callbacka za zadany okres czasu
	dmb_timer_type_continous,		// ciagla praca co zadany okres czasu
	dmb_timer_type_pre_delay,		// ciagla praca co zadany okres czasu z opoznieniem startowym
}dmb_timer_type;


int8_t dmb_timer_add_task_with_predelay(uint16_t predelay, uint16_t interval, dmb_timer_callback callback );
int8_t dmb_timer_add_task(uint16_t interval, dmb_timer_type type, dmb_timer_callback callback );

void dmb_timer_continue_task(int8_t timer_number);
void dmb_timer_disable_task(uint8_t id);
void dmb_timer_events();
void dmb_timer_init(uint8_t interval );
void dmb_timer_pause_task(int8_t timer_number);
void dmb_timer_reset_ticks();
void dmb_timer_tick();


#endif /* DMB_TIMER_DMB_TIMER_H_ */
