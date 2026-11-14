# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> Stan na 14.11.2026. Przed zmianą kodu przeczytaj ten plik i `AGENTS.md`. Kolejność prac wyznacza **Aktywny plan** na początku pliku.

## Cel PrawkoNaRaz

Instruktor tworzy i edytuje sytuację drogową, odtwarza ją, wybiera kamerę i eksportuje PNG lub MP4 w jednym programie Windows. Funkcjonalnym punktem odniesienia jest VICOM. Najpierw doprowadzamy cały przepływ do działania na dostępnych modelach. Wspólny model C++ oraz OpenDRIVE i OpenSCENARIO obsługują edytor i agenta. [Zakres funkcji i kryteria produktu](docs/PRAWKO-VICOM.md)

## 14.11.2026 — PORZĄDKI: wyłącznie własna biblioteka Prawko (po poprawce dróg)

**Czysty katalog produktowy — implementacja i testy PASS.** Użytkownik świadomie zdecydował, że stare przykłady i prefabrykaty z RoadMaker były testowe; od nowa tworzymy własne komponenty. Aktywny manifest `assets/library/manifest.json` ma **12 produktów: 6 Prawko | Gotowe drogi, 6 Prawko | Krawężniki i chodniki**, a kod dodaje **6 Uczestników ruchu** (samochód, ciężarówka, autobus, motocykl, rower, pieszy). Łącznie **18 widocznych kafelków i tylko 3 kategorie**, w Map i Scenario. Wyszukiwanie, filtry, zewnętrzne overlaye i menu kontekstowe nie ujawniają starych kategorii. Upuszczenie aktora przełącza do Scenario. Ekran Welcome nie pokazuje dawnych scen przykładowych.

- Fizyczne czyszczenie zasobów: usunięte stare miniatury palety (w `editor/resources/resources.qrc` pozostało dokładnie 12 naszych), stare mesh-e `assets/library/props`, grafiki znaków `assets/signs/us` i `assets/samples`. Produkcyjne katalogi przykładów oraz `agent/projects` są **puste**. Przed usunięciem ośmiu lokalnych projektów testowych agenta wykonano sprawdzony ZIP: `archive/library-cleanup-20261114/agent-test-projects-backup.zip`.
- **Wewnętrzne wyjątki techniczne, NIE biblioteka produktu:** istniejące przykładowe pliki XODR/XOSC, modele OBJ/MTL i stare SVG przekierowano pod `core/tests/fixtures/legacy_scenes`, `legacy_prop_sources`, `legacy_sign_sources`. To dane do testów wstecznej zgodności formatów, nie zasoby oferowane użytkownikowi i nie gotowe sceny. Nie usuwać ich bez przeniesienia testów generowania/parsowania na syntetyczne fixtury. Powiązane ścieżki w CI, CMake, skryptach, dokumentacji i testach dostosowano. Repo agenta: `test_roadctl.py` wskazuje nową ścieżkę testową.
- Automatyczna kontrola `scripts/check_curated_library.py` wykrywa powrót starych plików albo kategorii, jest uruchamiana w CI. `scripts/check_asset_licenses.py` **PASS**. Testy Qt: **1337 PASS, 1 SKIPPED (symlink), 0 FAIL** z 1338. Testy C++ **2072 PASS**, Python agenta **7 PASS**, `QUALITY_GATE_PASS` przy `scripts/prawko-verify-win.ps1`. Pełny build Windows **BUILD_PASS: all**, synchronizacja Python/bindingów **AGENT_BINDING_SYNC_PASS**.
- GUI: zbudowano nowe standardowe `build/dev-windows/editor/roadmaker-editor.exe`. Próba zrzutu w oddzielnym oknie PID 21432 została przerwana komunikatem `TEST_EDITOR_NOT_FOCUSED` (inne okno ma fokus), więc **nie uznawać ręcznego odbioru myszy za ukończony**; własny proces testowy zamknięto bez zapisywania scen. Testy Qt sprawdzają kategorie, 18 ikon, wyszukiwanie, overlaye, dostępność aktorów w Map/Scenario i działanie upuszczania kolejnych dróg.
- **Następne zadanie:** dodać do biblioteki kolejną kategorię dopiero po zaprojektowaniu i wykonaniu własnych realistycznych elementów 3D PrawkoNaRaz, odpowiednich testach i świadomym rozszerzeniu kontroli manifestu. Krótki odbiór w GUI, gdy użytkownik nie korzysta z myszy: uruchomić nową pustą scenę, sprawdzić wyłącznie 3 kategorie i upuszczenie naszych dróg, chodników i aktora. Przed kolejnym etapem sprawdzić aktualne HEAD i oba repo. Nie przedstawiać historycznych testowych XODR jako aktywnych scen. Aktualne pełne SHA i wyniki `-Action finish` potwierdzić po commit/push.

## 14.11.2026 — poprawka biblioteki: kolejne drogi na planszy (po 5b)

**Naprawa wykonana, pełne testy automatyczne PASS, niezapisane okno użytkownika nienaruszone.** Powód zgłoszenia „po pierwszej drodze nie mogę upuścić drugiej” był potrójny: (1) szablon z biblioteki trafiał w zaznaczony koniec starej drogi i narzędzie `CreateRoadTool` interpretowało go jako polecenie **przedłużenia** tej samej drogi; (2) ponowny drop przy już aktywnym `Create Road` dopisywał punkty do niezakończonego szkicu; (3) w trybie `Scenario` akcja `Create Road` była wyłączona i drop niczego nie aktywował. Teraz `MainWindow::on_library_drop` dla `RoadTemplate` przełącza Scenario→Map, czyści wybór starej drogi, aktywuje narzędzie i fokus, a `CreateRoadTool::begin_at` zaczyna **nową** sesję bez trybu extend. Zwykłe, świadome wydłużanie zaznaczonej drogi przez narzędzie toolbaru nadal działa. Toast, wskazówka narzędzia i etykieta biblioteki mówią po polsku, że do utworzenia drogi trzeba dodać punkty i zatwierdzić Enter/dwuklikiem.

- Kod: `editor/src/app/main_window.cpp`, `editor/src/tools/create_road_tool.cpp/.hpp`, `editor/src/panels/library_panel.cpp`. Testy: `editor/tests/test_create_road_tool.cpp` (druga i trzecia droga, stara wybrana, drop na jej końcu, Undo/Redo i zapis XODR; nowy drop zastępujący niedokończony szkic); `editor/tests/test_main_window_layout.cpp` (prawdziwe zdarzenia Qt `QDragEnterEvent`/`QDropEvent` do `ViewportWidget`, kliknięcia i Enter, zapis przez menu, realna liczba `<road>` w pliku **1→2→3**; między dropami tryb Scenario i automatyczny powrót do Map; bez błędów diagnostycznych).
- **Testy i build:** początkowo bezpieczny `TARGET_BUILD_PASS` na testach; po zamknięciu poprzedniego okna przez użytkownika **`scripts/prawko-build-win.ps1 -Target all -Jobs 3` → `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`** i finalny build `roadmaker-editor` + `roadmaker_editor_tests` → PASS. **77 celowanych Qt PASS**, pełny Qt **1339 PASS, 1 SKIPPED (test symlink), 0 FAIL** z 1340 testów i prawdziwym esmini 3.5.0; logi `archive/road-multi-drop-20261114/qt-targeted.log`, `qt-full.log`. Niezależny plik `roadmaker-editor-multiroads.exe` (SHA256 `75A92652A19CC16AB1E0781A794211BE2E5673F9DD26BD003D51244E210A1309`) powstał jeszcze przed normalną kompilacją; **standardowy `build/dev-windows/editor/roadmaker-editor.exe` został następnie przebudowany pomyślnie**. Testy integracyjne używają wyłącznie kopii / `QTemporaryDir`.
- **Ręczny odbiór kursorem Windows — PASS:** na początku poprzedni edytor użytkownika PID **11064** z `scene.xodr*` był otwarty i niezapisany, dlatego go nie dotykano. Gdy użytkownik sam zamknął swoje okno (kontrola procesów pokazała brak `roadmaker-editor`), wykonano normalny build, uruchomiono oddzielne okno PID **6160** na **kopii** `archive/road-multi-drop-20261114/gui/roads-acceptance.xodr`, a realnym systemowym kursorem przeciągnięto pierwszy szablon `road.prawko.urban` z Library na trawę i zatwierdzono kolejnym kliknięciem + Enter; potem bez przełączania narzędzia przeciągnięto drugi szablon z biblioteki i zatwierdzono. Screeny `gui/baseline.png`, `drop1.png`, `finish1.png`, `drop2.png`, `finish2.png`, `saved.png` pokazują realny przebieg z trzema widocznymi jezdniami. Zapisana kopia XODR ma dokładnie **3 drogi** (id 1 `Ulica szkolna`, 2 `Road 2`, 3 `Road 3`) — oryginalny sample i sceny użytkownika pozostały nietknięte. Okno testowe zostało normalnie zamknięte. Podczas testu zauważono, że dolny licznik błędnie nadal pokazywał `1 roads` aż do ponownego otwarcia; osobnym małym patchem po odbiorze podłączono go do `Document::topology_changed` i dodano regresję w teście Qt obejmującą licznik 1→2→3 i Undo/Redo. Nie twierdzić, że ten ostatni licznik był już ręcznie przetestowany myszą — regresja Qt po poprawce jest PASS.
- **Handoff:** `agent_head=35c48c11cdb7efca8e08d1be731463657bbe7a32`, agent repo bez zmian. Pierwsza poprawka 3 przyczyn została scalona do commitu **`98244ac2bf5059600019b97b6c03f944c2f3c396`**, wypchnięta na `feature/realistic-road-surfaces` i po niej `scripts/prawko-git.ps1 -Action finish` potwierdził `QUALITY_GATE_PASS`, `SESSION_FINISH_PASS`, kopie obu repo + scen. Następna mała zmiana licznika po ręcznym odbiorze wymaga osobnego końcowego commitu, push i ponownego `-Action finish`. Przy kontynuacji zaczynać od najnowszego HEAD, pełnego Quality Gate i raportu aktualnej sceny. Sam drag z biblioteki nadal rozpoczyna rysowanie: zakończenie wymaga punktu i Enter/dwukliku.

## Stan bieżący

