#pragma once
#include <string>

class Entity
{
public:
    virtual ~Entity() = default;

    virtual int getId() const = 0;
    virtual void setId(int value) = 0;
    virtual std::string getDisplayName() const = 0;
};