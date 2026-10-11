#include "task_code.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>

#include "model/board_json.hpp"
#include "profile_transfer.hpp"

namespace volta::services::task {

namespace {

constexpr const char* kHost = "aufgabe";
constexpr int kMaxPayload = 64 * 1024; // entpackt; schuetzt vor uebergrossen Daten aus fremden Codes
const char kAlphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ"; // Crockford-Base32
const QStringList kBands{QStringLiteral("volta1"), QStringLiteral("volta2"), QStringLiteral("volta3"),
                         QStringLiteral("zuse1")};

bool splitShortId(const QString& id, int& stage, int& number) {
    const QStringList parts = id.split(QLatin1Char('.'));
    if (parts.size() != 2) return false;
    bool ok1 = false, ok2 = false;
    stage = parts[0].toInt(&ok1);
    number = parts[1].toInt(&ok2);
    return ok1 && ok2 && stage >= 0 && stage <= 7 && number >= 1 && number <= 31;
}

// Pruefziffer ueber 36 Datenbits (9 Halbbytes): Rotieren und Verknuepfen
quint64 checksum(quint64 data) {
    unsigned h = 0;
    for (int i = 8; i >= 0; --i) {
        h = ((h << 1) | (h >> 3)) & 15u;
        h ^= static_cast<unsigned>((data >> (4 * i)) & 15u);
    }
    return (h ^ 10u) & 15u;
}

int decodeChar(QChar c) {
    c = c.toUpper();
    if (c == QLatin1Char('O')) c = QLatin1Char('0');
    if (c == QLatin1Char('I') || c == QLatin1Char('L')) c = QLatin1Char('1');
    for (int i = 0; i < 32; ++i)
        if (c == QLatin1Char(kAlphabet[i])) return i;
    return -1;
}

} // namespace

QString toLink(const Task& task) {
    // passt die Aufgabe in einen Code, traegt der Link nur diesen: kleiner, grober QR-Code (gut vom Beamer lesbar)
    if (const QString code = toCode(task); !code.isEmpty())
        return QStringLiteral("%1://%2?c=%3").arg(QLatin1String(transfer::kScheme), QLatin1String(kHost), code);
    QJsonObject o{{QStringLiteral("v"), 1}, {QStringLiteral("band"), task.band}};
    if (!task.experiments.isEmpty()) o.insert(QStringLiteral("e"), QJsonArray::fromStringList(task.experiments));
    if (!task.title.isEmpty()) o.insert(QStringLiteral("t"), task.title.left(80));
    if (!task.text.isEmpty()) o.insert(QStringLiteral("x"), task.text.left(kMaxTextLength));
    if (task.board) o.insert(QStringLiteral("b"), model::boardToJson(*task.board));
    const QByteArray packed = qCompress(QJsonDocument(o).toJson(QJsonDocument::Compact), 9);
    const QByteArray data = packed.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return QStringLiteral("%1://%2?d=%3").arg(QLatin1String(transfer::kScheme), QLatin1String(kHost),
                                              QString::fromLatin1(data));
}

bool fromLink(const QString& link, Task& out) {
    const QUrl url(link.trimmed());
    if (url.scheme() != QLatin1String(transfer::kScheme) || url.host() != QLatin1String(kHost)) return false;
    if (const QString code = QUrlQuery(url).queryItemValue(QStringLiteral("c")); !code.isEmpty()) return fromCode(code, out);
    const QByteArray data = QUrlQuery(url).queryItemValue(QStringLiteral("d"), QUrl::FullyDecoded).toLatin1();
    const QByteArray packed = QByteArray::fromBase64(data, QByteArray::Base64UrlEncoding);
    if (packed.size() < 4) return false;
    const quint32 size = (quint32(quint8(packed[0])) << 24) | (quint32(quint8(packed[1])) << 16) |
                         (quint32(quint8(packed[2])) << 8) | quint32(quint8(packed[3]));
    if (size == 0 || size > kMaxPayload) return false;
    const QJsonObject o = QJsonDocument::fromJson(qUncompress(packed)).object();
    if (o.value(QStringLiteral("v")).toInt() != 1) return false;
    Task t;
    t.band = o.value(QStringLiteral("band")).toString(QStringLiteral("volta1"));
    for (const auto& v : o.value(QStringLiteral("e")).toArray()) {
        int stage = 0, number = 0;
        if (splitShortId(v.toString(), stage, number)) t.experiments << v.toString();
    }
    t.title = o.value(QStringLiteral("t")).toString().left(80);
    t.text = o.value(QStringLiteral("x")).toString().left(kMaxTextLength);
    if (o.contains(QStringLiteral("b"))) {
        model::Board b;
        if (!model::boardFromJson(o.value(QStringLiteral("b")).toObject(), b)) return false;
        t.board = b;
    }
    if (t.experiments.isEmpty() && !t.board) return false;
    out = t;
    return true;
}

QString toCode(const Task& task) {
    const int band = kBands.indexOf(task.band);
    if (band < 0 || task.board || !task.text.isEmpty() || task.experiments.isEmpty() ||
        task.experiments.size() > kMaxCodeExperiments)
        return {};
    quint64 data = (quint64(band) << 34) | (quint64(task.experiments.size() - 1) << 32);
    for (int i = 0; i < task.experiments.size(); ++i) {
        int stage = 0, number = 0;
        if (!splitShortId(task.experiments[i], stage, number)) return {};
        data |= quint64((stage << 5) | number) << (24 - 8 * i);
    }
    const quint64 value = (data << 4) | checksum(data);
    QString code;
    for (int i = 7; i >= 0; --i) {
        code += QLatin1Char(kAlphabet[(value >> (5 * i)) & 31u]);
        if (i == 4) code += QLatin1Char('-');
    }
    return code;
}

bool fromCode(const QString& code, Task& out) {
    QString clean;
    for (QChar c : code)
        if (!c.isSpace() && c != QLatin1Char('-')) clean += c;
    if (clean.size() != 8) return false;
    quint64 value = 0;
    for (QChar c : clean) {
        const int d = decodeChar(c);
        if (d < 0) return false;
        value = (value << 5) | quint64(d);
    }
    const quint64 data = value >> 4;
    if ((value & 15u) != checksum(data)) return false;
    const int band = int((data >> 34) & 3u);
    const int count = int((data >> 32) & 3u) + 1;
    Task t;
    t.band = kBands[band];
    for (int i = 0; i < count; ++i) {
        const int byte = int((data >> (24 - 8 * i)) & 0xFFu);
        const int stage = byte >> 5, number = byte & 31;
        if (number < 1) return false;
        t.experiments << QStringLiteral("%1.%2").arg(stage).arg(number);
    }
    // ungenutzte Plaetze muessen leer sein (sonst Tippfehler, den die Pruefziffer nicht bemerkt hat)
    for (int i = count; i < kMaxCodeExperiments; ++i)
        if ((data >> (24 - 8 * i)) & 0xFFu) return false;
    out = t;
    return true;
}

bool parse(const QString& linkOrCode, Task& out) {
    const QString s = linkOrCode.trimmed();
    return s.contains(QLatin1String("://")) ? fromLink(s, out) : fromCode(s, out);
}

} // namespace volta::services::task