- **Punkt wyjścia etapu 5a.** Edytor `feature/realistic-road-surfaces` na `798812aec48fd3d5e53430626779073d4fd4e968`, agent `main` na `35c48c11cdb7efca8e08d1be731463657bbe7a32`. `prawko-resume.ps1 -Verify` potwierdził czyste katalogi i zgodność obu z `origin` przed zmianami.
- **Wdrożono:** przycisk `Scenario` i `View → Scenario Mode`. Tryb wyświetla narzędzia uczestnika oraz trasy. `File → Export Scene Image…` zapisuje PNG w rozdzielczości aktualnego widoku z bieżącej kamery. Eksport pomija siatkę, prowadnice, podświetlenia i narzędzia, zapis przez `QSaveFile`.
- **Wdrożono przebieg w tle.** `Scenario → Run Simulation` uruchamia esmini bez okna, z możliwością anulowania i limitem 60 s czasu rzeczywistego. Edytor używa `dat2csv` z katalogu tej samej dystrybucji. Wynik w pamięci ma parę XODR/XOSC ze skrótem SHA-256, wersję silnika, krok 0,05 s, ziarno 123, próbki pełnej pozy aktorów, identyfikatory dróg/pasów, start i koniec zdarzeń oraz kolizje. Raport wskazuje zdarzenia nierozpoczęte, niedokończone lub o niejednoznacznych nazwach. Edycja lub otwarcie innej sceny oznacza raport jako pochodzący z poprzedniej wersji.
- **Wdrożono odtwarzanie w widoku 3D.** Panel `Simulation` ma `Play/Pause`, `From Start`, suwak z kreskami zdarzeń, listę `Jump to event…` i `Return to Editing`. Podgląd pobiera pozycję i orientację każdego aktora z telemetrii, interpoluje między sąsiednimi próbkami, zachowuje rozmiar bryły aktora i aktualizuje tylko transformacje instancji. Kamera i eksport PNG korzystają z bieżącej klatki. Zmiana geometrii (także podczas przeciągania), scenariusza albo otwarcie innego dokumentu wygasza podgląd i pozostawia raport.
- **Wdrożono eksport MP4.** Po aktualnej symulacji wybierz `Simulation → Export MP4…` lub `File → Export Scenario Video…`. Ustaw zakres czasu, 720p lub 1080p oraz 24, 25, 30 albo 60 kl./s. Film bez dźwięku korzysta z bieżącej kamery i czystego renderu widoku 3D. Aplikacja wykrywa FFmpeg obok edytora lub na `PATH`, pozwala też wskazać plik z obsługą `libx264` i zapamiętuje ścieżkę po sukcesie. Pokazuje postęp i umożliwia anulowanie. Dotychczasowy plik pozostaje na dysku aż do udanego zakończenia kodowania.
- **Weryfikacja.** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, **2072 PASS C++** (10 wyłączonych), **7 PASS Python**, **1316 PASS Qt + 1 SKIPPED** z esmini 3.5.0 i FFmpeg 9.0.2, `QUALITY_GATE_PASS`. GUI Windows wyeksportowało z kopii `run-maneuver.xodr` zakres 1,95–4,95 s. `ffprobe` potwierdził H.264, 1920 × 1080, 30 kl./s, 90 klatek i 3,000 s. W klatkach 0 i 60 oba pojazdy przesuwają się zgodnie z telemetrią, a Target zmienia pas. Anulowanie ponownego eksportu zachowało SHA-256 filmu, otwarcie innej sceny wyłączyło eksport poprzedniego przebiegu. Test integracyjny potwierdził ochronę pliku także przy błędzie przechwycenia i błędzie procesu. Dowody i film są w `archive/simulation-stage3-20261113/`. Skróty kopii XODR/XOSC i oryginalnej sceny miejskiej pozostały identyczne.
- **Wdrożono spójny zapis i odzyskiwanie.** Edytor przygotowuje XODR, XOSC, ustawienia widoku i ewentualny teren w osobnym katalogu. Zachowuje poprzednie pliki na czas wymiany. Błąd pozostawia dokument niezapisany, a dziennik przerwanego zapisu jest wycofywany przy następnym otwarciu lub zapisie. `Save As` aktualizuje `LogicFile` i nadaje kopii terenu nazwę od nowej sceny. Autosave obejmuje XOSC i widok, a kopie nadal aktywnej sesji nie pojawiają się na liście odzyskiwania. Starsze autosave odzyskują ostatni zapisany XOSC z pierwotnego katalogu.
- **Odbiór zapisu.** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, `QUALITY_GATE_PASS`. Zaliczono **2072 C++** (10 wyłączonych), **7 Python**, **1322 Qt + 1 SKIPPED**, w tym **53/53 regresji zapisu**. W Windows odzyskano kopię z `event_recovered` i kamerą ortograficzną, następnie wymuszono błąd XOSC. Edytor zachował oznaczenie niezapisanych zmian, a obok blokady nie pojawiły się XODR ani ustawienia widoku. Zapis pod nową nazwą dał komplet plików. Ponowne otwarcie zapisanej kopii w edytorze i uruchomienie symulacji potwierdziło start odzyskanego zdarzenia o 1,95 s oraz koniec o 3,90 s. SHA-256 kopii źródłowej i oryginalnej sceny miejskiej pozostały identyczne. Dowody są w `archive/scenario-stage4-save-20261113/`.
- **Wdrożono kamerę kierowcy i scenę dydaktyczną.** W panelu `Simulation` wybór `Camera` zapisuje się z dokumentem. Podgląd, PNG i MP4 wyliczają pozycję oraz orientację oczu z tej samej interpolowanej pozy aktora. Klatka bez pozy wybranego pojazdu używa kamery edytora. Na ekranie startowym są dwa warianty ćwiczenia o widoczności przed przejściem. Parkujący dostawczak zasłania pieszego w jednym wariancie. Ruch pieszego i auta jest taki sam, a oba przebiegi kończą się bez kolizji.
- **Odbiór kamery i wariantów.** W Windows zapisano i ponownie otwarto dwie kopie oraz sprawdzono przywrócenie `Driver view — Auto`. Wyeksportowano po dwa filmy MP4 z kabiny i z góry. Wszystkie mają H.264, 1920 × 1080, 30 kl./s, 302 klatki i 10,067 s. W 2,70 s pieszy jest widoczny w wariancie bez dostawczaka oraz zasłonięty w drugim. W 5,40 s widać go już na przejściu także w drugim wariancie. Zgodność auto/pieszy potwierdza telemetria esmini 3.5.0. Walidator agenta zgłosił zero błędów dla obu par. `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, **1325 Qt + 1 SKIPPED**, **2072 C++** (10 wyłączonych), **7 Python**, `QUALITY_GATE_PASS`. Dowody są w `archive/scenario-stage4-driver-20261114/`. Pliki obu przykładów i oryginalna scena miejska zachowały skróty SHA-256.
- **Wdrożono edycję kursorem.** Tryb `Scenario` otwiera bibliotekę **Uczestnicy ruchu**. Upuszczenie pojazdu zaznacza go i włącza `Trasa`. Kliknięcie połączonego pasa wyznacza trasę od miejsca postawienia. Pieszego można postawić poza jezdnią i kliknąć punkty przejścia. Węzły można przeciągać, a `Zakończ trasę` lub Enter wraca do zaznaczania. Ścieżka pieszego jest zapisywana jako otwarta, czasowa polilinia w `FollowTrajectoryAction` w `Init`. Używa prędkości 1,4 m/s i zatrzymania na ostatnim punkcie. Początkowa prędkość pojazdu bez własnej nastawy to 30 km/h. Złożone trajektorie importowane pozostają w XOSC. Storyboard jest w menu `Scenario → Storyboard…`.
- **Weryfikacja edycji kursorem.** Kompilacja i synchronizacja biblioteki Pythona ukończone. **1329 Qt PASS + 1 SKIPPED**, **2072 C++ PASS** (10 wyłączonych), **7 Python PASS**, `QUALITY_GATE_PASS`. Test Qt dostarcza zdarzenia `QDragEnterEvent`, `QDropEvent` i kliknięcia do rzeczywistego `ViewportWidget`, następnie zapisuje XOSC z trasą pojazdu i pieszego. Osobny test potwierdził zapis, odczyt i Undo polilinii. Esmini 3.5.0 potwierdził ruch pieszego z y = −6 do +6 m i pozostanie na ostatnim punkcie. Logi w `archive/scenario-cursor-20261114/`. Skróty oryginałów 4/4 zgodne.
- **Otwarte.** Trajektorie wyzwalane przez zdarzenia (w tym ruch pieszego z przykładów przejścia) nie mają jeszcze edycji węzłów kursorem. Pojazd korzysta z trasy do wyboru drogi, a zatrzymanie na końcu wymaga osobnej akcji hamowania. W działającym oknie Windows potwierdzono widoczność palety, lecz narzędzie przechwytywania nie udostępniło obrazu ani geometrii do ręcznego przeciągnięcia. FFmpeg jest zewnętrzną zależnością. Podgląd używa brył dla `Vehicle` i `Pedestrian`. Odtwarzanie świateł i obsługa aktorów z katalogów pozostają do rozbudowy. Relatywne odwołania do zewnętrznych modeli i katalogów w importowanym XOSC mogą wymagać dostosowania przy zapisie do innego folderu.

## 14.11.2026 — etap 5b, podetap: zaznaczanie uczestników i przycisk symulacji

**Kod i testy automatyczne potwierdzone; odbiór pełnego GUI Windows otwarty.** Przejęto bez cofania 17 zmian pozostawionych po wyczerpaniu limitu poprzedniego agenta na `c12f872`. Zmiany w `ViewportWidget`, `picking`, `actor_placement`, `SelectTool`, `ActorTool`, `RouteTool` trafiają promieniem w widoczną obróconą bryłę aktora przed drogą, ograniczają chwytanie najbliższą przeszkodą i podświetlają zaznaczonego uczestnika. `MainWindow` ma akcję `run_scenario_simulation` na głównym toolbarze, zaraz po `Scenario`; dostępność zależy od uczestników i aktywnej symulacji, klik otwiera dock oraz uruchamia odtwarzanie po obliczeniu przebiegu. `Select` i `Trasa` opuszczają podgląd, aby można było znowu edytować.

- Testy kompilacji: `cmake --build build/dev-windows --target roadmaker_editor_tests` **PASS** bez naruszania otwartego `roadmaker-editor.exe`; komplet Qt **1326 PASS, 6 SKIPPED, 0 FAIL** (`archive/scenario-stage5b-20261114/stage5b-full-qt.log`); testy celowane **79 PASS, 5 SKIPPED** (`stage5b-targeted.log`), obejmują ray-pick, toolbar i edycję trasy.
- `scripts/prawko-resume.ps1 -Verify` na niezacommitowanym stanie wykonał **2072 PASS C++**, **7 PASS Python**, `QUALITY_GATE_PASS`, ale zwrócił kod 1 wyłącznie przez dirty Git (skrypt celowo wymaga czystego repo). Test zewnętrznego esmini (`SimulationPanel.ToolbarRunStartsPlaybackWhenTheCurrentRunFinishes`) pozostał SKIPPED, bo w środowisku procesu nie odnaleziono `ESMINI_PATH`.
- **Uwaga na niezapisany dokument użytkownika:** aktywny proces `roadmaker-editor.exe` PID 12196, tytuł `scene.xodr*`. Nie zatrzymywać, nie podmieniać wykonywalnego edytora w czasie jego działania, nie zapisywać ani nie wykonywać akcji myszą w tym oknie. Z tego powodu nie przeprowadzono pełnego `prawko-build-win.ps1 -Target all` ani fizycznego testu przycisku/myszy w nowym EXE. Testy Qt są `offscreen`, nie zastępują odbioru w widocznym Windows.
- **Pierwsze następne zadanie:** po zamknięciu przez użytkownika niezapisanej sceny (albo budowie odizolowanego EXE do testów) wykonać pełny build, test GUI na osobnej scenie: klik auta/pieszego bez zaznaczania drogi, podświetlenie, włączanie/wyłączanie trybu `Trasa`, działanie przycisku `Uruchom symulację` i automatyczne Play z prawdziwym esmini; poprawić ewentualne błędy. Następny podetap 5b obejmuje ustawienia prędkości i edycji trasy (opisane poniżej), później edycję chwili startu, zachowania na końcu oraz ścieżek wyzwalanych zdarzeniami bez utraty triggerów. Nie oznaczać całego etapu 5b jako ukończonego. Oryginały scen niezmienione.

## 14.11.2026 — etap 5b, podetap: prędkość w km/h i Edytuj trasę

**Kod i testy Qt potwierdzone; odbiór nowego UI w odizolowanym oknie Windows pozostaje otwarty.** W panelu Properties dla zaznaczonego uczestnika pole `Prędkość początkowa` prezentuje teraz km/h, a `push_actor_speed` zapisuje m/s w istniejącym `osc::edit::set_entity_init_speed`. Test pilnuje, aby zaokrąglenie do 0,01 km/h nie nadpisywało precyzyjnej wartości prędkości importowanej z XOSC i nie tworzyło zbędnego kroku Undo. Nie ingeruje to w nieobsługiwany `RelativeTargetSpeed` (pozostaje tylko do odczytu). Nowy przycisk `Edytuj trasę` w Properties przekazuje zaznaczonego aktora do istniejącego `MainWindow::actions_->tool_route`, bez tworzenia alternatywnego modelu rysowania, zapisuje te same trasy XOSC.

- `editor/tests/test_panels.cpp`: test pola km/h→m/s (54→15), brak wpisu Undo dla zera i zaokrąglonego odczytu, Undo/Redo, oraz klik przycisku tylko przy właściwym zaznaczeniu. `editor/tests/test_main_window_layout.cpp`: integracyjny test kliknięcia `Edytuj trasę` w miejsce ręcznego otwarcia narzędzia; zmiana trasy trafia do XOSC.
- Zbudowano `roadmaker_editor_tests` przez MSVC/Ninja **bez podmiany otwartego pliku roadmaker-editor.exe**; testy celowane `178 PASS, 5 SKIPPED` w `archive/scenario-stage5b-20261114/stage5b-targeted.log`; pełny Qt `1327 PASS, 6 SKIPPED, 0 FAIL` w `stage5b-full-qt.log` (pięć testów esmini bez `ESMINI_PATH`, jeden test symlink zgodnie z poprzednim pominięciem). Wcześniejszy Quality Gate na tym samym bazowym HEAD: 2072 C++ PASS, 7 Python PASS, QUALITY_GATE_PASS. Ponowny `-Action finish` po commicie zweryfikuje oba repo.
- `roadmaker-editor.exe` PID 12196 zawiera nadal **niezapisaną** scenę `scene.xodr*`: nie zatrzymywać, nie uruchamiać podmiany exe ani nie przejmować jego myszy. Nie przetestowano dlatego fizycznej interakcji w tym nowym buildzie. Oryginalne sceny użytkownika pozostają nietknięte.
- **Pierwsze otwarte zadanie 5b:** bezpieczny osobny GUI build/test na kopii i odbiór myszy dla wyboru brył, edycji trasy, km/h i `Uruchom symulację`; wskazać dostępny binarny esmini z `dat2csv` i potwierdzić Auto Play. Potem w odrębnym podetapie: edytowalna chwila startu, zachowanie na końcu trasy, trasy wyzwalane zdarzeniami BEZ utraty warunków `StartTrigger`; nie utożsamiać trasy po pasach z automatycznym zatrzymaniem auta. Nie oznaczać 5b jako kompletnego.

## 14.11.2026 — etap 5b: swobodne przemieszczanie uczestników i pełny odbiór Windows

**Wykonane i potwierdzone:** naprawiono brak przeciągania samochodu w trybie **Select** i **Move**. Wcześniejszy `SelectTool::mouse_move` świadomie ignorował `PressState.actor`, choć oddzielny `ActorTool` umiał przestawiać aktora. Oba narzędzia korzystają teraz z tej samej logiki przeciągania; rozpoznanie bryły w `ViewportWidget` obsługuje także hover. LMB w aktora nie rozpoczyna przesuwania jezdni. `Ctrl`/`Shift` zachowują funkcję wielokrotnego zaznaczania, `Esc` anuluje gest. Podgląd upuszczenia używa `ActorTool`.

- **Zachowanie XOSC:** zmiana pozycji pojazdu z trasą `Init/AssignRouteAction` synchronizuje jej pierwszy waypoint wyłącznie gdy był zgodny ze starą pozycją; dwa polecenia scenariusza stanowią jedno Undo/Redo przez `QUndoStack::beginMacro/endMacro`. Gdy trasa ma niezależny początek, nie jest nadpisywana. Dla pieszego z `Init/FollowTrajectoryAction` przesuwana jest pierwsza pozycja polilinii z aktualizacją czasu pierwszego odcinka; końcowy postój i następne odcinki zachowują względne czasy. Przeprowadzenie podobnej edycji dla trajektorii wyzwalanych przez **Storyboard Event** nadal pozostaje osobnym otwartym zadaniem (nie było zmieniane).
- **Testy Qt:** pełna kompilacja `scripts/prawko-build-win.ps1 -Target all -Jobs 3` zakończyła się `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`. Z oficjalnymi esmini **3.5.0** i `dat2csv` ustawionymi przez `ESMINI_PATH`: **1336 PASS, 1 SKIPPED (symlink), 0 FAIL** z 1337 Qt testów. Log `archive/scenario-stage5b-actor-drag-20261114/qt-full.log`. Regresje: `SelectTool.DraggingCarAcrossLaneMovesOnlyActorWithOneUndoAndRedo` w Select/Move; `CarCanBeDraggedOntoAnotherRoadWithoutMovingEitherRoad`; `ActorInvalidDropAndEscapeLeaveRoadAndActorUntouched`; `ActorTool.MovingVehicleMovesMatchingRouteStartInOneUndo`; `MovingPedestrianAlsoMovesAuthoredWalkingStartAndTiming`. Testy prawdziwego esmini, wcześniej SKIPPED, tym razem przeszły. Binary esmini dostępne lokalnie w `archive/scenario-stage5b-actor-drag-20261114/esmini/extracted/esmini/bin/esmini.exe` (archiwum ignorowane przez Git).
- **Pełny test systemową myszą, tylko kopia** `archive/scenario-stage5b-actor-drag-20261114/gui/accept-actor.{xodr,xosc,rmscene.json}`: kliknięcie samochodu Auto, przeciągnięcie wzdłuż pasa w górnym widoku, porównanie zrzutów `baseline.png`, `clicked.png`, `dragged.png`, `undo.png`, `redo.png`, `saved.png`, `reopened.png`, `run.png`, `returned-edit.png`. **Undo/Redo wizualnie PASS**, plik XOSC po Ctrl+S i ponownym otwarciu: `Auto` i pierwszy punkt trasy mają zgodne `s=39.08641772143433 m` (przed 12 m), ten sam `road=1`, `lane=-1`; Pieszy `x=58, y=-6.6` bez zmian. W XODR zapis normalizuje istniejący atrybut pasa środkowego `level=\"false\"`, przez co surowy SHA pliku po zapisie jest inny; **geometria planView, elevationProfile, lateralProfile, pasy, obiekty, sygnały i długość drogi są semantycznie identyczne** (`gui/compare-road.py`). Oryginalny sample `assets/samples/prawko_crosswalk_clear.*` i sceny użytkownika nie były zapisywane.
- **Odbiór przycisku i symulacji:** ponownie otwarta kopia, klik `Uruchom symulację` na górnej belce, wynik **Simulation finished**, automatyczne Play (widoczny Pause i postępujący suwak), kamera `Driver view — Auto`, esmini 3.5.0: **2 aktorów, 404 próbki, 10.05 s**. Po zakończeniu podgląd wrócił do edycji, a okno testowe zostało normalnie zamknięte. Żaden katalog oryginalnego projektu XODR/XOSC ani niezapisana scena użytkownika nie zostały naruszone.
- **Ograniczenia / następny krok 5b:** ścieżki wyzwalane eventami (np. pieszy z przykładu `prawko_crosswalk_clear`) nadal nie mają obsługi przeciągania powiązanej trajektorii z zachowaniem `StartTrigger`; wymagają osobnego testu i implementacji. Pozostają ustawienia czasu rozpoczęcia ruchu i zachowania na końcu trasy przy wybranym uczestniku, w tym hamowania pojazdu. Nie deklarować całego 5b zakończonym ani pełnego fotorealizmu. Przed kontynuacją nowy agent uruchamia `prawko-resume.ps1 -Verify`.

## Aktywny plan

### 1. Ukończono — kontrolowany przebieg esmini w tle

Punkt wejścia stanowią [panel symulacji](editor/src/panels/simulation_panel.cpp) i [wynik przebiegu](editor/src/document/esmini_simulation.hpp). Testy z prawdziwym silnikiem uruchamiają się po ustawieniu `ESMINI_PATH` na plik esmini z dystrybucji zawierającej również `dat2csv`.

### 2. Wdrożono — odtwarzanie w istniejącym widoku Qt

Punkt wejścia stanowią [panel symulacji](editor/src/panels/simulation_panel.cpp), [odczyt klatek przebiegu](editor/src/document/simulation_trace.hpp) oraz `ViewportWidget::set_simulation_frame()`. Podgląd i przechodzenie po klatkach pozostawiają mapę, scenariusz i Undo w dotychczasowym stanie. Odbiór aktora i trasy myszą powtórz na kopii przy następnym działającym przechwytywaniu okna.

### 3. Ukończono — pierwsze nagranie MP4

Punkty wejścia to `MainWindow::export_scenario_video()`, [proces eksportu](editor/src/document/scenario_video_export.cpp) i `ViewportWidget::capture_simulation_frame()`. MP4 i PNG korzystają ze wspólnego renderera. Próbkowanie filmu używa `SimulationPlayback::poses_at()` i czasu wyliczanego z numeru klatki.

### 4. Scena wzorcowa, kamery i bezpieczny zapis

**4a. Wdrożono bezpieczny zapis i odzyskiwanie.** Punkty wejścia to `Document::save()`, `Document::load_recovery()`, [moduł zapisu](editor/src/document/scene_storage.cpp) i `AutosaveManager`.

