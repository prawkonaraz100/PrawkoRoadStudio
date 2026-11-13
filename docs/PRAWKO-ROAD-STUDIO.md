# PrawkoRoadStudio — osobny projekt oparty na RoadMaker

Stan początkowy: 12.11.2026. Repozytorium projektu: `F:\PrawkoRoadStudio`.
Gałąź projektowa: `feature/realistic-road-surfaces`, fork lokalny RoadMaker
(upstream Robomous/RoadMaker, commit bazowy `c7a3f5a`, Apache-2.0).

## Kierunek produktu

PrawkoRoadStudio ma być **jednym programem do tworzenia, edycji, odtwarzania
i nagrywania sytuacji drogowych dla PrawkoNaRaz**. Punktem odniesienia jest
publicznie opisany sposób pracy w VICOM. Instruktor wybiera szablon otoczenia,
układa drogi i uczestników ruchu, ustawia przebieg oraz kamerę, ogląda próbę
i eksportuje obraz lub film z tego samego okna. Najpierw rozwijamy ten
przepływ na dostępnych modelach zastępczych. Rozbudowa biblioteki modeli
i dopracowanie wyglądu mają odrębny harmonogram.

Pozostajemy przy C++20/Qt 6, OpenDRIVE i OpenSCENARIO. Narzędzia są dostępne
myszką w aplikacji. Program może używać silnika symulacji i kodera w tle,
a ich uruchamianie, komunikaty, postęp i wynik obsługuje interfejs edytora.
[Opis VICOM](https://vicomeditor.de/#how_it_works) ·
[Zakres funkcji i kryteria odbioru](PRAWKO-VICOM.md)

### Wymagania dla geometrii i wyglądu

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

**Pozostaje otwarte S2.2c:** pełny profil fazowanego krawężnika przez
naroże, warianty z pochyleniem, dodatkowa kontrola siatki z bliska i
z wysokości kierowcy, eksport glTF/UV oraz niezależna wizualna ocena
jakości. Obniżenia śródodcinkowe (S2.2a) i ich połączenie z narożnikiem
(S2.2b) mają osobne wpisy poniżej. Materiały i modele drzew pozostają
uproszczone. **Nie uznawać wizualnej jakości za odebraną** na podstawie
samej zielonej kompilacji/testów. Szczegółowa kolejność jest w `HANDOFF.md`.

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

**Nieobsługiwane w podetapie S2.2a:** przejścia tuż przed
skrzyżowaniem, gdy brakuje miejsca na rampę przed wlotem
(ich połączenie z narożnikiem dodano w S2.2b poniżej).
Nadal otwarte S2.2c: ciągła faza przez naroże i pełny odbiór wizualny.

Dwa testy `Mesh` kontrolują obniżenia obu stron,
zbieżność z przejściem, nachylenie normalnych,
brak degeneracji siatki, poprawność przyrostowej
przebudowy i odwrócenie po usunięciu przejścia,
oraz brak fałszywych zjazdów dla błędnego zasięgu
i orientacji. Scena kontrolna (niepublikowana
w repo): `archive/visual-regression/s2-midblock-curb-drop.xodr`.
Szczegółowy bieżący stan testów i następne zadanie:
`HANDOFF.md`.

## Implementacja S2.2b — obniżenia połączone z narożnikami (13.11.2026)

- Gdy przejście poprzeczne zaczyna się blisko wlotu, chodnik
  **dochodzi do granicy skrzyżowania na poziomie jezdni**,
  zamiast sztucznie kończyć zjazd na ostatnim metrze drogi.
  Siatka narożnika przyjmuje ten poziom na kapie wlotu
  i wznosi się od niego profilem smoothstep na 3 m.
- Wartości wysokości są wyznaczane z istniejącego
  `JunctionSidewalkBand` i tego samego OpenDRIVE object,
  bez dodawania odrębnej mapy wysokości w pliku.
- Ściany krawężnika korzystają z tej samej funkcji
  wysokości co chodniki, mają odrębne normalne/UV,
  a trójkąty o zerowej wysokości nie są generowane.
  Na pochylonych częściach chodnika normalne są przeliczane.
- `remesh_objects` odświeża zarówno drogi, jak i narożniki,
  a `Document::after_kernel_mutation` przekazuje tym funkcjom
  `studio_mesh_options()` — w GUI również podczas Undo/Redo.
- `raised_sidewalks=false` pozostaje trybem kompatybilnym.
  Podstawowy regresyjny scenariusz X przechodzi test zgodności
  wierzchołków i pełnego/przyrostowego meshu, również po
  usunięciu przejść. Ogólne testy C++: **2059 PASS**,
  testy agenta: **6 PASS**, 10 testów C++ wyłączonych.
- Scena z rzeczywistym przykładowym skrzyżowaniem X:
  `archive/visual-regression/s2.2b-crosswalks-junction.xodr`
  (walidator: 0 błędów), zrzut testowy GUI:
  `archive/visual-regression/s2.2b-gui-window-print.png`
  (pliki lokalne, poza publicznym Git).
- Test Qt w `editor/tests/test_document_commands.cpp` sprawdza
  usuwanie przejścia, Undo i Redo oraz zgodność z pełną siatką
  3D. Błąd Qt zidentyfikowano: testy żądały pluginu
  `offscreen`, a obok edytora wdrożono jedynie `windows`.
  `editor/tests/CMakeLists.txt` kopiuje teraz `qoffscreen.dll`
  automatycznie przed wykrywaniem testów GTest na Windows.
  Testy celowane: **5/5 PASS**. Pełny zestaw Qt:
  **1284 PASS, 1 SKIPPED, 0 FAIL** (exit 0).
  Potwierdzono kompilację i uruchomienie bez ręcznego
  `QT_PLUGIN_PATH`; szczegóły są w `HANDOFF.md`.

**Nadal nieukończone:** fazowany (5 cm) profil przez sam łuk
narożnika, przejścia w nietypowych T/Y i przy zmianach niwelety,
kontrola glTF i wizualny odbiór z wysokości kierowcy. Materiały
nie spełniają jeszcze docelowego standardu realizmu.

## Implementacja S2.2c — fazowane narożniki i kontrola eksportu (13.11.2026)

- W trybie `raised_sidewalks=true` `junction_surface.cpp` wytwarza rzeczywistą
  fazę o nominalnej szerokości 0,05 m między ścianą krawężnika a odsuniętą
  górną krawędzią chodnika. Faza zanika przy wlotach i obniżeniach; dla
  krótkich trójkątów przesunięcie jest ograniczane, aby nie odwracać siatki.
- `gltf_exporter.cpp` zachowuje `TEXCOORD_0` dla podsiatek z pełnymi UV.
  Oryginalne `.xodr` i kompatybilny tryb płaskich chodników nie są zmieniane.
- Skrypt budowania Windows synchronizuje natywny moduł Python agenta z
  aktualną kompilacją i porównuje SHA-256, a Quality Gate odmówi PASS
  przy różnicy. Test eksportu agenta odczytuje rzeczywisty GLB, wymaga
  faz narożników i ich atrybutów UV — eliminuje cichy powrót do geometrii 2D.
- Obrazy rzeczywistych importów do Blender 5.3 scen X/T/Y i zrzut GUI są
  zapisane **lokalnie** w `archive/visual-regression/`. Bryła i ciągłość faz
  na sprawdzonych ujęciach są potwierdzone. Wciąż otwarte: wygładzenie
  widocznej segmentacji łuków, profil wydajnościowy remesh, docelowe
  materiały PBR, skala tekstur, światło i odbiór fotorealizmu.
- Aktualne wyniki testów, SHA obu repozytoriów i dokładny kolejny krok:
  `HANDOFF.md` (jedyne bieżące przekazanie).

## Implementacja S2.2d — shading faz krawężnika i charakterystyka betonu (13.11.2026)

- W `junction_surface.cpp` normalne powierzchni fazy są uśredniane
  deterministycznie, z wagą równą polu trójkąta, między duplikatami
  rzeczywiście wspólnych wierzchołków. Nie są wygładzane krawędzie
  między fazą, pionową ścianą i górną płytą chodnika. Nie zmieniono
  obrysów, liczby trójkątów ani formatu OpenDRIVE.
- Ujednolicono neutralny kolor betonu krawężników w edytorze i eksportach.
  Materiały Curb/Sidewalk/Border mają `roughnessFactor = 0.78`
  w GLB/USD i manifestach; testy sprawdzają zgodność. Eksport nadal
  **nie osadza** map albedo/normal/roughness dla tych materiałów.
- Odrzucono próbę globalnego zwiększenia gęstości łuków, ponieważ
  naruszała jakość siatki ciasnych T oraz golden prostych wlotów.
  Segmentacja sylwetki łuku pozostaje do rozwiązania osobno, bez
  osłabienia kontroli triangulacji.
- Ponowny import X/T/Y w Blenderze 5.3 sprawdził liczbę powierzchni,
  UV, brak zdegenerowanych trójkątów, wysokości i chropowatość.
  Zapisano lokalnie zestaw renderów przed/po i zrzut działającego GUI.
  **PBR, światło i fotorealizm nadal nie są odebrane.**
- Wyniki testów i dokładny następny etap zapisano w `HANDOFF.md`.

## Implementacja S2.2e — GLB PBR i wydajność siatki (13.11.2026)

- GLB w trybie 3D ma osadzone i współdzielone (per rodzina materiałów)
  zestawy trzech istniejących map CC0 Poly Haven: albedo JPEG, normal
  OpenGL PNG, roughness JPEG dla asfaltu i betonu. Tekstury korzystają z
  REPEAT i UV w metrach przeskalowanego do 4 m na powtórzenie.
  Brak źródłowych map w lokalnej instalacji oznacza awaryjny eksport
  bazowych kolorów; ich pakowanie dla wersji instalacyjnej nadal otwarte.
  GLB z obecnego repo jest samowystarczalny.
- core/src/io/export_preview.cpp i testy ExportPreview raportują i
  sprawdzają rzeczywiste obrazy oraz referencje UV. Starsze testy
  test_gltf.cpp odczytują obrazy strukturalnie (bez dekodera w
  tinygltf), podczas gdy prawdziwe dekodowanie wykonano w Blenderze.
  Zweryfikowano w Blender 5.3 sceny X/T/Y i ich trzy obrazy PBR na
  materiałach jezdni oraz chodników/krawężników; lokalny plik
  archive/visual-regression/s2.2e-pbr-x-eevee.png.
- Test PrawkoRemeshBudget powtarza 41 razy remesh_objects na X/T/Y
  z przejściami i wyniesionymi chodnikami oraz porównuje z pełnym
  meshem. Pierwszy pomiar lokalny: X p50=9,03 ms / p95=9,60 ms,
  T 4,94 / 5,42 ms, Y 5,41 / 5,96 ms. Nie obejmuje czasu GPU,
  rysowania i obsługi okna.
- W renderach widać **powtarzalność spękań asfaltu**, a naroża
  mają nadal segmentowy obrys (X ~9 odcinków fazy). Nie
  zagęszczano globalnie siatki ze względu na naruszenie budżetu
  sliverów i goldenów w poprzednim etapie.
- Potwierdzone BUILD_PASS: all, QUALITY_GATE_PASS (2071 C++ i 7
  Python), Qt 1284 PASS / 1 SKIPPED. **Nie ogłaszać końcowego
  fotorealizmu ani bezpieczeństwa globalnej zmiany gęstości łuków.**
  Kolejne działania i szczegóły: HANDOFF.md.

## Implementacja S2.2f — opt-in LOD łuków krawężnika (13.11.2026)

- Bez zmiany parametrów wyjściowej triangulacji CDT i bez zmiany
  OpenDRIVE, dodatkowy etap w render-meshu dzieli bezpieczne odcinki
  zakrzywionej krawędzi Driving/Sidewalk po obu stronach tej samej
  granicy. Korekta położenia punktu ma najwyżej 8 mm,
  a kryterium jakości nowych trójkątów to minimum 8° i >=22%
  poprzedniego pola (bez odwróceń).
- Opcja MeshOptions.curb_render_lod jest domyślnie wyłączona w
  jądrze; tylko GUI PrawkoRoadStudio i agent eksportujący GLB
  świadomie włączają ją razem z raised_sidewalks. Dotychczasowe
  goldeny i kompatybilność domyślnej siatki pozostają bez zmian.
- Pomiary X/T/Y: trójkąty jezdnia + chodniki 676->828,
  469->555, 522->662; liczba sliverów <5° bez wzrostu
  (16/16, 5/5, 18/18). Fazy w prawdziwym GLB X 72->120,
  T 108->174, Y 72->116; Blender 5.3 X/T/Y PASS.
- BUILD_PASS all, 2072/2072 aktywnych testów C++ PASS,
  7/7 testów agenta PASS, Qt 1284 PASS + 1 SKIPPED.
  Rzeczywiste okno GUI otwiera kontrolną scenę X, ale
  **odbiór pełnego realizmu materiałowego i pomiar GPU są otwarte**.
  Szczegółowe logi, ścieżki renderów i dalsze kroki w HANDOFF.md.

## Weryfikacja S2.2g — sprzętowy GPU i wizualny LOD (13.11.2026)

- Rozdzielono CPU meshing, synchroniczny upload geometrii, cold-start
  tekstur PBR, CPU submission, czas GPU GL_TIME_ELAPSED i oczekiwanie
  na zakończenie pracy GPU. Ręczny target `prawko_gpu_probe`
  (EXCLUDE_FROM_ALL) wykorzystuje produkcyjny GLRenderer i te same
  materiały; pomiary na GeForce GTX 1050 Ti w OpenGL 3.3, 1280×720,
  15 rozgrzewkowych + 65 mierzonych klatek na 5 kamer dla X/T/Y.
  Mierzy offscreen, a nie prezentację przez QWidget/kompozytor Windows.
- Mediany GPU (LOD) z góry / ukośnie / kierowca 1,55 m:
  X 0,383 / 0,423 / 0,292 ms;
  T 0,348 / 0,373 / 0,258 ms;
  Y 0,340 / 0,372 / 0,278 ms.
  Powtarzany synchronizowany upload całej siatki: X 2,56,
  T 1,57, Y 1,36 ms. Ten upload obejmuje wywołania sterownika
  i synchronizację, nie sam transfer DMA. Większy LOD nie wykazuje
  znaczącej regresji w tych kontrolnych scenach.
- Rzeczywiste kadry samego GLRenderer, porównane RGB piksel po
  pikselu dla base/LOD: wpływ na obraz subtelny
  (0,0125–0,1518% pikseli przy progu 8/255).
  Na niskich kadrach pozostały ciemne ściany krawężników,
  ostre przejścia cieniowania i kontrast tekstur między drogą
  a płaską nawierzchnią skrzyżowania.
  Nie stwierdzono otwartych dziur w oglądanych kadrach, ale
  nadal BRAK pomiaru realnego FPS, kompozytora, przeciągania
  i dynamicznego z-fightingu. Nie uznawać fotorealizmu za zakończony.
- Pełne logi i 30 zdjęć oraz 15 map różnic pozostają lokalnie:
  `archive/visual-regression/s2.2g-v2/`; procedura i następny
  krok zapisane w HANDOFF.md. QUALITY_GATE_PASS (2072 C++
  i 7 Python), Qt 1284 PASS + 1 SKIPPED, BUILD_PASS all.

## S2.2h — pomiar FPS i spójność nawierzchni (13.11.2026)

- Osobny, manualny `prawko_qt_fps_probe` (`EXCLUDE_FROM_ALL`) testuje
  realne widoczne Qt/Windows z `QOpenGLWidget::frameSwapped`, na GTX 1050 Ti,
  1600×900, po 180 klatek po rozgrzewce. Z symulowanym `Alt+LMB` orbitem:
  X 59,97 FPS / p95 17,35 ms, T 60,01 / 17,30 ms,
  Y 60,00 / 17,29 ms; zero klatek >33,33 ms.
  To nie jest fizyczny pomiar wyświetlacza ani jank przebudowy siatki;
  raportowane `viewport->format().samples() == 0` wymaga sprawdzenia MSAA.
- Qt `build_scene` nadaje teraz metryczne UV world-XY siatkom skrzyżowań,
  betonowym nakładkom i utwardzonym `SurfaceMesh` bez własnych UV.
  Odpowiada to istniejącemu fallbackowi GLB; źródłowa siatka i OpenDRIVE
  pozostają bez zmian, trawa nie otrzymuje zbędnych UV.
- Pionowe powierzchnie betonowe w trybie Textured otrzymują ograniczone
  oświetlenie rozproszone (`Material.vertical_fill=0.38`), z zachowaniem
  domyślnej wartości 0 w innych materiałach. Singularne XY UV na pionowych
  powierzchniach chroni shaderowy warunek wyznacznika normal mappingu.
- W nowym renderze rzeczywistego GUI powierzchnia X ma wzór asfaltu
  również w środku skrzyżowania; zbliżenia GPU X/T/Y pokazują bardziej
  czytelne krawężniki. Nadal widoczne są regularne spękania powtarzanej
  tekstury i ograniczenia materiałowego realizmu; nie uznawać tych
  fragmentów za fotorealistycznie ukończone.
- BUILD_PASS all, Quality Gate C++ 2072 PASS i agent 7 PASS;
  Qt 1286 PASS + 1 SKIPPED. Lokalizacje logów i pierwszy kolejny krok
  w `HANDOFF.md` (S2.2h).

## S2.2i — PBR atlasy asfaltu i betonu (13.11.2026)

- Oryginalne źródła CC0 Poly Haven pozostają nietknięte. Reprodukowalne
  `scripts/build_pbr_atlas.py` wytwarza sześć dopasowanych map albedo,
  normal_gl, roughness w `assets/textures/prawko_atlas/` (2048²).
  Losowanie jest deterministyczne: 4×4 łagodnie łączone przesunięcia
  wzoru z oryginalnych 512²; pełny cykl 16 m zamiast 4 m.
- Qt qrc + `MaterialCatalog` i glTF eksportują dokładnie te same mapy;
  nowe `uv_scale=0.0625` dla standardowego asfaltu i betonu; wariant
  `asphalt_worn` bez zmian. Betonu nie należy mylić z osobnym
  optycznym materiałem czoła krawężnika. OpenDRIVE, CDT, UV źródłowe
  i siatki geometryczne bez zmian.
- Kontrola materiałów: `PBR_ATLAS_QA_PASS`, asset licenses PASS,
  Qt 1287 PASS/1 SKIPPED, C++ 2072 PASS, agent Python 7 PASS.
  Nowe GLB X/T/Y importują się w Blender 5.3 z obrazami 2048²,
  3 kanałami PBR oraz UV i niezerową geometrią faz/czoła.
- Sprzęt GTX 1050 Ti: QOpenGLWidget 1600×900 / Alt+LMB
  X/T/Y ~60 FPS (p95 ~17,11–17,36 ms); GPU GLRenderer p50
  ukośnie ~0,43–0,49 ms. Większy atlas zwiększył wtedy
  koszt zimnego wczytania do 0,55–0,63 s i GLB ~16,7 MB.
  **Zobacz zweryfikowane zmiany S2.2j poniżej.**

## S2.2j — optymalizacja kosztu PBR i pomiar edycji (13.11.2026)

- Parametry UV, topologia, geometria i oryginalne obrazy albedo 2048²
  bez zmian. Normal i roughness 1024², normal ponownie znormalizowany;
  generator deterministyczny, testy map oraz licencji PASS.
  Qt i GLB czytają te same trzy kanały.
- Rozmiary eksportu GLB X/T/Y 5,49 / 5,42 / 5,40 MB (około 67% mniej).
  Cold decode+GPU upload X/T/Y 268 / 273 / 280 ms (poprzednio
  612 / 626 / 551 ms). Około 50% mniej obliczeniowego rozmiaru
  RGBA8 i mipmap, nie jest to pomiar rezydentnej pamięci VRAM.
  30 statycznych porównań obrazu z S2.2i: maksymalna średnia
  różnica 0,1741/255 RGB; żadnych pikseli ze zmianą >16/255.
- MSAA **4 próbki w rzeczywistym FBO**, także bez dodatkowego
  `ViewportWidget::setFormat`: poprzedni odczyt
  `viewport->format().samples()==0` nie potwierdzał braku
  antyaliasingu. Diagnostyka opt-in używa teraz
  `GL_SAMPLE_BUFFERS` i `GL_SAMPLES`; kod produkcyjny bez zmian.
- Osobny widoczny benchmark produkcyjnego `Document` i
  `update_node_drag`/remesh: X/T/Y około 60 FPS, p95 17,23–17,37 ms,
  205–206 sygnałów zmiany siatki na scenę, bez przerw >33 ms.
  Sterowanie ruchem węzła odbywało się z timera, nie fizycznej myszy.
  Otwartej niezapisanej sceny użytkownika nie dotykano.
- Blender 5.3 import X/T/Y PASS, oświetlenie referencyjne SUN +
  ambient w trybie Standard; nie ma pełnego fizycznego parytetu
  kolorów i oświetlenia z Qt. Fotorealizm pozostaje OTWARTY.
  Szczegółowy raport wyników, ścieżki logów i następny krok
  znajdują się w `HANDOFF.md`.

## S2.2k — biblioteka gotowych elementów drogowych (13.11.2026)

Aby pracować myszą, podczas zwykłego uruchamiania edytora otwiera
się po lewej stronie zakładka **Library**, w której aktywna jest
kategoria **Prawko | Gotowe drogi**. Przeciągnięcie szablonu
uruchamia rysowanie nowej drogi — należy dodać jej punkty, a następnie
zatwierdzić standardową operacją Create Road. Aby przebudować już
istniejącą drogę, wybierz kategorię **Prawko | Krawężniki i chodniki**
i upuść styl na jej jezdni; cofnięcie działa przez `Ctrl+Z`.

| Identyfikator wariantu | Geometria nowej drogi/stylu |
|---|---|
| `prawko.urban` | Obustronne chodniki 1,8 m; krawężniki 15 cm |
| `prawko.wide` | Obustronne chodniki 2,5 m; krawężniki 15 cm |
| `prawko.promenade` | Obustronne chodniki 3,0 m; krawężniki 15 cm |
| `prawko.one_side` | Lewy chodnik 2,5 m, prawy bez chodnika |
| `prawko.parking` | Obie strony: pas postojowy 2,4 m, chodnik 2,0 m |
| `prawko.no_curb` | Dwa pasy jazdy bez chodników i krawężników |

Każdy wariant ma parę pozycji w bibliotece: `road.prawko.*`
i `style.prawko.*`, razem 12 miniaturek. Używają istniejącej
siatki jezdni, `Sidewalk` i generowanych ścian/faz krawężników.
Nie są to wolne obiekty 3D ani prefabrykaty krawężnika
najazdowego. Zachowany jest OpenDRIVE, a styl jest zwykłą
operacją `apply_road_style` obsługiwaną przez Undo/Redo.

Pierwszy zwykły start po aktualizacji jednorazowo aktywuje
`Textured Rendering` (PBR) i pamięta wykonaną migrację;
później świadoma zmiana przez użytkownika pozostaje zapisana.
Nie zmienia to dokumentów użytkownika ani istniejącej sceny.
Istniejący skrót `C:\Users\tomas\Desktop\PrawkoRoadStudio.lnk`
odwołuje się do skompilowanego programu
`F:\PrawkoRoadStudio\build\dev-windows\editor\roadmaker-editor.exe`.

Weryfikacja S2.2k: BUILD_PASS all, 1292 PASS + 1 SKIPPED
w testach Qt, Quality Gate PASS, licencje PASS, render w działającym
Windows Qt oraz testy parametrów geometrycznych, eksportu XODR,
przeciągania stylu i cofania. Testuje się dane i rzeczywistą
geometrię, nie tylko występowanie miniaturki.

Potwierdzono **normalny start z istniejącego skrótu pulpitu** na Windows:
`archive/visual-regression/s2.2k-real-launch-desktop.png` pokazuje
kategorię `Prawko | Gotowe drogi` i sześć miniatur już po uruchomieniu
aplikacji z istniejącą sceną użytkownika, przy włączonych teksturach.
Scena pozostała niezmieniona (SHA256 zgodny z kontrolą sprzed startu).
Program pozostawiono otwarty, gotowy do używania myszą.

## Obsługa biblioteki myszą — S2.2l

Zwykłe pojedyncze kliknięcie kafelka wybiera zasób, pokazując
pod listą kategorii krótką instrukcję jego zastosowania.
**Przeciągnięcie kafelka na scenę** wyświetla znacznik położenia
i opis planowanej operacji przy znaczniku: pozycja i docelowy
element są obliczane tym samym resolverem co operacja zatwierdzająca.
Punkt nieprawidłowy jest oznaczony kolorem ostrzegawczym.

- **Nowa droga**: przeciągnij szablon na teren i klikaj kolejne
  punkty; Enter kończy rysowanie, Esc anuluje.
- **Zmiana istniejącej drogi**: przeciągnij styl na jej jezdnię.
  Po udanej zmianie droga jest automatycznie zaznaczona, a panel
  Properties otwarty do dalszej edycji. Alternatywnie zaznacz
  dokładnie jedną drogę i **kliknij dwukrotnie** kafelek jej stylu.
- **Materiał i oznaczenia**: przeciągnij na odpowiedni pas jezdni.
  Przed zwolnieniem przycisku sprawdź podgląd docelowego miejsca.
  Zmiany zatwierdzane przez system komend można cofnąć przez Ctrl+Z.

Wdrożenie tej obsługi nie tworzy jeszcze samodzielnych prefabrykatów
obniżonych krawężników, nie wprowadza swobodnego przesuwania wszystkich
obiektów po stworzeniu i nie stanowi odbioru końcowego realizmu.
Test GUI rzeczywistym kursorem Windows wykonano na osobnej kopii
sceny, nie na oryginalnym projekcie.

## Edycja bezpośrednio po upuszczeniu — S2.2m

Po przeciągnięciu znaku, sygnalizatora, drzewa, prefabrykatu
obiektowego, oznakowania strzałką, przejścia albo skrzyżowania
na prawidłowe miejsce, edytor wybiera właśnie **nowo dodane**
elementy i pokazuje panel Properties. Można w nim zmienić
pozycję i wysokość sygnału, właściwości znaku, albo pozycję,
obrót i rozmiar pojedynczego modelu 3D, stosownie do jego
istniejących kontrolek. Zbiorczy prefabrykat może zaznaczyć
więcej niż jeden nowy obiekt. Po nałożeniu materiału lub
oznakowania drogowego zaznaczany jest dokładny pas drogowy,
nie cała droga.

Nie trzeba ponownie klikać małego znaku lub drzewa, aby
uzyskać dostęp do edycji. Cofanie i ponawianie działa przez
istniejący mechanizm komend i nie zmienia schematu OpenDRIVE.
Niepoprawny upust, np. materiału poza pasem, jest odrzucany
z wyjaśnieniem.

Nie ma jeszcze obsługi swobodnego przesuwania znaków
i sygnalizatorów myszą w 3D; odpowiednie parametry można
zmieniać w Properties. Ręczny test upuszczenia przejścia
z powiązaniem stop-line na rzeczywistym wlocie jest nadal
otwarty — istnieją tylko testy logiki C++. Brak też swobodnych
transformacji całych złożonych prefabrykatów ani niezależnej
od drogi lokalizacji wszystkich obiektów. Te funkcje
nie mogą być nazywane ukończonymi. Realizm wizualny ma
osobne kryteria odbioru.

## S2.2n — przesuwanie znaków, sygnalizatorów i grup obiektów

W trybie **Select** lub **Move** możesz chwycić na scenie znak albo
sygnalizator i przeciągnąć go w nowe miejsce w pobliżu jego drogi.
Po zwolnieniu myszy pozycja zapisuje się jako jedna operacja
`Ctrl+Z`. Wyjście poza dopuszczalny obszar zatrzymuje podgląd,
powrót wznawia ruch, `Esc` anuluje. Ruch `s/t` nie zmienia
kierunku, którego dotyczy znak (`orientation`), ani jego ustawionego
kąta wizualnego (`hOffset`). Aby **obrócić** znak, zaznacz go,
włącz **Move** i chwyć **podświetlony okrągły pierścień**;
obrót zapisuje `hOffset` bez zmiany `orientation`.
Ten gest zweryfikowano na rzeczywistym oknie Qt Windows.

Kilka wybranych obiektów 3D na **jednej drodze** można przesunąć
razem: zaznacz ich zbiór, chwyć dowolną wybraną część i przeciągnij.
Grupa zachowuje odległości w osi `s/t`, pod warunkiem że cały
prefabrykat mieści się w dozwolonym zasięgu jednej drogi.
Undo/Redo obejmuje całą grupę. Wariant dla różnych dróg,
wspólny obrót grupy i swobodny ruch poza drogą nie są jeszcze
dostępne.

Na testowej scenie T **bez wcześniejszych zebr** przeciągnięcie
`Zebra crosswalk` na wlot utworzyło jedno przejście i powiązaną
linię zatrzymania. Upust poza wlotem został odrzucony.
Pliki kontrolne, screenshoty, testy i ograniczenia opisano
w `HANDOFF.md`; nie są dowodem fotorealizmu.
