#include "BrandRepository.h"

#include <cstdint>
#include <istream>
#include <memory>
#include <stdexcept>
#include <utility>

BrandRepository::BrandRepository(DatabaseConnection& database) : database(database)
{
}

std::vector<Brand> BrandRepository::getAll()
{
	auto& connection = database.getConnection();
	std::unique_ptr<sql::PreparedStatement> statement(connection.prepareStatement("CALL GetBrands()"));
	std::unique_ptr<sql::ResultSet> result(statement->executeQuery());
	std::vector<Brand> brands;

	while (result->next())
	{
		Brand brand;

		brand.setId(result->getInt("ID"));
		brand.setName(result->getString("name").c_str());

		brands.push_back(std::move(brand));
	}

	return brands;
}

Brand BrandRepository::getBrandDetails(int brandID)
{
    auto& connection = database.getConnection();

    std::unique_ptr<sql::PreparedStatement> statement(
        connection.prepareStatement("CALL GetBrandDetails(?)")
    );

    statement->setInt(1, brandID);

    std::unique_ptr<sql::ResultSet> result(
        statement->executeQuery()
    );

    // Ga naar de eerste rij
    if (!result->next())
    {
        throw std::runtime_error("Automerk niet gevonden");
    }

    // Nu kunnen we de gegevens uitlezen
    Brand brand;

    brand.setId(result->getInt("brandID"));
    brand.setName(result->getString("brandName").c_str());

    // Het logo uit de BLOB-kolom lezen
    std::unique_ptr<std::istream> logoStream(
        result->getBlob("logo")
    );

    if (logoStream)
    {
        std::vector<std::uint8_t> logoBytes;
        char byte;

        while (logoStream->get(byte))
        {
            logoBytes.push_back(
                static_cast<std::uint8_t>(
                    static_cast<unsigned char>(byte)
                    )
            );
        }

        brand.setLogo(std::move(logoBytes));
    }

    brand.setCityName(result->getString("cityName").c_str());
    brand.setCountryName(result->getString("countryName").c_str());

    return brand;
}