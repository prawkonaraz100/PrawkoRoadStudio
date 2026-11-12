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
Chodniki narożne w skrzyżowaniach nadal są płaskimi pasmami.
Potrzebne są spójne wzniesione naroża, zjazdy i obniżone krawężniki
przy przejściach, normalne i oświetlenie oraz regulacja wysokości
i profilu krawężnika myszką. Nie opisywać tych funkcji jako wykonanych.
