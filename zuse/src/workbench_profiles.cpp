// Forscherprofile, Freigaben und Zuruecksetzen (Elternbereich, wie Volta). Auf geteilten Geraeten hat jedes Kind ein
// Profil mit Symbol und Spitznamen (ohne Klarnamen, ohne Passwort); Werkbank, Erfindungen und Forscherheft liegen je
// Profil in einem eigenen Ordner (ProfileStore von Volta). Ohne Profile liegt alles direkt im Datenordner.
#include <algorithm>

#include "workbench_controller.hpp"

using volta::logic::ChipDef;
using volta::logic::Library;

namespace zuse {

// Daten eines anderen Ordners laden (Profilwechsel): Werkbank, Erfindungen, Entwuerfe und Forscherheft
void WorkbenchController::useDataDir(const QString& dir) {
    endDemo();
    m_dataDir = dir;
    m_library = Library{};
    m_lent.clear();
    m_done.clear();
    m_drafts.clear();
    m_symbolStyle = QStringLiteral("din");
    m_numbersVisible = true;
    m_boardSize = 0;
    m_journal = volta::services::JournalStore(journalFile(dir));
    if (!dir.isEmpty()) load();
    dropStaleChips();
    loadTask(); // leiht auch die Erfindungen
    startFirstOpen();
    emit journalChanged();
    emit symbolStyleChanged();
    emit numbersVisibleChanged();
}

// erste noch offene Stufe des Lernpfads (sonst die letzte)
void WorkbenchController::startFirstOpen() {
    int first = -1;
    for (size_t i = 0; i < m_stages.size() && first < 0; ++i)
        if (!m_stages[i].free && !m_done.contains(m_stages[i].kuerzel)) first = int(i);
    if (first < 0)
        for (size_t i = 0; i < m_stages.size(); ++i)
            if (!m_stages[i].free) first = int(i);
    m_lastStage = std::max(first, 0);
    startStage(m_lastStage);
}

// ---- Forscherprofile

void WorkbenchController::useDataRoot(const QString& dir) {
    m_dataRoot = dir;
    m_profiles = volta::services::ProfileStore(dir);
    emit profilesChanged();
}

void WorkbenchController::setProfilesEnabled(bool on) {
    if (on == m_profilesEnabled) return;
    m_profilesEnabled = on;
    m_currentProfile.clear();
    if (!on) useDataDir(m_dataRoot); // wieder die Daten ohne Profil
    emit profilesChanged();
}

QVariantList WorkbenchController::profiles() const {
    QVariantList out;
    for (const auto& p : m_profiles.profiles()) out << QVariantMap{{"id", p.id}, {"symbol", p.symbol}, {"name", p.name}};
    return out;
}

QVariantMap WorkbenchController::currentProfile() const {
    const volta::services::Profile* p = m_profiles.find(m_currentProfile);
    if (!p) return {};
    return {{"id", p->id}, {"symbol", p->symbol}, {"name", p->name}};
}

QString WorkbenchController::addProfile(const QString& symbol, const QString& name) {
    const QString id = m_profiles.add(symbol, name);
    emit profilesChanged();
    return id;
}

bool WorkbenchController::selectProfile(const QString& id) {
    if (!m_profilesEnabled || !m_profiles.find(id)) return false;
    useDataDir(m_profiles.dataDir(id));
    m_currentProfile = id;
    emit profilesChanged();
    return true;
}

void WorkbenchController::leaveProfile() {
    if (m_currentProfile.isEmpty()) return;
    endDemo();
    m_currentProfile.clear();
    emit profilesChanged();
}

bool WorkbenchController::removeProfile(const QString& id) {
    if (id == m_currentProfile) {
        m_currentProfile.clear();
        useDataDir({}); // Dateien des Profils nicht mehr benutzen
    }
    const bool ok = m_profiles.remove(id);
    emit profilesChanged();
    return ok;
}

void WorkbenchController::removeAllProfiles() {
    m_currentProfile.clear();
    if (m_profilesEnabled) useDataDir({});
    m_profiles.removeAll();
    emit profilesChanged();
}

// ---- Freigaben: alle Experimente offen. Fehlen dafuer Erfindungen aus frueheren Experimenten, leiht Zuse die
// Musterloesung aus (nur im Werkzeugfach, nicht in der Werkzeugkiste, wird nicht gespeichert). Baut das Kind die
// Erfindung selbst, ersetzt seine die geliehene.

void WorkbenchController::setUnlockAll(bool on) {
    if (on == m_unlockAll) return;
    m_unlockAll = on;
    lendChips();
    emit stagesChanged();
    emit stageChanged();
    emit viewChanged();
}

// Geliehen wird: bei der Freigabe alles; mit einer Aufgabe der Lehrkraft die Erfindungen der Stufen vor ihrem letzten
// Experiment; dazu, was ein Aufbau im freien Labor (aus einer Aufgabe) braucht – jeweils samt der Erfindungen darin.
void WorkbenchController::lendChips() {
    Library& own = m_demo ? m_ownLibrary : m_library;
    for (const QString& k : std::as_const(m_lent)) own.remove(k.toStdString());
    m_lent.clear();
    QStringList wanted;
    int last = -1;
    for (int i : std::as_const(m_taskStages)) last = std::max(last, i);
    for (size_t i = 0; i < m_stages.size(); ++i)
        if (m_stages[i].makesChip && (m_unlockAll || int(i) < last)) wanted << m_stages[i].kuerzel;
    if (const auto free = m_drafts.find(freeKuerzel()); free != m_drafts.end())
        for (const auto& p : free->second.parts) wanted << QString::fromStdString(p.type);
    while (!wanted.isEmpty()) {
        const QString k = wanted.takeFirst();
        if (own.find(k.toStdString())) continue;
        const ChipDef* chip = m_demoLibrary.find(k.toStdString());
        if (!chip) continue; // Grundgatter
        own.add(*chip);
        m_lent.insert(k);
        for (const auto& p : chip->parts) wanted << QString::fromStdString(p.type);
    }
    if (!m_demo) rebuild(); // Aufbauten mit geliehenen Erfindungen neu berechnen
}

// ---- Zuruecksetzen (Elternbereich, mit Rueckfrage)

void WorkbenchController::resetProgress() {
    endDemo();
    m_library = Library{};
    m_lent.clear();
    m_done.clear();
    m_drafts.clear();
    save();
    lendChips();
    m_lastStage = 0;
    startStage(0);
}

} // namespace zuse
