#include "DatabaseConnection.h"
#include "brandRepository.h"
#include "carRepository.h"

#include <iostream>
#include <exception>

int main()
{
    try
    {
        // Maak verbinding met MariaDB
        DatabaseConnection database(
            "127.0.0.1",
            "needforspeed",
            "nfs",
            "underground"
        );

        database.connect();

        std::cout << "Verbinding met MariaDB gelukt!\n\n";

        // Maak de repository
        BrandRepository brandRepository(database);

        // Haal alle merken op
        auto brands = brandRepository.getAll();

        std::cout << "Aantal merken: " << brands.size() << "\n\n";
        std::cout << "=== Merkenlijst ===\n";

        for (const Brand& brand : brands)
        {
            std::cout << brand.getId()
                << " - "
                << brand.getName()
                << '\n';
        }

        // Test de details van het eerste merk
        if (!brands.empty())
        {
            const int brandID = brands.front().getId();

            Brand details =
                brandRepository.getBrandDetails(brandID);

            std::cout << "\n=== Details van "
                << details.getName()
                << " ===\n";

            std::cout << "ID: " << details.getId() << '\n';
            std::cout << "Stad: " << details.getCityName() << '\n';
            std::cout << "Land: " << details.getCountryName() << '\n';
            std::cout << "Logo bytes: "
                << details.getLogo().size() << '\n';
        }
        else
        {
            std::cout << "\nEr zijn nog geen merken in de database.\n";
        }

        CarRepository carRepository(database);

        int selectedBrandID = 3; // Voorbeeld: Ford

        auto cars = carRepository.getCarsByBrandId(selectedBrandID);

        std::cout << "\n=== Auto's van merk ID "
            << selectedBrandID << " ===\n";

        for (const Car& car : cars)
        {
            std::cout << car.getId() << " - "
                << car.getModelName() << " - "
                << car.getTypeName() << " - "
                << car.getPowerHP() << " PK\n";
        }

        database.disconnect();
    }
    catch (const sql::SQLException& e)
    {
        std::cerr << "Databasefout: " << e.what() << '\n';
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fout: " << e.what() << '\n';
        return 1;
    }

    return 0;
}