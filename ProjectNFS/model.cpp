#include "model.h"

model::model(int cId, const std::string& cName, const int cBrandId)
{
	id = cId;
	name = cName;
	brandId = cBrandId;
};

std::string model::toString()
{
	return name;
}

int model::getId()
{
	return id;
}

