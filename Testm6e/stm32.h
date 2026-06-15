/* ================================================================
   stm32.h
   Detection STM32 Nucleo + envoi commandes antennes
   Le port COM reste ouvert en permanence pour eviter
   le reset DTR a chaque commande
   ================================================================ */
#ifndef STM32_H
#define STM32_H

#include "common.h"

int  stm32_detect(int m6e_com_port, STM32Info* stm32);
int  stm32_send_cmd(STM32Info* stm32, int antenna_num);
void stm32_close(STM32Info* stm32);

#endif