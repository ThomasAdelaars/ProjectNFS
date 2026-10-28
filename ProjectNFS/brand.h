#pragma once
#include "entity.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Brand : public Entity
{
public:
    const std::string& getName() const
    {
        return name;
    }

    void setName(std::string value)
    {
        name = std::move(value);
    }

    const std::vector<std::uint8_t>& getLogo() const
    {
        return logo;
    }

    void setLogo(std::vector<std::uint8_t> value)
    {
        logo = std::move(value);
    }

    const std::string& getCityName() const
    {
        return cityName;
    }

    void setCityName(std::string value)
    {
        cityName = std::move(value);
    }

    const std::string& getCountryName() const
    {
        return countryName;
    }

    void setCountryName(std::string value)
    {
        countryName = std::move(value);
    }

    std::string getDisplayName() const override
    {
        return name;
    }
private:
    std::string name;
    std::vector<std::uint8_t> logo;
    std::string cityName;
    std::string countryName;
};