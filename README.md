# RFID Measurement Arch

## 1. Présentation du projet

Ce dépôt regroupe les codes développés pour l’automatisation des mesures RFID UHF sur une arche de mesure, utilisant le lecteur **ThingMagic M6e** et un ensemble d’antennes.

Le projet comporte deux versions distinctes, disponibles dans des branches Git séparées :

- **`motor-control-heatmap_3D`** : version avec moteur, permettant le contrôle du moteur et la visualisation des mesures sous forme de heatmap 3D.
- **`Version_sans_moteur_2D`** : version sans moteur, dédiée aux mesures RFID et à leur visualisation en 2D.

**Important :** avant de modifier le code, vérifiez que vous travaillez sur la bonne branche en fonction de la version souhaitée.

## 2. Fonctionnalités principales

Les programmes permettent principalement de :

- Piloter le lecteur **ThingMagic M6e** via la MercuryAPI.
- Configurer la fréquence et la puissance d’émission.
- Sélectionner les antennes de l’arche et automatiser les séquences de mesure.
- Récupérer le RSSI (*Received Signal Strength Indicator*) et déterminer la puissance minimale d’activation (**Pmin**) des tags RFID.
- Enregistrer et traiter les mesures en fonction de la fréquence et de la position du tag.
- Analyser les données et générer des courbes et des heatmaps pour visualiser les résultats.
- Contrôler le moteur et réaliser une cartographie 3D des mesures dans la version avec moteur.


## 3. Installation et configuration

Avant de compiler ou d’exécuter les programmes, il est nécessaire de vérifier et de configurer correctement l’environnement de développement, notamment :

- La **MercuryAPI** et ses bibliothèques associées.
- Les drivers nécessaires à la communication avec le lecteur M6e.
- Les ports de communication et les paramètres de connexion.
- Les outils de compilation et les dépendances logicielles.
- Les branchements matériels entre le lecteur, l’arche, les antennes et, pour la version concernée, le moteur et son système de commande.

Vérifiez soigneusement les branchements avant toute mise sous tension et assurez-vous que les composants sont correctement alimentés.

## 4. Recommandations et dépannage

- **Vérifier les branchements :** contrôler les connexions matérielles avant de lancer une mesure.
- **Tester la communication :** vérifier que le lecteur M6e est correctement détecté avant d’exécuter une séquence complète.
- **Lecteur M6e non réactif :** le lecteur peut parfois cesser de répondre. Un arrêt suivi d’une remise sous tension peut résoudre le problème. Si nécessaire, vérifier également l’alimentation, les connexions et les paramètres de communication.
- **Tester progressivement :** commencer par un test simple de connexion et de lecture avant de lancer l’ensemble des mesures.
- **Préserver les versions :** effectuer les modifications et les `push` sur la branche correspondant à la version concernée, sans écraser involontairement l’autre version.

## 5. Documentation

Une copie du rapport de stage est disponible dans le dossier suivant, si celui-ci est présent sur la branche consultée :

`Testm6e/Testm6e/draft/rapport`

Il est recommandé de consulter ce rapport avant de modifier le code afin de mieux comprendre l’architecture du système, les branchements matériels et le principe des mesures.

## 7. Contact

Pour toute question concernant le fonctionnement du système, la configuration de l’environnement ou la procédure de mesure, vous pouvez me contacter :

**E-mail :** tesnimkachradi4@gmail.com
