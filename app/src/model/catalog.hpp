#pragma once

#include <map>
#include <string>
#include <vector>

namespace volta::model {

// Eintrag aus content/catalog/bauteile.json (docs/content-format.md, Bauteilkatalog).
struct CatalogEntry {
    std::string type;               // "led_rot"
    std::string model;              // Modellbezug im Kern: "led", "resistor", ...
    std::vector<std::string> pins;  // Anschlussnamen in Katalogreihenfolge
    std::map<std::string, double> params;
    int fromStage = 0;

    double param(const std::string& key, double fallback) const {
        const auto it = params.find(key);
        return it == params.end() ? fallback : it->second;
    }
};

struct ModuleEntry {
    std::string type;              // "blinkmodul"
    std::vector<std::string> pins; // Anschlussnamen in Leistenreihenfolge
    std::string kind;              // "teilschaltung", spaeter "makromodell", "verhaltensmodell"
    int fromStage = 0;
};

struct Catalog {
    std::map<std::string, CatalogEntry> parts;
    std::map<std::string, ModuleEntry> modules;

    const CatalogEntry* part(const std::string& type) const {
        const auto it = parts.find(type);
        return it == parts.end() ? nullptr : &it->second;
    }
    const ModuleEntry* module(const std::string& type) const {
        const auto it = modules.find(type);
        return it == modules.end() ? nullptr : &it->second;
    }
};

} // namespace volta::model
