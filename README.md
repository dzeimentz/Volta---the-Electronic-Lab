# Volta – Das Elektronik-Labor

Virtuelle Elektronik-Experimentierkaesten fuer Kinder und Jugendliche: Volta 1 (8–12 Jahre), Volta 2, Volta 3,
spaeter Zuse 1 – Computertechnik fuer Einsteiger. Android zuerst, dann Windows und Linux (x86-64) und iPadOS.

DC Software Engineering · Daniel Zeimentz · https://dc-software-engineering.com · info@dc-software-engineering.com

## Schnellstart

```bash
# Simulationskern bauen und testen (ohne Qt)
cmake --preset core-only && cmake --build --preset core-only && ctest --preset core-only

# Inhaltspakete pruefen
python3 tools/validate_content.py

# Desktop-Anwendung (Qt 6.8+, Umgebungsvariable QT_ROOT_DESKTOP auf die Qt-Installation)
cmake --preset desktop-debug && cmake --build --preset desktop-debug
```

## Dokumentation
- Einstieg fuer Claude Code und Entwickler: `CLAUDE.md`
- Konzept: `docs/Konzept_Volta_v3.0.pdf`
- Architektur, Fachmodell, Formate, Plattformen: `docs/`
- Architekturentscheidungen: `docs/adr/`

© 2026 DC Software Engineering. Alle Rechte vorbehalten.
