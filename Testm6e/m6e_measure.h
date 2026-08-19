/* ================================================================
   m6e_measure.h
   Mesure de puissance minimale par dichotomie + raffinement
   Balayage de toutes les antennes de l'arche CAMA25
   ================================================================ */
#ifndef M6E_MEASURE_H
#define M6E_MEASURE_H

/* Includes dans le bon ordre — common.h en premier */
#include "common.h"
#include "stm32.h"
#include "antenna_map.h"
#include "csv_writer.h"

/* Puissance en centidBm */
#define POWER_START_CDBM    3150    /* 31.5 dBm */
#define POWER_MIN_CDBM       500    /* 5.0 dBm  */

/* Dichotomie — pas grossier (centidBm) */
#define POWER_STEP_COARSE    500    /* 5.0 dBm  */

/* Raffinement — pas fin (centidBm) */
#define POWER_STEP_FINE       50    /* 0.5 dBm  */

/* Timeout lecture par mesure (ms) */
#define MEASURE_TIMEOUT_MS   500

/* Fonction principale de mesure */
int m6e_measure_all(TMR_Reader*  r,
                    STM32Info*   stm32,
                    uint32_t*    freq_table,
                    int          freq_count,
                    const char*  target_epc);

/* Mesure un point (antenne x frequence) et ecrit la ligne CSV
 * associee — angle_deg est le tag angle moteur de l'arche pour
 * cette ligne (0.0 si non applicable). Reutilise par m6e_heatmap. */
void m6e_measure_antenna(TMR_Reader*  r,
                    STM32Info*   stm32,
                    int          antenna_num,
                    AntennaAxe   axe,
                    uint32_t     freq_khz,
                    const char*  target_epc,
                    float        angle_deg);

#endif /* M6E_MEASURE_H */
