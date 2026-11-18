# PrawkoRoadStudio — rejestr produkcji polskich znaków drogowych 3D

**Aktualizacja:** 18 listopada 2026 · **Gałąź:** `feature/realistic-road-surfaces` · **Status:** pierwsze cztery modele wykonane i zarejestrowane w bibliotece; odbiór prawny i pełny GUI nadal otwarte.

Ten plik jest **jedynym rejestrem statusu produkcji polskich znaków** dla wspólnej biblioteki PrawkoRoadStudio. Nie oznaczaj znaku jako gotowego na podstawie samego skryptu Blendera, wyrenderowanej grafiki, modelu amerykańskiego lub podobnego symbolu. Każdy znak musi być polski, trójwymiarowy, czytelny i umieszczalny myszą.

## Stan liczbowy

| Etap | Liczba |
| --- | ---: |
| Zweryfikowane znaki polskie (ukończone) | **0** |
| W produkcji | **4** |
| Plan pierwszej partii | **4** |
| Pozostałe w tym rejestrze | **173** |
| Razem wpisów A–D w rejestrze | **177** |

**Nie mylić ze stanem całej biblioteki:** są w niej inne autorskie GLB (np. wiata, ławka, sygnalizator świetlny), lecz nie są to kompletne polskie *pionowe znaki drogowe A–D*. Po etapie P0 w `assets/props/` i `assets/library/manifest.json` są **4 nowe GLB polskich znaków**, lecz jeszcze żaden nie uzyskał końcowego statusu ✅ Gotowy. Kryterium obejmuje również ocenę zgodności lica z aktualnym wzorem i Windows GUI. Nie licz starych symboli US ze starych fixture'ów do gotowych polskich znaków.

## Zasada statusów i odbioru

| Status | Warunek |
| --- | --- |
| ⬜ Do wykonania | Nie ma zaakceptowanego modelu 3D — pozycja na liście. |
| 🟡 W Blenderze | Powstała geometria tarczy, słupka, obrzeży i mocowań z rzeczywistymi wymiarami; roboczy `.blend`. |
| 🔵 Wyeksportowano | Prawdziwy poprawny GLB z materiałami i licem zweryfikowany w Blenderze. |
| 🟣 W bibliotece | Model z miniaturą w `assets/props/` i wspólnym `assets/library/manifest.json`; walidator PASS. |
| ✅ Gotowy | Znak poprawny według polskiego wzorca prawnego, dodany przeciągnięciem w Windows GUI, widoczny, wybierany, przesuwany, obracany; testy i commit/push PASS. |
| ⚠️ Zablokowany | Wzór, materiał, skala, prawa do grafiki albo operacje GUI wymagają poprawy; opisz blokadę. |

**Checkbox [x] zaznaczaj WYŁĄCZNIE na etapie ✅ Gotowy.** Gdy znak jest częściowo ukończony, dopisz przy nim bieżący etap, identyfikator commit i dowód testu; nie zaznaczaj [x]. Po każdym etapie aktualizuj licznik na podstawie pozycji, bez ręcznego udawania postępu. Dla B-33 rozdzielaj warianty liczbowe (pierwszy: 50 km/h). Przed seryjnym modelowaniem zweryfikuj geometrię i grafikę lica w aktualnych polskich aktach, bo wzory i wymiary mogły ulec zmianie w 2026 r.

## Pierwsza partia — najważniejsze na egzamin i do scen drogowych

| Kod | Nazwa i wariant | Blender | GLB | Biblioteka | Windows GUI | Status |
| --- | --- | --- | --- | --- | --- | --- |
| **A-7** | Ustąp pierwszeństwa | PASS | PASS | PASS | do odbioru | 🟣 W bibliotece |
| **B-20** | STOP | PASS | PASS | PASS | do odbioru | 🟣 W bibliotece |
| **D-6** | Przejście dla pieszych | PASS | PASS | PASS | do odbioru | 🟣 W bibliotece |
| **B-33** | Ograniczenie prędkości — 50 km/h | PASS | PASS | PASS | do odbioru | 🟣 W bibliotece |


### P0 / R7b — dowód wykonania części technicznej (18.11.2026)

**Wykonano fizycznie w lokalnym Blenderze 5.3.0:** 4 modele z autorską geometrią i materiałami PBR, osobnymi tarczami 3D, licem, stalowym słupkiem, podstawą, uchwytami i śrubami. Eksport GLB walidowany pod kątem realnych meshów i materiałów; pliki .blend, rendery, JSON metadanych, GLB i miniatury są w repo. Nie są to płaskie zdjęcia ani amerykańskie znaki z odzyskanego zestawu.

