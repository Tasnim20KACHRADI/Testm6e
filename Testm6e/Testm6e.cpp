/* ================================================================
   m6e_init.c
   Phase d'initialisation automatique — ThingMagic M6e + STM32
   MercuryAPI C v1.37.x — Visual Studio 2022 — Windows
   ================================================================ */

   /* --- DOIT etre en premier, avant tout include --- */
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define HAVE_STRUCT_TIMESPEC

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "tm_reader.h"

/* ================================================================
   CONFIGURATION
   ================================================================ */
#define COM_MIN          1
#define COM_MAX          20
#define URI_MAX_LEN      32
#define MODEL_MAX_LEN    64
#define VERSION_MAX_LEN  64
#define MAX_FREQ         64

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
    int  com_port;
    char description[128];
} STM32Info;

/* ================================================================
   UTILITAIRES
   ================================================================ */
static void print_sep(void)
{
    printf("-----------------------------------\n");
}

static void print_header(const char* title)
{
    printf("\n===================================\n");
    printf("  %s\n", title);
    printf("===================================\n");
}

/* ================================================================
   DETECTION PORT COM (Windows API)
   ================================================================ */
static int com_port_exists(int port_num)
{
    char   path[32];
    HANDLE h;
    sprintf(path, "\\\\.\\COM%d", port_num);
    h = CreateFileA(path,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) return 0;
    CloseHandle(h);
    return 1;
}

/* ================================================================
   FONCTION 1 — m6e_connect
   Detection automatique COM + baudrate + validation modele M6e
   ================================================================ */
static const uint32_t BAUDRATES[] = {
    115200, 9600, 19200, 38400, 57600, 230400, 460800, 921600
};
static const int BAUDRATE_COUNT =
(int)(sizeof(BAUDRATES) / sizeof(BAUDRATES[0]));

int m6e_connect(TMR_Reader* r, M6eInfo* info)
{
    int        port, b, found = 0;
    char       uri[URI_MAX_LEN];
    TMR_Status ret;
    TMR_String model, version;
    char       model_buf[MODEL_MAX_LEN];
    char       version_buf[VERSION_MAX_LEN];

    print_header("ETAPE 1 - DETECTION M6e");
    print_sep();
    printf("  Scan COM%d a COM%d\n", COM_MIN, COM_MAX);
    print_sep();

    for (port = COM_MIN; port <= COM_MAX && !found; port++)
    {
        if (!com_port_exists(port)) continue;

        printf("\n  Port detecte : COM%d\n", port);
        printf("  Test baudrates :\n");
        sprintf(uri, "tmr:///COM%d", port);

        for (b = 0; b < BAUDRATE_COUNT && !found; b++)
        {
            printf("    COM%d @ %6u ... ", port, BAUDRATES[b]);
            fflush(stdout);

            ret = TMR_create(r, uri);
            if (ret != TMR_SUCCESS)
            {
                printf("Erreur create\n");
                continue;
            }

            TMR_paramSet(r, TMR_PARAM_BAUDRATE, &BAUDRATES[b]);

            ret = TMR_connect(r);
            if (ret != TMR_SUCCESS)
            {
                printf("Timeout\n");
                memset(r, 0, sizeof(TMR_Reader));
                continue;
            }

            model.value = model_buf;
            model.max = MODEL_MAX_LEN;
            ret = TMR_paramGet(r, TMR_PARAM_VERSION_MODEL, &model);
            if (ret != TMR_SUCCESS)
            {
                printf("Erreur model\n");
                TMR_destroy(r);
                memset(r, 0, sizeof(TMR_Reader));
                continue;
            }

            if (strstr(model_buf, "M6e") == NULL &&
                strstr(model_buf, "m6e") == NULL)
            {
                printf("Non M6e (%s)\n", model_buf);
                TMR_destroy(r);
                memset(r, 0, sizeof(TMR_Reader));
                continue;
            }

            version.value = version_buf;
            version.max = VERSION_MAX_LEN;
            TMR_paramGet(r, TMR_PARAM_VERSION_SOFTWARE, &version);

            printf("OK !\n");
            strncpy(info->uri, uri, URI_MAX_LEN - 1);
            strncpy(info->model, model_buf, MODEL_MAX_LEN - 1);
            strncpy(info->version, version_buf, VERSION_MAX_LEN - 1);
            info->uri[URI_MAX_LEN - 1] = '\0';
            info->model[MODEL_MAX_LEN - 1] = '\0';
            info->version[VERSION_MAX_LEN - 1] = '\0';
            info->com_port = port;
            info->baudrate = BAUDRATES[b];
            found = 1;
        }
    }

    if (!found)
    {
        printf("\n  [ERREUR] Aucun lecteur M6e detecte.\n");
        printf("  -> Verifiez cable USB, alimentation, antenne\n");
        return 0;
    }

    printf("\n");
    print_sep();
    printf("  M6e detecte sur COM%d\n", info->com_port);
    printf("  Modele   : %s\n", info->model);
    printf("  Firmware : %s\n", info->version);
    printf("  Baudrate : %u baud\n", info->baudrate);
    print_sep();
    return 1;
}

