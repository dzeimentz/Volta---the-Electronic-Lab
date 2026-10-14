#pragma once

// Bausteinformat (JSON, deutsche Schluessel) nach docs/Konzept_Zuse.md Abschnitt 8.3
#include <QJsonObject>

#include "volta/logic/chip.hpp"

namespace zuse {

QJsonObject chipToJson(const volta::logic::ChipDef& def);
// Liefert false bei fehlenden Pflichtangaben; unbekannte Schluessel werden ignoriert.
bool chipFromJson(const QJsonObject& obj, volta::logic::ChipDef& def);

} // namespace zuse
