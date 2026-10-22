#pragma once
#include "entity.h"
#include <string>

class city : public IEntity {
public:
	std::string toString() override;
	int getId() override;
	city(int cId, const std::string& cName, const int cCountry)
	{
		id = cId;
		name = cName;
		countryId = cCountry;
	};
private:
	int id;
	std::string name;
	int countryId;
};