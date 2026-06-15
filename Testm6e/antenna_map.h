/* ================================================================
   antenna_map.h
   Table des antennes de l'arche CAMA25
   Theta : [1..12, 25..36, 49..60]
   Phi   : [13..24, 37..48, 61..72]
   ================================================================ */
#ifndef ANTENNA_MAP_H
#define ANTENNA_MAP_H

#include "common.h"

/* Nombre total d'antennes */
#define ANT_THETA_COUNT  36
#define ANT_PHI_COUNT    36
#define ANT_TOTAL        72

/* Listes des numéros d'antennes */
extern const int ANTENNA_THETA[];
extern const int ANTENNA_PHI[];

/* Axe de l'antenne */
typedef enum {
    AXE_THETA = 0,
    AXE_PHI   = 1
} AntennaAxe;

/* Retourne "Theta" ou "Phi" */
const char* axe_to_str(AntennaAxe axe);

/* Retourne l'axe d'une antenne selon son numéro */
AntennaAxe get_antenna_axe(int antenna_num);

#endif
