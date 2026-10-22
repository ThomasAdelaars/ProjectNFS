#pragma once
#include "entity.h"
#include <string>
#include <vector>

class brand : public IEntity {
public:
	std::string toString() override;
	int getId() override;
	brand(int cId, const std::string& cName, const int cCity, const std::vector<char>& cLogo)
	{
		id = cId;
		name = cName;
		cityId = cCity;
		logo = cLogo;
	};
private:
	int id;
	std::string name;
	int cityId;
	std::vector<char> logo;
	
};