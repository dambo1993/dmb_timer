/*
 * dmb_timer.c
 *
 *  Created on: 02.04.2018
 *      Author: Przemek
 */

#include "../../../DMB_TIMER/dmb_timer/dmb_timer/dmb_timer.h"

#include <inttypes.h>
#include <libs_config/dmb_timer_settings.h>

static uint8_t _dmb_timer_actual_enabled_counter = 0; // ile timerow jest aktualnie aktywnych

typedef enum {
	dmb_timer_state_off,		// wylaczony timer
	dmb_timer_state_on,			// wlaczony timer
	dmb_timer_state_to_add,		// timer do dodania - jest to dodatkowy stan, poniewaz gdybysmy dodawali nowe taski z poziomu taskow
								// bez tego - moglyby miec one niedeterministyczne czasy uruchomienia
	dmb_timer_state_pause,		// timer zastopowany
	dmb_timer_state_to_off,		// timer do zatrzymania
}dmb_timer_state_e;

/*
 * Wewnetrzna struktura opisujaca pojedynczy task
 */
typedef struct _dmb_timer_task
{
	dmb_timer_state_e	state; 			// czy timer jest wlaczony, czy nie
	dmb_timer_callback 	callback; 		// wskaznik na funkcje do wykonania
	uint8_t 			type;			// continous/single
	uint16_t 			interval;		// interwal wystapienia/czas za ile wystapi w przypadku pojedynczego uruchomienia
	uint16_t			counter;		// aktualna wartosc licznika dla timera
	uint16_t			pre_counter;	// aktualna wartosc "przedlicznika" dla timera - opoznienie od wywolania do uruchomienia cyklicznego wywolywania
}_dmb_timer_task;


//! glowny licznik tykniec
static volatile uint8_t _dmb_timer_tick_counter = 0;

//! ilosc timerow do dodania:
static volatile uint8_t _dmb_timers_to_add_counter = 0;

//! glowna tablica zawierajaca taski do wykonania
static _dmb_timer_task tasks_array[DMB_TIMER_TASKS_NUMBER];

//! licznik timerow do wylaczenia
static uint8_t timers_to_off_counter = 0;

/*
 * Okreslenie w ms co ile wystepuje Tick timera.
 */
static uint8_t _dmb_timer_tick_interval;


/*
 * Inicjalizacja biblioteki
 *
 * @param interval czas co jaki bedzie wywolywane tykniecie timera. Na jego podstawie przeliczane sa czasy dla taskow.
 */
void dmb_timer_init( uint8_t interval )
{
	_dmb_timer_tick_interval = interval;
}

/*
 * Funkcja "tykajaca" - zapewnic jej cykliczne wywolywanie. Zapala ona tylko flage do funkcji sprawdzenia eventow.
 */
void dmb_timer_tick()
{
	_dmb_timer_tick_counter++;
}

/*
 * Funkcja sprawdzajaca eventy - do umieszczenia w petli glownej aplikacji.
 */
void dmb_timer_events()
{
	// tutaj mozemy dac "if" lub "while" - jaka jest roznica pomiedzy nimi
	// jesli jakas funkcja zajmie za duzo czasu - wtedy w przypadku "if-a" czasy nam sie przesuna
	// aczkolwiek w sumie nie powinnismy doprowadzic do takiej sytuacji, ze funkcje sie za dlugo wykonuja
	// jednak wersja z "while" bedzie bardziej uniwersalna - wykona wszystkie pominiete "ticki"
	while( _dmb_timer_tick_counter )
	{
		_dmb_timer_tick_counter--; // -- -> bo jest to uniwersalne rozwiazanie dla while i ifa

		uint8_t sprawdzone_timery = 0;

		for( uint8_t i = 0; i < DMB_TIMER_TASKS_NUMBER && (sprawdzone_timery != _dmb_timer_actual_enabled_counter); i++ )
		{
			if( tasks_array[i].state == dmb_timer_state_on )
			{
				if(tasks_array[i].type == dmb_timer_type_pre_delay)
				{
					tasks_array[i].pre_counter--;

					// kiedy precounter dojezdza do zera - zmieniamy timer na zwykly
					if( tasks_array[i].pre_counter == 0 )
					{
						tasks_array[i].state = dmb_timer_state_to_add;
						tasks_array[i].type = dmb_timer_type_continous;
						_dmb_timers_to_add_counter++;
						_dmb_timer_actual_enabled_counter--;
						sprawdzone_timery--;
					}
				}
				else
				{
					tasks_array[i].counter++;

					if( tasks_array[i].counter == tasks_array[i].interval )
					{
						// wywolanie podpietego callbacka
						if( tasks_array[i].callback )
						{
							tasks_array[i].callback(i);
						}

						// jesli jest to pojedyncze wywolanie timera - wylacz go
						if( tasks_array[i].type == dmb_timer_type_single )
						{
							tasks_array[i].state = dmb_timer_state_off;
							_dmb_timer_actual_enabled_counter--;
							sprawdzone_timery--;
						}

						tasks_array[i].counter = 0;
					}
				}
				sprawdzone_timery++;
			}
		}

		// sprawdzenie, czy jakies timery sa do wylaczenia i w takim przypadku - wylaczenie ich
		if( timers_to_off_counter )
		{
			for( uint8_t i = 0; i < DMB_TIMER_TASKS_NUMBER && timers_to_off_counter; i++ )
			{
				if(tasks_array[i].state == dmb_timer_state_to_off)
				{
					tasks_array[i].state = dmb_timer_state_off;
				}
			}
		}

		// dodawanie timerow - dzieki temu mechanizmowi beda mialy dobre czasy niezaleznie od umieszczenia w tablicy
		// + zawsze sa dodawane w jednym miejscu
		if( _dmb_timers_to_add_counter )
		{
			sprawdzone_timery = 0;

			for( uint8_t i = 0; i < DMB_TIMER_TASKS_NUMBER && (sprawdzone_timery != _dmb_timers_to_add_counter); i++ )
			{
				if( tasks_array[i].state == dmb_timer_state_to_add )
				{
					tasks_array[i].state = dmb_timer_state_on;
					_dmb_timer_actual_enabled_counter++;
					sprawdzone_timery++;
				}
			}
			_dmb_timers_to_add_counter = 0;
		}
	}
}

