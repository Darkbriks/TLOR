# threshold_evaluation.csv

## English

Full audit table for every tested point-threshold combination across three demand scenarios.

Recommended use: Role 1/Role 4 sensitivity analysis and policy review. point_policy.csv contains the selected row per point.

## Français

Table d'audit complète pour chaque combinaison point-seuil testée dans trois scénarios de demande.

Utilisation recommandée : Analyse de sensibilité et revue de politique pour les rôles 1/4. point_policy.csv contient la ligne sélectionnée par point.

## Columns / Colonnes

| # | Column / Colonne | Type | Unit / Unité | English description | Description française | Notes |
|---:|---|---|---|---|---|---|
| 1 | `point_id` | integer | — | Internal numeric point identifier; matches point order in points.csv and the distance/time matrices. | Identifiant numérique interne du point ; correspond à l'ordre des points dans points.csv et dans les matrices de distance/temps. | 0–328; point 328 is the depot |
| 2 | `threshold` | number | fraction of capacity | Candidate or selected fill threshold that triggers a collection. | Seuil de remplissage candidat ou sélectionné qui déclenche une collecte. | 0–1; tested values 0.70–0.95 |
| 3 | `collections_80pct` | integer | count | Number of recommended collections over the study horizon under 80% demand. | Nombre de collectes recommandées sur l'horizon d'étude avec demande à 80 %. |  |
| 4 | `overflow_events_80pct` | integer | point-days | Number of simulated daily capacity exceedances under 80% demand. | Nombre de dépassements journaliers simulés de capacité avec demande à 80 %. | Conservative diagnostic; within-day timing is unknown |
| 5 | `low_fill_events_80pct` | integer | count | Number of recommended collections occurring below 50% fill under 80% demand. | Nombre de collectes recommandées effectuées sous 50 % de remplissage avec demande à 80 %. | Technical diagnostic |
| 6 | `mean_fill_at_collection_80pct` | number | fraction | Mean fill rate when a collection is recommended under 80% demand. | Taux de remplissage moyen lorsqu'une collecte est recommandée avec demande à 80 %. | Blank/NaN if no collection occurs |
| 7 | `ending_inventory_l_80pct` | number | litres | Simulated uncollected volume remaining at the end of the horizon under 80% demand. | Volume simulé non collecté restant à la fin de l'horizon avec demande à 80 %. |  |
| 8 | `ending_inventory_kg_80pct` | number | kg | Simulated uncollected mass remaining at the end of the horizon under 80% demand. | Masse simulée non collectée restant à la fin de l'horizon avec demande à 80 %. |  |
| 9 | `collections_100pct` | integer | count | Number of recommended collections over the study horizon under baseline demand. | Nombre de collectes recommandées sur l'horizon d'étude avec demande de référence. |  |
| 10 | `overflow_events_100pct` | integer | point-days | Number of simulated daily capacity exceedances under baseline demand. | Nombre de dépassements journaliers simulés de capacité avec demande de référence. | Conservative diagnostic; within-day timing is unknown |
| 11 | `low_fill_events_100pct` | integer | count | Number of recommended collections occurring below 50% fill under baseline demand. | Nombre de collectes recommandées effectuées sous 50 % de remplissage avec demande de référence. | Technical diagnostic |
| 12 | `mean_fill_at_collection_100pct` | number | fraction | Mean fill rate when a collection is recommended under baseline demand. | Taux de remplissage moyen lorsqu'une collecte est recommandée avec demande de référence. | Blank/NaN if no collection occurs |
| 13 | `ending_inventory_l_100pct` | number | litres | Simulated uncollected volume remaining at the end of the horizon under baseline demand. | Volume simulé non collecté restant à la fin de l'horizon avec demande de référence. |  |
| 14 | `ending_inventory_kg_100pct` | number | kg | Simulated uncollected mass remaining at the end of the horizon under baseline demand. | Masse simulée non collectée restant à la fin de l'horizon avec demande de référence. |  |
| 15 | `collections_120pct` | integer | count | Number of recommended collections over the study horizon under 120% demand. | Nombre de collectes recommandées sur l'horizon d'étude avec demande à 120 %. |  |
| 16 | `overflow_events_120pct` | integer | point-days | Number of simulated daily capacity exceedances under 120% demand. | Nombre de dépassements journaliers simulés de capacité avec demande à 120 %. | Conservative diagnostic; within-day timing is unknown |
| 17 | `low_fill_events_120pct` | integer | count | Number of recommended collections occurring below 50% fill under 120% demand. | Nombre de collectes recommandées effectuées sous 50 % de remplissage avec demande à 120 %. | Technical diagnostic |
| 18 | `mean_fill_at_collection_120pct` | number | fraction | Mean fill rate when a collection is recommended under 120% demand. | Taux de remplissage moyen lorsqu'une collecte est recommandée avec demande à 120 %. | Blank/NaN if no collection occurs |
| 19 | `ending_inventory_l_120pct` | number | litres | Simulated uncollected volume remaining at the end of the horizon under 120% demand. | Volume simulé non collecté restant à la fin de l'horizon avec demande à 120 %. |  |
| 20 | `ending_inventory_kg_120pct` | number | kg | Simulated uncollected mass remaining at the end of the horizon under 120% demand. | Masse simulée non collectée restant à la fin de l'horizon avec demande à 120 %. |  |
