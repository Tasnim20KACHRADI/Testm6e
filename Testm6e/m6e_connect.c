/* ================================================================
   m6e_connect.c
   Detection automatique COM + baudrate + validation modele M6e

   API MercuryAPI :
   - TMR_create()                   — URI "tmr:///COMx"
   - TMR_paramSet(BAUDRATE)         — avant connect()
   - TMR_connect()                  — connexion serie
   - TMR_paramGet(VERSION_MODEL)    — valider "M6e"
   - TMR_paramGet(VERSION_SOFTWARE) — firmware
   ================================================================ */
#include "m6e_connect.h"

static const uint32_t BAUDRATES[] = {
    115200, 9600, 19200, 38400, 57600, 230400, 460800, 921600
};
static const int BAUDRATE_COUNT =
    (int)(sizeof(BAUDRATES) / sizeof(BAUDRATES[0]));

int m6e_connect(TMR_Reader* r, M6eInfo* info)
{
    int        port, b, found = 0;/*Indices de boucle + flag de succès*/
    char       uri[URI_MAX_LEN];/*URI de connexion (ex: "tmr:///COM3")*/
    TMR_Status ret;/*code d'erreur de chaque operation*/
    TMR_String model, version;/*Chaînes de caractères pour les informations sur le modèle et le firmware*/
    char       model_buf[MODEL_MAX_LEN];
    char       version_buf[VERSION_MAX_LEN];

    print_header("ETAPE 1 - DETECTION M6e");
    print_sep();
    printf("  Scan COM%d a COM%d\n", COM_MIN, COM_MAX);
    print_sep();

	for (port = COM_MIN; port <= COM_MAX && !found; port++)//Scanner tous les ports COM
    {
		if (!com_port_exists(port)) continue;//Vérifie si le port COM existe avant de tenter la connexion
        printf("\n  Port detecte : COM%d\n", port);
        sprintf(uri, "tmr:///COM%d", port);

        for (b = 0; b < BAUDRATE_COUNT && !found; b++)
        {
            printf("    COM%d @ %6u ... ", port, BAUDRATES[b]);
            fflush(stdout);

            ret = TMR_create(r, uri);/* Instancie un objet TMR_Reader*/
            if (ret != TMR_SUCCESS)
            {
                printf("create err\n");
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
            model.max   = MODEL_MAX_LEN;
			ret = TMR_paramGet(r, TMR_PARAM_VERSION_MODEL, &model);/* Récupère le modèle du lecteur pour vérifier s'il s'agit d'un M6e*/
            if (ret != TMR_SUCCESS ||
               (strstr(model_buf, "M6e") == NULL &&
                strstr(model_buf, "m6e") == NULL))
            {
                printf("Non M6e\n");
                TMR_destroy(r);
                memset(r, 0, sizeof(TMR_Reader));
                continue;
            }

            version.value = version_buf;
            version.max   = VERSION_MAX_LEN;
            TMR_paramGet(r, TMR_PARAM_VERSION_SOFTWARE, &version);

            printf("OK !\n");
            strncpy(info->uri,     uri,         URI_MAX_LEN - 1);/*le bloc de sauvegarde des informations du lecteur une fois la connexion validée*/
            strncpy(info->model,   model_buf,   MODEL_MAX_LEN - 1);
            strncpy(info->version, version_buf, VERSION_MAX_LEN - 1);
            info->uri[URI_MAX_LEN - 1]         = '\0';
            info->model[MODEL_MAX_LEN - 1]     = '\0';
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

    printf("\n"); print_sep();
    printf("  M6e    : COM%d | %u baud\n", info->com_port, info->baudrate);
    printf("  Modele : %s\n", info->model);
    printf("  FW     : %s\n", info->version);
    print_sep();
    return 1;
}
