/* ================================================================
   stm32.c — VERSION CORRIGÉE
   Detection STM32 Nucleo + commandes switch antenne

   CORRECTIONS:
   1. DTR/RTS desactivés (n'etaient pas vraiment desactivés avant)
   2. Logging du contenu réellement reçu
   3. Null-termination des strings
   4. Protocole flexible (accepte différents formats)
   5. Meilleur timeout handling
   ================================================================ */
#include "stm32.h"

   /* ================================================================
      stm32_configure_port serie
      ================================================================ */
static int stm32_configure_port(HANDLE h)
{
    DCB          dcb;
    COMMTIMEOUTS timeouts;

    memset(&dcb, 0, sizeof(dcb));
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(h, &dcb)) return 0;

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    /* *** CORRECTION #1: DTR/RTS DESACTIVÉS *** */
    dcb.fDtrControl = DTR_CONTROL_DISABLE;    /* ÉTAIT: DTR_CONTROL_ENABLE */
    dcb.fRtsControl = RTS_CONTROL_DISABLE;    /* ÉTAIT: RTS_CONTROL_ENABLE */

    dcb.fAbortOnError = FALSE;
    dcb.fBinary = TRUE;

    if (!SetCommState(h, &dcb)) return 0;

    /* Configuration des timeouts */
    timeouts.ReadIntervalTimeout = 10;              /* Interval entre caractères */
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 3000;      /* 3 sec max */
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 500;
    SetCommTimeouts(h, &timeouts);

    return 1;
}

/* ================================================================
   stm32_write_cmd — envoie *N# caractere par caractere
   ================================================================ */
static void stm32_write_cmd(HANDLE h, int antenna_num)
{
    char  cmd[16];
    int   ci;
    DWORD w;

    sprintf(cmd, "*%d#", antenna_num);
    for (ci = 0; cmd[ci] != '\0'; ci++)
    {
        WriteFile(h, &cmd[ci], 1, &w, NULL);
        Sleep(30);
    }
    WriteFile(h, "\r\n", 2, &w, NULL);
    FlushFileBuffers(h);
}

/* ================================================================
   stm32_detect
   Ouvre le port COM une fois et garde le handle dans stm32->handle
   ================================================================ */
