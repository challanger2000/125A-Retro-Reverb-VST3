# P004.4 – Systematischer adversarialer Audit (2026-11-14)

**Urteil: PARTIAL / NOT VERIFIED – RELEASE NO-GO. Kein FULL AUDIT, keine QA-PASS-/FINAL-Freigabe.**

Gegenstand ist ausschließlich **125A Retro Reverb**, nicht das gesonderte Produkt 125A Ugly Reverb. Dieser Bericht folgt `125A-Engineering/START-HERE.md`, `QA/AUDIT-COMPLETENESS-CONTRACT.md`, `QA/RELEASE-QA.md` und den einschlägigen Engineering-Standards. Es wurde **kein Produkt-DSP, keine GUI, kein kundenfähiges Artefakt** geändert. Der Audit ist eine abgeschlossene Untersuchung des hier zugänglichen Materials; die vertraglich vorgeschriebenen echten Host-/Funktionstests sind **nicht** als bestanden fingiert.

## 1. Provenienz, Baseline, Umgebung

| Feld | Geprüfte Grundlage |
|---|---|
| Repository | `challanger2000/125A-Retro-Reverb-VST3` |
| Audit-Baseline | `develop/p001-original-freeverb` @ `890225e980ab868687ab4b2857c33ec1e2dc79c2`, P004.4 / CMake 0.3.4 |
| Stable `main` | `0ed7aecf7ad649ed9a7f680784078ea273357c58`; nicht verändert |
| Entwicklung / PR | Draft PR #1; Quelle und CI-Artefakt stammen vom identischen P004.4-Commit |
| Windows CI | https://github.com/challanger2000/125A-Retro-Reverb-VST3/actions/runs/43587011552 – `completed/success` |
| VST3-Artefakt | `125A-Retro-Reverb-P0044-NoZoom-Windows-x64-VST3`, ID `14110684005`, Windows x64 |
| Tatsächliche VST3-DLL | `125A_Retro_Reverb.vst3/Contents/x86_64-win/125A_Retro_Reverb.vst3`, 4,113,408 Bytes |
| DLL SHA-256 | `6e639667f59f79444332cb3f29c03f050df106414c492813381ef8f11b849eb5` |
| P74/P98-Belege | Actions Artifact `14109824830`, `125A-Retro-Reverb-P74-Independent-Test-Lab-evidence` |
| Externe GUI | `125A-Ugly-Reverb-Final@631c79b5fddfb19b916d927f0918134576a3a031`, konkrete `UglyControls.h/.cpp` und Filmstrip-Generator gelesen |
| Test Lab | `125A-Plugin-Test-Lab@7213fe1f81f4036f74431cb5b2d7dbd09b73ee5c`; technische Ergebnisse und verwendete Runner-/Capture-/Recall-Testpfade untersucht |
| SDK | Extern bezogenes Steinberg VST3 SDK `v3.8.1_build_84`; vollständiger SDK-Quellcode **nicht auditiert** |
| Ausführungsumgebung des Zusatz-Audits | Quelltextreview per GitHub-Connector, heruntergeladene unveränderte Windows-Artefakt-ZIPs, PE-/ZIP-/WAV-/JSON-Prüfungen, eigenständig kompilierte **isolierte Quellgleichungs-Gegenproben** mit g++/C++23; keine Windows-VST3-Laufzeit im Prüfumfeld |

**FULL/Demo-Herkunft (Pflichtabgleich):**

| FULL-/kommerzieller Baseline-Status | Demo-Status |
|---|---|
| Aktuelle Retro-Reverb-Quelle ist ein **Entwicklungskandidat 0.3.4**, kein nachgewiesen freigegebenes und ausgeliefertes kommerzielles FULL-Produkt. Kundenseitiger freigegebener FULL-ZIP-/SHA-/Gumroad-Beleg fehlt: **NOT VERIFIED**. | Kein separater Demo-Zweig, Demo-Build oder produktspezifische Demo-Lizenz bei dieser P004.4-Quelle belegt: **N/A für aktuelle Funktionsprüfung; ein realer Produkt-Release-Abgleich bleibt offen**. |
| Das im Engineering-Register genannte **125A Ugly Reverb V2** ist ein *anderes Produkt* und bloß Quelle ausgewählter GUI-Klassen, nicht freigegebene Retro-Reverb-Baseline. | Keine Demo-Quelle darf stillschweigend zur FULL-Quelle erklärt werden. |

## 2. Bidirektionale Inventur und Leseabdeckung

