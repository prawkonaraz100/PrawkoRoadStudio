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

## R6b2e - dzialajace nocne latarnie (16.11.2026)

Zrodlo jest w rzeczywistym PrawkoRoadStudio Windows/OpenGL 3.3. Lampy wynikaja z pozycji/obrotu modelu r6b-latarnia, swieca tylko o zmierzchu/noca i oswietlaja rzeczywiste 3D mesh i podloze w shaderach. Scena testowa 06g-r6b2e-latarnie-noc.* to kopia 06f, z dwiema dodatkowymi latarniami przy sciezce; poprzednie sceny sa niezmienione. A/B w tych zdjeciach to IDENTYCZNA scena/kamera/pora, rozniaca sie wylacznie eksperymentalnym wyciszeniem oswietlenia latarni przez zmienna PRAWKO_R6B2E_PROOF_DISABLE_STREETLIGHTS.
- r6b2e-porownanie-noc-calosc.png - screenshoty Qt NOC OFF i ON z identycznej kamery, nie fotomontaz AI
- r6b2e-porownanie-noc-przejscie.png - screenshoty Qt przy przejsciu OFF i ON
- r6b2e-qt-wide-day-off.png / r6b2e-qt-wide-day-on.png - techniczna kontrola ze dzien pozostal niezmieniony
- r6b2e-qt-wide-night-off.png / r6b2e-qt-wide-night-on.png - surowe screenshoty calej sceny noca
- r6b2e-qt-path-night-off.png / r6b2e-qt-path-night-on.png - surowe screenshoty przejscia noca
- r6b2e-qt-path-dusk-on.png - widok o zmierzchu
- r6b2e-qt-path-rain-night-on.png - widok noca podczas deszczu

Granica modelu: ograniczona dyfuzyjna emisja swiatla ze swiata 3D, nie pelne ray-tracing/IES, brak osobnych map cienia od latarni i pelnych refleksow mokrej nawierzchni. Nie deklarowac fotometrycznej poprawnosci.

## R6b2f - mapy cieni latarni (16.11.2026)

Zrzuty wykonane bezposrednio z Windows PrawkoRoadStudio / OpenGL 3.3; NIE sa obrazami AI. Scena 06h jest geometrycznie identyczna z 06g. A/B pokazuje te same oprawy i kolor swiatla w tej samej scenie oraz kamerze. Zmienna eksperymentalna PRAWKO_R6B2F_PROOF_DISABLE_SHADOWS wylacza tylko cienie latarni, nie ich samo swiatlo.
- r6b2f-porownanie-cienie-ulica.png - realny A/B bez i z mapami cieni dla pelnego widoku
- r6b2f-porownanie-cienie-przejscie.png - realny A/B dla dojscia pomiedzy kamienicami
- r6b2f-porownanie-cienie-podworko.png - realny A/B dla zblizenia podworka
- r6b2f-qt-{wide-night-off,wide-night-on,path-night-off,path-night-on,court-night-off,court-night-on}.png - oryginalne obrazy A/B Qt
- r6b2f-qt-{path-day-off,path-day-on}.png - dowod zachowania dnia bez zmiany
- r6b2f-qt-path-dusk-on.png - rzeczywisty shader w zmierzchu
- r6b2f-qt-path-night-rain-on.png - rzeczywisty shader deszczu i cieni

Ograniczenia: max 4 mapy cieni 512x512, brak fizycznego IES, moga byc artefakty na krawedziach, nie ma ray-tracing ani fizycznych odbic mokrych powierzchni. Wyniki FPS i Quality Gate w HANDOFF.md.

## R6b2g - emisja klosza glTF i mokre nawierzchnie PBR (16.11.2026)

Autentyczne zrzuty z PrawkoRoadStudio Windows/OpenGL 3.3, nie obrazy AI. Material glTF emissiveFactor z autorskiego GLB w assets/prawko/blender/oprawa_r6b2g/ jest importowany przez core i dodawany do rzeczywistych fragmentow klosza w shaderze. Model jest oddzielnym, edytowalnym elementem biblioteki. R6b2g zmniejsza roughness poziomych powierzchni GLB PBR w deszczu i liczy swiatlo zwierciadlane od rzeczywistych latarni z oslona cieni. Brak pelnego SSR/raytracingu i kalibracji fotometrycznej.