/* ================================================================
   FONCTION 2 — m6e_config
   Configuration region OPEN + lecture hop table filtree EU
   ================================================================ */
int m6e_config(TMR_Reader* r,
    uint32_t* freq_table,
    int* freq_count)
{
    TMR_Status     ret;
    TMR_Region     region = TMR_REGION_OPEN;
    TMR_uint32List hop;
    uint32_t       hop_buf[MAX_FREQ];
    int            i;

    print_header("ETAPE 2 - CONFIGURATION REGION");

    ret = TMR_paramSet(r, TMR_PARAM_REGION_ID, &region);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] Region OPEN : %s\n", TMR_strerr(r, ret));
        return 0;
    }
    printf("  Region : OPEN\n\n");

    hop.list = hop_buf;
    hop.max = MAX_FREQ;
    hop.len = 0;

    ret = TMR_paramGet(r, TMR_PARAM_REGION_HOPTABLE, &hop);
    if (ret != TMR_SUCCESS)
    {
        printf("  [ERREUR] Hop table : %s\n", TMR_strerr(r, ret));
        return 0;
    }

    /* Filtrer : ne garder que la plage 865000-869000 kHz */
    *freq_count = 0;
    for (i = 0; i < (int)hop.len; i++)
    {
        if (hop_buf[i] >= 865000 && hop_buf[i] <= 869000)
        {
            freq_table[*freq_count] = hop_buf[i];
            (*freq_count)++;
        }
    }

    print_sep();
    printf("  Frequences plage EU (865-869 MHz) : %d\n", *freq_count);
    print_sep();
    for (i = 0; i < *freq_count; i++)
        printf("  [%02d] %u kHz\n", i + 1, freq_table[i]);
    print_sep();
    return 1;
}

/* ================================================================
   FONCTION 3 — stm32_probe
   Reproduit exactement le comportement de Termite :
   - 115200 bauds, 8N1, no parity, no handshake
   - Append CR+LF comme Termite ("*1#\r\n")
   - DTR enable (Termite l'active par defaut)
   - Timeouts adaptes pour lecture bloquante

   Protocole STM32 (control_arch_motor_v22_v01.ino) :
     Commande : *1#\r\n
     Reponse  : "Output N1\r\n"
   ================================================================ */
