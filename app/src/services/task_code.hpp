#pragma once

#include <QString>
#include <QStringList>

#include <optional>

#include "model/board.hpp"

namespace volta::services::task {

// Aufgabe der Lehrkraft (docs/ui-spec.md, Aufgaben per QR-Code): nur von der Lehrkraft zu den Kindern, ohne Netz.
// Inhalt: Experimente aus dem Lernpfad (kurze Kennungen wie "2.4") und/oder ein eigener Aufbau mit Titel und
// Aufgabentext. Keine personenbezogenen Daten.
struct Task {
    QString band = QStringLiteral("volta1");
    QStringList experiments; // kurze Kennungen innerhalb des Bandes, z. B. "2.4"
    QString title;           // Titel des eigenen Aufbaus
    QString text;            // Aufgabentext (frei, kurz)
    std::optional<model::Board> board;
};

inline constexpr int kMaxCodeExperiments = 4; // so viele passen in den Code zum Eintippen
inline constexpr int kMaxTextLength = 160;

// Link fuer den QR-Code: "volta1://aufgabe?c=<Code>", wenn die Aufgabe in einen Code passt, sonst
// "volta1://aufgabe?d=<base64url(zlib(JSON))>"
QString toLink(const Task& task);
bool fromLink(const QString& link, Task& out);

// Code zum Eintippen (nur Lernpfad, hoechstens 4 Experimente, Stufe 0-7, Nummer 1-31): 8 Zeichen Crockford-Base32
// mit Pruefziffer, angezeigt als "ABCD-EFGH". Verwechselbare Zeichen (O/0, I/L/1) werden beim Lesen gleichgesetzt.
// Leer, wenn die Aufgabe nicht in einen Code passt (eigener Aufbau, Text, zu viele Experimente).
QString toCode(const Task& task);
bool fromCode(const QString& code, Task& out);

// Link oder Code
bool parse(const QString& linkOrCode, Task& out);

} // namespace volta::services::task
