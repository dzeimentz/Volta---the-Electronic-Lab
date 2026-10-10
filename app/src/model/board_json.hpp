#pragma once

#include <QJsonObject>

#include "board.hpp"

namespace volta::model {

// Aufbau als JSON (automatisches Speichern, services/session_store): verlustfrei inklusive Pultstellung, Werkzeugen,
// Spannungsanzeigen und Breite der Grundplatte. Unbekannte Schluessel werden ignoriert, fehlende bekommen die
// Vorgabewerte von Board; der Hauptschalter wird beim Laden nicht wieder eingeschaltet (Start im Steckmodus).
QJsonObject boardToJson(const Board& board);
bool boardFromJson(const QJsonObject& json, Board& out);

} // namespace volta::model