static int stm32_probe(int port_num)
{
    char         path[32];
    HANDLE       h;
    DCB          dcb;
    COMMTIMEOUTS timeouts;
    DWORD        written = 0;
    DWORD        read_bytes = 0;
    DWORD        err = 0;
    char         response[256];
    int          is_stm32 = 0;
    DWORD        di;
    BOOL         ok;

    sprintf(path, "\\\\.\\COM%d", port_num);

    /* --- 1. Ouverture du port --- */
    h = CreateFileA(path,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,   /* mode synchrone */
        NULL);
    if (h == INVALID_HANDLE_VALUE)
    {
        printf("\n    [DEBUG] CreateFile echoue : err=%lu\n    ",
            GetLastError());
        return 0;
    }
    printf("\n    [DEBUG] Port ouvert OK\n");

    /* --- 2. Lire le DCB courant --- */
    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb))
    {
        printf("    [DEBUG] GetCommState echoue : err=%lu\n    ",
            GetLastError());
        CloseHandle(h);
        return 0;
    }

    /* --- 3. Configurer 115200 8N1 no handshake
     *        Identique a Termite "115200 bps, 8N1, no handshake"
     *        DTR_CONTROL_ENABLE reproduit le comportement Termite
     * --- */
    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fParity = FALSE;
    /* Handshake : aucun (no handshake comme Termite) */
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    /* DTR enable — Termite active DTR par defaut */
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    /* RTS enable — Termite active RTS par defaut */
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = FALSE;
    dcb.fBinary = TRUE;

    if (!SetCommState(h, &dcb))
    {
        printf("    [DEBUG] SetCommState echoue : err=%lu\n    ",
            GetLastError());
        CloseHandle(h);
        return 0;
    }
    printf("    [DEBUG] DCB configure : 115200 8N1 no handshake\n");

    /* --- 4. Timeouts
     *   ReadIntervalTimeout = MAXDWORD avec les deux autres = 0
     *   -> ReadFile retourne immediatement avec ce qui est disponible
     *   On utilise plutot un timeout total fixe pour attendre la reponse
     * --- */
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 4000; /* 4 secondes max */
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(h, &timeouts);
    printf("    [DEBUG] Timeouts : read=4000ms write=1000ms\n");

    /* --- 5. NE PAS purger immediatement —
     *        L'ouverture du port peut resetter la STM32 via DTR
     *        Elle envoie "Ready to Switch\r\nOutput N1\r\n" au boot
     *        On attend et on lit ce message de boot d'abord
     * --- */
    Sleep(2000); /* attendre le boot STM32 (delay(1000) + init) */

    /* Lire le message de boot si present */
    memset(response, 0, sizeof(response));
    ok = ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL);
    printf("    [DEBUG] Boot msg  : %lu octets : [", (unsigned long)read_bytes);
    if (read_bytes > 0)
    {
        for (di = 0; di < read_bytes; di++)
            printf("%c", (response[di] >= 32 && response[di] < 127)
                ? response[di] : '.');
        /* Si le boot message contient deja "Output" -> STM32 confirmee */
        if (strstr(response, "Output") != NULL)
        {
            printf("]\n    [DEBUG] Output detecte dans boot message !\n    ");
            CloseHandle(h);
            return 1;
        }
    }
    else
        printf("(vide)");
    printf("]\n");

    /* Purger maintenant pour envoyer une commande propre */
    PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
    Sleep(100);

    /* --- 7. Envoyer "*1#\r\n" caractere par caractere
     *        La STM32 lit dans loop() char par char
     *        On envoie lentement pour ne pas rater le '*'
     * --- */
    ok = WriteFile(h, "*", 1, &written, NULL);
    Sleep(50);
    ok = WriteFile(h, "1", 1, &written, NULL);
    Sleep(50);
    ok = WriteFile(h, "#", 1, &written, NULL);
    Sleep(50);
    ok = WriteFile(h, "\r\n", 2, &written, NULL);
    err = GetLastError();
    printf("    [DEBUG] WriteFile : ok=%d err=%lu\n",
        ok, (unsigned long)(ok ? 0 : err));
    FlushFileBuffers(h);

    /* --- 8. Attendre la reponse STM32
     *        3 secondes pour laisser le loop() STM32 traiter
     *        la commande meme si le moteur stepper tourne
     * --- */
    Sleep(3000);

    /* --- 9. Lire la reponse --- */
    memset(response, 0, sizeof(response));
    ok = ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL);
    err = GetLastError();
    printf("    [DEBUG] ReadFile  : ok=%d bytes=%lu err=%lu\n",
        ok, (unsigned long)read_bytes, (unsigned long)(ok ? 0 : err));

    /* --- 10. Afficher le contenu brut --- */
    printf("    [DEBUG] Reponse   : [");
    if (read_bytes > 0)
        for (di = 0; di < read_bytes; di++)
            printf("%c", (response[di] >= 32 && response[di] < 127)
                ? response[di] : '.');
    else
        printf("(vide)");
    printf("]\n    ");

    /* --- 11. Validation --- */
    if (read_bytes > 0 && strstr(response, "Output") != NULL)
        is_stm32 = 1;

    CloseHandle(h);
    return is_stm32;
}

