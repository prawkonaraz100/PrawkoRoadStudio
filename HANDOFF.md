# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> **Jedyny aktualizowany dziennik przekazania między rozmowami.**
> Kolejny agent ma czytać ten plik **przed edycją kodu**.
> Stan dokumentu: 2026-11-13, S2.2d: poprawione normalne faz krawężników i neutralna charakterystyka betonu, import GLB w Blenderze X/T/Y PASS, pełne testy C++/Qt/agent PASS. Próba zagęszczenia obrysu została wycofana z powodu regresji jakości triangulacji. Docelowy realizm PBR, sylwetka łuków i profilowanie remesh pozostają OTWARTE.
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
4. **NASTĘPNE:** zweryfikować wydajność `remesh_objects` w scenach X/T/Y z przejściami; sprawdzić większe pochylenia, brak z-fightingu i zbliżenia z kamery kierowcy w GUI. Sylwetka łuków pozostaje segmentowa: opracować i przetestować osobną bezpieczną metodę zwiększenia detalu (np. LOD renderowania) **bez zmian goldenów prostych wlotów i bez przekroczenia budżetu sliverów**. Nie wprowadzać globalnie prób z S2.2d.
5. **PBR NADAL OTWARTE:** przeanalizować wykorzystanie istniejących legalnie dołączonych map concrete/asphalt (albedo, normal, roughness) w GUI i glTF z rzeczywistą skalą UV, bez sztucznych jaskrawych materiałów. Potem dopracować światło i detale oraz przeprowadzić odbiór realistycznego wyglądu. Obecny rendering nie jest fotorealistyczny.

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

<!-- agent_head: 02cbdeb5e943f7a710d3292a9fbc5e85d3cc95fc -->
