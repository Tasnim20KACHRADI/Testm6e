/* ================================================================
   m6e_heatmap.h
   Mesure complete avec rotation de l'arche (moteur pas-a-pas)
   Boucle : angle x frequence x antenne -> 1 CSV avec colonne Angle_deg
   Ce CSV est ensuite lu par python/heatmap.py pour tracer la heatmap.
   ================================================================ */
#ifndef M6E_HEATMAP_H
#define M6E_HEATMAP_H

#include "common.h"
#include "stm32.h"
#include "motor.h"
#include "antenna_map.h"
#include "m6e_measure.h"

/* Balayage angulaire — a adapter a la mecanique de l'arche */
#define HEATMAP_ANGLE_START_DEG    0.0f
#define HEATMAP_ANGLE_STOP_DEG   350.0f
#define HEATMAP_ANGLE_STEP_DEG    10.0f

int m6e_measure_heatmap(TMR_Reader*  r,
                        STM32Info*   stm32,
                        uint32_t*    freq_table,
                        int          freq_count,
                        const char*  target_epc);

#endif /* M6E_HEATMAP_H */
