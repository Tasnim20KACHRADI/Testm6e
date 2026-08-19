/* ================================================================
   m6e_heatmap.h
   Balayage angle (moteur) x frequence x antenne Theta/Phi
   Genere 1 fichier CSV : heatmap_*.csv (colonne Angle_deg en plus)
   ================================================================ */
#ifndef M6E_HEATMAP_H
#define M6E_HEATMAP_H

#include "common.h"
#include "stm32.h"
#include "motor.h"
#include "antenna_map.h"
#include "csv_writer.h"
#include "m6e_measure.h"
#include "m6e_config.h"

/* Configuration heatmap */
#define HEATMAP_ANGLE_START_DEG  0.0f
#define HEATMAP_ANGLE_STOP_DEG   350.0f
#define HEATMAP_ANGLE_STEP_DEG   10.0f

int m6e_measure_heatmap(TMR_Reader* r,
    STM32Info* stm32,
    uint32_t* freq_table,
    int          freq_count,
    const char* target_epc);

#endif
