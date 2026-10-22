#pragma once
#include "entity.h"
#include <string>

class country : public IEntity {
	public:
		std::string toString() override;
		int getId() override;
		country(int cId, const std::string& cName);
	private:
		int id;
		std::string name;
};