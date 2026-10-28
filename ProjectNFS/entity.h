#pragma once
#include <string>

class Entity
{
public:
	virtual ~Entity() = default;

	int getId() const
	{
		return id;
	}

	void setId(int value)
	{
		id = value;
	}

	virtual std::string getDisplayName() const = 0;

protected:
	int id = 0;
};