# PrawkoRoadStudio jako studio scenariuszy drogowych

**Cel PrawkoNaRaz, 13.11.2026.** Osoba prowadząca naukę jazdy ma samodzielnie przygotować, zmienić, obejrzeć i nagrać sytuację drogową w jednej aplikacji Windows. Rozwój funkcji ma pierwszeństwo przed wymianą modeli i tekstur. Materiały dla kursantów mogą być nieruchomymi ilustracjami albo filmami. Scena źródłowa pozostaje edytowalna po wyprodukowaniu materiału.

## Zakres porównania z VICOM

Macierz obejmuje funkcje pokazane publicznie przez producenta. Oś czasu, przyciski odtwarzania i konkretne formaty poniżej są propozycją obsługi w PrawkoRoadStudio. [Opis pracy](https://vicomeditor.de/#how_it_works) · [Funkcje i przykłady](https://vicomeditor.de/#features)

| Funkcja VICOM | Punkt wyjścia PrawkoRoadStudio | Odbiór funkcjonalny |
| --- | --- | --- |
| Modułowe szablony miasta, terenu pozamiejskiego i autostrady | Są style dróg, gotowe skrzyżowania i przykładowe mapy. | **Nowy scenariusz** otwiera wybrany szablon jako niezależną kopię z gotowymi pasami i punktami startowymi. |
| Swobodne i wspomagane umieszczanie pojazdów, pieszych, rowerzystów i zwierząt | Model OpenSCENARIO, narzędzie Actor, trasy po pasach i modele zastępcze. | Mysz pozwala umieścić uczestnika w wybranym punkcie albo przyciągnąć do pasa, ustawić kierunek i narysować trasę. Każda z kategorii może być reprezentowana prostą bryłą. |
| Edycja otoczenia, oznakowania i sygnalizacji | Są biblioteka i narzędzia mapy, edycja obiektów oraz faz sygnalizacji. | Element można dodać, przestawić i cofnąć zmianę. Zmiana drogi wyświetla zerwane odwołania do pasów, stref i sygnałów przed uruchomieniem. |
| Sytuacje statyczne i animowane | Rdzeń ma pozycje, prędkości, trasy, warunki i zdarzenia. Edytor ma Storyboard i podgląd w zewnętrznym esmini. | Użytkownik wskazuje aktora, trasę i akcję, uruchamia odtwarzanie, zatrzymuje je oraz przewija czas. Zdarzenie ma widoczny stan i znacznik na osi. Trasy i ruch sygnałów są widoczne w tym samym widoku 3D. |
| Warianty tego samego zdarzenia w różnych otoczeniach | Projekt otwiera pary XODR/XOSC, a agent generuje je z JSON. | Jednym poleceniem powstaje wariant z własnymi ustawieniami. Przy podmianie mapy użytkownik przypisuje role pasów i stref do nowych elementów, a edytor zgłasza powiązania wymagające korekty. |
| Zmiana pogody, pory dnia i sezonu | Są ustawienia renderowania i materiały. | Presety dnia, nocy, deszczu, mgły i warunków zimowych dają różne parametry widoczności oraz wyglądu w podglądzie i eksporcie. Wpływ warunków na zachowanie uczestników, jeśli potrzebny, jest jawnym ustawieniem scenariusza. |
| Zdjęcia i filmy z różnych perspektyw, także z kokpitu, w różnych formatach | Jest kamera edytora i zrzut klatki. Symulacja nie ma wspólnego odtwarzacza i eksportera filmu. | Kamera kierowcy, widok z góry i kamera swobodna korzystają z tego samego przebiegu. Wybór kamery, czasu, rozdzielczości i liczby klatek prowadzi do gotowego PNG lub MP4. Kolejne presety obejmują kamerę przy drodze, ujęcie podążające, cięcia między kamerami oraz opcjonalny uproszczony kokpit. |

## Jeden przepływ w edytorze

1. **Mapa.** Instruktor wybiera szablon i układa drogi, przejścia, oznakowanie oraz strefy zdarzeń.
2. **Scenariusz.** Dodaje uczestników myszą, wybiera pasy lub rysuje trasę. Formularz wyświetla prędkość, warunki i akcje w czytelnych jednostkach. Storyboard udostępnia szczegółowe zależności.
3. **Odtwarzanie.** Stałe przyciski **Odtwórz**, **Pauza** i **Od początku**, suwak czasu oraz znaczniki akcji korzystają z wyników jednego przebiegu. Zmiana mapy lub scenariusza oznacza starszą próbę jako pochodzącą z wcześniejszej rewizji.
4. **Nagranie.** Użytkownik wybiera kamerę, początek i koniec ujęcia oraz format. Render odbywa się klatka po klatce z przebiegu sceny, bez paneli i znaczników edycyjnych. Program pokazuje postęp i ścieżkę ukończonego pliku. Przerwanie albo błąd kodowania nie niszczy poprzedniego eksportu.

## Kolejność dostarczania

1. **Wejście do edycji i nieruchomy obraz.** Widoczny przycisk trybu, narzędzia aktora i trasy, automatyczne otwarcie Storyboard oraz eksport PNG z aktualnego kadru. Odbiór w rzeczywistym Windows na kopii XODR/XOSC.
2. **Odtwarzanie i pierwsze nagranie MP4.** Uruchomić esmini jako zarządzany proces bez własnego okna, zapisać pozycje i zdarzenia, odtworzyć je w widoku Qt, a następnie wyrenderować zakres czasu i zakodować film z GUI. Sprawdzić także błąd procesu, limit czasu bez oczekiwanego zdarzenia oraz zachowanie przy przerwaniu eksportu. Przypięta wersja silnika, stały krok i ziarno pozwalają odtworzyć próbę. [Dokumentacja esmini](https://esmini.github.io/)
3. **Pełna scena wzorcowa i ochrona danych.** „Widoczność przed przejściem dla pieszych” z pojazdem, pieszym i przesłoną. Instruktor zmienia moment zdarzenia, nagrywa ujęcie z kamery kierowcy i z góry, po ponownym otwarciu zachowuje oba warianty. Zapis i odzyskiwanie obejmują spójną rewizję mapy, scenariusza i ustawień ujęcia.
4. **Warianty i skala.** Szablony miejskie, pozamiejskie i autostradowe, zmiana otoczenia, pogody, sezonu i pory dnia, dodatkowe kategorie uczestników, kamery, kokpit, presety i eksport wielu wariantów.

Test pierwszego filmu obejmuje weryfikację rozdzielczości, FPS i czasu oraz porównanie co najmniej dwóch klatek, na których wybrany aktor faktycznie zmienia położenie. Kamera, zakres i chwila zdarzenia muszą zgadzać się z odtwarzaniem. Pomiar powtarzalności porównuje zdarzenia i pozycje, z ustaloną tolerancją zamiast wymagania identycznego pliku MP4.

## Ćwiczenie z widoczności przed przejściem

Na ekranie startowym wybierz jeden z dwóch przykładów **Przejście dla pieszych**. Użyj **File → Save As**, aby utworzyć kopię do edycji. Kliknij **Uruchom symulację** na górnym pasku. W panelu **Simulation** wybierz **Camera → Driver view — Auto**. W **Camera → Editor camera** zapisany widok pokazuje tę samą sytuację z góry. **Export MP4…** nagrywa wybrane ujęcie.

W obu wariantach samochód zaczyna hamować po 2,45 s, a pieszy wchodzi na przejście po 2,70 s. W drugim wariancie zaparkowany dostawczak przesłania pieszego od strony kierowcy. Porównaj początek przejścia w obu ujęciach. Zatrzymanie samochodu jest zaprogramowane w scenariuszu.

## Rysowanie ruchu kursorem

1. Włącz **Scenario**. Biblioteka otworzy kategorię **Uczestnicy ruchu** z samochodem, ciężarówką, autobusem, motocyklem, rowerem i pieszym.
2. Przeciągnij uczestnika na scenę. Pojazd przyciąga się do środka pasa. Pieszego można upuścić także na chodnik. Edytor zaznacza obiekt, przełącza na **Trasę** i pokazuje, co kliknąć dalej.
3. Kliknij cel i kolejne punkty. Przeciągnij węzeł, aby go skorygować. Kliknij **Zakończ trasę** albo wciśnij Enter, a następnie dodaj następnego uczestnika. Ctrl+Z cofa pojedyncze ustawienie lub zmianę trasy.
4. Po zakończeniu trasy zaznacz uczestnika, aby w **Properties** zmienić **Prędkość początkową** w km/h lub nacisnąć **Edytuj trasę**, co włącza to samo narzędzie rysowania punktów. W pliku XOSC prędkość jest zapisana w m/s; samo otwarcie panelu nie modyfikuje zapisu.
5. Aby przestawić **już ustawiony samochód**, wybierz **Select** lub **Move**, chwyć myszą jego widoczną bryłę i upuść w wybranym miejscu pasa — pozycja przyciągnie się do drogi. Jeśli początek własnej trasy odpowiadał poprzedniemu położeniu, przesunie się z samochodem w jednym kroku Undo. Zwykła jezdnia pozostaje w miejscu. Pieszego z trasą rysowaną od początku scenariusza przesuwa się analogicznie; ścieżki pieszego uruchamiane przez zdarzenia w Storyboard wymagają nadal oddzielnej edycji trajektorii.
6. Kliknij **Uruchom symulację** na górnym pasku obok **Scenario**. Odtwarzanie ruszy po obliczeniu przebiegu. **Wróć do edycji** lub **Select** pozwala ponownie zaznaczyć uczestnika na scenie.

Domyślna prędkość pojazdu bez ustawionej prędkości to 30 km/h. Pieszy porusza się po narysowanej ścieżce od początku przebiegu z prędkością 1,4 m/s i zatrzymuje na ostatnim punkcie. Trasę pojazdu esmini wykorzystuje do wyboru dróg i pasów. Zatrzymanie auta wymaga akcji hamowania w **Scenario → Storyboard**.

Założenie techniczne to wspólny model C++ i jeden widok Qt, z silnikiem oraz enkoderem zarządzanymi przez edytor. Zintegrowana symulacja zasila odtwarzanie i render tym samym zapisem przebiegu. Obraz statyczny i film korzystają ze wspólnego sposobu rysowania kadru. Powiązania z mapą i przebieg mają wersjonowane dane scenariusza, a wynikowe pliki PNG/MP4 trafiają do wskazanej lokalizacji. Interaktywna odpowiedź kursanta pozostaje rozwinięciem dydaktycznym po zamknięciu podstawowego przepływu produkcji materiałów. [Wcześniejszy audyt architektury](PRAWKO-SCENARIUSZE-DYDAKTYCZNE.md)
