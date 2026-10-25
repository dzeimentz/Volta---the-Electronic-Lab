// Aufgabe der Lehrkraft (wie Volta): Die Lehrkraft stellt im Elternbereich Experimente und/oder den Aufbau aus dem
// freien Labor zusammen; Zuse zeigt einen QR-Code und, wenn es passt, einen Code zum Eintippen. Das Kind nimmt die
// Aufgabe an; sie bleibt je Profil gespeichert (aufgabe.txt), bis eine neue kommt oder sie entfernt wird.
#include <QDir>
#include <QFile>
#include <QSaveFile>

#include "model/qr_code.hpp"
#include "workbench_controller.hpp"
#include "zuse_task.hpp"

using volta::logic::ChipDef;
using volta::logic::primitiveFromName;

namespace zuse {

QVariantMap WorkbenchController::qrCode(const QString& text) const {
    volta::model::QrMatrix m;
    if (!volta::model::makeQrCode(text.toStdString(), m)) return {};
    QString cells;
    cells.reserve(static_cast<qsizetype>(m.dark.size()));
    for (bool d : m.dark) cells += d ? QLatin1Char('1') : QLatin1Char('0');
    return {{QStringLiteral("size"), m.size}, {QStringLiteral("cells"), cells}};
}

QString WorkbenchController::freeKuerzel() const {
    for (const Stage& s : m_stages)
        if (s.free) return s.kuerzel;
    return {};
}

int WorkbenchController::freeLabParts() const {
    const auto free = m_drafts.find(freeKuerzel());
    return free == m_drafts.end() ? 0 : int(free->second.parts.size());
}

bool WorkbenchController::boardUsable(const ChipDef& board) const {
    for (const auto& p : board.parts)
        if (!primitiveFromName(p.type) && !m_demoLibrary.find(p.type)) return false;
    return true;
}

namespace {
task::Task makeTask(const QStringList& labels) {
    task::Task t;
    for (const QString& l : labels)
        if (t.experiments.size() < task::kMaxExperiments) t.experiments << l;
    return t;
}
} // namespace

QString WorkbenchController::makeTaskLink(const QStringList& labels, const QString& text, bool withBoard,
                                          const QString& title) const {
    task::Task t = makeTask(labels);
    t.text = text.trimmed().left(task::kMaxTextLength);
    if (withBoard)
        if (const auto free = m_drafts.find(freeKuerzel()); free != m_drafts.end() && !free->second.parts.empty()) {
            t.board = free->second;
            t.title = title.trimmed();
        }
    if (t.experiments.isEmpty() && !t.board) return {};
    return task::toLink(t);
}

QString WorkbenchController::makeTaskCode(const QStringList& labels) const { return task::toCode(makeTask(labels)); }

QVariantMap WorkbenchController::readTask(const QString& linkOrCode) const {
    task::Task t;
    if (!task::parse(linkOrCode, t)) return {{QStringLiteral("ok"), false}};
    const bool ours = t.band == QLatin1String("zuse1");
    QVariantList exps;
    for (const QString& label : std::as_const(t.experiments))
        for (size_t i = 0; ours && i < m_stages.size(); ++i)
            if (!m_stages[i].free && stageLabel(int(i)) == label)
                exps << QVariantMap{{"index", int(i)},
                                    {"label", label},
                                    {"title", m_stages[i].title},
                                    {"done", m_done.contains(m_stages[i].kuerzel)}};
    const bool hasBoard = ours && t.board && boardUsable(*t.board);
    return {{QStringLiteral("ok"), ours && (!exps.isEmpty() || hasBoard)},
            {QStringLiteral("otherBand"), !ours},
            {QStringLiteral("experiments"), exps},
            {QStringLiteral("title"), t.title},
            {QStringLiteral("text"), t.text},
            {QStringLiteral("hasBoard"), hasBoard}};
}

QVariantMap WorkbenchController::teacherTask() const {
    if (m_taskLink.isEmpty()) return {};
    return readTask(m_taskLink);
}

void WorkbenchController::setTaskLink(const QString& link) {
    m_taskLink = link;
    m_taskStages.clear();
    for (const QVariant& e : teacherTask().value(QStringLiteral("experiments")).toList())
        m_taskStages.insert(e.toMap().value(QStringLiteral("index")).toInt());
    lendChips();
    emit teacherTaskChanged();
    emit stagesChanged();
    emit viewChanged();
}

bool WorkbenchController::acceptTask(const QString& linkOrCode) {
    task::Task t;
    if (!task::parse(linkOrCode, t) || !readTask(linkOrCode).value(QStringLiteral("ok")).toBool()) return false;
    const QString link = task::toLink(t); // Code wird als Link abgelegt
    if (!m_dataDir.isEmpty()) {
        QDir().mkpath(m_dataDir); // erster Start: Ordner gibt es noch nicht
        QSaveFile f(m_dataDir + QStringLiteral("/aufgabe.txt"));
        if (f.open(QIODevice::WriteOnly)) {
            f.write(link.toUtf8());
            f.commit();
        }
    }
    setTaskLink(link);
    return true;
}

void WorkbenchController::clearTask() {
    if (!m_dataDir.isEmpty()) QFile::remove(m_dataDir + QStringLiteral("/aufgabe.txt"));
    setTaskLink({});
}

void WorkbenchController::loadTask() {
    QString link;
    if (!m_dataDir.isEmpty()) {
        QFile f(m_dataDir + QStringLiteral("/aufgabe.txt"));
        if (f.open(QIODevice::ReadOnly) && f.size() < 256 * 1024) link = QString::fromUtf8(f.readAll()).trimmed();
    }
    setTaskLink(link);
}

// Aufbau der Aufgabe ins freie Labor (ersetzt den bisherigen Aufbau dort); fehlende Erfindungen werden geliehen
bool WorkbenchController::startTaskBoard() {
    task::Task t;
    if (m_taskLink.isEmpty() || !task::parse(m_taskLink, t) || !t.board || !boardUsable(*t.board)) return false;
    const QString free = freeKuerzel();
    if (free.isEmpty()) return false;
    ChipDef board = *t.board;
    board.kuerzel = free.toStdString();
    endDemo();
    m_drafts[free] = board;
    lendChips();
    save();
    startFreeLab();
    return true;
}

} // namespace zuse
