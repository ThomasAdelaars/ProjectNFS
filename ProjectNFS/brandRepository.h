#pragma once

#include "Brand.h"
#include "DatabaseConnection.h"
#include "IReadRepository.h"

#include <vector>

class BrandRepository : public IReadRepository<Brand>
{
public:
    BrandRepository(DatabaseConnection& database);

    std::vector<Brand> getAll() override;

    Brand getBrandDetails(int brandID);

private:
    DatabaseConnection& database;
};