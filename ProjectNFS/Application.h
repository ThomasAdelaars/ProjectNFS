#pragma once

#include "DatabaseConnection.h"
#include "MainWindow.h"
#include "brandRepository.h"
#include "carRepository.h"
#include "LogoCache.h"

#include <d3d11.h>
#include <memory>
#include <string>
#include <vector>


class Application
{
public:
    explicit Application(ID3D11Device* graphicsDevice);
    ~Application() = default;

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    bool shouldClose() const { return closeRequested; }

    void initialize();
    void render();

private:
    DatabaseConnection database;
    BrandRepository brandRepository;
    CarRepository carRepository;

    MainWindow mainWindow;
    std::vector<Car> allCars;
    std::unique_ptr<LogoCache> logoCache;

    bool closeRequested = false;
    void bindCallbacks();
    void refreshData();
    void ensureConnected();
    void handleBrandSelected(int brandId);
    void handleCarSelected(int carId);
    void handleAddCar(int brandId, const std::string& modelName, const std::string& typeName, int powerHP);
    void handleUpdateCar(int carId, const std::string& modelName, const std::string& typeName, int powerHP);
    void handleDeleteCar(int carId);
    void reloadCarsForBrand(int brandId);
};
