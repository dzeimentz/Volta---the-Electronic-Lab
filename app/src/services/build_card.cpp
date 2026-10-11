#include "build_card.hpp"

#include <QFontMetricsF>
#include <QHash>
#include <QPageLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>

#include <algorithm>
#include <cmath>
#include <map>

#include "model/anchor.hpp"

namespace volta::services {

namespace {

const QColor kNavy(0x1B, 0x3A, 0x5C);
const QColor kInk(0x1E, 0x2B, 0x38);
const QColor kGrey(0x4A, 0x58, 0x66);
const QColor kField(0xF1, 0xE8, 0xD4);
const QColor kFieldBorder(0xB5, 0xA4, 0x84);

QColor wireColor(const std::string& c) {
    if (c == "rot") return QColor(0xD3, 0x2F, 0x2F);
    if (c == "schwarz") return QColor(0x22, 0x22, 0x22);
    if (c == "blau") return QColor(0x1E, 0x63, 0xC6);
    if (c == "gelb") return QColor(0xE0, 0xB0, 0x00);
    if (c == "gruen") return QColor(0x2E, 0x7D, 0x32);
    if (c == "weiss") return QColor(0xFA, 0xFA, 0xFA);
    return QColor(0x80, 0x80, 0x80);
}

// Kurzbeschriftung eines Anschlusspunkts: "P:+9V" -> "+9V", "M:3" -> "M3", "MESS:VOHM" -> "V/Ohm"
QString fieldLabel(const std::string& a) {
    QString s = QString::fromStdString(a);
    if (s.startsWith(QLatin1String("P:"))) return s.mid(2);
    if (s.startsWith(QLatin1String("M:"))) return QStringLiteral("M") + s.mid(2);
    if (s.startsWith(QLatin1String("MESS:"))) return s.mid(5).replace(QLatin1String("VOHM"), QStringLiteral("V/Ω"));
    return s;
}

// Polaritaet bzw. Anschlussname in der Zeichnung (Farbe nie allein: Zeichen statt Farbe)
QString pinMark(const std::string& type, const std::string& pin) {
    if (pin == "anode" || pin == "plus") return QStringLiteral("+");
    if (pin == "kathode" || pin == "minus") return QStringLiteral("−");
    if (type == "transistor_npn") return pin == "b" ? QStringLiteral("B") : pin == "c" ? QStringLiteral("C") : QStringLiteral("E");
    if (type == "wechselschalter" && pin == "mitte") return QStringLiteral("M");
    if (type == "relais" && (pin == "com" || pin == "no")) return QString::fromStdString(pin).toUpper();
    return {};
}

// Zeichnung der Grundplatte: Pultleiste, Felder A-E, Modulleiste (nur wenn genutzt)
class BoardDrawing {
public:
    BoardDrawing(const model::Board& board, const model::Catalog& catalog, const QMap<QString, QString>& labels,
                 double width)
        : m_board(board), m_catalog(catalog), m_labels(labels), m_width(width) {
        // Welche Bereiche werden gebraucht?
        auto use = [this](const std::string& a) {
            const std::string r = resolve(a);
            if (r.rfind("MESS:", 0) == 0) m_meter = true;
            if (r.rfind("M:", 0) == 0) m_moduleRow = true;
        };
        for (const auto& p : board.parts)
            for (const auto& pin : p.pins) use(pin.second);
        for (const auto& w : board.wires) {
            use(w.from);
            use(w.to);
        }
        if (!board.modules.empty()) m_moduleRow = true;
        layout();
    }

    double height() const { return m_height; }

