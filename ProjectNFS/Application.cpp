#include "Application.h"

#include <algorithm>
#include <exception>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

Application::Application(ID3D11Device* graphicsDevice)
	: database("127.0.0.1", "needforspeed", "nfs", "underground"),
	brandRepository(database),
	carRepository(database),
	logoCache(std::make_unique<LogoCache>(graphicsDevice))
{
	bindCallbacks();
	mainWindow.setStatus("Database nog niet verbonden.");
}

void Application::bindCallbacks()
{
	mainWindow.onRefreshBrands = [this]() { refreshData(); };
	mainWindow.onBrandSelected = [this](int brandId) { handleBrandSelected(brandId); };
	mainWindow.onCarSelected = [this](int carId) { handleCarSelected(carId); };
	mainWindow.onAddCar = [this](int brandId, const std::string& modelName, const std::string& typeName, int powerHP)
		{
			handleAddCar(brandId, modelName, typeName, powerHP);
		};
	mainWindow.onUpdateCar = [this](int carId, const std::string& modelName, const std::string& typeName, int powerHP)
		{
			handleUpdateCar(carId, modelName, typeName, powerHP);
		};
	mainWindow.onDeleteCar = [this](int carId) { handleDeleteCar(carId); };
	mainWindow.onExitRequested = [this]()
		{
			closeRequested = true;
		};
}

void Application::initialize()
{
	refreshData();
}

void Application::render()
{
	mainWindow.render();
}

void Application::ensureConnected()
{
	if (!database.isConnected())
		database.connect();
}

void Application::refreshData()
{
	try
	{
		ensureConnected();
	}
	catch (const std::exception& e)
	{
		allCars.clear();
		mainWindow.clearBrandSelection();
		mainWindow.setStatus(std::string("Databaseverbinding mislukt: ") + e.what());
		return;
	}

	bool brandsLoaded = false;
	std::size_t brandCount = 0;

	try
	{
		const std::vector<Brand> loadedBrands = brandRepository.getAll();
		std::vector<BrandListItem> brandItems;
		brandItems.reserve(loadedBrands.size());

		for (const Brand& brand : loadedBrands)
			brandItems.push_back({ brand.getId(), brand.getName() });

		brandCount = brandItems.size();
		mainWindow.setBrands(brandItems);
		brandsLoaded = true;
	}
	catch (const std::exception& e)
	{
		mainWindow.setStatus(std::string("Merken laden mislukt: ") + e.what());
	}

	bool carsLoaded = false;
	try
	{
		allCars = carRepository.getAll();
		carsLoaded = true;
	}
	catch (const std::exception& e)
	{
		allCars.clear();
		mainWindow.setCars({});
		mainWindow.setStatus(std::string("Auto's laden mislukt: ") + e.what());
	}

	const int selectedBrandId = mainWindow.getSelectedBrandId();
	if (selectedBrandId >= 0 && brandsLoaded)
	{
		handleBrandSelected(selectedBrandId);
		if (!carsLoaded)
			mainWindow.setStatus("Merk geladen, maar de autolijst kon niet worden opgehaald.");
	}
	else if (selectedBrandId < 0)
	{
		mainWindow.clearBrandSelection();
		if (brandsLoaded && carsLoaded)
		{
			mainWindow.setStatus("Merken geladen: " + std::to_string(brandCount) +
				" | Auto's geladen: " + std::to_string(allCars.size()));
		}
		else if (brandsLoaded && !carsLoaded)
		{
			mainWindow.setStatus("Merken geladen: " + std::to_string(brandCount) +
				". Auto's konden niet worden geladen.");
		}
	}
}

void Application::handleBrandSelected(int brandId)
{
	try
	{
		ensureConnected();
		const Brand brand = brandRepository.getBrandDetails(brandId);

		BrandDetails details;
		details.id = brand.getId();
		details.name = brand.getName();
		details.cityName = brand.getCityName();
		details.countryName = brand.getCountryName();

		const auto& logoBytes = brand.getLogo();
		const LogoTexture texture = logoCache->get(
			brandId,
			logoBytes.empty() ? nullptr : logoBytes.data(),
			logoBytes.size());
		details.logoTexture = (ImTextureID)texture.srv;
		details.logoWidth = texture.width;
		details.logoHeight = texture.height;
		mainWindow.setBrandDetails(details);

		std::vector<CarListItem> carItems;
		for (const Car& car : allCars)
		{
			if (car.getBrandID() != brandId)
				continue;

			carItems.push_back({ car.getId(), car.getModelName(), car.getTypeName() });
		}

		mainWindow.setCars(carItems);
		mainWindow.setStatus("Merk geselecteerd: " + brand.getName() +
			" | Auto's: " + std::to_string(carItems.size()));
	}
	catch (const std::exception& e)
	{
		mainWindow.clearBrandSelection();
		mainWindow.setStatus(std::string("Merk laden mislukt: ") + e.what());
	}
}

