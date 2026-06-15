/* ================================================================
   csv_writer.c
   Ecriture fichier CSV des resultats de mesure
   ================================================================ */
#include "csv_writer.h"

static FILE* csv_file = NULL;

/* ================================================================
   csv_open — ouvre le fichier et écrit l'en-tête
   ================================================================ */
int csv_open(const char* filename)
{
    csv_file = fopen(filename, "w");
    if (!csv_file)
    {
        printf("[ERREUR] Impossible de creer %s\n", filename);
        return 0;
    }

    /* En-tête CSV */
    fprintf(csv_file,
            "Freq_kHz,Antenne,Axe,P_min_dBm,"
            "RSSI_dBm,Status,EPC,Timestamp\n");

    printf("  CSV ouvert : %s\n", filename);
    return 1;
}

/* ================================================================
   csv_write_row — écrit une ligne de résultat
   ================================================================ */
void csv_write_row(const CsvRow* row)
{
    if (!csv_file) return;

    if (strcmp(row->status, "DEAD_ZONE") == 0)
    {
        fprintf(csv_file, "%u,%d,%s,%.1f,%.1f,%s,%s,%s\n",
                row->freq_khz,
                row->antenna_num,
                row->axe,
                -1.0f,
                -999.0f,
                "DEAD_ZONE",
                row->epc,
                row->timestamp);
    }
    else
    {
        fprintf(csv_file, "%u,%d,%s,%.1f,%.1f,%s,%s,%s\n",
                row->freq_khz,
                row->antenna_num,
                row->axe,
                row->p_min_dbm,
                row->rssi_dbm,
                "OK",
                row->epc,
                row->timestamp);
    }

    /* Forcer l'écriture immédiate — pas de perte si crash */
    fflush(csv_file);
}

/* ================================================================
   csv_close — ferme le fichier
   ================================================================ */
void csv_close(void)
{
    if (csv_file)
    {
        fclose(csv_file);
        csv_file = NULL;
        printf("  CSV ferme.\n");
    }
}
