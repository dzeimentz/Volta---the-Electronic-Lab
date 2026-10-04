#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "volta/core/component_model.hpp"
#include "volta/core/types.hpp"

namespace volta::core {

// Netzliste aus Knoten und Bauteilinstanzen. Wird vom Schaltungsmodell der App erzeugt und bei jedem
// Topologiewechsel als Ganzes an den Simulator uebergeben (verschiebbar, nicht kopierbar).
class Circuit {
public:
    Circuit() = default;
    Circuit(Circuit&&) noexcept = default;
    Circuit& operator=(Circuit&&) noexcept = default;

    NodeId addNode() { return m_nodeCount++; }
    std::size_t nodeCount() const noexcept { return m_nodeCount; }

    // Legt ein Bauteil an und gibt eine Referenz darauf zurueck.
    template <class T, class... Args>
    T& add(Args&&... args) {
        auto p = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *p;
        m_components.push_back(std::move(p));
        return ref;
    }

    std::size_t componentCount() const noexcept { return m_components.size(); }
    ComponentModel& component(std::size_t i) noexcept { return *m_components[i]; }
    const ComponentModel& component(std::size_t i) const noexcept { return *m_components[i]; }

    // Index des Bauteils mit der Kennung id oder kNoComponent.
    std::size_t indexOf(const std::string& id) const noexcept;

    // true, wenn alle Anschluesse auf vorhandene Knoten zeigen.
    bool isValid() const noexcept;

private:
    std::size_t m_nodeCount = 1; // Masse existiert immer
    std::vector<std::unique_ptr<ComponentModel>> m_components;
};

} // namespace volta::core
