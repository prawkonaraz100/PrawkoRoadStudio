# PrawkoRoadStudio — osobny projekt oparty na RoadMaker

Stan początkowy: 12.11.2026. Repozytorium projektu: `F:\PrawkoRoadStudio`.
Gałąź projektowa: `feature/realistic-road-surfaces`, fork lokalny RoadMaker
(upstream Robomous/RoadMaker, commit bazowy `c7a3f5a`, Apache-2.0).

## Kierunek produktu

Budujemy **nową jakość wizualną geometrii**, a nie nakładkę kolorystyczną
na stary edytor. Pozostajemy przy natywnym C++20/Qt 6, OpenDRIVE i
OpenSCENARIO. Żaden element nie może być dostępny wyłącznie przez CLI:
każda funkcja projektowania musi również działać w interfejsie myszką.

### Priorytety

1. Jezdnia: realna objętość i kształt przekroju, spadki poprzeczne,
   delikatne różnice wysokości i płynne przejścia, pasy i oznakowanie.
2. Chodnik: osobna powierzchnia wyniesiona nad jezdnię, geometria
   obrzeża, różne szerokości, materiały i realistyczne łączenie w narożach.
3. Krawężnik: oddzielne powierzchnie pionowe i fazy, promień,
   obniżenia przy przejściach i zjazdach, ciągłość na łukach.
4. Skrzyżowania: poprawne i płynne połączenie nawierzchni jezdni,
   chodnika, wysp i wyokrągleń; brak pęknięć i z-fightingu.
5. Materiały: fizycznie przekonujące UV, normal, roughness i światło,
   skala zbliżona do rzeczywistych nawierzchni; bez przypadkowych ozdobników.
6. Otoczenie: pobocza, ścieżki, teren, pasy zieleni, oznakowanie
   przystosowane do polskich scen dydaktycznych.

### Definicja ukończenia funkcji

- Natywne operacje C++/Python i edycja GUI myszką wywołują te same reguły.
- OpenDRIVE zapisuje trwałe odwołania do dróg, pasów, obiektów.
- Zapis/odczyt oraz Undo/Redo zachowują geometrię.
- Testy jednostkowe i scenariusz regresji z rzeczywistą aplikacją Windows.
- Sprawdzenie zarówno odcinka prostego, łuku i skrzyżowania T/X.
- Zero błędów walidacji, brak utraty danych przy ponownym otwarciu.

### Reguły architektoniczne

- Nie korzystać z projektów Unreal Engine.
- Nie modyfikować klona porównawczego `F:\PrawkoRoadStudio\reference\RoadMaker`.
- Zmiany C++ robić w tym repo, w osobnych commitach po testach.
- Dotychczasowy interfejs agenta znajduje się w `F:\PrawkoRoadStudio\agent`;
  podczas budowania nowej wersji trzeba przełączyć go na nowe EXE i
  natywne bindingi Python po ich przekompilowaniu.
- Zasilanie projektu scenami dla człowieka i AI pozostaje wspólne.
- Zasoby Qt i CMake/Python mogą być używane z instalacji
  `F:\PrawkoRoadStudio\reference\RoadMaker` bez kosztownego kopiowania.

## Standard jakości wizualnej (decyzja projektowa, 12.11.2026)

**Cel:** sceny dróg w Polsce mają wyglądać wiarygodnie z kamery kierowcy,
a nie jak płaski, sterylny prototyp lub przypadkowy zestaw gotowych modeli.
Stonowany, spójny wygląd; bez przesadnych efektów, sztucznego połysku,
losowych ozdobników i powtarzalnego „growego kiczu”. Kierunek docelowy:
wiarygodny realizm symulatorowy, nie deklarowany z góry fotorealizm.

Nie wystarczy poprawna siatka 3D ani sama wymiana tekstur. Wymagane są
**jednocześnie bryła, materiały, światło, detale i weryfikacja w edytorze**.

### Kolejność wdrażania

1. **Geometria i proporcje:** spadki jezdni, realistyczne profile chodników
   i krawężników, ciągłość na łukach i skrzyżowaniach T/X, obniżenia przy
   przejściach, brak szczelin, przenikania powierzchni i z-fightingu.
2. **Materiały PBR:** asfalt, beton chodnikowy, beton krawężników, kostka,
   oznakowanie i grunt. Każdy materiał ma naturalną, niezbyt nasyconą
   kolorystykę, poprawną skalę UV w metrach, normal map i roughness.
   Wdrożenie wymaga potwierdzenia obsługi tych parametrów w rendererze Qt;
   samo dodanie plików tekstur nie oznacza ukończenia.
3. **Detale proceduralne:** dyskretne dylatacje, łączenia i segmenty
   krawężników, wpusty deszczowe, studzienki, niewielkie różnice
   nawierzchni, strefy przy krawężnikach i polskie oznakowanie.
   Nie rozmieszczać dekoracji przypadkowo ani jednakowo w każdej scenie.
