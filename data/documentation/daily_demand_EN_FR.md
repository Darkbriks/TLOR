# daily_demand.csv

## English

Standardized daily demand extracted from the provided linear forecast.

Recommended use: Demand simulation input. The forecast was supplied with the subject; it was not fitted by Role 1.

## Français

Demande journalière standardisée extraite de la prévision linéaire fournie.

Utilisation recommandée : Entrée de simulation de la demande. La prévision a été fournie avec le sujet ; elle n'a pas été ajustée par le rôle 1.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `date` | date | YYYY-MM-DD | Calendar date. | Date calendaire. | ISO 8601 |
| 3 | `daily_kg` | number | kg/day | Forecast material generated at the point during the day. | Matière prévue générée au point pendant la journée. | Provided linear forecast |
| 4 | `source_cumulative_kg` | number | kg | Cumulative forecast mass from 2023-04-14 through the current date, as supplied by the source. | Masse prévue cumulée du 14/04/2023 jusqu'à la date courante, telle que fournie par la source. | Does not reset after collections |
| 5 | `daily_l` | number | litres/day | Forecast volume generated at the point during the day. | Volume prévu généré au point pendant la journée. | Provided linear forecast |
| 6 | `source_cumulative_l` | number | litres | Cumulative forecast volume from 2023-04-14 through the current date, as supplied by the source. | Volume prévu cumulé du 14/04/2023 jusqu'à la date courante, tel que fourni par la source. | Does not reset after collections |
| 7 | `capacity_l` | number | litres | Working container capacity assigned to the point, based on the modal observed capacity. | Capacité de travail du conteneur attribuée au point, basée sur la capacité observée la plus fréquente. | May be blank for inactive points without observations |
