# Autorskie sceny PrawkoNaRaz

Pliki OpenDRIVE i OpenSCENARIO przygotowane 15 listopada 2026 r. od podstaw (Apache-2.0).

| Scena | Warunki | Zdarzenie |
| --- | --- | --- |
| 01-miasto-lusterka | dzien/lato | Samochod z przodu i z tylu, poruszajacy sie Ego |
| 02-przejscie-pieszy | zmierzch/jesien | Pieszy wchodzi na pasy, Ego hamuje |
| 03-deszcz-luk | deszcz/jesien | Luk drogi, AutoPrzed hamuje, Ego reaguje |
| 04-zima-mgla | mgla/zima | Wolniejsza jazda i ograniczona widocznosc |
| 05-wiata-test | dzien/lato | Osobna scena do testu GLB i materialow realistycznej wiaty przystankowej |
| 07-pokaz-biblioteki-assetow | dzien/deszcz | Scena pokazowa z 19 rodzajami modeli GLB: 42 obiekty i 3 uczestnicy ruchu |

**Scena pokazowa 07 (17.11.2026):** to kolejna scena w TYM SAMYM projekcie, nie drugi projekt. Korzysta z geometrii i scenariusza 06l, zachowując wszystkie poprzednie pliki. Pierwotnie projektowy `assets/library/manifest.json` zawierał 19 modeli z lokalnymi plikami GLB i miniaturami. Po migracji wszystkie 19 modeli i miniatury ma jedna globalna biblioteka `F:\PrawkoRoadStudio\assets`, a projektowy manifest nie zawiera odrębnych wpisów tych modeli. Zachowano stare lokalne pliki jedynie dla zgodności wstecznej. Nowy model bariery pochodzi z autorskiego pliku `assets/prawko/blender/prawko_bariera_drogowa.glb`. Po otwarciu 07 w aktualnym edytorze panel Library powinien wskazać `Prawko | Obiekty 3D`, skąd można przeciągać modele na drogę i edytować ich położenie/obrót. Jedyny skrót na pulpicie `PrawkoRoadStudio` otwiera główny edytor z ekranem WYBORU projektu (bez autowczytywania). Zaznacz `Prawko-Scenariusze` i kliknij `Otwórz wybrany`, następnie edytuj scenę 07 lub inną. Jedyna wspólna biblioteka 12 elementów drogowych + 19 modeli GLB znajduje się w `F:\PrawkoRoadStudio\assets` i jest dostępna dla każdego projektu. Projektowy manifest po migracji nie zawiera własnych wpisów tych 19 modeli; dawne kopie plików pozostają dla zgodności. Katalogu scen nie kopiować do `archive/render-*`.

Scena 05-wiata-test powstala osobno na potrzeby etapu Prawko Assets A1. Model GLB i miniatura znajduja sie w assets/props i assets/library. Cztery pierwotne scenariusze pozostaja bez zmian, a generator scripts/create-prawko-scenarios.cjs odtwarza tylko pierwotny pakiet czterech. Wiata jest przetestowana w dzien, deszcz i noc (noc bez realistycznego oswietlenia), ale pelne tekstury PBR/UV sa nadal otwarte.

Otworz plik .xodr w edytorze, a w panelu Symulacja uruchom przejazd.
Odpowiadajace mu pliki .xosc i .rmscene.json zostana automatycznie odczytane.
W dniu 15.11.2026 wszystkie cztery sceny uruchomiono w prawdziwym esmini 3.11.0:
zapis ruchu Ego i pozostałych aktorów, zdarzenia hamowania oraz przechodzenia
pieszego wykonano i sprawdzono bez raportów niezamierzonych kolizji.

Potwierdzone filmy z pierwszoosobowej kamery PrawkoRoadStudio, kokpitem,
trzema lusterkami i animowaną pogodą są w lokalnym folderze (poza projektem):
`archive/scene-replacement-20261115/{nazwa-sceny}-HD.mp4`.
Każdy film ma 1280 × 720, 24 kl./s. Czas: 13 / 11 / 13 / 12 s.
Filmy nie są częścią repozytorium Git ze względu na rozmiar.

Nie jest to jeszcze fotorealistyczne otoczenie ani zatwierdzony materiał
egzaminacyjny. Autor scen: PrawkoNaRaz. Reprodukcja od zera w pustym katalogu:
`node scripts/create-prawko-scenarios.cjs` — skrypt odmawia nadpisania scen.
