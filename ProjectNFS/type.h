#pragma once
#include "entity.h"
#include <string>

class type : public IEntity {
public:
	std::string toString() override;
	int getId() override;
	type(int cId, const std::string& cName)
	{
		id = cId;
		name = cName;
	};
private:
	int id;
	std::string name;
};