    void paint(QPainter& p, QPointF origin) {
        p.save();
        p.translate(origin);
        p.setRenderHint(QPainter::Antialiasing);
        // Modulkoerper unter den Feldern der Leiste
        for (const auto& m : m_board.modules) {
            const model::ModuleEntry* e = m_catalog.module(m.type);
            const int n = e ? static_cast<int>(e->pins.size()) : 1;
            const QRectF a = m_rects.value(QStringLiteral("M:%1").arg(m.slot));
            const QRectF b = m_rects.value(QStringLiteral("M:%1").arg(m.slot + n - 1));
            if (a.isNull() || b.isNull()) continue;
            // Name unter den Feldern: Leitungen kommen von oben und verdecken ihn nicht
            const QRectF body(a.left() - 2, a.top() - 2, b.right() - a.left() + 4, a.height() + 17);
            p.setPen(QPen(kNavy, 1.2));
            p.setBrush(QColor(0xDD, 0xE7, 0xF3));
            p.drawRoundedRect(body, 4, 4);
            p.setPen(kNavy);
            p.setFont(font(7.5, true));
            const QString id = QString::fromStdString(m.id);
            p.drawText(QRectF(body.left(), body.bottom() - 14, body.width(), 13), Qt::AlignCenter,
                       m_labels.value(id, id));
        }
        // Felder
        for (auto it = m_rects.cbegin(); it != m_rects.cend(); ++it) {
            p.setPen(QPen(kFieldBorder, 0.8));
            p.setBrush(kField);
            p.drawRoundedRect(it.value(), 3, 3);
        }
        // Belegung je Feld: Bauteilbeine zuerst, dann Leitungsenden; nebeneinander im Feld verteilt
        std::map<std::string, int> total, next;
        for (const auto& part : m_board.parts)
            for (const auto& pin : part.pins) ++total[resolve(pin.second)];
        for (const auto& w : m_board.wires) {
            ++total[resolve(w.from)];
            ++total[resolve(w.to)];
        }
        auto slot = [&](const std::string& anchor) -> QPointF {
            const std::string r = resolve(anchor);
            const QRectF rect = m_rects.value(QString::fromStdString(r));
            if (rect.isNull()) return {};
            const int k = next[r]++;
            const int n = std::max(1, total[r]);
            return {rect.left() + rect.width() * (k + 1) / (n + 1), rect.top() + rect.height() * 0.68};
        };
        // Lage aller Beine und Leitungsenden (Bauteile zuerst, damit sie im Feld links sitzen)
        std::vector<std::vector<std::pair<QPointF, std::string>>> partPts;
        for (const auto& part : m_board.parts) {
            auto& pts = partPts.emplace_back();
            for (const auto& pin : part.pins) {
                const QPointF pt = slot(pin.second);
                if (!pt.isNull()) pts.emplace_back(pt, pin.first);
            }
        }
        std::vector<std::pair<QPointF, QPointF>> wireEnds;
        for (const auto& w : m_board.wires) {
            const QPointF a = slot(w.from);
            wireEnds.emplace_back(a, slot(w.to));
        }
        // Leitungen unter den Bauteilen: in einer Reihe als Bogen nach oben, zwischen Reihen als S-Kurve; heller
        // Rand, damit auch die schwarze Leitung auf dem Feld gut zu sehen ist
        for (std::size_t i = 0; i < m_board.wires.size(); ++i) {
            const auto& w = m_board.wires[i];
            const auto [a, b] = wireEnds[i];
            if (a.isNull() || b.isNull()) continue;
            QPainterPath path(a);
            const double dy = b.y() - a.y();
            if (std::abs(dy) < 12) {
                const double lift = std::min(30.0, 10.0 + std::abs(b.x() - a.x()) * 0.12);
                path.cubicTo(a + QPointF(0, -lift), b + QPointF(0, -lift), b);
            } else {
                path.cubicTo(a + QPointF(0, dy * 0.45), b - QPointF(0, dy * 0.45), b);
            }
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(w.color == "weiss" ? QColor(0x70, 0x70, 0x70) : QColor(Qt::white), 4.2, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(path);
            p.setPen(QPen(wireColor(w.color), 2.4, Qt::SolidLine, Qt::RoundCap));
            p.drawPath(path);
            p.setPen(Qt::NoPen);
            p.setBrush(wireColor(w.color));
            p.drawEllipse(a, 2.6, 2.6);
            p.drawEllipse(b, 2.6, 2.6);
        }
        // Bauteile: Beine vom Koerper zu den Feldern, Koerper mit Bezeichnung, Polaritaet am Bein
        for (std::size_t i = 0; i < m_board.parts.size(); ++i) {
            const auto& part = m_board.parts[i];
            const auto& pts = partPts[i];
            if (pts.empty()) continue;
            QPointF c;
            for (const auto& [pt, name] : pts) c += pt;
            c /= static_cast<double>(pts.size());
            p.setPen(QPen(QColor(0x55, 0x55, 0x55), 2.2, Qt::SolidLine, Qt::RoundCap));
            for (const auto& [pt, name] : pts) p.drawLine(c, pt);
            p.setBrush(QColor(0x55, 0x55, 0x55));
            p.setPen(Qt::NoPen);
            for (const auto& [pt, name] : pts) p.drawEllipse(pt, 2.4, 2.4);
            p.setFont(font(7.5, true));
            p.setPen(kInk);
            for (const auto& [pt, name] : pts) {
                const QString mark = pinMark(part.type, name);
                if (!mark.isEmpty()) p.drawText(QRectF(pt.x() - 8, pt.y() + 2, 16, 10), Qt::AlignCenter, mark);
            }
            const QString id = QString::fromStdString(part.id);
            const QString label = m_labels.value(id, id);
            p.setFont(font(7, true));
            const double w = QFontMetricsF(p.font()).horizontalAdvance(label) + 8;
            const QRectF body(c.x() - w / 2, c.y() - 6.5, w, 13);
            p.setPen(QPen(kNavy, 1));
            p.setBrush(Qt::white);
            p.drawRoundedRect(body, 3, 3);
            p.setPen(kNavy);
            p.drawText(body, Qt::AlignCenter, label);
        }
        // Feldnamen zuletzt, auf eigenem Grund: bleiben lesbar, auch wenn eine Leitung darueber laeuft
        p.setFont(font(6.5, true));
        const QFontMetricsF fm(p.font());
        for (auto it = m_rects.cbegin(); it != m_rects.cend(); ++it) {
            const QString name = fieldLabel(it.key().toStdString());
            const QRectF r(it.value().left() + 2, it.value().top() + 1.5, fm.horizontalAdvance(name) + 3, fm.height());
            p.setPen(Qt::NoPen);
            p.setBrush(kField);
            p.drawRect(r);
            p.setPen(kGrey);
            p.drawText(r, Qt::AlignCenter, name);
        }
        p.restore();
    }

private:
    static QFont font(double pt, bool bold) {
        QFont f;
        f.setPointSizeF(pt);
        f.setBold(bold);
        return f;
    }

    // Modulanschluss ueber den Namen ("BL1.takt") auf das Feld der Leiste abbilden
    std::string resolve(const std::string& a) const {
        if (model::classifyAnchor(a) != model::AnchorKind::ModulePin) return a;
        const auto dot = a.find('.');
        const std::string id = a.substr(0, dot), pin = a.substr(dot + 1);
        for (const auto& m : m_board.modules) {
            if (m.id != id) continue;
            const model::ModuleEntry* e = m_catalog.module(m.type);
            if (!e) break;
            const auto it = std::find(e->pins.begin(), e->pins.end(), pin);
            if (it != e->pins.end()) return model::moduleSlot(m.slot + static_cast<int>(it - e->pins.begin()));
        }
        return a;
    }

    void layout() {
        const double gap = 3;
        double y = 0;
        // Pultleiste (und Buchsen des Messgeraets, falls genutzt)
        std::vector<std::string> desk = model::deskFields();
        if (m_meter)
            for (const auto& j : model::meterJacks()) desk.push_back(j);
        const double dw = m_width / static_cast<double>(desk.size());
        for (std::size_t i = 0; i < desk.size(); ++i)
            m_rects.insert(QString::fromStdString(desk[i]), QRectF(i * dw + gap / 2, y, dw - gap, 24));
        y += 24 + 22;
        // Grundplatte
        const int cols = std::clamp(m_board.columns, 8, 12);
        const double cw = m_width / cols;
        for (char row = 'A'; row <= 'E'; ++row) {
            for (int c = 1; c <= cols; ++c)
                m_rects.insert(QStringLiteral("%1%2").arg(QChar(row)).arg(c), QRectF((c - 1) * cw + gap / 2, y, cw - gap, 32));
            y += 32 + 18;
        }
        // Modulleiste
        if (m_moduleRow) {
            y += 4;
            const int slotCount = model::BoardLayout::volta1().moduleSlots;
            const double mw = m_width / slotCount;
            for (int s = 1; s <= slotCount; ++s)
                m_rects.insert(QString::fromStdString(model::moduleSlot(s)), QRectF((s - 1) * mw + gap / 2, y, mw - gap, 24));
            y += 24 + 16;
        } else {
            y -= 18;
        }
        m_height = y + 4;
    }

    const model::Board& m_board;
    const model::Catalog& m_catalog;
    const QMap<QString, QString>& m_labels;
    double m_width;
    double m_height = 0;
    bool m_meter = false;
    bool m_moduleRow = false;
    QMap<QString, QRectF> m_rects;
};

// Fliesstext mit Seitenumbruch
class Writer {
public:
    Writer(QPdfWriter& pdf, QPainter& p, const QString& footer) : m_pdf(pdf), m_p(p), m_footer(footer) {
        m_w = pdf.width();
        m_h = pdf.height();
        drawFooter();
    }
    double width() const { return m_w; }
    double y() const { return m_y; }
    void skip(double dy) { m_y += dy; }
    void need(double h) {
        if (m_y + h <= m_h - kFooter) return;
        m_pdf.newPage();
        m_y = 0;
        drawFooter();
    }
    // Text mit Umbruch; indent: Einzug (z. B. fuer Nummern)
    void text(const QString& s, double pt, bool bold, const QColor& color, double indent = 0, bool italic = false) {
        QFont f;
        f.setPointSizeF(pt);
        f.setBold(bold);
        f.setItalic(italic);
        const QRectF r = QFontMetricsF(f).boundingRect(QRectF(0, 0, m_w - indent, 10000), Qt::TextWordWrap, s);
        need(r.height());
        m_p.setFont(f);
        m_p.setPen(color);
        m_p.drawText(QRectF(indent, m_y, m_w - indent, r.height() + 2), Qt::TextWordWrap, s);
        m_y += r.height();
    }
    void heading(const QString& s) {
        need(64); // Ueberschrift nie allein am Seitenende: Platz fuer mindestens eine Zeile darunter
        m_y += 10;
        text(s, 13, true, kNavy);
        m_p.setPen(QPen(kNavy, 0.8));
        m_p.drawLine(QPointF(0, m_y + 2), QPointF(m_w, m_y + 2));
        m_y += 8;
    }
    QPainter& painter() { return m_p; }

private:
    static constexpr double kFooter = 30;
    void drawFooter() {
        QFont f;
        f.setPointSizeF(7.5);
        m_p.setFont(f);
        m_p.setPen(kGrey);
        m_p.drawLine(QPointF(0, m_h - kFooter + 6), QPointF(m_w, m_h - kFooter + 6));
        m_p.drawText(QRectF(0, m_h - kFooter + 9, m_w, kFooter - 9), Qt::TextWordWrap, m_footer);
    }
    QPdfWriter& m_pdf;
    QPainter& m_p;
    QString m_footer;
    double m_w = 0, m_h = 0, m_y = 0;
};

} // namespace

bool writeBuildCardPdf(QIODevice& out, const model::Board& board, const model::Catalog& catalog,
                       const BuildCardTexts& t) {
    QPdfWriter pdf(&out);
    pdf.setResolution(72); // Einheit 1 pt
    pdf.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(40, 36, 40, 30),
                                  QPageLayout::Point));
    pdf.setTitle(t.brand + QStringLiteral(" - ") + t.title);
    pdf.setCreator(t.brand);
    QPainter p;
    if (!p.begin(&pdf)) return false;
    Writer w(pdf, p, t.footer);

