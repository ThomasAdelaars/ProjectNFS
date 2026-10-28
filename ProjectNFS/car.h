#pragma once

#include "entity.h"

#include <string>
#include <utility>

class Car : public Entity
{
public:
    int getBrandID() const { return brandID;}
    void setBrandID(int value) { brandID = value; }
    int getModelID() const
    {
        return modelID;
    }

    void setModelID(int value)
    {
        modelID = value;
    }

    const std::string& getModelName() const
    {
        return modelName;
    }

    void setModelName(std::string value)
    {
        modelName = std::move(value);
    }

    int getTypeID() const
    {
        return typeID;
    }

    void setTypeID(int value)
    {
        typeID = value;
    }

    const std::string& getTypeName() const
    {
        return typeName;
    }

    void setTypeName(std::string value)
    {
        typeName = std::move(value);
    }

    int getPowerHP() const
    {
        return powerHP;
    }

    void setPowerHP(int value)
    {
        powerHP = value;
    }

    std::string getDisplayName() const override
    {
        if (typeName.empty())
        {
            return modelName;
        }

        return modelName + " (" + typeName + ")";
    }

private:
    int modelID = 0;
    int brandID = 0;
    std::string modelName;
    int typeID = -1;        // -1 betekent dat er geen type gekoppeld is.
    std::string typeName;
    int powerHP = 0;
};