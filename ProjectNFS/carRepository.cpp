#include "carRepository.h"

#include <memory>
#include <stdexcept>
#include <utility>

CarRepository::CarRepository(DatabaseConnection& database)
    : database(database)
{
}

std::vector<Car> CarRepository::getCarsByBrandId(int brandID)
{
    auto& connection = database.getConnection();

    std::unique_ptr<sql::PreparedStatement> statement(connection.prepareStatement("CALL GetCarsByBrand(?)"));

    statement->setInt(1, brandID);

    std::unique_ptr<sql::ResultSet> result(statement->executeQuery());

    std::vector<Car> cars;

    while (result->next())
    {
        Car car;

        // ID van de rij in modelType
        car.setId(result->getInt("carID"));

        car.setModelID(result->getInt("modelID"));
        car.setModelName(
            result->getString("modelName").c_str()
        );

        // Een type mag NULL zijn
        if (result->isNull("typeID"))
        {
            car.setTypeID(-1);
            car.setTypeName("");
        }
        else
        {
            car.setTypeID(result->getInt("typeID"));
            car.setTypeName(
                result->getString("typeName").c_str()
            );
        }

        car.setPowerHP(result->getInt("PowerHP"));

        cars.push_back(std::move(car));
    }

    return cars;
}

void CarRepository::addCar(
    int brandID,
    const std::string& modelName,
    const std::string& typeName,
    int powerHP
)
{
    auto& connection = database.getConnection();

    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement(
            "CALL AddCar(?, ?, ?, ?)"
        )
    );

    statement->setInt(1, brandID);
    statement->setString(2, modelName);
    statement->setString(3, typeName);
    statement->setInt(4, powerHP);

    statement->execute();
}

void CarRepository::updateCar(
    int carID,
    const std::string& modelName,
    const std::string& typeName,
    int powerHP
)
{
    auto& connection = database.getConnection();

    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement(
            "CALL UpdateCar(?, ?, ?, ?)"
        )
    );

    statement->setInt(1, carID);
    statement->setString(2, modelName);
    statement->setString(3, typeName);
    statement->setInt(4, powerHP);

    statement->execute();
}

void CarRepository::deleteCar(int carID)
{
    auto& connection = database.getConnection();

    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement(
            "CALL DeleteCar(?)"
        )
    );

    statement->setInt(1, carID);

    statement->execute();
}