Git-Tree dieses SHA: **50 Einträge = 37 Dateien + 13 Verzeichnisse**. Alle **36 textuellen Repository-Dateien** (Produkt, Build, Tests, Werkzeuge, README, Referenzen und Lizenzerläuterung) wurden eingelesen; die **eine binäre** Grafik `resource/retro_lever_off_on_125.png` wurde anhand SHA, PNG-Struktur und mitgeliefertem Windows-GUI-Artefakt untersucht. Original-GUI-Fremdquelle und die für den P74-Nachweis relevanten Test-Lab-Implementierungen wurden gesondert gelesen. Die vollständigen 318 Test-Lab-Tree-Einträge und das gesamte externe VST3-SDK sind **keine vollständig überprüften Abhängigkeiten**.

| Bereich | Geprüfte Dateien / Funktionen / Datenwege |
|---|---|
| VST3 Factory/Metadaten | `plugin/factory.cpp` → Klassenkennung, Version, `RetroProcessor::createInstance`; tatsächliche PE-Strings und paketiertes `moduleinfo.json` |
| Host-/Audio-Vertrag | `retro_processor.hpp/.cpp`: `initialize`, Bus-Konfiguration, `setupProcessing`, `canProcessSampleSize`, `setActive`, `setProcessing`, `process`, `getTailSamples` |
| Parameter und Persistenz | IDs 100–104; `getState`, `setState`; Parameter-Queues, Grenzen, 24-Byte-State, GUI `getEditorState`/`setEditorState` |
| Aktiver DSP | `src/freeverb_mix.hpp`; Freeverb `revmodel`, `comb`, `allpass`, Tuning, Denormal-Handling; tatsächlich gelinkte `freeverb_core` |
| GUI | `retro_gui.cpp`, `retro_reverb.uidesc`, externes `UglyKnob`, `UglyLogo`, VSTGUI `COnOffButton`; Ressourcen; Editor-Erzeugung/Close/Zoom-100%-Pfad |
| Build/Supply Chain | `build_windows.bat`, `CMakeLists.txt`, GitHub Actions, ZIP-Paketierung, Steinberg SDK/GUI/Test-Lab-Bezug |
| Tests | `reverb_tests`, `freeverb_mix_tests`, `reference_reverb_tests`, `juce_616_differential_tests`, `check_gui_native_assets.py`, `check_toggle_sprite.py` |
| Hilfsprogramme | `vocaid_ab_renderer.cpp`, `compare_vocaid.py`; `README.md`, `START-HERE.md`, `THIRD_PARTY.md`, `docs/*`, `references/*` |
| NUR Forschung / nicht aktive VST3-Engine | `src/reference_reverb.hpp`, `third_party/juce_616/juce_Reverb.h`, JUCE-Testshim; **kein** Teil des ausgelieferten Reverb-DSP |
| Keine sichtbare Funktion | MIDI-Ausgabe, ROM, SF2, Impulsantwort-Import, Sequencer, externe Lizenzprüfung: **N/A**, keine Implementierung oder Produktzusage für P004.4 erkennbar |

### Vollständiges interaktives UI-Inventar

| UI-Funktion | Sichtbare Interaktionen | ID | Kontrollkette |
|---|---|---:|---|
| Enable | transparenter ON/OFF-Hebel | 100 | `COnOffButton` → VSTGUI Editor → `kEnable` → `enabled_` → `process` Bypass → State |
| Size | 125px `UglyKnob` + Textfeld | 101 | Editor/Tag → `kSize` → `size_` → `revmodel::setroomsize` → State |
| Damping | 125px `UglyKnob` + Textfeld | 102 | Editor/Tag → `kDamping` → `damping_` → `revmodel::setdamp` → State |
| Width | 125px `UglyKnob` + Textfeld | 103 | Editor/Tag → `kWidth` → `width_` → `revmodel::setwidth` → State |
| Mix | 125px `UglyKnob` + Textfeld | 104 | Editor/Tag → `kMix` → `mix_` → `applyMix`/0%-Mute → State |

**5 Parameter/Control-Gruppen, 9 interaktive UI-Bedienflächen** (5 Custom-Views + 4 Textfelder). Nicht interaktiv: Faceplate, Logo, Labels. Zoom-Steuerung in P004.4 entfernt; nur 100% ist zugelassen. UIDESC und vollständige Controller-IDs stimmen statisch überein. **Klicken, Ctrl-Linksklick-Reset, Tippen, Automation-Redraw, echte DAW-Save/Reopen für jedes Element: NICHT AUSGEFÜHRT.**

## 3. Tatsächliche CI- und Artefakt-Nachweise

