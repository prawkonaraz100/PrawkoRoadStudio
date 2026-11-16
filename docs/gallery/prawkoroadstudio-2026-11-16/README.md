# PrawkoRoadStudio â€” publiczne rendery, 16 listopada 2026

Wszystkie obrazy zostaĹ‚y wygenerowane przez rzeczywisty program PrawkoRoadStudio pod Windows / OpenGL albo pochodzÄ… z niezaleĹĽnego renderera Blender Cycles. Bez generatywnej podmiany.

- wiata-zblizenie.png: program Windows, preset wiata-close.xodr, widok kamery zapisanej w testowej scenie.
- wiata-dzien.png: program Windows, preset wiata-day.xodr.
- wiata-deszcz.png: program Windows, preset wiata-rain.xodr.
- wiata-noc.png: program Windows, scena statyczna bez samochodu i Ĺ›wiateĹ‚ Ego; dlatego celowo ciemna.
- jazda-noc-reflektory.png: rzeczywista klatka 2.0 s z nocnego filmu esmini + PrawkoRoadStudio r4b3a-traffic-headlights-night.mp4, 3 lusterka, projekt 01-miasto-lusterka.
- wiata-blender-cycles.png: niezaleĹĽny render Blender 5.3/Cycles modelu GLB z biblioteki, z odbioru R5.

Klatki z Qt i Blendera nie sÄ… parytetem fotometrycznym. Wyniki/testy R5 i R4b3a zapisane w HANDOFF.md, peĹ‚ne materiaĹ‚y ĹşrĂłdĹ‚owe w lokalnym ignorowanym archiwum archive/a1-renderer-20261115.

- r4b3b-reflektory-ruchu-wlaczone.png / r4b3b-reflektory-ruchu-wylaczone.png: identyczne nocne klatki esmini (t=2.0 s) na Windows OpenGL. Tylko swiatla najblizszego pojazdu ruchu sa przelaczane; lampy Ego pozostaja bez zmian. Test A/B: 7663 jasniejszych probek, 0 ciemniejszych co 2 px. Brak jeszcze cieni rzucanych przez te reflektory.

- r6-kamienice-qt-front.png: realne dwa modele kamienicy i wiata na osobnej scenie 06-r6-otoczenie-prototyp, natywny Windows OpenGL 3.3; R6 pozostaje prototypem.
- r6-kamienica-blender.png: render wlasnego GLB w Blender Cycles 5.3; to niezalezny renderer, nie jest fotometrycznie identyczny z Qt.

## R6b1 â€” rzeczywiste testy i obrazy (16.11.2026)

Wszystkie zrzuty Qt pochodzÄ… bezpoĹ›rednio z PrawkoRoadStudio Windows/OpenGL 3.3, nie z generatora obrazĂłw. Renderingi Blender to autorskie modele .blend/.glb w assets/prawko/blender/ulica_r6b/. Obrazy majÄ… wĹ‚asne ĹşrĂłdĹ‚a w repozytorium, a dokumentacja PASS i ograniczeĹ„ jest w HANDOFF.md.

- r6b-qt-calosc.png â€” nowa scena 06b, dwa budynki, 4 chodniki prefabrykowane, 4 drzewa, 3 latarnie, wiata i droga (866x783)
- r6b-qt-ta-sama-kamera.png â€” dokĹ‚adnie ten sam kÄ…t/odlegĹ‚oĹ›Ä‡/kadr/dzieĹ„/pogoda co R6a, do porĂłwnania zmian bez przesuniÄ™cia kamery
- r6b-porownanie-przed-po.png â€” zestawienie dwĂłch prawdziwych obrazĂłw z identycznej kamery, po lewej R6a, po prawej R6b
- r6b-qt-dzien-zblizenie.png â€” realny podglÄ…d pĹ‚yt i detali przy budynku w dzieĹ„ (866x783)
- r6b-qt-deszcz-zblizenie.png â€” realny podglÄ…d tej samej kamery w deszczu (866x783)
- r6b-blender-chodnik.png, r6b-blender-drzewo.png, r6b-blender-latarnia.png â€” niezaleĹĽne podglÄ…dy Cycles oryginalnych assetĂłw, NIE zrzuty edytora

