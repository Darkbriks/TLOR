# actual_point_day.csv

## English

Historical collections aggregated to one row per point and collection date.

Recommended use: Validation and baseline analysis. Several source container rows on the same point-date are summed.

## Français

Collectes historiques agrégées à une ligne par point et par date de collecte.

Utilisation recommandée : Validation et analyse de référence. Plusieurs lignes de conteneurs sources pour un même point-date sont additionnées.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `date` | date | YYYY-MM-DD | Calendar date. | Date calendaire. | ISO 8601 |
| 3 | `description` | text | — | Point description from the historical collection records. | Description du point issue des enregistrements historiques de collecte. |  |
| 4 | `first_tour` | integer | tour identifier | First tour number recorded for the point on this date. | Premier numéro de tournée enregistré pour le point à cette date. | Identifier, not the number of tours |
| 5 | `tour_count` | integer | count | Number of distinct tour identifiers associated with the point-date. | Nombre d'identifiants de tournée distincts associés au point-date. |  |
| 6 | `container_records` | integer | count | Number of original container rows aggregated into this point-date row. | Nombre de lignes de conteneurs d'origine agrégées dans cette ligne point-date. |  |
| 7 | `collected_l` | number | litres | Total volume collected at the point on the date. | Volume total collecté au point à cette date. |  |
| 8 | `observed_capacity_l` | number | litres | Total observed container capacity represented by the aggregated source records. | Capacité totale observée des conteneurs représentée par les enregistrements sources agrégés. |  |
| 9 | `collected_kg` | number | kg | Total mass collected at the point on the date. | Masse totale collectée au point à cette date. |  |
| 10 | `observed_fill_rate` | number | fraction | Collected volume divided by observed capacity for the point-date. | Volume collecté divisé par la capacité observée pour le point-date. | 0–1 normally; inspect exceptions |