void Application::handleCarSelected(int carId)
{
	const auto selectedCar = std::find_if(
		allCars.begin(), allCars.end(),
		[this, carId](const Car& car)
		{
			return car.getId() == carId &&
				car.getBrandID() == mainWindow.getSelectedBrandId();
		});

	if (selectedCar == allCars.end())
	{
		mainWindow.setCarDetails(CarDetails{});
		mainWindow.setStatus("De geselecteerde auto kon niet worden gevonden.");
		return;
	}

	CarDetails details;
	details.id = selectedCar->getId();
	details.modelName = selectedCar->getModelName();
	details.typeName = selectedCar->getTypeName();
	details.powerHP = selectedCar->getPowerHP();
	mainWindow.setCarDetails(details);
	mainWindow.setStatus("Auto geselecteerd: " + selectedCar->getDisplayName());
}

void Application::handleAddCar(int brandId, const std::string& modelName,
	const std::string& typeName, int powerHP)
{
	try
	{
		ensureConnected();
		if (brandId < 0 || brandId != mainWindow.getSelectedBrandId())
			throw std::runtime_error("Selecteer eerst een geldig automerk.");

		carRepository.addCar(brandId, modelName, typeName, powerHP);
		mainWindow.closeCarForm();
		reloadCarsForBrand(brandId);

		const auto addedCar = std::find_if(
			allCars.begin(), allCars.end(),
			[&](const Car& car)
			{
				return car.getBrandID() == brandId &&
					car.getModelName() == modelName &&
					car.getTypeName() == typeName &&
					car.getPowerHP() == powerHP;
			});
		if (addedCar != allCars.end())
			handleCarSelected(addedCar->getId());

		mainWindow.setStatus("Auto succesvol toegevoegd.");
	}
	catch (const std::exception& e)
	{
		mainWindow.setStatus(std::string("Auto toevoegen mislukt: ") + e.what());
	}
}

void Application::handleUpdateCar(int carId, const std::string& modelName,
	const std::string& typeName, int powerHP)
{
	try
	{
		ensureConnected();
		const auto selectedCar = std::find_if(
			allCars.begin(), allCars.end(),
			[this, carId](const Car& car)
			{
				return car.getId() == carId &&
					car.getBrandID() == mainWindow.getSelectedBrandId();
			});
		if (selectedCar == allCars.end())
			throw std::runtime_error("De geselecteerde auto hoort niet bij het geselecteerde merk.");

		const int brandId = selectedCar->getBrandID();
		carRepository.updateCar(carId, modelName, typeName, powerHP);
		mainWindow.closeCarForm();
		reloadCarsForBrand(brandId);
		handleCarSelected(carId);
		mainWindow.setStatus("Auto succesvol aangepast.");
	}
	catch (const std::exception& e)
	{
		mainWindow.setStatus(std::string("Auto aanpassen mislukt: ") + e.what());
	}
}

void Application::handleDeleteCar(int carId)
{
	try
	{
		ensureConnected();
		const auto selectedCar = std::find_if(
			allCars.begin(), allCars.end(),
			[this, carId](const Car& car)
			{
				return car.getId() == carId &&
					car.getBrandID() == mainWindow.getSelectedBrandId();
			});
		if (selectedCar == allCars.end())
			throw std::runtime_error("De geselecteerde auto kon niet worden gevonden.");

		const int brandId = selectedCar->getBrandID();
		carRepository.deleteCar(carId);
		mainWindow.closeCarForm();
		reloadCarsForBrand(brandId);
		mainWindow.setStatus("Auto succesvol verwijderd.");
	}
	catch (const std::exception& e)
	{
		mainWindow.setStatus(std::string("Auto verwijderen mislukt: ") + e.what());
	}
}

void Application::reloadCarsForBrand(int brandId)
{
	allCars = carRepository.getAll();
	handleBrandSelected(brandId);
}
