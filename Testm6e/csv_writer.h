/* ================================================================
   csv_writer.h
   Ecriture du fichier CSV de resultats
   ================================================================ */
#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include "common.h"

#define CSV_FILENAME    "rfid_results.csv"  /* nom de base */
#define CSV_PATH        "C:\\Users\\tkachradi\\Desktop\\code\\Testm6e\\Testm6e\\Release\\"

/* Structure d'une ligne de résultat */
typedef struct {
    uint32_t freq_khz;
    int      antenna_num;
    char     axe[8];        /* "Theta" ou "Phi" */
    float    p_min_dbm;     /* -1.0 si DEAD_ZONE */
    float    rssi_dbm;      /* -999.0 si DEAD_ZONE */
    char     status[16];    /* "OK" ou "DEAD_ZONE" */
    char     timestamp[32];
    char     epc[64];
} CsvRow;

int  csv_open(const char* filename);
void csv_write_row(const CsvRow* row);
void csv_close(void);

#endif
