/* ================================================================
   stm32.c
   Detection STM32 Nucleo + commandes switch antenne

   SOLUTION AU PROBLEME DE RESET DTR :
   Port ouvert UNE SEULE FOIS dans stm32_detect()
   Handle garde ouvert dans stm32->handle
   stm32_send_cmd() utilise ce handle — PAS de reset DTR

   Protocole STM32 (control_arch_motor_v22_v01.ino) :
     Boot     : "Output N1\r\n" + "Ready to Switch\r\n"
     Commande : *N#  -> "Output NN\r\n"
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
	if (!GetCommState(h, &dcb)) return 0;/*get commstateguration actuelle du port série dans la structure DCB*/

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
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = FALSE;
    dcb.fBinary = TRUE;

	if (!SetCommState(h, &dcb)) return 0;/*set commstate appliquer la configuration du port série à partir de la structure DCB*/

    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 3000;
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
        Sleep(2000);

        /* Lire le boot message */
        memset(response, 0, sizeof(response));
        ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL);

        if (read_bytes > 0 && strstr(response, "Output") != NULL)
        {
            printf(" STM32 detectee (boot) !\n");
            stm32->com_port = port;
            stm32->handle = h;
            sprintf(stm32->description, "STM32 Nucleo");
            found = 1;
            break;
        }

        /* Fallback : envoyer *1# */
        PurgeComm(h, PURGE_RXABORT | PURGE_RXCLEAR |
            PURGE_TXABORT | PURGE_TXCLEAR);
        Sleep(100);
        stm32_write_cmd(h, 1);
        Sleep(500);

        memset(response, 0, sizeof(response));
        ReadFile(h, response, sizeof(response) - 1, &read_bytes, NULL);

        if (read_bytes > 0 && strstr(response, "Output") != NULL)
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
        return 0;
    }

    print_sep();
    printf("  STM32 : COM%d | %s\n", stm32->com_port, stm32->description);
    printf("  Port  : maintenu ouvert (pas de reset DTR)\n");
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

    if (stm32->handle == INVALID_HANDLE_VALUE) return 0;

    for (attempt = 0; attempt < 3; attempt++)
    {
        /*
         * Purge complete avant chaque envoi
         * Evite saturation buffer apres plusieurs commandes
         */
        PurgeComm(stm32->handle, PURGE_RXABORT | PURGE_RXCLEAR |
            PURGE_TXABORT | PURGE_TXCLEAR);
        Sleep(50);

        /* Envoyer la commande */
        stm32_write_cmd(stm32->handle, antenna_num);

        /* Attendre la reponse */
        Sleep(300);

        /* Lire la reponse */
        memset(response, 0, sizeof(response));
        ReadFile(stm32->handle, response, sizeof(response) - 1,
            &read_bytes, NULL);

        if (read_bytes > 0 && strstr(response, "Output") != NULL)
            return 1;

        /* Pas de reponse — attendre avant retry */
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