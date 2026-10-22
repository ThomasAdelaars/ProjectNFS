#include "type.h"

type::type(int cId, const std::string& cName)
{
	id = cId;
	name = cName;
};

std::string type::toString()
{
	return std::to_string(id) + " - " + name;
}

int type::getId()
{
	return id;
}

