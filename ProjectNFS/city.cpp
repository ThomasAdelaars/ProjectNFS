#include "city.h"

std::string city::toString()
{
    return name + ", " + std::to_string(countryId);
}

int city::getId()
{
    return id;
}