- Windows Build: **PASS** für den exakt referenzierten Commit. CTest **4/4 PASS**, Steinberg VST3 Validator **47/47 PASS**; alle CI-Jobschritte erfolgreich.
- Native GUI-Assets: statische Pixel-/Layout-/Sprite-Prüfungen **PASS**; aus P74-Artefakt tatsächlich erfasste 930×410-GUI mit vier Knobs und Hebel visuell überprüft. Dies **beweist keine** Interaktion, Default-Reset oder Projektkompatibilität im Zielhost.
- Original Freeverb-Test: aktive Freeverb-Algorithmen und Parameterlaw werden als separate Bibliothek getestet. `juce_616_differential_tests.cpp` vergleicht hingegen **JUCE-Testquelle mit `VocaidReferenceReverb`**, nicht mit der ausgelieferten Windows-VST3-/Freeverb-Engine.
- P74/P98 meldet `status=TECHNICAL_BASELINE_PASS` und `native_technical=PASS`, gleichzeitig **`required_feature_coverage=NOT DECLARED`, `approved_product_cases=NOT DECLARED`, `exploratory_clicks_performed=0`, `release_approved=false`**.
- Bei unabhängigem Lesen des tatsächlich mitgelieferten P34-WAVs: **352.800 Stereo-Frames, 705.600 Samples, 0 von 705.600 ungleich 0**, Peak 0. Die drei aufgenommenen/frisch gerenderten WAVs sind digital still, **weil die generische Host-Fixture keinen positiven Audiosignaltest für diesen Effekt erzeugt**. Daraus **nicht** ableiten, dass das Produkt selbst stumm ist; ableiten: die Testspur kann Wet/Dry-Korrektheit nicht belegen.
- P74 Fresh-Recall berichtet `fresh_process_recall=PASS`, jedoch **`audio_determinism=NOT COVERED`, `musical_correctness=NOT COVERED`, `saved_state_integrity=NOT COVERED`**. 24-Byte-Default-State tatsächlich dekodiert: Magic `0x125A0001`, Werte 1, 0.5, 0.5, 1, 0.33333334.
- Überprüfte ZIP- und DLL-Hashes sowie Inhalt: verifizierbar; kein unbemerkter Quell- oder Produktbuild im Rahmen dieses Audits.

## 4. Nachweis-Matrix: Soll, Negativkontrolle, Ist

**Status**: PASS nur für konkret beobachteten Check; FAIL für reproduzierbare Gegenbeispiele bzw. konkret fehlenden Pflichterfolg; NOT RUN für nicht ausführbare Tests; INCONCLUSIVE für nicht ausreichend untersuchte Behauptungen.

