#include "CarBrandUI.h"

CarBrandUI::CarBrandUI()
    : info_merk("Merk: -")
    , info_founded("Stad: -")
    , info_country("Land: -")
    , status("Klaar")
{
}

void CarBrandUI::setBrands(const std::vector<BrandItem>& newBrands)
{
    brands = newBrands;
    selected_brand = -1;
    cars.clear();
    selected_car = -1;
}

void CarBrandUI::setCars(const std::vector<CarItem>& newCars)
{
    cars = newCars;
    selected_car = -1;
}

void CarBrandUI::setBrandInfo(const std::string& merk, const std::string& founded,
    const std::string& country, const std::string& models)
{
    info_merk = merk;
    info_founded = founded;
    info_country = country;
}

void CarBrandUI::render()
{
    ImGui::SetNextWindowSize(ImVec2(1280, 760), ImGuiCond_FirstUseEver);
    ImGui::Begin("Car Brand Manager");

    ImGui::Columns(2, "main_columns", true);
    ImGui::SetColumnWidth(0, 300);

    // ---------------- LEFT: brand list ----------------
    ImGui::Text("Lijst met merken");
    ImGui::Separator();

    ImGui::BeginChild("BrandsList", ImVec2(0, 400), true);
    for (int i = 0; i < (int)brands.size(); ++i)
    {
        ImGui::PushID(i);                       // unique id, names can repeat
        if (ImGui::Selectable(brands[i].name.c_str(), selected_brand == i))
        {
            selected_brand = i;
            if (onBrandSelected)
                onBrandSelected(brands[i].id);  // <-- database call happens here
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    if (ImGui::Button("Verversen", ImVec2(-1, 0)) && onRefresh)
        onRefresh();

    ImGui::NextColumn();

    // ---------------- RIGHT: info ----------------
    ImGui::BeginChild("TopSection", ImVec2(0, 280), true);
    {
        ImGui::Columns(2, "top_columns", true);
        ImGui::SetColumnWidth(0, 300);

        ImGui::Text("Logo");

        const ImVec2 box(250.0f, 170.0f);
        ImVec2 p = ImGui::GetCursorScreenPos();

        if (logo_tex && logo_w > 0 && logo_h > 0)
        {
            // letterbox: keep aspect ratio inside the 250x170 box
            float scale = (box.x / logo_w < box.y / logo_h) ? box.x / logo_w
                : box.y / logo_h;
            ImVec2 draw(logo_w * scale, logo_h * scale);

            // centre it
            ImGui::SetCursorScreenPos(ImVec2(p.x + (box.x - draw.x) * 0.5f,
                p.y + (box.y - draw.y) * 0.5f));
            ImGui::Image(logo_tex, draw);

            ImGui::SetCursorScreenPos(ImVec2(p.x, p.y));
            ImGui::Dummy(box);                  // reserve the full box for layout
        }
        else
        {
            ImGui::Dummy(box);
            ImGui::GetWindowDrawList()->AddRect(
                p, ImVec2(p.x + box.x, p.y + box.y),
                ImGui::GetColorU32(ImGuiCol_Border));
        }

        ImGui::NextColumn();

        ImGui::Text("Info merk");
        ImGui::Separator();
        ImGui::TextUnformatted(info_merk.c_str());     // safe, no format parsing
        ImGui::TextUnformatted(info_founded.c_str());
        ImGui::TextUnformatted(info_country.c_str());

        ImGui::Columns(1);
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Button("toevoegen", ImVec2(100, 0)); ImGui::SameLine();
    ImGui::Button("bewerken", ImVec2(100, 0)); ImGui::SameLine();
    ImGui::Button("verwijderen", ImVec2(100, 0)); ImGui::SameLine();
    if (ImGui::Button("verversen", ImVec2(100, 0)) && onRefresh) onRefresh();

    ImGui::Separator();

    // ---------------- BOTTOM: car list ----------------
    ImGui::Text("Lijst met auto's");
    ImGui::BeginChild("CarsList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
    for (int i = 0; i < (int)cars.size(); ++i)
    {
        ImGui::PushID(i);
        if (ImGui::Selectable(cars[i].label.c_str(), selected_car == i))
        {
            selected_car = i;
            if (onCarSelected) onCarSelected(cars[i].id);
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::TextUnformatted(status.c_str());

    ImGui::Columns(1);
    ImGui::End();
}