#include "modeltype.h"

modeltype::modeltype(int cId, int cModelId, int cTypeId, int cPowerHP)
{
	id = cId;
	modelId = cModelId;
	typeId = cTypeId;
	PowerHP = cPowerHP;
	powerKW = cPowerHP * 0.735498753;
};

std::string modeltype::toString()
{
	return std::to_string(id) + " - " + std::to_string(modelId) + " - " + std::to_string(typeId) + " - " + std::to_string(PowerHP) + " - " + std::to_string(powerKW);
}

int modeltype::getId()
{
	return id;
}
