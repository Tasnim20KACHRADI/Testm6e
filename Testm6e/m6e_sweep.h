/* ================================================================
   m6e_sweep.h
   Sweep frequence x P_min sur une antenne choisie
   ================================================================ */
#ifndef M6E_SWEEP_H
#define M6E_SWEEP_H

#include "common.h"
#include "stm32.h"
#include "csv_writer.h"

   /* Plage de frequences EU */
#define SWEEP_FREQ_START         865000   /* kHz */
#define SWEEP_FREQ_STOP          869000   /* kHz */
#define SWEEP_FREQ_STEP             200   /* kHz */

/* Puissance */
#define SWEEP_POWER_START          3150   /* centidBm = 31.5 dBm */
#define SWEEP_POWER_STOP            500   /* centidBm = 5.0  dBm */
#define SWEEP_POWER_STEP_COARSE     500   /* centidBm = 5.0  dBm (dichotomie) */
#define SWEEP_POWER_STEP_FINE        50   /* centidBm = 0.5  dBm (raffinement) */

/* Timeout lecture */
#define SWEEP_TIMEOUT_MS            500

/* Fonction principale */
int m6e_sweep(TMR_Reader* r,
    STM32Info* stm32,
    const char* target_epc);

#endif /* M6E_SWEEP_H */