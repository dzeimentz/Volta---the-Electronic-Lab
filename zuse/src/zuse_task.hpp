#pragma once

// Aufgabe der Lehrkraft wie bei Volta (nur Hinweg, ohne Netz): Experimente aus dem Lernpfad (kurze Kennungen wie
// "4.7") und/oder der Aufbau aus dem freien Labor mit Titel und Aufgabentext. Keine personenbezogenen Daten.
// Code zum Eintippen im gleichen Format wie Volta (Band "zuse1"), damit sich Codes der Reihe nicht verwechseln.
#include <QString>
#include <QStringList>

#include <optional>

#include "volta/logic/chip.hpp"

namespace zuse::task {

struct Task {
    QString band = QStringLiteral("zuse1");
    QStringList experiments; // kurze Kennungen, z. B. "4.7"
    QString title;           // Titel des Aufbaus
    QString text;            // Aufgabentext (frei, kurz)
    std::optional<volta::logic::ChipDef> board; // Aufbau aus dem freien Labor
};

inline constexpr int kMaxCodeExperiments = 4; // so viele passen in den Code zum Eintippen
inline constexpr int kMaxExperiments = 8;
inline constexpr int kMaxTextLength = 160;

// Link fuer den QR-Code: "zuse1://aufgabe?c=<Code>", wenn die Aufgabe in einen Code passt, sonst
// "zuse1://aufgabe?d=<base64url(zlib(JSON))>"
QString toLink(const Task& task);
bool fromLink(const QString& link, Task& out);

// Code zum Eintippen (nur Lernpfad, hoechstens 4 Experimente, Stufe 0-7, Nummer 1-31): 8 Zeichen Crockford-Base32
// mit Pruefziffer, angezeigt als "ABCD-EFGH". Verwechselbare Zeichen (O/0, I/L/1) werden beim Lesen gleichgesetzt.
// Leer, wenn die Aufgabe nicht hineinpasst (Aufbau, Text, zu viele Experimente).
QString toCode(const Task& task);
bool fromCode(const QString& code, Task& out);

// Link oder Code
bool parse(const QString& linkOrCode, Task& out);

} // namespace zuse::task
