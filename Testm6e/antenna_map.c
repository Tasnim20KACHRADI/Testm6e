/* ================================================================
   antenna_map.c
   Table des antennes CAMA25
   ================================================================ */
#include "antenna_map.h"

/* Antennes Theta : [1..12, 25..36, 49..60] */
const int ANTENNA_THETA[ANT_THETA_COUNT] = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12,
    25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60
};

/* Antennes Phi : [13..24, 37..48, 61..72] */
const int ANTENNA_PHI[ANT_PHI_COUNT] = {
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72
};

const char* axe_to_str(AntennaAxe axe)
{
    return (axe == AXE_THETA) ? "Theta" : "Phi";
}

AntennaAxe get_antenna_axe(int antenna_num)
{
    int i;
    for (i = 0; i < ANT_THETA_COUNT; i++)
        if (ANTENNA_THETA[i] == antenna_num)
            return AXE_THETA;
    return AXE_PHI;
}
