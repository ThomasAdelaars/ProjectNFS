#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <functional>

struct BrandItem
{
    int         id = -1;
    std::string name;
};

struct CarItem
{
    int         id = -1;
    std::string label;   // "Focus - Sedan - 125 PK"
};

class CarBrandUI
{
public:
    CarBrandUI();

    // data injection (called from main)
    void setBrands(const std::vector<BrandItem>& newBrands);
    void setCars(const std::vector<CarItem>& newCars);
    void setBrandInfo(const std::string& merk,
        const std::string& founded,
        const std::string& country,
        const std::string& models);
    void setStatus(const std::string& msg) { status = msg; }

    // called every frame
    void render();

    // callbacks – main hooks the database logic in here
    std::function<void(int brandId)> onBrandSelected;
    std::function<void()>            onRefresh;
    std::function<void(int carId)>   onCarSelected;
    void setLogo(ImTextureID tex, int w, int h) { logo_tex = tex; logo_w = w; logo_h = h; }

private:
    ImTextureID logo_tex = (ImTextureID)0;
    int logo_w = 0, logo_h = 0;    std::vector<BrandItem> brands;
    std::vector<CarItem>   cars;

    int selected_brand = -1;   // index in brands
    int selected_car = -1;   // index in cars

    std::string info_merk;
    std::string info_founded;
    std::string info_country;
    std::string info_models;
    std::string status;
};