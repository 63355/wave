#ifndef BUTTON_USER_H
#define BUTTON_USER_H

#include "button.h"
#include "gpio.h"
#include "tim.h"

extern uint8_t single_flag,double_flag,long_falg;
void button_init(void);

#endif //BUTTON_USER_H
