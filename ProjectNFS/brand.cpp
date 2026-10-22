#include "brand.h"

brand::brand(int cId, const std::string& cName, int cCityId)
{
	id = cId;
	name = cName;
	cityId = cCityId;
};

std::string brand::toString()
{
	return std::to_string(id) + " - " + name + " - " + std::to_string(cityId);
}

int brand::getId()
{
	return id;
}