4. **Oświetlenie i prezentacja:** spójne światło dzienne, wiarygodne miękkie
   cienie i kontakt cienia w miejscach styku, kontrolowane odbicia,
   ekspozycja bez przepaleń; delikatne AO tylko gdy jest obsługiwane.
5. **Regresja wizualna:** stałe sceny referencyjne — prosta, łuk, T, X,
   przejście z obniżeniem, zmiana materiału, ujęcie z wysokości kierowcy.
   Porównywać ujęcia z jedną ustaloną kamerą i światłem, przy różnych
   odległościach, oraz kontrolować wydajność.

### Kryterium odbioru wyglądu

- Wrzucenie tekstury lub zmiana koloru **nie oznacza** ukończenia etapu.
- Nie raportować „realistycznie”, „premium” ani „fotorealistycznie”
  bez oględzin uruchomionego edytora i porównania ze zdjęciami
  referencyjnymi polskich ulic (legalnie pozyskanymi).
- Na zbliżeniu krawężnik ma czytelny profil i kontakt z jezdnią,
  bez widocznego zawieszenia; z perspektywy kierowcy prawidłowe
  proporcje, skala tekstur i brak rażącej powtarzalności.
- Sprawdzić zachowanie na prostych, łukach i skrzyżowaniach T/X
  oraz w warunkach bez dodatkowych materiałów (tryb awaryjny).
- Wymagane nadal testy geometrii, zapis/odczyt, Undo/Redo,
  wspólne C++/Python/GUI i testy z prawdziwego programu Windows.
- Każdą ukończoną część zatwierdzać w Git, odnotowywać dowody
  wizualne i aktualizować `HANDOFF.md`, aby kolejny agent wiedział,
  co jest wykonane, a co pozostaje planem.

**Stan na 12.11.2026:** podstawowa geometria wyniesionego chodnika na
odcinkach dróg jest zaimplementowana (S1). Wyniesione naroża, obniżenia
przy przejściach, zestaw PBR i dopracowane światło są **planowane**,
nie oznaczone jako gotowe.

## Środowisko Windows

- MSVC: `F:\PrawkoRoadStudio\toolchain\VS2022`
- CMake/Ninja/Python/Qt: `F:\PrawkoRoadStudio\reference\RoadMaker\toolchain` i `F:\PrawkoRoadStudio\reference\RoadMaker\.qt`
- Qt przez junction: `F:\PrawkoRoadStudio\.qt`
- Budowanie: `scripts\prawko-build-win.ps1` (kompilator lokalny).
- Stary projekt Unreal `F:\RoadSceneEditor` przeznaczony do usunięcia.
  Przed usuwaniem archiwizowane są poprawki Git w
  `F:\PrawkoRoadStudio\archive\legacy-backup\2026-11-12`.

**Uwaga:** priorytety powyżej to wymagania rozwojowe, a nie lista
ukończonych funkcji. Dopiero kod i testy potwierdzają realizację.

## Potwierdzone wykonanie — etap S1 (12.11.2026)

Pierwsza zmiana w generatorze geometrii jest zaimplementowana i potwierdzona na Windows.
To nie oznacza ukończenia docelowego stylu.

- MeshOptions.raised_sidewalks: w API domyślnie false dla kompatybilności,
  ale Qt PrawkoRoadStudio włącza true podczas budowania i aktualizacji siatki.
- Chodniki: osobna powierzchnia na wysokości 0,15 m, dopasowana do krzywej drogi.
- Krawężniki: pionowa ścianka i faza do 5 cm z własnymi normalnymi i UV.
- Przy styku ze skrzyżowaniem zastosowano krótki zjazd do poziomu starych pasm.
- OpenDRIVE pozostaje formatem nadrzędnym; nie przekształcano danych mapy.
- Nowy Python binding zawiera raised_sidewalks, a roadctl włącza geometrię
  przy eksporcie do glTF.
- Testy: wybrana grupa Mesh 11/11 PASS i RoadMakerAgent 6/6 PASS.
- GUI Windows: PrawkoRoadStudio uruchomione ze sceną miejskiej drogi w łuku.
- Pliki: F:\PrawkoRoadStudio\agent\projects\prawko_urban_curve\scene.xodr,
  scene.xosc, scene.glb i preview.png.

### Pozostałe prace

Domyślne modele drzew, podłoże i materiały nadal są uproszczone.
S2.1 podniósł chodniki narożne o 15 cm i dodał ściany krawężników.
S2.2a dodaje obniżenia przy przejściach śródodcinkowych, ale
naroża skrzyżowań nadal nie mają pełnej ciągłej fazy i obniżeń
przy przejściach tuż przed wlotem. Nieukończone pozostają materiały
PBR, oświetlenie, kontrola wyglądu z poziomu kierowcy oraz
regulacja krawężnika myszką. Nie opisywać tych prac jako wykonanych.

