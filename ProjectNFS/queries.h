#pragma once

inline const char* QUERY_CARS = R"sql(
SELECT 
    b.name AS 'Merk',
    CASE 
        WHEN t.name IS NOT NULL THEN CONCAT(m.name, ' ', t.name)
        ELSE m.name
    END AS 'Type',
    mt.PowerHP AS 'Vermogen [pk]',
    ROUND(mt.PowerHP * 0.735, 0) AS 'Vermogen [kW]',
    CONCAT(c.name, ', ', co.name) AS 'Hoofdkantoor'
FROM modelType mt
JOIN model m ON mt.modelID = m.ID
JOIN brand b ON m.brandID = b.ID
JOIN city c ON b.cityID = c.ID
JOIN country co ON c.countryID = co.ID
LEFT JOIN `type` t ON mt.typeID = t.ID
ORDER BY b.name, m.name
)sql";