    // Kopfband
    p.setPen(Qt::NoPen);
    p.setBrush(kNavy);
    p.drawRoundedRect(QRectF(0, 0, w.width(), 26), 4, 4);
    QFont bf;
    bf.setPointSizeF(11);
    bf.setBold(true);
    p.setFont(bf);
    p.setPen(Qt::white);
    p.drawText(QRectF(10, 0, w.width() - 20, 26), Qt::AlignVCenter | Qt::AlignLeft, t.brand);
    p.drawText(QRectF(10, 0, w.width() - 20, 26), Qt::AlignVCenter | Qt::AlignRight, t.subtitle);
    w.skip(36);
    w.text(t.title, 18, true, kInk);
    if (!t.question.isEmpty()) {
        w.skip(4);
        w.text(t.question, 11, false, kGrey, 0, true);
    }
    if (!t.warning.isEmpty()) {
        w.skip(8);
        QFont f;
        f.setPointSizeF(11);
        f.setBold(true);
        const double h = QFontMetricsF(f).height() + 10;
        w.need(h);
        p.setPen(QPen(QColor(0xE6, 0x51, 0x00), 1.2));
        p.setBrush(QColor(0xFF, 0xE9, 0xDC));
        p.drawRoundedRect(QRectF(0, w.y(), w.width(), h), 4, 4);
        p.setFont(f);
        p.setPen(QColor(0x8A, 0x32, 0x00));
        p.drawText(QRectF(8, w.y(), w.width() - 16, h), Qt::AlignVCenter, QStringLiteral("⚠ ") + t.warning);
        w.skip(h);
    }

