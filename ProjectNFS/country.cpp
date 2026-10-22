#include "country.h"

country::country(int cId, const std::string& cName)
{
	id = cId;
	name = cName;
};

std::string country::toString()
{
    return std::to_string(id) + " - " + name;
}

int country::getId()
{
    return id;
}