- r6b2g-porownanie-klosz-noc.png - identyczna scena/pora/kamera, material emisyjny OFF/ON, pozostale oswietlenie bez zmian
- r6b2g-porownanie-mokre-plyty-noc.png - identyczny deszcz i noc, tylko nowa reakcja mokrych GLB PBR OFF/ON
- r6b2g-porownanie-mokre-plyty-dzien.png - analogiczny test mokrego materialu w dziennym deszczu
- r6b2g-qt-* - 13 oryginalnych klatek renderera w roznych warunkach
- r6b2g-blender-oprawa-emisja.png - osobny autorski Cycles modelu, NIE screen z Qt

Scena testowa 06i-r6b2g-mokry-plac-i-oprawy to nowa kopia 06h, z piecioma zamienionymi na nowy GLB latarniami. Stare sceny i stara latarnia pozostaja bez zmian i sa dostepne.

## R6b2h - kaĹ‚uĹĽe i refleksy PLANARNE rzeczywistego otoczenia (16.11.2026)

Wybrane poprawne zrzuty oznaczone r6b2h-qt-* powstaly bezposrednio w PrawkoRoadStudio Windows OpenGL 3.3, nie sa AI ani stockiem. Dwie nowe autorskie kaĹ‚uĹĽe 3D ze zrodĹ‚ami Blender .blend i GLB sa w assets/prawko/blender/kaluze_r6b2h/. KaĹĽdy model jest w bibliotece assetow i mozna go dodac/przesunac/obrocic/zapisac. Material glTF o prefiksie PrawkoWater. aktywuje osobna scene-reflection: istniejÄ…cy renderer rysuje budynki i inne nieprzezroczyste obiekty jeszcze raz z kamery odbitej wzgledem plaskiej tafli (maksymalnie 512Ă—384), a fragmenty rzeczywistej wody pobieraja kolor z odbicia. Nie jest to statyczne odbicie nieba ani przemalowany obraz.

- r6b2h-porownanie-odbicia-dzien.png / noc.png: identyczna kamera i deszcz, tylko odbicie PLANARNE OFF/ON
- r6b2h-porownanie-odbicia-podworko.png: identyczne ujÄ™cie podwĂłrza noca
- r6b2h-porownanie-odbicia-zblizenie.png: ten sam plan wody z bliska
- r6b2h-qt-path-clear-day-off/on.png: regresja suchych obrazow, efekt wylaczony
- r6b2h-qt-*.png: 12 surowych screenshotow z Windows
- r6b2h-kaluza-{nieregularna-240,placowa-140}-blender.png: osobne rendery Cycles oryginalnych modeli

Ograniczenia: jedna plaska wysokoĹ›Ä‡ odbicia na kamera; dopasowanie do pierwszej widocznej kaĹ‚uĹĽy, nie pelny SSR/ray tracing, odbicie pomija przezroczystosci w pomocniczym przebiegu, zmniejszona rozdzielczosc odbicia, brak fizycznej falowania wody i mglistej dyspersji. Efekt aktywny tylko w deszczu, w trybie Textured, i gdy istnieje powierzchnia PrawkoWater.


## R6b2i — odbicie fasady, witryn i drzwi w rzeczywistej kałuży (16.11.2026)

Autentyczne obrazy aplikacji PrawkoRoadStudio Windows/OpenGL 3.3, nie obrazy AI ani stock. Porównania OFF/ON mają tę samą kamerę, geometrię, porę dnia oraz identyczny układ kropli; zmienia się wyłącznie odbicie planarne. Surowy bufor GPU 424 x 384, przechwycony jedynie diagnostycznie, pokazał odwróconą geometrię kamienicy, rzędy okien i drzwi; kod przechwytywania został usunięty z produkcji.

- r6b2i-porownanie-lustrzana-kamienica-dzien.png — dzień i deszcz, OFF/ON, rozpoznawalne odwrócone drzwi i pionowe struktury fasady.
- r6b2i-porownanie-lustrzana-kamienica-noc.png — ten sam kadr po zmroku.
- r6b2i-porownanie-lustrzana-kamienica-kadr.png — szerszy kadr podczas deszczu.
- r6b2i-porownanie-sucha-kontrola.png — dokładnie ten sam suchy widok OFF/ON, zerowa różnica.
- r6b2i-qt-*.png — 11 nieprzerobionych screenshotów z programu Windows.
- r6b2i-kaluza-przykraweznikowa-700-blender.png — osobny render Cycles modelu, NIE screenshot Qt.

Model r6b2i-kaluza-przykraweznikowa-700 (ok. 6,96 x 2,16 m, 4 części GLB) korzysta z istniejącego generatora Blender R6b2h i materiału glTF PrawkoWater.Surface (BLEND, alfa 0,69), ma własny GLB, plik .blend i miniaturę. To osiemnasty model biblioteki, edytowalny metodą przeciągnij-upuść, przesuwania, obracania i zapisu. Nowa osobna scena 06k zawiera 37 obiektów, z czego 5 to kałuże; wcześniejsza 06j nie została zmieniona. Shader wzmacnia tylko prawdziwą teksturę odbicia w tafli przy deszczu.