## Potwierdzone wykonanie — etap S2.1 (12.11.2026)

Pierwsza wersja przestrzennej geometrii **narożników skrzyżowań**
została zaimplementowana w tym samym C++ co GUI i binding Python.
Nie jest to jeszcze pełny realizm wizualny ani gotowe przejścia dla pieszych.

- `MeshOptions.raised_sidewalks=true` podnosi części chodnikowe
  wspólnej siatki skrzyżowania o `defaults::kCurbHeight` (0,15 m).
  Wykorzystuje istniejące obrysy i triangulację, bez zmian w OpenDRIVE.
- Krawędź wspólna chodnika z jezdnią otrzymuje pionową ścianę
  `LaneType::Curb`, własne normalne i UV; nie generuje się sztucznej ściany
  przy zewnętrznej granicy i między sąsiednimi chodnikami.
- Na wlotach dróg istniejący krawężnik zachowuje pełną wysokość,
  a jego faza 5 cm zwęża się do pionowego profilu na ostatnim metrze.
  Wariant bez meshowanej części chodnikowej nadal ma dotychczasowy zjazd.
- `raised_sidewalks=false` pozostaje zgodny z poprzednim płaskim sposobem
  generowania; siatka jezdni nie zmienia geometrii.
- Testy S2.1: **8/8 PASS** na wariantach T/X/Y, w tym mieszanych i ciasnych,
  kontrola obu krawędzi na wlotach, pionowych normalnych i wysokości.
  Po pełnym przebudowaniu testów Quality Gate: **2056 C++ PASS**,
  **6 testów agenta PASS**, 10 wyłączonych C++.
- Edytor Windows: **BUILD_PASS: editor**. W GUI uruchomiono kopię
  `gs1_urban_intersection.xodr`; screenshoty lokalne:
  `archive/visual-regression/s2-verified-gui.png` i
  `archive/visual-regression/s2-first-pass-close.png`.
  Osobne okno testowe zostało zamknięte bez modyfikowania oryginalnych scen.

**Pozostaje otwarte S2.2:** pełny profil fazowanego krawężnika przez naroże,
obniżenia przy przejściach, warianty z pochyleniem, dodatkowa kontrola
siatki z bliska i z wysokości kierowcy, eksport glTF/UV oraz niezależna
wizualna ocena jakości. Obecne kolory, materiały i drzewa nadal są
uproszczone. **Nie uznawać jakości wizualnej za odebraną** na podstawie
samej zielonej kompilacji/testów. Szczegółowa kolejność kontynuacji
znajduje się w `HANDOFF.md`.

## Implementacja S2.2a — obniżenia na przejściach śródodcinkowych (12.11.2026)

W `core/src/mesh/mesh_builder.cpp` użyto istniejących obiektów
`ObjectType::Crosswalk` (położenie, orientacja i rozmiar) do
**proceduralnego** obniżenia chodnika przy przejściach, bez edycji
pliku OpenDRIVE. Zjazd obejmuje pełną szerokość chodnika, a jego
styk z jezdnią leży na poziomie 0 m (krawężnik poza przejściem:
0,15 m). Obie 3-metrowe rampy mają profil cubic smoothstep
z maksymalnym nachyleniem około 7,5%. Ich próbkowanie
zawiera wymuszone węzły i środki przejść niezależnie od
kroku meshera; normalne wierzchołków zależą od pochodnej
tego profilu. Fragmenty ścian/faz o zerowym polu są odrzucane.

**Aktywacja jest ostrożna:** zjazd występuje tylko przy prostokątnym,
poprzecznym przejściu, które sięga krawędzi jezdni po konkretnej
stronie. Wariant `raised_sidewalks=false` nie zmienia istniejącej
geometrii. Przy edycji/wymazaniu obiektów w trybie 3D
`remesh_objects` odbudowuje pełną geometrię danej drogi,
ponieważ funkcja nie jest już niezależna od oznakowania
(koszt większy niż dotychczasowy remesh samej farby).

**Nieobsługiwane w tym podetapie:** przejścia tuż przed
skrzyżowaniem, gdy nie ma 3 m na zakończenie rampy
przed końcem wyniesionego narożnika; ciągła faza przez
naroże, łagodne obniżenia na łuku narożnika i
pełny odbiór wizualny. One należą do S2.2b.

Dwa testy `Mesh` kontrolują obniżenia obu stron,
zbieżność z przejściem, nachylenie normalnych,
brak degeneracji siatki, poprawność przyrostowej
przebudowy i odwrócenie po usunięciu przejścia,
oraz brak fałszywych zjazdów dla błędnego zasięgu
i orientacji. Scena kontrolna (niepublikowana
w repo): `archive/visual-regression/s2-midblock-curb-drop.xodr`.
Szczegółowy bieżący stan testów i następne zadanie:
`HANDOFF.md`.
