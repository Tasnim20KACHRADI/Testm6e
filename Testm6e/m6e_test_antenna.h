/* ================================================================
   m6e_test_antenna.h
   Module de test des antennes de l'arche CAMA25
   Balayage Theta et Phi en fichiers CSV separes
   ================================================================ */
#ifndef M6E_TEST_ANTENNA_H
#define M6E_TEST_ANTENNA_H

#include "common.h"
#include "stm32.h"
#include "antenna_map.h"
#include "csv_writer.h"

/* Nombre de lectures par antenne pour le test */
#define TEST_READS_PER_ANT   3

/* Timeout lecture test (ms) */
#define TEST_TIMEOUT_MS      500

/* Puissance de test fixe — max pour maximiser les detections */
#define TEST_POWER_CDBM      3150   /* 31.5 dBm */

/* Fonction principale de test */
int m6e_test_antennas(TMR_Reader*  r,
                      STM32Info*   stm32,
                      const char*  target_epc);

#endif /* M6E_TEST_ANTENNA_H */
