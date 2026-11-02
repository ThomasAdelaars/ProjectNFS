#include "MainWindow.h"

#include <exception>
#include <stdexcept>
#include <vector>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
	const float kMinimumBrandPaneWidth = 220.0f;
	const float kPreferredBrandPaneWidth = 280.0f;
	const float kMinimumBrandDetailsHeight = 175.0f;
	const float kMaximumBrandDetailsHeight = 250.0f;
	const float kLogoBoxWidth = 200.0f;
	const float kLogoBoxHeight = 125.0f;
	const float kMaximumFormFieldWidth = 360.0f;

	const double kKilowattsPerPK = 0.73549875;

	const char* valueOrDash(const std::string& value)
	{
		return value.empty() ? "-" : value.c_str();
	}

	void drawPlaceholderBox(const ImVec2& size)
	{
		const ImVec2 topLeft = ImGui::GetCursorScreenPos();
		ImGui::Dummy(size);
		ImGui::GetWindowDrawList()->AddRect(
			topLeft,
			ImVec2(topLeft.x + size.x, topLeft.y + size.y),
			ImGui::GetColorU32(ImGuiCol_Border));
	}

	void copyToBuffer(std::array<char, 128>& destination, const std::string& source)
	{
		destination.fill('\0');
		const std::size_t length = std::min(source.size(), destination.size() - 1);
		if (length > 0)
			std::memcpy(destination.data(), source.data(), length);
	}

	std::string trimWhitespace(std::string value)
	{
		std::size_t first = 0;
		while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
			first++;

		std::size_t last = value.size();
		while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
			last--;

		return value.substr(first, last - first);
	}
}

MainWindow::MainWindow(ID3D11Device* graphicsDevice)
	: database("127.0.0.1", "needforspeed", "nfs", "underground"),
	brandRepository(database),
	carRepository(database),
	logoCache(graphicsDevice)
{
	status = "Database nog niet verbonden.";
}

void MainWindow::initialize()
{
	refreshData();
}

void MainWindow::refreshData()
{
	bool brandsLoaded = false;
	std::size_t brandCount = 0;

	try
	{
		database.connect();
		const std::vector<Brand> loadedBrands = brandRepository.getAll();
		database.disconnect();
		std::vector<BrandListItem> brandItems;
		brandItems.reserve(loadedBrands.size());

		for (int i = 0; i < loadedBrands.size(); i++)
		{
			const Brand& brand = loadedBrands[i];

			brandItems.push_back({ brand.getId(), brand.getName() });
		}

		brandCount = brandItems.size();
		setBrands(brandItems);
		brandsLoaded = true;
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		setStatus(std::string("Merken laden mislukt: ") + e.what());
	}

	bool carsLoaded = false;
	try
	{
		database.connect();
		allCars = carRepository.getAll();
		database.disconnect();
		carsLoaded = true;
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		allCars.clear();
		setCars({});
		setStatus(std::string("Auto's laden mislukt: ") + e.what());
	}

	if (selectedBrandId >= 0 && brandsLoaded)
	{
		brandSelected(selectedBrandId);
		if (!carsLoaded)
		{
			setStatus("Merk geladen, maar de autolijst kon niet worden opgehaald.");
		}
	}
	else if (selectedBrandId < 0)
	{
		clearBrandSelection();
		if (brandsLoaded && carsLoaded)
		{
			setStatus("Merken geladen: " + std::to_string(brandCount) + " | Auto's geladen: " + std::to_string(allCars.size()));
		}
		else if (brandsLoaded && !carsLoaded)
		{
			setStatus("Merken geladen: " + std::to_string(brandCount) +
				". Auto's konden niet worden geladen.");
		}
	}
}