WaĹĽne ograniczenia: trawnik/teren pozostaje rozlegĹ‚y i jednorodny, korony drzew sÄ… jeszcze uproszczone, samochody nadal sÄ… zastÄ™pczymi bryĹ‚ami, a latarnie sÄ… wyĹ‚Ä…cznie obiektami 3D bez emitowania Ĺ›wiatĹ‚a w nocy. Nie uznawaÄ‡ tego etapu za koĹ„cowy realizm wizualny.

## R6b2 - rzeczywisty samochod GLB i dynamiczna scena (16.11.2026)

Wszystkie ponizsze zrzuty Qt wykonano w PrawkoRoadStudio na Windows OpenGL 3.3. Model samochodu, jego zrodlo .blend, generator .py i oryginalny GLB sa w assets/prawko/blender/samochod_r6b2/.

- r6b2-qt-ulica-samochod.png: rzeczywisty renderer Qt 866x783, samochod zamiast pomaranczowej bryly na scenie 06c-r6b2-samochody
- r6b2-qt-samochod-zblizenie.png: zblizenie samochodu na scenie Qt, prawdziwe kola i przeszklenie (866x783)
- r6b2-porownanie-bryla-samochod.png: dwa oryginalne rendery sceny sprzed i po zmianie, ta sama kamera
- r6b2-blender-samochod.png: autorski render Blender Cycles, NIE zrzut Qt
- r6b2-esmini-jazda-6s.mp4: 72 klatki natywnego ruchu esmini OpenGL, 960x540 / 6 sekund / 12 FPS, 3 lusterka
- r6b2-esmini-kadr-0s.png i r6b2-esmini-kadr-4s.png: dwie klatki pochodzace z rzeczywistego testu MP4

Ograniczenia: to jeszcze niskopoligonowa karoseria, bez animacji obrotu kol i bez fizycznej refrakcji. Dotychczasowe reflektory dynamiczne pozostaja logika silnika, nie modelem lampy. Inne kategorie pojazdow oraz projekty bez tego GLB nadal korzystaja z bryl zastepczych. Teren nadal jest zbyt jednolity.

## R6b2b - rzeczywiste zielen i teren 3D (16.11.2026)

Kazdy z trzech assetow jest autorskim modelem Blender, zrodlowy .blend + .glb + generator Python znajduje sie w assets/prawko/blender/teren_r6b2b/. Miniatury i kopie GLB zostaly wpisane do biblioteki Prawko-Scenariusze. Odrebna scena 06d-r6b2b-naturalne-otoczenie zawiera dodatkowe 5 pasow zieleni, 2 rabaty i 2 sciezki. Oryginalne sceny pozostaly nienaruszone.

- r6b2b-qt-calosc.png â€” faktyczny PrawkoRoadStudio Windows/OpenGL, ten sam kadr co 06c w R6b2a
- r6b2b-qt-pas-zieleni.png â€” rzeczywisty Qt: zblizenie przyulicznych kep i krzewow
- r6b2b-qt-ogrody.png â€” rzeczywisty Qt: rabaty i mineralne sciezki za kamienicami
- r6b2b-porownanie-przed-po.png â€” porownanie prawdziwych zrzutow Qt sprzed i po, ta sama kamera; R6b2b korzysta dodatkowo z czterech skal szumu podloza
- r6b2b-blender-pas-zieleni.png, r6b2b-blender-rabata.png, r6b2b-blender-sciezka.png â€” osobne autorskie rendery Cycles; NIE udawaja ekranow Qt

Nadal nie jest to fotorealizm: brakuje urozmaicenia architektury wszystkich segmentow, trawa w oddali jest shaderem proceduralnym, a korony starszych drzew R6b1 wymagaja osobnej poprawy. Docelowo naturalne otoczenie bedzie wypracowane seria niezaleznych komponentow 3D, bez sztucznego stockowego tla.