| ID / Vertrag / Quellpfad | Unabhängiger Sollzustand und Angriff | Tatsächliche Evidenz | Status |
|---|---|---|---|
| C01 Factory/Version | DLL-/Modul-/Klassen-Version gleich 0.3.4; negative absichtlich alte Klassen-Version | Ausgelieferter `moduleinfo.json`: `Version=0.3.4`, aber `Classes[0].Version=0.3.2`; PE enthält 0.3.2 und keine 0.3.4; synthetischer korrekter Positivfall funktioniert | **FAIL** |
| C02 Enable 100 | OFF = exakt trocken; Wiedereinschalten soll keine zeitlich eingefrorenen Alt-Tails hervorholen; Wechsel während Block | Source `bypass` überspringt DSP und friert Delay-Tank ein; isolierte Quellgleichungs-Gegenprobe: 3.000 Samples OFF, nach ON alter Hall 1.016 Samples später erneut | **FAIL (source-level)** |
| C03 Size 101 | kompletter Regler 0/default/25/50/75/100; tatsächliche Wet-Änderung und State/Reset | Tag und `setroomsize` statisch bestätigt; generische Core-Tests, aber kein VST3-Parameter-Audio-Sweep oder realer Reset | **NOT RUN (end-to-end)** |
| C04 Damping 102 | Frequenzabhängige Dämpfung im tatsächlichen Wet-PCM, korrektes Normalisieren/Recall | Tag und `setdamp` statisch bestätigt, keine unabhängige gemessene Host-PCM-Orakelreihe | **NOT RUN (end-to-end)** |
| C05 Width 103 | Links/Rechts-Output, Endpunkte, Stereo-Mono-Mapping, Recall | `setwidth` statisch, Core-Tests zeigen Stereo; kein VST3-Gesamt-Sweep bei 1→1/1→2/2→2 | **NOT RUN (end-to-end)** |
| C06 Mix 104, 0% | 0% bitgenau dry/unity, alter Hall verworfen; bewusst falsche Wet-Beimischung erkennen | Original-Freeverb-Kern + Source-Gegenprobe 0%-Dry positiv; `freeverb_mix_tests.cpp` simuliert VST3-BYPASS aber von Hand statt auf echtem Plugin zu prüfen | **PASS Core / NOT RUN VST3** |
| C07 Mix 100% / Tail | 100% wet ohne dry; Impuls-/Silence-Postroll & Sample-Rate-Tail überprüfen | Core-Wet-Onset (L 1116 / R 1139) und Modell-Gegenprobe PASS; im echten Test-Lab-WAV nur Null-PCM; 15-Sekunden-Hosttail nicht verifiziert | **NOT RUN (VST3)** |
| C08 Parameter-Automation | Später Sample-Offset 224 von 256 darf vorherige 224 Samples nicht umschalten; echte Zwischenpunkte bearbeiten | `process` verwertet nur letzten Queue-Punkt und verwendet dessen Offset nicht; Source-Transkript testet späten Switch: **224/256 zeitlich falsch zugewiesene Samples** | **FAIL (source-level)** |
| C09 NaN/Inf-Eingaben/-Parameter | Nonfinite Samples und ungültige Host-Werte begrenzen/isolieren, keine bleibend vergiftete Audio-Engine | Source weist keinen NaN-Schutz auf; Quellgleichungs-Gegenprobe: ein NaN-Impuls → **7.952 nonfinite Frames von 10.000**; `std::clamp(NaN,0,1)` bleibt NaN | **FAIL (source-level)** |
| C10 Audio-Host-Buffer | Gültige Mono/Stereo-Topologie, Null-Buffer nicht dereferenzieren; gültige und ungültige Hostdaten | `process` prüft tatsächlich `in.channelBuffers32`, `out.channelBuffers32` und individuelle Input-Channel-Pointer vor Dereferenz; zuvor vermuteter Null-Parent-Deref durch Source-Review **ausgeschlossen**. Native Fehlermatrix fehlt. | **PASS statische Schutzprüfung / NOT RUN Host** |
| C11 LifeCycle | setup, active, processing, stop/start, 0-Frame und 44.1/48/96k valide | P74 generische technische Baseline PASS; kein gezielter Product-Audio-Restart-/Tail-oracle, kein 125A Plugin Tester full run | **PASS technische Baseline / NOT RUN kritisch** |
| C12 State Save/Restore | gültig, kurz, ungültig, NaN, Legacy; gleiche Parameter UND hörbar gleiche PCM nach frischer Instanz | Default 24-Byte-State und generisches Fresh-Recall PASS; kein unabhängiger Audio-/State-Hash-Orakel, keine Invalid-State-Matrix für dieses Produkt | **NOT RUN kritisch** |
| C13 Realtime State Race | Concurrent `process` vs `getState`/GUI ohne Data Race/torn State | Nackte float-Felder ohne sichtbare Synchronisierung; `getState` und Audio-Queue-Zugriff nicht thread-sicher belegt; TSAN/native Concurrent-State-Oracle fehlt | **INCONCLUSIVE / HIGH RISK** |
| C14 GUI Controls 5/9 | Jeder Drag/Klick/ON+OFF/Textedit/Ctrl-Reset → Parameter → PCM → Anzeige → Reopen | UIDESC/Quell/PE-Inventar geprüft; absichtlicher falscher Mix-Tag `999` wurde von getrenntem Inventar-Test erkannt; P74 real `clicks=0` | **PASS statisches Inventar / NOT RUN Funktion** |
| C15 GUI 100% und Legacy Zoom | Vier 125px-Knobs, Hebel, kein Zoom; altes Editor-State re-open | Echtes Windows-GUI-Bild/Asset vorhanden und inspiziert; Zoom-Hardlock und 2-Int-Legacystate nur Quellprüfung, keine DAW-Altprojektöffnung | **PASS visuelle 100% / NOT RUN Legacy** |
| C16 Package/DLL | richtige Klassen-ID und Dateien, korrekte Identität über Version/State | Bundle x64/PE/Resources/Hashes PASS, aber **Version FAIL** C01 | **FAIL gesamter Metadatenvertrag** |
| C17 A/B Prüfprogramm | Externe, unabhängig gemessene User-Vocaid-WAVs statt self-reference | `compare_vocaid.py::selftest` erzeugt Wet-Referenz **mit demselben Renderer**, mit dem er die Kandidaten rendern lässt; nur selbstkonsistenter PCM24-Roundtrip; Nutzer-WAVs/externes Orakel nicht ausgeführt | **NOT RUN (Fidelity)** |
| C18 Aktiver Reverb-DSP | Original-Freeverb-Unverändertheit (dokumentierte Konstruktor-/Mute-Korrekturen), keine JUCE-Engine in Binary | `CMakeLists.txt` linkt `freeverb_core`, Quelle und Ziel der 4 Core-CTests gelesen; JUCE-Vergleich gilt NICHT für aktive VST3 | **PASS Source-/Core-Umfang, NOT RUN unabhängige VST3-PCM-Parität** |
| C19 CPU/Realtime | Keine Callback-Heaps/Locks; p95/p99/max und Deadline unter Last inkl. Automation | Statische Pfade im `process` ohne offensichtliche Heaps; keine belastbare Windows-ASIO-repräsentative Produktmessung | **NOT RUN** |
| C20 125A Plugin Tester | vollständige Windows Release-/Audio-/State-/GUI-Gates | Nur 125A **Plugin Test Lab P74/P98** vorhanden, ausdrücklich `release_approved=false` – nicht mit vollständigem Plugin Tester gleichzusetzen | **NOT RUN** |
| C21 Fender Studio Pro ASIO | Alle Bedienelemente, Automation, Bypass, alte Projekte, echte Audio-Hallfahne in Ziel-DAW | Keine tatsächliche DAW-/ASIO-Session im Audit | **NOT RUN** |
| C22 Musik und Fremd-A/B | reales Material, Wet/Dry, Pegelgleichheit, Klangnutzen und Original-Vocaid-Vergleich | Keine privaten Aufnahmefixturen oder reale Hörabnahme im Ausführungsraum | **NOT RUN** |