void MainWindow::brandSelected(int brandId)
{
	try
	{
		database.connect();
		const Brand brand = brandRepository.getBrandDetails(brandId);
		database.disconnect();

		BrandDetails details;
		details.id = brand.getId();
		details.name = brand.getName();
		details.cityName = brand.getCityName();
		details.countryName = brand.getCountryName();

		const auto& logoBytes = brand.getLogo();
		const LogoTexture texture = logoCache.get(
			brandId,
			logoBytes.empty() ? nullptr : logoBytes.data(),
			logoBytes.size());
		details.logoTexture = (ImTextureID)texture.srv;
		details.logoWidth = texture.width;
		details.logoHeight = texture.height;
		setBrandDetails(details);

		std::vector<CarListItem> carItems;
		for (std::size_t i = 0; i < allCars.size(); i++)
		{
			const Car& car = allCars[i];

			if (car.getBrandID() != brandId)
			{
				continue;
			}

			carItems.push_back({ car.getId(), car.getModelName(), car.getTypeName() });
		}

		setCars(carItems);
		setStatus("Merk geselecteerd: " + brand.getName() +
			" | Auto's: " + std::to_string(carItems.size()));
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		clearBrandSelection();
		setStatus(std::string("Merk laden mislukt: ") + e.what());
	}
}

void MainWindow::carSelected(int carId)
{
	const Car* selectedCar = nullptr;
	for (const Car& car : allCars)
	{
		if (car.getId() == carId && car.getBrandID() == selectedBrandId)
		{
			selectedCar = &car;
			break;
		}
	}

	if (selectedCar == nullptr)
	{
		setCarDetails(CarDetails{});
		setStatus("De geselecteerde auto kon niet worden gevonden.");
		return;
	}

	CarDetails details;
	details.id = selectedCar->getId();
	details.modelName = selectedCar->getModelName();
	details.typeName = selectedCar->getTypeName();
	details.powerHP = selectedCar->getPowerHP();
	setCarDetails(details);
	setStatus("Auto geselecteerd: " + selectedCar->getDisplayName());
}

void MainWindow::addCar(int brandId, const std::string& modelName, const std::string& typeName, int powerHP)
{
	try
	{
		if (brandId < 0 || brandId != selectedBrandId)
		{
			throw std::runtime_error("Selecteer eerst een geldig automerk.");
		}

		database.connect();
		carRepository.addCar(brandId, modelName, typeName, powerHP);
		database.disconnect();
		closeCarForm();
		reloadCarsForBrand(brandId);

		int addedCarId = -1;
		for (const Car& car : allCars)
		{
			if (car.getBrandID() == brandId &&
				car.getModelName() == modelName &&
				car.getTypeName() == typeName &&
				car.getPowerHP() == powerHP)
			{
				addedCarId = car.getId();
				break;
			}
		}

		if (addedCarId >= 0)
		{
			carSelected(addedCarId);
		}

		setStatus("Auto succesvol toegevoegd.");
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		setStatus(std::string("Auto toevoegen mislukt: ") + e.what());
	}
}

void MainWindow::updateCar(int carId, const std::string& modelName, const std::string& typeName, int powerHP)
{
	try
	{
		const Car* selectedCar = nullptr;
		for (const Car& car : allCars)
		{
			if (car.getId() == carId && car.getBrandID() == selectedBrandId)
			{
				selectedCar = &car;
				break;
			}
		}

		if (selectedCar == nullptr)
		{
			throw std::runtime_error("De geselecteerde auto hoort niet bij het geselecteerde merk.");
		}
		const int brandId = selectedCar->getBrandID();
		database.connect();
		carRepository.updateCar(carId, modelName, typeName, powerHP);
		database.disconnect();
		closeCarForm();
		reloadCarsForBrand(brandId);
		carSelected(carId);
		setStatus("Auto succesvol aangepast.");
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		setStatus(std::string("Auto aanpassen mislukt: ") + e.what());
	}
}