Realny test A/B (75 429 równych próbek): dzień/deszcz 2681 istotnych zmian, noc/deszcz 1646, szerszy kadr 940, sucha kontrola 0. Przykładowe trzy pasy jasności w tafli: ON 82,42 / 67,57 / 78,95 (ściana-drzwi-ściana); OFF 55,31 / 54,94 / 55,36. R6B2I_PLANAR_FACADE_NATIVE_VISUAL_GATE_PASS. Testy: Qt 1366 PASS / 7 SKIPPED, C++ 2074 PASS, Python 7 PASS, BUILD_PASS. GTX 1050 Ti / MSAA4 / 1600x900: z odbiciem 59,84 FPS steady / 60,00 orbit; bez odbicia 59,83 / 60,00; 0 klatek powyżej 33,33ms. Qt synchronizowane ~60Hz, to nie maksymalny FPS GPU.

Ograniczenia: wizualnie potwierdzono fragmenty fasady i drzwi w danej kompozycji; nie gwarantuje to idealnego odbicia wszystkich pięter we wszystkich kamerach. Renderer korzysta z jednej wysokości lustra na kamerę, ograniczonego FBO do 512x384; brak pełnego SSR, ray tracing i odbicia transparentnego szkła. Sucha scena i materiały inne niż PrawkoWater zachowują dotychczasowy wygląd.

## R6b2j â€” dwa poziomy odbicia / kamera w ruchu / esmini (16.11.2026)

Dwa niezalezne rzeczywiste FBO OpenGL 3.3 (kazdy maksymalnie 512x384), a nie nowy prerender ani mapa z internetu. Odbicia aktualizuja sie przy zmianie kamery, a shader PrawkoWater wybiera najblizsza z dwoch wysokosci Z. Obydwa przebiegi sa ograniczone i uzywaja tej samej geometrii obiektow 3D. Scena 06l-r6b2j-dwie-wysokosci-kaluz stanowi osobna kopie 06k; jezâ€‹dnia 0.0 m, chodnik +0.17 m. Wszystkie 18 assetow biblioteki sa zachowane (nie tworzono nowego modelu tylko ponownie uzyto istniejÄ…cych).

- r6b2j-porownanie-poziomy-dzien.png â€” realny render Qt z jednym i dwoma poziomami, ta sama kamera, te same krople deszczu
- r6b2j-porownanie-poziomy-noc.png â€” odpowiednik nocny
- r6b2j-porownanie-chodnik.png â€” zblizenie rzeczywistej powierzchni wody na chodniku
- r6b2j-porownanie-sucho.png â€” scena sucha identyczna OFF/ON
- r6b2j-cztery-pozycje-kamery.png â€” CZTERY ODDZIELNE, rzeczywiste statyczne rendery programu z kolejnych pozycji kamery, nie odtwarzanie MP4
- r6b2j-driver-esmini.mp4 â€” jesli plik obecny: faktyczna symulacja esmini + kamera kierowcy + FFmpeg, nie montaz z obrazow; pliki .first.png, .moving.png i .dual-{on,off}.png z natywnego testu gtest
- r6b2j-porownanie-esmini-kierowca.png â€” jesli plik obecny: porownanie w tej samej klatce realnego esmini przy 2 poziomach ON/OFF
- r6b2j-*.png â€” surowe, nieprzerobione zrzuty wlasnego Qt Windows. NIE obrazy AI

Ograniczenia: maksymalnie dwie wysokosci lustra na widok/kamere (trzecia zostaje przypisana do blizszej); brak ray tracingu, SSR dla innych materialow oraz transparentnego szkĹ‚a w odbiciu. FPS odczytuje realny frameSwapped synchronizowany ~60 Hz; nie mierzy maksymalnej wydajnosci GPU.


### Optymalizacja renderera — osobna gałąź (16.11.2026)

Wyniki dotyczą tylko gałęzi codex/render-quality-performance, bez połączenia ze zmianami R6b2j. Wszystkie obrazy to rzeczywiste renderowania z Windows/OpenGL, a nie obrazy AI lub stockowe.

