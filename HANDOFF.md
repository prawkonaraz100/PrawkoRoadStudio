# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> **Jedyny aktualizowany dziennik przekazania między rozmowami.**
> Kolejny agent ma czytać ten plik **przed edycją kodu**.
> Stan dokumentu: 2026-11-13, po wdrożeniu S2.2b i weryfikacji silnika; testy Qt zidentyfikowane jako blokada środowiskowa.
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

**Blokada dodatkowych testów Qt (NIE PASS):** skompilowano
`roadmaker_editor_tests.exe`, lecz `gtest_discover_tests`
nie potrafi uruchomić go na tym Windows: początkowo brak
biblioteki (`0xc0000135`) bez PATH Qt, następnie przy
prawidłowym PATH start procesów utykał (timeout 5 i 60 s),
a diagnostyczna próba pominięcia `QApplication` podczas
listowania dała `0xc0000409`. Zmiany eksperymentalne
w `qt_gtest_main.cpp` i `editor/tests/CMakeLists.txt`
cofnięto do stanu repo, aby nie włączać niedziałającego obejścia.
`DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity`
oraz rozszerzone `test_mesh_parity.cpp` kompilowały się,
ale **nie zostały uruchomione i nie są zaliczone**.
Należy ustalić dokładną przyczynę startu dużej binarki Qt
(m.in. pluginy/DLL/ustawienia procesu/Windows event logs)
i dopiero wtedy zaliczyć testy GUI. Zestaw 2059 C++
i 6 agenta jest osobno potwierdzony PASS.

**Następny krok S2.2c:** dokończyć 5 cm fazę krawężnika
przez cały łuk narożnika (obecna ściana narożnika jest
pionowa, a faza drogi zwęża się przed wlotem), zszyć
górną krawędź i sprawdzić profile z nachyleniem oraz
przejścia na ciasnych T/Y. Następnie eksport glTF,
test wizualny z kamery kierowcy i dopiero później PBR.
Nie uznawać obecnych materiałów za realistycznie odebrane.

**Kontynuować przebudowę realistycznej geometrii dróg i chodników.** Nie zaczynać ponownie od konfiguracji Git ani migracji folderów.

1. Uruchomić `scripts/prawko-resume.ps1` i sprawdzić oba repozytoria oraz otwarte sceny. Nie przeładowywać niezapisanych dokumentów.
2. **NAJPIERW odblokować testy Qt Windows:** `roadmaker_editor_tests.exe` nie przechodzi odkrywania GTest (po naprawie PATH nadal timeout i diagnostyczny fastfail 0xc0000409). Uruchomić `DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity` i `MeshParity.*` z `QT_QPA_PLATFORM=offscreen`, sprawdzić wyniki, nie oznaczać testów GUI jako PASS na podstawie samej kompilacji.
3. **S2.2c:** zrealizować ciągłą 5-centymetrową fazę na łuku narożnika, wykorzystując constrained edges i wspólne wierzchołki. Chronić kompatybilny tryb `raised_sidewalks=false`.
4. Dopisać testy ciasnych T/Y, nachylonych ulic i przejść, brak z-fightingu, eksport glTF/UV, pełny Undo/Redo i widok kamery kierowcy. Sprawdzić koszt przebudowy `remesh_objects` w trybie 3D.
5. Dopiero po poprawnej geometrii i odbiorze GUI przejść do PBR, skali UV, oświetlenia i proceduralnych detali.

**Kryterium ukończenia:** kod + testy + rzeczywista ocena GUI; potem commity, push i aktualizacja TEGO pliku. Bez zgadywania stanu na podstawie dawnych raportów.

## Jak rozpocząć i zweryfikować

W PowerShell (z dowolnego miejsca):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1 -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-build-win.ps1 -Target editor -Jobs 3
```

- `prawko-resume.ps1` czyta **bieżący Git** i ten dokument, sygnalizuje odchylenia względem `origin`.
- `-Verify` uruchamia Quality Gate. Pełna kompilacja i **ręczny test GUI** są osobną kontrolą.
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

<!-- agent_head: 147a08d537530a4b07b0bd7840f6197dc9f62512 -->
