#include "MainWindow.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
	constexpr float kMinimumBrandPaneWidth = 220.0f;
	constexpr float kPreferredBrandPaneWidth = 280.0f;
	constexpr float kMinimumBrandDetailsHeight = 175.0f;
	constexpr float kMaximumBrandDetailsHeight = 250.0f;
	constexpr float kLogoBoxWidth = 200.0f;
	constexpr float kLogoBoxHeight = 125.0f;
	constexpr float kMaximumFormFieldWidth = 360.0f;

	constexpr double kKilowattsPerPK = 0.73549875;

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
		const auto isNotSpace = [](unsigned char ch) { return std::isspace(ch) == 0; };
		const auto first = std::find_if(value.begin(), value.end(), isNotSpace);
		if (first == value.end())
			return {};

		const auto last = std::find_if(value.rbegin(), value.rend(), isNotSpace).base();
		return std::string(first, last);
	}
}

void MainWindow::setBrands(const std::vector<BrandListItem>& newBrands)
{
	brands = newBrands;

	const auto selected = std::find_if(brands.begin(), brands.end(),
		[this](const BrandListItem& brand) { return brand.id == selectedBrandId; });

	if (selected == brands.end())
		clearBrandSelection();
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
				if (onBrandSelected)
					onBrandSelected(brand.id);
			}
		}
		ImGui::PopID();
	}
	ImGui::EndChild();

	if (ImGui::Button("Merken vernieuwen", ImVec2(-1.0f, 0.0f)))
	{
		if (carFormMode != CarFormMode::None)
			status = "Sla de huidige auto op of annuleer eerst.";
		else if (onRefreshBrands) onRefreshBrands();
	}
	if (ImGui::Button("Afsluiten", ImVec2(-1.0f, 0.0f)))
	{
		if (onExitRequested) onExitRequested();
	}
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
				if (onCarSelected)
					onCarSelected(car.id);
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
				if (onDeleteCar && carId >= 0)
					onDeleteCar(carId);
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
			if (onAddCar)
				onAddCar(selectedBrandId, modelName, typeName, powerHPInput);
			else
				status = "Toevoegen is niet gekoppeld aan Application.";
		}
		else
		{
			if (onUpdateCar && selectedCarId >= 0)
				onUpdateCar(selectedCarId, modelName, typeName, powerHPInput);
			else
				status = "Aanpassen is niet gekoppeld aan Application.";
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
