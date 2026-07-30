# RFID Measurement Arch

Ce dépôt regroupe les codes développés pour l’automatisation des mesures RFID UHF sur l’arche.

Les programmes permettent principalement de :

* piloter le lecteur **ThingMagic M6e** via MercuryAPI ;
* configurer la fréquence et la puissance d’émission ;
* sélectionner les antennes de l’arche et automatiser les séquences de mesure ;
* récupérer le **RSSI** et déterminer la **puissance minimale d’activation (Pmin)** des tags ;
* enregistrer et traiter les mesures en fonction de la fréquence et de la position du tag ;
* analyser les données et générer des courbes/heatmaps.

### ⚠️ Work in Progress (WIP)

Ce projet est actuellement **en cours de développement, de test et de raffinement**. Certains modules sont encore expérimentaux et peuvent évoluer au fur et à mesure des validations sur l’arche.

Pour assurer le bon fonctionnement des codes, l’environnement doit être correctement déployé et configuré : **MercuryAPI, bibliothèques/dépendances, drivers du lecteur M6e, ports de communication et outils de compilation**.

La version actuelle ne doit donc pas être considérée comme une version finale du système.