| Znak | Wspólny model GLB | Źródło Blender (.blend) |
| --- | --- | --- |
| A-7 | `assets/props/pl-a7-ustap-pierwszenstwa.glb` | `assets/prawko/blender/polskie_znaki_p0/pl-a7-ustap-pierwszenstwa.blend` |
| B-20 | `assets/props/pl-b20-stop.glb` | `assets/prawko/blender/polskie_znaki_p0/pl-b20-stop.blend` |
| D-6 | `assets/props/pl-d6-przejscie-pieszych.glb` | `assets/prawko/blender/polskie_znaki_p0/pl-d6-przejscie-pieszych.blend` |
| B-33 (50) | `assets/props/pl-b33-50.glb` | `assets/prawko/blender/polskie_znaki_p0/pl-b33-50.blend` |

**Potwierdzone:** Blender run 44405740700 PASS; strzeżony installer run 44406072029 PASS; walidator biblioteki run 44406199218 PASS; native Quality Gate run 44406214910 PASS. Kategoria w GUI: `Prawko | Polskie znaki 3D` (osobna od innych obiektów 3D). Test regresyjny manifestu poprawiony o osobną kategorię polskich znaków; rebuild Qt run 44407223608 PASS i pełne Qt run 44407670488 PASS (pierwsza próba Qt wykryła stary test kategorii, naprawiony i ponownie sprawdzony). Odbiór w żywym GUI nadal PENDING: aplikacja została uruchomiona PID 21492 (run 44407943539), ale Windows zgłosił nieważne wymiary okna w runach 44408427582 i 44408590308, co blokuje strzeżone akcje myszy; nic nie zapisano w scenie.

**NIE ukończono odbioru prawnego:** przy odczycie podglądów D-6 ma uproszczoną sylwetkę pieszego; wszystkie wzory wymagają porównania z aktualnymi tablicami technicznymi, szczególnie szerokości obwódek, barw folii i liter. Materiały zastosowano jako aproksymację PBR, nie gwarantują fizycznej retrorefleksyjności. Dopiero po tej kontroli i po teście myszą (upuszczenie, przesunięcie, obrót) wolno oznaczyć ✅ i zaznaczyć odpowiednie checkboxy.

**Następne po pierwszej partii:** D-1, C-12, B-2, B-36, A-17, A-16, B-21/B-22, D-40/D-41, D-42/D-43; są już uwzględnione w szczegółowej kolejce poniżej. Przy projektowaniu wersji z blachy i folii zadbaj o krawędź tarczy, tył, uchwyty, słupek, prawidłowy kształt lica, odblaskowy wygląd materiału oraz proporcje. Nie generuj piktogramów na oko jako zgodnych z prawem.

## Szczegółowa kolejka A–D (do wykonania)

Poniżej znajduje się lista robocza kodów i nazw, **nie zaświadczenie o kompletnym audycie wszystkich obowiązujących wariantów prawnych**. Niektóre znaki mają warianty rozmiaru, kierunku, liczby lub wartości, których nie wolno błędnie zliczać jako jeden zweryfikowany GLB.

### A — Ostrzegawcze (42)