## 5. Konkrete Befunde, Ursache und Regression

**F01 – HOCH – Block-zu-frühe Automation (Quellfehler, isoliert reproduziert).** In `retro_processor.cpp::process` wird aus jeder `IParamValueQueue` ausschließlich `getPoint(getPointCount()-1,...)` gelesen. Der `sampleOffset` bleibt wirkungslos. Ein Mix/Enable-Umschalten zum Sample 224/256 wirkt schon ab 0; Zwischenpunkte verschwinden. **Regressionsforderung:** echte `ProcessData` mit zwei unabhängig erwarteten PCM-Abschnitten, Offset 0/1/224/255, mehreren Parametern und blockgrößeninvariantem Referenzaudio. Der negative spät im Block platzierte Punkt muss den bisherigen Code zum FAIL bringen. Der externe SDK-Validator prüft diesen userseitigen Timingvertrag nicht durch die genannten 47 Passes.

**F02 – HOCH – NaN-/Inf-Vergiftung (Quellgleichungs-Reproduktion).** Weder Parameterautomation noch Host-Audio wird vor `revmodel` auf nichtfinite Werte geprüft. Ein NaN gelangt in Comb-Feedback und Allpass; 7.952 nonfinite Frames in 10.000 source-equation-Testframes. Auch `std::clamp` entfernt NaN nicht. **Regressionsforderung:** im realen VST3 NaN/Inf in Input + Queue, nachfolgende endliche PCM, State-Inhalt, Recovery, mono/stereo, alle Blockgrößen; Positivfall zuerst. Kein implizites Rechtemodell verändern.

**F03 – MITTEL – Eingefrorener Hall bei Enable OFF (Quellgleichungs-Reproduktion).** Der `bypass`-Branch führt keine DSP-Berechnung aus, daher bleibt die Tank-Uhr während ausgeschaltetem Enable stehen; beim erneuten Aktivieren werden zeitlich alte Hall-Impulse fortgesetzt. Separate `Mix=0%`-Semantik verwirft absichtlich die Hallfahne und ist **nicht** der gleiche Fehler. **Regressionsforderung:** realer Impuls, 100 Samples ON, 3.000 OFF, Rückschaltung ON, unabhängig erwarteter Bypass-Continuity-Vertrag und kein späte Alt-Hallfahne.

**F04 – MITTEL – Tatsächlich auslieferbarer Versionswiderspruch.** `plugin/factory.cpp` meldet `0.3.2`; CMake und Bundle-Modul melden `0.3.4`. Geprüft an **echtem ausgeliefertem PE und `moduleinfo.json`**. **Regressionsforderung:** Release-Validator muss Factory-Klassen-Version, `moduleinfo.json`, Produktversion und Paketkandidat vor Upload automatisch abgleichen; Test gegen absichtlich alten Versionsstring.

**F05 – HOCHES RISIKO, noch nicht als realer Crash nachgewiesen – Unabgesicherte Zustandskonkurrenz.** `process` verändert `enabled_, size_, damping_, width_, mix_`; `getState` liest dieselben nackten `float`; `setState` und GUI-State-Interaktion besitzen keinen erkennbaren Threadvertrag. Parallele Host-Callbacks könnten Data Races/inkonsistente Snapshots erzeugen. **Regressionsforderung:** native Build-Instrumentierung, parallele Save/Automation/Reopen-Tests, explizites Host-Threadmodell und State-PCM-Gleichheit. **Bis dahin INCONCLUSIVE**, nicht als konkret reproduzierter Runtime-Fehler bezeichnen.

