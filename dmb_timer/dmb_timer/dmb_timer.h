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

//! typ callbacka wywolywanego podczas przekroczenia czasu wykonywania
typedef void (*dmb_timer_execution_time_problem_callback)(void);

//! enum okreslajacy typ timera/taska
typedef enum
{
	dmb_timer_type_not_initialized, // nie byl zainicjalizowany, uzywany wewnetrznie
	dmb_timer_type_single,			// pojedyncze odpalenie callbacka za zadany okres czasu
	dmb_timer_type_continous,		// ciagla praca co zadany okres czasu
	dmb_timer_type_pre_delay,		// ciagla praca co zadany okres czasu z opoznieniem startowym
}dmb_timer_type_e;

/*
 * Inicjalizacja biblioteki
 *
 * @param interval czas co jaki bedzie wywolywane tykniecie timera. Na jego podstawie przeliczane sa czasy dla taskow.
 */
void dmb_timer_init(uint8_t interval );

/*
 * Rejestracja funkcji wykonywanej gdy czas wykonywania zadan przekracza interwal czasu.
 *
 * @param funkcja do wywolania
 */
void dmb_timer_register_execution_time_problem(dmb_timer_execution_time_problem_callback ptr);

/*
 * Funkcja sprawdzajaca eventy - do umieszczenia w petli glownej aplikacji.
 */
void dmb_timer_events(void);

/*
 * Funkcja "tykajaca" - zapewnic jej cykliczne wywolywanie. Zapala ona tylko flage do funkcji sprawdzenia eventow.
 */
void dmb_timer_tick(void);

/*
 * Dodanie taska do timera. Funkcja zwraca jego id.
 */
int8_t dmb_timer_add_task(uint16_t interval, dmb_timer_type_e type, dmb_timer_callback callback );

/*
 * Dodanie taska cyklicznego do timera z opoznieniem startowym. Zakladam, ze jest to zawsze timer continous.
 * Dla singla wystarczy zawsze tylko dodac delay.
 */
int8_t dmb_timer_add_task_with_predelay(uint16_t predelay, uint16_t interval, dmb_timer_callback callback );

/*
 * Pauzowanie okreslonego timera/taska. Mozemy go potem wznowic.
 */
void dmb_timer_pause_task(int8_t timer_number);

/*
 * Kontynuowanie okreslonego timera/taska.
 */
void dmb_timer_continue_task(int8_t timer_number);

/*
 * Wylaczenie timera o danym id.
 */
void dmb_timer_disable_task(uint8_t id);

/*
 * Kasowanie aktualnie zliczonych tikow - mozna je wykonac przed petla glowna gdzie jest sprawdzany event.
 * Wtedy zaczynamy odmierzanie czasu w tym samym momencie.
 */
void dmb_timer_reset_ticks(void);

#endif /* DMB_TIMER_DMB_TIMER_H_ */
