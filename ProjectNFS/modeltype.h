#pragma once
#include "entity.h"
#include <string>

class modeltype : public IEntity {
public:
	std::string toString() override;
	int getId() override;
	modeltype(int cId, int cModelId, int cTypeId, int cPowerHP);
private:
	int id;
	int modelId;
	int typeId;
	int PowerHP;
	int powerKW;
};