**F06 – HOCH, QA-Deckung – Scheinbar grünes Reverb-Host-QA ohne positives Audiosignal.** `125A Plugin Test Lab` P34/Recall speichert WAV/State und prüft Identität/Container, aber tatsächlich untersuchte Aufnahmen sind **alle völlig stumm**. Die generische P74-Pipeline legt selbst die begrenzte `TECHNICAL_BASELINE_PASS`-Bedeutung offen. **Regressionsforderung:** Product Fixture mit messbarem nicht-stummen trockenem Input, genau bekanntem Freeverb-Impuls-Response/PCM an 0/50/100%, mindestens 2 Raten und 3 Channel-Layouts, tatsächliche Parameter-State-Recall-Ausgabe und tote Effekt-Engine als negative Testmutante. Keine Null-WAVs als Funktions-PASS werten.

**F07 – MITTEL, Testorakel – Reverb-Selbstvergleich und vorgetäuschter Wrapper-Test.** `compare_vocaid.py::selftest` generiert seine `wet`-WAV mit dem **gleichen** `vocaid_ab_renderer`, der erneut als Kandidat läuft. Eine konsistente aber fehlerhafte Engine kann damit grün bleiben. Und `tests/freeverb_mix_tests.cpp::checkZeroWetAndNoOldTail` schreibt die gewünschten BYPASS-Ausgaben direkt in den **Test**, statt das reale VST3-`process` aufzurufen. **Regressionsforderung:** unabhängiger externer PCM-Golden-Oracle und tatsächliche Windows-Processor-Grenze; eine bewusst stumme oder falsch wet-dry-gemischte Test-Engine muss zuverlässig auffallen. **Nicht** JUCE-Referenz-PASS dem aktiven Freeverb zuschreiben.

**F08 – NIEDRIG/MITTEL – Dokumentationsdrift.** README führt ältere GUI 650×298, `UglyZoomControl` und vorherige Schalterimplementierung als aktuelle P004-Beschreibung, während P004.4 930×410, kein Zoom und Sprite-`COnOffButton` verwendet. Ein Audit des Quellcodes kann diese Aussagen widerlegen. **Regressionsforderung:** produktive UI-Details gegen aktuelle UIDESC/Factory/State synchronisieren und historische P004-Unterabschnitte explizit historisch kennzeichnen.

**F09 – MITTEL/RISIKO – Supply-Chain Identität.** GUI `Ugly` und P74 sind commit-gepinnt, der VST3-SDK-Bezug nutzt `--branch v3.8.1_build_84` und verifiziert bei bereits vorhandenem `external/vst3sdk` die Revision nicht explizit. Das belegt **keine kompromittierte DLL**, schränkt aber reproduzierbare Source-Provenienz ein. **Regressionsforderung:** geprüfter SDK-Commit-/Submodule-Manifest-Check im Build.

**Bewusst entkräfteter Fehlalarm:** In `RetroProcessor::process` wird `in.channelBuffers32` entgegen erster Vermutung vor dem Kanalzugriff explizit auf null geprüft; ein Befund „unbedingter Null-Parent-Dereferenz“ wäre **falsch** und wird nicht als Fehler gemeldet.

## 6. Testqualität, Gegenproben, Unabhängigkeit

**Ausgeführt:**
1. **POSITIV:** Native 930×410-Grafik als echte aus Windows-CI extrahierte Bilddatei inspiziert; Produkttags gegen tatsächlich gepackte UIDESC und Factory/Parameter-Code abgeglichen.
2. **POSITIV + NEGATIV:** Versions-/Metadaten-Orakel akzeptiert einen synthetischen korrekten 0.3.4/0.3.4-Fall und **verwirft** die ausgelieferte echte Kombination 0.3.4/0.3.2. Zusätzlich PE-Strings kontrolliert.
3. **POSITIV + Test-only MUTATION:** Statischer GUI-Tag-Check akzeptiert echte 100–104-Belegung; absichtlich falscher `Mix`-Tag 999 wird **KILLED**. Kein GUI-Asset oder Produktfile geändert.
4. **POSITIV + NEGATIV:** Audiosignal-Gate akzeptiert künstlichen bekannten hörbaren Test-WAV, verwirft tatsächliche generische CI-Null-PCM. Dies beweist Test-Lücken, keine Produkt-Stummheit.
5. **POSITIV + adversariale Quellgleichungs-Gegenproben:** Unabhängig in C++23 kompilierte Freeverb-Comb/Allpass-/Mix-Gleichungen liefern 0%-Dry korrekt und Wet-Onset bei Sample 1116; sie reproduzieren dann spezifisch Enable-Tank-Freeze, Nonfinite-Poison und verspätete Blockautomation. **Nicht** als Test des ausgelieferten Windows VST3 ausgeben.