    // Zeichnung
    w.heading(t.boardHeading);
    BoardDrawing drawing(board, catalog, t.labels, w.width());
    w.need(drawing.height());
    drawing.paint(p, QPointF(0, w.y()));
    w.skip(drawing.height());

    // Teileliste
    w.heading(t.partsHeading);
    for (const QString& line : t.parts) {
        w.text(QStringLiteral("☐  ") + line, 10.5, false, kInk); // Kaestchen zum Abhaken
        w.skip(3);
    }

    // Schritte
    if (!t.steps.isEmpty()) {
        w.heading(t.stepsHeading);
        for (int i = 0; i < t.steps.size(); ++i) {
            QFont f;
            f.setPointSizeF(10.5);
            const QRectF r = QFontMetricsF(f).boundingRect(QRectF(0, 0, w.width() - 22, 10000), Qt::TextWordWrap, t.steps[i]);
            w.need(r.height());
            p.setFont(f);
            p.setPen(kNavy);
            QFont nf = f;
            nf.setBold(true);
            p.setFont(nf);
            p.drawText(QRectF(0, w.y(), 20, r.height()), Qt::AlignRight | Qt::AlignTop, QString::number(i + 1) + QLatin1Char('.'));
            w.text(t.steps[i], 10.5, false, kInk, 26);
            w.skip(4);
        }
    }
    return p.end();
}

} // namespace volta::services
