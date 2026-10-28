#pragma once

#include "car.h"
#include "DatabaseConnection.h"

#include <string>
#include <vector>

class CarRepository
{
public:
    explicit CarRepository(DatabaseConnection& database);

    std::vector<Car> getCarsByBrandId(int brandID);

    void addCar(
        int brandID,
        const std::string& modelName,
        const std::string& typeName,
        int powerHP
    );

    void updateCar(
        int carID,
        const std::string& modelName,
        const std::string& typeName,
        int powerHP
    );

    void deleteCar(int carID);

private:
    DatabaseConnection& database;
};