**Grenzen:** Die Quellgleichungs-Gegenproben sind ein **separater Self-Challenge**, kein externer, unabhängiger zweiter Reviewer und kein Host-Test. Es gibt **keinen** im gegenwärtigen Umfeld tatsächlich ausgeführten C++-Mutationstest **gegen die geladene Windows VST3**; entsprechende Beweiskette bleibt NOT RUN. Ebenso fehlen angeforderte proprietäre User-WAVs und Testhardware im Audit-Runtime.

**Stärkste verbleibende, noch ungetestete Hypothesen:** Data Race bei parallelem Projekt-Save, echte Klick-/Textedit-/Ctrl-Reset-Verkettung aller neun UI-Facetten, aktive Mono/Stereo-Busumkonfiguration unter laufendem Audio, Automation an Host-Offsets, produktive Nonfinite-Recovery, erhaltene/nicht erhaltene Tail-States beim Bypass, alte GUI-Zoom-Projektzustände, Fenster-Open/Close/Move in Fender Studio Pro, ASIO-P99 und 125A Plugin Tester Vollabnahme.

## 7. Abschluss der Untersuchung vs. Audit-/Release-Freigabe

| Prüfachse | Evidenzstatus |
|---|---|
| 37 Projektdateien / eigener Quell- und Testbestand | **SOURCE REVIEW COMPLETE für Repository-Dateien**; externe SDK-Vollquelle nicht inspiziert |
| Ausgelieferte Windows-DLL, Ressourcen, Metadaten | **GEPRÜFT – Versionsvertrag FAIL** |
| Windows CI / CTests / Steinberg Validator | **PASS (4/4 und 47/47)** – nur ausgeführte Einzeltests |
| Native P74/P98 technische Baseline | **PASS** in ihrem ausdrücklich beschränkten Sinn |
| Independent adversarial challenges / test adequacy | **PARTIAL**: echte Asset-Gegenproben und source-equation Repros; keine native VST3-Mutationsserie |
| Funktionale VST3-Audio-/GUI-/Automation-Controller-Matrix | **NOT VERIFIED**, kritische Pfade ohne echten positiven Host-PCM-Test |
| Vollständiger 125A Plugin Tester | **NOT RUN** |
| Reale Fender Studio Pro / ASIO / Interaktion / alte Projekte | **NOT RUN** |
| Unabhängiger zweiter Reviewer | **NOT RUN** – nur Self-Challenge |
| Echte Nutzeraudio-A/B und musikalische Hörabnahme | **NOT RUN** |
| FULL AUDIT / QA PASS / RELEASE READY | **NEIN / NO-GO** |

**Erforderliche weitere, konkrete Nachweise** (keine pauschale weitere PASS-Behauptung): zuerst produktive Host-PCM-/Event-/Invalid-Input-Testorakel samt Mutationsnegativen und der oben genannten Defekt-Regressionsfälle; dann neues exakt gebautes Windows-VST3-Artifact mit Versions- und 125A Plugin Tester Volltest; danach Fender-Studio-Pro-ASIO-Sessions für 5 Parameter/9 UI-Bedienflächen, Automation/State-Recall, reale Audio- und Hörabnahme sowie unabhängige Reviewer-Gegenprüfung. Erst danach darf gemäß `QA/AUDIT-COMPLETENESS-CONTRACT.md` der Status neu beurteilt werden.

**Wichtig:** Die ursprünglichen CI-PASS-Protokolle werden nicht „zurückgenommen“; lediglich ihr tatsächlicher Aussagebereich wird korrekt eingegrenzt.


---

## Nachtrag: Echter lokaler 125A Plugin Tester 0.3.2 vom 14.11.2026