## R6b2c - autorskie drzewa i strefa tylnej elewacji (16.11.2026)

Pliki oznaczone r6b2c-qt-* to autentyczne screenshoty PrawkoRoadStudio Windows/OpenGL 3.3, a nie generowane ilustracje. Wariant r6b2c-blender-* to oryginalne Cycles z Blender 5.3 i plikow .blend/.glb w assets/prawko/blender/drzewa_r6b2c/.

- r6b2c-qt-calosc.png â€” nowa scena 06e z lipami i brzozami zamiast powtarzajacych sie kulistych drzew; ta sama kamera jak 06d
- r6b2c-qt-drzewa.png â€” zblizenie roznorodnych drzew przy ulicy
- r6b2c-qt-podworka.png â€” zblizenie przejscia przez zabudowe i rabat przy tylnej elewacji
- r6b2c-porownanie-stare-nowe-drzewa.png â€” zestawienie oryginalnych screenshotow 06d/06e z identycznej kamery
- r6b2c-blender-lipa.png / r6b2c-blender-brzoza.png / r6b2c-blender-rabata-fundament.png â€” oddzielne renderingi z Blender Cycles

3 nowe edytowalne GLB: lipa 8.83m, brzoza 9.64m, pas rabaty 12m. 06e to osobna kopia 06d; cztery stare drzewa zastapiono tylko w scenie 06e, zachowujac w bibliotece oryginalny model R6b1. Obrocono 2 sciezki 12m tak, aby wspolnie laczyly chodnik z podworkiem miedzy kamienicami, dodano 2 pasy nasadzen przy tylnych elewacjach i 1 brzoze na terenie przy podworku.

Granice jakosci: modele sa autorskimi przyblizeniami geometrycznymi, nie fotorealistyczna fotogrametria; dalsza naturalizacja dotyczy roslinnosci, nawierzchni, skali i otoczenia poza bezposrednia ulica.

## R6b2d - rzeczywiste przejscia nawierzchni z wlasnymi teksturami PBR (16.11.2026)

Modelami ĹşrĂłdĹ‚owymi sa wlasne .blend/.glb w assets/prawko/blender/styki_r6b2d/. Proceduralne mapy albedo, normal i ORM sa autorstwa PrawkoNaRaz, zapisane jako PNG i osadzone w GLB. Obiekty sa edytowalne myszka po dodaniu przez biblioteke. Nowa scena: 06f-r6b2d-nawierzchnie-pbr.xodr/.xosc/.rmscene.json. Stare sceny nie byly modyfikowane.

- r6b2d-qt-calosc-dzien.png - prawdziwy widok edytora Qt / OpenGL 3.3, ta sama kamera co R6b2c
- r6b2d-qt-przejscie-dzien.png - zblizenie nowych plyt 12m, fizyczne fugi i odwodnienie
- r6b2d-qt-przejscie-deszcz.png - rzeczywisty widok Qt z pogoda rain (nie render AI)
- r6b2d-qt-przejscie-noc.png - rzeczywisty widok Qt z pora night (nie render AI)
- r6b2d-qt-styk-chodnika.png - nowa krawedz mineralno-glebowa 20m z kepkami trawy
- r6b2d-porownanie-przed-po.png - R6b2c/R6b2d, ten sam kadr i pogoda day/clear
- r6b2d-blender-przejscie-pbr.png, r6b2d-blender-styk-pbr.png - osobne rendery Cycles, nie zrzuty Qt

Ograniczenia: nawierzchnie sa proceduralnymi autorskimi materialami PBR oraz wyodrebniona geometria, ale nie fotogrametria. Nie ma docelowych mokrych odbic SSR ani fizycznych kaustyk. Dalsza praca to zgodnosc granic terenu, roznorodnosc budynkow i lokalne mokre materialy.
