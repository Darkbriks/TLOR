# active_points.csv

## English

Canonical point register with location, observed capacity, source-presence flags, and the final active/inactive decision.

Recommended use: All roles. Filter active = True for modelling; point 328 is the depot and must not be treated as a collection point.

## Français

Registre canonique des points avec localisation, capacité observée, indicateurs de présence dans les sources et décision finale actif/inactif.

Utilisation recommandée : Tous les rôles. Filtrer active = True pour la modélisation ; le point 328 est le dépôt et ne doit pas être traité comme un point de collecte.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `label` | text | — | Normalized human-readable point name. | Nom normalisé et lisible du point. |  |
| 3 | `identifiant` | text | — | Original external identifier supplied for the point. | Identifiant externe d'origine fourni pour le point. |  |
| 4 | `addresse?` | text | — | Street address when available in the source file. | Adresse postale lorsqu'elle est disponible dans le fichier source. | Source column name retained as supplied |
| 5 | `commune` | text | — | Municipality containing the point. | Commune dans laquelle se trouve le point. |  |
| 6 | `code_postal` | text/integer | — | Postal code of the point. | Code postal du point. | Interpret as an identifier, not a quantity |
| 7 | `latitude` | number | decimal degrees | Geographic latitude in WGS84. | Latitude géographique en WGS84. |  |
| 8 | `longitude` | number | decimal degrees | Geographic longitude in WGS84. | Longitude géographique en WGS84. |  |
| 9 | `capacity_l` | number | litres | Working container capacity assigned to the point, based on the modal observed capacity. | Capacité de travail du conteneur attribuée au point, basée sur la capacité observée la plus fréquente. | May be blank for inactive points without observations |
| 10 | `minimum_observed_capacity_l` | number | litres | Smallest container capacity observed for the point in historical data. | Plus petite capacité de conteneur observée pour le point dans les données historiques. |  |
| 11 | `maximum_observed_capacity_l` | number | litres | Largest container capacity observed for the point in historical data. | Plus grande capacité de conteneur observée pour le point dans les données historiques. |  |
| 12 | `capacity_variant_count` | integer | count | Number of distinct capacity values observed for the point. | Nombre de valeurs de capacité distinctes observées pour le point. |  |
| 13 | `capacity_observation_count` | integer | count | Number of point-date capacity observations used to derive the working capacity. | Nombre d'observations de capacité point-date utilisées pour déterminer la capacité de travail. |  |
| 14 | `capacity_kg` | number | kg | Working container capacity in kilograms, converted with the point-specific kg-per-litre ratio from the supplied forecast. | Capacité de travail du conteneur en kilogrammes, convertie avec le ratio kg par litre propre au point issu de la prévision fournie. | Canonical capacity field for optimization |
| 15 | `is_depot` | boolean | — | True if the row is the depot rather than a collection point. | Vrai si la ligne représente le dépôt plutôt qu'un point de collecte. | True/False |
| 16 | `in_actual` | boolean | — | True if the point appears in the historical collection data. | Vrai si le point apparaît dans les données historiques de collecte. | True/False |
| 17 | `in_prediction` | boolean | — | True if the point appears in the provided prediction data. | Vrai si le point apparaît dans les données de prévision fournies. | True/False |
| 18 | `listed_disappeared` | boolean | — | True if the point is listed as disappeared in the supplied RTF list. | Vrai si le point est indiqué comme disparu dans la liste RTF fournie. | True/False |
| 19 | `active` | boolean | — | Final inclusion flag used by the Role 1 dataset. | Indicateur final d'inclusion utilisé par le jeu de données du rôle 1. | True only for non-depot points present in both actual and prediction data and not listed as disappeared |
| 20 | `status_reason` | text | — | Machine-readable explanation for the active/inactive decision. | Explication lisible par machine de la décision actif/inactif. |  |
