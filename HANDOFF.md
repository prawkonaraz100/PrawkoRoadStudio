# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> **Jedyny aktualizowany dziennik przekazania między rozmowami.**
> Kolejny agent ma czytać ten plik **przed edycją kodu**.
> Stan dokumentu: 2026-11-13, **S2.2f**: w GUI oraz GLB agenta działa opcjonalny, bezpieczny post-CDT LOD zakrzywionych faz krawężników na X/T/Y, bez nowych sliverów i bez zmian historycznych goldenów; 2072/2072 C++ PASS (10 DISABLED), 7/7 agent PASS, Qt 1284 PASS + 1 SKIPPED, GLB/Blender 5.3 X/T/Y PASS, pełny Windows BUILD PASS. **Odbiór finalnego fotorealizmu i wydajności GPU nadal OTWARTY.**
> Poniższe SHA to punkt kontrolny, nie zamiennik aktualnego `git status`.

## Gotowe polecenie do nowej rozmowy

> **Kontynuuj PrawkoRoadStudio od miejsca przerwania. Połącz się z moim komputerem przez Desktop Commander. Zacznij od `F:\PrawkoRoadStudio\HANDOFF.md` i `AGENTS.md`; uruchom `scripts/prawko-resume.ps1`, sprawdź oba repozytoria Git oraz testy. Nie zgaduj stanu z poprzedniej rozmowy. Dokończ pierwsze niewykonane zadanie z HANDOFF.md, a na koniec zaktualizuj HANDOFF.md, wykonaj commity, push i Quality Gate. Nie nadpisuj niezapisanych scen.**

## Gdzie pracować

- Komputer przez Desktop Commander: `DESKTOP-G2FV0GJ`.
- **Aktywny katalog:** `F:\PrawkoRoadStudio` (C++20, Qt 6).
- **Agent AI — osobne repo Git:** `F:\PrawkoRoadStudio\agent`.
- Repo publiczne edytora: https://github.com/prawkonaraz100/PrawkoRoadStudio
- Repo publiczne agenta: https://github.com/prawkonaraz100/PrawkoRoadStudio-Agent
- Obie gałęzie edytora (`main`, `feature/realistic-road-surfaces`) wskazywały ten sam kod przed utworzeniem tego przekazania.
- Kopia oryginału `reference/RoadMaker/` jest **tylko referencyjna** — nie edytuj, nie wysyłaj zmian do upstream.
- `toolchain/`, `.qt/`, `build/`, `agent/projects/` i `archive/` pozostają na dysku; część jest ignorowana przez Git i nie występuje w publicznych repozytoriach.

## Ostatnie potwierdzone wykonanie

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

## Co robimy dalej — najbliższe zadanie

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

**Następne pierwsze niewykonane zadanie:** PBR w GUI i lokalny detal
łuków z perspektywy kierowcy; dokładnie zmierzyć błąd sylwetki
na ekranie w różnych odległościach/pochyleniach, rozważyć większy
LOD tylko przy spełnieniu jakości siatki; osobno mierzyć koszt
GPU-upload i czas wyświetlenia klatki. Nie globalnie zagęszczać
CDT. Nadal otwarte powtarzalne spękania asfaltu i oświetlenie.

**Historyczny plan S2.2c:** dokończyć 5 cm fazę krawężnika
przez cały łuk narożnika (obecna ściana narożnika jest
pionowa, a faza drogi zwęża się przed wlotem), zszyć
górną krawędź i sprawdzić profile z nachyleniem oraz
przejścia na ciasnych T/Y. Następnie eksport glTF,
test wizualny z kamery kierowcy i dopiero później PBR.
Nie uznawać obecnych materiałów za realistycznie odebrane.

**Kontynuować przebudowę realistycznej geometrii dróg i chodników.** Nie zaczynać ponownie od konfiguracji Git ani migracji folderów.