void MainWindow::deleteCar(int carId)
{
	try
	{
		const Car* selectedCar = nullptr;
		for (const Car& car : allCars)
		{
			if (car.getId() == carId && car.getBrandID() == selectedBrandId)
			{
				selectedCar = &car;
				break;
			}
		}

		if (selectedCar == nullptr)
			throw std::runtime_error("De geselecteerde auto kon niet worden gevonden.");

		const int brandId = selectedCar->getBrandID();
		database.connect();
		carRepository.deleteCar(carId);
		database.disconnect();
		closeCarForm();
		reloadCarsForBrand(brandId);
		setStatus("Auto succesvol verwijderd.");
	}
	catch (const std::exception& e)
	{
		database.disconnect();
		setStatus(std::string("Auto verwijderen mislukt: ") + e.what());
	}
}

void MainWindow::reloadCarsForBrand(int brandId)
{
	database.connect();
	allCars = carRepository.getAll();
	database.disconnect();

	brandSelected(brandId);
}

void MainWindow::setBrands(const std::vector<BrandListItem>& newBrands)
{
	brands = newBrands;

	bool selectedBrandExists = false;
	for (const BrandListItem& brand : brands)
	{
		if (brand.id == selectedBrandId)
		{
			selectedBrandExists = true;
		}
	}

	if (!selectedBrandExists) 
	{
		clearBrandSelection();
	}
}

void MainWindow::setBrandDetails(const BrandDetails& details)
{
	brandDetails = details;
	selectedBrandId = details.id;
}

void MainWindow::setCars(const std::vector<CarListItem>& newCars)
{
	cars = newCars;
	selectedCarId = -1;
	carDetails = CarDetails{};
	cancelCarForm();
}

void MainWindow::setCarDetails(const CarDetails& details)
{
	carDetails = details;
	selectedCarId = details.id;
	carFormMode = CarFormMode::None;
	copyCarDetailsToInputs();
}

void MainWindow::clearBrandSelection()
{
	selectedBrandId = -1;
	selectedCarId = -1;
	brandDetails = BrandDetails{};
	carDetails = CarDetails{};
	cars.clear();
	cancelCarForm();
}

void MainWindow::setStatus(const std::string& message)
{
	status = message;
}

void MainWindow::closeCarForm()
{
	cancelCarForm();
}

void MainWindow::render()
{
	const ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);

	constexpr ImGuiWindowFlags windowFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	ImGui::Begin("Need For Speed - Car Manager", nullptr, windowFlags);

	const ImVec2 available = ImGui::GetContentRegionAvail();
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float brandPaneWidth = std::min(
		std::max(kMinimumBrandPaneWidth, kPreferredBrandPaneWidth),
		std::max(kMinimumBrandPaneWidth, available.x * 0.30f));
	const float rightPaneWidth = std::max(0.0f, available.x - brandPaneWidth - spacing);

	ImGui::BeginChild("BrandPane", ImVec2(brandPaneWidth, available.y), true);
	renderBrandList(available.y);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("RightPane", ImVec2(rightPaneWidth, available.y), false);
	const ImVec2 rightAvailable = ImGui::GetContentRegionAvail();
	const float footerHeight = ImGui::GetTextLineHeightWithSpacing();
	const float contentHeight = std::max(0.0f, rightAvailable.y - footerHeight - ImGui::GetStyle().ItemSpacing.y);
	const float brandDetailsHeight = std::min(kMaximumBrandDetailsHeight, std::max(kMinimumBrandDetailsHeight, contentHeight * 0.30f));

	renderBrandDetails(brandDetailsHeight);
	ImGui::Spacing();

	const ImVec2 bottomAvailable = ImGui::GetContentRegionAvail();
	const float bottomPanelHeight = std::max(0.0f, contentHeight - brandDetailsHeight - ImGui::GetStyle().ItemSpacing.y);
	const float carListWidth = std::max(180.0f, bottomAvailable.x * 0.48f);

	ImGui::BeginChild("CarsPane", ImVec2(carListWidth, bottomPanelHeight), true);
	renderCarList(carListWidth);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("CarDetailsPane", ImVec2(0.0f, bottomPanelHeight), true);
	renderCarDetails();
	ImGui::EndChild();

	ImGui::TextDisabled("%s", status.c_str());
	ImGui::EndChild();

	ImGui::End();
}

