#pragma once

#include "imgui.h"
#include "DatabaseConnection.h"
#include "brandRepository.h"
#include "carRepository.h"
#include "LogoCache.h"

#include <array>
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
    explicit MainWindow(ID3D11Device* graphicsDevice);

    void initialize();
    void render();
    bool shouldClose() const { return closeRequested; }

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

    // Database and repositories
    DatabaseConnection database;
    BrandRepository brandRepository;
    CarRepository carRepository;
    LogoCache logoCache;
    std::vector<Car> allCars;

    // Window state
    bool closeRequested = false;
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

    // Database operations
    void refreshData();
    void brandSelected(int brandId);
    void carSelected(int carId);
    void addCar(int brandId, const std::string& modelName, const std::string& typeName, int powerHP);
    void updateCar(int carId, const std::string& modelName, const std::string& typeName, int powerHP);
    void deleteCar(int carId);
    void reloadCarsForBrand(int brandId);

    // GUI helper functions
    void setBrands(const std::vector<BrandListItem>& newBrands);
    void setBrandDetails(const BrandDetails& details);
    void setCars(const std::vector<CarListItem>& newCars);
    void setCarDetails(const CarDetails& details);
    void clearBrandSelection();
    void setStatus(const std::string& message);
    void closeCarForm();

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
};
