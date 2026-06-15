/* ================================================================
   common.h
   Structures et defines partages entre tous les modules
   ================================================================ */
#ifndef COMMON_H
#define COMMON_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef HAVE_STRUCT_TIMESPEC
#define HAVE_STRUCT_TIMESPEC
#endif

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "tm_reader.h"

/* ================================================================
   CONFIGURATION GENERALE
   ================================================================ */
#define COM_MIN              1
#define COM_MAX              20
#define URI_MAX_LEN          32
#define MODEL_MAX_LEN        64
#define VERSION_MAX_LEN      64
#define MAX_FREQ             64
#define EPC_STR_MAX          64
#define MAX_TAGS             32

/* Longueur EPC attendue — filtre anti-phantom */
#define EPC_EXPECTED_BITS    96

/* Antennes */
#define ANTENNA_1            1
#define ANTENNA_31           31

/* Nombre de lectures par antenne */
#define READS_PER_ANTENNA    5

/* Puissance maximale : 31.5 dBm */
#define POWER_MAX_CDBM       3150

/* Timeout lecture en ms */
#define READ_TIMEOUT_MS      500

/* ================================================================
   STRUCTURES
   ================================================================ */
typedef struct {
    char     uri[URI_MAX_LEN];
    int      com_port;
    uint32_t baudrate;
    char     model[MODEL_MAX_LEN];
    char     version[VERSION_MAX_LEN];
} M6eInfo;

typedef struct {
    int    com_port;
    char   description[128];
    HANDLE handle;   /* port COM ouvert en permanence — pas de reset DTR */
} STM32Info;

typedef struct {
    char    epc[EPC_STR_MAX];
    int     epc_bits;

    int16_t rssi_a1[READS_PER_ANTENNA];
    int     count_a1;
    float   mean_a1;

    int16_t rssi_a31[READS_PER_ANTENNA];
    int     count_a31;
    float   mean_a31;

    float   delta;
    int     seen_both;
} TagProfile;

/* ================================================================
   UTILITAIRES INLINE
   ================================================================ */
static void print_sep(void)
{
    printf("-----------------------------------\n");
}

static void print_header(const char* t)
{
    printf("\n===================================\n");
    printf("  %s\n", t);
    printf("===================================\n");
}

static float compute_mean(const int16_t* s, int n)
{
    int i; float sum = 0.0f;
    if (n == 0) return -999.0f;
    for (i = 0; i < n; i++) sum += (float)s[i];
    return sum / (float)n;
}

static int find_tag(TagProfile* tags, int count, const char* epc)
{
    int i;
    for (i = 0; i < count; i++)
        if (strcmp(tags[i].epc, epc) == 0) return i;
    return -1;
}

static int com_port_exists(int port_num)
{
    char path[32]; HANDLE h;
    sprintf(path, "\\\\.\\COM%d", port_num);
    h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
                    0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    CloseHandle(h); return 1;
}

#endif /* COMMON_H */
