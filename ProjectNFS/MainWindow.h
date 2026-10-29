#pragma once

#include "imgui.h"

#include <array>
#include <functional>
#include <string>
#include <vector>

struct BrandListItem
{
    int id = -1;
    std::string name;
};

struct CarListItem
{
    int id = -1;
    std::string modelName;
    std::string typeName;
};

struct BrandDetails
{
    int id = -1;
    std::string name;
    std::string cityName;
    std::string countryName;
    ImTextureID logoTexture = (ImTextureID)0;
    int logoWidth = 0;
    int logoHeight = 0;
};

struct CarDetails
{
    int id = -1;
    std::string modelName;
    std::string typeName;
    int powerHP = 0;
};

class MainWindow
{
public:
    MainWindow() = default;

    void render();

    void setBrands(const std::vector<BrandListItem>& newBrands);
    void setBrandDetails(const BrandDetails& details);
    void setCars(const std::vector<CarListItem>& newCars);
    void setCarDetails(const CarDetails& details);
    void clearBrandSelection();
    void setStatus(const std::string& message);
    void closeCarForm();

    int getSelectedBrandId() const { return selectedBrandId; }
    int getSelectedCarId() const { return selectedCarId; }

    std::function<void()> onRefreshBrands;
    std::function<void(int brandId)> onBrandSelected;
    std::function<void(int carId)> onCarSelected;
    std::function<void()> onExitRequested;
    std::function<void(int brandId, const std::string& modelName, const std::string& typeName, int powerHP)> onAddCar;
    std::function<void(int carId, const std::string& modelName, const std::string& typeName, int powerHP)> onUpdateCar;
    std::function<void(int carId)> onDeleteCar;

private:
    enum class CarFormMode
    {
        None,
        Add,
        Edit
    };

    enum class PowerUnit
    {
        PK,
        KW
    };

    void renderBrandList(float availableHeight);
    void renderBrandDetails(float panelHeight);
    void renderCarList(float panelWidth);
    void renderCarDetails();
    void renderCarForm();
    void renderPowerUnitSelector();
    void renderLogo(float availableWidth, float availableHeight);

    void beginAddCar();
    void beginEditCar();
    void cancelCarForm();
    void copyCarDetailsToInputs();

    std::vector<BrandListItem> brands;
    std::vector<CarListItem> cars;

    BrandDetails brandDetails;
    CarDetails carDetails;

    int selectedBrandId = -1;
    int selectedCarId = -1;
    CarFormMode carFormMode = CarFormMode::None;

    std::array<char, 128> modelNameInput{};
    std::array<char, 128> typeNameInput{};
    int powerHPInput = 100;
    PowerUnit powerUnit = PowerUnit::PK;

    std::string status = "Gereed";
};