- [ ] **A-1** — Niebezpieczny zakręt w prawo
- [ ] **A-2** — Niebezpieczny zakręt w lewo
- [ ] **A-3** — Niebezpieczne zakręty — pierwszy w prawo
- [ ] **A-4** — Niebezpieczne zakręty — pierwszy w lewo
- [ ] **A-5** — Skrzyżowanie dróg
- [ ] **A-6a** — Skrzyżowanie z drogą podporządkowaną po obu stronach
- [ ] **A-6b** — Droga podporządkowana z prawej
- [ ] **A-6c** — Droga podporządkowana z lewej
- [ ] **A-6d** — Wlot drogi jednokierunkowej z prawej
- [ ] **A-6e** — Wlot drogi jednokierunkowej z lewej
- [ ] **A-8** — Skrzyżowanie o ruchu okrężnym
- [ ] **A-9** — Przejazd kolejowy z zaporami
- [ ] **A-10** — Przejazd kolejowy bez zapór
- [ ] **A-11** — Nierówna droga
- [ ] **A-11a** — Próg zwalniający
- [ ] **A-12a** — Zwężenie jezdni — dwustronne
- [ ] **A-12b** — Zwężenie jezdni — prawostronne
- [ ] **A-12c** — Zwężenie jezdni — lewostronne
- [ ] **A-13** — Ruchomy most
- [ ] **A-14** — Roboty na drodze
- [ ] **A-15** — Śliska jezdnia
- [ ] **A-16** — Przejście dla pieszych
- [ ] **A-17** — Dzieci
- [ ] **A-18a** — Zwierzęta gospodarskie
- [ ] **A-18b** — Zwierzęta dzikie
- [ ] **A-19** — Boczny wiatr
- [ ] **A-20** — Odcinek jezdni o ruchu dwukierunkowym
- [ ] **A-21** — Tramwaj
- [ ] **A-22** — Niebezpieczny zjazd
- [ ] **A-23** — Stromy podjazd
- [ ] **A-24** — Rowerzyści
- [ ] **A-25** — Spadające odłamki skalne
- [ ] **A-26** — Lotnisko
- [ ] **A-27** — Nabrzeże lub brzeg rzeki
- [ ] **A-28** — Sypki żwir
- [ ] **A-29** — Sygnały świetlne
- [ ] **A-30** — Inne niebezpieczeństwo
- [ ] **A-31** — Niebezpieczne pobocze
- [ ] **A-32** — Oszronienie jezdni
- [ ] **A-33** — Zator drogowy
- [ ] **A-34** — Wypadek drogowy

### B — Zakazu (46)

- [ ] **B-1** — Zakaz ruchu w obu kierunkach
- [ ] **B-2** — Zakaz wjazdu
- [ ] **B-3** — Zakaz wjazdu pojazdów silnikowych
- [ ] **B-3a** — Zakaz wjazdu autobusów
- [ ] **B-4** — Zakaz wjazdu motocykli
- [ ] **B-5** — Zakaz wjazdu samochodów ciężarowych
- [ ] **B-6** — Zakaz wjazdu ciągników rolniczych
- [ ] **B-7** — Zakaz wjazdu pojazdów silnikowych z przyczepą
- [ ] **B-8** — Zakaz wjazdu pojazdów zaprzęgowych
- [ ] **B-9** — Zakaz wjazdu rowerów
- [ ] **B-10** — Zakaz wjazdu motorowerów
- [ ] **B-11** — Zakaz wjazdu wózków rowerowych
- [ ] **B-12** — Zakaz wjazdu wózków ręcznych
- [ ] **B-13** — Zakaz wjazdu pojazdów z materiałami wybuchowymi lub łatwopalnymi
- [ ] **B-13a** — Zakaz wjazdu pojazdów z towarami niebezpiecznymi
- [ ] **B-14** — Zakaz wjazdu pojazdów z towarami mogącymi skazić wodę
- [ ] **B-15** — Zakaz wjazdu pojazdów o szerokości ponad ... m
- [ ] **B-16** — Zakaz wjazdu pojazdów o wysokości ponad ... m
- [ ] **B-17** — Zakaz wjazdu pojazdów o długości ponad ... m
- [ ] **B-18** — Zakaz wjazdu pojazdów o rzeczywistej masie całkowitej ponad ... t
- [ ] **B-19** — Zakaz wjazdu pojazdów o nacisku osi ponad ... t
- [ ] **B-21** — Zakaz skręcania w lewo
- [ ] **B-22** — Zakaz skręcania w prawo
- [ ] **B-23** — Zakaz zawracania
- [ ] **B-24** — Koniec zakazu zawracania
- [ ] **B-25** — Zakaz wyprzedzania
- [ ] **B-26** — Zakaz wyprzedzania przez samochody ciężarowe
- [ ] **B-27** — Koniec zakazu wyprzedzania
- [ ] **B-28** — Koniec zakazu wyprzedzania przez samochody ciężarowe
- [ ] **B-29** — Zakaz używania sygnałów dźwiękowych
- [ ] **B-30** — Koniec zakazu używania sygnałów dźwiękowych
- [ ] **B-31** — Pierwszeństwo dla nadjeżdżających z przeciwka
- [ ] **B-32** — Stój — kontrola
- [ ] **B-34** — Koniec ograniczenia prędkości
- [ ] **B-35** — Zakaz postoju
- [ ] **B-36** — Zakaz zatrzymywania się
- [ ] **B-37** — Zakaz postoju w dni nieparzyste
- [ ] **B-38** — Zakaz postoju w dni parzyste
- [ ] **B-39** — Strefa ograniczonego postoju
- [ ] **B-40** — Koniec strefy ograniczonego postoju
- [ ] **B-41** — Zakaz ruchu pieszych
- [ ] **B-42** — Koniec zakazów
- [ ] **B-43** — Strefa ograniczonej prędkości
- [ ] **B-44** — Koniec strefy ograniczonej prędkości

