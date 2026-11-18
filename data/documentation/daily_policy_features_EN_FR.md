# daily_policy_features.csv

## English

Daily predicted state under the selected point-specific collection threshold.

Recommended use: Main hand-off to routing/scheduling. recommended_collection is a planning signal, not a final vehicle route.

## Français

État journalier prédit selon le seuil de collecte sélectionné pour chaque point.

Utilisation recommandée : Transmission principale au routage/ordonnancement. recommended_collection est un signal de planification, pas une tournée finale de véhicule.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `date` | date | YYYY-MM-DD | Calendar date. | Date calendaire. | ISO 8601 |
| 3 | `daily_l` | number | litres/day | Forecast volume generated at the point during the day. | Volume prévu généré au point pendant la journée. | Provided linear forecast |
| 4 | `daily_kg` | number | kg/day | Forecast material generated at the point during the day. | Matière prévue générée au point pendant la journée. | Provided linear forecast |
| 5 | `capacity_l` | number | litres | Working container capacity assigned to the point, based on the modal observed capacity. | Capacité de travail du conteneur attribuée au point, basée sur la capacité observée la plus fréquente. | May be blank for inactive points without observations |
| 6 | `selected_threshold` | number | fraction of capacity | Point-specific threshold selected in point_policy.csv. | Seuil spécifique au point sélectionné dans point_policy.csv. | 0–1 |
| 7 | `predicted_inventory_l` | number | litres | Simulated volume immediately after adding the day's demand and before a triggered reset. | Volume simulé immédiatement après l'ajout de la demande du jour et avant une remise à zéro déclenchée. |  |
| 8 | `predicted_inventory_kg` | number | kg | Simulated mass immediately after adding the day's demand and before a triggered reset. | Masse simulée immédiatement après l'ajout de la demande du jour et avant une remise à zéro déclenchée. |  |
| 9 | `predicted_fill_rate` | number | fraction | Predicted inventory volume divided by point capacity. | Volume de stock prévu divisé par la capacité du point. |  |
| 10 | `estimated_days_until_capacity` | integer | days | Floor estimate of days remaining before capacity is reached at the current daily volume. | Estimation entière du nombre de jours restant avant d'atteindre la capacité au volume journalier actuel. | 9999 means daily_l = 0 |
| 11 | `recommended_collection` | boolean | — | True when predicted_fill_rate reaches the selected threshold; the simulation resets inventory after this signal. | Vrai lorsque predicted_fill_rate atteint le seuil sélectionné ; la simulation remet ensuite le stock à zéro. | Planning signal only; not a final route |
