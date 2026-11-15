# Autorskie sceny PrawkoNaRaz

Pliki OpenDRIVE i OpenSCENARIO przygotowane 15 listopada 2026 r. od podstaw (Apache-2.0).

| Scena | Warunki | Zdarzenie |
| --- | --- | --- |
| 01-miasto-lusterka | dzien/lato | Samochod z przodu i z tylu, poruszajacy sie Ego |
| 02-przejscie-pieszy | zmierzch/jesien | Pieszy wchodzi na pasy, Ego hamuje |
| 03-deszcz-luk | deszcz/jesien | Luk drogi, AutoPrzed hamuje, Ego reaguje |
| 04-zima-mgla | mgla/zima | Wolniejsza jazda i ograniczona widocznosc |

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