### C — Nakazu (19)

- [ ] **C-1** — Nakaz jazdy w prawo przed znakiem
- [ ] **C-2** — Nakaz jazdy w lewo przed znakiem
- [ ] **C-3** — Nakaz jazdy w prawo za znakiem
- [ ] **C-4** — Nakaz jazdy w lewo za znakiem
- [ ] **C-5** — Nakaz jazdy prosto
- [ ] **C-6** — Nakaz jazdy prosto lub w prawo
- [ ] **C-7** — Nakaz jazdy prosto lub w lewo
- [ ] **C-8** — Nakaz jazdy w prawo lub w lewo
- [ ] **C-9** — Nakaz jazdy z prawej strony znaku
- [ ] **C-10** — Nakaz jazdy z lewej strony znaku
- [ ] **C-11** — Nakaz jazdy z prawej lub lewej strony znaku
- [ ] **C-12** — Ruch okrężny
- [ ] **C-13** — Droga dla rowerów
- [ ] **C-13a** — Koniec drogi dla rowerów
- [ ] **C-14** — Prędkość minimalna
- [ ] **C-15** — Koniec prędkości minimalnej
- [ ] **C-16** — Droga dla pieszych
- [ ] **C-16a** — Koniec drogi dla pieszych
- [ ] **C-17** — Nakazany kierunek jazdy dla pojazdów z towarami niebezpiecznymi

### D — Informacyjne (70)

- [ ] **D-1** — Droga z pierwszeństwem
- [ ] **D-2** — Koniec drogi z pierwszeństwem
- [ ] **D-3** — Droga jednokierunkowa
- [ ] **D-4a** — Droga bez przejazdu
- [ ] **D-4b** — Wjazd na drogę bez przejazdu
- [ ] **D-5** — Pierwszeństwo na zwężonym odcinku jezdni
- [ ] **D-6a** — Przejazd dla rowerzystów
- [ ] **D-6b** — Przejście dla pieszych i przejazd dla rowerzystów
- [ ] **D-7** — Droga ekspresowa
- [ ] **D-8** — Koniec drogi ekspresowej
- [ ] **D-9** — Autostrada
- [ ] **D-10** — Koniec autostrady
- [ ] **D-11** — Początek pasa ruchu dla autobusów
- [ ] **D-12** — Pas ruchu dla autobusów
- [ ] **D-13** — Początek pasa ruchu powolnego
- [ ] **D-14** — Koniec pasa ruchu
- [ ] **D-15** — Przystanek autobusowy
- [ ] **D-16** — Przystanek trolejbusowy
- [ ] **D-17** — Przystanek tramwajowy
- [ ] **D-18** — Parking
- [ ] **D-18a** — Parking — miejsce zastrzeżone
- [ ] **D-18b** — Parking zadaszony
- [ ] **D-19** — Postój taksówek
- [ ] **D-20** — Koniec postoju taksówek
- [ ] **D-21** — Szpital
- [ ] **D-21a** — Policja
- [ ] **D-22** — Punkt opatrunkowy
- [ ] **D-23** — Stacja paliwowa
- [ ] **D-23a** — Stacja paliwowa z gazem do napędu pojazdów
- [ ] **D-23b** — Stacja paliwowa z punktem ładowania pojazdów
- [ ] **D-23c** — Punkt ładowania pojazdów elektrycznych lub hybrydowych
- [ ] **D-24** — Telefon
- [ ] **D-25** — Poczta
- [ ] **D-26** — Stacja obsługi technicznej
- [ ] **D-26a** — Wulkanizacja
- [ ] **D-26b** — Myjnia
- [ ] **D-26c** — Toaleta
- [ ] **D-27** — Bufet lub kawiarnia
- [ ] **D-28** — Restauracja
- [ ] **D-29** — Hotel / motel
- [ ] **D-30** — Obozowisko (kemping)
- [ ] **D-31** — Obozowisko wyposażone w podłączenia elektryczne
- [ ] **D-32** — Pole biwakowe
- [ ] **D-33** — Schronisko młodzieżowe
- [ ] **D-34** — Punkt informacji turystycznej
- [ ] **D-34a** — Informacja radiowa o ruchu drogowym
- [ ] **D-35** — Przejście podziemne dla pieszych
- [ ] **D-36** — Przejście nadziemne dla pieszych
- [ ] **D-37** — Tunel
- [ ] **D-38** — Koniec tunelu
- [ ] **D-39** — Dopuszczalne prędkości
- [ ] **D-40** — Strefa zamieszkania
- [ ] **D-41** — Koniec strefy zamieszkania
- [ ] **D-42** — Obszar zabudowany
- [ ] **D-43** — Koniec obszaru zabudowanego
- [ ] **D-44** — Strefa płatnego parkowania
- [ ] **D-45** — Koniec strefy płatnego parkowania
- [ ] **D-46** — Droga wewnętrzna
- [ ] **D-47** — Koniec drogi wewnętrznej
- [ ] **D-48** — Zmiana pierwszeństwa
- [ ] **D-49** — Pobór opłat
- [ ] **D-50** — Zatoka
- [ ] **D-51** — Automatyczna kontrola prędkości
- [ ] **D-51a** — Automatyczna kontrola średniej prędkości
- [ ] **D-51b** — Koniec automatycznej kontroli średniej prędkości
- [ ] **D-52** — Strefa ruchu
- [ ] **D-53** — Koniec strefy ruchu
- [ ] **D-54** — Strefa czystego transportu
- [ ] **D-55** — Koniec strefy czystego transportu

