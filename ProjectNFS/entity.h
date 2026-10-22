#pragma once
#include <string>

class IEntity{
	public:
		virtual int getId() = 0;
		virtual std::string toString() = 0;
	private:
		const int id = 0;
};