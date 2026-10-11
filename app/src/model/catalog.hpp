#pragma once

#include <cmath>
#include <map>
#include <string>
#include <vector>

namespace volta::model {

// Eintrag aus content/catalog/bauteile.json (docs/content-format.md, Bauteilkatalog).
struct CatalogEntry {
    std::string type;               // "led_rot"
    std::string model;              // Modellbezug im Kern: "led", "resistor", ...
    std::vector<std::string> pins;  // Anschlussnamen in Katalogreihenfolge
    std::vector<std::string> spans; // "kurz", "lang", "traeger" (3 Felder in Reihe) bzw. "platine_2x2"
    std::vector<std::string> layout; // Traegerplatine: Anschluesse in Lage-Reihenfolge (leer: wie pins);
                                     // platine_2x2: oben links, oben rechts, unten links, unten rechts
    std::map<std::string, double> params;
    // Auswahlvarianten ("varianten"), z. B. Gluehlampe 2,5 / 6 / 10 V; der Wert des Bauteils (PartPlacement::value)
    // waehlt die Variante ueber u_nenn_V. Leer: keine Auswahl.
    std::vector<std::map<std::string, double>> variants;
    std::string lexiconKey; // "lexikon_key": Text im Bauteillexikon
    int fromStage = 0;

    double param(const std::string& key, double fallback) const {
        const auto it = params.find(key);
        return it == params.end() ? fallback : it->second;
    }
    // Parameter der Variante mit variants[i][selector] == value, sonst der allgemeine Parameter
    double variantParam(const std::string& selector, double value, const std::string& key, double fallback) const {
        for (const auto& v : variants) {
            const auto s = v.find(selector);
            const auto k = v.find(key);
            if (s != v.end() && k != v.end() && std::abs(s->second - value) < 1e-9) return k->second;
        }
        return param(key, fallback);
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

// Leuchtdioden sind in der Farbe austauschbar, solange ein Experiment die Farbe nicht festlegt (»farbe_fest«):
// Vorlage mit roter LED, Kind nimmt eine gruene - die Schaltung funktioniert gleich (nur die Flussspannung
// unterscheidet sich etwas). Gilt fuer Erfolgsbedingung, »Fertig« und Schritt-fuer-Schritt-Hilfe.
inline bool isLed(const std::string& type) { return type.rfind("led_", 0) == 0; }
inline bool sameKind(const std::string& a, const std::string& b, bool colourFixed) {
    return a == b || (!colourFixed && isLed(a) && isLed(b));
}

} // namespace volta::model