Nach Erstellung der ursprünglichen Matrix stellte der Nutzer sechs **tatsächliche lokale Windows-Prüfprotokolle** bereit: QA-Report (JSON und Text), Steinberg Validator, Editor-Lifecycle, I/O/Event und Multi-Instance. Diese aktualisieren die bisherige Aussage **„vollständiger 125A Plugin Tester NOT RUN“** zu **„Tester RUN: WARNING / RELEASE REVIEW WARNINGS“**; die historischen Aussagen oben sind als damaliger Evidenzstand zu lesen. Das installierte Paket liegt unter \`C:\Program Files\Common Files\VST3\Eigene\125A_Retro_Reverb.vst3\`; der Bericht datiert von 2026-11-14 11:26:40 UTC, Tester-Version 0.3.2, Tester-Source SHA \`f053c1d8f858df06231111861550f0c2c69ef179\`. **Tester-Version 0.3.2 ist nicht die Produktversion.** Die installierte Binärdatei wurde hier nicht separat gehasht; Zuordnung zum CI-Artefakt \`890225e\` beruht auf Namen/Klasse, nicht auf bewiesener Binärgleichheit.

**Gesamtstatus: 41 PASS / 4 WARNING / 0 FAIL, exitCode 0; RELEASE = REVIEW WARNINGS, nicht freigegeben.** Die vier gemeldeten Warnungen:

1. **ID 100 Enable – falscher Parameter-Text-Roundtrip:** Angezeigtes \`On\`, normalisiert \`1.000000\` ergibt bei \`getParamValueByString\` unerwartet **\`0.000000\`**. Dies ist ein konkret nachgewiesener Host-Text-Konvertierungsfehler und für das Ausschalten durch Werteingabe relevant.
2. **Controller-Komponentenzustand:** \`setComponentState\` akzeptierte Component-State nicht. Component \`getState/setState\` und Controller-eigenes \`getState/setState\` bestanden separat; das widerlegt die Warnung nicht.
3. **Post-Stress-State-Restore:** Eine oder mehrere Restore-Aufrufe wurden zurückgewiesen. Ohne Individualtrace ist die genaue Auslöseeingabe noch nicht bekannt.
4. **Fresh-Instance-State-Verifikation:** Persistenz/Controller/Lifecycle mit einer oder mehreren Warnungen. Kein belegter Voll-PASS für Projekt-Recall.

**Zusätzliche präzise Befunde:** Im *tatsächlichen* Steinberg-Validator-Protokoll steht bei 32-bit **„Not all points have been read via IParameterChanges“**, obwohl das Testcase \`Succeeded\` meldet. Das stützt die Quellcodefeststellung zu ausgelassenen Parameterevents, ist jedoch keine gemessene PCM-Abweichung. Weiter meldet der Validator für Mix-ID 104 \`getParamValueByString("33")=0.330000\` statt ursprünglicher \`0.333333\`; dies ist für eine bewusst auf ganze Prozent gerundete Anzeige erklärbar, aber kein exakter Roundtrip. Der Validator bestätigt auch die bereits festgestellte Klassenversionsangabe **0.3.2**.

**Ausgeführte positive Windows-Nachweise:** 47/47 Steinberg Validator; 50/50 akzeptierte Sample-Rate-/Blocksize-Setups; Silence, Impulse (peak 0.666667, RMS 0.0294628), DC (peak 0.333333) und Denormal (1004 subnormale Samples) jeweils **finite Ausgabe**; 5 automatable Parameter mit 15 angenommenen Punkten in einem Block; 8 wiederholte Lifecycle-Stresszyklen; 5 vollständig angehängte/abgehängte Windows-Editorzyklen; Mono-/Stereo-Buswechsel; Controller-Isolation von zwei Instanzen (ID 100: erwartet B=1, beobachtet B=1). Gemeldete lokale CPU-Werte für 256 Blöcke @ 48 kHz: mean 18.0516 µs, p95 18.9 µs, p99 31.6 µs, max 34.6 µs, 0 Deadline-Überschreitungen; 32 separat host-getaktete Blöcke: max 168.7 µs, 0 Fristüberschreitungen. **Das sind echte begrenzte Benchmarks, keine ASIO-Freigabe.**

**Nicht ausgeführt / ausdrücklich nicht nachgewiesen:** Editorbericht \`SA01\` meldet \`NOT TESTED\` für Benachrichtigung mehrerer Views; I/O-Bericht \`SA02\` meldet \`NOT TESTED\` für block-partition-Äquivalenz; Multi-Instance-Probe grenzt DSP/Audio und zwei Views ausdrücklich aus. \`Automation stress ... accepted\` beweist nicht, dass die sample-offset-genauen Audioergebnisse stimmen. Auch die reinen finite-/Peak-Audio-Stresstests sind keine unabhängige Freeverb-Impulsantwort-, Wet/Dry- oder musikalische Klangprüfung. DAW und ASIO weiterhin NOT RUN.

**Urteilsänderung gegenüber dem Ursprungsbericht:** 125A Plugin Tester = **AUSGEFÜHRT, WARNING**, nicht \`NOT RUN\`. Die vier Warnungen sind neue konkrete Audit-Blocker, von denen insbesondere die \`Enable\`-Textkonvertierung bereits eindeutig fehlerhaft ist. **FULL AUDIT / RELEASE READY bleibt NO-GO.** Keine Reparatur am Produkt erfolgt durch diesen Nachtrag.
