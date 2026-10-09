# ADR-0009: Tonausgabe und Vorlesen mit Qt Multimedia und Qt TextToSpeech

- Status: angenommen
- Datum: 2026-10-09

## Kontext
Die Klanganzeige (Leuchtring) soll durch echten Ton ergaenzt werden (Konzept 5.4), und Texte sollen vorgelesen
werden koennen (Konzept 5.6, Barrierefreiheit). ADR-0001 sieht dafuer Qt Multimedia und Qt TextToSpeech vor.
Zu klaeren war, wie das mit ADR-0006 (kein Netz) vereinbar ist: `Qt6Multimedia` laedt `Qt6Network` als
Abhaengigkeit, und das FFmpeg-Backend bringt `avformat` mit, das Netzprotokolle beherrscht.

## Entscheidung
- **Ton**: Volta erzeugt die Abtastwerte selbst (`model::ToneSynth`, ohne Qt, getestet) und gibt sie ueber
  `QAudioSink` im Ziehbetrieb aus (`services::AudioService`). Es werden keine Medien-Dateien oder URLs abgespielt.
  Unter Windows wird das Windows-eigene Backend erzwungen (`QT_MEDIA_BACKEND=windows`); das FFmpeg-Plugin und die
  FFmpeg-Bibliotheken werden nicht ausgeliefert (`tools/deploy_release.py`).
- **Vorlesen**: `QTextToSpeech` mit der lokalen Systemstimme (Windows: SAPI; Android/iPadOS: Systemdienst).
  Luxemburgisch: keine Systemstimme, dort spaeter eigene Tonaufnahmen im Inhaltspaket (docs/i18n.md).
- `Qt6Network` wird nur als Bibliotheksabhaengigkeit geladen; Volta ruft keine Netzwerkfunktionen auf
  (`QNetworkAccessManager` bleibt verboten).

## Folgen
- Gehoerschutz: Summenaussteuerung hoechstens ca. 30 %, weiche Begrenzung, gleitende Ein- und Ausschaltvorgaenge.
- Einstellung »Ton über Lautsprecher« (AppSettings `anzeige/ton`); Ton nur, solange das Labor sichtbar ist. Fuer
  den Klassenmodus (Phase 3) kann die Ausgabe zentral abgeschaltet werden; die Klanganzeige bleibt aussagekraeftig.
- Tests laufen ohne Tonausgabe (der Dienst entsteht nur in `main.cpp`); Bildschirmfoto-Entwicklerhilfen sind still.
