#pragma once

#include <string>

#include "board.hpp"

namespace volta::model {

// Umgebung der Bauteile (Bedienhandlungen, kein Verlauf): Raumlicht sowie Taschenlampe und Foehn, die das Kind auf
// die Grundplatte legt (DeskState::tools). Lage in Feldeinheiten: col 1..12 (Spalte), row 0..4 (Zeile A..E).
// Ein Bauteil steht in der Mitte seiner Felder; die Wirkung eines Werkzeugs nimmt mit dem Abstand d (in
// Feldabstaenden) ab wie 1 / (1 + (d / d0)^2).
//  - Raumlicht: 0 dunkel 0,5 lx, 1 Daemmerung 20 lx, 2 hell 300 lx (Zimmer).
//  - Taschenlampe (eingeschaltet): bis 5000 lx direkt darueber (d0 = 0,5: ein Feld weiter ca. 1000 lx).
//  - Foehn (eingeschaltet): Luft bis 80 Grad C direkt davor (d0 = 0,7: ein Feld weiter ca. 40 Grad C).
inline constexpr double kRoomLux[3] = {0.5, 20.0, 300.0};
inline constexpr double kFlashLux = 5000.0;
inline constexpr double kFlashReach = 0.5;
inline constexpr double kDryerRiseK = 60.0;
inline constexpr double kDryerReach = 0.7;
inline constexpr const char* kToolFlashlight = "taschenlampe";
inline constexpr const char* kToolDryer = "foehn";

// Mitte eines Bauteils in Feldeinheiten; false, wenn es nicht auf Feldern der Grundplatte steckt
bool partCenter(const PartPlacement& part, double& col, double& row);
// Beleuchtungsstaerke am Bauteil in lx
double illuminanceAt(const Board& board, const PartPlacement& part);
// Lufttemperatur am Bauteil in K
double airTemperatureAt(const Board& board, const PartPlacement& part);

} // namespace volta::model
