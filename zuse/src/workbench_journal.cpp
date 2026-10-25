// Forscherheft der Werkbank (wie Volta): je Experiment Vermutung, Beobachtung, Vergleich, eigene Notizen
// und Fotos. Zetti schreibt Vermutung und Ergebnis in Alltagssprache hinein (in der Sprache, in der das Kind gerade
// arbeitet); die Vorfuehrung schreibt nichts. Gespeichert nur lokal (forscherheft.json im Datenordner, Fotos daneben).
#include <QDateTime>
#include <QJsonArray>
#include <QLocale>
#include <QUrl>
#include <algorithm>

#include "workbench_controller.hpp"

using volta::logic::Level;

namespace zuse {

namespace {

QString onOff(const QStringList& states, const QString& label, bool on) {
    if (states.size() == 2) return states[on ? 1 : 0];
    return QStringLiteral("%1 %2").arg(label, on ? QStringLiteral("1") : QStringLiteral("0"));
}

} // namespace

QString WorkbenchController::journalFile(const QString& dataDir) {
    return dataDir.isEmpty() ? QString() : dataDir + QStringLiteral("/forscherheft.json");
}

// Schalterstellung einer Zeile und was die Lampen tun, in Worten: »Fenster 1 offen, Fenster 2 zu → Warnlampe leuchtet«
QString WorkbenchController::situationLine(const Stage& s, size_t row, const std::vector<Level>& outs) const {
    QStringList setting;
    for (qsizetype i = 0; i < s.inputs.size(); ++i) {
        const bool on = (row >> (s.inputs.size() - 1 - i)) & 1u;
        setting << onOff(s.inputStates.value(i), s.inputLabels.value(i, s.inputs[i]), on);
    }
    QStringList result;
    for (size_t o = 0; o < outs.size() && o < size_t(s.outputs.size()); ++o)
        result << onOff(s.outputStates.value(qsizetype(o)), s.outputLabels.value(qsizetype(o), s.outputs[qsizetype(o)]),
                        outs[o] == Level::One);
    return setting.join(QStringLiteral(", ")) + QStringLiteral(" → ") + result.join(QStringLiteral(", "));
}

void WorkbenchController::journalGuess() {
    if (m_demo || !isDiscover()) return;
    const Stage& s = m_stages[size_t(m_stage)];
    QString text;
    if (m_guessSkipped) {
        text = tr("Keine Vermutung – ich habe gleich ausprobiert.");
    } else {
        QStringList lines;
        for (size_t r = 0; r < m_guess.size(); ++r) lines << situationLine(s, r, {m_guess[r]});
        text = lines.join(QLatin1Char('\n'));
    }
    if (m_journal.setGuess(s.kuerzel, text)) emit journalChanged();
}

void WorkbenchController::journalResult() {
    if (m_demo || m_stages.empty()) return;
    const Stage& s = m_stages[size_t(m_stage)];
    if (s.free) return;
    const QString name = s.name.isEmpty() ? s.title : s.name;
    QString text, comparison;
    if (isDiscover()) {
        text = tr("Die passende Kiste heißt %1. %2").arg(name, s.merksatz);
        if (m_guessSkipped) {
            comparison = QStringLiteral("offen");
        } else {
            QStringList differ;
            for (size_t r = 0; r < s.table.size() && r < m_guess.size(); ++r)
                if (s.table[r].empty() || m_guess[r] != s.table[r][0]) differ << situationLine(s, r, s.table[r]);
            comparison = differ.isEmpty() ? QStringLiteral("passt") : QStringLiteral("anders");
            if (!differ.isEmpty()) text += QLatin1Char('\n') + tr("So war es wirklich:") + QLatin1Char('\n') + differ.join(QLatin1Char('\n'));
        }
    } else if (isCounting()) {
        text = tr("»%1« geschafft. %2").arg(s.title, s.merksatz);
    } else {
        text = (s.makesChip ? tr("»%1« erfunden, geprüft und eingepackt.").arg(name) : tr("»%1« gebaut und geprüft.").arg(name))
               + QLatin1Char(' ') + s.merksatz;
        if (m_solutionShown) text += QLatin1Char(' ') + tr("Dabei habe ich mir die Musterlösung angeschaut.");
    }
    if (m_journal.setObservation(s.kuerzel, text, comparison)) emit journalChanged();
}

QVariantList WorkbenchController::journal() const {
    QVariantList out;
    for (const QJsonObject& e : m_journal.entries()) {
        const QString id = e.value(QStringLiteral("id")).toString();
        QString title = id, kind;
        int index = -1;
        for (size_t i = 0; i < m_stages.size(); ++i)
            if (m_stages[i].kuerzel == id) {
                title = stageLabel(int(i)) + QLatin1Char(' ') + m_stages[i].title;
                kind = m_stages[i].kind;
                index = int(i);
            }
        const QDateTime when = QDateTime::fromString(e.value(QStringLiteral("datum")).toString(), Qt::ISODate);
        QStringList notes;
        for (const QJsonValue& n : e.value(QStringLiteral("notizen")).toArray()) notes << n.toString();
        QVariantList photos; // {file, url}
        for (const QJsonValue& p : e.value(QStringLiteral("fotos")).toArray())
            photos << QVariantMap{{"file", p.toString()},
                                  {"url", QUrl::fromLocalFile(m_journal.photoDir() + QLatin1Char('/') + p.toString())}};
        const QString cmp = e.value(QStringLiteral("vergleich")).toString();
        out << QVariantMap{{"id", id},
                           {"index", index},
                           {"kind", kind},
                           {"title", title},
                           {"date", QLocale().toString(when, QLocale::ShortFormat)},
                           {"stamp", e.value(QStringLiteral("datum")).toString()},
                           {"guess", e.value(QStringLiteral("vermutung")).toString()},
                           {"observation", e.value(QStringLiteral("beobachtung")).toString()},
                           {"comparison", cmp == QLatin1String("passt") ? 1 : cmp == QLatin1String("anders") ? 2
                                          : cmp == QLatin1String("offen") ? 3 : 0},
                           {"notes", notes},
                           {"photos", photos}};
    }
    // neueste zuerst; bei gleicher Zeit (mehrere Schritte in einer Sekunde) das spaetere Experiment im Lernpfad
    std::stable_sort(out.begin(), out.end(), [](const QVariant& a, const QVariant& b) {
        const QVariantMap x = a.toMap(), y = b.toMap();
        const QString sx = x["stamp"].toString(), sy = y["stamp"].toString();
        return sx != sy ? sx > sy : x["index"].toInt() > y["index"].toInt();
    });
    return out;
}

bool WorkbenchController::addJournalNote(const QString& id, const QString& text) {
    if (m_demo || !m_journal.addNote(id, text)) return false;
    emit journalChanged();
    return true;
}

bool WorkbenchController::addJournalPhoto(const QString& id, const QUrl& source) {
    if (m_demo) return false;
    const QString path = source.isLocalFile() ? source.toLocalFile() : source.toString();
    if (m_journal.addPhoto(id, path).isEmpty()) return false;
    emit journalChanged();
    return true;
}

bool WorkbenchController::removeJournalPhoto(const QString& id, const QString& file) {
    if (!m_journal.removePhoto(id, file)) return false;
    emit journalChanged();
    return true;
}

void WorkbenchController::resetJournal() {
    m_journal.reset();
    emit journalChanged();
}

} // namespace zuse