**4b. Wdrożono kamerę kierowcy i scenę wzorcową.** Punkty wejścia to `SimulationPanel::present_frame()`, `ViewportWidget::simulation_frame()` i `driver_camera()`. `SceneState::driver_view_actor` przechowuje nazwę wybranego pojazdu. Przykłady `prawko_crosswalk_clear` i `prawko_crosswalk_obstructed` są w `assets/samples/`.

### 5. Dalsze funkcje wzorowane na VICOM

**5a. Wdrożono bibliotekę uczestników i rysowanie ruchu kursorem.** Punkty wejścia to `MainWindow::on_library_drop()`, `RouteTool` i `osc::edit::set_init_polyline()`. Instruktor ma dodawać i edytować ruch kursorem. Parametry techniczne są dostępne na żądanie.

**5b. Następne zadanie — wygodne ustawienia ruchu i odbiór kursorem.** Na kopii zwykłej mapy sprawdź realną myszą przeciągnięcie auta i pieszego, wyznaczenie tras, korektę węzła, cofnięcie, ponowne otwarcie i uruchomienie. Dodaj przy zaznaczonym uczestniku czytelne ustawienia prędkości (km/h), chwili startu i zachowania na końcu oraz przycisk edycji trasy. Podepnij ustawienia do tego samego XOSC i pozwól edytować obsługiwane ścieżki zdarzeń bez utraty wyzwalaczy. Kryterium odbioru to ustawienie auta i pieszego oraz nagranie przejścia w jednym oknie bez otwierania Storyboard dla typowej sceny.

**5c. Szablony i warianty.** Dodaj tworzenie niezależnej kopii szablonu z ekranu startowego i menu `File`, z zachowaniem spójnego zapisu i kontroli niezapisanych zmian. W dalszym kroku dodaj szablony miejskie, pozamiejskie i autostradowe, podmianę otoczenia, przypisywanie ról pasów i stref oraz raport odwołań do korekty.

**5d. Warunki i nagrania.** Dodaj pogodę, porę dnia i sezon, dodatkowe ujęcia, kokpit, presety oraz eksport wielu wariantów. Do dopracowania modeli i materiałów wróć po potwierdzeniu przepływu pracy.

## Gotowe polecenie do nowej rozmowy

> Kontynuuj PrawkoRoadStudio w `F:\PrawkoRoadStudio`. Przeczytaj `HANDOFF.md` i `AGENTS.md`, uruchom `scripts/prawko-resume.ps1 -Verify` i potwierdź stan obu repozytoriów GitHub, ostatnie commity oraz testy. Kontynuuj pierwszy niewykonany etap z **Aktywnego planu**. Testuj na kopiach i nie nadpisuj niezapisanych scen. Po zakończeniu etapu wykonaj testy, uaktualnij stan i kolejny krok w `HANDOFF.md`, zacommituj, wypchnij i wykonaj `scripts/prawko-git.ps1 -Action finish`.

## Historia prac

Poniższe wpisy dokumentują wcześniejsze odbiory i wyniki. Bieżące zadanie znajdziesz w sekcji **Aktywny plan**.

## 13.11.2026 — odtwarzanie w edytorze Windows

- `c26421ea81a95c241a149e89e6e52704310c1406` dostarczył odtwarzanie w 3D, pauzę, suwak i znaczniki zdarzeń. W klatkach 0,00 / 1,95 / 3,90 s na kopii `run-maneuver.xodr` przesunięcia obu brył zgadzały się z telemetrią z dokładnością około 1 px. Test Qt porównał skok z listy zdarzeń z pozycją esmini i potwierdził wygaszenie po edycji drogi. Dowody są w `archive/simulation-stage2-20261113/`.

## 13.11.2026 — pierwszy przebieg esmini w tle

- `5bd64dada0aa1c2b7efc4535d5913030c1d8f085` dostarczył panel raportu, anulowanie i telemetrię. W odbiorze Windows wariant 4 m rozpoczął `event_1` po 1,95 s i zakończył po 3,90 s. Pierwsza kolizja pojawiła się po 4,50 s. Wariant 15 m nie rozpoczął zdarzenia. Obie próby dały 35,05 s, 2 aktorów i 1404 próbki. Dowody zachowano w `archive/simulation-stage1-20261113-202759/`.

## 13.11.2026 — audyt scenariuszy dla szkoły jazdy PrawkoNaRaz

**Stan przed etapem VICOM:** przegląd kodu i obu repozytoriów, pomiary esmini oraz wcześniejsze propozycje dla kursantów są w [PRAWKO-SCENARIUSZE-DYDAKTYCZNE.md](docs/PRAWKO-SCENARIUSZE-DYDAKTYCZNE.md).