void MainWindow::renderBrandList(float availableHeight)
{
	ImGui::TextUnformatted("Automerken");
	ImGui::Separator();

	const float buttonHeight = 2.0f * ImGui::GetFrameHeightWithSpacing();
	const float listHeight = std::max(0.0f, availableHeight - ImGui::GetCursorPosY() - buttonHeight - ImGui::GetStyle().ItemSpacing.y);

	ImGui::BeginChild("BrandList", ImVec2(0.0f, listHeight), false);
	for (const BrandListItem& brand : brands)
	{
		ImGui::PushID(brand.id);
		if (ImGui::Selectable(brand.name.c_str(), selectedBrandId == brand.id))
		{
			if (carFormMode != CarFormMode::None)
			{
				status = "Sla de huidige auto op of annuleer eerst.";
			}
			else
			{
				selectedBrandId = brand.id;
				selectedCarId = -1;
				brandDetails = BrandDetails{};
				carDetails = CarDetails{};
				cars.clear();
				brandSelected(brand.id);
			}
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (ImGui::Button("Afsluiten", ImVec2(-1.0f, 0.0f)))
		closeRequested = true;
}

void MainWindow::renderBrandDetails(float panelHeight)
{
	ImGui::BeginChild("BrandDetailsPanel", ImVec2(0.0f, panelHeight), true);
	ImGui::TextUnformatted("Merkdetails");
	ImGui::Separator();

	ImGui::BeginChild("BrandLogoArea", ImVec2(kLogoBoxWidth + 12.0f, 0.0f), false);
	renderLogo(kLogoBoxWidth, kLogoBoxHeight);
	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginChild("BrandTextArea", ImVec2(0.0f, 0.0f), false);
	ImGui::Text("Merk: %s", valueOrDash(brandDetails.name));
	ImGui::Spacing();
	ImGui::Text("Stad: %s", valueOrDash(brandDetails.cityName));
	ImGui::Text("Land: %s", valueOrDash(brandDetails.countryName));
	ImGui::EndChild();

	ImGui::EndChild();
}

void MainWindow::renderLogo(float availableWidth, float availableHeight)
{
	const ImVec2 box(availableWidth, availableHeight);
	const ImVec2 topLeft = ImGui::GetCursorScreenPos();

	if (brandDetails.logoTexture != (ImTextureID)0 &&
		brandDetails.logoWidth > 0 && brandDetails.logoHeight > 0)
	{
		const float scaleX = box.x / static_cast<float>(brandDetails.logoWidth);
		const float scaleY = box.y / static_cast<float>(brandDetails.logoHeight);
		const float scale = std::min(scaleX, scaleY);
		const ImVec2 imageSize(
			static_cast<float>(brandDetails.logoWidth) * scale,
			static_cast<float>(brandDetails.logoHeight) * scale);
		const ImVec2 imagePos(
			topLeft.x + (box.x - imageSize.x) * 0.5f,
			topLeft.y + (box.y - imageSize.y) * 0.5f);

		ImGui::SetCursorScreenPos(imagePos);
		ImGui::Image(brandDetails.logoTexture, imageSize);
		ImGui::SetCursorScreenPos(topLeft);
		ImGui::Dummy(box);
	}
	else
	{
		drawPlaceholderBox(box);
		const char* placeholder = brandDetails.name.empty() ? "Geen logo" : "Logo niet beschikbaar";
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(topLeft.x + 8.0f, topLeft.y + box.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f),
			ImGui::GetColorU32(ImGuiCol_TextDisabled), placeholder);
	}
}

void MainWindow::renderCarList(float panelWidth)
{
	(void)panelWidth;
	ImGui::TextUnformatted("Auto's van geselecteerd merk");
	ImGui::Separator();

	if (selectedBrandId < 0)
	{
		ImGui::TextWrapped("Selecteer links een automerk om de bijbehorende auto's te bekijken.");
		return;
	}

	if (cars.empty())
	{
		ImGui::TextDisabled("Geen auto's gevonden voor dit merk.");
		return;
	}

	for (const CarListItem& car : cars)
	{
		std::string label = car.modelName;
		if (!car.typeName.empty())
			label += " - " + car.typeName;

		ImGui::PushID(car.id);
		if (ImGui::Selectable(label.c_str(), selectedCarId == car.id))
		{
			if (carFormMode != CarFormMode::None)
			{
				status = "Sla de huidige auto op of annuleer eerst.";
			}
			else
			{
				selectedCarId = car.id;
				carDetails = CarDetails{};
				carSelected(car.id);
			}
		}
		ImGui::PopID();
	}
}

void MainWindow::renderCarDetails()
{
	ImGui::TextUnformatted("Autodetails");
	ImGui::Separator();

	if (selectedBrandId < 0)
	{
		ImGui::TextWrapped("Selecteer eerst een automerk.");
		return;
	}

	if (carFormMode == CarFormMode::None)
	{
		if (ImGui::Button("Toevoegen"))
			beginAddCar();

		if (carFormMode == CarFormMode::None && selectedCarId >= 0 && carDetails.id >= 0)
		{
			ImGui::SameLine();
			if (ImGui::Button("Bewerken"))
				beginEditCar();

			if (carFormMode == CarFormMode::None)
			{
				ImGui::SameLine();
				if (ImGui::Button("Verwijderen"))
					ImGui::OpenPopup("Bevestig verwijderen");
			}
		}

		if (carFormMode == CarFormMode::None &&
			ImGui::BeginPopupModal("Bevestig verwijderen", nullptr,
				ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextWrapped("Weet je zeker dat je deze auto wilt verwijderen?");
			ImGui::Spacing();
			if (ImGui::Button("Ja, verwijderen", ImVec2(140.0f, 0.0f)))
			{
				const int carId = selectedCarId;
				if (carId >= 0)
					deleteCar(carId);
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Annuleren", ImVec2(110.0f, 0.0f)))
				ImGui::CloseCurrentPopup();
			ImGui::EndPopup();
		}
	}

	if (carFormMode != CarFormMode::None)
	{
		ImGui::Spacing();
		renderCarForm();
		return;
	}

	if (selectedCarId < 0 || carDetails.id < 0)
	{
		ImGui::Spacing();
		ImGui::TextWrapped("Selecteer een auto om de details te bekijken of te bewerken.");
		return;
	}

	ImGui::Spacing();
	ImGui::Text("Model: %s", valueOrDash(carDetails.modelName));
	ImGui::Text("Type: %s", valueOrDash(carDetails.typeName));
	ImGui::Spacing();
	ImGui::TextUnformatted("Vermogenseenheid:");
	ImGui::SameLine();
	renderPowerUnitSelector();

	if (powerUnit == PowerUnit::PK)
	{
		ImGui::Text("Vermogen: %d PK", carDetails.powerHP);
	}
	else
	{
		const double kilowatts = static_cast<double>(carDetails.powerHP) * kKilowattsPerPK;
		ImGui::Text("Vermogen: %.1f kW", kilowatts);
	}
}

void MainWindow::renderCarForm()
{
	const bool isAdding = carFormMode == CarFormMode::Add;
	ImGui::TextUnformatted(isAdding ? "Nieuwe auto" : "Auto aanpassen");
	ImGui::Separator();

	const float availableWidth = ImGui::GetContentRegionAvail().x;
	const float fieldWidth = std::max(0.0f, std::min(kMaximumFormFieldWidth, availableWidth));

	ImGui::TextUnformatted("Modelnaam");
	ImGui::SetNextItemWidth(fieldWidth);
	ImGui::InputText("##Modelnaam", modelNameInput.data(), modelNameInput.size());

	ImGui::TextUnformatted("Type (optioneel)");
	ImGui::SetNextItemWidth(fieldWidth);
	ImGui::InputText("##Type", typeNameInput.data(), typeNameInput.size());

	ImGui::Spacing();
	ImGui::TextUnformatted("Vermogenseenheid:");
	ImGui::SameLine();
	renderPowerUnitSelector();

	if (powerUnit == PowerUnit::PK)
	{
		int displayedHP = powerHPInput;
		ImGui::TextUnformatted("Vermogen (PK)");
		ImGui::SetNextItemWidth(std::min(220.0f, fieldWidth));
		if (ImGui::InputInt("##PowerPK", &displayedHP, 1, 10))
			powerHPInput = displayedHP;
	}
	else
	{
		float displayedKW = static_cast<float>(static_cast<double>(powerHPInput) * kKilowattsPerPK);
		ImGui::TextUnformatted("Vermogen (kW)");
		ImGui::SetNextItemWidth(std::min(220.0f, fieldWidth));
		if (ImGui::InputFloat("##PowerKW", &displayedKW, 1.0f, 10.0f, "%.1f"))
		{
			if (std::isfinite(displayedKW) && displayedKW >= 0.0f && displayedKW <= 100000.0f)
			{
				const double convertedPK = static_cast<double>(displayedKW) / kKilowattsPerPK;
				powerHPInput = static_cast<int>(std::lround(convertedPK));
			}
		}
	}

	ImGui::Spacing();
	if (ImGui::Button(isAdding ? "Auto toevoegen" : "Wijzigingen opslaan", ImVec2(170.0f, 0.0f)))
	{
		const std::string modelName = trimWhitespace(std::string(modelNameInput.data()));

		const std::string typeName = trimWhitespace(std::string(typeNameInput.data()));

		if (modelName.empty())
		{
			status = "Vul een modelnaam in.";
		}
		else if (powerHPInput <= 0)
		{
			status = "Vermogen moet groter zijn dan 0 PK.";
		}
		else if (isAdding)
		{
			addCar(selectedBrandId, modelName, typeName, powerHPInput);
		}
		else if (selectedCarId >= 0)
		{
			updateCar(selectedCarId, modelName, typeName, powerHPInput);
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Annuleren", ImVec2(110.0f, 0.0f))) cancelCarForm();
}

void MainWindow::renderPowerUnitSelector()
{
	if (ImGui::RadioButton("PK", powerUnit == PowerUnit::PK))
		powerUnit = PowerUnit::PK;

	ImGui::SameLine();

	if (ImGui::RadioButton("kW", powerUnit == PowerUnit::KW))
		powerUnit = PowerUnit::KW;
}

void MainWindow::beginAddCar()
{
	carFormMode = CarFormMode::Add;
	modelNameInput.fill('\0');
	typeNameInput.fill('\0');
	powerHPInput = 100;
	status = "Vul de gegevens van de nieuwe auto in.";
}

void MainWindow::beginEditCar()
{
	if (selectedCarId < 0 || carDetails.id < 0)
		return;

	carFormMode = CarFormMode::Edit;
	copyCarDetailsToInputs();
	status = "Pas de gegevens aan en sla de wijzigingen op.";
}

void MainWindow::cancelCarForm()
{
	carFormMode = CarFormMode::None;
	copyCarDetailsToInputs();
}

void MainWindow::copyCarDetailsToInputs()
{
	copyToBuffer(modelNameInput, carDetails.modelName);
	copyToBuffer(typeNameInput, carDetails.typeName);
	powerHPInput = carDetails.powerHP > 0 ? carDetails.powerHP : 100;
}