/*
 * Pauzowanie okreslonego timera/taska. Mozemy go potem wznowic.
 */
void dmb_timer_pause_task(int8_t timer_number)
{
	tasks_array[timer_number].state = dmb_timer_state_pause;
}

/*
 * Kontynuowanie okreslonego timera/taska. Musimy potem wybrac
 */
void dmb_timer_continue_task(int8_t timer_number)
{
	if( tasks_array[timer_number].state == dmb_timer_state_pause )
	{
		tasks_array[timer_number].state = dmb_timer_state_on;
	}
}

/*
 * Wylaczenie timera o danym id.
 */
void dmb_timer_disable_task(uint8_t id)
{
	// oznaczenie timera jako timer do wylaczenia
	tasks_array[id].state = dmb_timer_state_to_off;

	// dodanie timera do wylaczenia
	timers_to_off_counter++;
}

/*
 * Kasowanie aktualnie zliczonych tikow - mozna je wykonac przed petla glowna gdzie jest sprawdzany event.
 * Wtedy zaczynamy odmierzanie czasu w tym samym momencie.
 */
void dmb_timer_reset_ticks()
{
	_dmb_timer_tick_counter = 0;
}

/*
 * Dodanie taska do timera. Funkcja zwraca jego id.
 */
int8_t dmb_timer_add_task( uint16_t interval, dmb_timer_type_e type, dmb_timer_callback callback )
{
	int8_t timer_number = -1;

	// poszukiwanie wolnego timera w tablicy
	for( uint8_t i = 0; i < DMB_TIMER_TASKS_NUMBER; i++ )
	{
		if( tasks_array[i].state == dmb_timer_state_off )
		{
			tasks_array[i].state = dmb_timer_state_to_add;
			tasks_array[i].callback = callback;
			tasks_array[i].type = type;
			tasks_array[i].counter = 0;
			tasks_array[i].interval = interval / _dmb_timer_tick_interval;

			_dmb_timers_to_add_counter++;
			timer_number = i;

			break;
		}
	}

	return timer_number;
}

/*
 * Dodanie taska cyklicznego do timera z opoznieniem startowym. Zakladam, ze jest to zawsze timer continous.
 * Dla singla wystarczy zawsze tylko dodac delay.
 */
int8_t dmb_timer_add_task_with_predelay( uint16_t predelay, uint16_t interval, dmb_timer_callback callback )
{
	int8_t timer_number = -1;

	// poszukiwanie wolnego timera w tablicy
	for( uint8_t i = 0; i < DMB_TIMER_TASKS_NUMBER; i++ )
	{
		if( tasks_array[i].state == dmb_timer_state_off )
		{
			tasks_array[i].state = dmb_timer_state_to_add;
			tasks_array[i].callback = callback;
			tasks_array[i].type = dmb_timer_type_pre_delay;
			tasks_array[i].counter = 0;
			tasks_array[i].pre_counter = predelay / _dmb_timer_tick_interval;
			tasks_array[i].interval = interval / _dmb_timer_tick_interval;

			_dmb_timers_to_add_counter++;
			timer_number = i;

			break;
		}
	}

	return timer_number;
}