1. Uruchomić `scripts/prawko-resume.ps1` i sprawdzić oba repozytoria oraz otwarte sceny. Nie przeładowywać niezapisanych dokumentów.
2. **Testy Qt Windows już odblokowane:** przed zmianami GUI sprawdzić `roadmaker_editor_tests` i `qoffscreen.dll` w `build/dev-windows/editor/platforms/`; CMake kopiuje plugin automatycznie. Ostatnia pełna sesja: 1284 PASS, 1 SKIPPED, 0 FAIL; wyniki oraz konfiguracja w akapicie S2.2b powyżej. Nie zmieniać `qt_gtest_main.cpp` bez odtwarzalnej regresji.
3. **S2.2c — odbiór bazowej geometrii X/T/Y i niezależnego GLB wykonany.** Zrzut bieżącego GUI, kamera kierowcy i izolat narożnika w Blenderze oraz import GLB są w lokalnym `archive/visual-regression/`. Testy i dowody wyżej. Zwróć uwagę na segmentację łuku — nie mylić z ostatecznym wyglądem.
4. **NASTĘPNE — odbiór wyglądu i kosztu GPU:** opcjonalny `curb_render_lod` S2.2f jest wdrożony i przechodzi Quality Gate, nie trzeba go budować ponownie. Wykonać ilościowy test sylwetki przy kamerze kierowcy w GUI (zbliżenia i większe nachylenia) oraz rozróżnić subtelny zysk <=8 mm od wizualnej gotowości. Sprawdzić z-fighting i normy faz, mierzyć GPU upload, całkowity frametime i ewentualne zacięcia edycji, nie tylko CPU `remesh_objects`. Bez zmian starego formatu OpenDRIVE i globalnych parametrów triangulacji.
5. **NASTĘPNE — materiały i oświetlenie:** usunąć wyraźne powtarzanie spękań 4-metrowej tekstury, urealnić tint i rozkład roughness, sprawdzić przejścia asfalt/beton oraz oświetlenie na niskim ujęciu i w samym edytorze. Export GLB zawiera już CC0 albedo/normal/roughness (Blender X/T/Y PASS), ale realizm, instalacja materiałów i pełna zgodność wyglądu GUI vs Blender **nie** są odebrane.

**Kryterium ukończenia:** kod + testy + rzeczywista ocena GUI; potem commity, push i aktualizacja TEGO pliku. Bez zgadywania stanu na podstawie dawnych raportów.

## Jak rozpocząć i zweryfikować

W PowerShell (z dowolnego miejsca):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1 -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-build-win.ps1 -Target editor -Jobs 3
```

- `prawko-resume.ps1` czyta **bieżący Git** i ten dokument, sygnalizuje odchylenia względem `origin`.
- `-Verify` uruchamia Quality Gate jądra i agenta; **nie obejmuje testów Qt edytora**. Te ostatnie trzeba budować oddzielnie (target `roadmaker_editor_tests` lub `prawko-build-win.ps1 -Target all`) i uruchamiać z `QT_QPA_PLATFORM=offscreen` oraz Qt `bin` w `PATH`; plugin `qoffscreen.dll` kopiuje CMake. Ostatnia kompletna seria: 1284 PASS, 1 SKIPPED. Pełna kompilacja i **ręczny test GUI** są osobną kontrolą.
- Start programu: `build/dev-windows/editor/roadmaker-editor.exe`.
- Agent: `scripts/prawko-agent.cmd`.
- Główne wymagania: `docs/PRAWKO-ROAD-STUDIO.md`.
- Reguły współpracy: `AGENTS.md`.

## Reguła przekazywania stanu — po każdym ważnym etapie i na końcu

**Nie czekaj na zakończenie rozmowy.** Po każdym ukończonym kroku aktualizuj poniższy stan, zapisuj commity i wypychaj je na GitHub. Nagłe urwanie rozmowy nie powinno oznaczać utraty poprzednich punktów kontrolnych.

1. Nie zostawiaj rozpoczętych zmian bez informacji. Jeśli praca została przerwana, wpisz do tego pliku **co zmieniono, co nie zostało przetestowane i co powinno być zrobione jako pierwsze**. Nie nazywaj takiego kroku PASS.
2. Wpisz aktualny stan i następny konkretny krok do **tej samej sekcji** (bez mnożenia notatek). Zmień znacznik `agent_head` poniżej po ostatnim commicie agenta.
3. Zacommituj zmiany w odpowiednich repozytoriach; ostatni commit edytora powinien zawierać aktualizację `HANDOFF.md`. Wypchnij do `origin` i sprawdź SHA zdalnych gałęzi.
4. Wykonaj `scripts/prawko-git.ps1 -Action finish` (sprawdzenie czystości, Quality Gate i backup). Przekaż użytkownikowi oba SHA, wyniki, ewentualne blokady.
5. **Nie nadpisuj otwartej niezapisanej sceny.** Pliki `agent/projects/` nie są objęte Git; backupują się lokalnie, ale niezapisanej sceny GUI nie da się odzyskać z tych kopii.
6. Jeśli nie ma dostępu do komputera lub testów, opisz ograniczenie, zamiast twierdzić, że praca została wykonana.

<!-- agent_head: 35c48c11cdb7efca8e08d1be731463657bbe7a32 -->
