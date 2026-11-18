# prediction_validation.csv

## English

Observed collection events matched with forecast volume accumulated since the previous observed collection.

Recommended use: Forecast diagnostics only; do not use directly as daily routing demand.

## Français

Événements de collecte observés associés au volume prévu accumulé depuis la collecte observée précédente.

Utilisation recommandée : Diagnostic de prévision uniquement ; ne pas utiliser directement comme demande journalière de routage.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `date` | date | YYYY-MM-DD | Calendar date. | Date calendaire. | ISO 8601 |
| 3 | `actual_collected_kg` | number | kg | Mass actually collected on the observed collection date. | Masse réellement collectée à la date de collecte observée. |  |
| 4 | `predicted_accumulated_kg` | number | kg | Forecast mass accumulated since the previous observed collection through this date. | Masse prévue accumulée depuis la collecte observée précédente jusqu'à cette date. |  |
| 5 | `actual_collected_l` | number | litres | Volume actually collected on the observed collection date. | Volume réellement collecté à la date de collecte observée. |  |
| 6 | `predicted_accumulated_l` | number | litres | Forecast volume accumulated since the previous observed collection through this date. | Volume prévu accumulé depuis la collecte observée précédente jusqu'à cette date. |  |