- render-quality-street-day.png, render-quality-street-rain-night.png: rzeczywiste zrzuty Qt (866 x 783), ukazujące nadal uproszczoną geometrię oraz nocne oświetlenie.
- render-quality-gpu-baseline.png, render-quality-gpu-cache.png, render-quality-gpu-aniso4.png: syntetyczny test GPU, nie realistyczna scena uliczna. Przy takim samym filtrowaniu obrazy cache OFF i ON są identyczne pikselowo.
- render-quality-benchmark.json: pomiary GTX 1050 Ti, OpenGL, 1280 x 720, 1000 instancji / 8 części, cztery latarnie i cień słońca, 15 warmup + 90 klatek. GPU p95 5,976 / 5,302 / 5,311 ms; CPU submission p95 0,656 / 0,466 / 0,455 ms, odpowiednio cache OFF / ON / ON + anizotropia 4x. Statyczne przesłania GPU: 4860 / 0 / 0, 276 514 560 / 0 / 0 bajtów.
- render-quality-qt-fps.json: QOpenGLWidget, 1600 x 900 MSAA4, 180 próbek, ok. 59,85 FPS na postoju i 60,01 FPS podczas orbitowania, bez klatek powyżej 33,33 ms. Limit odświeżania około 60 Hz, nie maksymalny FPS GPU.
- render-quality-driver-mirrors.mp4, render-quality-driver-first.png, render-quality-driver-moving.png: rzeczywisty przejazd esmini i trzy niezależne lusterka, 72 klatki / 6 sekund / 12 FPS, test integracyjny PASS.

Build all PASS, Qt 1371 PASS / 7 SKIPPED, C++ 2074 PASS, Python 7 PASS. Stabilizacja słońca sprawdzona matematycznie, bez ilościowego testu migotania przez całą długą jazdę. Scalenie z R6b2j pozostaje otwarte.


## R6b2k - integracja dwóch poziomów kałuż z optymalizacją renderera (Windows)

To są NOWE wyniki z połączonego kodu R6b2j (commit 0af977e) i optymalizacji (commit 5677026), a nie stare wyniki dwóch osobnych gałęzi. Kod zachowuje 18 assetów biblioteki, scenę 06l, dwa poziomy rzeczywistych FBO i poruszające się samochody. Cache ponownie wykorzystuje instancje w głównym przebiegu, cieniach słońca i lamp oraz odbiciach; cienie słoneczne stabilizowane są do pełnych tekseli. Anizotropia materiałów GLB do 4x z fallbackiem.

Materiały r6b2k-rain-day-cache-off/on.png, r6b2k-rain-night-cache-off/on.png, r6b2k-clear-day-cache-off/on.png i r6b2k-moving1/2-cache-off/on.png to prawdziwe PNG z Qt Windows na GTX 1050 Ti (identyczna geometria, kamera, deszcz; zamrożone krople wyłącznie podczas próby). Cache ON/OFF dał ZERO zmian nawet na najmniejszych badanych próbkach RGB we wszystkich pięciu parach. Pliki r6b2k-rain-day-onelevel.png, r6b2k-rain-night-onelevel.png i r6b2k-sidewalk-onelevel.png oraz odpowiadające im pliki twolevels potwierdzają działanie drugiego poziomu: 928 / 512 / 1351 istotnych zmian w A/B.

r6b2k-driver-esmini.mp4 i r6b2k-esmini-*.png są faktycznym przejazdem esmini z kamerą kierowcy, trzema lusterkami, FFmpeg i OFF/ON drugiego poziomu przy tym samym czasie/pozie pojazdu (30 istotnych próbek). r6b2k-gpu-0/1/2.png i r6b2k-gpu-benchmark.json są SYNTETYCZNYM benchmarkiem GPU, a NIE miejską sceną. Benchmark 90 próbek / 1000 instancji po 8 części na GTX 1050 Ti: GPU p95 6.116 ms cache OFF; 4.780 ms cache ON; 4.785 ms ON z filtrem 4x. Zysk 21.8% w tym benchmarku, nie gwarancja przyspieszenia dowolnej sceny; transfery 4860 do 0. rzeczywiste Qt FPS (frameSwapped przy synchronizacji około 60 Hz) z cache ON: steady_refresh: 60.00 FPS, alt_lmb_orbit: 60.00 FPS. Surowe dane w r6b2k-fps-cache-*.json i r6b2k-validated-summary.json.

Pełny build PASS, 1371 Qt PASS / 7 SKIPPED, 2074 C++ PASS, 7 Python PASS, QUALITY_GATE_PASS, realny przejazd esmini PASS. Ograniczenia: maksymalnie dwie wysokości kałuż na widok; bufor odbić nadal alokowany przy zmianie rozdzielczości kamer, brak globalnego HDR, SSR i ray tracingu; FPS zsynchronizowane z ekranem nie oznacza maksymalnego FPS GPU.
