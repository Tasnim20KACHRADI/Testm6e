/* ================================================================
   m6e_scan.h
   Scan tags RFID avec filtre anti-phantom + RSSI dual antenne
   ================================================================ */
#ifndef M6E_SCAN_H
#define M6E_SCAN_H

#include "common.h"
#include "stm32.h"

int m6e_scan_tags(TMR_Reader* r,
                  STM32Info*  stm32,
                  TagProfile* tags,
                  int*        tag_count);

#endif
