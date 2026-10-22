#pragma once
#include "entity.h"
#include <string>

class model : public IEntity {
public:
	std::string toString() override;
	int getId() override;
	model(int cId, const std::string& cName, const int cBrandId)
	{
		id = cId;
		name = cName;
		brandId = cBrandId;
	};
private:
	int id;
	std::string name;
	int brandId;
};