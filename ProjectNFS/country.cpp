#include "country.h"

country::country(int cId, const std::string& cName)
{
	id = cId;
	name = cName;
};

std::string country::toString()
{
    return name;
}

int country::getId()
{
    return id;
}

