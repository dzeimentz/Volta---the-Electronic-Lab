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
inline constexpr const char* kToolWater = "wasser";
inline constexpr double kWaterReach = 0.6; // Wassertropfen: nass direkt darunter, ein Feld weiter kaum
// Abdeckkarte (ab Volta 2): schattet ab, was direkt darunter liegt (Licht und IR-Strahlung); d0 = 0,45: direkt
// darunter kommen 0,5 % an, ein Feld weiter 84 %
inline constexpr const char* kToolCard = "karte";
inline constexpr double kCardBlock = 0.995;
inline constexpr double kCardReach = 0.45;
// IR-Strahlung einer IR-LED am Fototransistor (ab Volta 2): Lichtstrom = k * I_LED mit k = kIrCoupling / (1 + (d /
// kIrReach)^2), d = Abstand der Bauteilmitten in Feldern (zwei Felder: 4 %); die Abstrahlrichtung bleibt
// unberuecksichtigt (Vereinfachung). Ausgangswert fuer die Kalibrierung.
inline constexpr double kIrCoupling = 0.2;
inline constexpr double kIrReach = 1.0;
// Gewichte (ab Volta 3): liegen auf dem Kraftsensor, wenn ihre Mitte hoechstens kWeightReach Felder von der Mitte des
// Sensors entfernt ist; die Massen addieren sich.
inline constexpr const char* kToolWeight100 = "gewicht100";
inline constexpr const char* kToolWeight200 = "gewicht200";
inline constexpr const char* kToolWeight500 = "gewicht500";
inline constexpr double kWeightReach = 1.0;
// Magnet (ab Volta 3): kleiner Scheibenmagnet, Flussdichte direkt ueber dem Hall-Sensor 90 mT, nach aussen wie ein
// Dipol B = B0 / (1 + (d / d0)^2)^1,5 mit d0 = 0,7 Feld (ein Feld weiter ca. 17 mT). Eingeschaltet zeigt der Suedpol
// zur Grundplatte (positives Feld), ausgeschaltet (umgedreht) der Nordpol.
inline constexpr const char* kToolMagnet = "magnet";
inline constexpr double kMagnetPeakMt = 90.0;
inline constexpr double kMagnetReach = 0.7;

// Mitte eines Bauteils in Feldeinheiten; false, wenn es nicht auf Feldern der Grundplatte steckt
bool partCenter(const PartPlacement& part, double& col, double& row);
// Beleuchtungsstaerke am Bauteil in lx
double illuminanceAt(const Board& board, const PartPlacement& part);
// Anteil von Licht und IR-Strahlung, der am Bauteil ankommt (Abdeckkarte), 0..1
double shadeAt(const Board& board, const PartPlacement& part);
// Kopplungsfaktor IR-LED -> Fototransistor (Lichtstrom je A LED-Strom), ohne Abschattung
double irCoupling(const PartPlacement& led, const PartPlacement& receiver);
// Lufttemperatur am Bauteil in K
double airTemperatureAt(const Board& board, const PartPlacement& part);
// Naesse am Bauteil 0..1 (Wassertropfen; d0 = 0,6: direkt darunter nass, ein Feld weiter ca. 0,26)
double wetnessAt(const Board& board, const PartPlacement& part);
// Last auf dem Kraftsensor in kg (Summe der aufgelegten Gewichte)
double loadAt(const Board& board, const PartPlacement& part);
// Flussdichte am Hall-Sensor in mT (Vorzeichen: Suedpol zur Grundplatte positiv)
double magneticFieldAt(const Board& board, const PartPlacement& part);
// true, wenn ein Werkzeug dieses Typs auf der Grundplatte liegt (ein oder aus)
bool toolPlaced(const Board& board, const std::string& type);

} // namespace volta::model