int stm32_detect(int m6e_com_port, STM32Info* stm32)
{
    int    port, found = 0;
    HANDLE h;
    DWORD  read_bytes;
    char   path[32];
    char   response[256];
    int    i;

    stm32->handle = INVALID_HANDLE_VALUE;

    print_header("ETAPE 3 - DETECTION STM32");
    print_sep();

    for (port = COM_MIN; port <= COM_MAX; port++)
    {
        if (port == m6e_com_port) continue;
        if (!com_port_exists(port)) continue;

        printf("  Test COM%d ...", port);
        fflush(stdout);

        sprintf(path, "\\\\.\\COM%d", port);

        h = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
            0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE)
        {
            printf(" acces refuse\n");
            continue;
        }

        if (!stm32_configure_port(h))
        {
            printf(" config echec\n");
            CloseHandle(h);
            continue;
        }

        /* Attendre le boot STM32 */
        /* CORRECTION #2: Augmenté à 5 secondes (peut-être que DTR causait des reset avant) */
        Sleep(5000);

        /* Lire le boot message */
        /* CORRECTION #3: Null-terminer le string */
        memset(response, 0, sizeof(response));
        if (!ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL))
        {
            printf(" ReadFile echec\n");
            CloseHandle(h);
            continue;
        }
        if (read_bytes > 0)
        {
            response[read_bytes] = '\0';
        }

        /* CORRECTION #4: LOGGING du contenu réellement reçu */
        if (read_bytes > 0)
        {
            printf("\n    [DEBUG] Recu (%ld bytes): ", read_bytes);
            for (i = 0; i < (int)read_bytes && i < 60; i++)
            {
                if (response[i] >= 32 && response[i] < 127)
                    printf("%c", response[i]);
                else if (response[i] == '\r')
                    printf("\\r");
                else if (response[i] == '\n')
                    printf("\\n");
                else
                    printf("[%02X]", (unsigned char)response[i]);
            }
            printf("\n  ");
        }

        /* CORRECTION #5: Protocole flexible — accepter "Output" OU d'autres formats */
        if (read_bytes > 0)
        {
            if (strstr(response, "Output") != NULL ||
                strstr(response, "Ready") != NULL ||
                read_bytes > 5)  /* Au moins quelque chose */
            {
                printf(" STM32 detectee (boot) !\n");
                stm32->com_port = port;
                stm32->handle = h;
                sprintf(stm32->description, "STM32 Nucleo");
                found = 1;
                break;
            }
        }

        /* Fallback : envoyer *1# */
        printf("[tentative *1#] ");
        fflush(stdout);

        PurgeComm(h, PURGE_RXABORT | PURGE_RXCLEAR |
            PURGE_TXABORT | PURGE_TXCLEAR);
        Sleep(100);
        stm32_write_cmd(h, 1);
        Sleep(500);

        memset(response, 0, sizeof(response));
        if (!ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL))
        {
            printf(" ReadFile echec\n");
            CloseHandle(h);
            continue;
        }
        if (read_bytes > 0)
        {
            response[read_bytes] = '\0';
        }

        if (read_bytes > 0)
        {
            printf("\n    [DEBUG] Recu (%ld bytes): ", read_bytes);
            for (i = 0; i < (int)read_bytes && i < 60; i++)
            {
                if (response[i] >= 32 && response[i] < 127)
                    printf("%c", response[i]);
                else if (response[i] == '\r')
                    printf("\\r");
                else if (response[i] == '\n')
                    printf("\\n");
                else
                    printf("[%02X]", (unsigned char)response[i]);
            }
            printf("\n  ");
        }

        if (read_bytes > 0)
        {
            printf(" STM32 detectee (*1#) !\n");
            stm32->com_port = port;
            stm32->handle = h;
            sprintf(stm32->description, "STM32 Nucleo");
            found = 1;
            break;
        }

        printf(" pas de reponse\n");
        CloseHandle(h);
    }

    if (!found)
    {
        printf("\n  [!] STM32 non detectee.\n");
        printf("  [!] Verifiez:\n");
        printf("      - Solder bridges SB13/SB14 fermes\n");
        printf("      - UART TX/RX de la Nucleo\n");
        printf("      - Firmware STM32 envoie des donnees au boot\n");
        return 0;
    }

    print_sep();
    printf("  STM32 : COM%d | %s\n", stm32->com_port, stm32->description);
    printf("  Port  : maintenu ouvert (DTR/RTS desactives)\n");
    print_sep();
    return 1;
}

/* ================================================================
   stm32_send_cmd
   Envoie *N# sur le handle permanent — 3 tentatives avec retry
   ================================================================ */
int stm32_send_cmd(STM32Info* stm32, int antenna_num)
{
    DWORD read_bytes;
    char  response[128];
    int   attempt;
    int   i;

    if (stm32->handle == INVALID_HANDLE_VALUE) return 0;

    for (attempt = 0; attempt < 3; attempt++)
    {
        PurgeComm(stm32->handle, PURGE_RXABORT | PURGE_RXCLEAR |
            PURGE_TXABORT | PURGE_TXCLEAR);
        Sleep(50);

        stm32_write_cmd(stm32->handle, antenna_num);

        Sleep(300);

        memset(response, 0, sizeof(response));
        if (!ReadFile(stm32->handle, response, sizeof(response) - 1,
            &read_bytes, NULL))
        {
            if (attempt < 2) printf(" [retry %d]", attempt + 1);
            Sleep(200);
            continue;
        }

        if (read_bytes > 0)
        {
            response[read_bytes] = '\0';
        }

        if (read_bytes > 0 && strstr(response, "Output") != NULL)
        {
            return 1;
        }

        if (attempt < 2)
        {
            printf(" [retry %d]", attempt + 1);
            fflush(stdout);
            Sleep(200);
        }
    }

    return 0;
}

/* ================================================================
   stm32_close
   ================================================================ */
void stm32_close(STM32Info* stm32)
{
    if (stm32->handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(stm32->handle);
        stm32->handle = INVALID_HANDLE_VALUE;
    }
}