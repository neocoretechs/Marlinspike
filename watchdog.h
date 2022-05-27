#ifndef WATCHDOG_H
#define WATCHDOG_H
#include <avr/wdt.h>
#include <math.h>
#include "RoboCore.h"
#include "WInterruptService.h"
class WatchdogTimer {
  private:
  public:
  static InterruptService* dogservice;
  // initialize watch dog with a 1 sec interrupt time
  void watchdog_init(uint16_t timeout );
  void watchdog_init(uint16_t timeout, InterruptService* is);
  // pad the dog/reset watchdog. MUST be called at least every second after the first watchdog_init or AVR will go into emergency procedures..
  void watchdog_reset();
};
#endif