- **Stan produktu:** istnieje wspólny model C++ dróg, aktorów, tras, zdarzeń i sygnalizacji, a Qt eksportuje migawkę do esmini. W działającym Windows Qt na kopii `cut_in` widoczny jest podgląd esmini, a `View` nie zawiera `Scenario Mode`. Kod tworzy akcję bez domyślnego skrótu i podpina tryb, ale nie dodaje przełącznika do menu. `Document::save` oznacza dokument jako czysty także po błędzie XOSC. Autosave zapisuje mapę i ustawienia widoku bez XOSC.
- **Symulacja na kopiach:** świeże `cut_in` z 0 błędów walidacji dwukrotnie kończy się kodem 0 bez manewru. W esmini 3.5.0 przy kroku 0,05 s oba CSV mają taki sam SHA-256. Warunek `rising` z progiem 15 m jest spełniony już w pierwszym kroku, co kontrola `none` wykazała przy odległości 7,21 m. Próba z progiem 4 m przy `rising` wyzwala zmianę pasa o 1,95 s i zgłasza kolizję o 4,50 s. `urban_crossing` kończy próbę po 30,05 s, a oba pojazdy ruszają od skrzyżowania. Logi, telemetria i kopie testowe są lokalnie w `archive/scenario-audit/2026-11-13/`.
- **Repo i testy:** punkt wyjścia edytora `b6ed5d7bcd15a1d1643148037eb2a322b18d4e41`, agenta `35c48c11cdb7efca8e08d1be731463657bbe7a32`, oba potwierdzone w `origin`. Odzyskany test obrotu i dowód z zachowanych kopii zapisano osobno w `9552a8e6302f96599940a4c861183b1e9eec811f` i wypchnięto. Pełny build `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, C++ 2072 PASS (10 wyłączonych), Qt 1304 PASS + 1 SKIPPED, agent 7 PASS, `QUALITY_GATE_PASS`, demo 0 błędów. Esmini 3.5.0 przyjął 3 poprawne repozytoryjne XOSC i odrzucił 1 uszkodzony. Testy esmini i GUI korzystały z kopii. Oryginalnych scen nie zapisano. SHA256 `agent/projects/prawko_urban_curve/scene.xodr` pozostał `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e`.

## S2.2o — test rzeczywistego prefabrykatu i korekta przeciągania (13.11.2026)

**Zakończony podetap: wybór cienkiego słupa, bezpieczne przesunięcie czterech elementów, Undo/Redo i zapis.** Nie oznacza pełnego końca S2.2o.

- Przy odtworzeniu błędu Traffic signal, mast arm LMB trafiało w drogę pod wąskim masztem. Dodano tolerancję 7 px do rzeczywistych rzutowanych krawędzi modelu tylko dla **już zaznaczonej grupy** w picking.cpp/.hpp, używaną przy mouse-press SelectTool. Zwykłe kliknięcie poza obiektem nie zostaje przechwycone. Test: PickScreen.ThinSelectedPrefabPoleHasLimitedPixelGrabWithoutStealingRoadClicks.
- Drugim, niezależnym błędem była próba przesuwania każdego fragmentu rm:assembly przez edit::move_object w kompozycie. Jądro poprawnie odmawia takiej operacji. SelectTool::update_object_move rozpoznaje rzeczywiste Object::assembly i korzysta z istniejącego move_object_command → **edit::move_assembly_by_part**, który przesuwa CAŁY sztywny prefabrykat jednym poleceniem. Swobodna multi-selekcja zwykłych obiektów nadal korzysta z kompozytu. Nieudana próba nie dodaje Undo ani fałszywego toastu sukcesu.
- Test SelectTool.RealFourPartSignalMastMovesAsOneRigidAssemblyOnMouseDrag: wszystkie cztery części przesunięte z zachowaniem offsetów, preview, jeden wpis Undo, byte-identical Undo, Redo. Testy docelowe **46/46 PASS**, pełne Qt **1303 PASS + 1 SKIPPED, 0 FAIL** (logi archive/visual-regression/s2.2o-qt-{targeted,full}.log). prawko-build-win.ps1 -Target all -Jobs 3: **BUILD_PASS all**, AGENT_BINDING_SYNC_PASS; prawko-resume.ps1 -Verify uruchomiony na dirty worktree: **2072 C++ PASS**, **7 Python PASS**, QUALITY_GATE_PASS, kod wyjścia 1 wyłącznie od wykrytego dirty Git.
- Rzeczywisty GUI Qt z kursorem Windows i osobną kopią archive/visual-regression/s2.2o-mouse/gui-check-20261113f.xodr: wstawiono czteroczęściowy maszt z biblioteki, kliknięto cienki element, przesunięto LMB, zapisano verified-before-f.xodr, verified-after-f.xodr, verified-undo-f.xodr, verified-redo-f.xodr. verify-assembly.py (analiza XML) potwierdza **4/4 części: identyczne Δs=+5.0914 m, Δt=−1.1220 m; geometria obu dróg BEZ ZMIAN; Undo/Redo PASS**. Zrzuty verify-gui-{placement,before-drag,after-drag,undo,redo}.png. Własny proces Qt zamknięty, oryginalnych scen nie zapisano. Wcześniejsze nieudane próby kalibracji GUI pozostawiono jako diagnostykę tylko w ignorowanym katalogu archive/.
- Odzyskany punkt kontrolny obrotu: zastany test `GizmoDrag.RotatingSignalMastAssemblyMovesAllFourPartsInOneUndo` sprawdzony w pełnym Qt (1304 PASS, 1 SKIPPED). Ponowna analiza zapisanych przez poprzedniego agenta kopii z rzeczywistej myszy (`archive/visual-regression/s2.2o-rotation-mouse2/verify-rotation.py`) potwierdziła obrót czterech części o −90°, zachowanie odległości w `s/t`, geometrii dróg i poprawność Undo/Redo. Bez osobnego odbioru ponownego otwarcia przez GUI ani innych prefabrykatów.
- Otwarte w S2.2o: odbiór ponownego otwarcia obróconej kopii, inne prefabrykaty i geometrie, stencil/marking.

## S2.2n — podetap 2: grupy prefabrykatów i końcowy odbiór myszą (13.11.2026)

**Wykonane i zweryfikowane:**

- Dokończono w `SelectTool` bezpośrednie przeciąganie `SignalId`
  (statyczny znak / dynamiczne światło) LMB w trybie Select lub Move.
  Zmienia `s/t` tylko względem **macierzystej drogi**, zachowuje
  `orientation` (kierunek obowiązywania) i `hOffset` (obrót
  wizualny). Po opuszczeniu dozwolonej odległości od drogi blokuje
  klatkę i wznawia po powrocie, `Esc` anuluje, zwolnienie
  zatwierdza **jeden** wpis Undo. Pierścień `Move` do obrotu
  znaku istniał wcześniej; potwierdzono jego działanie rzeczywistą
  myszą.
- Zaznaczone **dwa lub więcej obiektów 3D na jednej drodze**
  przesuwa się razem, jeśli użytkownik chwyci jeden z zaznaczonych
  elementów. `ObjectMoveState` zapamiętuje pozycje początkowe
  grupy; `edit::composite("Move selected prefab")` przesuwa
  je z jednakowym przyrostem `s/t` w podglądzie, jednym Undo.
  Jeśli jeden element przekracza długość macierzystej drogi albo
  dozwolony offset boczny, cała zmiana jest odrzucana, bez
  częściowego przestawienia. Dla różnych dróg pozostaje
  bezpieczny ruch jednego elementu, bez udawania translacji grupy.
  **To jest przesuw, nie obrót całej grupy.**
- `editor/tests/test_select_tool.cpp`: **5 nowych testów PASS**:
  znaki statyczne i dynamiczne światła, Undo/Redo, zachowanie
  `orientation/hOffset`, Esc i zwykły klik bez wpisu Undo,
  przebywanie poza drogą / powrót, translacja całej wybranej
  grupy i odmowa, gdy końcowy element przekracza granicę.
  `archive/visual-regression/s2.2n-targeted-final.log`.
- **Rzeczywisty Windows Qt, mysz systemowa, wyłącznie kopie scen**:
  `archive/visual-regression/s2.2n-mouse/signal-body-test.xodr`
  – po upuszczeniu znaku STOP panel Properties wskazał
  `s=134,221 m`, `t=-0,574 m`, `hOffset=0,052 rad`.
  Po bezpośrednim LMB drag:
  `s=113,762 m`, `t=1,802 m`,
  `hOffset=0,052 rad`, toast `Sign or traffic light moved`.
  Przy `Move` próba chwycenia niepodświetlonego fragmentu
  pierścienia wybrała drogę i ją przesunęła; **ten nieudany
  gest został cofnięty Ctrl+Z**. Następnie
  `sign-ring-hover-diagonal.png` pokazuje podświetloną obręcz,
  a właściwy jej obrót (`sign-ring-active-rotate-after.png`)
  zmienił **`hOffset=0,052 → -1,833 rad`** i wyświetlił
  `Rotated sign to -105° — Ctrl+Z to undo`. Wykonano
  Ctrl+Z/Ctrl+Shift+Z, Ctrl+S i zamknięto **tylko testowe PID18504**.
  Zrzuty `stop-sign-direct-move-after.png`,
  `sign-ring-active-rotate-after.png`,
  `sign-rotate-undo.png`, `sign-rotate-redo.png`.
- **Przejście na czystym wlocie T:** testowa kopia
  `archive/visual-regression/s2.2n-mouse/crosswalk-clean-test.xodr`
  powstała z `agent/projects/t_junction/scene.xodr`
  przez usunięcie 3 wcześniejszych `object type="crosswalk"`
  **tylko w kopii**. `crosswalk-clean-baseline.png` pokazuje
  T bez zebry, `clean-zebra-invalid-after.png` odrzuca
  upust poza wlotem, `clean-zebra-valid-after.png` dokumentuje
  wstawienie **jednej zebry i stop-line** (toast)
  na wlocie oraz zaznaczenie obiektu w Properties.
  Ctrl+Z/Ctrl+Shift+Z i zapis kopii; ponowny odczyt XML:
  **dokładnie 1 `object type="crosswalk"`**.
  PID24164 zamknięto normalnie; oryginalnego `t_junction`
  ani `prawko_urban_curve` nie modyfikowano.
- **Build/test:** `archive/visual-regression/s2.2n-final-build2.log`
  **`BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`**;
  `s2.2n-quality-gate.log` **`QUALITY_GATE_PASS`**;
  `s2.2n-qt-full.log` **1301 PASS, 1 SKIPPED, 0 FAIL**
  (1302 testów/160 suites).
  Przejściowy `LNK1168` w `s2.2n-assembly-build.log`
  wynikał z otwartej testowej instancji `roadmaker-editor.exe`.
  Testową scenę bezpiecznie zapisano i zamknięto, a pełny
  build w `s2.2n-final-build2.log` powtórzono do PASS.
  Oryginalne `agent/projects/prawko_urban_curve/scene.xodr`:
  SHA256 **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**,
  bez zmiany; projekt `reference/RoadMaker` nietknięty.

**Pierwsze niewykonane zadanie S2.2o:** test rzeczywistą myszą
przesuwania całego prefabrykatu `PropAssembly` (nie tylko
unit test z dwoma drzewami), a następnie parametryczny
**obrót całej grupy** z zachowaniem wzajemnych odległości,
snapowania i OpenDRIVE, także w scenach wielodrogowych.
Osobno skontrolować `Stencil`/`Marking`, blokadę
**drugiej zebry na tym samym wlocie** oraz udokumentować
końcowy wygląd z poziomu kierowcy. Nie zmieniać globalnego
CDT ani goldenów bez testu, nie oznaczać fotorealizmu PASS.

## S2.2n — podetap 1: ruch znaków/sygnalizacji myszą, test przejścia na X (13.11.2026)

**Stan: podetap wykonany w kodzie i sprawdzony na pojedynczym sygnalizatorze w widocznym GUI, lecz S2.2n jako całość otwarty.**

- Przejęto trzy zastane niezacommitowane pliki `editor/src/tools/select_tool.cpp/.hpp` i `editor/tests/test_select_tool.cpp` bez kasowania ich zmian. `SelectTool` rozpoznaje naciśnięcie na `PickHit::signal`, osobny ruch LMB po przekroczeniu tolerancji oraz aktywny preview `edit::move_signal` z przyciąganiem do **macierzystej** drogi. Nie przesuwa całej drogi; zachowuje `orientation` i `h_offset`. Esc przywraca stan, zwolnienie LMB kończy jeden Undo. Ruch poza progiem drogi zatrzymuje się na ostatniej poprawnej pozycji. Przy braku poprawnego preview zwolnienie nie udaje udanego ruchu.
- Testy `SelectTool`: statyczny znak i dynamiczny sygnalizator, snap/Undo/Redo i zachowanie kierunku, opuszczenie drogi i powrót, anulowanie Esc, zwykły klik bez Undo, odrzucenie całkowicie niepoprawnego przesunięcia bez Undo.
- Rzeczywiste zdarzenia kursora/przycisku Windows na **kopii** `archive/visual-regression/s2.2n-mouse/intersection-test.xodr` (16 dróg, skrzyżowanie X): `signal-click.png` wskazuje `signal tl3`, `s=8.000 m`, `t=-6.000 m`; po LMB drag `signal-drag-after.png` panel pokazuje `s=21.856 m`, `t=-2.239 m`, `hOffset=0.052 rad` bez zmiany. To test widocznego Qt, nie bezpośrednie wywołanie funkcji.
- W tej samej testowej kopii rzeczywisty drag Zebra crosswalk: poza wlotem odrzucony (`crosswalk-invalid-after.png`), na wlocie zgłoszony jako `Placed Zebra crosswalk + stop line`, dodany `object 11` i otwarte Properties (`crosswalk-valid-after.png`). UWAGA: ta scena **miała już** przejścia na wlotach, dlatego nie zaliczono testu wizualnej poprawności pojedynczej nowej zebry na pustym wlocie; wymagana osobna czysta scena oraz kontrola nakładania duplikatów.
- Okno kopii `PID 15600` zakończyło działanie przed kolejną próbą klawiszy Undo/Redo. Nie zgłaszać tego testu GUI ani testu obrotu gizmem jako PASS. Weryfikację jednej operacji Undo/Redo zapewniają testy Qt; materiał z GUI jest tylko dowodem przesunięcia LMB. Zastana scena użytkownika `agent/projects/prawko_urban_curve/scene.xodr` zachowała SHA256 `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e` (certutil). Nie nadpisywano oryginalnego `s2.2b-crosswalks-junction.xodr`.
- **Pełna kompilacja bieżącej wersji:** `prawko-build-win.ps1 -Target all -Jobs 3`: **BUILD_PASS: all**, `AGENT_BINDING_SYNC_PASS`. Pierwsza próba po małej poprawce trafiła na przejściowy konflikt `ninja -t restat` (Permission denied przy równoległym procesie); po ustaniu konfliktu powtórzono build i otrzymano PASS. `archive/visual-regression/s2.2n-qt-full.log`: **1301/1302 Qt PASS, 1 SKIPPED, 0 FAIL** (160 suites). Testy SelectTool w pełnym przebiegu, w tym nowy test no-op poza drogą, PASS. `-Verify` jądra + agenta odczytać z bieżącej sesji lub wykonać ponownie; Qt odpalono osobno z `QT_QPA_PLATFORM=offscreen`.
- **Ograniczenie modelu:** bezpośredni ruch znaków to `s/t` względem drogi w płaszczyźnie, a nie dowolny ruch 3D poza drogą. Istniejący gizmo odpowiada za obrót, lecz osobny odbiór obrotu myszą nadal otwarty. Format sceny OpenDRIVE, triangulacja i materiały niezmienione; repo agenta bez zmian.

**Historyczny następny krok po podetapie 1 S2.2n (zrealizowany w podetapie 2 powyżej):** utworzyć izolowaną testową scenę T/X **bez gotowych przejść** na wybranym wlocie i sprawdzić w rzeczywistym Qt poprawny/odrzucony drag zebry, stop-line, Undo/Redo i ponowne otwarcie zapisanego wyniku; rozważyć blokadę duplikatu na tym samym wlocie. Następnie analogicznie sprawdzić `Stencil`, `Marking`, złożony `PropAssembly` oraz obrót znaku i sygnalizatora gizmem; dla braków dopiero projektować zmiany kodu z testami. NIE oznaczać całej edycji złożonych prefabrykatów ani pełnego S2.2n jako zakończone.

## S2.2m — edycja komponentów bez szukania ich po upuszczeniu (13.11.2026)

**Potwierdzona implementacja:**
- W `document/library_drop.hpp/.cpp` `capture_library_entities` zapisuje
  aktywne identyfikatory obiektów, znaków i skrzyżowań **przed** operacją,
  a `created_library_entities` porównuje je z faktycznym stanem **po**
  `Document::push_command`. Nie dobiera obiektu po największym numerze,
  kolejności sceny czy najbliższym punkcie — poprawne także przy istniejących
  obiektach i przetworzonych slotach aren.
- `MainWindow::on_library_drop` po udanym dodaniu **drzew, pojedynczych
  obiektów, złożonych prefabrykatów, znaków statycznych, sygnalizacji,
  skrzyżowań, strzałek i przejść** zaznacza faktycznie dodane elementy,
  przełącza Select oraz otwiera Properties. **Materiały i oznakowanie**
  nie tworzą nowego obiektu: resolver wskazuje dokładny `RoadId + LaneId`
  i po nałożeniu podświetla zmieniony pas.
- Przejście nadal tworzy swój obiekt i powiązanie ze stop-line w jednym
  makrze Undo. Oznaczenia i materiały nadal idą przez zwykłe komendy.
  Nie zmieniono CDT, struktury sceny ani OpenDRIVE.
- `LibraryPanel` wyświetla kontekstowe instrukcje dla `Signal`,
  `Tree`, `PropAssembly`, `Crosswalk`, `Stencil`, `Material`
  i `Marking`. Pojedynczy wybór katalogowy nadal tylko wybiera
  zasób; przeciągnięcie jest osobną operacją.
- Testy `LibraryDrop.FreshlyPlacedObjectsAndSignalsAreFoundByIdentityForProperties`
  oraz `LibraryDrop.NewlyInsertedJunctionIsDistinguishedFromExistingAssets`;
  dodatkowe asercje dokładnego pasa dla `Material` i `Marking`
  oraz obiektu przejścia dla `Crosswalk`. **55/55 testów resolvera PASS**.
- **Pełny build:** `archive/visual-regression/s2.2m-edit-after-drop-final-build2.log`
  **BUILD_PASS: all, AGENT_BINDING_SYNC_PASS**. Przejściowy LNK1168
  w `s2.2m-edit-after-drop-final-build.log` wynikał z uruchomienia
  procesu testowego równocześnie z linkerem, nie z błędu kodu;
  build powtórzony sekwencyjnie i zaliczony.
  `s2.2m-qt-full-final.log`: **1295 PASS / 1 SKIPPED, 0 FAIL**
  (1296 testów, 160 suites). `s2.2m-quality-gate.log`:
  **QUALITY_GATE_PASS**.
- **Rzeczywisty widoczny Qt + zdarzenia myszy Windows**, wyłącznie na kopii
  `archive/visual-regression/s2.2m-mouse/all-assets-test.xodr`:
  odrzucony upust asfaltu poza właściwym pasem (`material-hover.png`),
  udany upust na pas z automatycznym Properties
  (`material-valid-after.png`), drzewo `Pine tree`
  z panelem `s/t, Heading, Z offset, Height` (`pine-after.png`),
  znak `Stop ahead` z edycją oznaczenia/kąta/dimensions
  (`stop-sign-after.png`), **sygnalizator dynamiczny**
  (własny nowy `signal 2`, `traffic-light-after.png`).
  Po sygnalizatorze wykonano Ctrl+Z, Ctrl+Shift+Z, Ctrl+S, zamknięto
  wyłącznie proces testowy PID 20212. Screenshoty w tym samym folderze.
  Również `traffic-light-undo.png` i `traffic-light-redo.png`.
  `agent/projects/prawko_urban_curve/scene.xodr` pozostał bez zmian:
  SHA256 `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e`.
  Kopia sceny została zapisana osobno.

**Granice odbioru:** ręczny upust przejścia na **wlocie skrzyżowania**
nie został sprawdzony myszą w tej sesji — jedyna testowa scena miała
jedną drogę bez skrzyżowania; test C++ waliduje resolver, komendę
stop-line i identyfikację obiektu. **Nie oznaczać tego testu GUI PASS.**
Nie ma jeszcze pełnego interaktywnego przesuwania znaku lub
sygnalizatora po scenie (Properties pozwala edytować `s/t`);
samodzielne obiekty bez drogi nadal ogranicza model OpenDRIVE.
Również swobodne transformowanie całych złożonych prefabrykatów,
nowe obniżone krawężniki, wysepki i fotorealizm nadal są **otwarte**.

**Pierwszy następny krok S2.2n:** utworzyć kopię testowej sceny
T/X z wlotami; rzeczywistą myszą sprawdzić prawidłowy/odrzucony
upust przejścia + stop-line, a także stencil/marking i
złożony prefabrykat. Zbadać wygodę przesuwania znaków i sygnalizacji
myszą (z poprawnym orientowaniem, snapowaniem i Undo), dopiero potem
wdrożyć brakujące gesty. Każda zmiana z osobnymi testami CI,
odbiorem w Qt i wpisem do HANDOFF. Nie przypisywać modelowi
geometrii/fotorealizmu nieudokumentowanego w kodzie.

## S2.2l — intuicyjne drag & drop i edycja z biblioteki (13.11.2026)

**Zrealizowane i zweryfikowane, bez zmiany formatu sceny ani siatki:**

- `LibraryPanel` pokazuje krótką instrukcję zależną od wybranego
  rodzaju zasobu (rysowanie nowej drogi; stosowanie stylu do istniejącej;
  materiał/oznaczenia na pasie; skrzyżowania; ogólne obiekty).
  Kafelki nie wymagają ręcznego importu, a filtr kategorii pozostaje czynny.
- `ViewportWidget::draw_drag_ghost` wyświetla przy oznaczonym na scenie
  punkcie upuszczenia czytelny, ograniczony do kadru podpis:
  np. **„Puść, aby zmienić przekrój drogi”**. Kolor ostrzegawczy
  nadal oznacza nieprawidłową lokalizację. Pozycja podglądu i miejsce
  zastosowania nadal korzystają z jednego `resolve_library_drop`.
- Po upuszczeniu **RoadStyle** w `MainWindow::on_library_drop`
  program automatycznie włącza Select/Move, zaznacza zmienioną drogę
  i pokazuje zakładkę Properties do dalszej edycji (tylko po udanym
  `Document::push_command`, bez wpływu na inne przypadki).
  Alternatywnie: **zaznacz jedną drogę i kliknij dwukrotnie kafelek
  RoadStyle**. Polecenie trafia do zwykłego Undo/Redo; przy braku
  jednoznacznego wyboru pokazuje ostrzeżenie, nie zmienia sieci.
  Podwójny klik RoadTemplate uzbraja Create Road z jego profilem.
  Zwykły pojedynczy klik służy tylko do wyboru zasobu.
- **Weryfikacja fizyczną myszą Windows** na **osobnych kopiach sceny**:
  `archive/visual-regression/s2.2l-mouse/`
  (`mouse-test-scene.xodr`, `post-change-scene.xodr`).
  Kursor systemowy przeszedł od kafelka do jezdni; na ekranie zaobserwowano
  podświetlenie drogi, znacznik, nowy podpis **przed** zwolnieniem
  (`updated-real-style-hover.png`), toast po modyfikacji
  (`updated-real-style-released.png`), automatyczne zaznaczenie
  zmienionej drogi i rzeczywisty dwuklik
  (`updated-real-style-double-click-result.png`). Następnie wykonano
  Undo, Redo, zapis **wyłącznie** `post-change-scene.xodr`
  i zamknięto tylko testowy proces PID 7936. Ślady:
  `updated-real-undo.png`, `updated-real-redo.png`,
  `updated-real-copy-saved.png`. Nie mylić z testem myszy
  fizycznego urządzenia USB: są to **systemowe zdarzenia kursora
  i przycisku Windows**, wysyłane do rzeczywistego widocznego Qt,
  nie bezpośrednie wywołania funkcji modelu.
- **Testy:** `archive/visual-regression/s2.2l-dnd-full-build.log`:
  **BUILD_PASS: all**, `AGENT_BINDING_SYNC_PASS`;
  `s2.2l-dnd-quality-gate.log`: **QUALITY_GATE_PASS**;
  `s2.2l-dnd-full-qt.log`: **1293 PASS, 1 SKIPPED, 0 FAIL**
  (1294 uruchomione, 160 zestawów). Nowy
  `LibraryPanel.RoadStyleHasActionableHelpAndRealDoubleClick` PASS
  sprawdza teksty instrukcji i dostarczenie sygnału dwukliku
  z `QTest::mouseDClick`.
- **Nienaruszona scena użytkownika:**
  `agent/projects/prawko_urban_curve/scene.xodr`,
  SHA256 **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**
  po teście, identyczny ze stanem sprzed testu. Zapisany wynik
  zmian jest w osobnym `archive/visual-regression/s2.2l-mouse/post-change-scene.xodr`.
  Oryginalna biblioteka, OpenDRIVE/XOSC, PBR i krawężniki niezmienione.

**Otwarty zakres:** to usprawnienie sposobu używania **już obsługiwanych**
typów zasobów; nie implementuje pełnego przesuwania, obracania, skalowania
ani osobnej edycji wszystkich prefabrykatów po upuszczeniu. Osobne obniżone
krawężniki, wysepki oraz pełny realizm materiałów nadal pozostają
niezrealizowane. W kolejnym etapie rozszerzyć testy fizycznego przeciągania
o sygnały, drzewa, przejścia, materiały i błędne obszary (z zachowaniem
przyczepiania OpenDRIVE do drogi oraz Undo/Redo); dopiero potem
projektować nowe prefabrykaty. Nie oznaczać tych dalszych rzeczy jako PASS.

## S2.2k — gotowe elementy drogowe dostępne myszą (13.11.2026)

**Wykonane (kod i testy; nie deklaracja końcowego fotorealizmu):**

- 12 zasobów w `assets/library/manifest.json`, wbudowanych przez
  `editor/resources/resources.qrc`, wszystkie z odrębnymi miniaturami
  `assets/library/thumbnails/{road,style}_prawko_*.png`:
  **6 szablonów do narysowania od zera**
  (`Prawko | Gotowe drogi`) oraz **6 nakładanych na istniejącą drogę**
  (`Prawko | Krawężniki i chodniki`).
  Warianty: `urban` (chodniki 2×1,8 m), `wide` (2×2,5 m),
  `promenade` (2×3,0 m), `one_side` (1×2,5 m po lewej),
  `parking` (po obu stronach: pas postojowy 2,4 m + chodnik 2,0 m),
  `no_curb` (bez chodników/krawężników). Typowa jezdnia lokalna
  zachowuje dwa pasy jazdy szerokości 3,0 m.
- Po przeciągnięciu **Road Template** edytor uzbraja narzędzie Create Road
  i pozwala rysować punkty myszą; po przeciągnięciu **Road Style**
  na istniejącą drogę wykonuje `edit::apply_road_style`
  przez `Document::push_command`, z `Ctrl+Z` Undo.
  Krawężniki nie są przyklejoną atrapą 3D: istniejący generator
  `MeshOptions.raised_sidewalks=true` buduje ścianę 15 cm oraz fazę
  z rzeczywistej granicy pasa Sidewalk. Wariant bez chodników nie ma
  ścian. `curb_render_lod` pozostaje włączony. Środowisko i domyślne
  globalne parametry geometrii/format XODR pozostają niezmienione.
- Normalne uruchomienie `main.cpp` wywołuje
  `Settings::enable_studio_materials_once()` (jednorazowa migracja
  z Sober do Textured, zapamiętany wcześniejszy wybór, bez zmiany
  plików scen i bez wpisów w read-only screenshotach), a po pokazaniu
  `MainWindow` wywołuje `open_studio_library()`.
  `LibraryPanel::focus_category` teraz **naprawdę ustawia filtr
  kategorii**, nie tylko przewija listę.
  `C:\Users\tomas\Desktop\PrawkoRoadStudio.lnk` już wskazuje na
  `F:\PrawkoRoadStudio\build\dev-windows\editor\roadmaker-editor.exe`
  i dotychczasowy projekt `agent/projects/prawko_urban_curve/scene.xodr`.
  Nie wymaga od użytkownika kopiowania assetów, instalacji ani
  ręcznego zapisywania.
- **Testy**: `s2.2k-library-build.log`: `BUILD_PASS: all`,
  `AGENT_BINDING_SYNC_PASS`; `s2.2k-quality-gate.log`:
  `QUALITY_GATE_PASS` (C++/agent); `s2.2k-qt-full-final.log`:
  **1292 PASS, 1 SKIPPED, 0 FAIL**. Nowe testy 6 profilów
  obejmują prawdziwe indeksy siatki krawężników, walidację XODR,
  style do zastosowania na drogach, Undo, 12 miniaturek qresource;
  testy migracji ustawień i automatycznej kategorii.
  Pierwszy pełny Qt ujawnił sztywne liczniki 59 elementów i przestarzałe
  zachowanie `focus_category`; zaktualizowano testy oczekujące
  teraz 71 elementów i prawdziwy filtr, a następnie wszystkie PASS.
  `scripts/check_asset_licenses.py`: **PASS** po dodaniu 12 wpisów
  dla miniatur original-work Apache-2.0.
  `archive/visual-regression/s2.2k-library-gui.png` potwierdza działanie
  GUI i teksturowaną scenę; to screenshot trybu `--screenshot-ui`
  z `--raise-dock`, więc z definicji pokazuje All categories;
  osobny test `MainWindowLayoutTest.StudioLibraryOpensTheNewRoadTemplatesWithoutUserSetup`
  potwierdza wybór nowej kategorii przy rzeczywistym normalnym starcie.
- **Granice zrealizowanego zakresu:** wysokość krawężnika jest
  standardem generatora 0,15 m — NIE ma osobnych prefabrykatów
  krawężnika najazdowego/obniżonego niezależnego od przejścia,
  wysepki jako osobnego zasobu ani narzędzia osobnego stawiania
  luźnych segmentów. Poprawki nie stanowią dowodu pełnego
  fotorealizmu. Nie nadpisano ani nie zapisywano scen użytkownika.

**Potwierdzenie normalnego uruchomienia przez skrót — PASS:**
po zakończeniu `SESSION_FINISH_PASS` uruchomiono z pulpitu
`C:\Users\tomas\Desktop\PrawkoRoadStudio.lnk`.
Rzeczywisty proces `roadmaker-editor.exe` PID **28400**, responsive,
z otwartym `agent/projects/prawko_urban_curve/scene.xodr`;
screen **`archive/visual-regression/s2.2k-real-launch-desktop.png`**:
kategoria **`Prawko | Gotowe drogi`** aktywna, wszystkie sześć
przekrojów z miniaturkami widoczne; trawa i droga renderowane
w trybie Textured. Brak gwiazdki niezapisanego dokumentu;
SHA256 sceny **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**
zgodny ze SHA odczytanym podczas weryfikacji sprzed uruchomienia.
**Proces pozostawiony otwarty do używania; nie zamykać go
ani nie nadpisywać niezapisanych ewentualnych późniejszych zmian.**

**Historyczny plan po S2.2k (nieaktualny punkt startu — aktualny krok S2.2n jest na początku HANDOFF):** zaprojektować i wdrożyć
osobne parametryczne prefabrykaty obniżonego krawężnika,
wysepki i strefy przejścia bez nakładania atrap na siatkę
(osobne testy XODR, geometrii i przecięć); następnie wrócić
do nieodebranego światła i realistycznych materiałów przy
dużych scenach. Nie zaczynać nowego projektu.

## Gdzie pracować

- Komputer przez Desktop Commander: `DESKTOP-G2FV0GJ`.
- **Aktywny katalog:** `F:\PrawkoRoadStudio` (C++20, Qt 6).
- **Agent AI — osobne repo Git:** `F:\PrawkoRoadStudio\agent`.
- Repo publiczne edytora: https://github.com/prawkonaraz100/PrawkoRoadStudio
- Repo publiczne agenta: https://github.com/prawkonaraz100/PrawkoRoadStudio-Agent
- Obie gałęzie edytora (`main`, `feature/realistic-road-surfaces`) wskazywały ten sam kod przed utworzeniem tego przekazania.
- Kopia oryginału `reference/RoadMaker/` jest **tylko referencyjna** — nie edytuj, nie wysyłaj zmian do upstream.
- `toolchain/`, `.qt/`, `build/`, `agent/projects/` i `archive/` pozostają na dysku; część jest ignorowana przez Git i nie występuje w publicznych repozytoriach.

## Historyczny punkt kontrolny po przeniesieniu projektu

1. Przeniesiono projekt do jednego katalogu, poprawiono ścieżki skryptów i skrótu na pulpicie.
2. Skompilowano edytor i moduł Python: `BUILD_PASS: editor`.
3. Quality Gate: **2048/2048 aktywnych testów C++ PASS**, **6/6 testów agenta PASS**. W C++ jest 10 testów wyłączonych, których nie zaliczamy do wykonanych.
4. Przykładowa scena `agent/projects/prawko_urban_curve/scene.xodr`: walidacja bez błędów. To nie zastępuje oględzin wszystkich scen.
5. Utworzono dwa repozytoria na GitHub, **publiczne**, zsynchronizowano historię commitów; bez modyfikacji publicznego `Robomous/RoadMaker`.
6. Wdrożono `scripts/prawko-git.ps1` (status/backup/finish); archiwa Git są sprawdzane przez odtworzenie w pustym repo. Ostatnia potwierdzona kopia sprzed tego dokumentu: `archive/git-backups/*-20261112-223011.*` oraz `scenes-20261112-223011/`. Po każdej sesji odczytaj nowszy manifest.
7. Stan kodu geometrii z etapu S1: wyniesione chodniki 0,15 m i geometria krawężnika z fazą; **nie oznacza to gotowego realistycznego wyglądu ani ukończonych narożników**.

Punkt kontrolny sprzed tego dokumentu:
- edytor: `73b547e4adb9be04d0983904a917ccb33247d3cc` (`main` i `feature/realistic-road-surfaces`);
- agent: `147a08d537530a4b07b0bd7840f6197dc9f62512` (`main`).

## Historyczny plan rozwoju geometrii i wyglądu

**Uzgodniony standard wizualny (12.11.2026):** szczegółowe wymagania
bryły, materiałów PBR, skali tekstur, detali proceduralnych, oświetlenia
i wizualnej regresji zapisano w `docs/PRAWKO-ROAD-STUDIO.md`, sekcja
`Standard jakości wizualnej`. To wymaganie obowiązuje wszystkich kolejnych
agentów. Samo poprawne działanie geometrii **nie wystarcza** do zatwierdzenia
wyglądu; wymagane są oględziny uruchomionego programu z perspektywy
kierowcy i na zbliżeniach. Żaden etap PBR/oświetlenia nie jest jeszcze
potwierdzony jako ukończony. Najpierw należy wykonać S2 (ciągła bryła
narożników i krawężników), później materiały, detale i światło.

**Etap S2.1 — częściowo wdrożony i sprawdzony (12.11.2026).**
W trybie `raised_sidewalks=true` silnik podnosi o 0,15 m chodniki
na skrzyżowaniu i generuje rzeczywiste pionowe ściany krawężników
tylko na wspólnej granicy chodnika z jezdnią. Wykorzystuje triangulację
i obrysy istniejącego generatora. Podniesione chodniki dróg dochodzą
do skrzyżowania bez dawnego 4-metrowego wygaszenia; ich 5-centymetrowa
faza krawężnika zwęża się do zera na ostatnim metrze dla zgodności
z pionową ścianką narożnika. Tryb legacy `raised_sidewalks=false`
pozostaje bez zmian.

**Weryfikacja S2.1:** `BUILD_PASS: editor` (po korekcie konfliktu
linkowania z uruchomionym testowym EXE), `BUILD_PASS: tests`,
2056 testów C++ PASS, 10 wyłączonych, 6 testów agenta PASS,
`QUALITY_GATE_PASS`; 8 nowych testów sprawdziło T/X/Y, ciasne
i szerokie łuki narożników, warianty chodnika i dopasowanie obu
krawędzi na wlotach. Uruchomiono osobną kopię `gs1_urban_intersection.xodr`
w GUI, potwierdzono załadowanie bez crashu i wykonano zrzuty
`archive/visual-regression/s2-verified-gui.png` oraz
`s2-first-pass-close.png` (lokalne, ignorowane przez Git).
**Nie potwierdzono jeszcze wizualnie jakości z wysokości kierowcy
ani braku wszystkich artefaktów w zbliżeniu. S2 jako całość NIE jest
zakończone.** Zrzuty pokazują nadal uproszczone materiały, drzewa
i mało czytelny detal krawężnika.

**Architektura po S2.1 — sprawdzony kod:**
- `mesh_builder.cpp`: pasy chodników na odcinku korzystają ze wspólnej
  wysokości `defaults::kCurbHeight`. Przy zidentyfikowanym połączeniu
  z wyniesionym narożem nie wygasza się już wysokości przed wlotem;
  faza 5 cm przechodzi płynnie w pionową ściankę na ostatnim metrze.
- `junction_surface.cpp` / `junction_surface.hpp`: naroża
  wykorzystują istniejący podział triangulacji. W trybie 3D ich
  wierzchołki otrzymują +0,15 m, a wspólne krawędzie z jezdnią
  tworzą pionowe ściany z oddzielnymi normalnymi i UV.
- `junction_sidewalk_bands.cpp` pozostaje źródłem geometrii obrysów
  (bez kopiowania generatora i modyfikacji OpenDRIVE).
- `test_junction_sidewalk_bands.cpp` ma 8 nowych testów parametrycznych
  S2.1 z kontrolą wysokości, normalnych, ścian i łączenia przy wlotach.
- W tej sesji otwarto jedynie odrębną kopię testową
  `archive/visual-regression/s2-urban-intersection.xodr`, którą
  zamknięto po zrzucie. Wcześniejszy stan `scene.xodr*` użytkownika
  nie został w tej sesji nadpisany ani zamknięty przez agenta.
  Przed kolejnymi działaniami trzeba **na nowo** sprawdzić otwarte procesy
  i niezapisane sceny.

**S2.2a — obniżenia na odcinkach drogi (12.11.2026):**
Dodano w `core/src/mesh/mesh_builder.cpp` generowanie obniżeń
na podstawie istniejących `ObjectType::Crosswalk` (wymiary,
pozycja `s/t` i obrót obiektu). W trybie `raised_sidewalks=true`
chodnik opada z +0,15 m do poziomu jezdni na całej szerokości
poprawnie zlokalizowanego przejścia i wznosi się przez dwie
3-metrowe rampy z łagodnym profilem `smoothstep`.
Normalne chodnika uwzględniają pochylenie. Generator nie tworzy
zerowych trójkątów ściany/fazy w miejscu obniżenia.

**Warunki bezpieczeństwa:** przejście musi fizycznie sięgać
krawędzi jezdni po danej stronie i być obrócone w poprzek drogi.
Wyloty z wyniesionym narożem, gdzie brakuje co najmniej 3 m
na pełną rampę i odzyskanie 15 cm wysokości przed stykiem,
są świadomie pomijane — **nie mają jeszcze obniżenia w S2.2a**.
Stan kompatybilny `raised_sidewalks=false` nie zmienia się.
Przy edycji/usuwaniu obiektu `remesh_objects` przebudowuje
siatkę drogi w trybie 3D, aby nie zostawiać starego zagłębienia.

Dodano dwa testy do `core/tests/test_mesh.cpp`:
obniżenie obu stron, ciągłość wysokości, normalne,
brak degeneracji ścian, zgodność pełnego i przyrostowego
remesh oraz powrót do stanu pierwotnego po usunięciu
przejścia; brak obniżenia przy błędnym zasięgu/obrocie.
Testy skoncentrowane: 2/2 PASS. Scena do kontroli GUI:
`archive/visual-regression/s2-midblock-curb-drop.xodr`
(z lokalnego wzorca, tylko do testów; walidator: 0 błędów).
**Potwierdzono kompilację i testy:** `BUILD_PASS: tests`,
`BUILD_PASS: editor`, pełny `QUALITY_GATE_PASS`,
**2058 testów C++ PASS**, 10 wyłączonych, **6/6 testów agenta PASS**.
Nowa scena `s2-midblock-curb-drop.xodr` przeszła walidację
bez błędów, została otwarta w nowym edytorze bez awarii,
a lokalne zrzuty `s2.2a-midblock-editor.png` i
`s2.2a-midblock-close.png` są w `archive/visual-regression/`.
Osobne okno testowe zamknięto. Z kamery ogólnej i na niskim
ujęciu profil krawężnika jest nadal bardzo słabo czytelny,
a materiały pozostają uproszczone. **Nie zaliczono jeszcze
wizualnego odbioru w zbliżeniu ani glTF.**

**S2.2b — zjazdy połączone z narożnikami (13.11.2026):**
`core/src/mesh/mesh_builder.cpp` utrzymuje poziom 0 cm na granicy
wlotu, jeżeli prostokątne przejście jest za blisko skrzyżowania
na pełne przywrócenie wysokości 15 cm. Resztę 3-metrowego
wzniesienia przejmuje siatka `core/src/mesh/junction_surface.cpp`
według gładkiego profilu liczonego od dokładnej krawędzi
ramienia. Ta sama funkcja wysokości kształtuje chodnik
i pionową ścianę; zerowe trójkąty są pomijane, normalne
nachylonego chodnika są aktualizowane. Wciąż korzystamy
z istniejących poligonów/trójkątów; pliki OpenDRIVE nie są
przekształcane, legacy `raised_sidewalks=false` pozostaje
bez zmian.

`remesh_objects` odtwarza sąsiednie narożniki przy
dodawaniu/usuwaniu przejścia. Naprawiono ważną niezgodność
w `editor/src/document/document.cpp`: **remesh_junctions
i remesh_objects** używają teraz `studio_mesh_options()`,
tak samo jak `Document::load`; wcześniej edycja mogła
przywrócić płaską geometrię. Dodano test dla czterowlotowego
skrzyżowania: porównanie wysokości na wspólnych wierzchołkach,
pełnego i przyrostowego meshu, usunięcia przejść
— **1/1 PASS**. Pełna kontrola silnika: **2059/2059
aktywnych C++ PASS**, 10 wyłączonych; testy agenta **6/6 PASS**
(`QUALITY_GATE_PASS`). Scena testowa z 4 przejściami:
`archive/visual-regression/s2.2b-crosswalks-junction.xodr`
— walidator zgłasza **0 błędów**.

**Weryfikacja GUI:** potwierdzony `BUILD_PASS: editor`
oraz ponownie zlinkowany `roadmaker-editor.exe` po poprawce
`Document`. Osobna kopia sceny X została otwarta w edytorze
bez awarii; obraz aplikacji pobrano bez przejmowania okna użytkownika
do `archive/visual-regression/s2.2b-gui-window-print.png`.
Widok ogólny działa, ale nie zastępuje odbioru z kamery kierowcy.
Osobne okno testowe zamknięte bez zapisania zmian.

**Odblokowano testy Qt Windows (13.11.2026).** Zrzut błędu
`RoadMakerEditorTests` dostarczony przez użytkownika ujawnił
komunikat: `No Qt platform plugin could be initialized`;
dostępna była wyłącznie platforma `windows`, podczas gdy
`qt_gtest_main.cpp` wymaga `offscreen`. W lokalnym Qt
`F:\PrawkoRoadStudio\.qt\6.8.3\msvc2022_64\plugins\platforms\qoffscreen.dll`
istniał, ale `windeployqt` kopiuje obok edytora tylko
`qwindows.dll`. To wyjaśniało zawieszenie w modalnym oknie
błędu podczas `gtest_discover_tests`.

Naprawa w `editor/tests/CMakeLists.txt`: na Windows po
zlinkowaniu `roadmaker_editor_tests` CMake kopiuje
`$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>` do
`$<TARGET_FILE_DIR:roadmaker_editor_tests>/platforms`
**przed automatycznym wykrywaniem GTest**. Wymagany
plugin jest sprawdzany jako target Qt (brak -> błąd konfiguracji).
Zweryfikowano bez specjalnych `QT_PLUGIN_PATH`
ani `QT_QPA_PLATFORM_PLUGIN_PATH`: testy
`DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity`
oraz `MeshParity.*` — **5/5 PASS**. Ręcznie skopiowany
plugin kontrolny usunięto przed przebudową; automatyczna
kompilacja `roadmaker_editor_tests` zakończyła się
`EDITOR_TEST_DISCOVERY_BUILD_PASS` i sama odtworzyła
`qoffscreen.dll`. Pełna binarka edytora:
**1285 przypadków, 1284 PASS, 1 SKIPPED, 0 FAIL**, exit 0.
Log lokalny:
`archive/visual-regression/s2.2b-qt-full-tests.log`.
Dawnych prób diagnostycznych nie utrzymywano w kodzie.
Osobny Quality Gate jądra: 2059 testów C++ i 6 agenta PASS.
**Nie mylić testów Qt z odbiorem estetyki renderingu.**

**S2.2c — etap kodu i testów (13.11.2026):** odzyskano istniejące lokalne zmiany bez ich nadpisania i dokończono kontrolę automatyczną. `junction_surface.cpp` wytwarza odsuniętą krawędź górnego chodnika i oddzielną rzeczywistą fazę krawężnika szerokości nominalnej 5 cm, zmniejszaną na końcach wlotów i przy zjazdach. Dla ciasnych trójkątów CDT przesunięcie jest ograniczane testem orientacji, aby nie odwrócić siatki. Górna krawędź fazy korzysta z pozycji siatki chodnika, dolna z krawędzi ściany. Dodane testy T/Y, nieodwracania trójkątów, wspólnych krawędzi i eksportu GLB. `gltf_exporter.cpp` zapisuje `TEXCOORD_0` dla dowolnej podsiatki z kompletnymi UV — poprzednio exporter je pomijał, mimo że miały je fazy.

**Potwierdzone 13.11:** `BUILD_PASS: tests`, `BUILD_PASS: editor`; C++ **2068 PASS, 10 DISABLED**, Python agent **6 PASS**, `QUALITY_GATE_PASS`; Qt edytora **1284 PASS, 1 SKIPPED, 0 FAIL** (1285 przypadków, exit 0). W czasie pracy żadne okno `roadmaker-editor` nie było otwarte, nie zmieniano zapisanych scen użytkownika. **BRAK odbioru wizualnego S2.2c z kamery kierowcy i zbliżeń T/Y, brak potwierdzonego oglądu GLB w zewnętrznym viewerze i profilowania czasu remesh** — nie deklarować pełnego ukończenia etapu wizualnego. Najbliższa praca: osobna kopia sceny testowej, inspekcja krawężników w GUI pod małym kątem (ciasne T/Y, przejścia, nachylenia), sprawdzenie glTF w viewerze i kosztu remesh_objects; dopiero po tym ocena geometrii jako gotowej do PBR.

**S2.2c — niezależny ogląd GLB i naprawa eksportu agenta (13.11.2026):**
Wykryto rozjazd skompilowanego `build/dev-windows/python/_roadmaker*.pyd`
i ignorowanego przez Git `agent/vendor/roadmaker/_roadmaker*.pyd` (inne SHA-256).
Stary agent eksportował płaskie narożniki bez faz: GLB 127232 B, 41 meshy;
po synchronizacji z aktualnym modułem ten sam XODR eksportuje GLB
157824 B, 49 meshy, 4 ściany i 4 fazy narożników z UV oraz 0–0,15 m
wysokości. To był błąd ścieżki CLI, nie generatora C++.
`scripts/prawko-build-win.ps1` dla targetów editor/python/all teraz
kopiuje moduł do agenta i kontroluje SHA-256; `prawko-verify-win.ps1`
blokuje PASS przy stale binding; `agent/roadctl.py` wymaga
`raised_sidewalks` i nie może już po cichu użyć starej geometrii.
Agent ma nowy test end-to-end: prawdziwy GLB, 4 fazy, atrybuty
POSITION/NORMAL/TEXCOORD_0, dostępny VEC2 accessor. Repo agenta:
`02cbdeb5e943f7a710d3292a9fbc5e85d3cc95fc` na `origin/main`.

**Rzeczywista ocena 3D:** Blender 5.3 bezbłędnie zaimportował GLB
skrzyżowania X, nowo utworzonego miejskiego T i niestandardowego Y
(z zapisanych XODR z `agent/projects/visual_s2_2c_{t,y}`).
X: 49 meshy, 4 fazy; T: 14 meshy, 3 fazy; Y: 13 meshy,
3 fazy. Na diagnostycznych renderach z góry, z poziomu kierowcy
i na wyizolowanym zbliżeniu widać ciągłą wzniesioną bryłę,
oddzielną ścianę oraz fizyczne połączenie fazy z chodnikiem.
Kolor pomarańczowy faz na renderach jest **wyłącznie diagnostyczny**,
nie jest nowym materiałem produkcyjnym.
Rzeczywista aplikacja Qt otworzyła oddzielną scenę testową X bez awarii:
`archive/visual-regression/s2.2c-current-editor-gui.png`; proces
zamknięto bez zapisywania zmian. Lokalne dowody w
`archive/visual-regression/`: `s2.2c-independent-review-current.glb`,
`s2.2c-t-urban-current.glb`, `s2.2c-y-urban-current.glb`,
`s2.2c-glb-geometry-top.png`, `s2.2c-glb-corner-isolated.png`,
`s2.2c-glb-corner-driver.png`, `s2.2c-y-glb-oblique.png`.
Pliki diagnostyczne są ignorowane przez Git i **nie są** zdalnym backupem.

**Granice odbioru:** zakładany profil 15 cm / 5 cm jest widoczny,
a w pokazanych ujęciach nie widać przerw w fazie. Łuk pozostaje
widocznie segmentowy; nie ma jeszcze PBR, realnych tekstur ani
kompletnego odbioru estetyki z kamery kierowcy w samym edytorze.
Nie wykonano pomiaru czasu `remesh_objects`. **Odbiór realizmu
materiałowego i finalny odbiór wizualny produktu NIE SĄ PASS.**

**Testy końcowe:** C++ 2068 PASS (10 DISABLED), Qt 1284 PASS,
1 SKIPPED / 0 FAIL; agent 7 PASS wraz z nową regresją GLB.
`AGENT_BINDING_PARITY_PASS`, `BUILD_PASS: python`, kompilacja
edytora i pełny Quality Gate (aktualny log lokalny w
`archive/visual-regression/s2.2c-final-quality-gate.log`).

**S2.2d — korekta wizualna normalnych i betonu (13.11.2026):**
Analiza scen X/T/Y w Blenderze potwierdziła, że na fazie krawężnika
każdy trójkąt miał własną płaską normalną. W `junction_surface.cpp`
dodano deterministyczne, ważone polem uśrednianie normalnych we
wspólnych punktach **wyłącznie tej samej fazy**. Jej górna granica
z chodnikiem i dolna granica z pionową ścianą pozostają ostre, a
współrzędne wierzchołków, trójkąty i format OpenDRIVE są niezmienione.
Na podstawie surowych danych GLB X: 136 niezgodności normalnych >0,05°
na zduplikowanych wierzchołkach -> 0; maks. rozbieżność 6,7358°
-> 0,0165° (float32), 72 trójkąty faz przed/po. T: 210 wspólnych
wierzchołków fazy i 0 niezgodności; Y: 138 i 0 niezgodności.

Żółtawo-zielony odcień betonu krawężnika (`0.55/0.55/0.50`)
zastąpiono neutralnym `0.61/0.61/0.59` w GUI oraz w eksporcie.
Dla typów `Curb/Sidewalk/Border` ustawiono wspólną chropowatość
`0.78` (pozostałe pasy nadal `0.95`) w glTF, USD i podglądzie
manifestu; test kontroluje zgodność. **To nie są materiały PBR
z teksturami!** GLB nadal ma dla pasa krawężnika wyłącznie
`baseColorFactor`, `roughnessFactor` i UV; brak obrazów albedo,
normal i roughness w jego materiale. Istniejące katalogowe tekstury
edytora nie są jeszcze osadzane w GLB.

Próba globalnego zagęszczenia łuków (sagitta 8->4 mm,
`kBoundarySimplify` 5->1 mm, próg tangencji 8->4 cm)
**została odrzucona i w całości wycofana**: pełne testy
wykryły 6 i 4 wąskie trójkąty przy dopuszczalnych 2
na wlotach T oraz niezgodność z goldenem prostych skrzyżowań.
Przywrócono dotychczasową geometrię i wszystkie te testy znów PASS.
Nie podnosić gęstości globalnej ani nie aktualizować złotego wzorca
bez odrębnie zweryfikowanego rozwiązania. Segmentowana **sylwetka**
łuku nie jest usunięta (zmiana normalnych koryguje oświetlenie,
nie obrys).

**Niezależny GLB/Blender 5.3:** aktualne `s2.2d-x.glb`,
`s2.2d-t.glb`, `s2.2d-y.glb` wyeksportowane przez agenta,
zaimportowane do Blender 5.3; X: 4 fazy i 4 ściany,
T/Y: po 3 fazy i 3 ściany, UV dostępne, wszystkie trójkąty
niezerowe, wysokie części 0,15 m, roughness 0,78. Render X
przed/po z tym samym ustawieniem kamery oraz widoki T/Y:
`archive/visual-regression/s2.2d-before-x-*.png`,
`s2.2d-after-x-*.png`, `s2.2d-after-t-*.png`,
`s2.2d-after-y-*.png`. Dodatkowo włączono realne GUI
Windows na osobnej scenie testowej X, zapisano zrzut
`s2.2d-current-editor-gui.png` i zamknięto okno bez zapisu.
Pliki obrazów/GLB w `archive` są lokalne i ignorowane przez Git.
Zgoda użytkownika na udoskonalanie zapisanych scen jest aktualna,
ale w tym etapie nie trzeba było modyfikować żadnego XODR: wygląd
zmienia się globalnie po przebudowie edytora.

**Wyniki:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`,
`AGENT_BINDING_PARITY_PASS`, C++ 2068 PASS / 10 DISABLED,
Qt 1284 PASS / 1 SKIPPED / 0 FAIL, 7 testów agenta PASS;
`QUALITY_GATE_PASS`. Logi: `s2.2d-final-build.log`,
`s2.2d-final-gate.log`, `s2.2d-final-qt.log` w lokalnym
`archive/visual-regression/`. Nie wykonano pomiaru kosztu
`remesh_objects` ani zatwierdzenia realizmu z kamery kierowcy
w samym edytorze. **PBR i finalny realizm — OTWARTE.**

**S2.2e — remesh i rzeczywiste tekstury w GLB (13.11.2026):**

**Wydajność geometrii:** dodano trwały, replikowalny test w
`core/tests/test_prawko_remesh_budget.cpp`. Buduje X (4 ramiona, 4
przejścia), T (3, 3) i Y (3, 3) z `raised_sidewalks=true`,
porównuje `remesh_objects` do pełnej siatki (pozycje, indeksy pasów,
narożników) i mierzy 41 iteracji. Pierwszy pomiar na tym PC:
X p50=9,026 ms, p95=9,598 ms;
T p50=4,941 ms, p95=5,417 ms;
Y p50=5,410 ms, p95=5,959 ms. W końcowym Quality Gate po PBR:
X 9,180/9,976 ms, T 5,329/5,622 ms, Y 5,899/6,159 ms.
Pomiar ma typową zmienność zależną od obciążenia komputera.
Wszystkie trzy testy PASS. **To tylko wywołanie C++** bez uploadu GPU
i prezentacji klatki. Nie nazywać tego FPS ani SLA 16 ms GUI.
Pierwszy osobny commit: `f33cc89`.

**GLB/PBR:** `core/src/io/gltf_exporter.cpp` zapisuje teraz
osadzone (w BIN) obrazy CC0 Poly Haven JPEG albedo, PNG normal OpenGL
oraz JPEG roughness dla `Driving` (asphalt) i
`Sidewalk/Border/Curb` (brushed concrete); źródła już były w
`assets/textures/`, z wpisami w `ASSETS_LICENSES.md`. Deduplicacja
trzech obrazów na rodzinę materiału, REPEAT sampler, metrystyczne UV
`0.25 powtórzenia/metr` = płytka 4 m. Dla pasów drogowych używa
`road.uvs`, dla submeshy zachowuje UV, a dla CDT bez UV stosuje
planarną projekcję XY. Normal i roughness texture są podpięte
do materiału, `metallicFactor=0`. JPEG roughness ma identyczne
kanały RGB (1024 próbki na każdą z obu map bez różnic), więc jego G
jest prawidłowy. `export_preview.cpp` raportuje liczbę obrazów
i textured flags zgodne z faktycznym GLB; testy `ExportPreview` i
`GltfExport` obejmują osadzone mapy, UV, współistnienie z sygnalizacją
i strukturalny reload mimo braku dekodera obrazów w binarce testowej.
**Instalowany edytor poza tym drzewem:** źródłowa ścieżka zasobów jest
zdefiniowana przez CMake; gdy plików brakuje, GLB może użyć bazowych
kolorów. Dostarczanie tekstur z pakietem dystrybucyjnym pozostaje
odrębnym zadaniem; wyeksportowany GLB jest samowystarczalny.

**Blender 5.3 / X/T/Y:** `archive/visual-regression/s2.2e-pbr-x.glb`
(1 380 340 B), `s2.2e-pbr-t.glb` (1 314 116 B),
`s2.2e-pbr-y.glb` (1 289 912 B); wszystkie trzy ponownie
zaimportowane i faktycznie zdekodowane: X 49 meshy, po 4 bevel/wall,
T 14 meshy, Y 13 meshy, po 3 bevel/wall, poprawne UV,
niezdegenerowane powierzchnie, 3 image texture nodes w materiałach
`lane_0`, `lane_4`, `lane_9`. Wynik
`PBR_GLTF_IMPORT_PASS` X/T/Y. EEVEE render produkcyjnego GLB:
`archive/visual-regression/s2.2e-pbr-x-eevee.png`. To pliki
**lokalne ignorowane przez Git**; nie traktować jako backupu poza F:.
Na renderze wyraźnie widać powtarzalny wzór spękań asfaltu i
ciemne, monotonne bloki betonu — **realizm NIE odebrany**.
Utrzymujemy bazową kompatybilność OpenDRIVE i siatkę narożników.

**Łuki / analiza i decyzja:** X ma 18 trójkątów na fazę
narożnika (~9 odcinków), więc widoczna jest segmentacja zarysu.
W S2.2d zagęszczanie globalne naruszało dozwolony budżet
`sliver <=2` na ciasnych T (osiągało 4–6) i złoty wzorzec
prostopadłych wlotów. **W S2.2e nie zmieniono obrysów** ani
współrzędnych nawierzchni. Rozwiązanie wymaga odseparowanego
LOD tylko renderowania albo lokalnej retriangulacji naroża
z nowymi testami i nienaruszonym goldenem; w tym etapie
nie ma potwierdzonej bezpiecznej poprawki sylwetki.

**Weryfikacja:** `BUILD_PASS: all`,
`AGENT_BINDING_SYNC_PASS`, pełny Quality Gate
`2071/2071 C++ PASS` (10 testów DISABLED), agent
`7/7 PASS`, edytor Qt `1284 PASS + 1 SKIPPED`,
`QUALITY_GATE_PASS`. Logi:
`archive/visual-regression/s2.2e-final-gate.log`,
`s2.2e-pbr-qt.log`, `s2.2e-pbr-build-all.log`,
`s2.2e-blender-x.log`, `s2.2e-blender-t.log`,
`s2.2e-blender-y.log`.
**Otwarte:** zagęszczenie sylwetki bez psucia CDT,
ograniczenie tilingu/odcienia, realne światło oraz pełna ocena
z kamery kierowcy w GUI (nie tylko Blender). Pomiar i odbiór
GPU upload+FPS również nadal otwarte. **Potwierdzenie aplikacji Qt:**
aktualny `roadmaker-editor.exe` otworzył odrębną scenę X bez awarii,
zrzut: `archive/visual-regression/s2.2e-current-editor-gui.png`.
To był widok ogólny; nadal nie jest odbiorem z wysokości kierowcy.
Okno testowe zamknięte bez zapisu. **GUI nie korzysta jeszcze ze
wszystkich map PBR tak samo jak eksport GLB**, czego dowodzą
różnice w renderze Blendera i neutralnym obrazie GUI.

**S2.2f — lokalny LOD faz krawężników bez naruszenia CDT (13.11.2026):**

- W `core/src/mesh/junction_curb_refinement.hpp` wprowadzono **opcjonalny**
  podział trójkątów na wspólnej zakrzywionej krawędzi jezdnia/chodnik.
  Działa PO pierwotnej triangulacji CDT i klasyfikacji pasów. Tę samą
  nową pozycję wierzchołka dzielą OBA sąsiednie trójkąty, a ściana fazy
  i górna płyta podążają za granicą, więc nie ma T-seam w nowym detalu.
  Korekta Catmull-Rom jest tylko prostopadła do cięciwy, ograniczona
  do 8 mm / 1% cięciwy. Dzielone są tylko odcinki 0,45–5,0 m
  z dwoma dobrze uwarunkowanymi sąsiadami (>=8 stopni, >=22% pola).
  Klasyfikacja, oryginalne wejściowe obrysy, historyczne goldeny
  oraz zapis ASAM OpenDRIVE są bez zmian.
- Nowa opcja `MeshOptions.curb_render_lod` domyślnie = **false**
  w jądrze; działa wyłącznie razem z `raised_sidewalks=true`.
  Chroni zgodność oryginalnych siatek zwykłych i wyniesionych.
  `studio_mesh_options()` Qt włącza ją do rzeczywistego GUI, także
  dla `remesh_junctions` i `remesh_objects`; Python agent/GLB również
  włącza ją jawnie, a regresja agenta porównuje liczbę trójkątów
  GLB względem domyślnej wersji bez LOD.
- Test `JunctionSidewalkBands.VisualCurbArcLODAddsSharedVerticesWithoutSlivers`
  porównuje X/T/Y: trójkąty jezdnia+chodniki 676->828,
  469->555, 522->662; slivery <5 stopni odpowiednio 16->16,
  5->5, 18->18 (liczone na całych tych powierzchniach,
  nie mylić z osobnym budżetem CDT T-junction!). Dodatkowo
  test sprawdza deterministyczne bufory, orientację/niedegenerację,
  exact-match dolnych krawędzi ścian z wierzchołkami jezdni.
- Rzeczywisty agent GLB X/T/Y: triangulacja **faz** X 72->120,
  T 108->174, Y 72->116; ściany również X 72->120,
  T 108->174, Y 72->116. Podział jest zapisany w GLB.
  Blender 5.3 ponownie importował/działające PBR X/T/Y: PASS.
  Wyeksportowane pliki: `archive/visual-regression/s2.2f-opt-{x,t,y}.glb`.
  Logi: `s2.2f-opt-blender-{x,t,y}.log`, render EEVEE
  `s2.2f-opt-x-eevee.png`. Geometria before/after w tych samych
  kamerach: `s2.2f-before-x-{top,driver,isolated}.png`,
  `s2.2f-after-x-{top,driver,isolated}.png`,
  `s2.2f-after-{t,y}-{top,driver}.png`. To lokalne, ignorowane
  przez Git dowody; nie są kopią poza dyskiem F:.
- Test wydajności `PrawkoRemeshBudget` uruchamia teraz **optyczną
  wersję LOD**, a nie tylko stary kształt: podczas focused Qt+core
  X mediana/p95 ~10,37/10,90 ms, T 6,21/7,17 ms,
  Y 7,00/7,70 ms (zmienne wraz z obciążeniem PC).
  Nie są to wyniki uploadu GPU ani FPS.
- `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`,
  `AGENT_BINDING_PARITY_PASS`, pełny `QUALITY_GATE_PASS`:
  C++ **2072/2072 PASS** (10 DISABLED), Python **7/7 PASS**;
  testy Qt Windows **1284 PASS, 1 SKIPPED, 0 FAIL**.
  W Qt `DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity`
  nadal rygorystycznie porównuje bufory pełnego i inkrementalnego
  budowania, ale od S2.2f używa tej samej opcji LOD co GUI.
  Logi: `s2.2f-opt-final-gate.log`, `s2.2f-opt-qt-final.log`,
  `s2.2f-isolated-build-all.log`, `s2.2f-opt-qt-fix-build.log`.
  Realny edytor Windows otworzył testową kopię X (bez awarii),
  screenshot: `s2.2f-current-editor-gui.png`; okno zamknięto
  bez zapisywania scen. Żadna scena użytkownika nie została zmieniona.
- **Ocena estetyczna:** z bliskiej kamery geometria jest lepiej
  podzielona, lecz w statycznych renderach różnica sylwetki jest
  subtelna (przesunięcie geometrii <=8 mm). Nie deklarować
  docelowej płynności, fotorealizmu, odbioru z kamery kierowcy
  w SAMYM edytorze ani GPU/FPS. Trzeba sprawdzić większe skosy,
  z-fighting, powtarzanie tekstur, światło i detale.
- Osobne repo agenta po zmianie: `35c48c11cdb7efca8e08d1be731463657bbe7a32`
  (origin/main). Należy zawsze sprawdzić aktualny HEAD obu repo.

**S2.2g — pomiar karty GPU i pięć widoków X/T/Y (13.11.2026):**

- Dodano **oddzielny, ręcznie uruchamiany** target
  `prawko_gpu_probe` (`editor/tests/prawko_gpu_probe.cpp`;
  `EXCLUDE_FROM_ALL` w `editor/tests/CMakeLists.txt`). Korzysta z tej
  samej implementacji `GLRenderer`, shaderów, materiałów i budowania
  siatki co PrawkoRoadStudio; sprzętowy OpenGL 3.3 przez Qt offscreen
  na **NVIDIA GeForce GTX 1050 Ti (4096 MiB, sterownik 582.78)**.
  Próba jest pomiarem sprzętowym, nie Mesa/llvmpipe.
- Metodologia: ten sam zapisany X/T/Y, `raised_sidewalks=true`,
  LOD wyłączony/włączony; 1280×720, 5 kamer: z góry, ukośnie,
  kamera kierowcy (około **1,55 m**), z przeciwnej strony (1,55 m)
  i zbliżenie (około **0,60 m**). Dla T/Y kamera poziomu drogi
  celuje w centroid rzeczywiście zbudowanego najdłuższego
  krawężnika, zamiast w narożnik X. Dla każdego punktu 15 warm-up,
  65 pomiarów czasu `GL_TIME_ELAPSED` przez natywny GPU timer query.
  Mierzone są osobno CPU submission i oczekiwanie na wykonanie GPU;
  `QPainter`, Qt `QOpenGLWidget`, prezentacja klatki, synchronizacja
  monitora i kompozytor Windows NIE są objęte testem.
- W **finalnej, skorygowanej serii S2.2g-v2** mediana GPU draw:
  X góra **0,383 ms**, ukos **0,423 ms**, kierowca **0,292 ms**;
  T **0,348 / 0,373 / 0,258 ms**;
  Y **0,340 / 0,372 / 0,278 ms**. CPU draw submission
  ok. **0,008–0,025 ms**; CPU do odczytu zakończenia GPU
  ok. **0,4–0,65 ms**. Włączenie LOD zmienia mediany GPU
  o około 0–0,002 ms (zależne od kamery); bez widocznej
  regresji renderingowej w tych scenach.
- Oddzielono koszt wczytania PBR tekstur od siatek. Powtarzane
  synchronizowane `GLRenderer::upload` całej siatki+GPU `glFinish`
  (10 rozgrzanych próbek): mediana X **2,56 ms** (bez LOD 2,74),
  T **1,57 ms** (bez 1,54), Y **1,36 ms** (bez 1,51).
  Obejmuje CPU driver, tworzenie VAO/VBO/EBO i GPU fence, a NIE
  sam transfer DMA. Początkowe dekodowanie+upload sześciu
  obrazów PBR ok. **42–56 ms**, zależne od cache; wartości
  baseline/LOD nieporównywalne na zimno. Rozmiary buforów
  geometrii X 0,281→0,290 MiB, T 0,267→0,278,
  Y 0,230→0,237. Odpowiednio 1620→1764, 1423→1621
  i 1047→1179 trójkątów całego renderowanego obszaru
  (to inne liczniki niż test samej jezdni/chodników S2.2f).
- Obiektywne porównanie 15 par kadrów przy progu max RGB
  >=8/255: zmiana X góra **0,0125%**, kierowca **0,0248%**,
  macro **0,1518%**; T **0,0284 / 0,1341 / 0,0267%**;
  Y **0,0160 / 0,0627 / 0,1214%**. Na mapach różnic
  zmiany ograniczają się do linii łuków. Odbiór estetyczny:
  brak widocznych otwartych dziur w tych kadrach, ale
  **przyrost płynności bardzo subtelny**; miejscami wyraźny
  ciemny/czarny pas ściany krawężnika, nadal segmentowane
  cieniowanie i gwałtowne przejście teksturowanej jezdni
  do jednolitej posadzki junction. Nie ogłaszać
  braku z-fightingu w animacji ani realizmu.
- Źródła pomiarów (lokalne, ignorowane przez Git):
  `archive/visual-regression/s2.2g-v2/{x,t,y}-gpu-benchmark.json`,
  `{x,t,y}-{base,lod}-{top,oblique,driver,opposite,macro}.png`,
  `{x,t,y}-diff-*.png` + `s2.2g-v2-pixel-diff.ps1`.
  Nie używać wcześniejszej próbnej serii `s2.2g/`
  jako finalnej: T/Y kamery nie zawsze pokazywały krawężnik;
  **v2** ma dynamiczne celowanie w rzeczywisty narożnik.
- `BUILD_PASS: all`, `QUALITY_GATE_PASS`:
  **2072/2072 C++ PASS** (10 DISABLED), **7/7 agent PASS**;
  testy Qt **1284 PASS, 1 SKIPPED**. Logi `s2.2g-build-all.log`,
  `s2.2g-final-gate.log`, `s2.2g-qt.log`.
  Testowe okno prawdziwego edytora otwarto ze sceną X
  i bezpiecznie zamknięto bez zapisu, obraz:
  `s2.2g-current-editor-gui.png`. Nie modyfikowano
  scen użytkownika ani geometrii, agent repo bez zmian.

**S2.2h — widoczna aplikacja Qt, spójne UV i oświetlenie betonu (13.11.2026):**

- Dodano opcjonalny `prawko_qt_fps_probe` (`editor/tests/prawko_qt_fps_probe.cpp`, `EXCLUDE_FROM_ALL`). Tworzy prawdziwe widoczne `MainWindow` z teksturami, HUD, rzeczywistym `QOpenGLWidget` i `frameSwapped` (bez zapisu scen, `Settings::set_read_only(true)`). Symuluje `Alt+LMB` orbit przez normalne zdarzenia myszy Qt. 1600x900, GTX 1050 Ti, zegar Qt 16 ms, po 30 rozgrzewkowych i 180 mierzonych odstępów między `frameSwapped`; Windows Qt, nie `QT_QPA_PLATFORM=offscreen`.
- **Powtórka już po przebudowie materiałów** `archive/visual-regression/s2.2h-final-fps-{x,t,y}.json`: spoczynek X/T/Y odpowiednio **60,00 / 59,99 / 60,00 FPS**; obrót **59,97 / 60,01 / 60,00 FPS**. P95 obrotu odpowiednio **17,35 / 17,30 / 17,29 ms**, P99 **17,52 / 17,54 / 17,61 ms**, **0 próbek >33,33 ms** na scenę. To pomiar częstotliwości sygnału `frameSwapped` (Windows+Qt) przy sztucznym odświeżaniu co 16 ms; nie jest niezależnym pomiarem maksymalnej przepustowości GPU ani fizycznego momentu pojawienia się fotonu na ekranie. `viewport->format().samples()` zgłasza **0**, choć aplikacja prosi o 4x MSAA — nadal do weryfikacji. **Jank przy edycji węzłów i przebudowie siatki NIE sprawdzony.**
- **Naprawa ciemnego, jednolitego asfaltu w skrzyżowaniach:** `editor/src/render/scene_builder.cpp` generuje metryczne world-XY UV z dokładnie tych samych wierzchołków co eksporter `gltf_exporter.cpp` dla CDT floors i ich betonowych narożników. Szanuje jawne UV; materializowany asfalt/beton w `SurfaceMesh` ma UV, natomiast trawa pozostaje bez UV. Geometria, topologia, OpenDRIVE i model C++ niezmienione. Dodano testy Qt `BuildScene.JunctionAndCornerPavementUseWorldPlanarTextureCoordinates` oraz `BuildScene.MaterializedGroundSurfaceGetsPlanarUvButGrassDoesNot`.
- **Doświetlenie krawężników:** `Material.vertical_fill` domyślnie **0.0**, `ViewportWidget::material_for` przypisuje **0.38** tylko `SurfaceKind::Concrete` w trybie Textured, a GLSL dodaje rozproszone niebieskoszare odbicie nieba wyłącznie przy niemal pionowej normalnej geometrycznej. Bez globalnego podbijania asfaltu, trawy, malowania czy trybu Sober. Przy pionowych ścianach world-XY UV bywają singularne — ochrona wyznacznika ramki normal mapy w `GLRenderer` zachowuje wtedy normalną geometryczną (bez NaN i smug). Zaktualizowano ręczny `prawko_gpu_probe` do nowego parametru `vertical_fill`.
- **Kontrola wizualna:** 30 nowo wygenerowanych kadrów GLRenderer X/T/Y `archive/visual-regression/s2.2h-material-after/` (5 kamer × base/LOD na scenę) i rzeczywisty screenshot uruchomionego `roadmaker-editor.exe --screenshot --textured`: `s2.2h-gui-x-orbit.png`. Wszystkie X/T/Y pomiary wykonane z RC 0. Widać fakturę asfaltu i spękania przez całą powierzchnię skrzyżowania zamiast jednolitej szarej łaty, oraz bardziej czytelną, nie całkiem czarną ścianę krawężnika. Nadal widoczne **powtarzanie spękań co 4 m**, ostre przejścia niektórych fragmentów betonu i brak szczegółów/światła cieniowania — nie oznaczać pełnego fotorealizmu jako ukończonego. Nie zmieniano żadnej sceny użytkownika.
- **Testy:** `BUILD_PASS: all` + `AGENT_BINDING_SYNC_PASS`; `QUALITY_GATE_PASS` — C++ **2072/2072 PASS**, agent **7/7 PASS**; finalny Qt **1286 PASS, 1 SKIPPED, 0 FAIL** (1287 uruchomionych). Pierwsze uruchomienie Qt ujawniło jeden stary test wymagający `junction UV==empty`; w tym samym etapie poprawiono jego oczekiwania, zachowano guard zabezpieczający, ponowny test `QT_FINAL_RC=0`. Dowody: `archive/visual-regression/s2.2h-full-build.log`, `s2.2h-final-gate.log`, `s2.2h-qt-pass.log`, `s2.2h-final-fps-{x,t,y}.json`. Nie potwierdzaj niezweryfikowanych funkcji.

**S2.2i — lepsze materiały i kontrola FPS PO zmianach (13.11.2026):**

- Zostawiono wszystkie oryginalne pliki Poly Haven CC0 bez zmian. `scripts/build_pbr_atlas.py` (Pillow + NumPy) produkuje deterministyczne, powtarzalne **4×4 atlas PBR 2048²** z dopasowanych przesunięć tych samych map albedo/nor_gl/rough; lekko osłabia dominujące pęknięcie w asfalcie, neutralizuje brąz betonu, wygładza mikro-normal i reguluje roughness. Wyjście `assets/textures/prawko_atlas/` (6 plików, zapisywane w Git). Kontrola `scripts/test_pbr_atlas.py`: **PBR_ATLAS_QA_PASS** (6/6: rozdzielczość, różnorodność kafelków, seamy <25/255). `scripts/check_asset_licenses.py`: **PASS**; sześć wpisów w `ASSETS_LICENSES.md`.
- Qt `editor/resources/resources.qrc` używa nowego atlasu, `MaterialCatalog` dla `asphalt` i `concrete` ma `uv_scale=0.0625` (pełna faza 16 m; wewnętrzne kafelki 4 m), roughness scalar 0.87 / 0.93, normal_strength 0.85 / 0.75. `asphalt_worn` oraz materiały użytkownika pozostawione bez zmian. Eksporter glTF `mesh_export_common.hpp` i `gltf_exporter.cpp` odczytuje dokładnie te same nowe PBR i UV 0.0625; format scen/triangulacja bez zmian. Test Qt `SurfaceTextures.PrawkoAtlasKeepsFourMeterDetailButVariesOverSixteenMeters`.
- Odbiór obrazu: 30 aktualnych kadrów sprzętowego GLRenderer `archive/visual-regression/s2.2i-material-after/` (X/T/Y, 5 kamer, base + LOD), bezpośredni screenshot uruchomionego `roadmaker-editor.exe`: `s2.2i-gui-x-orbit.png`. W porównaniu z S2.2h spękania są mniej dominujące i nie powracają w identycznej konfiguracji co 4 m; beton mniej brązowy, przejście ulicy przez junction nadal poprawne. **Nie mylić z końcowym realizmem** — miejscami widoczne powtarzalne pęknięcia w skali 16 m, brak pełnego oświetlenia i PBR.
- GLB wykonany dla X/T/Y przez aktualny agent (`s2.2i-pbr-{x,t,y}.glb`, odpowiednio 16,75 / 16,69 / 16,66 MB); Blender 5.3 niezależnie zaimportował nowe materiały o obrazach **2048²**, obecne 3 mapy PBR, UV fazy i czoła, geometria niezerowego pola; `PBR_ATLAS_GLTF_IMPORT_PASS` dla X/T/Y. Render EEVEE X: `s2.2i-pbr-x-eevee.png` / logi `s2.2i-blender-{x,t,y}.log`. Blender ciemniejszy od Qt z powodu niezależnego oświetlenia — nie uznawać fotometrycznego parytetu.
- **GPU after** GTX 1050 Ti: mediana GL_TIME_ELAPSED LOD ukośnie X **0,493 ms**, T **0,473 ms**, Y **0,430 ms** (wzrost względem S2.2g ~0,07–0,10 ms); `texture_decode_upload_sync_ms` zimny **612/626/551 ms** X/T/Y vs wcześniejsze ~42–56 ms. Jest to **istotny koszt pierwszego wczytania**; nie występuje co klatkę. Dane `s2.2i-material-after/{x,t,y}-gpu-benchmark.json`.
- **Widoczny Qt FPS after**: 1600×900, po 180 próbkach `frameSwapped`, sztuczny timer 16 ms, symulowany gest `Alt+LMB`, X **60,00 FPS, p95 17,19 ms**; T **60,00, p95 17,36 ms**; Y **60,01, p95 17,11 ms**. We wszystkich **0 klatek >33,33 ms**. JSON `archive/visual-regression/s2.2i-fps-{x,t,y}.json`. Nadal `msaa_samples=0` mimo ustawienia 4x w żądanym formacie. To NIE jest pomiar frametime przeciągania węzłów ani niezależnego maksimum FPS bez VSYNC.
- **Build/test:** pierwszy `prawko-build-win.ps1 -Target all` trafił na stary uszkodzony artefakt CMake `junction_surface.cpp.obj` (LNK1236; kod tego pliku NIENARUSZONY). Usunięto **wyłącznie** ten wygenerowany `.obj`, ponowny `BUILD_PASS: all`. `s2.2i-quality-gate.log`: `QUALITY_GATE_PASS` (C++ 2072 PASS, Python 7 PASS); `s2.2i-qt.log`: **1287 PASS, 1 SKIPPED, 0 FAIL**; nie usuwano scen, nie zmieniano oryginalnego RoadMaker. Agenta Git bez zmian.

**S2.2j — optymalizacja PBR, MSAA, FPS podczas remesh, kontrola Blender (13.11.2026):**

- `scripts/build_pbr_atlas.py` zachowuje **albedo 2048²**, ale po filtrowaniu Lanczos i ponownej normalizacji zapisuje `*_nor_gl_1024.png` i `*_rough_1024.jpg`. Qt qrc i `core/src/io/mesh_export_common.hpp` odczytują dokładnie te same nowe źródła; `uv_scale=0.0625` bez zmian, poprzednie 2048² normal/rough usunięte tylko z bieżącego zestawu wygenerowanych pochodnych (historia Git i oryginalne Poly Haven CC0 nietknięte). `scripts/test_pbr_atlas.py`: **PBR_ATLAS_QA_PASS** (6 plików, ciągłość brzegów, różne kafelki, normalizacja normalnych). `check_asset_licenses.py` **PASS**, rejestr uaktualniony.
- Trzy nowe GLB przez aktualnego agenta: `archive/visual-regression/s2.2j-{x,t,y}.glb` **5 486 412 / 5 424 288 / 5 395 088 B** względem 16 748 684 / 16 686 448 / 16 657 240 B w S2.2i (redukcja ~67%). Blender 5.3 **S2.2J_BLENDER_GLTF_PASS X/T/Y**: dwie mapy albedo 2048², normal i rough 1024², niezerowe pola faz i czoła krawężnika, kompletne UV i materiały. Logi `s2.2j-blender-{x,t,y}.log`. Blender X z kontrolowanym standardowym światłem SUN/ambient i standardowym view transform: `s2.2j-x-blender-daylight.png`. **Nie istnieje pikselowy parytet Qt vs Blender**: dwa różne modele światła, tonemapping i oświetlenie czoła; nie markować jako fotorealizm.
- Rzeczywisty GLRenderer NVIDIA GTX 1050 Ti: `archive/visual-regression/s2.2j-gpu/{x,t,y}-gpu-benchmark.json` oraz 30 zrzutów. Zimny decode+upload (z synchronizacją) **X 267,5 / T 272,6 / Y 279,6 ms**, wobec S2.2i **612,3 / 625,8 / 550,5 ms**. Przy RGBA8 oszacowana pamięć 6 map bez mips spadła ~96 MiB → ~48 MiB (to **obliczenie z rozdzielczości**, nie odczyt rzeczywistego VRAM). GPU p50 LOD oblique **0,583 / 0,568 / 0,558 ms**, względem wcześniejszych 0,493 / 0,473 / 0,430 ms — porównanie serii nie jest odizolowaną ablacją, nie przypisywać różnicy tylko MSAA ani teksturom. `s2.2j-pixel-compare.py` porównał **30 identycznych kadrów** z S2.2i: maks. średnia różnica RGB **0,1741 / 255**, żadnych pikseli różniących się o ponad 16 poziomów; **S2.2J_PBR_IMAGE_REGRESSION_PASS**, zapis `s2.2j-pixel-diff.json`.
- **MSAA diagnoza (bez zmiany kodu produkcyjnego):** pierwotne `viewport->format().samples()==0` w `prawko_qt_fps_probe` NIE mierzyło rzeczywistego antyaliasingu. Po dodaniu diagnostyki `QOpenGLWidget::makeCurrent` i `glGetIntegerv(GL_SAMPLE_BUFFERS / GL_SAMPLES)`, próbka rzeczywistego FBO **bufory=1, próbki=4** dla X/T/Y. Potwierdzono **także po cofnięciu eksperymentalnego `ViewportWidget::setFormat(4)`**: `s2.2j-msaa-no-explicit-viewport-format.json` — nadal 4 próbki. **Nie było udowodnionej usterki MSAA.** Koniec etapu pozostawia produkcyjny `ViewportWidget` bez niepotrzebnej ingerencji.
- `prawko_qt_edit_probe` (manualny `EXCLUDE_FROM_ALL`) używa widocznego Windows Qt `ViewportWidget`, prawdziwego `Document` oraz produkcyjnego `update_node_drag` / `begin_preview` / `update_preview` / remesh / upload GPU, bez zapisu plików scen. `archive/visual-regression/s2.2j-edit-fps-{x,t,y}.json`: **X 60,02 FPS p95 17,27 ms, CPU edycji p95 7,89 ms, 205 zmian siatki; T 60,00 FPS p95 17,23 ms, CPU 6,82 ms, 206 zmian; Y 60,01 FPS p95 17,37 ms, CPU 5,21 ms, 206 zmian; wszystkie 0 klatek >33,33 ms**. `real_edit_verified=true` i aktywna sesja preview. Skrypt wymusza pozycje węzłów timerem 16 ms — NIE jest testem fizycznej myszy ani przeciążonej kilkunastotysięcznej sceny. Dodatkowo pełne GUI `prawko_qt_fps_probe` `s2.2j-qt-fps-{x,t,y}.json`: ~60 FPS, p95 obrotu X 17,20, T 17,23, Y 17,23 ms, 0 jank.
- Kompilacja `s2.2j-final-build.log`: **BUILD_PASS all**, synchronizacja agenta PASS; `s2.2j-final-gate.log`: **QUALITY_GATE_PASS** (C++ 2072 PASS, Python 7 PASS), `s2.2j-final-qt.log`: **1287 PASS + 1 SKIPPED**, 0 FAIL. Po cofnięciu redundantnego `setFormat` ponowiono pełny build i testy. **Uwaga na żywą pracę użytkownika:** od 13:36 `roadmaker-editor.exe` PID 4744, tytuł `scene.xodr* - PrawkoRoadStudio...` (**asterysk = niezapisane zmiany**). Nie ubijać, nie przeładowywać i nie nadpisywać tej sceny. Ponowiony opt-in benchmark edycji dla ostatecznej wersji binarnej celowo **wstrzymano**, żeby nie obciążać aktywnej sesji graficznej; powyższy benchmark edycji był wykonany wcześniej z jedyną eksperymentalną zmianą `setFormat`, potem cofniętą po ablacjach FBO. Nie uznawać braku końcowego powtórzenia za PASS tego konkretnego porównania.

**Historyczny plan po S2.2l (częściowo zrealizowany w S2.2m; aktualny krok S2.2n jest na początku HANDOFF):** S2.2l (instrukcje i obsługa stylów przez przeciąganie i dwuklik) zakończony — wynik i dowody na początku HANDOFF. Rozszerzyć testy fizycznego drag & drop na obiekty, sygnały, przejścia, materiały i obszary niedozwolone, ustalić spójny model edycji po upuszczeniu. Następnie wdrożyć parametryczny krawężnik obniżony/rampę oraz wysepkę jako samodzielnie przeciągany obiekt z zachowaniem siatki i OpenDRIVE. Powtórzyć test obciążeniowy większych scen (wiele dróg/props, ruch myszy, drag/remesh, p95/p99, VRAM). Oddzielnie wykończyć oświetlenie Qt/Blender oraz widoczne długie spękania. Nie uznawać fotorealizmu za ukończony. Nie zmieniać CDT/OpenDRIVE/goldenów bez uzasadnienia.





**Historyczny plan S2.2c:** dokończyć 5 cm fazę krawężnika
przez cały łuk narożnika (obecna ściana narożnika jest
pionowa, a faza drogi zwęża się przed wlotem), zszyć
górną krawędź i sprawdzić profile z nachyleniem oraz
przejścia na ciasnych T/Y. Następnie eksport glTF,
test wizualny z kamery kierowcy i dopiero później PBR.
Nie uznawać obecnych materiałów za realistycznie odebrane.

**Historyczna lista odbiorów geometrii po S2.2j i S2.2k.**

1. Uruchomić `scripts/prawko-resume.ps1` i sprawdzić oba repozytoria oraz otwarte sceny. Nie przeładowywać niezapisanych dokumentów.
2. **Testy Qt Windows:** ostatnia pełna sesja **S2.2k 1292 PASS, 1 SKIPPED, 0 FAIL**, `archive/visual-regression/s2.2k-qt-full-final.log`; `qoffscreen.dll` w `build/dev-windows/editor/platforms/`. Nowe 12 pozycji biblioteki są wbudowane w exe i dostępne od startu, nie trzeba ich importować.
3. **S2.2c — odbiór bazowej geometrii X/T/Y i niezależnego GLB wykonany.** Zrzut bieżącego GUI, kamera kierowcy i izolat narożnika w Blenderze oraz import GLB są w lokalnym `archive/visual-regression/`. Testy i dowody wyżej. Zwróć uwagę na segmentację łuku — nie mylić z ostatecznym wyglądem.
4. **S2.2j edycja geometrii i MSAA:** 205–206 aktualizacji siatki na scenę X/T/Y z prawdziwą sesją preview, 60 FPS, p95 17,23–17,37 ms (timer testowy). GL_SAMPLE_BUFFERS=1 i GL_SAMPLES=4 — wcześniejsze `format().samples()==0` było niemiarodajne. **Następne:** test pełnej sceny pod obciążeniem i fizycznej myszy, gdy użytkownik zamknie lub zapisze otwarty niezapisany dokument. Nie zmieniać CDT, OpenDRIVE ani goldenów.
5. **S2.2j odciążenie materiałów wykonane:** zimny upload 0,268–0,280 s, GLB 5,4–5,5 MB, 30 widoków RGB regresja PASS, Blender X/T/Y PASS. Światło Qt/Blender porównano z kontrolowanym ambient+SUN, ale wynik nadal nie jest fotometrycznie równoważny, krawężnik i długie spękania w 16 m atlasie nadal potrzebują dopracowania.

**Kryterium ukończenia:** kod + testy + rzeczywista ocena GUI; potem commity, push i aktualizacja TEGO pliku. Bez zgadywania stanu na podstawie dawnych raportów.

## Jak rozpocząć i zweryfikować

W PowerShell (z dowolnego miejsca):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1 -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-build-win.ps1 -Target editor -Jobs 3
```

- `prawko-resume.ps1` czyta **bieżący Git** i ten dokument, sygnalizuje odchylenia względem `origin`.
- `-Verify` obejmuje Quality Gate jądra i agenta. Testy Qt uruchom oddzielnie po `prawko-build-win.ps1 -Target all`, z `QT_QPA_PLATFORM=offscreen` i Qt `bin` w `PATH`. Plik wykonywalny to `build/dev-windows/editor/roadmaker_editor_tests.exe`. Aktualne wyniki zapisuj w sekcji **Stan bieżący**.
- Start programu: `build/dev-windows/editor/roadmaker-editor.exe`.
- Agent: `scripts/prawko-agent.cmd`.
- Główne wymagania: `docs/PRAWKO-ROAD-STUDIO.md`.
- Reguły współpracy: `AGENTS.md`.

## Reguła przekazywania stanu — po każdym ważnym etapie i na końcu

**Zapisuj postęp po każdym ukończonym etapie.** Wykonaj właściwe testy, zaktualizuj sekcje **Stan bieżący** i **Aktywny plan** na górze, zacommituj zmiany i wypchnij je na GitHub.

1. Nie zostawiaj rozpoczętych zmian bez informacji. Jeśli praca została przerwana, wpisz do tego pliku **co zmieniono, co nie zostało przetestowane i co powinno być zrobione jako pierwsze**. Nie nazywaj takiego kroku PASS.
2. Wpisz aktualny stan i następny konkretny krok do **tej samej sekcji** (bez mnożenia notatek). Zmień znacznik `agent_head` poniżej po ostatnim commicie agenta.
3. Zacommituj zmiany w odpowiednich repozytoriach; ostatni commit edytora powinien zawierać aktualizację `HANDOFF.md`. Wypchnij do `origin` i sprawdź SHA zdalnych gałęzi.
4. Wykonaj `scripts/prawko-git.ps1 -Action finish` (sprawdzenie czystości, Quality Gate i backup). Przekaż użytkownikowi oba SHA, wyniki, ewentualne blokady.
5. **Nie nadpisuj otwartej niezapisanej sceny.** Pliki `agent/projects/` nie są objęte Git; backupują się lokalnie, ale niezapisanej sceny GUI nie da się odzyskać z tych kopii.
6. Jeśli nie ma dostępu do komputera lub testów, opisz ograniczenie, zamiast twierdzić, że praca została wykonana.

<!-- agent_head: 35c48c11cdb7efca8e08d1be731463657bbe7a32 -->