/* ================================================================
   FONCTION 4 — stm32_detect
   Scanner les ports COM et identifier la STM32 par sa reponse
   ================================================================ */
int stm32_detect(int m6e_com_port, STM32Info* stm32)
{
    int port, found = 0;

    print_header("ETAPE 3 - DETECTION STM32 NUCLEO");
    print_sep();
    printf("  Protocole : envoi '*1#' -> reponse attendue 'Output'\n");
    print_sep();

    for (port = COM_MIN; port <= COM_MAX; port++)
    {
        if (port == m6e_com_port) continue;
        if (!com_port_exists(port)) continue;

        printf("  Test COM%d ...", port);
        fflush(stdout);

        if (stm32_probe(port))
        {
            printf("STM32 detectee !\n");
            stm32->com_port = port;
            sprintf(stm32->description, "STM32 Nucleo");
            found = 1;
            break;
        }
        else
        {
            printf("pas de reponse\n");
        }
    }

    printf("\n");

    if (!found)
    {
        printf("  [!] STM32 non detectee.\n");
        printf("  -> Verifiez que la Nucleo est branchee\n");
        printf("  -> Verifiez que le firmware est actif\n");
        return 0;
    }

    print_sep();
    printf("  STM32 detectee sur COM%d\n", stm32->com_port);
    print_sep();
    return 1;
}

/* ================================================================
   MAIN
   ================================================================ */
int main(void)
{
    TMR_Reader reader;
    M6eInfo    m6e;
    STM32Info  stm32;
    uint32_t   freq_table[MAX_FREQ];
    int        freq_count = 0;
    int        connected = 0;
    int        stm32_ok = 0;

    memset(&reader, 0, sizeof(reader));
    memset(&m6e, 0, sizeof(m6e));
    memset(&stm32, 0, sizeof(stm32));

    printf("\n");
    printf("===================================\n");
    printf("  INITIALISATION RFID M6e + STM32  \n");
    printf("===================================\n");

    /* 1. Connexion M6e */
    if (!m6e_connect(&reader, &m6e))
        goto end;
    connected = 1;

    /* 2. Configuration region OPEN + hop table */
    if (!m6e_config(&reader, freq_table, &freq_count))
        goto end;

    /* 3. Detection STM32 */
    stm32_ok = stm32_detect(m6e.com_port, &stm32);

    /* ---- RESUME FINAL ---- */
    printf("\n");
    printf("===================================\n");
    printf("       RESUME INITIALISATION       \n");
    printf("===================================\n");
    printf("  M6e    : COM%d | %u baud\n", m6e.com_port, m6e.baudrate);
    printf("  Modele : %s\n", m6e.model);
    printf("  FW     : %s\n", m6e.version);
    printf("  Region : OPEN | %d frequences EU\n", freq_count);

    if (stm32_ok)
        printf("  STM32  : COM%d | %s\n",
            stm32.com_port, stm32.description);
    else
        printf("  STM32  : Non detectee\n");

    printf("===================================\n");
    printf("  Pret a mesurer\n");
    printf("===================================\n\n");

end:
    if (connected)
        TMR_destroy(&reader);

    printf("Appuyez sur Entree pour quitter...\n");
    getchar();
    return 0;
}