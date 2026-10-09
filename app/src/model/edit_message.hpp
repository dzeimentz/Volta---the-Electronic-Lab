#pragma once

#include <QtGlobal>

#include <string>
#include <utility>
#include <vector>

namespace volta::model {

// Meldung an das Kind (z. B. warum ein Bauteil nicht passt). source ist der deutsche Quelltext mit Platzhaltern
// %1, %2 ..., markiert mit QT_TRANSLATE_NOOP("Meldung", ...), damit lupdate ihn findet; die Oberflaeche uebersetzt
// ihn mit QCoreApplication::translate("Meldung", source) und setzt args ein (Kennungen, Feldnamen, Zahlen - keine
// Woerter). text() liefert den deutschen Text (Tests, Protokoll).
struct Message {
    const char* source = nullptr;
    std::vector<std::string> args;

    bool empty() const { return source == nullptr; }
    std::string text() const {
        if (!source) return {};
        std::string out = source;
        for (std::size_t i = args.size(); i-- > 0;) { // %10 vor %1 ersetzen
            const std::string key = "%" + std::to_string(i + 1);
            for (std::size_t pos = out.find(key); pos != std::string::npos; pos = out.find(key, pos + args[i].size()))
                out.replace(pos, key.size(), args[i]);
        }
        return out;
    }
};

inline Message msg(const char* source, std::vector<std::string> args = {}) { return Message{source, std::move(args)}; }

} // namespace volta::model