## Kolejne rodziny — audyt kodów i wariantów przed wyliczaniem sztuk

- [ ] **E** — znaki kierunku i miejscowości (drogowskazy, numeracja dróg, tablice przeddrogowskazowe i szlakowe; treść indywidualna, nie zakładać jednego gotowego wzorca).
- [ ] **F** — znaki uzupełniające (kierunki, organizacja pasów, przebieg drogi).
- [ ] **G** — dodatkowe znaki przed przejazdami kolejowymi (m.in. słupki wskaźnikowe i krzyże św. Andrzeja).
- [ ] **T** — tabliczki pod znakami (odległości, długości odcinków, rodzaj pojazdu, przebieg pierwszeństwa; warianty liczbowe).
- [ ] **P** — oznakowanie poziome jezdni (osobna biblioteka nawierzchni/dekali, nie fizyczna tarcza).
- [ ] **R oraz pozostałe** — oznaczenia szlaków rowerowych, sygnalizacja specjalna, nowe lub zmienione wzory; najpierw audyt oficjalnego wykazu.

Nie dodawaj do liczby `177` pozycji z rodzin E/F/G/T/P, dopóki nie powstanie lista rzeczywistych wariantów i numerów zgodna z aktualnymi przepisami.

## Źródła wzorców (podstawa do przyszłego audytu)

- [Ministerstwo Infrastruktury — Znaki i sygnały drogowe](https://www.gov.pl/web/infrastruktura/znaki-i-sygnaly-drogowe).
- [Dz.U. 2019 poz. 2310 — tekst jednolity rozporządzenia o znakach i sygnałach](https://eli.gov.pl/api/acts/DU/2019/2310/text.html), wraz z późniejszymi zmianami.
- [Dz.U. 2019 poz. 2311 — warunki techniczne i wzory](https://eli.gov.pl/eli/DU/2019/2311/ogl), wraz z późniejszymi zmianami.
- [Dz.U. 2026 poz. 132 — zmiana wymagań technicznych](https://eli.gov.pl/eli/DU/2026/132/ogl).
- [Dz.U. 2026 poz. 133 — zmiana znaków i sygnałów](https://eli.gov.pl/eli/DU/2026/133/ogl), obowiązująca od 19.02.2026; uwzględniać m.in. D-23c.

## Lokalizacja plików i zasada pracy

- Jedyna biblioteka: `F:\PrawkoRoadStudio\assets\library\manifest.json`.
- Gotowe eksporty GLB: `F:\PrawkoRoadStudio\assets\props\`.
- Źródło modeli: `F:\PrawkoRoadStudio\assets\prawko\blender\polskie_znaki\` (tworzyć dopiero przy pierwszym modelu; nie duplikować bibliotek).
- Miniatury: `F:\PrawkoRoadStudio\assets\library\thumbnails\`.
- W `HANDOFF.md` zamieszczaj tylko aktualny etap i link do **TEGO** rejestru. Nie twórz kolejnego niezależnego zestawienia.
- Dla każdej ukończonej pozycji zapisz ścieżkę źródła `.blend`, identyfikator GLB w manifeście, commit SHA i run testowy Windows. Gdy test nie przeszedł, status pozostaje niegotowy.
