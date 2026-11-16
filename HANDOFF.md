# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> Stan tej gałęzi na 16.11.2026. Przed zmianą kodu przeczytaj ten plik i `AGENTS.md`. Kolejność prac wyznacza **Aktywny plan** na początku pliku.

## Cel PrawkoNaRaz

Instruktor tworzy i edytuje sytuację drogową, odtwarza ją, wybiera kamerę i eksportuje PNG lub MP4 w jednym programie Windows. Funkcjonalnym punktem odniesienia jest VICOM. **Podstawowy przepływ pracy został już potwierdzony; aktualnym priorytetem jest realistyczne otoczenie drogowe i produkcja własnych modeli 3D w Blenderze, nie dalsza ogólna rozbudowa edytora.** Wspólny model C++ oraz OpenDRIVE i OpenSCENARIO obsługują edytor i agenta. [Zakres funkcji i kryteria produktu](docs/PRAWKO-VICOM.md)

## 16.11.2026 — niezależny etap poprawy renderera (PASS, integracja otwarta)

**Najnowsze polecenie:** poprawić wygląd oraz wydajność silnika. Użytkownik potwierdził, że drugi agent nadal pracuje nad niezacommitowanym R6b2j (dwie wysokości kałuż) w głównym katalogu. Dlatego ten etap powstaje wyłącznie w worktree `F:\PrawkoRoadStudio\archive\render-opt`, na gałęzi `codex/render-quality-performance`, od bazy `73adc800cec2eb6eee3d04dfdc21ff3f0bb87dd9`. NIE nadpisywać głównego katalogu ani jego builda. Zmiany R6b2j nie należą do tej gałęzi. Agent AI bez zmian, HEAD `67a92fa24b4054b7f094c9b667f3634c35ad9210`.

**Plan i stan:**
1. W tym etapie — cache rzeczywistej zawartości bufora instancji współdzielony przez główny pass, cienie i odbicia. Zmiana macierzy w miejscu, zmiana liczby instancji oraz różne spans dla tej samej siatki muszą unieważniać cache. Osobny test sprzętowy porównuje piksele i zlicza transfery, a nie tylko FPS ograniczone synchronizacją monitora.
2. W tym etapie — filtrowanie anizotropowe tekstur materiałów do 4x, ograniczone możliwościami karty (bez wymagania rozszerzenia), oraz stabilizacja siatki cienia kierunkowego w pełnych texelach. Nie zmieniać perspektywicznych cieni latarni ani formatów scen.
3. Następny etap po scaleniu z R6b2j — profilowanie pełnego przejazdu z trzema lusterkami i kałużami, pomijanie obiektów poza bryłą widzenia, oddzielny cache statycznych cieni i budżety detalu LOD. Kryterium to p95 CPU/GPU i poprawność ruchomych uczestników we wszystkich widokach.
4. Następnie — wspólny liniowy HDR i kontrolowana ekspozycja/tonemapping, odbicia środowiska i lepszy kontakt obiektów z podłożem. Każdy efekt wymaga porównania dzień/noc/deszcz i kontroli kosztu na GTX 1050 Ti. Nie deklarować pełnego fotorealizmu.

**Weryfikacja wejściowa:** główny `prawko-resume.ps1 -Verify` potwierdził zgodność obu HEAD z GitHub, 2074 testy core C++ PASS (10 disabled), 7 Python PASS, QUALITY_GATE_PASS. Ostrzeżenie o dirty worktree jest oczekiwane z powodu pracy drugiego agenta. Pełny osobny build i odbiór nowego kodu zakończono PASS; potwierdzone wyniki zapisano poniżej. Narzędzia `.qt`, `reference`, `toolchain` są współdzielone wyłącznie jako zależności; binaria/build oraz kopia repo agenta są osobne. Testować wyłącznie sceny z tej kopii. Integracja do gałęzi funkcjonalnej dopiero po zakończeniu równoległego etapu, po przeglądzie wspólnych plików gl_renderer i CMakeLists; bez automatycznego merge do brudnego katalogu.

**Odbiór końcowy odizolowanej gałęzi (16.11.2026):** Osobny build Windows all PASS, AGENT_BINDING_SYNC_PASS, Qt 1371 PASS / 7 SKIPPED, C++ 2074 PASS, agent Python 7 PASS, QUALITY_GATE_PASS. Nowy cache macierzy instancji jest współdzielony przez pass główny i cienie, aktualizuje się po zmianie pozycji i oddzielnych spanach tej samej siatki. Stabilizacja cienia słońca w całych tekselach; anizotropia 4x z bezpiecznym fallbackiem 1x. Bez zmian plików XODR/XOSC i paneli GUI.

**Wynik sprzętowy (syntetyczny, nie maksymalne FPS sceny):** GTX 1050 Ti, 1000 instancji po 8 części, cienie słońca i 4 latarni, 1280x720, 15 warmup + 90 próbek. GPU p95 5,976 / 5,302 / 5,311 ms (bez cache / cache / cache z filtrem 4x). CPU submission p95 0,656 / 0,466 / 0,455 ms, niepotrzebne przesłania 4860 / 0 / 0; 276 514 560 / 0 / 0 bajtów. Pixele identyczne dla cache OFF i ON przy 1x; próby ruchu, współdzielonej siatki i zmian instancji PASS. QOpenGLWidget MSAA4 1600x900: 59,85 FPS steady i 60,01 FPS orbit, zero klatek >33,33ms przy synchronizacji ok. 60Hz. Esmini 3.11 + FFmpeg i 3 lusterka PASS: 72 klatki / 6 s, zmienione próbki w lusterkach 190 / 252 / 39. Dowody źródłowe: build/quality-*.log (lokalnie) i docs/gallery/prawkoroadstudio-2026-11-16/render-quality-* (w gałęzi).

**Izolacja:** Wszystkie prace dotyczą wyłącznie worktree F:\PrawkoRoadStudio\archive\render-opt i osobnej gałęzi codex/render-quality-performance od 73adc80. Główna gałąź ma już zakończony etap R6b2j, HEAD 0af977ec8bbdd6252db6884468cd79118bdbcd73, lecz tych zmian NIE scalamy automatycznie. Na przyszły etap integracji pozostają konflikty gl_renderer.cpp/hpp, CMakeLists i HANDOFF; zachować dwie płaszczyzny odbicia oraz ponownie wykonać build/Qt/C++/Python, realną jazdę z lustrem i kałużami, pomiar GPU. Projekt testowy tylko w odizolowanym worktree miał zmienione last_scene przez test GUI; przywrócono jedynie to jedno pole po weryfikacji reszty. Modelowanie otoczenia, HDR/IBL, SSR i ilościowy test migotania cienia nadal otwarte.

## 15.11.2026 — PrawkoBridge v0.2 — lokalna instalacja PASS / ChatGPT Tunnel PENDING

**Cel:** dedykowany, bezpieczny konektor do F:\PrawkoRoadStudio. Wdrożenie nie może nadpisywać niescalonych zmian A1/R3.

**Weryfikacja na Windows DESKTOP-G2FV0GJ:**
- Desktop Commander działa; pierwotny projekt i lokalnie zmodyfikowany kod R3 są obecne na gałęzi feature/realistic-road-surfaces, HEAD 7b4a657.
- Rozpakowano pobrany PrawkoBridge_Installer.zip, po uprzednim sprawdzeniu SHA256 df62f4adff45d45406e715b36a0fb838d0c340d3c5b23525028277ee297d155c, do nowego F:\PrawkoRoadStudio\toolchain\PrawkoBridge.
- Poprawiono start serwera dla MCP Python SDK v2 (mcp.server.mcpserver.MCPServer), instalator i diagnostykę. Osobne środowisko .venv: Python 3.12.10, mcp 2.3.0.
- Instalator Windows: PRAWKO_BRIDGE_LOCAL_TESTS_PASS, 9/10 testów PASS, 1 SKIPPED (tworzenie symlinków niedozwolone bez wymaganych uprawnień Windows).
- Bezpośredni test klient/serwer: 10/10 MCP tools rozpoznanych oraz skuteczne bridge_status, handoff_progress, project_read(HANDOFF.md). Sprawdzono in-process i uruchomienie prawdziwego serwera jako stdio subprocess: PRAWKO_BRIDGE_E2E_PASS.
- Oficjalny OpenAI tunnel-client v0.0.21 z Github Releases: ZIP 29 683 613 B, SHA256 2faf80ae62b58485fcef534cd3f78cd8b82e71cb04d14c2a5c73dbb4eb6c575a, suma zgodna z metadanymi wydania. EXE w toolchain\PrawkoBridge\tunnel-client\v0.0.21\tunnel-client.exe. --version oraz help quickstart PASS.
- Przygotowano scripts/connect-tunnel.ps1 oraz scripts/run-tunnel.cmd z lokalną ścieżką EXE. Nie pobierano, nie wyświetlano, nie zapisywano sekretów ani kluczy API.
- Przygotowano kreator scripts/setup-tunnel-interactive.ps1 (test parsowania PowerShell PASS) oraz plik POLACZ-PRAWKOBRIDGE.cmd. Utworzono na pulpicie Windows skrót 'PrawkoBridge - polacz.lnk'. Kreator wprowadza Tunnel ID i klucz runtime wyłącznie lokalnie w pamięci procesu; nie uruchomiono go bez danych użytkownika.

**Kontrola MCP Browser / Platform 15.11.2026:** w otwartych zakładkach Platform `Tunnels` jest komunikat `No tunnels yet`, w zakładce `API keys` `0 results`. MCP Browser dostępny w tym trybie tylko do listowania i odczytu zakładek (bez kliknięć/formularzy). Ponowny test lokalny: `tunnel-client profiles list` → `No profiles found`, proces `tunnel-client` nie działa, w bieżącej sesji brak `CONTROL_PLANE_API_KEY`; prywatnego tunelu nie utworzono. Według aktualnej dokumentacji Secure MCP Tunnel wymaga `tunnel_id` oraz klucza **runtime**; nie jest to wywołanie płatnego modelu. Łączenie własnych serwerów MCP z zapisem zależy dodatkowo od uprawnień/planu ChatGPT. Nie wolno oznaczać jako skonfigurowanego, nie wolno pozyskiwać sekretów z przeglądarki. Źródła: https://developers.openai.com/api/docs/guides/secure-mcp-tunnels i https://help.openai.com/en/articles/12584461-connecting-custom-mcp-servers-in-chatgpt.

**Status: lokalna instalacja oraz stdio — PASS. Połączenie z ChatGPT — PENDING.** Brak CONTROL_PLANE_API_KEY w procesie i skonfigurowanego tunnel_id; nie utworzono jeszcze prywatnej wtyczki w ChatGPT. Uprawnienia tuneli Platform oraz utworzenie własnego pluginu wymagają działania użytkownika w jego koncie.

**Wznowienie:** konfiguracja Secure MCP Tunnel wyłącznie z sekretami w lokalnym środowisku użytkownika: scripts/connect-tunnel.ps1 -TunnelId ..., scripts/run-tunnel.cmd, w ChatGPT Plugins → Add custom MCP server → Tunnel. Potwierdzić z ChatGPT narzędzia bridge_status/handoff_progress. Następnie podjąć A1/R3 i dopiero po kompilacji, pełnych testach oraz testach wizualnych commit. Nie wykonywać git reset/clean ani modyfikacji niecommitowanych plików R3.

## 15.11.2026 — PRIORYTET A1/R: renderer GLB — PLAN I REJESTR REALIZACJI

**Polecenie użytkownika:** poprawić PrawkoRoadStudio w zakresie tekstur UV, PBR, szkła, oświetlenia i cieni, a **każdy etap i jego wynik wpisać do HANDOFF**; istniejący projekt Blender i aktywne sceny zachować.

**Stan wejściowy:** HEAD edytora `acf5664` (`feature/realistic-road-surfaces`), agenta `67a92fa` (`main`), bez zmian roboczych. Handoff i AGENTS przeczytane. Działający importer przenosi jedynie średnie kolory (warning #507), scalar roughness i BLEND alpha; renderer ma istniejące shaderowe słońce/ambient i osobny prosty pass przezroczystości, ale nie przenosi UV ani obrazów glTF do GPU. Brak światła opartego na prawdziwych mapach cieni. Nie oznaczać jako ukończonych.

**Etapy / kolejność (przed testami wszystkie PENDING):**

1. **A1/R1: UV + baseColorTexture — PASS (15.11.2026).** Wdrożone: bezpieczny importer glTF `TEXCOORD_0` (float/znormalizowane UBYTE/USHORT), oryginalne RGBA8 embedded GLB i względne pliki `.gltf` wyłącznie z katalogu modelu, limit 64 MiB tekstur na model, deduplikacja obrazu, GPU upload tylko raz na model podczas przebudowy sceny oraz współdzielenie we wszystkich instancjach; poprawna przestrzeń sRGB dla albedo. Dla nieprawidłowych UV/obrazu zachowany jawnie diagnozowany fallback średniego koloru, starsze elementy bez obrazów bez zmian. Wdrożone w `core/include/roadmaker/assets/{prop_library,prop_import}.hpp`, `core/src/assets/prop_import.cpp`, `editor/src/render/{renderer.hpp,gl_functions.hpp,gl_renderer.cpp,scene_builder.cpp}`, `editor/src/viewport/viewport_widget.{cpp,hpp}`. Odbiór rzeczywistego EXE na kopii sceny `archive/a1-renderer-20261115/r1-uv-close.png` (866×783): widoczne autorskie tekstury drewna i betonu wiaty, BLEND nadal czytelny. Testy najnowszego kodu: pełny Windows `BUILD_PASS: all`, Qt **1354 PASS / 7 SKIPPED / 0 FAIL** (1361 testów, `r1-full-qt.log`), celowane Qt **2/2 PASS** i core import **7/7 PASS** (`r1-qt.log`, `r1-core.log`), niezależny `scripts/prawko-verify-win.ps1` — **2072 C++ PASS (10 disabled), 7 Python PASS, QUALITY_GATE_PASS**. Test starego spłaszczania zastąpiono weryfikacją zachowanych UV i bajtów sRGB, a rejestr projektu weryfikuje ponowny import bez ostrzeżenia `flattened`. **Nie obejmuje metalicznego PBR, fizycznego szkła ani prawdziwych cieni — R2–R5 pozostają otwarte.**
2. **A1/R2: metal/rough PBR — PASS (15.11.2026).** Wdrożono glTF 2.0 `metallicFactor`, `roughnessFactor`, liniową mapę `metallicRoughnessTexture` (G=roughness, B=metallic), liniową mapę normalnych z `scale`, rozdzielenie sRGB albedo od liniowych danych i oddzielny opt-in shader **Cook–Torrance/GGX/Schlick/Smith**; dotychczasowy shader dróg/elementów bez PBR i tryb Sober zachowują stary przebieg. Metaliczna odpowiedź na słońce i aproksymacja hemisferycznego odbicia otoczenia w Blenderowej wiacie są widoczne w realnym OpenGL, **to nie pełne image-based lighting, SSR ani ray tracing**. Dedup GPU tekstur per model niezależnie od liczby kopii; uwzględniono UV0 i odrębne formaty sRGB/linear. Reprodukowana fixture `core/tests/data/gltf/pbr_metal_box.glb` z generatora `scripts/gen_gltf_fixtures.py` testuje 3 faktyczne mapy (albedo, MR, normal) i współczynnik metalu 0,75 / rough 0,60. Zmiany: `core/{include/roadmaker/assets/prop_library.hpp,src/assets/prop_import.cpp,tests/test_prop_import.cpp}`, `editor/src/{render/{gl_functions.hpp,gl_renderer.hpp,gl_renderer.cpp,renderer.hpp,scene_builder.cpp},viewport/viewport_widget.cpp}`. Dowody: `archive/a1-renderer-20261115/{r2-pbr-close.png,r2-pbr-day.png,r2-core.log,r2-qt.log,r2-full-qt.log}`. Windows **BUILD_PASS: all**, celowane **5 core + 2 Qt PASS**, całe Qt **1354 PASS / 7 SKIPPED / 0 FAIL** (1361), Quality Gate **2073 C++ + 7 Python PASS**. **Otwarta nadal fizyczna refrakcja/transmisja szkła i rzeczywiste cienie; brak map środowiskowych HDR/IBL.**
3. **A1/R3: szkło — PASS (15.11.2026).** Wdrożono poprawną obsługę alphaMode OPAQUE/MASK/BLEND i alphaCutoff, sortowanie przezroczystych części względem kamery i uproszczone odbicie szkła; brak fizycznej refrakcji/transmisji i ray tracingu. Odbiór: BUILD_PASS all, 6/6 testów C++, 2/2 celowanych Qt, pełne Qt 1354 PASS / 7 SKIPPED / 0 FAIL, QUALITY_GATE_PASS. Rzeczywiste zrzuty edytora: archive/a1-renderer-20261115/r3-glass-close.png i r3-glass-day.png, podglądy JPEG. R4 i R5 nadal PENDING.
4. **A1/R4: światło i cienie — R4a (mapa głębokości cienia) PASS, R4b PENDING (15.11.2026).** Dodano rzeczywisty przebieg cienia OpenGL 3.3 z FBO/depth 1024×1024, mapowaniem dla kierunkowego słońca, próbkowaniem PCF 3×3 na drogach/obiektach/trawie, alpha-cutoff dla glTF MASK i pominięciem BLEND jako nieprzezroczystych rzucających cień. Instancje GLB zachowują transformacje; render cienia przywraca oryginalny FBO Qt dla kamer i luster. W Textured cień działa z dniem/świtem/zmierzchem (noc ma słabe słońce i pomija pass); Sober i domyślne materiały bez opt-in zachowują stary przebieg. Kod: editor/src/render/gl_shadow.cpp, gl_renderer.cpp/.hpp, gl_functions.hpp, renderer.hpp, editor/CMakeLists.txt. **Weryfikacja R4a:** BUILD_PASS: all, Qt 1354 PASS / 7 SKIPPED / 0 FAIL, 2074 testy C++ PASS, 7 testów Python PASS, QUALITY_GATE_PASS; log rzeczywistego Windows GL: R4 depth shadow map READY: 1024x1024. Zrzuty archive/a1-renderer-20261115/r4-shadow-{close,day}.png i podglądy JPEG; obraz faktycznie zawiera cienie konstrukcji i dachu na betonowym podłożu (6315/3334 zmienionych próbkowanych co 4 pikseli vs identyczny kadr R3, próg sumy RGB 12). **Otwarte R4b:** płynny ruch i 3 lusterka, nocne światła punktowe/reflektory, kontrola zbyt ciemnego cienia dachu i brzegów, wydajność 1/25 instancji; bez deklaracji pełnego fotorealizmu ani cascaded shadow maps. R5 odbiór Blender i FPS nadal PENDING.

**A1/R4b1 — dynamiczna jazda, trzy lusterka i obciążenie wiatami — PASS (15.11.2026).** Na natywnym Windows/OpenGL 3.3 test PrawkoRealVideo.DriverViewStreamsActualTrafficAndMirrorsToMp4 uruchomił rzeczywiste esmini 3.11.0 na scenie 01-miasto-lusterka, z zachowaniem R4 depth shadow map READY 1024x1024. Wyeksportowano 6 s / 72 klatki / 960×540 / 12 FPS do archiwum r4b-driver-mirrors-6s.mp4. Weryfikacja zawartości każdego lusterka na klatkach pierwszej i późniejszej: środkowe 17 grup kolorów i 33 zmienione próbki, lewe 14 i 35, prawe 14 i 9; wszystkie bufory niepuste, animacja kamer realna. Dodano trwałe asercje do editor/tests/test_prawko_real_video.cpp: trzy osobne ROI wykluczające ramki, niepusta zawartość i zmiany obrazu; test opt-in (nie działa w zwykłym Qt headless), realny Windows/PRAWKO_VIDEO_PROOF_OUT; nowy przebieg r4b-mirror-regression-6s.mp4 i log r4b-regression-qt.log. Wydajność: prawdziwy ekran MainWindow/QOpenGLWidget na NVIDIA GeForce GTX 1050 Ti z FBO 4× MSAA przy 1600×900, 180 próbek per tryb/30 warmup; kopie z archive/a1-wiata-performance-project/ mają dokładnie 1 i 25 obiektów wiaty. 1 wiata: steady 59,95 FPS/p95 17,73 ms, Alt+LMB 59,97 FPS/p95 17,59 ms. 25 wiat: steady 59,95 FPS/p95 17,44 ms, Alt+LMB 59,95 FPS/p95 17,61 ms. W każdej serii 0 klatek powyżej 33,33 ms. Pliki r4b-fps-wiata{1,25}.json i r4b-mirror-regions.json w archive/a1-renderer-20261115. Jest to pomiar frameSwapped wymuszany co 16 ms, NIE maksymalny FPS silnika, NIE pomiar osobno samego GPU ani jawna weryfikacja cienia konkretnej wiaty w każdym lusterku podczas ruchu. Kontrole końcowe: Windows BUILD_PASS all, pełne Qt 1354 PASS / 7 SKIPPED / 0 FAIL, Quality Gate C++ 2074 PASS i agent Python 7 PASS, git diff --check; finalny commit/push i backup potwierdza osobny skrypt odbioru. **R4b nadal częściowo otwarty:** optyczna kontrola zbyt ciemnej przestrzeni pod dachem, brak rzeczywistych reflektorów i światła punktowego dla jazdy nocnej; test 25 wiat nie obejmuje pełnych scen miejskich ani ruchu na ekranie. R5 nadal PENDING.

**A1/R4b2 — reflektory pojazdu Ego i łagodne doświetlenie cieni — PASS w opisanym zakresie (15.11.2026).** Textured z aktywną symulacją Ego o zmierzchu/nocą: dwa światła stożkowe OpenGL poruszają się razem z autem i oświetlają rzeczywistą geometrię drogi, gruntu i modeli glTF (miękkie granice stożka, spadek z dystansem, limit 65 m), z uproszczoną reakcją glTF PBR. Delikatne doświetlenie cienia niebem 0,11 w dzień, 0,07 o świcie i 0,05 o zmierzchu nie zastępuje mapy cienia kierunkowego. Tryb Sober, sceny statyczne i noc bez cienia słonecznego nie otrzymują fałszywej poświaty. Kod: editor/src/render/renderer.hpp, gl_renderer.hpp, gl_renderer.cpp oraz editor/src/viewport/viewport_widget.cpp. Regresja: editor/tests/test_actor_rendering.cpp oraz test_prawko_real_video.cpp. Wymuszenie nocy tylko w teście przez PRAWKO_VIDEO_PROOF_FORCE_NIGHT; oryginalne XODR/XOSC i sidecary pozostały bez zmian. **Pomiary Windows OpenGL:** BUILD_PASS all; archiwum archive/a1-renderer-20261115/r4b2-fill-{close,day}.png (866×783), odpowiednio 5428 i 1340 próbkowanych co 4 pikseli rozjaśnionych, 0 przyciemnionych względem R4a; cienie dachu pozostały czytelne. Nocny ruch esmini 02-przejscie-pieszy: 6 sekund / 72 klatki / 960×540, trzy niepuste lusterka i zmieniające się widoki; nowy test nocny potwierdził 121 jasnych próbek nawierzchni i 1/1 PASS, film r4b2-night-headlights-verified.mp4 i log r4b2-night-headlights-verified.log. Pierwsza wersja testu zgłosiła FAIL przez nieadekwatne progi jasności lusterek z dnia; progi poprawiono, zachowując wymaganie niepustych, zmieniających się obrazów. Końcowe testy: Quality Gate 2074 C++ PASS, 7 Python PASS, QUALITY_GATE_PASS, pełne Qt PASS z pliku r4b2-full-qt.log. Commit/push i backup w osobnym skrypcie finalize. **Granice R4b2:** to nie fizyczne reflektory z własną mapą cieni; nie ma niezależnych reflektorów innych pojazdów, photometry/IES ani HDR/SSR/IBL. Pomiar wydajności po zmianie świateł i końcowy parytet Blender/Qt pozostają dla R5. R4b3: oddzielne światła innych pojazdów / occlusion shadows to nadal dodatkowy zakres PENDING.

5. **A1/R5: odbiór — PASS TECHNICZNY (15.11.2026), parytet fotometryczny nadal NIEPOTWIERDZONY.** Porównano bezpośrednio produkcyjny plik GLB z biblioteki importowany do Blendera 5.3/Cycles z tym samym GLB załadowanym przez PrawkoRoadStudio/Qt. Blender import: **156 mesh, 12 materiałów, rozmiar 4,960 × 2,350 × 2,769 m PASS**, zgodny z metadanymi. Raport archive/a1-renderer-20261115/r5-blender-imported-glb.json i niezależny render r5-blender-imported-glb.png. Renderer Windows OpenGL wygenerował z prawdziwych scen r5-qt-{close,day,night,rain}-accept.png (4 × 866×783, wszystkie PASS). Obrazy oceniono wizualnie: poprawna bryła, szkło, słupy, ławka, detale i UV; na deszczu widoczne opady i ciemniejsza nawierzchnia. Statystyczna średnia jasność kadru: close 74,46, day 87,70, rain 55,74, night 8,18 (0–255). Statyczna noc jest bardzo ciemna, bo nie ma aktywnego pojazdu; R4b2 esmini pokazuje realne światła Ego i trzy lusterka. **Blender i Qt nie są pikselowo ani fotometrycznie równoważne**: inny kąt/otoczenie, Cycles vs uproszczony GL PBR, widoczna różnica jasności dachu/metalu. Nie deklarować zgodności Cycles ani HDR/IBL, SSR, fizycznej transmisji szkła i własnych cieni reflektorów.

**R5 — wydajność i regresja (PASS).** W editor/tests/prawko_qt_fps_probe.cpp dodano opt-in benchmark day_driver / night_driver, uruchamiający realny QOpenGLWidget z kamerą kierowcy, trzema FBO lusterek i w nocy dwoma światłami Ego; pierwotny benchmark 1/25 wiat bez nowego argumentu zachowuje stare działanie. GTX 1050 Ti, 1600×900, 4× MSAA, po 180 próbek/30 warmup per mode: wiata 1 — 59,92 FPS steady/p95 17,56 ms, orbit 59,97/p95 17,72; wiata 25 — 59,97/p95 17,61, orbit 59,97/p95 17,64; kierowca i trzy lustra dzień — 59,78/p95 16,98, drugi tryb 59,95/p95 16,89; kierowca i trzy lustra noc z reflektorami — 59,78/p95 17,21, drugi tryb 59,95/p95 17,08. Każda seria 0 klatek >33,33 ms. Archiwum r5-fps-{wiata1,wiata25,driver-day,driver-night}.json i logi. **To pomiar kadencji frameSwapped przy timerze 16 ms, NIE maksymalny FPS GPU ani 180 klatek animowanego esmini.** Dynamiczne esmini/FFmpeg (R4b1/R4b2) zweryfikowano osobno. Pierwszy pełny build zatrzymał się w post-build gtest_discover_tests; samodzielne ponowienie odkrywania testów przy Qt offscreen PASS, następny cały Windows build PASS — bez zmiany kodu silnika. Odbiór jakości: Windows BUILD_PASS all, Qt 1355 PASS/7 SKIPPED/0 FAIL, C++ 2074 PASS (10 disabled), Python 7 PASS, QUALITY_GATE_PASS, git diff --check, osobny commit/push/backup. **Poza tym odbiorem:** R4b3 reflektory innych pojazdów/cienie od lamp, HDR/IBL, pełna fizyczna refrakcja i bliższy parytet oświetlenia Blender/Qt; nie nazywać tego photoreal PASS.

**A1/R4b3a — reflektory najbliższego pojazdu ruchu (16.11.2026, PASS TECHNICZNY):** Dodano drugą, niezależną parę świateł dla najbliższego samochodu innego niż Ego (maks. 130 m), obliczaną z jego transformacji, wymiarów i kierunku w symulacji; shader oświetla geometrię, bez ekranu-emisji. Wyłącznie Textured podczas prowadzenia Ego o zmierzchu/nocą; statyczny widok, dzień, Sober i piesi bez dodatkowego światła. Zmiany w renderer.hpp, gl_renderer.hpp/.cpp, viewport_widget.hpp/.cpp i test_actor_rendering.cpp. **Potwierdzone:** Windows `BUILD_PASS: all` i `AGENT_BINDING_SYNC_PASS`; celowane Qt 2/2 PASS, pełne Qt **1356 PASS / 7 SKIPPED / 0 FAIL**; natywne C++ **2074 PASS**, agent Python **7 PASS**, `QUALITY_GATE_PASS`. Rzeczywisty nocny test OpenGL/esmini `01-miasto-lusterka` (3 samochody): **72 klatki MP4 960×540 / 6 s, 3 lusterka PASS**, 106 jasnych próbek jezdni; logi `archive/a1-renderer-20261115/r4b3a-{night-video,quality-gate,full-qt,targeted-qt}.log`, film `r4b3a-traffic-headlights-night.mp4`, sprawdzony wyodrębniony kadr `r4b3a-night-frame2.png`. **FPS Windows GTX 1050 Ti, 1600×900 4×MSAA:** ~59,82 FPS i p95 17,96 ms (driver_refresh), ~59,95 FPS i p95 17,06 ms (pointer), 180 próbek/mode i **0 >33,33 ms**, `r4b3a-fps-driver-night.json`; porównywać tylko jako synchronizowaną kadencję Qt, nie GPU maximum. Obrazy potwierdzają działający nocny renderer, lecz **izolowana różnica jasności z włączonym/wyłączonym światłem pojazdu ruchu NIE jest jeszcze obiektywnie zmierzona** — nie nazywać tego fotometrycznym odbiorem. `projects/Prawko-Scenariusze/project.json` automatycznie zmieniony przez probe, przywrócono oryginalne `last_scene=04-zima-mgla.xodr`; XODR/XOSC nietknięte. R4b3 cienie od reflektorów pozostałych pojazdów i wiele jednoczesnych źródeł światła nadal poza tym podetapem.

**16.11.2026 — publiczna galeria renderów (6 PNG, przygotowana do publikacji):** Cztery świeże zrzuty rzeczywistego `roadmaker-editor.exe --screenshot` na Windows OpenGL 3.3 (wiata: zbliżenie, dzień, deszcz, statyczna noc), osobny kadr 2,0 s z zatwierdzonego nocnego MP4 `r4b3a-traffic-headlights-night.mp4` (kokpit i trzy lustra), oraz oryginalny render Blender Cycles R5. Katalog `docs/gallery/prawkoroadstudio-2026-11-16/` zawiera sześć PNG i README z opisem pochodzenia. Rzeczywiste zrzuty Qt mają 866×783, kadr MP4 960×540; nie sugerować rozdzielczości 1440×900 tylko na podstawie argumentu CLI. Statyczna noc jest celowo bardzo ciemna bez aktywnego pojazdu; nie ma parytetu fotometrycznego Blender/Qt. Projekt/sceny użytkownika bez zmian. Po zatwierdzeniu i synchronizacji należy podać publiczny adres galerii.

**A1/R4b3b — nocne porównanie świateł innych pojazdów (16.11.2026, PASS A/B GPU):** Dodano opt-in test GPU: `PRAWKO_VIDEO_PROOF_COMPARE_TRAFFIC` uruchamia dwa zrzuty tej samej klatki esmini (t=2 s, te same pozy/kamera/noc), zmieniając jedynie `PRAWKO_VIDEO_PROOF_NO_TRAFFIC_LIGHTS`. Test zapisuje obrazy `*.traffic-on.png`/`*.traffic-off.png`, liczy liczbę rozjaśnionych/przyciemnionych pikseli i weryfikuje rzeczywisty wzrost jasności. Światła Ego i zapisane sceny pozostają bez zmian. **Potwierdzone w natywnym Windows:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, Qt **1356 PASS / 7 SKIPPED / 0 FAIL**, C++ **2074 PASS**, Python **7 PASS**, `QUALITY_GATE_PASS`. GPU Windows OpenGL 3.3 + esmini `01-miasto-lusterka` (3 samochody, t=2 s), 960×540: **7663 próbkowane piksele jaśniejsze, 0 ciemniejszych**, w identycznych pozycjach/kadrze; test `PrawkoRealVideo.DriverViewStreamsActualTrafficAndMirrorsToMp4` 1/1 PASS, MP4 72 klatki/6 s i trzy lusterka PASS. Zrzuty `archive/a1-renderer-20261115/r4b3b-night-ab.mp4.traffic-{on,off}.png`, logi `r4b3b-{ab-build,full-qt,quality,night-ab}.log`, film `r4b3b-night-ab.mp4`. Nie jest to pomiar zgodności fotometrycznej i **nie ma jeszcze map cieni od reflektorów**. Sceny XODR/XOSC nie były edytowane. Własne cienie reflektorów oraz R6 wizualne otoczenie pozostają kolejnymi zadaniami, nie deklarować PASS do potwierdzenia testami.

**A1/R6a — autorska kamienica miejska i scena wzorcowa (16.11.2026, PASS PROTOTYPU):** Blender 5.3 wygenerował model `assets/prawko/blender/kamienica_miejska_r6/r6_kamienica_miejska.glb`, źródło `.blend`, generator `.py`, teksturę tynku i obraz Cycles. Model ma rzeczywiste wnęki okienne, szkło, balkony, drzwi, dach, rynny; wymiary ok. 14,7 × 10,79 × 13,64 m. **Pierwszy nieudany import** wynikał z przekroczenia limitu 256 części (271 mesh); poprawiono model przez łączenie geometrii wg 9 materiałów bez utraty bryły i detali. Po ponownym eksporcie **rzeczywisty renderer Qt pokazał obie kamienice w osobnej scenie** `06-r6-otoczenie-prototyp.xodr` (z XOSC i sidecar), zdjęcie `archive/a1-renderer-20261115/r6-kamienice-qt-front.png` (866×783). Zarejestrowano model i miniaturę w istniejącej bibliotece; wcześniejsze sceny 01–05 bez zmian. **Potwierdzone:** Blender 5.3 `PRAWKO_R6_BUILDING_BLENDER_PASS`, rzeczywisty Windows OpenGL 3.3 `r6-kamienice-qt-front.png` (866×783, dwie widoczne kamienice), Qt drag/drop+zmiana s i heading+zapis/ponowny odczyt GLB→XODR **1/1 PASS**, Windows `BUILD_PASS: all` + `AGENT_BINDING_SYNC_PASS`, pełne Qt **1357 PASS / 7 SKIPPED / 0 FAIL**, Quality Gate natywne C++ **2074 PASS** i agent Python **7 PASS**, `QUALITY_GATE_PASS`. Archiwum `archive/a1-renderer-20261115/r6-kamienica-{build,targeted-qt,full-qt,quality}.log`; prawdziwe zrzuty i źródłowy model również w `docs/gallery/prawkoroadstudio-2026-11-16/` (Qt i Blender). **To tylko pierwszy poprawny asset R6a, nie odbiór całego realistycznego otoczenia:** jednolita płaska zieleń, puste pobocza, prymitywne pojazdy i brak wiarygodnego kontekstu ulicy pozostają do R6b; efekty Blender Cycles nie są fotometrycznie równoważne z Qt. Testy wykonano na oddzielnej nowej scenie 06 i na niezależnym `QTemporaryDir`, sceny 01–05 i XODR/XOSC użytkownika nienaruszone. Po walidacji poprawiać wyraźnie płaskie otoczenie: teren, pobocza, budynki i zieleń na scenie wzorcowej.

**A1/R6b1 — obiekty strefy przydrożnej i scena testowa (16.11.2026, PASS TECHNICZNY):** Skrypt `assets/prawko/blender/ulica_r6b/create_ulica.py` generuje trzy oddzielne autorskie modele Blender 5.3/GLB: `r6b-chodnik-20m` (geometria osobnych płyt, rzeczywiste spoiny, obrzeża, odwodnienie), `r6b-drzewo` (misa, pień, konary, wielosegmentowa korona i trawa jako siatka), `r6b-latarnia` (stalowy słup i oprawa; **bez funkcjonalnego źródła światła w shaderze**). Modele wstawiono do tej samej kategorii bibliotek z indywidualnymi miniaturami. Powstała **odrębna** scena `projects/Prawko-Scenariusze/06b-r6b-otoczenie-wzorcowe.{xodr,xosc,rmscene.json}` na kopii R6a (oryginalne sceny nietknięte): 4 moduły chodnika przy elewacjach kamienic, 4 drzewa, 3 lampy. Windows Qt/OpenGL 3.3 wyrenderował faktycznie `archive/a1-renderer-20261115/r6b-qt-prototype.png` (866×783), ładowanie 1 road/0 diagnostics. **Odbiór potwierdzony:** Blender 5.3 `PRAWKO_R6B_THREE_EXPORTS_PASS`, 3 GLB: chodnik 20 m/7 części, drzewo/8 części, latarnia/3 części; oryginalne `.blend`, generator, metadane i miniatury obecne. Windows `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`; docelowy Qt `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved` **1/1 PASS**: trzy osobne przeciągnięcia, zmiana pozycji/obrotu, zapis/odczyt XODR w izolowanym projekcie. Pełne Qt **1358 PASS / 7 SKIPPED / 0 FAIL**, core C++ **2074 PASS**, agent Python **7 PASS**, `QUALITY_GATE_PASS`. Natywny Qt/OpenGL 3.3: scena `06b` 1 droga, 0 diagnostyk ładowania, screenshoty **866×783** `archive/a1-renderer-20261115/r6b-{qt-prototype,qt-close-day,qt-close-rain,qt-same-r6a-camera}.png`; w galerii repo `docs/gallery/prawkoroadstudio-2026-11-16/` kopie PNG i zestawienie R6a/R6b tej samej kamery. FPS Windows NVIDIA GTX 1050 Ti, 1600×900 4×MSAA, 180 klatek + warmup na tryb: R6a steady 60,01 FPS/p95 17,60 ms, orbit 59,78/p95 17,02; R6b steady 60,00 FPS/p95 17,74 ms, orbit 60,02/p95 17,81; **0 klatek >33,33 ms** we wszystkich czterech próbach. To kadencja frameSwapped z ograniczeniem ~60 Hz, a NIE maksymalny FPS GPU. Logi `archive/a1-renderer-20261115/r6b-{build,qt-targeted,qt-full,quality,fps-*}.log` i JSON `r6b-fps-{r6a-baseline,r6b-streetfront}.json`. Test GUI na sztucznej prostej scenie po zapisaniu raportował 3 diagnostyki przy sąsiadujących obiektach (nie badano ich kategorii); rzeczywista scena 06b ładuje się z 0 diagnostykami. **R6b1 jest odebranym prototypem, a pełny R6 nadal OTWARTY.** Duża pusta zieleń, uproszczone sylwetki drzew, brak realistycznych pojazdów oraz świetlne działanie latarni pozostają otwarte; nie nazywać fotorealistycznym R6. Oryginalne materiały Blender, nową scenę oraz rzeczywiste zrzuty Qt przygotowano do wersjonowania razem z tą zmianą, w tym porównanie R6a/R6b z tej samej kamery. Po zatwierdzeniu potwierdzić PUSH i `scripts/prawko-git.ps1 -Action finish` oraz podać linki publiczne. Następny krok **R6b2**: rzeczywista struktura dużej połaci terenu, wypełnienie otoczenia bez sztampy, bardziej naturalna roślinność, docelowy model samochodu i osobno funkcjonalne oświetlenie latarni; R7 dopiero po odbiorze wzorcowej ulicy.

**A1/R6b2a — samochód aktora zamiast pomarańczowej bryły (16.11.2026, PASS TECHNICZNY):** Blender 5.3 wygenerował oryginalny model z siedmioma materiałami i rzeczywistymi kołami, szybami, lampami i lusterkami; skrypt, .blend, .glb, metadane i Cycles są w assets/prawko/blender/samochod_r6b2/. GLB ma 658 152 B, wymiary około 4,591 × 1,919 × 1,425 m (z lusterkami). Model jest **low-poly, NIE fotorealistyczny**. Dodano do biblioteki projektu z miniaturą, a nową scenę 06c-r6b2-samochody.{xodr,xosc,rmscene.json} skopiowano z R6b1 bez modyfikacji scen 01–06b.

**Integracja rzeczywistej geometrii w scenie i esmini:** editor/src/render/scene_builder.cpp/.hpp — tylko aktorzy OpenSCENARIO vehicleCategory=car i tylko przy poprawnie zarejestrowanym GLB korzystają z oryginalnych części modelu zamiast rm:actor_box; importowane współrzędne normalizowane do OSC [-0,5;+0,5]³, zachowany ten sam aktorowy transform (rear-axle center offset, yaw/pitch/roll, długość/szerokość/wysokość). Pedestrian/Truck, brak GLB i inne projekty mają stary box fallback. viewport_widget.cpp/.hpp zachowuje materiały GLB i rozdziela aktorów w czasie przejazdu esmini od statycznego położenia XOSC; istniejące reflektory i lusterka pozostają na ścieżce symulacyjnej. Nie implementowano rotacji kół podczas jazdy ani fizycznej refrakcji szkła.

**Potwierdzenia:** Windows BUILD_PASS: all, AGENT_BINDING_SYNC_PASS; celowane Qt 2/2 PASS, w tym nowy model w bibliotece dodany myszą, przesunięty, obrócony i ponownie odczytany z XODR. Full Qt 1359 PASS / 7 SKIPPED / 0 FAIL, core C++ 2074 PASS, agent Python 7 PASS, QUALITY_GATE_PASS. Test ActorRenderingR6b2.ProjectCarGlbReplacesOnlyCarBoxesAndKeepsOscTransform potwierdził 7 części GLB, dopasowaną obwiednię i zachowanie oddzielnej reprezentacji Truck. Realny OpenGL 3.3 z Windows wygenerował archive/a1-renderer-20261115/r6b2-qt-overview.png i r6b2-qt-car-close.png (oba 866×783), scenę ładowano jako 1 road / 0 diagnostics; samochód widoczny zamiast pomarańczowej bryły. Opt-in dynamiczny esmini+Qt+FFmpeg na scenie 06c: 1/1 PASS, 72 klatki MP4 / 6 s / 960×540 / 12 FPS, wszystkie trzy lusterka niepuste i zmienne; pliki archive/a1-renderer-20261115/r6b2-live-car-esmini-6s.mp4, r6b2-live-car-esmini.log oraz *.first.png, *.moving.png. Autentyczna galeria i porównanie tej samej kamery w docs/gallery/prawkoroadstudio-2026-11-16/. Test GUI na celowo nachodzących na siebie obiektach na sztucznej scenie zgłosił 6 diagnostyk po zapisaniu; właściwa scena 06c ma 0 diagnostyk ładowania.

**Wydajność na rzeczywistym Windows NVIDIA GeForce GTX 1050 Ti:** 1600×900, 4×MSAA; probe R6b2 (180 próbek na tryb) steady 59,77 FPS / p95 17,34 ms, Alt+LMB 60,00 FPS / p95 17,13 ms, **0 klatek ponad 33,33 ms**. Wyniki archive/a1-renderer-20261115/r6b2-fps-car.json i .log; jest to kadencja Qt z synchronizacją około 60 Hz, NIE maksymalny FPS GPU. Oryginalny projekt.json i sidecar zachowano bez zmian po testach. **Dalej:** R6b2b — monotonny, rozległy zielony teren wymaga wiarygodnych nawierzchni, mikrogeometrii, ścieżek i urozmaiconej roślinności; następnie kolejne sylwetki pojazdów oraz animacja kół i lampy w nocy. Nie deklarować pełnego R6 PASS ani fotorealizmu. Commit i PUSH muszą być potwierdzone w git log i origin, backup obu repozytoriów w scripts/prawko-git.ps1 -Action finish; linki do nowych renderów podawać tylko po publicznym HTTP 200.

**A1/R6b2b — teren i zieleń wzorcowej ulicy (16.11.2026, PASS TECHNICZNY):** Przygotowano trzy nowe ORYGINALNE modele źródłowe w Blender 5.3, wyeksportowane jako GLB w `assets/prawko/blender/teren_r6b2b/`: `r6b2b-pas-zieleni-20m` (20×2,98×0,841 m, 10 części: nierówna darń, kępy trawy w prawdziwej geometrii, ściółka, 6 krzewów), `r6b2b-rabata-ogrodowa` (14×6,493×0,991 m, 11 części: nieregularne podłoże i nasadzenia) oraz `r6b2b-sciezka-zwirowa-12m` (12×1,618×0,23 m, 6 części: prawdziwe drobne kamienie i obrzeża). Błąd wysokości gałązek podczas grupowego łączenia obiektów w Blenderze wychwycono przed importem; gałęzie przepisano na bezpośrednią geometrię world/model space, a kontrola końcowych wymiarów jest asercją generatora. Każdy asset ma kod generatora, źródłowy .blend, plik .glb, render Cycles i metadane. Modele dodano do istniejącej biblioteki drag-and-drop z trzema miniaturami, bez zastępowania starego zestawu.

**Nowa odrębna scena:** `projects/Prawko-Scenariusze/06d-r6b2b-naturalne-otoczenie.{xodr,xosc,rmscene.json}`, skopiowana z 06c, zawiera **9 nowych obiektów**: pięć pasów zieleni z dala od chodnika, dwie rabaty za kamienicami i dwie ścieżki. Wszystkie wcześniejsze sceny 01–06c pozostają nienaruszone. Oprócz GLB zmieniono wyłącznie proceduralny shader dużej płaszczyzny trawy w `editor/src/render/gl_renderer.cpp`, w trybie Textured: 4 skale szumu w metrach świata (wielkość płatów, wilgotniejsze i suchsze strefy, bliskie detale), przy zachowaniu sezonowości, śniegu, cieni i reflektorów. Shader nadal renderuje płaską powierzchnię podłoża, a jego szum NIE jest geometrycznymi kępami — te pochodzą z GLB. To pierwszy etap naturalizacji, NIE fotorealizm; nadal widać regularność modułów i proste korony drzew z R6b1.

**Odbiór rzeczywisty Windows:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, docelowy test GUI `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved` **1/1 PASS** po rozszerzeniu na wszystkie siedem assetów (Qt MIME/drop, przesunięcie, obrót, zapis i odczyt). W sztucznym fixture GUI, gdzie obiekty upuszczano w zbliżonej pozycji, zapis ostrzegał o 21 diagnostykach; to NIE raport o produkcyjnej scenie. Właściwa 06d: **1 droga / 0 diagnostyk ładowania**. Pełne Qt **1359 PASS / 7 SKIPPED / 0 FAIL**, native C++ **2074 PASS**, Python agent **7 PASS**, `QUALITY_GATE_PASS`. Prawdziwy Windows/OpenGL 3.3 wygenerował trzy screenshoty 866×783 w `archive/a1-renderer-20261115/r6b2b-qt-{overview,pas-zieleni,ogrody}.png`; kopie i wizualizacja porównania R6b2a/R6b2b tej samej kamery oraz trzy Cycles trafiły do `docs/gallery/prawkoroadstudio-2026-11-16/`. Rzeczywisty NVIDIA GeForce GTX 1050 Ti, 1600×900, 4× MSAA, 180 klatek + rozgrzewka na tryb: 06c baseline 59,93 FPS / 59,71 FPS, R6b2b 59,75 FPS / 59,95 FPS (steady / orbit), **0 klatek powyżej 33,33 ms** we wszystkich 4 pomiarach. Są to pomiary kadencji frameSwapped przy ~60 Hz, nie maksymalny FPS GPU. Logi `archive/a1-renderer-20261115/r6b2b-{blender-fixed,build,targeted-qt,full-qt,quality,fps-*}.log`, JSON `r6b2b-fps-{r6b2a,r6b2b}.json`. **Następnie:** zróżnicować moduły zieleni, poprawić korony drzew, zaprojektować realistyczne zakończenie terenu i chodników oraz odrębnie udoskonalić materiały i ogólny poziom detalu. Nie oznaczać całego R6 jako zakończony.

**A1/R6b2c — drzewa i strefy przy zabudowie (16.11.2026, PASS TECHNICZNY):** Poprzedni czysty HEAD przed zmianą `6179564fd601bc10f78e870ed9e54e8ab77de262`. W `assets/prawko/blender/drzewa_r6b2c/` utworzono **trzy autorskie, edytowalne modele GLB** ze źródłami Blender 5.3 `.blend`, generatorem `create_roslinnosc.py`, metadanymi i rzeczywistymi renderami Cycles: lipa uliczna **8,917 m / 9 części / GLB 1 280 636 B**, brzoza **9,756 m / 10 części / GLB 1 176 752 B** i rabata przy tylnej elewacji **12,129 × 1,64 × 0,516 m / 9 części**. Korony są zbudowane z osobnych geometrycznych liści i gałęzi zamiast kulistych brył. Po pierwszej recenzji rzeczywistego obrazu Qt podwojono gęstość liści i dostosowano ich wielkość; druga iteracja została ponownie wyeksportowana i jest podstawą odbioru. Model glTF sprawdzono także pod kątem obecności dwustronnych materiałów liści (9/10/9 części). **Nie twierdzić, że modele są fotorealistyczne** — to celowane ulepszenie wizualne względem uproszczonego R6b1.

**Scena i interakcje:** `projects/Prawko-Scenariusze/06e-r6b2c-drzewa-i-podworka.{xodr,xosc,rmscene.json}` to nowa oddzielna kopia `06d`, bez modyfikacji wcześniejszych scen. Cztery dotychczasowe drzewa wymieniono wyłącznie w `06e` na 2 lipy i 2 brzozy; dodano piątą brzozę na obrzeżu podwórza i dwie rabaty przy tylnych ścianach kamienic. Dwa odcinki istniejącej 12-metrowej ścieżki ustawiono poprzecznie (hdg π/2), by tworzyły przejście od chodnika do podwórza między budynkami. Wymiary/radius/height są zsynchronizowane z **finalnymi** metadanymi GLB. Biblioteka Prawko-Scenariusze ma teraz 12 pozycji z trzema nowymi miniaturami. Test `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved` **1/1 PASS** po rozszerzeniu na 10 modeli: rzeczywiste Qt MIME drag/drop, przesunięcie, obrót, zapis i odczyt XODR. W syntetycznym fixture GUI, gdzie modele są celowo upuszczane blisko siebie, jest 43 diagnostyk kolizji/rozmieszczenia; to **nie** wynik docelowej sceny, która ładuje się z **1 road / 0 diagnostics**.

**Odbiór Windows/OpenGL:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, Qt **1359 PASS / 7 SKIPPED / 0 FAIL**, core C++ **2074 PASS**, agent Python **7 PASS**, `QUALITY_GATE_PASS`. Rzeczywisty Qt/GL 3.3 wygenerował trzy zrzuty po finalnej iteracji **866×783**: `archive/a1-renderer-20261115/r6b2c-qt-{calosc,drzewa,podworka}.png`. Galeria `docs/gallery/prawkoroadstudio-2026-11-16/` zawiera kopie PNG, porównanie obrazu 06d/06e z identycznej kamery oraz trzy samodzielne renderingi Blender (odróżnione od Qt). NVIDIA GTX 1050 Ti, 1600×900, 4× MSAA, 180 próbek po rozgrzewce na tryb: **R6b2b baseline 59,70/59,95 FPS**, **R6b2c 59,73/59,95 FPS** (steady/orbit), w żadnej z czterech prób nie było klatek powyżej 33,33 ms. Pomiar oznacza kadencję Qt frameSwapped ~60 Hz, NIE maksymalne FPS GPU. Logi: `archive/a1-renderer-20261115/r6b2c-{build,qt-targeted,qt-full,quality,fps-*}.log` i JSON. **Ograniczenia:** modele nadal uproszczone, okolica poza najbliższą ulicą wciąż bardzo pusta, 3D trawa i rabaty pozostają modularne, noc i deszcz nie były ponownie odbierane w R6b2c, test animowanego esmini nie był ponawiany. Poprzedni R6b2a zachował potwierdzony przejazd esmini. **Następny krok R6b2d:** dopracowanie detali zabudowy i przejść materiałów PBR (płyty/ziemia/fundament/zieleń), nieregularności otoczenia przy zachowaniu wydajności GPU, ponowny test noc/deszcz. W całym R6 nadal nie deklarować finalnego fotorealizmu. `agent_head=67a92fa24b4054b7f094c9b667f3634c35ad9210`.

**A1/R6b2d — granice chodnik–ziemia i przejście z mapami PBR (16.11.2026, PASS TECHNICZNY, NOCNA CZYTELNOŚĆ OTWARTA):** Wejściowy czysty HEAD edytora `4822adc99972704a90ffa5dfb0a60989b115d6d2`, agent `67a92fa24b4054b7f094c9b667f3634c35ad9210`. W `assets/prawko/blender/styki_r6b2d/` są **dwa nowe autorskie źródła Blender 5.3** i GLB: `r6b2d-przejscie-plyty-12m` (12-metrowe przejście piesze z fizycznymi płytami, fugami, fazowanymi krawędziami, odwodnieniem, kępkami trawy; GLB 1 572 824 B, 7 części) oraz `r6b2d-styk-chodnik-ziemia-20m` (20-metrowe obrzeże z nieregularną glebą i trawą; GLB 929 464 B, 5 części). `create_styki.py` generuje również **oryginalne** sześć map PNG 256×256 (albedo, tangent-space normal, ORM dla betonu i ziemi), zapisuje źródła `.blend`, render Cycles oraz metadane, a GLB osadza tekstury. Parser glTF sprawdził w obu assetach obecność UV0 oraz wskaźników `baseColorTexture`, `normalTexture`, `metallicRoughnessTexture` (pierwszy GLB: 3 obrazy; drugi: 6). **Test faktycznego importera C++ `PropAssetImport.PrawkoR6b2dAuthoredPbrAssetsPreserveUvNormalAndOrm` PASS** potwierdził zachowanie tekstur i UV po imporcie, bez spłaszczania kolorów. Nie twierdzić, że powstały fizyczne odbicia SSR lub fotogrametria.

**Scena i GUI:** Nowa, osobna `projects/Prawko-Scenariusze/06f-r6b2d-nawierzchnie-pbr.{xodr,xosc,rmscene.json}`, 30 obiektów, stworzona bez zmiany wcześniejszych scen. Zastąpiono **tylko w 06f** dwa odcinki ścieżki (ID 908/909) nowym przejściem PBR; cztery odcinki przejścia chodnik–ziemia (ID 970–973, s=50/70/90/110, t=8,94) umieszczono przy krawędzi chodnika. Zachowano istniejące identyfikatory i materiał sceny 06e. Biblioteka projektu liczy 14 wpisów, dwa nowe modele z miniaturami i edycją myszą. GUI `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved` **1/1 PASS** rozszerzony do 12 modeli (Qt drag/drop, pozycja, obrót, zapis/odczyt); sztuczny testowy projekt celowo z wieloma blisko upuszczonymi obiektami zgłaszał 62 diagnostyki rozmieszczenia, **nie** dotyczy to 06f, która ładuje się z **1 drogą i 0 diagnostykami**.

**Odbiór:** Win `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, celowane testy Qt **2/2 PASS**, pełne Qt **1360 PASS / 7 SKIPPED / 0 FAIL**, core C++ **2074 PASS**, agent Python **7 PASS**, `QUALITY_GATE_PASS`. Natywny Windows Qt/OpenGL 3.3 wygenerował rzeczywiste PNG **866×783** dla `06f`: `archive/a1-renderer-20261115/r6b2d-qt-{calosc-dzien,przejscie-dzien,przejscie-deszcz,przejscie-noc,styk-chodnika}.png`; archiwizowano też porównanie 06e/06f z tej samej kamery i dwa odrębne rendery Cycles w `docs/gallery/prawkoroadstudio-2026-11-16/`. Deszcz zmienia materiały i pokazuje opady, ale nie ma pełnych refleksów powierzchni mokrych. **Noc: render działa, lecz dojście do podwórza pozostaje zbyt ciemne, ponieważ latarnie są siatką 3D bez emisji światła — NOT VISUAL PASS; nie oznaczać całej nocy jako odebranej.** FPS rzeczywista NVIDIA GTX 1050 Ti, 1600×900, 4× MSAA, 180 próbek + warmup na tryb: `06e` 59,73/59,95 FPS, `06f` 59,74/59,95 FPS (steady/orbit); **0 klatek >33,33 ms** we wszystkich czterech próbach. Ograniczenie ~60 Hz Qt frameSwapped, to NIE maksymalny FPS GPU. Logi `archive/a1-renderer-20261115/r6b2d-{blender,build,qt-targeted,qt-full,quality,fps-*}.log`, JSON `r6b2d-fps-{r6b2c,r6b2d}.json`.

**Następny krok R6b2e (otwarty):** prawdziwe światło latarni, powiązane z oprawami i trybem nocnym, oświetlające dojście, grunt i elementy GLB (bez sztucznej poświaty), z odbiorem A/B noc/w dzień i testem regresyjnym cieni/wydajności; później poprawa spójności granic teren–elewacja i mokrych materiałów. **Pełny R6 nadal otwarty.**

**A1/R6b2e — rzeczywiste światło latarni ulicznych, 16.11.2026 (PASS TECHNICZNY; cienie od latarni OTWARTE):** Czysty HEAD przed zmianą `91ec407b103ffa1b9a6d5f483d5d6a0797cd521c`; agent `67a92fa24b4054b7f094c9b667f3634c35ad9210`. Dodano bezpośrednie światła z **istniejącego GLB `r6b-latarnia`**, a nie punktów o wpisanej na sztywno pozycji w scenie. W `editor/src/render/renderer.hpp` funkcja `street_lamp_from_prop(InstanceData)` przenosi lokalny punkt klosza (0, 1,50, 5,43 m) do świata po zmianie pozycji, obrotu i skali słupa; kierunek światła jest wyliczany z transformacji. `ViewportWidget` pobiera pozycje instancji modelu i wybiera **maksymalnie 4 najbliższe latarnie** do aktualnej kamery, włącza je tylko w trybie Textured o zmierzchu/nocą, bez wpływu na dzień i Sober. `GLRenderer` używa identycznych światów źródeł do shaderów GL 3.3 podłoża i geometrii (w tym autorskich GLB/PBR): wiązka skierowana w dół, ciepłe światło, ograniczona odległość 24 m, tłumienie i dyfuzja zależna od normalnej. Intensywność noc 3,0, zmierzch 1,3 (jednostki modelu; **NIE fotometria ani IES**). Zmienna dowodowa `PRAWKO_R6B2E_PROOF_DISABLE_STREETLIGHTS` wyłącza jedynie nowe źródła podczas testu A/B, nie zmienia sceny ani świateł auta.

**Scena:** nowa, odrębna `projects/Prawko-Scenariusze/06g-r6b2e-latarnie-noc.{xodr,xosc,rmscene.json}` to kopia 06f z **dwoma dodatkowymi latarniami** (ID 980/981, okolica dojścia między budynkami), razem **32 obiekty i pięć instancji modelu latarni** (shader wybiera do 4 na kamerę). Poprzednie sceny 01–06f niezmienione; edycja/synchronizacja instancji podąża za istniejącym mechanizmem przesuwania i obracania. Nowy test CPU `RenderLightingR6b2e.StreetlampEmitterFollowsPhysicalFixtureAndPropRotation` potwierdza przesunięcie, obrót 90°, zmianę skali i wyłączony stan domyślny. Test GUI dwunastu modeli `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved` pozostał PASS; sztuczny fixture GUI podaje 62 diagnostyki gęsto upuszczonych obiektów, **nie** jest to 06g, która ładuje się z **1 road / 0 diagnostics**.

**Dowód A/B na rzeczywistym Windows:** `archive/a1-renderer-20261115/r6b2e-qt-{wide-day-off,wide-day-on,wide-night-off,wide-night-on,path-night-off,path-night-on,path-dusk-on,path-rain-night-on}.png` (8 rzeczywistych screenshotów OpenGL 3.3 **866×783**). Ten sam projekt, kamera, pora i pogoda w A/B, różnica tylko w kontrolnym wyłączeniu światła. Kontrola pikseli `r6b2e-visual-gate.ps1` (próbkowanie co 3 piksele): **DZIEŃ identyczny (avg ΔRGB=0,000, 0 zmienionych próbek)**, **NOC szeroki kadr avg Δ=3,163, 10 276 istotnych próbek / 75 429**, **NOC dojście avg Δ=3,692, 13 188 istotnych / 75 429** — `R6B2E_VISUAL_A_B_GATE_PASS`. Widać rzeczywiste oświetlenie jezdni, chodników, trawnika i płyt. W `docs/gallery/prawkoroadstudio-2026-11-16/` porównania `r6b2e-porownanie-noc-{calosc,przejscie}.png` plus 8 surowych obrazów. Test na GTX 1050 Ti, 1600×900, 4× MSAA, 180 próbek + warmup: **latarnie OFF steady 59,70 / orbit 59,95 FPS**, **ON steady 59,72 / orbit 59,95 FPS**, 0 klatek >33,33 ms we wszystkich 4 pomiarach, `R6B2E_GPU_FPS_A_B_PASS`. Wyniki to synchronizacja Qt ~60 Hz, nie maksymalny FPS GPU.

**Quality Gate:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`; celowane testy CPU/GUI **2/2 PASS**, pełne Qt **1361 PASS / 7 SKIPPED / 0 FAIL**, C++ **2074 PASS**, Python **7 PASS**, `AGENT_BINDING_PARITY_PASS`, `QUALITY_GATE_PASS`. Podczas pierwszego uruchomienia kontrolnego skrypt PowerShell błędnie traktował oczekiwany stderr `libpng error: Read Error` z testu uszkodzonego obrazu jako błąd wykonania; skrypt lokalny poprawiono na obsługę wyjść testów przez exit code i ponowiono **pełny pakiet z PASS**. Logi `archive/a1-renderer-20261115/r6b2e-{build,qt-targeted,qt-full,quality,fps-*}.log`, JSON `r6b2e-fps-{off,on}.json`.

**Ograniczenia i kolejny etap:** nowe źródła oświetlają prawdziwą geometrię, ale **nie rzucają osobnych cieni od latarni/nie mają testu occlusion**, więc światło może przenikać przez ściany; brak fizycznych odbić mokrego chodnika i IES. **R6b2f OTWARTE:** własne cienie przeszkód w świetle latarni (koszt GPU i test nocny A/B), realny wygląd świecących opraw oraz dopracowanie mokrych materiałów z zachowaniem wydajności i regresji dnia/esmini. Nie oznaczać całego R6 jako fotorealistycznie zakończonego.

**A1/R6b2f — własne mapy cieni dla rzeczywistych latarni (16.11.2026, PASS TECHNICZNY, dalszy realizm R6 otwarty):** Wejściowy czysty HEAD `bf9cd95b2ef59e4931a1140dbb9adf557a1cc50e`, agent `67a92fa24b4054b7f094c9b667f3634c35ad9210`. W `editor/src/render/gl_shadow.cpp` rozszerzono istniejący depth-only OpenGL 3.3 renderer o **4 niezależne mapy głębi 512×512** dla maksymalnie czterech najbliższych latarni, opartych na ich rzeczywistym kierunku i położeniu wyliczanym z instancji GLB `r6b-latarnia`; osobna dzienna mapa kierunkowa **1024×1024 pozostaje**. Ponownie używany przebieg `draw_depth_casters` wspiera instancjonowanie i przezroczystość typu MASK (pomija BLEND). Perspektywiczny projektor 144°, bias depth i 2×2 PCF w shaderach geometrii i podłoża ograniczają bezpośredni składnik światła tam, gdzie przesłaniają go obiekty. `StreetLights::cast_shadows` domyślnie false; w `ViewportWidget` włącza się **tylko** w trybie Textured o zmierzchu/nocą, gdy istnieją realne latarnie. Dzienne i Sober, headlighty oraz sceny bez latarni zachowują stan wyłączony. Ostateczny shader ma **ten sam stożek 0,37–0,61 przy A/B OFF i ON**. Ograniczenie: to lokalne mapy cieni przy 4 lampach, nie ray tracing, mapy IES ani pełna global illumination.

**Scena i dowód A/B:** `projects/Prawko-Scenariusze/06h-r6b2f-cienie-latarni.{xodr,xosc,rmscene.json}` jest odrębną kopią 06g, dokładnie identyczna geometria XODR (**32 obiekty**, 5 latarni, do 4 aktywnych); 06g i starsze bez modyfikacji. Kontrola GPU korzysta z `PRAWKO_R6B2F_PROOF_DISABLE_SHADOWS`, która wyłącza wyłącznie occlusion, nie światło ani stożek. **Pierwszy przebieg A/B odrzucono**, ponieważ OFF miał szerszy stożek; naprawiono i ponowiono KOMPLETNY odbiór; stare zdjęcia i logi wyłącznie w `archive/a1-renderer-20261115/r6b2f-first-visual-pass/`, nie publikować ich jako finalnych. Finalne PNG na Windows Qt/OpenGL 3.3 **866×783**: `archive/a1-renderer-20261115/r6b2f-qt-{wide-night-off,wide-night-on,path-night-off,path-night-on,court-night-off,court-night-on,path-day-off,path-day-on,path-dusk-on,path-night-rain-on}.png`. Obrazy trzech A/B oraz 10 oryginalnych PNG są także w `docs/gallery/prawkoroadstudio-2026-11-16/` (README opisuje pochodzenie). Finalny audyt pikseli co 3 px, próbka 75 429: **dzień identyczny 0 próbek**, **noc ulica 2 522 próbek z ΔRGB >18, avg Δ=0,680**, **dojście 551, avg Δ=0,178**, **podwórze 2 186, avg Δ=0,667**, wszystkie PASS. W zbliżeniu na podwórze widać cień słupa i zacienione fragmenty trawnika. Obrazy nie są generacjami AI.

**Wydajność GTX 1050 Ti, 1600×900, MSAA4, 180 klatek + warmup/tryb, steady/orbit:** cienie OFF **59,70 / 59,95 FPS**, ON **59,96 / 59,94 FPS**, 0 klatek >33,33 ms w czterech pomiarach; `R6B2F_GPU_FPS_A_B_PASS`. Ze względu na synchronizację Qt ~60 Hz NIE oznacza to pomiaru maksymalnej wydajności GPU. CPU test `RenderLightingR6b2f.StreetlampShadowsRequireExplicitOptIn` PASS, dotychczasowy GUI test dodawania/przesuwania/obracania/zapisu 12 modeli PASS, scena 06h ładuje się **1 road / 0 diagnostics**. Po końcowej korekcie: `BUILD_PASS: all`, Qt **1362 PASS / 7 SKIPPED / 0 FAIL**, C++ **2074 PASS**, Python **7 PASS**, `AGENT_BINDING_PARITY_PASS`, `QUALITY_GATE_PASS`. Logi `archive/a1-renderer-20261115/r6b2f-{build,build-final,capture-final,fps-final,verify-final,qt-targeted,qt-full,quality}.log`, `r6b2f-fps-{off,on}.json`. Nie deklarować siedmiu SKIPPED jako PASS.

**Następny etap R6b2g (otwarty):** klosze/oprawy widocznie emitujące światło z rzeczywistych materiałów źródłowych, oparta na materiałach PBR wilgoć płyt/chodnika z poprawnym zachowaniem w deszczu i oświetleniu nocnym; dodatkowe testy deszcz/noc/odbicia/cienie i FPS (w tym esmini). Obecne światła nadal nie mają IES, fotometrycznej kalibracji, pełnego okludowania przez przezroczyste szkło, global illumination ani fizycznych SSR; nie deklarować fotorealizmu całego R6.

**A1/R6b2g — emisyjne materiały opraw oraz mokre GLB PBR (16.11.2026, PASS TECHNICZNY, SSR/kałuże OTWARTE):** Czysty HEAD przed zmianą `040683f8a679f3e1d4c56777eb8fd943675b1269`, agent `67a92fa24b4054b7f094c9b667f3634c35ad9210`. Utworzono **autorski nowy model** `assets/prawko/blender/oprawa_r6b2g/r6b2g-latarnia-emisyjna.{blend,glb}` oraz generator `create_oprawa.py`, metadata i odrębny Cycles. Model pochodzi wyłącznie z istniejącego własnego `r6b-latarnia.glb`, zachowuje fizyczne położenie oprawy (lokalna geometria klosza pod wysięgnikiem), ale jej materiał ma prawdziwy standardowy `glTF emissiveFactor` (w podglądzie barwa ciepła), a nie dekoracyjny ekranowy halo. Oryginalna latarnia niezmieniona. Nowy model wpisano do istniejącej biblioteki projektu (`projects/Prawko-Scenariusze/assets/library/manifest.json`), z plikiem `assets/props/r6b2g-latarnia-emisyjna.glb` i miniaturą — **łącznie 15 pozycji biblioteki**.

**Architektura bez wiązania efektu z jedną sceną:** importer core `PropPart` odczytuje i ogranicza `emissiveFactor` (nieprawidłowe wartości prowadzą do zera), `SceneBuilder` przekazuje emisję do `RenderMeshData` i `ViewportWidget::Material`, `GLRenderer` dodaje w shaderze rzeczywistych fragmentów geometrii emisyjność w liniowym RGB z kodowaniem do niesRGB RGBA8. Domyślny materiał ma emisję [0,0,0], więc pozostałe modele bez emisji zachowują się jak przed zmianą. Specyficznie **dla nowej latarni** emisja klosza jest włączana w trybie Textured: noc 1,0, zmierzch 0,54, dzień i Sober 0; inne materiały GLB z emisją mogą działać niezależnie. Stary model i nowy `r6b2g-latarnia-emisyjna` mogą generować światło/cienie z tych samych rzeczywistych transformacji `street_lamp_from_prop`; przesunięcie/obrót myszą wpływają na światło. Shader 17 KB GLSL rozdzielono na trzy osobne, składane przy starcie krótsze fragmenty `std::string`, ponieważ MSVC zgłaszał C2026 przy pojedynczym litera­le; pełny build po tej poprawce PASS. Bez oddzielnego silnika i bez zmian scen 01–06h.

**PBR w deszczu:** dotychczasowy `Environment::wetness` dla asfaltu zachowano; osobne `pbr_wetness` z presetu Rain w trybie Textured zmienia tylko importowane GLB PBR (uwzględnia poziomą normalną i delikatne, zmienne przestrzennie zróżnicowanie). Mokrą powierzchnię przyciemnia, zmniejsza szorstkość i oblicza kierunkowe refleksy GGX-like zależne od kamery, odległości, stożka i cieni faktycznych latarni. Funkcja jest wspólna dla modeli GLB PBR, a nie związana z jedną sceną ani jednym obrazem; domyślnie `pbr_wetness=0`, więc scena sucha nie zmienia obrazu. Jest to **przybliżenie lokalnego odblasku**, NIE pełne odbicia otoczenia SSR, ray tracing, kałuże, fotometryczne IES czy fizyczna dyspersja. Kontrolne przełączniki tylko na potrzeby A/B: `PRAWKO_R6B2G_PROOF_DISABLE_EMISSION`, `PRAWKO_R6B2G_PROOF_DISABLE_WET_PBR`.

**Scena kontrolna:** `projects/Prawko-Scenariusze/06i-r6b2g-mokry-plac-i-oprawy.{xodr,xosc,rmscene.json}` — kopia `06h`, **32 obiekty i 5 latarni** ze starych modeli GLB zastąpionych wyłącznie w `06i` nowym GLB; geometria drogi, budynków i XOSC pozostaje nienaruszona. Scena otwiera się w rzeczywistym Qt Windows jako **1 droga, 0 diagnostics**. GUI/drag-drop test rozbudowany z 12 do **13 modeli**, obejmuje nową lampę (dodawanie myszą, położenie, obrót, zapis i odczyt XODR): PASS. Nowy celowany `PropAssetImport.R6b2gGlbEmissiveLensIsImportedFromAuthorMaterial` PASS: emisyjny jest tylko klosz (part), nie słup ani stalowe elementy. `BUILD_PASS: all`, łączny **Qt 1363 PASS / 7 SKIPPED / 0 FAIL**, C++ **2074 PASS**, Python **7 PASS**, `QUALITY_GATE_PASS`.

**Rzeczywisty odbiór Windows/OpenGL 3.3:** `archive/a1-renderer-20261115/r6b2g-qt-*.png` 13 surowych screenshotów **866×783** w dzień/noc/zmierzch/deszcz (kadr ogólny, przejście, ujęcie z bliska). `docs/gallery/prawkoroadstudio-2026-11-16/` zawiera te oryginały, osobny render Blendera i zestawienie trzech prawdziwych A/B z tej samej kamery/pory/pogody. Kontrola co trzeci piksel (75 429 próbek): **suchy dzień emisja OFF/ON identyczny 0 różnic**, **noc zbliżenie oprawy emisja OFF/ON 38 istotnie zmienionych próbek**, **deszcz w dzień nowe mokre PBR OFF/ON 6 355**, **deszcz w nocy OFF/ON 1 882**; `R6B2G_ALL_VISUAL_A_B_PASS`. Z bliska widać świecącą dolną powierzchnię klosza, stal nadal ciemna; w deszczu płyty ciemnieją, a kierunkowe odbicia są subtelne, bez obrazu budynków w kałużach.

**Wydajność:** rzeczywista GTX 1050 Ti, 1600×900, MSAA4, 180 próbek + warmup na tryb, deszcz/noc: PBR wet OFF **59,78/59,95 FPS**, ON **59,81/59,95 FPS** (steady/orbit); wszystkie cztery pomiary **0 klatek >33,33 ms**, `R6B2G_GPU_FPS_A_B_PASS`. Kadencja Qt synchronizowana ~60 Hz NIE jest maksymalnym FPS GPU. Logi `archive/a1-renderer-20261115/r6b2g-{blender,build,build-fixed,build-final,capture,qt-targeted,qt-full,quality,verify-run,fps-run,visual-gate,gallery}.*`; błędy dwóch wcześniejszych prób kompilacji MSVC C2026 i rozdzielenia łańcucha usunięto przed końcowym build PASS. **Kolejny krok R6b2h:** bardziej złożony model mokrych nawierzchni (lokalne kałuże/normal maps/planar reflections z ograniczeniem GPU), kontrola świateł samochodów i nocnej symulacji esmini, dalsze realistyczne otoczenie. Pełny R6 nie jest jeszcze fotorealistycznie ukończony.

**A1/R6b2h — autorskie kałuże + odbicie planarne wody (16.11.2026; PASS TECHNICZNY, CZYTELNE ODBICIE ELEWACJI NADAL OTWARTE):** Start czysty edytor HEAD `bb26bd40e6748984dec4c0a1c0b098f5557ec7d2`, agent `67a92fa24b4054b7f094c9b667f3634c35ad9210`. Własne modele `assets/prawko/blender/kaluze_r6b2h/`: generator `create_kaluze.py`, dwa pliki `.blend`, `.glb`, metadata oraz rzeczywiste Cycles. `r6b2h-kaluza-nieregularna-240` (ok. 2,413×1,392 m) i `r6b2h-kaluza-placowa-140` (ok. 1,415×0,789 m), **4 części każda**, nieregularna tafla, osobne wilgotne obrzeże i grunt/roślinność; wodny materiał `PrawkoWater.Surface` ma autentyczne glTF **alphaMode BLEND, alpha 0,69**. Zarejestrowano GLB w `projects/Prawko-Scenariusze/assets/props/`, dodano miniatury i 2 wpisy do istniejącego `assets/library/manifest.json`; łącznie **17** pozycjach biblioteki. W teście GUI 15 kolejnych modeli — Qt MIME drag/drop, przesuwanie, obrót i XODR save/reload — PASS. Poprzednie modele i sceny bez zmian. Nowa osobna `06j-r6b2h-kaluze-odbicia-budynkow.{xodr,xosc,rmscene.json}` jako kopia `06i` z 4 rozmieszczonymi kałużami (**36 obiektów**, 1 road, 0 diagnostics). Zapisane dokładne źródła i historia w repozytorium.

**Implementacja wspólna dla assetów:** importer core rozpoznaje po **nazwie MATERIAŁU** `PrawkoWater.` w glTF (nie po model ID ani XODR), przenosi `water_surface` przez `PropPart`, `RenderMeshData` do `Material`, domyślnie false; `Environment::planar_puddles_enabled` włączane tylko w trybie Textured podczas deszczu, gdy `pbr_wetness>0.05`. GLRenderer OpenGL 3.3 ma pojedynczą dodatkową scenę renderowaną do **RGBA8+depth FBO ≤512×384**: prawdziwa istniejąca geometria renderowana ponownie ze zmirrorowanej kamery względem poziomu najbliższej tafli (dla natywnego 866×783 FBO **424×384**); przezroczystą wodę i tło-grid pomija w odbiciu, przycina obiekty pod wodą, zapobiega rekurencji i ponownemu liczeniu map cieni. GLB wodny pobiera rzeczywisty kolor FBO według wyprojektowanych współrzędnych, z przyciemnioną barwą tafli, lekkim lokalnym rippling i mieszaniem Fresnela. Odbicie **nie jest stockowym zdjęciem, screen-space „poświatą” ani samym kolorem nieba**, ale z powodu ograniczeń kamery i skali w bieżącej scenie efekt na kałużach jest subtelny; **nie twierdzić, że odbicie całego budynku zostało już wizualnie potwierdzone**. Jedna wysokość płaszczyzny na kamerę, brak prawdziwego falowania, ray tracing, SSR innych powierzchni, odbicia nie zawierają przezroczystego szkła w pomocniczym przebiegu. Dodatkowy widok `facade` błędnie trafił w puste niebo, kontrolę odrzucono; oba PNG wyłącznie w ignorowanym `archive/a1-renderer-20261115/r6b2h-unusable-facade-camera/`, nie publikować. Pierwsze 12 PNG odrzucono z powodu zmieniającego się timingu kropli i zbyt białej tafli; wyłącznie w `r6b2h-first-visual-pass/`. Kontrolny env `PRAWKO_R6B2H_PROOF_FREEZE_PRECIPITATION` zamraża układ deszczu **wyłącznie na potrzeby A/B**, bez zmian normalnego renderowania; `PRAWKO_R6B2H_PROOF_DISABLE_PLANAR` wyłącza tylko odbicia, przy tym samym świetle, deszczu i geometrii.

**Rzeczywisty odbiór Windows:** build ostatni `archive/a1-renderer-20261115/r6b2h-build-qa.log`: `AGENT_BINDING_SYNC_PASS`, `BUILD_PASS: all` (poprzednie próby miały poprawiony błąd testu enum i konflikt dwóch Ninja, nie błąd shaderu). Qt/OpenGL screenshoty 866×783, 12 oryginalnych finalnych PNG + 4 pary A/B w `docs/gallery/prawkoroadstudio-2026-11-16/` i 2 własne Cycles. R6b2h na deszczu emituje dzienny/nocny obraz 1 road, 0 diagnostics, FBO 424×384. **Wynik finalnej metryki A/B przy tej samej klatce kropel (75 429 próbek co 3 px)**: **suchy dzień 0 istotnych różnic**, **deszcz/dzień 203**, **deszcz/noc 23**, **noc/podwórze 521**, **niska kamera/deszcz 382**, `R6B2H_FAST_VISUAL_GATE_PASS`. Zmiany są na rzeczywistej tafli, nie w deszczu. **Nie oznaczać jeszcze estetycznego odbicia fasady jako PASS.**

**Jakość i wydajność:** celowane Qt `PropAssetImport.R6b2hAuthoredPuddleMaterialIsReusableAndTransparent`, `RenderLightingR6b2h.PlanarReflectionDefaultsOffWithoutRainAndPuddle`, `MainWindowLayoutTest.R6BStreetAssetsCanBeDraggedMovedRotatedAndSaved`: **3/3 PASS**. Pełny Qt **1365 PASS / 7 SKIPPED / 0 FAIL**, core C++ **2074 PASS**, agent Python **7 PASS**, `QUALITY_GATE_PASS`, `R6B2H_FULL_QUALITY_PASS`. GTX 1050 Ti, 1600×900 4×MSAA, night+rain/180 próbek + warmup na tryb: odbicie OFF **59,84 FPS steady / 60,00 orbit** i ON **59,85 / 60,00**, 0 klatek >33,33 ms, `R6B2H_REAL_GPU_FPS_GATE_PASS` — to jedynie kadencja Qt ~60 Hz, NIE maksymalna liczba GPU FPS. Logi QA i PNG w ignorowanym `archive/a1-renderer-20261115/r6b2h-*`.

**Następny R6b2i (otwarty):** wizualnie czytelne odbicie geometrii elewacji — poprawny kadr / właściwe rozmieszczenie i wysokości kałuż oraz kompozycja kamery, lokalny bilans refleksów i szorstkości, bez iluzorycznych przejaśnień; osobna kontrola widoku kierowcy/esmini, noc/deszcz i wydajność GPU przy trudniejszych scenach. Nie oznaczać całego R6 jako fotorealistycznie ukończonego.

**A1/R6b2i — potwierdzone odbicie elewacji i wejścia w wodzie (16.11.2026, VISUAL PASS dla zbliżenia):** Czysty start: edytor b224a2489e5191ebc0ee905c44b7c07d20636bca, agent 67a92fa24b4054b7f094c9b667f3634c35ad9210, prawko-resume PASS. Diagnostyka wykazała problem położenia poprzednich kałuż pomiędzy kamienicami i niewłaściwego poziomu zOffset przy chodniku. Tymczasowy odczyt realnej tekstury planarnego GPU FBO 424x384 w archive/r6b2i-probe dowiódł, że odwrócona elewacja i okna faktycznie trafiają do odbicia. Kod diagnostyczny usunięto przed finalnym buildem (gl_functions.hpp pozostał nietknięty).

**Zmiany kodu i model:** editor/src/render/gl_renderer.cpp — wodny materiał PrawkoWater przy włączonym odbiciu deszczu próbuje rzeczywisty obraz GPU z wyższym udziałem refleksu: clamp(0.88+0.10*edge_fresnel,0.88,0.98) oraz finalna alfa max(albedo.a,0.92) pozwalają zobaczyć elementy fasady, zamiast rozmytej plamy na asfalcie. Pozostałe materiały i suche sceny bez zmian. Nowy reużywalny GLB r6b2i-kaluza-przykraweznikowa-700: ok. 6,96 x 2,158 m, 4 części, autorski Blender .blend/Cycles i wspólny materiał PrawkoWater (glTF alpha BLEND 0,69). Generator assets/prawko/blender/kaluze_r6b2i/create_kaluze_700.py ponownie wykorzystuje assets/prawko/blender/kaluze_r6b2h/create_kaluze.py; oryginalna funkcja export zabezpieczona if __name__ == __main__ przed ponownym uruchomieniem i z poprawionym skalowaniem podglądu, wcześniejsze GLB NIE zmienione. Biblioteka assets/library/manifest.json w projekcie Prawko-Scenariusze ma teraz 18 modeli. Nowy edytowalny model jest w projektowym assets/props i posiada miniaturę. Nowa osobna scena 06k-r6b2i-kaluze-przed-elewacja.xodr + xosc + rmscene.json: 1 droga, 37 obiektów, 0 diagnostics, 5 kałuż (w tym nowa długa przy krawężniku), kamera na fasadę target [68,2.4,0.4], dystans 6.3, pitch .32, yaw -pi/2, deszcz/dzień. Stare 06j i wszystkie wcześniejsze sceny zachowane. project.json.last_scene pozostaje 04-zima-mgla.xodr.

**Dowody prawdziwej sceny Windows:** 11 niezmanipulowanych 866x783 screenshotów PrawkoRoadStudio oraz 4 zestawienia OFF/ON + oddzielny Cycles są w docs/gallery/prawkoroadstudio-2026-11-16/r6b2i-*.png, opis dokładny w tym samym README.md. A/B miało identyczną geometrię, czas, krople i światło, przełączano tylko prawdziwe odbicie planarne. 75 429 próbek co 3 piksele: DAY_RAIN 2681 istotnych zmian, NIGHT_RAIN 1646, SHALLOW_RAIN 940, DRY_CONTROL 0. Rzeczywiste pasy wody z elewacją: ON 82,42/67,57/78,95 (ściana/ciemne drzwi/ściana) wobec OFF 55,31/54,94/55,36 (jednolity odcień). R6B2I_PLANAR_FACADE_NATIVE_VISUAL_GATE_PASS. Build Windows AGENT_BINDING_SYNC_PASS i BUILD_PASS: all, Qt targeted 4/4 PASS (w tym import PBR GLB i real GUI drag/drop, move, rotate, XODR save/reload wszystkich 16 modeli), całe Qt 1366 PASS / 7 SKIPPED / 0 FAIL, core C++ 2074 PASS, Python agent 7 PASS, QUALITY_GATE_PASS. FPS rzeczywistej GTX1050Ti, MSAA4 1600x900 noc/deszcz, OFF 59,83 FPS steady / 60,00 orbit, ON 59,84/60,00, zero klatek ponad 33,33ms, R6B2I_REAL_GPU_FPS_GATE_PASS. Qt synchronizowane ok.60Hz, bez deklaracji maksymalnej wydajności GPU. Logi archive/r6b2i-final-build.log i archive/r6b2i-final/*.

**Odbiór i dalszy zakres:** W wybranej kompozycji kałuża WYRAŹNIE odbija parter kamienicy, wejście, witryny i pionowe elementy, a nie tylko ogólną zmianę koloru — kryterium wizualne R6b2i zaliczone. Nie potwierdzono fotorealizmu CAŁEJ elewacji na każdej odległości: jedna wysokość płaszczyzny odbicia na kamerę, ograniczony FBO, bez ray tracingu, SSR innych mokrych powierzchni i transparentnego szkła. Otwarta późniejsza kontrola ruchu, esmini i widoku kierowcy oraz kałuż na różnych wysokościach. Wszystkie wyniki zapisane przed commitem. R6b2h historyczny i jego ograniczenia pozostają w dokumencie, ten wpis zamyka jego otwarte kryterium fasady.

**Zasada pracy:** po każdej potwierdzonej implementacji dopisać do tej sekcji wynik, miejsca zmian i dowody; commitować tylko ukończony krok. Przy przerwaniu rozpocząć od pierwszego PENDING; nigdy nie oznaczać zdjęcia statycznego jako przejazdu, ani braku błędów kompilacji jako poprawnego cienia. Materiały wielokrotnego użycia, geometrię starego kokpitu i XODR/XOSC pozostawić nienaruszone.

## 15.11.2026 — Prawko Assets A1: model wiaty, integracja GLB i odbiór jakości

**Stan: PROTOTYP WYKONANY, pełna bramka realizmu MATERIAŁÓW OTWARTA.** Blender 5.3 utworzył własny model przystanku (156 obiektów; 4,96 × 2,35 × 2,769 m): cztery stalowe filary, mocowania, przeszklenie, ławka, gablota rozkładu jazdy, dach, rynna, betonowa podstawa, reliefy i detale. Pliki źródłowe (.blend, generator create_wiata.py, GLB, dwie tekstury albedo betonu i drewna, metadane oraz render Cycles) są w assets/prawko/blender/wiata_przystankowa/. Oryginalny plik GLB zawiera 156 nodes, 156 meshes, 12 materiałów, 2 tekstury; szkło w GLB ma alphaMode BLEND / alpha 0,23 / roughness 0,09 / transmission 0,64.

**Biblioteka i osobna scena:** projekt projects/Prawko-Scenariusze ma teraz kopię GLB w assets/props, własną miniaturę 96×96 i manifest w assets/library/manifest.json w kategorii „Prawko | Obiekty 3D”. Utworzono tylko NOWĄ scenę 05-wiata-test.xodr oraz dopasowane .xosc i .rmscene.json. Cztery poprzednie scenariusze pozostały bez zmian, projekt.json zachował last_scene=04-zima-mgla.xodr. Testy obciążenia i zrzuty korzystają z niezależnej kopii w ignorowanym archive/a1-wiata-performance-project/, więc nie nadpisują projektów użytkownika.

**Wąska poprawka importu i renderowania:** PropPart / RenderMeshData przenoszą BLEND alpha i roughnessFactor; GLRenderer rysuje po nieprzezroczystej geometrii osobny transparentny przebieg z włączonym blendingiem i testem głębi, bez zapisu głębi; domyślny stary model jest nieprzezroczysty. To NIE jest pełne PBR: GLB kolorowe obrazy nadal spłaszcza do średniego koloru (diagnostyka flattened, #507), nie ma importu oryginalnych UV/tekstur w materiale prop, refrakcji i fizycznej transmisji szkła ani cieni. Model jest zatem wizualnie wyraźnie prostszy w edytorze niż w renderze Blender Cycles.

**Potwierdzone testy:** pełny Windows build scripts/prawko-build-win.ps1 -Target all -Jobs 3 → BUILD_PASS: all + AGENT_BINDING_SYNC_PASS (po poprawieniu include w nowym teście). Test PrawkoA1BlenderShelterKeepsAlphaAndRoughnessWhileReportingTextureLimits: importer przyjmuje prawdziwe GLB i zachowuje przezroczystość, chropowatość oraz zgłasza spłaszczone tekstury — PASS. MainWindowLayoutTest.BlenderShelterLibraryDropMoveRotateSave: rzeczywisty interfejs Qt (QDragEnter / QDrop) importuje model, przesuwa/obraca w panelu, zapisuje i ponownie czyta XODR — PASS. Poprzedni test bariery również PASS. Celowane 3/3 PASS; pełny Qt 1354 PASS / 7 SKIPPED / 0 FAIL z 1361. Niezależny `scripts/prawko-verify-win.ps1` zakończył się po wszystkich poprawkach: 2072/2072 testów C++ PASS (10 disabled), 7/7 testów agenta Python PASS, AGENT_BINDING_PARITY_PASS, QUALITY_GATE_PASS.

**Oględziny prawdziwego Windows OpenGL:** archive/a1-wiata-visual/wiata-close.png pokazuje szkło odsłaniające wnętrze, ławkę i gablotę. Są też zrzuty wiata-day, wiata-rain, wiata-night i wiata-editor-library. Rzeczywisty renderer otwiera wszystkie bez diagnostyk; noc jest jednak zbyt ciemna bez świateł. W oddzielnym projekcie z 1 i 25 instancjami wiaty udało się stworzyć oba obrazy bez błędu: 3063 ms dla 1 oraz 3040 ms dla 25 egzemplarzy, to całkowity czas uruchomienia EXE + zrzutu PNG, nie pomiar FPS gry. Dowody: archive/a1-acceptance/wiata-{1,25}-instance.png. Pełne UV/PBR, wierne szkło, rzeczywiste cienie, FPS i sprawdzenie tego konkretnego modelu z lusterkami NIEZALICZONE / niepotwierdzone.

**Pierwszy następny podetap w aktywnym PRIORYTECIE A1:** przed seryjną produkcją naprawić transport UV + nieuśrednionego base-color texture z GLB do instancjonowanego renderera i minimalny PBR (z ochroną limitów, wielokrotne użycie GPU tekstur, brak regresji istniejących materiałów). Następnie porównać obraz tej samej wiaty w Blenderze i Qt, zweryfikować cienie, szkło i odbicia oraz przeprowadzić test kamery kierowcy i wszystkich trzech lusterek oraz pomiar FPS w ruchu. Nie zaczynać dodatkowych ogólnych funkcji edytora ani masowej produkcji assetów przed tą bramką.

## AKTUALNA DECYZJA / PRIORYTET: PRAWKO ASSETS

**Od 15.11.2026 nadrzędny kierunek prac to realistyczne assety 3D tworzone we własnym Blenderze i wykorzystywane w PrawkoRoadStudio.** Bazowe funkcje programu wystarczają do rozpoczęcia produkcji modeli. **Nie kontynuować teraz rozbudowy edytora dla samej rozbudowy.** Pierwszy krok: jedna zaawansowana wiata przystankowa, test materiałów i integracji, dopiero potem produkcja większej biblioteki. Szczegółowe kryteria i kolejność znajdują się w **Aktywnym planie → PRIORYTET A1**, który ma pierwszeństwo przed starszymi punktami 5b–5d. To decyzja o kolejności prac, a nie deklaracja ukończenia wszystkich wcześniej otwartych funkcji.

## 15.11.2026 — S3.3: wymiana aktywnych scen i pełny odbiór jazdy MP4

**Decyzja użytkownika:** „Usuń wszystkie sceny i utwórz swoje.” W folderze `projects/` pozostał wyłącznie nowy projekt `Prawko-Scenariusze/`. Poprzedni demonstrator `projects/Prawko-Bariera-Demo` został usunięty z aktywnych projektów, zachowany w `tests/fixtures/Prawko-Bariera-Demo/` dla istniejącego testu importu GLB oraz w zweryfikowanym archiwum `archive/scene-replacement-20261115/Prawko-Bariera-Demo-backup.zip` (9 elementów, SHA-256 `1981B356CD799B5087C8DCFF827CAEA194F66D5912E2CCBC54FD3AD81C48C81D`). Nie kasowano historycznych fixture silnika, historii Git ani niezapisanych scen (nie było uruchomionego edytora). Archiwum jest lokalne/ignorowane, nie umieszczać go w GitHub.

**Nowe autorskie sceny — pliki z repo:** `projects/Prawko-Scenariusze/01-miasto-lusterka.{xodr,xosc,rmscene.json}` (trzy auta; dzień; 13 s), `02-przejscie-pieszy` (hamowanie, przechodzący pieszy i reakcja auta za Ego; zmierzch, jesień; 11 s), `03-deszcz-luk` (łuk drogi, hamowanie auta z przodu i Ego; deszcz; 13 s), `04-zima-mgla` (ograniczona widoczność, zwolnienie; zima; 12 s). Projekt ma `project.json` i `SCENARIUSZE.md`. Kod źródłowy generatora `scripts/create-prawko-scenarios.cjs` odtwarza pakiet w pustym katalogu i **odmawia nadpisania istniejącego projektu**. W każdej scenie `driver_view_actor=Ego`; nie modyfikowano źródłowych modeli Blendera ani silnika geometrii. To funkcjonalne, na razie oszczędne wizualnie sceny, nie finalne fotorealistyczne miasta.

**Prawdziwa symulacja:** pobrano oficjalny esmini 3.11.0 `toolchain/esmini-v3.11.0` (ignorowane przez Git), źródło `https://github.com/esmini/esmini/releases/tag/v3.11.0`, ZIP 43930106 bajtów, SHA-256 `6ec1b75f62d7aadb8b6eece79ccca1e714a450d02cacc964370a57c9bdbd7a35`. Każda scena została rzeczywiście uruchomiona w `esmini.exe --headless --fixed_timestep 0.05 --seed 42 --record` i odczytana przez `dat2csv.exe`. Wygenerowano odpowiednio 786, 666, 786, 726 próbek; Ego pokonał ok. 130,5 m / 55,3 m / 102,2 m / 76,3 m. Oryginalna scena z pieszym miała niezamierzone najechanie tylnego auta: dodano osobne `Bezpieczne_hamowanie_za_ego` i po ponownym uruchomieniu we wszystkich czterech scenach **zero wykrytych kolizji**. W logach widać wyzwolenie zdarzeń i zakończenie. Metryki w `archive/scene-replacement-20261115/simulation-metrics.json`.

**Zmiany kompatybilności:** `editor/src/document/simulation_trace.cpp` obsługuje zarówno stary CSV esmini z `roadId/laneId`, jak i nowy uproszczony CSV 3.11.0 z `speed/wheel_angle/wheel_rot`; bieżący eksport `dat2csv --extended` nadal działa bez zmiany przyjętego kontraktu. Test nowej wersji w `editor/tests/test_esmini_preview.cpp`. Testy `PrawkoNewScenes` sprawdzają czytanie czterech `Document`, walidację XOSC z siecią, snapshoty, warunki sidecar, a na komputerze z esmini także rzeczywiste rejestrowanie ruchu. Legacy test bariery odwołuje się teraz do `tests/fixtures/`. `qt_gtest_main.cpp` pozostaje **offscreen dla wszystkich zwykłych testów**; jedynie przy jawnie ustawionym `PRAWKO_VIDEO_PROOF_OUT` korzysta z natywnego Windows/OpenGL.

**Odbiór prawdziwego MP4:** test `editor/tests/test_prawko_real_video.cpp` uruchamia esmini, przekazuje rzeczywiste pozycje do `ViewportWidget::capture_simulation_frame` (kamera kierowcy, Blenderowy kokpit, trzy osobno renderowane lusterka, pogoda, wycieraczki, wskaźniki) i koduje przez `ScenarioVideoExport` + FFmpeg libx264 do osobnego katalogu `archive/scene-replacement-20261115`. Skrypt `render-hd-video.ps1` potwierdził `ffprobe` dla **czterech filmów 1280×720 / 24 kl./s**: `01-miasto-lusterka-HD.mp4` 13 s/312 klatek; `02-przejscie-pieszy-HD.mp4` 11 s/264; `03-deszcz-luk-HD.mp4` 13 s/312; `04-zima-mgla-HD.mp4` 12 s/288. Zrzuty `*.mp4.first.png` i `*.mp4.moving.png` obejrzano dla miasta, przejścia i deszczu: drogę widać, trzy lusterka renderują oddzielnie, wycieraczki zmieniają fazę. Wynik `PRAWKO_FOUR_COMPLETE_HD_SIMULATION_VIDEOS_PASS`. Przyspieszenie renderu offline nie jest pomiarem czasu rzeczywistego użytkowego GUI.

**Kontrole (po zmianach):** `scripts/prawko-build-win.ps1 -Target all -Jobs 3` → `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`. Qt targeted 5/5 PASS; cały Qt **1352 PASS, 7 SKIPPED, 0 FAIL** / 1359. `scripts/prawko-verify-win.ps1`: **2072/2072 C++ PASS** (10 disabled) + **7/7 Python PASS**; `QUALITY_GATE_PASS`. Pliki logów: `archive/scene-replacement-20261115/{qt-full.log,qt-targeted.log,*-video-proof.log}`. Test dedykowany MP4 jest opt-in i nie jest jednym z siedmiu obowiązkowo wykonywanych GUI testów (przeszedł osobno 4× na natywnym OpenGL).

**Ograniczenia / następne zadanie:** cztery filmy są działającymi demonstratorami, nie gotowymi materiałami egzaminacyjnymi. Otoczenie jest oszczędne, zmierzch ciemny bez świateł mijania, pieszy i auta używają uproszczonych modeli; brak walidacji prawnego oznakowania całego scenariusza i realnej fizyki przyczepności. Kolejny etap to **podniesienie wizualnej jakości tych czterech nowych scen** przez własne modele Blender, z zachowaniem parametrów bezpieczeństwa, poprawienie czytelności sceny o zmierzchu przez prawdziwe reflektory i pomiar FPS podczas ciągłego podglądu w GUI. Nie tworzyć ponownie starych przykładowych scen i nie uznawać całego S3.3 za zamknięty.

## 15.11.2026 — S3.3/5d: własna kabina 3D z Blendera i sterowane komponenty

**Stan kodu:** Własny, edytowalny model źródłowy `assets/prawko/blender/prawko_kokpit.blend` z generatora `create_kokpit.py` (Blender 5.3). Osobne rzeczywiste GLB: `prawko_kokpit_shell.glb` (przednia kabina, osłony deski, instrumenty, nawiewy, drzwi, słupki, rama szyby, konsola, wybierak), `prawko_kokpit_wheel.glb` (ruchoma kierownica) i `prawko_kokpit_wiper.glb` (wycieraczka wielokrotnego użycia). Geometria ma własne materiały, bryły i trójkąty, nie jest ekranowym obrazkiem. Oryginalny widok z prostą rysowaną kierownicą zastąpiono rzeczywistymi siatkami OpenGL w `ViewportWidget::render_frame`. Cyfry aktualnej prędkości pozostały ostre, rysowane tekstem nad rzeczywistymi zegarami; nie jest to jeszcze kompletny zestaw sygnalizatorów pojazdu.

**Punkt zaczepienia kamery:** Moduł `editor/src/render/cockpit.{hpp,cpp}` importuje z zasobów Qt trzy GLB używając istniejącego `props::import_prop_model`, a `kokpit_layout.json` przywraca pozycję oryginalną ze znormalizowanych współrzędnych (prawa/lewa, przód, góra) względem oka kierowcy. Pochylenie i przechył auta przenoszą fizyczny kokpit razem z kamerą. Model renderuje się tylko z aktywnej głównej kamery kierowcy; nie pojawia się w kamerze edytora ani trzech osobnych odbiciach. Przy pełnym przebudowaniu sceny uchwyty GL są czyszczone razem z resztą buforów. XODR/XOSC, scena i osobne repo agenta nie zmieniają formatu, nie muszą zawierać modelu kabiny.

**Zmienność i eksport:** `SimulationPlayback` wylicza prędkość km/h z różnic pozycji i przybliżenie kąta kierownicy z prędkości + zmiany kursu (model rowerowy 2,6 m, przełożenie kierownicy 14:1, clamp). To nie jest odczyt kąta z czujnika pojazdu. Wycieraczki mają osobną siatkę i fazę powtarzalną jako funkcja czasu symulacji, uruchamianą automatycznie dla deszczu/śniegu. Ten sam czas jest przekazywany do podglądu oraz `capture_simulation_frame` używanego przez eksport klatek MP4; lusterka i opad otrzymują czas symulacji zamiast zegara procesu. Podczas braku deszczu wycieraczka pozostaje w położeniu spoczynkowym.

**Realna kontrola geometrii:** Testowy `--camera driver-cockpit-forward-proof` kieruje kamerę wzdłuż nawierzchni, używa przykładowej wartości prędkości 45 km/h i obrotu kierownicy; ustawienie jest tylko dla zrzutów, niczego nie zapisuje. Na kopii `projects/Prawko-Bariera-Demo` dokonano korekty rzeczywistego modelu po oględzinach nieudanego pierwszego kadru (przestrzeń dachu zasłaniała jezdnię). W finalnym kadrze `archive/s33-cockpit-20261115/forward.png` jezdnia i oznakowanie są widoczne nad deską, obrót kierownicy jest widoczny i trzy lusterka nadal pokazują niezależne obrazy. Testowe `{day,forward,forward-rain,rain,snow,night}.png` wykonywane są na kopii sceny; użytkowe dokumenty bez zmian. Skrypty testowe w `archive/s33-cockpit-*.ps1`.

**Weryfikacja 15.11:** Build standardowego EXE **BUILD_PASS: all** i **AGENT_BINDING_SYNC_PASS**; celowane testy Qt **8/8 PASS**; pełny Qt **1348 PASS / 6 SKIPPED / 0 FAIL** z 1354. Niezależny `scripts/prawko-verify-win.ps1`: **2072/2072 C++ PASS** (10 dodatkowo wyłączonych), **7/7 Python PASS**, `QUALITY_GATE_PASS`. Realny EXE wygenerował 6 zrzutów 1026×883 px (dzień, kierunek jazdy, kierunek jazdy w deszczu, deszcz, śnieg, noc). Obrazy `forward.png` i `forward-rain.png` obejrzano: jezdnia pozostaje widoczna, łopatki wycieraczek zmieniają położenie, kierownica reaguje na kąt, lusterka renderują realne drogi. Testy: `archive/s33-cockpit-20261115/qt-targeted.log`, `qt-full.log`. Pliki użytkownika i istniejące sceny pozostawiono nienaruszone.

**Ograniczenia i kolejny krok:** Własny kokpit 3D i wycieraczki są wdrożoną bazą, ale **pełny interaktywny kokpit** z czujnikami pojazdu, manipulatorami, kierunkowskazami, światłami, pedałami, obrotomierzem, paliwem i aktywnymi kontrolkami nie jest zweryfikowany. Brak pomiaru FPS przez pełny przejazd, brak odegranego MP4 z oceną klatka po klatce oraz ręcznej kontroli rzeczywistym kursorem; nie oznaczać całego S3.3/5d jako zakończonego. Potwierdzone wyniki kompilacji i testów uzupełniają poniższe logi; po przerwaniu rozmowy wznowić od tej sekcji i aktywnego planu.

## 15.11.2026 — S3.3: pogoda, sezon, kokpit pierwszej wersji i nagłe hamowanie

**Potwierdzony kod:** PrawkoRoadStudio ma w panelu Symulacja dwa wybory: Pogoda = bez opadów/deszcz/mgła/śnieg i Pora roku = lato/jesień/zima. Wspólny shader daje mgłę z odległością (także w trzech lustrach), mokry i ciemniejszy asfalt, pokrywę śnieżną oraz sezonowe kolory trawy; opady to animowane nakładki Qt w widoku i każdym FBO lusterka. Ustawienia są przechowywane w atomowym .rmscene.json (weather, season); poprzednie sceny otwierają się przy bezopadowym lecie. Domyślny dzień bez opadów zachowuje wcześniejszą bazę. Dodano wektorowy panel dolnej części kokpitu z graficzną kierownicą i prędkościomierzem km/h — wartość pochodzi z sąsiednich próbek pozycji aktora w SimulationPlayback. Dodano przycisk Nagłe hamowanie w panelu Storyboard; wybiera nazwę pojazdu i sekundę, tworzy warunek czasu + akcję ustawiającą docelową prędkość 0 m/s przez istniejące XOSC i jedno Undo/Redo. Reszta zdarzeń nie jest przepisana. Nie dodawano kodu do osobnego repo agenta.

**Weryfikacja:** Początkowo editor HEAD ae93fc399631c21054d640fec1f3789e23dc434b i agent HEAD 67a92fa24b4054b7f094c9b667f3634c35ad9210 — oba czyste i zgodne z origin; scripts/prawko-resume.ps1 -Verify PASS. scripts/prawko-build-win.ps1 -Target all -Jobs 3 — **BUILD_PASS: all**, ponownie **BUILD_PASS: all** po dodaniu zdarzenia i po korekcie układu. Celowane Qt **7/7 PASS**: WeatherSeason, sidecar, przywracanie UI, interpolacja prędkości i StoryboardPanel.EmergencyBrakeIsARealTimedXoscEventWithOneUndo. Pełny Qt: **1344 PASS, 6 SKIPPED, 0 FAIL** z 1350 testów, log archive/s33-weather-20261115/qt-full.log. scripts/prawko-verify-win.ps1: **2072 C++ PASS (10 disabled)**, **7 Python PASS**, **QUALITY_GATE_PASS**, AGENT_BINDING_PARITY_PASS.

**Rzeczywiste zrzuty Windows:** archive/s33-weather-20261115/{clear,rain,fog,snow,autumn,winter}.png, 1026×883 px, sześć odrębnych obrazów ze standardowego EXE, identyczna kamera driver-mirrors-proof z trzema lusterkami. Zrzuty deszczu, mgły i śniegu obejrzane: efekt w głównej scenie i trzech lustrach, z dolnym kokpitem. Skrypt archive/s33-weather-capture.ps1 kopiował scenę bariery, więc projekty produkcyjne i niezapisane sceny nie były dotykane. Pełny log testów Qt: archive/s33-weather-20261115/qt-full.log.

**Otwarte — kolejny krok:** pełny kokpit 3D (zamiast prostej grafiki, ruchoma kierownica, wycieraczki, zegary, działające kontrolki), reflektory i jazda nocna, bardziej realistyczne materiały deszczu/śniegu oraz sezonowe modele drzew, dodatkowe zdarzenia (hamowanie przed przeszkodą, wtargnięcie pieszego, awarie, kolizje, przyczepność), wizualna edycja ich czasu i wyzwalaczy, test płynnego przejazdu i MP4 pod różnymi warunkami. **Nie oznaczać całego S3.3 jako zakończonego**. Kontrola rzeczywistego zachowania tego jednego zdarzenia przez esmini nadal otwarta. Zapisz osobny następny commit i Quality Gate przy kolejnym kroku. Obecny etap nie zmieniał oryginalnego RoadMaker, XODR ani XOSC żadnej istniejącej sceny.

## 15.11.2026 — S3.3/5d: dzień, świt, zmierzch i noc — potwierdzony podetap

**Wdrożono:** Panel Symulacja → Pora dnia: Dzień, Świt, Zmierzch i Noc. Zmiana ustawia kolor/kierunek/natężenie słońca, światło otoczenia i kolory nieba w istniejącym rendererze. Dzień nie zmienia poprzedniego światła Textured. UI przełącza tryb Textured automatycznie. Jedno środowisko dla kamery edytora, kamery kierowcy, 3 lusterek i klatek eksportu PNG/MP4. Pole SceneState.time_of_day jest zachowywane per scena w atomowym pliku .rmscene.json; starsze sceny bez pola otwierają domyślny dzień. Bez zmian XODR/XOSC, geometrii, tras lub repo agenta (agent_head=67a92fa24b4054b7f094c9b667f3634c35ad9210).

**Weryfikacja:** początkowy HEAD edytora 77b34f01c453edac8b15c67d011798470eb99010, czyste i zgodne z origin oba repo; prawko-resume.ps1 -Verify PASS. Po implementacji scripts/prawko-build-win.ps1 -Target all -Jobs 3: **BUILD_PASS: all / AGENT_BINDING_SYNC_PASS**. Trzy nowe testy TimeOfDay, SceneSidecar i SceneStateWindowTest + dwa DriverCamera: **5/5 PASS**. Pełny Qt: **1340 PASS / 6 SKIPPED / 0 FAIL** z 1346. scripts/prawko-verify-win.ps1: **2072/2072 C++ PASS** (10 disabled), **7/7 Python PASS**, **QUALITY_GATE_PASS**.

**Odbiór wizualny:** osobna kopia projects/Prawko-Bariera-Demo, 4 uruchomienia rzeczywistego EXE z kamerą driver-mirrors-proof, wynik **ATMOSPHERE_RENDER_PASS** każdej pory. Zrzuty 866×783 px: archive/s33-day-night-20261115/{day,dawn,dusk,night}.png. Zrzuty świtu/nocy obejrzano: otoczenie i trzy lustra zmieniają jasność/barwy; nie naruszono oryginalnych scen. Skrypt archive/s33-day-night-capture.ps1, logi testów archive/s33-day-night-20261115/qt-*.log.

**Ograniczenia i pierwszy kolejny krok:** Noc bez świateł pojazdu jest bardzo ciemna; pełny system reflektorów nie istnieje. Nie przeprowadzono ręcznego testu myszą, odtwarzania ruchu z oceną FPS ani pełnego MP4 dla presetów. Następny podetap S3.3: **pogoda** — deszcz, mgła, śnieg, mokra nawierzchnia i ograniczona widoczność w tym samym rendererze, w lusterkach i eksporcie. Następnie sezon, zdarzenia drogowe i pełny kokpit. S3.3/5d pozostaje otwarty.

## 14.11.2026 — S3.3 / 5d: rzeczywiste lusterka w widoku kierowcy

**Wdrożony fragment S3.3, nie całe 5d.** Po wybraniu pojazdu w symulacji jego istniejąca kamera DriverCamera zasila trzy dodatkowe kamery: środkową wsteczną, lewą boczną i prawą boczną (driver_mirror_camera). Śledzą położenie, yaw/pitch/roll aktora, patrzą w trzy kierunki do tyłu, a obraz jest odbijany poziomo podczas kompozycji. Wszystkie są osobnymi renderami OpenGL do małych FBO z tej samej sceny (nie są statycznymi teksturami). Nakładki w ViewportWidget::paintGL() pojawiają się tylko przy aktywnym widoku kierowcy; capture_simulation_frame() dodaje je również do klatek eksportowanych PNG/MP4. Nie dodawano deski rozdzielczej.

**Odbiór:** scripts/prawko-build-win.ps1 -Target all -Jobs 3 dał BUILD_PASS: all, a końcowy inkrementalny -Target editor również PASS. DriverCamera.* **2/2 PASS**, pełny Qt **1337 PASS / 6 SKIPPED / 0 FAIL** na 1343, C++ core **2072/2072 PASS** (10 disabled) i Python agent **7/7 PASS**, QUALITY_GATE_PASS. Rzeczywisty screenshot renderera Qt wykonano z --camera driver-mirrors-proof na projects/Prawko-Bariera-Demo/bariera-ustawiona.xodr; archive/driver-mirrors-s33/driver-mirrors-proof.png (866×783 px) został obejrzany: bariera jest w lusterku wstecznym i bocznym, nie w obrazie przedniej kamery. Ten specjalny preset istnieje **tylko do automatycznego odbioru** i nie zapisuje scen. **Nie wykonano ręcznego odbioru ciągłego ruchu systemową myszą ani pomiaru FPS eksportu** — nie traktować jako potwierdzone.

**Pozostaje do zrobienia w S3.3/5d:** UI wyboru pory dnia/nocy, pogody i sezonu, scenariusze zdarzeń (5b), warianty szablonów (5c), pełny kokpit i presety kamer. Odbiór kolejny: rzeczywista dynamiczna symulacja z wybranym autem i ruchem za nim, oba boczne kąty, zachowanie przy pitch/roll, tempo renderowania i końcowe MP4. Nie zmieniono decyzji projektowych ani scen użytkownika.

## 14.11.2026 — pierwszy rzeczywisty model Blender → PrawkoRoadStudio

**Zakres ściśle wg życzenia użytkownika:** własny model z Blendera, import do biblioteki, upuszczenie na drogę, przesuwanie, obracanie, Undo/Redo, zapis i ponowne otwarcie. Bez nowego edytora siatek i bez szerokich zmian funkcjonalnych.

- W Blenderze 5.3 powstała oryginalna bariera drogowa z dwustronnym czerwono-białym oznakowaniem: `assets/prawko/blender/{create_bariera.py,prawko_bariera_drogowa.blend,prawko_bariera_drogowa.glb}`. Oryginalny plik GLB jest modelowany z 38 obiektów, nie jest teksturą lub obrazkiem. Zawiera własne materiały i asymetryczną lampę ułatwiającą sprawdzenie kierunku. Projekt demonstracyjny `projects/Prawko-Bariera-Demo/` jest samodzielny: kopia GLB w `assets/props`, wyrenderowana w Blenderze miniatura `assets/library/thumbnails/bariera_prawko.png`, manifest z kategorią `Prawko | Obiekty 3D`, pusty `bariera-demo.xodr` do użytkowego drag/drop i `bariera-ustawiona.xodr` z obróconym modelem.
- Istniejący importer `import_prop_asset` i edytor pozycji s/t/Heading zachowano. Poprawiono tylko ich połączenie z biblioteką: GLB trafia do `Prawko | Obiekty 3D` (nie starego `Props`), wyświetlany jest jego kafelek z miniaturą. `Dodaj → Obiekty 3D` pojawia się **tylko przy otwartym projekcie z zaimportowanym modelem**. Stare dane testowe nadal nie wracają do biblioteki.
- **Naprawa istotnego błędu ponownego otwierania:** `Document::load()` buduje siatkę zanim `MainWindow` skojarzy projekt i zarejestruje jego modele GLB. `MainWindow::load_file()` po asocjacji wywołuje teraz `Document::refresh_registered_project_models()`, aby istniejący obiekt wyrenderował się bez wymuszania edycji. Operacja nie tworzy wpisu Undo. Regresja `PropAssetImport.PrawkoDemoSceneReloadsItsOwnBlenderModel` bada model przed i po rejestracji, a `MainWindowLayoutTest.BlenderBarrierLibraryDropAndPropertiesMoveRotateSave` przechodzi przez prawdziwe Qt QDragEnter/QDrop i spinboxy s/Heading.
- **Dowody:** `archive/blender-bariera-20261114/` (kod testowy, screenshoty UI 1440×900 i osobne zrzuty z bliska). Baza testów: `PropAssetImport.PrawkoBlenderBarrierDropsMovesRotatesAndSurvivesReload`, `LibraryPanel.RealImportedBlenderModelAppearsAndIsDraggable`, `MainWindowLayoutTest.BlenderBarrierLibraryDropAndPropertiesMoveRotateSave`, `PropAssetImport.PrawkoDemoSceneReloadsItsOwnBlenderModel`. Import, drop Qt, pozycja, obrót 90°, zapis XODR potwierdzone; wyniki kompletnej weryfikacji i końcowego standardowego builda opisano poniżej.
- **Ponowna weryfikacja 14.11.2026:** 4/4 testy Qt dedykowane modelowi PASS (w tym zdarzenia Qt drag/drop, s/Heading, Undo/Redo, reload i remesh); pełny Qt **1336 PASS, 6 SKIPPED, 0 FAIL** na 1342 (5 wymagających środowiska esmini i 1 symlink). `scripts/prawko-verify-win.ps1`: **2072 C++ PASS / 10 DISABLED**, **7 Python PASS**, `AGENT_BINDING_PARITY_PASS`, `QUALITY_GATE_PASS`; `check_curated_library.py` i `check_asset_licenses.py` PASS. Odrębne nowo zlinkowane EXE `build/dev-windows/editor/roadmaker-editor-proof.exe` pomyślnie zrealizowało realne zrzuty okna Qt 1440×900: `archive/blender-bariera-20261114/proof-drop.png` (dodanie przez Library) i `proof-reload.png` (GLB wyrenderowany po otwarciu zapisanego XODR). Zrzuty zostały obejrzane; **to nie jest ręczny test systemową myszą**.
- **Standardowy build i odbiór — PASS po zwolnieniu EXE:** podczas pierwszego builda edytor PID 15344 miał otwartą niezapisaną scenę `accept-actor.xodr*`, więc jego EXE był zablokowany (`LNK1168`). Nie zamykano procesu ani nie zapisywano sceny. Gdy proces zniknął bez ingerencji agenta, ponowiono `scripts/prawko-build-win.ps1 -Target all -Jobs 3`: **BUILD_PASS: all**, **AGENT_BINDING_SYNC_PASS**. Potwierdzono aktualizację standardowego `build/dev-windows/editor/roadmaker-editor.exe` oraz uruchomiono **na tym EXE** dwa testy rzeczywistego renderera Qt 1440×900 z wynikami `STANDARD_GUI_PASS drop` i `STANDARD_GUI_PASS reload`; zrzuty: `archive/blender-bariera-20261114/standard-drop.png` i `standard-reload.png`. Po ostatniej kompilacji ponownie 4/4 celowane testy Qt PASS. Pełny Qt, C++ i Python oraz weryfikacje repo PASS jak wyżej; standardowe EXE nie jest już zablokowane ani przestarzałe. **Nie wykonano jeszcze ręcznego systemowego przeciągnięcia kursorem w oknie użytkownika**; test granicy interfejsu Qt (QDragEnter/QDrop) oraz rzeczywisty renderer potwierdzono osobno. **Pierwsze kolejne zadanie:** gdy mysz użytkownika jest wolna, w oddzielnej kopii demo sprawdzić prawdziwe przeciągnięcie kafelka `Prawko | Obiekty 3D` kursorem, przesunięcie i obrót gizmem, zapis/ponowne otwarcie; bez dotykania aktywnych niezapisanych scen. Repo agenta bez zmian.
- **Wskazówka dla użytkownika:** otworzyć w edytorze `projects/Prawko-Bariera-Demo/bariera-demo.xodr`, następnie `Dodaj → Obiekty 3D → Bariera ostrzegawcza Prawko`, upuścić na drogę. Narzędzie `Przesuń` pozwala na przesunięcie i obrót pierścieniem; w Properties po zaznaczeniu dostępne są s/t/Heading/Z/Height. Osobny `bariera-ustawiona.xodr` zawiera zapisany przykład. Nie nadpisywać innych scen użytkownika.
- Repo agenta bez zmian (`agent_head=67a92fa24b4054b7f094c9b667f3634c35ad9210`). Zachować HEAD obu repo i procedury testów/przekazania przed dalszą pracą.

## 14.11.2026 — uproszczenie górnego paska PrawkoRoadStudio (po czyszczeniu biblioteki)

**Kod i odbiór Qt potwierdzone.** Po zgłoszeniu przeładowanego drugiego rzędu narzędzi zastąpiono dawną listę wszystkich narzędzi RoadMaker **jednym paskiem codziennych czynności** z ikonami 24 px i czytelnymi krótkimi etykietami. W widoku głównym są: `Nowa`, `Otwórz`, `Zapisz` | `Zaznacz`, `Przesuń`, `Usuń`, `Cofnij` | `Dodaj` z menu **Gotowe drogi / Krawężniki i chodniki / Uczestnicy ruchu** | `Droga` (Map) lub `Trasa` (Scenario) | `Scenariusz`, `Symulacja` | `Więcej`. Menu `Więcej` grupuje zaawansowane narzędzia według istniejącej klasyfikacji w `shortcut_registry`, używa **tych samych QAction i skrótów**, bez drugiego silnika edycji lub nowych prefabrykatów. `Dodaj` przenosi do rzeczywistego panelu Library i wybiera jedną z trzech naszych kategorii. Przycisk symulacji zachowuje dotychczasowy warunek aktywności (uczestnicy / gotowość przebiegu).

Dodatkowy pasek `toolbar.options` chowa się w stanie bez kontekstowych opcji; pojawia się dopiero przy `Create Road` (szablon), `Actor Place` (typ), `Route` (Zakończ trasę) lub `Terrain Brush` (parametry). Po odczycie wcześniejszego zapisanego układu ponownie synchronizuje widoczność z wybranym narzędziem, aby nie odtwarzać pustego paska. Wbudowany samouczek opisuje teraz nasze drogi, trzy kategorie Library, przesuwanie aktorów, trasę i symulację; usunięto instrukcje dotyczące dawnych drzew i skrzyżowań.

**Kontrole:** układ i przełączenia trybu, menu 3 kategorii, brak starego `toolbar.tools`, zachowanie wszystkich zaawansowanych akcji oraz samouczek: **37/37 celowanych Qt PASS**; pełny Qt po zmianie **1337 PASS, 1 SKIPPED (symlink), 0 FAIL** na 1338 testów; standardowy Windows `scripts/prawko-build-win.ps1 -Target all -Jobs 3` dał **BUILD_PASS: all / AGENT_BINDING_SYNC_PASS**. Screenshoty z realnego `roadmaker-editor.exe --screenshot-ui` (tryb bez zapisu historii ustawień) pokazują nowy pasek **1440×900, 1000×700 i 920×680**, bez znikających głównych przycisków; pliki `archive/toolbar-simplify-20261114/toolbar-*.png`. Zrzuty pochodzą z **wewnętrznej kopii testowej**, nie z otwartego dokumentu użytkownika. To jest test wizualny rzeczywistego okna Qt; **nie deklarować przeprowadzenia ręcznego testu kursorem w aktywnym oknie użytkownika**.

**Źródła:** `editor/src/app/main_window.cpp`, `editor/src/app/tour_controller.cpp`, `editor/tests/test_main_window_layout.cpp`, `editor/tests/test_tour_controller.cpp`; dokument nadrzędny `docs/PRAWKO-ROAD-STUDIO.md`. Repo agenta nie wymaga zmian: `agent_head=67a92fa24b4054b7f094c9b667f3634c35ad9210`. Końcowy ponowny build po formatowaniu: `FINAL_EDITOR_TEST_BUILD_PASS`; ponownie pełne Qt **1337 PASS / 1 SKIPPED**. Weryfikacja `scripts/prawko-verify-win.ps1` z osobnym TEMP: **2072/2072 C++ PASS**, **7/7 testów Python agenta PASS**, `QUALITY_GATE_PASS`. Ostateczne SHA edytora i status `SESSION_FINISH_PASS` potwierdza osobno skrypt `scripts/prawko-git.ps1 -Action finish`, wykonywany po commit/push; nie należy zgadywać HEAD na podstawie daty wpisu.

**Następne zadanie:** przy następnym etapie najpierw odczytać aktualny HEAD, kontrolę jakości i ten HANDOFF. Po zwolnieniu myszy można sprawdzić klik `Dodaj` → każda z trzech kategorii, przejście Map/Scenario, `Droga` / `Trasa` oraz menu `Więcej` kursorem w oddzielnej, nowej scenie; nie dotykać niezapisanych scen. Kolejne nowe obiekty dodawać do własnej biblioteki Prawko dopiero po stworzeniu geometrii, miniatur i testów.

## 14.11.2026 — PORZĄDKI: wyłącznie własna biblioteka Prawko (po poprawce dróg)

**Czysty katalog produktowy — implementacja i testy PASS.** Użytkownik świadomie zdecydował, że stare przykłady i prefabrykaty z RoadMaker były testowe; od nowa tworzymy własne komponenty. Aktywny manifest `assets/library/manifest.json` ma **12 produktów: 6 Prawko | Gotowe drogi, 6 Prawko | Krawężniki i chodniki**, a kod dodaje **6 Uczestników ruchu** (samochód, ciężarówka, autobus, motocykl, rower, pieszy). Łącznie **18 widocznych kafelków i tylko 3 kategorie**, w Map i Scenario. Wyszukiwanie, filtry, zewnętrzne overlaye i menu kontekstowe nie ujawniają starych kategorii. Upuszczenie aktora przełącza do Scenario. Ekran Welcome nie pokazuje dawnych scen przykładowych.

- Fizyczne czyszczenie zasobów: usunięte stare miniatury palety (w `editor/resources/resources.qrc` pozostało dokładnie 12 naszych), stare mesh-e `assets/library/props`, grafiki znaków `assets/signs/us` i `assets/samples`. Produkcyjne katalogi przykładów oraz `agent/projects` są **puste**. Przed usunięciem ośmiu lokalnych projektów testowych agenta wykonano sprawdzony ZIP: `archive/library-cleanup-20261114/agent-test-projects-backup.zip`.
- **Wewnętrzne wyjątki techniczne, NIE biblioteka produktu:** istniejące przykładowe pliki XODR/XOSC, modele OBJ/MTL i stare SVG przekierowano pod `core/tests/fixtures/legacy_scenes`, `legacy_prop_sources`, `legacy_sign_sources`. To dane do testów wstecznej zgodności formatów, nie zasoby oferowane użytkownikowi i nie gotowe sceny. Nie usuwać ich bez przeniesienia testów generowania/parsowania na syntetyczne fixtury. Powiązane ścieżki w CI, CMake, skryptach, dokumentacji i testach dostosowano. Repo agenta: `test_roadctl.py` wskazuje nową ścieżkę testową.
- Automatyczna kontrola `scripts/check_curated_library.py` wykrywa powrót starych plików albo kategorii, jest uruchamiana w CI. `scripts/check_asset_licenses.py` **PASS**. Testy Qt: **1337 PASS, 1 SKIPPED (symlink), 0 FAIL** z 1338. Testy C++ **2072 PASS**, Python agenta **7 PASS**, `QUALITY_GATE_PASS` przy `scripts/prawko-verify-win.ps1`. Pełny build Windows **BUILD_PASS: all**, synchronizacja Python/bindingów **AGENT_BINDING_SYNC_PASS**.
- GUI: zbudowano nowe standardowe `build/dev-windows/editor/roadmaker-editor.exe`. Próba zrzutu w oddzielnym oknie PID 21432 została przerwana komunikatem `TEST_EDITOR_NOT_FOCUSED` (inne okno ma fokus), więc **nie uznawać ręcznego odbioru myszy za ukończony**; własny proces testowy zamknięto bez zapisywania scen. Testy Qt sprawdzają kategorie, 18 ikon, wyszukiwanie, overlaye, dostępność aktorów w Map/Scenario i działanie upuszczania kolejnych dróg.
- **Następne zadanie:** dodać do biblioteki kolejną kategorię dopiero po zaprojektowaniu i wykonaniu własnych realistycznych elementów 3D PrawkoNaRaz, odpowiednich testach i świadomym rozszerzeniu kontroli manifestu. Krótki odbiór w GUI, gdy użytkownik nie korzysta z myszy: uruchomić nową pustą scenę, sprawdzić wyłącznie 3 kategorie i upuszczenie naszych dróg, chodników i aktora. Przed kolejnym etapem sprawdzić aktualne HEAD i oba repo. Nie przedstawiać historycznych testowych XODR jako aktywnych scen. Aktualne pełne SHA i wyniki `-Action finish` potwierdzić po commit/push.

## 14.11.2026 — poprawka biblioteki: kolejne drogi na planszy (po 5b)

**Naprawa wykonana, pełne testy automatyczne PASS, niezapisane okno użytkownika nienaruszone.** Powód zgłoszenia „po pierwszej drodze nie mogę upuścić drugiej” był potrójny: (1) szablon z biblioteki trafiał w zaznaczony koniec starej drogi i narzędzie `CreateRoadTool` interpretowało go jako polecenie **przedłużenia** tej samej drogi; (2) ponowny drop przy już aktywnym `Create Road` dopisywał punkty do niezakończonego szkicu; (3) w trybie `Scenario` akcja `Create Road` była wyłączona i drop niczego nie aktywował. Teraz `MainWindow::on_library_drop` dla `RoadTemplate` przełącza Scenario→Map, czyści wybór starej drogi, aktywuje narzędzie i fokus, a `CreateRoadTool::begin_at` zaczyna **nową** sesję bez trybu extend. Zwykłe, świadome wydłużanie zaznaczonej drogi przez narzędzie toolbaru nadal działa. Toast, wskazówka narzędzia i etykieta biblioteki mówią po polsku, że do utworzenia drogi trzeba dodać punkty i zatwierdzić Enter/dwuklikiem.

- Kod: `editor/src/app/main_window.cpp`, `editor/src/tools/create_road_tool.cpp/.hpp`, `editor/src/panels/library_panel.cpp`. Testy: `editor/tests/test_create_road_tool.cpp` (druga i trzecia droga, stara wybrana, drop na jej końcu, Undo/Redo i zapis XODR; nowy drop zastępujący niedokończony szkic); `editor/tests/test_main_window_layout.cpp` (prawdziwe zdarzenia Qt `QDragEnterEvent`/`QDropEvent` do `ViewportWidget`, kliknięcia i Enter, zapis przez menu, realna liczba `<road>` w pliku **1→2→3**; między dropami tryb Scenario i automatyczny powrót do Map; bez błędów diagnostycznych).
- **Testy i build:** początkowo bezpieczny `TARGET_BUILD_PASS` na testach; po zamknięciu poprzedniego okna przez użytkownika **`scripts/prawko-build-win.ps1 -Target all -Jobs 3` → `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`** i finalny build `roadmaker-editor` + `roadmaker_editor_tests` → PASS. **77 celowanych Qt PASS**, pełny Qt **1339 PASS, 1 SKIPPED (test symlink), 0 FAIL** z 1340 testów i prawdziwym esmini 3.5.0; logi `archive/road-multi-drop-20261114/qt-targeted.log`, `qt-full.log`. Niezależny plik `roadmaker-editor-multiroads.exe` (SHA256 `75A92652A19CC16AB1E0781A794211BE2E5673F9DD26BD003D51244E210A1309`) powstał jeszcze przed normalną kompilacją; **standardowy `build/dev-windows/editor/roadmaker-editor.exe` został następnie przebudowany pomyślnie**. Testy integracyjne używają wyłącznie kopii / `QTemporaryDir`.
- **Ręczny odbiór kursorem Windows — PASS:** na początku poprzedni edytor użytkownika PID **11064** z `scene.xodr*` był otwarty i niezapisany, dlatego go nie dotykano. Gdy użytkownik sam zamknął swoje okno (kontrola procesów pokazała brak `roadmaker-editor`), wykonano normalny build, uruchomiono oddzielne okno PID **6160** na **kopii** `archive/road-multi-drop-20261114/gui/roads-acceptance.xodr`, a realnym systemowym kursorem przeciągnięto pierwszy szablon `road.prawko.urban` z Library na trawę i zatwierdzono kolejnym kliknięciem + Enter; potem bez przełączania narzędzia przeciągnięto drugi szablon z biblioteki i zatwierdzono. Screeny `gui/baseline.png`, `drop1.png`, `finish1.png`, `drop2.png`, `finish2.png`, `saved.png` pokazują realny przebieg z trzema widocznymi jezdniami. Zapisana kopia XODR ma dokładnie **3 drogi** (id 1 `Ulica szkolna`, 2 `Road 2`, 3 `Road 3`) — oryginalny sample i sceny użytkownika pozostały nietknięte. Okno testowe zostało normalnie zamknięte. Podczas testu zauważono, że dolny licznik błędnie nadal pokazywał `1 roads` aż do ponownego otwarcia; osobnym małym patchem po odbiorze podłączono go do `Document::topology_changed` i dodano regresję w teście Qt obejmującą licznik 1→2→3 i Undo/Redo. Nie twierdzić, że ten ostatni licznik był już ręcznie przetestowany myszą — regresja Qt po poprawce jest PASS.
- **Handoff:** `agent_head=35c48c11cdb7efca8e08d1be731463657bbe7a32`, agent repo bez zmian. Pierwsza poprawka 3 przyczyn została scalona do commitu **`98244ac2bf5059600019b97b6c03f944c2f3c396`**, wypchnięta na `feature/realistic-road-surfaces` i po niej `scripts/prawko-git.ps1 -Action finish` potwierdził `QUALITY_GATE_PASS`, `SESSION_FINISH_PASS`, kopie obu repo + scen. Następna mała zmiana licznika po ręcznym odbiorze wymaga osobnego końcowego commitu, push i ponownego `-Action finish`. Przy kontynuacji zaczynać od najnowszego HEAD, pełnego Quality Gate i raportu aktualnej sceny. Sam drag z biblioteki nadal rozpoczyna rysowanie: zakończenie wymaga punktu i Enter/dwukliku.

## Stan bieżący

- **Punkt wyjścia etapu 5a.** Edytor `feature/realistic-road-surfaces` na `798812aec48fd3d5e53430626779073d4fd4e968`, agent `main` na `35c48c11cdb7efca8e08d1be731463657bbe7a32`. `prawko-resume.ps1 -Verify` potwierdził czyste katalogi i zgodność obu z `origin` przed zmianami.
- **Wdrożono:** przycisk `Scenario` i `View → Scenario Mode`. Tryb wyświetla narzędzia uczestnika oraz trasy. `File → Export Scene Image…` zapisuje PNG w rozdzielczości aktualnego widoku z bieżącej kamery. Eksport pomija siatkę, prowadnice, podświetlenia i narzędzia, zapis przez `QSaveFile`.
- **Wdrożono przebieg w tle.** `Scenario → Run Simulation` uruchamia esmini bez okna, z możliwością anulowania i limitem 60 s czasu rzeczywistego. Edytor używa `dat2csv` z katalogu tej samej dystrybucji. Wynik w pamięci ma parę XODR/XOSC ze skrótem SHA-256, wersję silnika, krok 0,05 s, ziarno 123, próbki pełnej pozy aktorów, identyfikatory dróg/pasów, start i koniec zdarzeń oraz kolizje. Raport wskazuje zdarzenia nierozpoczęte, niedokończone lub o niejednoznacznych nazwach. Edycja lub otwarcie innej sceny oznacza raport jako pochodzący z poprzedniej wersji.
- **Wdrożono odtwarzanie w widoku 3D.** Panel `Simulation` ma `Play/Pause`, `From Start`, suwak z kreskami zdarzeń, listę `Jump to event…` i `Return to Editing`. Podgląd pobiera pozycję i orientację każdego aktora z telemetrii, interpoluje między sąsiednimi próbkami, zachowuje rozmiar bryły aktora i aktualizuje tylko transformacje instancji. Kamera i eksport PNG korzystają z bieżącej klatki. Zmiana geometrii (także podczas przeciągania), scenariusza albo otwarcie innego dokumentu wygasza podgląd i pozostawia raport.
- **Wdrożono eksport MP4.** Po aktualnej symulacji wybierz `Simulation → Export MP4…` lub `File → Export Scenario Video…`. Ustaw zakres czasu, 720p lub 1080p oraz 24, 25, 30 albo 60 kl./s. Film bez dźwięku korzysta z bieżącej kamery i czystego renderu widoku 3D. Aplikacja wykrywa FFmpeg obok edytora lub na `PATH`, pozwala też wskazać plik z obsługą `libx264` i zapamiętuje ścieżkę po sukcesie. Pokazuje postęp i umożliwia anulowanie. Dotychczasowy plik pozostaje na dysku aż do udanego zakończenia kodowania.
- **Weryfikacja.** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, **2072 PASS C++** (10 wyłączonych), **7 PASS Python**, **1316 PASS Qt + 1 SKIPPED** z esmini 3.5.0 i FFmpeg 9.0.2, `QUALITY_GATE_PASS`. GUI Windows wyeksportowało z kopii `run-maneuver.xodr` zakres 1,95–4,95 s. `ffprobe` potwierdził H.264, 1920 × 1080, 30 kl./s, 90 klatek i 3,000 s. W klatkach 0 i 60 oba pojazdy przesuwają się zgodnie z telemetrią, a Target zmienia pas. Anulowanie ponownego eksportu zachowało SHA-256 filmu, otwarcie innej sceny wyłączyło eksport poprzedniego przebiegu. Test integracyjny potwierdził ochronę pliku także przy błędzie przechwycenia i błędzie procesu. Dowody i film są w `archive/simulation-stage3-20261113/`. Skróty kopii XODR/XOSC i oryginalnej sceny miejskiej pozostały identyczne.
- **Wdrożono spójny zapis i odzyskiwanie.** Edytor przygotowuje XODR, XOSC, ustawienia widoku i ewentualny teren w osobnym katalogu. Zachowuje poprzednie pliki na czas wymiany. Błąd pozostawia dokument niezapisany, a dziennik przerwanego zapisu jest wycofywany przy następnym otwarciu lub zapisie. `Save As` aktualizuje `LogicFile` i nadaje kopii terenu nazwę od nowej sceny. Autosave obejmuje XOSC i widok, a kopie nadal aktywnej sesji nie pojawiają się na liście odzyskiwania. Starsze autosave odzyskują ostatni zapisany XOSC z pierwotnego katalogu.
- **Odbiór zapisu.** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, `QUALITY_GATE_PASS`. Zaliczono **2072 C++** (10 wyłączonych), **7 Python**, **1322 Qt + 1 SKIPPED**, w tym **53/53 regresji zapisu**. W Windows odzyskano kopię z `event_recovered` i kamerą ortograficzną, następnie wymuszono błąd XOSC. Edytor zachował oznaczenie niezapisanych zmian, a obok blokady nie pojawiły się XODR ani ustawienia widoku. Zapis pod nową nazwą dał komplet plików. Ponowne otwarcie zapisanej kopii w edytorze i uruchomienie symulacji potwierdziło start odzyskanego zdarzenia o 1,95 s oraz koniec o 3,90 s. SHA-256 kopii źródłowej i oryginalnej sceny miejskiej pozostały identyczne. Dowody są w `archive/scenario-stage4-save-20261113/`.
- **Wdrożono kamerę kierowcy i scenę dydaktyczną.** W panelu `Simulation` wybór `Camera` zapisuje się z dokumentem. Podgląd, PNG i MP4 wyliczają pozycję oraz orientację oczu z tej samej interpolowanej pozy aktora. Klatka bez pozy wybranego pojazdu używa kamery edytora. Na ekranie startowym są dwa warianty ćwiczenia o widoczności przed przejściem. Parkujący dostawczak zasłania pieszego w jednym wariancie. Ruch pieszego i auta jest taki sam, a oba przebiegi kończą się bez kolizji.
- **Odbiór kamery i wariantów.** W Windows zapisano i ponownie otwarto dwie kopie oraz sprawdzono przywrócenie `Driver view — Auto`. Wyeksportowano po dwa filmy MP4 z kabiny i z góry. Wszystkie mają H.264, 1920 × 1080, 30 kl./s, 302 klatki i 10,067 s. W 2,70 s pieszy jest widoczny w wariancie bez dostawczaka oraz zasłonięty w drugim. W 5,40 s widać go już na przejściu także w drugim wariancie. Zgodność auto/pieszy potwierdza telemetria esmini 3.5.0. Walidator agenta zgłosił zero błędów dla obu par. `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, **1325 Qt + 1 SKIPPED**, **2072 C++** (10 wyłączonych), **7 Python**, `QUALITY_GATE_PASS`. Dowody są w `archive/scenario-stage4-driver-20261114/`. Pliki obu przykładów i oryginalna scena miejska zachowały skróty SHA-256.
- **Wdrożono edycję kursorem.** Tryb `Scenario` otwiera bibliotekę **Uczestnicy ruchu**. Upuszczenie pojazdu zaznacza go i włącza `Trasa`. Kliknięcie połączonego pasa wyznacza trasę od miejsca postawienia. Pieszego można postawić poza jezdnią i kliknąć punkty przejścia. Węzły można przeciągać, a `Zakończ trasę` lub Enter wraca do zaznaczania. Ścieżka pieszego jest zapisywana jako otwarta, czasowa polilinia w `FollowTrajectoryAction` w `Init`. Używa prędkości 1,4 m/s i zatrzymania na ostatnim punkcie. Początkowa prędkość pojazdu bez własnej nastawy to 30 km/h. Złożone trajektorie importowane pozostają w XOSC. Storyboard jest w menu `Scenario → Storyboard…`.
- **Weryfikacja edycji kursorem.** Kompilacja i synchronizacja biblioteki Pythona ukończone. **1329 Qt PASS + 1 SKIPPED**, **2072 C++ PASS** (10 wyłączonych), **7 Python PASS**, `QUALITY_GATE_PASS`. Test Qt dostarcza zdarzenia `QDragEnterEvent`, `QDropEvent` i kliknięcia do rzeczywistego `ViewportWidget`, następnie zapisuje XOSC z trasą pojazdu i pieszego. Osobny test potwierdził zapis, odczyt i Undo polilinii. Esmini 3.5.0 potwierdził ruch pieszego z y = −6 do +6 m i pozostanie na ostatnim punkcie. Logi w `archive/scenario-cursor-20261114/`. Skróty oryginałów 4/4 zgodne.
- **Otwarte.** Trajektorie wyzwalane przez zdarzenia (w tym ruch pieszego z przykładów przejścia) nie mają jeszcze edycji węzłów kursorem. Pojazd korzysta z trasy do wyboru drogi, a zatrzymanie na końcu wymaga osobnej akcji hamowania. W działającym oknie Windows potwierdzono widoczność palety, lecz narzędzie przechwytywania nie udostępniło obrazu ani geometrii do ręcznego przeciągnięcia. FFmpeg jest zewnętrzną zależnością. Podgląd używa brył dla `Vehicle` i `Pedestrian`. Odtwarzanie świateł i obsługa aktorów z katalogów pozostają do rozbudowy. Relatywne odwołania do zewnętrznych modeli i katalogów w importowanym XOSC mogą wymagać dostosowania przy zapisie do innego folderu.

## 14.11.2026 — etap 5b, podetap: zaznaczanie uczestników i przycisk symulacji

**Kod i testy automatyczne potwierdzone; odbiór pełnego GUI Windows otwarty.** Przejęto bez cofania 17 zmian pozostawionych po wyczerpaniu limitu poprzedniego agenta na `c12f872`. Zmiany w `ViewportWidget`, `picking`, `actor_placement`, `SelectTool`, `ActorTool`, `RouteTool` trafiają promieniem w widoczną obróconą bryłę aktora przed drogą, ograniczają chwytanie najbliższą przeszkodą i podświetlają zaznaczonego uczestnika. `MainWindow` ma akcję `run_scenario_simulation` na głównym toolbarze, zaraz po `Scenario`; dostępność zależy od uczestników i aktywnej symulacji, klik otwiera dock oraz uruchamia odtwarzanie po obliczeniu przebiegu. `Select` i `Trasa` opuszczają podgląd, aby można było znowu edytować.

- Testy kompilacji: `cmake --build build/dev-windows --target roadmaker_editor_tests` **PASS** bez naruszania otwartego `roadmaker-editor.exe`; komplet Qt **1326 PASS, 6 SKIPPED, 0 FAIL** (`archive/scenario-stage5b-20261114/stage5b-full-qt.log`); testy celowane **79 PASS, 5 SKIPPED** (`stage5b-targeted.log`), obejmują ray-pick, toolbar i edycję trasy.
- `scripts/prawko-resume.ps1 -Verify` na niezacommitowanym stanie wykonał **2072 PASS C++**, **7 PASS Python**, `QUALITY_GATE_PASS`, ale zwrócił kod 1 wyłącznie przez dirty Git (skrypt celowo wymaga czystego repo). Test zewnętrznego esmini (`SimulationPanel.ToolbarRunStartsPlaybackWhenTheCurrentRunFinishes`) pozostał SKIPPED, bo w środowisku procesu nie odnaleziono `ESMINI_PATH`.
- **Uwaga na niezapisany dokument użytkownika:** aktywny proces `roadmaker-editor.exe` PID 12196, tytuł `scene.xodr*`. Nie zatrzymywać, nie podmieniać wykonywalnego edytora w czasie jego działania, nie zapisywać ani nie wykonywać akcji myszą w tym oknie. Z tego powodu nie przeprowadzono pełnego `prawko-build-win.ps1 -Target all` ani fizycznego testu przycisku/myszy w nowym EXE. Testy Qt są `offscreen`, nie zastępują odbioru w widocznym Windows.
- **Pierwsze następne zadanie:** po zamknięciu przez użytkownika niezapisanej sceny (albo budowie odizolowanego EXE do testów) wykonać pełny build, test GUI na osobnej scenie: klik auta/pieszego bez zaznaczania drogi, podświetlenie, włączanie/wyłączanie trybu `Trasa`, działanie przycisku `Uruchom symulację` i automatyczne Play z prawdziwym esmini; poprawić ewentualne błędy. Następny podetap 5b obejmuje ustawienia prędkości i edycji trasy (opisane poniżej), później edycję chwili startu, zachowania na końcu oraz ścieżek wyzwalanych zdarzeniami bez utraty triggerów. Nie oznaczać całego etapu 5b jako ukończonego. Oryginały scen niezmienione.

## 14.11.2026 — etap 5b, podetap: prędkość w km/h i Edytuj trasę

**Kod i testy Qt potwierdzone; odbiór nowego UI w odizolowanym oknie Windows pozostaje otwarty.** W panelu Properties dla zaznaczonego uczestnika pole `Prędkość początkowa` prezentuje teraz km/h, a `push_actor_speed` zapisuje m/s w istniejącym `osc::edit::set_entity_init_speed`. Test pilnuje, aby zaokrąglenie do 0,01 km/h nie nadpisywało precyzyjnej wartości prędkości importowanej z XOSC i nie tworzyło zbędnego kroku Undo. Nie ingeruje to w nieobsługiwany `RelativeTargetSpeed` (pozostaje tylko do odczytu). Nowy przycisk `Edytuj trasę` w Properties przekazuje zaznaczonego aktora do istniejącego `MainWindow::actions_->tool_route`, bez tworzenia alternatywnego modelu rysowania, zapisuje te same trasy XOSC.

- `editor/tests/test_panels.cpp`: test pola km/h→m/s (54→15), brak wpisu Undo dla zera i zaokrąglonego odczytu, Undo/Redo, oraz klik przycisku tylko przy właściwym zaznaczeniu. `editor/tests/test_main_window_layout.cpp`: integracyjny test kliknięcia `Edytuj trasę` w miejsce ręcznego otwarcia narzędzia; zmiana trasy trafia do XOSC.
- Zbudowano `roadmaker_editor_tests` przez MSVC/Ninja **bez podmiany otwartego pliku roadmaker-editor.exe**; testy celowane `178 PASS, 5 SKIPPED` w `archive/scenario-stage5b-20261114/stage5b-targeted.log`; pełny Qt `1327 PASS, 6 SKIPPED, 0 FAIL` w `stage5b-full-qt.log` (pięć testów esmini bez `ESMINI_PATH`, jeden test symlink zgodnie z poprzednim pominięciem). Wcześniejszy Quality Gate na tym samym bazowym HEAD: 2072 C++ PASS, 7 Python PASS, QUALITY_GATE_PASS. Ponowny `-Action finish` po commicie zweryfikuje oba repo.
- `roadmaker-editor.exe` PID 12196 zawiera nadal **niezapisaną** scenę `scene.xodr*`: nie zatrzymywać, nie uruchamiać podmiany exe ani nie przejmować jego myszy. Nie przetestowano dlatego fizycznej interakcji w tym nowym buildzie. Oryginalne sceny użytkownika pozostają nietknięte.
- **Pierwsze otwarte zadanie 5b:** bezpieczny osobny GUI build/test na kopii i odbiór myszy dla wyboru brył, edycji trasy, km/h i `Uruchom symulację`; wskazać dostępny binarny esmini z `dat2csv` i potwierdzić Auto Play. Potem w odrębnym podetapie: edytowalna chwila startu, zachowanie na końcu trasy, trasy wyzwalane zdarzeniami BEZ utraty warunków `StartTrigger`; nie utożsamiać trasy po pasach z automatycznym zatrzymaniem auta. Nie oznaczać 5b jako kompletnego.

## 14.11.2026 — etap 5b: swobodne przemieszczanie uczestników i pełny odbiór Windows

**Wykonane i potwierdzone:** naprawiono brak przeciągania samochodu w trybie **Select** i **Move**. Wcześniejszy `SelectTool::mouse_move` świadomie ignorował `PressState.actor`, choć oddzielny `ActorTool` umiał przestawiać aktora. Oba narzędzia korzystają teraz z tej samej logiki przeciągania; rozpoznanie bryły w `ViewportWidget` obsługuje także hover. LMB w aktora nie rozpoczyna przesuwania jezdni. `Ctrl`/`Shift` zachowują funkcję wielokrotnego zaznaczania, `Esc` anuluje gest. Podgląd upuszczenia używa `ActorTool`.

- **Zachowanie XOSC:** zmiana pozycji pojazdu z trasą `Init/AssignRouteAction` synchronizuje jej pierwszy waypoint wyłącznie gdy był zgodny ze starą pozycją; dwa polecenia scenariusza stanowią jedno Undo/Redo przez `QUndoStack::beginMacro/endMacro`. Gdy trasa ma niezależny początek, nie jest nadpisywana. Dla pieszego z `Init/FollowTrajectoryAction` przesuwana jest pierwsza pozycja polilinii z aktualizacją czasu pierwszego odcinka; końcowy postój i następne odcinki zachowują względne czasy. Przeprowadzenie podobnej edycji dla trajektorii wyzwalanych przez **Storyboard Event** nadal pozostaje osobnym otwartym zadaniem (nie było zmieniane).
- **Testy Qt:** pełna kompilacja `scripts/prawko-build-win.ps1 -Target all -Jobs 3` zakończyła się `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`. Z oficjalnymi esmini **3.5.0** i `dat2csv` ustawionymi przez `ESMINI_PATH`: **1336 PASS, 1 SKIPPED (symlink), 0 FAIL** z 1337 Qt testów. Log `archive/scenario-stage5b-actor-drag-20261114/qt-full.log`. Regresje: `SelectTool.DraggingCarAcrossLaneMovesOnlyActorWithOneUndoAndRedo` w Select/Move; `CarCanBeDraggedOntoAnotherRoadWithoutMovingEitherRoad`; `ActorInvalidDropAndEscapeLeaveRoadAndActorUntouched`; `ActorTool.MovingVehicleMovesMatchingRouteStartInOneUndo`; `MovingPedestrianAlsoMovesAuthoredWalkingStartAndTiming`. Testy prawdziwego esmini, wcześniej SKIPPED, tym razem przeszły. Binary esmini dostępne lokalnie w `archive/scenario-stage5b-actor-drag-20261114/esmini/extracted/esmini/bin/esmini.exe` (archiwum ignorowane przez Git).
- **Pełny test systemową myszą, tylko kopia** `archive/scenario-stage5b-actor-drag-20261114/gui/accept-actor.{xodr,xosc,rmscene.json}`: kliknięcie samochodu Auto, przeciągnięcie wzdłuż pasa w górnym widoku, porównanie zrzutów `baseline.png`, `clicked.png`, `dragged.png`, `undo.png`, `redo.png`, `saved.png`, `reopened.png`, `run.png`, `returned-edit.png`. **Undo/Redo wizualnie PASS**, plik XOSC po Ctrl+S i ponownym otwarciu: `Auto` i pierwszy punkt trasy mają zgodne `s=39.08641772143433 m` (przed 12 m), ten sam `road=1`, `lane=-1`; Pieszy `x=58, y=-6.6` bez zmian. W XODR zapis normalizuje istniejący atrybut pasa środkowego `level=\"false\"`, przez co surowy SHA pliku po zapisie jest inny; **geometria planView, elevationProfile, lateralProfile, pasy, obiekty, sygnały i długość drogi są semantycznie identyczne** (`gui/compare-road.py`). Oryginalny sample `assets/samples/prawko_crosswalk_clear.*` i sceny użytkownika nie były zapisywane.
- **Odbiór przycisku i symulacji:** ponownie otwarta kopia, klik `Uruchom symulację` na górnej belce, wynik **Simulation finished**, automatyczne Play (widoczny Pause i postępujący suwak), kamera `Driver view — Auto`, esmini 3.5.0: **2 aktorów, 404 próbki, 10.05 s**. Po zakończeniu podgląd wrócił do edycji, a okno testowe zostało normalnie zamknięte. Żaden katalog oryginalnego projektu XODR/XOSC ani niezapisana scena użytkownika nie zostały naruszone.
- **Ograniczenia / następny krok 5b:** ścieżki wyzwalane eventami (np. pieszy z przykładu `prawko_crosswalk_clear`) nadal nie mają obsługi przeciągania powiązanej trajektorii z zachowaniem `StartTrigger`; wymagają osobnego testu i implementacji. Pozostają ustawienia czasu rozpoczęcia ruchu i zachowania na końcu trasy przy wybranym uczestniku, w tym hamowania pojazdu. Nie deklarować całego 5b zakończonym ani pełnego fotorealizmu. Przed kontynuacją nowy agent uruchamia `prawko-resume.ps1 -Verify`.

## Aktywny plan

**Aktualny priorytet tej gałęzi (16.11.2026):** etap poprawy renderera opisany na początku dokumentu — cache instancji, filtrowanie tekstur, stabilizacja cienia i niezależny odbiór GPU. Główne R6b2j jest już na HEAD 0af977e; integracja pozostaje otwarta. Po odbiorze tej gałęzi scalić obie prace, a następnie profilować pełną jazdę i wdrażać pomijanie niewidocznej geometrii/LOD. Poniższe zalecenia A1 i historyczne wyniki pozostają kontekstem, nie nakazem ponownego modelowania wiaty.

### PRIORYTET A1 — Prawko Assets: realistyczne modele drogowe (AKTYWNY, decyzja użytkownika 15.11.2026)

**Decyzja projektowa:** kończymy etap rozwoju **podstawowych funkcjonalności** PrawkoRoadStudio i przechodzimy do systematycznej produkcji **własnych, szczegółowych, realistycznych assetów 3D** dla czterech aktywnych scen `projects/Prawko-Scenariusze/`. To jest **pierwsze zadanie dla każdego kolejnego agenta**. Nie zaczynać od nowych paneli, silników, edytorów siatek, uniwersalnych mechanizmów komponentów ani dodatkowych scenariuszy, jeśli nie blokują importu lub wizualnej jakości modeli. Poniższe etapy VICOM (1–5) są historią i backlogiem dalszym, **nie zastępują priorytetu A1**. S3.3 nie jest globalnie zamknięte; niezakończone funkcje zostają odłożone.

**Pierwszy podetap — model wiaty: WYKONANY, PRÓBNE WDROŻENIE PASS.** Autorska wiata powstała w Blenderze 5.3 i została zaimportowana w formacie GLB wraz z miniaturą i manifestem. Test rzeczywistego interfejsu Qt sprawdził drag & drop, zmianę pozycji i obrotu, zapis/odczyt; 156 obiektów modelu ma osobną scenę `05-wiata-test`. Zachowanie częściowej transparentności i roughness w rendererze zostało poprawione. **Etapy UV/PBR/szkła, cieni słonecznych i trzech lusterek przeszły techniczny odbiór A1/R1–R5; pełny parytet fotometryczny i cienie od reflektorów nadal OTWARTE.** Dowody i lokalizacja plików są w najnowszej sekcji A1 na początku HANDOFF. **Bieżąca kolejność (decyzja 16.11.2026):** R4b3b — rzeczywisty test A/B reflektorów ruchu, potem ich cienie jako osobny etap; R6 — realistyczny wzorzec drogowy z pełną geometrią nawierzchni, poboczy i autorskimi modelami Blender; R7 — pojedynczy kompletny scenariusz egzaminacyjny. Zamykać wyłącznie po obrazach Windows, testach GUI i pełnym Quality Gate.

**Bramka jakości przed produkcją seryjną (częściowo potwierdzona — nadal niezamknięta):** obejrzeć **rzeczywisty renderer Qt**, porównać geometrię i materiały z Blenderem (w szczególności szkło/przezroczystość, tekstury/PBR, metal, cienie i odbicia), sprawdzić skalę i orientację, wybór myszą, transformacje i trwałość zapisu, widok kierowcy i trzy lusterka oraz dzień/noc/deszcz. W razie wykrycia braków renderera wykonać **wyłącznie poprawki konieczne do prawidłowego wyświetlenia modeli**, z osobnym testem regresyjnym. Zmierzyć podstawową wydajność przy powtórzeniu obiektu w scenie. Nie deklarować PBR, odbić, szkła ani wydajności jako PASS bez rzeczywistego wyniku. Potwierdzić Quality Gate, commit/push i aktualizację tego `HANDOFF.md`.

**Po pozytywnym odbiorze pojedynczego assetu:** ustalić spójny standard wymiarów w metrach, osi, pivotów, nazw i materiałów, budżetów geometrii/tekstur, wersjonowania oraz miniatur w istniejącej bibliotece. Dopiero wtedy zacząć **seryjne modelowanie** otoczenia do czterech nowych scen: krawężniki i nawierzchnie, budynki, chodniki, ogrodzenia, drzewa i roślinność sezonowa, znaki i infrastruktura, wiaty oraz pojazdy i piesi. Stosować prawdziwą geometrię i materiały, nie płaskie imitacje ani dekoracyjne nakładki.

**Ograniczenia nadal otwarte, ale niepriorytetowe:** pełna telemetria kokpitu i działające przełączniki, reflektory nocne, dopracowanie pogody, kolejne zdarzenia oraz bardziej rozbudowane UI. Wracać do nich, gdy staną się rzeczywistą przeszkodą dla jakości scen albo po zakończeniu etapu Prawko Assets. Nie modyfikować istniejących zapisanych scen użytkownika ani historii Git bez potrzeby.

### Wcześniejsze etapy i odłożony backlog (nie zaczynać od tych punktów)

### 1. Ukończono — kontrolowany przebieg esmini w tle

Punkt wejścia stanowią [panel symulacji](editor/src/panels/simulation_panel.cpp) i [wynik przebiegu](editor/src/document/esmini_simulation.hpp). Testy z prawdziwym silnikiem uruchamiają się po ustawieniu `ESMINI_PATH` na plik esmini z dystrybucji zawierającej również `dat2csv`.

### 2. Wdrożono — odtwarzanie w istniejącym widoku Qt

Punkt wejścia stanowią [panel symulacji](editor/src/panels/simulation_panel.cpp), [odczyt klatek przebiegu](editor/src/document/simulation_trace.hpp) oraz `ViewportWidget::set_simulation_frame()`. Podgląd i przechodzenie po klatkach pozostawiają mapę, scenariusz i Undo w dotychczasowym stanie. Odbiór aktora i trasy myszą powtórz na kopii przy następnym działającym przechwytywaniu okna.

### 3. Ukończono — pierwsze nagranie MP4

Punkty wejścia to `MainWindow::export_scenario_video()`, [proces eksportu](editor/src/document/scenario_video_export.cpp) i `ViewportWidget::capture_simulation_frame()`. MP4 i PNG korzystają ze wspólnego renderera. Próbkowanie filmu używa `SimulationPlayback::poses_at()` i czasu wyliczanego z numeru klatki.

### 4. Scena wzorcowa, kamery i bezpieczny zapis

**4a. Wdrożono bezpieczny zapis i odzyskiwanie.** Punkty wejścia to `Document::save()`, `Document::load_recovery()`, [moduł zapisu](editor/src/document/scene_storage.cpp) i `AutosaveManager`.

**4b. Wdrożono kamerę kierowcy i scenę wzorcową.** Punkty wejścia to `SimulationPanel::present_frame()`, `ViewportWidget::simulation_frame()` i `driver_camera()`. `SceneState::driver_view_actor` przechowuje nazwę wybranego pojazdu. Przykłady `prawko_crosswalk_clear` i `prawko_crosswalk_obstructed` są w `assets/samples/`.

### 5. Dalsze funkcje wzorowane na VICOM

**5a. Wdrożono bibliotekę uczestników i rysowanie ruchu kursorem.** Punkty wejścia to `MainWindow::on_library_drop()`, `RouteTool` i `osc::edit::set_init_polyline()`. Instruktor ma dodawać i edytować ruch kursorem. Parametry techniczne są dostępne na żądanie.

**5b. Backlog wstrzymany na czas priorytetu Prawko Assets — wygodne ustawienia ruchu i odbiór kursorem.** Na kopii zwykłej mapy sprawdź realną myszą przeciągnięcie auta i pieszego, wyznaczenie tras, korektę węzła, cofnięcie, ponowne otwarcie i uruchomienie. Dodaj przy zaznaczonym uczestniku czytelne ustawienia prędkości (km/h), chwili startu i zachowania na końcu oraz przycisk edycji trasy. Podepnij ustawienia do tego samego XOSC i pozwól edytować obsługiwane ścieżki zdarzeń bez utraty wyzwalaczy. Kryterium odbioru to ustawienie auta i pieszego oraz nagranie przejścia w jednym oknie bez otwierania Storyboard dla typowej sceny.

**5c. Szablony i warianty.** Dodaj tworzenie niezależnej kopii szablonu z ekranu startowego i menu `File`, z zachowaniem spójnego zapisu i kontroli niezapisanych zmian. W dalszym kroku dodaj szablony miejskie, pozamiejskie i autostradowe, podmianę otoczenia, przypisywanie ról pasów i stref oraz raport odwołań do korekty.

**5d. Warunki, kamery i nagrania (częściowo; dalsza rozbudowa odłożona na czas priorytetu Prawko Assets).** Są trzy rzeczywiste kamery lusterek, cztery pory dnia, cztery warianty pogody oraz trzy pory roku z zapisem per scena. Początkowy kokpit 2D zastąpiono własnym **modelem wnętrza 3D z Blendera**: prawdziwa deska, obudowa kabiny, ruchoma kierownica wyliczana z trajektorii, dwa ruchome pióra wycieraczek powiązane z czasem symulacji, czytelna prędkość km/h. Zweryfikowano rzeczywiste kadry OpenGL z drogą przed samochodem, deszczem i lusterkami. **Najbliższe zadanie:** pełne sterowanie i telemetria wyposażenia (światła, kontrolki, obrotomierz, paliwo, pedały), dynamiczny odbiór MP4/FPS w ruchomej scenie esmini oraz efekty reflektorów. Cały 5d nie jest jeszcze ukończony.

## Gotowe polecenie do nowej rozmowy

> Kontynuuj gałąź codex/render-quality-performance w F:\PrawkoRoadStudio\archive\render-opt. Przeczytaj najnowszy wpis tego HANDOFF i AGENTS. Sprawdź status obu repo, testy i dowody sprzętowe etapu renderera. Główna gałąź feature/realistic-road-surfaces jest rozwijana równolegle przez innego agenta (R6b2j). Nie nadpisuj jej zmian ani scen. Po zakończeniu tamtej pracy przejrzyj i scal oba etapy, następnie profiluj pełny przejazd z lusterkami/odbiciami przed dalszą optymalizacją. Wykonaj testy, commit, push i finish w odpowiednim katalogu roboczym.

## Historia prac

Poniższe wpisy dokumentują wcześniejsze odbiory i wyniki. Bieżące zadanie znajdziesz w sekcji **Aktywny plan**.

## 13.11.2026 — odtwarzanie w edytorze Windows

- `c26421ea81a95c241a149e89e6e52704310c1406` dostarczył odtwarzanie w 3D, pauzę, suwak i znaczniki zdarzeń. W klatkach 0,00 / 1,95 / 3,90 s na kopii `run-maneuver.xodr` przesunięcia obu brył zgadzały się z telemetrią z dokładnością około 1 px. Test Qt porównał skok z listy zdarzeń z pozycją esmini i potwierdził wygaszenie po edycji drogi. Dowody są w `archive/simulation-stage2-20261113/`.

## 13.11.2026 — pierwszy przebieg esmini w tle

- `5bd64dada0aa1c2b7efc4535d5913030c1d8f085` dostarczył panel raportu, anulowanie i telemetrię. W odbiorze Windows wariant 4 m rozpoczął `event_1` po 1,95 s i zakończył po 3,90 s. Pierwsza kolizja pojawiła się po 4,50 s. Wariant 15 m nie rozpoczął zdarzenia. Obie próby dały 35,05 s, 2 aktorów i 1404 próbki. Dowody zachowano w `archive/simulation-stage1-20261113-202759/`.

## 13.11.2026 — audyt scenariuszy dla szkoły jazdy PrawkoNaRaz

**Stan przed etapem VICOM:** przegląd kodu i obu repozytoriów, pomiary esmini oraz wcześniejsze propozycje dla kursantów są w [PRAWKO-SCENARIUSZE-DYDAKTYCZNE.md](docs/PRAWKO-SCENARIUSZE-DYDAKTYCZNE.md).

- **Stan produktu:** istnieje wspólny model C++ dróg, aktorów, tras, zdarzeń i sygnalizacji, a Qt eksportuje migawkę do esmini. W działającym Windows Qt na kopii `cut_in` widoczny jest podgląd esmini, a `View` nie zawiera `Scenario Mode`. Kod tworzy akcję bez domyślnego skrótu i podpina tryb, ale nie dodaje przełącznika do menu. `Document::save` oznacza dokument jako czysty także po błędzie XOSC. Autosave zapisuje mapę i ustawienia widoku bez XOSC.
- **Symulacja na kopiach:** świeże `cut_in` z 0 błędów walidacji dwukrotnie kończy się kodem 0 bez manewru. W esmini 3.5.0 przy kroku 0,05 s oba CSV mają taki sam SHA-256. Warunek `rising` z progiem 15 m jest spełniony już w pierwszym kroku, co kontrola `none` wykazała przy odległości 7,21 m. Próba z progiem 4 m przy `rising` wyzwala zmianę pasa o 1,95 s i zgłasza kolizję o 4,50 s. `urban_crossing` kończy próbę po 30,05 s, a oba pojazdy ruszają od skrzyżowania. Logi, telemetria i kopie testowe są lokalnie w `archive/scenario-audit/2026-11-13/`.
- **Repo i testy:** punkt wyjścia edytora `b6ed5d7bcd15a1d1643148037eb2a322b18d4e41`, agenta `35c48c11cdb7efca8e08d1be731463657bbe7a32`, oba potwierdzone w `origin`. Odzyskany test obrotu i dowód z zachowanych kopii zapisano osobno w `9552a8e6302f96599940a4c861183b1e9eec811f` i wypchnięto. Pełny build `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`, C++ 2072 PASS (10 wyłączonych), Qt 1304 PASS + 1 SKIPPED, agent 7 PASS, `QUALITY_GATE_PASS`, demo 0 błędów. Esmini 3.5.0 przyjął 3 poprawne repozytoryjne XOSC i odrzucił 1 uszkodzony. Testy esmini i GUI korzystały z kopii. Oryginalnych scen nie zapisano. SHA256 `agent/projects/prawko_urban_curve/scene.xodr` pozostał `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e`.

## S2.2o — test rzeczywistego prefabrykatu i korekta przeciągania (13.11.2026)

**Zakończony podetap: wybór cienkiego słupa, bezpieczne przesunięcie czterech elementów, Undo/Redo i zapis.** Nie oznacza pełnego końca S2.2o.

- Przy odtworzeniu błędu Traffic signal, mast arm LMB trafiało w drogę pod wąskim masztem. Dodano tolerancję 7 px do rzeczywistych rzutowanych krawędzi modelu tylko dla **już zaznaczonej grupy** w picking.cpp/.hpp, używaną przy mouse-press SelectTool. Zwykłe kliknięcie poza obiektem nie zostaje przechwycone. Test: PickScreen.ThinSelectedPrefabPoleHasLimitedPixelGrabWithoutStealingRoadClicks.
- Drugim, niezależnym błędem była próba przesuwania każdego fragmentu rm:assembly przez edit::move_object w kompozycie. Jądro poprawnie odmawia takiej operacji. SelectTool::update_object_move rozpoznaje rzeczywiste Object::assembly i korzysta z istniejącego move_object_command → **edit::move_assembly_by_part**, który przesuwa CAŁY sztywny prefabrykat jednym poleceniem. Swobodna multi-selekcja zwykłych obiektów nadal korzysta z kompozytu. Nieudana próba nie dodaje Undo ani fałszywego toastu sukcesu.
- Test SelectTool.RealFourPartSignalMastMovesAsOneRigidAssemblyOnMouseDrag: wszystkie cztery części przesunięte z zachowaniem offsetów, preview, jeden wpis Undo, byte-identical Undo, Redo. Testy docelowe **46/46 PASS**, pełne Qt **1303 PASS + 1 SKIPPED, 0 FAIL** (logi archive/visual-regression/s2.2o-qt-{targeted,full}.log). prawko-build-win.ps1 -Target all -Jobs 3: **BUILD_PASS all**, AGENT_BINDING_SYNC_PASS; prawko-resume.ps1 -Verify uruchomiony na dirty worktree: **2072 C++ PASS**, **7 Python PASS**, QUALITY_GATE_PASS, kod wyjścia 1 wyłącznie od wykrytego dirty Git.
- Rzeczywisty GUI Qt z kursorem Windows i osobną kopią archive/visual-regression/s2.2o-mouse/gui-check-20261113f.xodr: wstawiono czteroczęściowy maszt z biblioteki, kliknięto cienki element, przesunięto LMB, zapisano verified-before-f.xodr, verified-after-f.xodr, verified-undo-f.xodr, verified-redo-f.xodr. verify-assembly.py (analiza XML) potwierdza **4/4 części: identyczne Δs=+5.0914 m, Δt=−1.1220 m; geometria obu dróg BEZ ZMIAN; Undo/Redo PASS**. Zrzuty verify-gui-{placement,before-drag,after-drag,undo,redo}.png. Własny proces Qt zamknięty, oryginalnych scen nie zapisano. Wcześniejsze nieudane próby kalibracji GUI pozostawiono jako diagnostykę tylko w ignorowanym katalogu archive/.
- Odzyskany punkt kontrolny obrotu: zastany test `GizmoDrag.RotatingSignalMastAssemblyMovesAllFourPartsInOneUndo` sprawdzony w pełnym Qt (1304 PASS, 1 SKIPPED). Ponowna analiza zapisanych przez poprzedniego agenta kopii z rzeczywistej myszy (`archive/visual-regression/s2.2o-rotation-mouse2/verify-rotation.py`) potwierdziła obrót czterech części o −90°, zachowanie odległości w `s/t`, geometrii dróg i poprawność Undo/Redo. Bez osobnego odbioru ponownego otwarcia przez GUI ani innych prefabrykatów.
- Otwarte w S2.2o: odbiór ponownego otwarcia obróconej kopii, inne prefabrykaty i geometrie, stencil/marking.

## S2.2n — podetap 2: grupy prefabrykatów i końcowy odbiór myszą (13.11.2026)

**Wykonane i zweryfikowane:**

- Dokończono w `SelectTool` bezpośrednie przeciąganie `SignalId`
  (statyczny znak / dynamiczne światło) LMB w trybie Select lub Move.
  Zmienia `s/t` tylko względem **macierzystej drogi**, zachowuje
  `orientation` (kierunek obowiązywania) i `hOffset` (obrót
  wizualny). Po opuszczeniu dozwolonej odległości od drogi blokuje
  klatkę i wznawia po powrocie, `Esc` anuluje, zwolnienie
  zatwierdza **jeden** wpis Undo. Pierścień `Move` do obrotu
  znaku istniał wcześniej; potwierdzono jego działanie rzeczywistą
  myszą.
- Zaznaczone **dwa lub więcej obiektów 3D na jednej drodze**
  przesuwa się razem, jeśli użytkownik chwyci jeden z zaznaczonych
  elementów. `ObjectMoveState` zapamiętuje pozycje początkowe
  grupy; `edit::composite("Move selected prefab")` przesuwa
  je z jednakowym przyrostem `s/t` w podglądzie, jednym Undo.
  Jeśli jeden element przekracza długość macierzystej drogi albo
  dozwolony offset boczny, cała zmiana jest odrzucana, bez
  częściowego przestawienia. Dla różnych dróg pozostaje
  bezpieczny ruch jednego elementu, bez udawania translacji grupy.
  **To jest przesuw, nie obrót całej grupy.**
- `editor/tests/test_select_tool.cpp`: **5 nowych testów PASS**:
  znaki statyczne i dynamiczne światła, Undo/Redo, zachowanie
  `orientation/hOffset`, Esc i zwykły klik bez wpisu Undo,
  przebywanie poza drogą / powrót, translacja całej wybranej
  grupy i odmowa, gdy końcowy element przekracza granicę.
  `archive/visual-regression/s2.2n-targeted-final.log`.
- **Rzeczywisty Windows Qt, mysz systemowa, wyłącznie kopie scen**:
  `archive/visual-regression/s2.2n-mouse/signal-body-test.xodr`
  – po upuszczeniu znaku STOP panel Properties wskazał
  `s=134,221 m`, `t=-0,574 m`, `hOffset=0,052 rad`.
  Po bezpośrednim LMB drag:
  `s=113,762 m`, `t=1,802 m`,
  `hOffset=0,052 rad`, toast `Sign or traffic light moved`.
  Przy `Move` próba chwycenia niepodświetlonego fragmentu
  pierścienia wybrała drogę i ją przesunęła; **ten nieudany
  gest został cofnięty Ctrl+Z**. Następnie
  `sign-ring-hover-diagonal.png` pokazuje podświetloną obręcz,
  a właściwy jej obrót (`sign-ring-active-rotate-after.png`)
  zmienił **`hOffset=0,052 → -1,833 rad`** i wyświetlił
  `Rotated sign to -105° — Ctrl+Z to undo`. Wykonano
  Ctrl+Z/Ctrl+Shift+Z, Ctrl+S i zamknięto **tylko testowe PID18504**.
  Zrzuty `stop-sign-direct-move-after.png`,
  `sign-ring-active-rotate-after.png`,
  `sign-rotate-undo.png`, `sign-rotate-redo.png`.
- **Przejście na czystym wlocie T:** testowa kopia
  `archive/visual-regression/s2.2n-mouse/crosswalk-clean-test.xodr`
  powstała z `agent/projects/t_junction/scene.xodr`
  przez usunięcie 3 wcześniejszych `object type="crosswalk"`
  **tylko w kopii**. `crosswalk-clean-baseline.png` pokazuje
  T bez zebry, `clean-zebra-invalid-after.png` odrzuca
  upust poza wlotem, `clean-zebra-valid-after.png` dokumentuje
  wstawienie **jednej zebry i stop-line** (toast)
  na wlocie oraz zaznaczenie obiektu w Properties.
  Ctrl+Z/Ctrl+Shift+Z i zapis kopii; ponowny odczyt XML:
  **dokładnie 1 `object type="crosswalk"`**.
  PID24164 zamknięto normalnie; oryginalnego `t_junction`
  ani `prawko_urban_curve` nie modyfikowano.
- **Build/test:** `archive/visual-regression/s2.2n-final-build2.log`
  **`BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`**;
  `s2.2n-quality-gate.log` **`QUALITY_GATE_PASS`**;
  `s2.2n-qt-full.log` **1301 PASS, 1 SKIPPED, 0 FAIL**
  (1302 testów/160 suites).
  Przejściowy `LNK1168` w `s2.2n-assembly-build.log`
  wynikał z otwartej testowej instancji `roadmaker-editor.exe`.
  Testową scenę bezpiecznie zapisano i zamknięto, a pełny
  build w `s2.2n-final-build2.log` powtórzono do PASS.
  Oryginalne `agent/projects/prawko_urban_curve/scene.xodr`:
  SHA256 **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**,
  bez zmiany; projekt `reference/RoadMaker` nietknięty.

**Pierwsze niewykonane zadanie S2.2o:** test rzeczywistą myszą
przesuwania całego prefabrykatu `PropAssembly` (nie tylko
unit test z dwoma drzewami), a następnie parametryczny
**obrót całej grupy** z zachowaniem wzajemnych odległości,
snapowania i OpenDRIVE, także w scenach wielodrogowych.
Osobno skontrolować `Stencil`/`Marking`, blokadę
**drugiej zebry na tym samym wlocie** oraz udokumentować
końcowy wygląd z poziomu kierowcy. Nie zmieniać globalnego
CDT ani goldenów bez testu, nie oznaczać fotorealizmu PASS.

## S2.2n — podetap 1: ruch znaków/sygnalizacji myszą, test przejścia na X (13.11.2026)

**Stan: podetap wykonany w kodzie i sprawdzony na pojedynczym sygnalizatorze w widocznym GUI, lecz S2.2n jako całość otwarty.**

- Przejęto trzy zastane niezacommitowane pliki `editor/src/tools/select_tool.cpp/.hpp` i `editor/tests/test_select_tool.cpp` bez kasowania ich zmian. `SelectTool` rozpoznaje naciśnięcie na `PickHit::signal`, osobny ruch LMB po przekroczeniu tolerancji oraz aktywny preview `edit::move_signal` z przyciąganiem do **macierzystej** drogi. Nie przesuwa całej drogi; zachowuje `orientation` i `h_offset`. Esc przywraca stan, zwolnienie LMB kończy jeden Undo. Ruch poza progiem drogi zatrzymuje się na ostatniej poprawnej pozycji. Przy braku poprawnego preview zwolnienie nie udaje udanego ruchu.
- Testy `SelectTool`: statyczny znak i dynamiczny sygnalizator, snap/Undo/Redo i zachowanie kierunku, opuszczenie drogi i powrót, anulowanie Esc, zwykły klik bez Undo, odrzucenie całkowicie niepoprawnego przesunięcia bez Undo.
- Rzeczywiste zdarzenia kursora/przycisku Windows na **kopii** `archive/visual-regression/s2.2n-mouse/intersection-test.xodr` (16 dróg, skrzyżowanie X): `signal-click.png` wskazuje `signal tl3`, `s=8.000 m`, `t=-6.000 m`; po LMB drag `signal-drag-after.png` panel pokazuje `s=21.856 m`, `t=-2.239 m`, `hOffset=0.052 rad` bez zmiany. To test widocznego Qt, nie bezpośrednie wywołanie funkcji.
- W tej samej testowej kopii rzeczywisty drag Zebra crosswalk: poza wlotem odrzucony (`crosswalk-invalid-after.png`), na wlocie zgłoszony jako `Placed Zebra crosswalk + stop line`, dodany `object 11` i otwarte Properties (`crosswalk-valid-after.png`). UWAGA: ta scena **miała już** przejścia na wlotach, dlatego nie zaliczono testu wizualnej poprawności pojedynczej nowej zebry na pustym wlocie; wymagana osobna czysta scena oraz kontrola nakładania duplikatów.
- Okno kopii `PID 15600` zakończyło działanie przed kolejną próbą klawiszy Undo/Redo. Nie zgłaszać tego testu GUI ani testu obrotu gizmem jako PASS. Weryfikację jednej operacji Undo/Redo zapewniają testy Qt; materiał z GUI jest tylko dowodem przesunięcia LMB. Zastana scena użytkownika `agent/projects/prawko_urban_curve/scene.xodr` zachowała SHA256 `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e` (certutil). Nie nadpisywano oryginalnego `s2.2b-crosswalks-junction.xodr`.
- **Pełna kompilacja bieżącej wersji:** `prawko-build-win.ps1 -Target all -Jobs 3`: **BUILD_PASS: all**, `AGENT_BINDING_SYNC_PASS`. Pierwsza próba po małej poprawce trafiła na przejściowy konflikt `ninja -t restat` (Permission denied przy równoległym procesie); po ustaniu konfliktu powtórzono build i otrzymano PASS. `archive/visual-regression/s2.2n-qt-full.log`: **1301/1302 Qt PASS, 1 SKIPPED, 0 FAIL** (160 suites). Testy SelectTool w pełnym przebiegu, w tym nowy test no-op poza drogą, PASS. `-Verify` jądra + agenta odczytać z bieżącej sesji lub wykonać ponownie; Qt odpalono osobno z `QT_QPA_PLATFORM=offscreen`.
- **Ograniczenie modelu:** bezpośredni ruch znaków to `s/t` względem drogi w płaszczyźnie, a nie dowolny ruch 3D poza drogą. Istniejący gizmo odpowiada za obrót, lecz osobny odbiór obrotu myszą nadal otwarty. Format sceny OpenDRIVE, triangulacja i materiały niezmienione; repo agenta bez zmian.

**Historyczny następny krok po podetapie 1 S2.2n (zrealizowany w podetapie 2 powyżej):** utworzyć izolowaną testową scenę T/X **bez gotowych przejść** na wybranym wlocie i sprawdzić w rzeczywistym Qt poprawny/odrzucony drag zebry, stop-line, Undo/Redo i ponowne otwarcie zapisanego wyniku; rozważyć blokadę duplikatu na tym samym wlocie. Następnie analogicznie sprawdzić `Stencil`, `Marking`, złożony `PropAssembly` oraz obrót znaku i sygnalizatora gizmem; dla braków dopiero projektować zmiany kodu z testami. NIE oznaczać całej edycji złożonych prefabrykatów ani pełnego S2.2n jako zakończone.

## S2.2m — edycja komponentów bez szukania ich po upuszczeniu (13.11.2026)

**Potwierdzona implementacja:**
- W `document/library_drop.hpp/.cpp` `capture_library_entities` zapisuje
  aktywne identyfikatory obiektów, znaków i skrzyżowań **przed** operacją,
  a `created_library_entities` porównuje je z faktycznym stanem **po**
  `Document::push_command`. Nie dobiera obiektu po największym numerze,
  kolejności sceny czy najbliższym punkcie — poprawne także przy istniejących
  obiektach i przetworzonych slotach aren.
- `MainWindow::on_library_drop` po udanym dodaniu **drzew, pojedynczych
  obiektów, złożonych prefabrykatów, znaków statycznych, sygnalizacji,
  skrzyżowań, strzałek i przejść** zaznacza faktycznie dodane elementy,
  przełącza Select oraz otwiera Properties. **Materiały i oznakowanie**
  nie tworzą nowego obiektu: resolver wskazuje dokładny `RoadId + LaneId`
  i po nałożeniu podświetla zmieniony pas.
- Przejście nadal tworzy swój obiekt i powiązanie ze stop-line w jednym
  makrze Undo. Oznaczenia i materiały nadal idą przez zwykłe komendy.
  Nie zmieniono CDT, struktury sceny ani OpenDRIVE.
- `LibraryPanel` wyświetla kontekstowe instrukcje dla `Signal`,
  `Tree`, `PropAssembly`, `Crosswalk`, `Stencil`, `Material`
  i `Marking`. Pojedynczy wybór katalogowy nadal tylko wybiera
  zasób; przeciągnięcie jest osobną operacją.
- Testy `LibraryDrop.FreshlyPlacedObjectsAndSignalsAreFoundByIdentityForProperties`
  oraz `LibraryDrop.NewlyInsertedJunctionIsDistinguishedFromExistingAssets`;
  dodatkowe asercje dokładnego pasa dla `Material` i `Marking`
  oraz obiektu przejścia dla `Crosswalk`. **55/55 testów resolvera PASS**.
- **Pełny build:** `archive/visual-regression/s2.2m-edit-after-drop-final-build2.log`
  **BUILD_PASS: all, AGENT_BINDING_SYNC_PASS**. Przejściowy LNK1168
  w `s2.2m-edit-after-drop-final-build.log` wynikał z uruchomienia
  procesu testowego równocześnie z linkerem, nie z błędu kodu;
  build powtórzony sekwencyjnie i zaliczony.
  `s2.2m-qt-full-final.log`: **1295 PASS / 1 SKIPPED, 0 FAIL**
  (1296 testów, 160 suites). `s2.2m-quality-gate.log`:
  **QUALITY_GATE_PASS**.
- **Rzeczywisty widoczny Qt + zdarzenia myszy Windows**, wyłącznie na kopii
  `archive/visual-regression/s2.2m-mouse/all-assets-test.xodr`:
  odrzucony upust asfaltu poza właściwym pasem (`material-hover.png`),
  udany upust na pas z automatycznym Properties
  (`material-valid-after.png`), drzewo `Pine tree`
  z panelem `s/t, Heading, Z offset, Height` (`pine-after.png`),
  znak `Stop ahead` z edycją oznaczenia/kąta/dimensions
  (`stop-sign-after.png`), **sygnalizator dynamiczny**
  (własny nowy `signal 2`, `traffic-light-after.png`).
  Po sygnalizatorze wykonano Ctrl+Z, Ctrl+Shift+Z, Ctrl+S, zamknięto
  wyłącznie proces testowy PID 20212. Screenshoty w tym samym folderze.
  Również `traffic-light-undo.png` i `traffic-light-redo.png`.
  `agent/projects/prawko_urban_curve/scene.xodr` pozostał bez zmian:
  SHA256 `879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e`.
  Kopia sceny została zapisana osobno.

**Granice odbioru:** ręczny upust przejścia na **wlocie skrzyżowania**
nie został sprawdzony myszą w tej sesji — jedyna testowa scena miała
jedną drogę bez skrzyżowania; test C++ waliduje resolver, komendę
stop-line i identyfikację obiektu. **Nie oznaczać tego testu GUI PASS.**
Nie ma jeszcze pełnego interaktywnego przesuwania znaku lub
sygnalizatora po scenie (Properties pozwala edytować `s/t`);
samodzielne obiekty bez drogi nadal ogranicza model OpenDRIVE.
Również swobodne transformowanie całych złożonych prefabrykatów,
nowe obniżone krawężniki, wysepki i fotorealizm nadal są **otwarte**.

**Pierwszy następny krok S2.2n:** utworzyć kopię testowej sceny
T/X z wlotami; rzeczywistą myszą sprawdzić prawidłowy/odrzucony
upust przejścia + stop-line, a także stencil/marking i
złożony prefabrykat. Zbadać wygodę przesuwania znaków i sygnalizacji
myszą (z poprawnym orientowaniem, snapowaniem i Undo), dopiero potem
wdrożyć brakujące gesty. Każda zmiana z osobnymi testami CI,
odbiorem w Qt i wpisem do HANDOFF. Nie przypisywać modelowi
geometrii/fotorealizmu nieudokumentowanego w kodzie.

## S2.2l — intuicyjne drag & drop i edycja z biblioteki (13.11.2026)

**Zrealizowane i zweryfikowane, bez zmiany formatu sceny ani siatki:**

- `LibraryPanel` pokazuje krótką instrukcję zależną od wybranego
  rodzaju zasobu (rysowanie nowej drogi; stosowanie stylu do istniejącej;
  materiał/oznaczenia na pasie; skrzyżowania; ogólne obiekty).
  Kafelki nie wymagają ręcznego importu, a filtr kategorii pozostaje czynny.
- `ViewportWidget::draw_drag_ghost` wyświetla przy oznaczonym na scenie
  punkcie upuszczenia czytelny, ograniczony do kadru podpis:
  np. **„Puść, aby zmienić przekrój drogi”**. Kolor ostrzegawczy
  nadal oznacza nieprawidłową lokalizację. Pozycja podglądu i miejsce
  zastosowania nadal korzystają z jednego `resolve_library_drop`.
- Po upuszczeniu **RoadStyle** w `MainWindow::on_library_drop`
  program automatycznie włącza Select/Move, zaznacza zmienioną drogę
  i pokazuje zakładkę Properties do dalszej edycji (tylko po udanym
  `Document::push_command`, bez wpływu na inne przypadki).
  Alternatywnie: **zaznacz jedną drogę i kliknij dwukrotnie kafelek
  RoadStyle**. Polecenie trafia do zwykłego Undo/Redo; przy braku
  jednoznacznego wyboru pokazuje ostrzeżenie, nie zmienia sieci.
  Podwójny klik RoadTemplate uzbraja Create Road z jego profilem.
  Zwykły pojedynczy klik służy tylko do wyboru zasobu.
- **Weryfikacja fizyczną myszą Windows** na **osobnych kopiach sceny**:
  `archive/visual-regression/s2.2l-mouse/`
  (`mouse-test-scene.xodr`, `post-change-scene.xodr`).
  Kursor systemowy przeszedł od kafelka do jezdni; na ekranie zaobserwowano
  podświetlenie drogi, znacznik, nowy podpis **przed** zwolnieniem
  (`updated-real-style-hover.png`), toast po modyfikacji
  (`updated-real-style-released.png`), automatyczne zaznaczenie
  zmienionej drogi i rzeczywisty dwuklik
  (`updated-real-style-double-click-result.png`). Następnie wykonano
  Undo, Redo, zapis **wyłącznie** `post-change-scene.xodr`
  i zamknięto tylko testowy proces PID 7936. Ślady:
  `updated-real-undo.png`, `updated-real-redo.png`,
  `updated-real-copy-saved.png`. Nie mylić z testem myszy
  fizycznego urządzenia USB: są to **systemowe zdarzenia kursora
  i przycisku Windows**, wysyłane do rzeczywistego widocznego Qt,
  nie bezpośrednie wywołania funkcji modelu.
- **Testy:** `archive/visual-regression/s2.2l-dnd-full-build.log`:
  **BUILD_PASS: all**, `AGENT_BINDING_SYNC_PASS`;
  `s2.2l-dnd-quality-gate.log`: **QUALITY_GATE_PASS**;
  `s2.2l-dnd-full-qt.log`: **1293 PASS, 1 SKIPPED, 0 FAIL**
  (1294 uruchomione, 160 zestawów). Nowy
  `LibraryPanel.RoadStyleHasActionableHelpAndRealDoubleClick` PASS
  sprawdza teksty instrukcji i dostarczenie sygnału dwukliku
  z `QTest::mouseDClick`.
- **Nienaruszona scena użytkownika:**
  `agent/projects/prawko_urban_curve/scene.xodr`,
  SHA256 **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**
  po teście, identyczny ze stanem sprzed testu. Zapisany wynik
  zmian jest w osobnym `archive/visual-regression/s2.2l-mouse/post-change-scene.xodr`.
  Oryginalna biblioteka, OpenDRIVE/XOSC, PBR i krawężniki niezmienione.

**Otwarty zakres:** to usprawnienie sposobu używania **już obsługiwanych**
typów zasobów; nie implementuje pełnego przesuwania, obracania, skalowania
ani osobnej edycji wszystkich prefabrykatów po upuszczeniu. Osobne obniżone
krawężniki, wysepki oraz pełny realizm materiałów nadal pozostają
niezrealizowane. W kolejnym etapie rozszerzyć testy fizycznego przeciągania
o sygnały, drzewa, przejścia, materiały i błędne obszary (z zachowaniem
przyczepiania OpenDRIVE do drogi oraz Undo/Redo); dopiero potem
projektować nowe prefabrykaty. Nie oznaczać tych dalszych rzeczy jako PASS.

## S2.2k — gotowe elementy drogowe dostępne myszą (13.11.2026)

**Wykonane (kod i testy; nie deklaracja końcowego fotorealizmu):**

- 12 zasobów w `assets/library/manifest.json`, wbudowanych przez
  `editor/resources/resources.qrc`, wszystkie z odrębnymi miniaturami
  `assets/library/thumbnails/{road,style}_prawko_*.png`:
  **6 szablonów do narysowania od zera**
  (`Prawko | Gotowe drogi`) oraz **6 nakładanych na istniejącą drogę**
  (`Prawko | Krawężniki i chodniki`).
  Warianty: `urban` (chodniki 2×1,8 m), `wide` (2×2,5 m),
  `promenade` (2×3,0 m), `one_side` (1×2,5 m po lewej),
  `parking` (po obu stronach: pas postojowy 2,4 m + chodnik 2,0 m),
  `no_curb` (bez chodników/krawężników). Typowa jezdnia lokalna
  zachowuje dwa pasy jazdy szerokości 3,0 m.
- Po przeciągnięciu **Road Template** edytor uzbraja narzędzie Create Road
  i pozwala rysować punkty myszą; po przeciągnięciu **Road Style**
  na istniejącą drogę wykonuje `edit::apply_road_style`
  przez `Document::push_command`, z `Ctrl+Z` Undo.
  Krawężniki nie są przyklejoną atrapą 3D: istniejący generator
  `MeshOptions.raised_sidewalks=true` buduje ścianę 15 cm oraz fazę
  z rzeczywistej granicy pasa Sidewalk. Wariant bez chodników nie ma
  ścian. `curb_render_lod` pozostaje włączony. Środowisko i domyślne
  globalne parametry geometrii/format XODR pozostają niezmienione.
- Normalne uruchomienie `main.cpp` wywołuje
  `Settings::enable_studio_materials_once()` (jednorazowa migracja
  z Sober do Textured, zapamiętany wcześniejszy wybór, bez zmiany
  plików scen i bez wpisów w read-only screenshotach), a po pokazaniu
  `MainWindow` wywołuje `open_studio_library()`.
  `LibraryPanel::focus_category` teraz **naprawdę ustawia filtr
  kategorii**, nie tylko przewija listę.
  `C:\Users\tomas\Desktop\PrawkoRoadStudio.lnk` już wskazuje na
  `F:\PrawkoRoadStudio\build\dev-windows\editor\roadmaker-editor.exe`
  i dotychczasowy projekt `agent/projects/prawko_urban_curve/scene.xodr`.
  Nie wymaga od użytkownika kopiowania assetów, instalacji ani
  ręcznego zapisywania.
- **Testy**: `s2.2k-library-build.log`: `BUILD_PASS: all`,
  `AGENT_BINDING_SYNC_PASS`; `s2.2k-quality-gate.log`:
  `QUALITY_GATE_PASS` (C++/agent); `s2.2k-qt-full-final.log`:
  **1292 PASS, 1 SKIPPED, 0 FAIL**. Nowe testy 6 profilów
  obejmują prawdziwe indeksy siatki krawężników, walidację XODR,
  style do zastosowania na drogach, Undo, 12 miniaturek qresource;
  testy migracji ustawień i automatycznej kategorii.
  Pierwszy pełny Qt ujawnił sztywne liczniki 59 elementów i przestarzałe
  zachowanie `focus_category`; zaktualizowano testy oczekujące
  teraz 71 elementów i prawdziwy filtr, a następnie wszystkie PASS.
  `scripts/check_asset_licenses.py`: **PASS** po dodaniu 12 wpisów
  dla miniatur original-work Apache-2.0.
  `archive/visual-regression/s2.2k-library-gui.png` potwierdza działanie
  GUI i teksturowaną scenę; to screenshot trybu `--screenshot-ui`
  z `--raise-dock`, więc z definicji pokazuje All categories;
  osobny test `MainWindowLayoutTest.StudioLibraryOpensTheNewRoadTemplatesWithoutUserSetup`
  potwierdza wybór nowej kategorii przy rzeczywistym normalnym starcie.
- **Granice zrealizowanego zakresu:** wysokość krawężnika jest
  standardem generatora 0,15 m — NIE ma osobnych prefabrykatów
  krawężnika najazdowego/obniżonego niezależnego od przejścia,
  wysepki jako osobnego zasobu ani narzędzia osobnego stawiania
  luźnych segmentów. Poprawki nie stanowią dowodu pełnego
  fotorealizmu. Nie nadpisano ani nie zapisywano scen użytkownika.

**Potwierdzenie normalnego uruchomienia przez skrót — PASS:**
po zakończeniu `SESSION_FINISH_PASS` uruchomiono z pulpitu
`C:\Users\tomas\Desktop\PrawkoRoadStudio.lnk`.
Rzeczywisty proces `roadmaker-editor.exe` PID **28400**, responsive,
z otwartym `agent/projects/prawko_urban_curve/scene.xodr`;
screen **`archive/visual-regression/s2.2k-real-launch-desktop.png`**:
kategoria **`Prawko | Gotowe drogi`** aktywna, wszystkie sześć
przekrojów z miniaturkami widoczne; trawa i droga renderowane
w trybie Textured. Brak gwiazdki niezapisanego dokumentu;
SHA256 sceny **879fe41c58cc83bc7393d456681c329b5d84574b0eabda0d73b2f600b13c540e**
zgodny ze SHA odczytanym podczas weryfikacji sprzed uruchomienia.
**Proces pozostawiony otwarty do używania; nie zamykać go
ani nie nadpisywać niezapisanych ewentualnych późniejszych zmian.**

**Historyczny plan po S2.2k (nieaktualny punkt startu — aktualny krok S2.2n jest na początku HANDOFF):** zaprojektować i wdrożyć
osobne parametryczne prefabrykaty obniżonego krawężnika,
wysepki i strefy przejścia bez nakładania atrap na siatkę
(osobne testy XODR, geometrii i przecięć); następnie wrócić
do nieodebranego światła i realistycznych materiałów przy
dużych scenach. Nie zaczynać nowego projektu.

## Gdzie pracować

- Komputer przez Desktop Commander: `DESKTOP-G2FV0GJ`.
- **Aktywny katalog:** `F:\PrawkoRoadStudio` (C++20, Qt 6).
- **Agent AI — osobne repo Git:** `F:\PrawkoRoadStudio\agent`.
- Repo publiczne edytora: https://github.com/prawkonaraz100/PrawkoRoadStudio
- Repo publiczne agenta: https://github.com/prawkonaraz100/PrawkoRoadStudio-Agent
- Obie gałęzie edytora (`main`, `feature/realistic-road-surfaces`) wskazywały ten sam kod przed utworzeniem tego przekazania.
- Kopia oryginału `reference/RoadMaker/` jest **tylko referencyjna** — nie edytuj, nie wysyłaj zmian do upstream.
- `toolchain/`, `.qt/`, `build/`, `agent/projects/` i `archive/` pozostają na dysku; część jest ignorowana przez Git i nie występuje w publicznych repozytoriach.

## Historyczny punkt kontrolny po przeniesieniu projektu

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

## Historyczny plan rozwoju geometrii i wyglądu

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

**Odblokowano testy Qt Windows (13.11.2026).** Zrzut błędu
`RoadMakerEditorTests` dostarczony przez użytkownika ujawnił
komunikat: `No Qt platform plugin could be initialized`;
dostępna była wyłącznie platforma `windows`, podczas gdy
`qt_gtest_main.cpp` wymaga `offscreen`. W lokalnym Qt
`F:\PrawkoRoadStudio\.qt\6.8.3\msvc2022_64\plugins\platforms\qoffscreen.dll`
istniał, ale `windeployqt` kopiuje obok edytora tylko
`qwindows.dll`. To wyjaśniało zawieszenie w modalnym oknie
błędu podczas `gtest_discover_tests`.

Naprawa w `editor/tests/CMakeLists.txt`: na Windows po
zlinkowaniu `roadmaker_editor_tests` CMake kopiuje
`$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>` do
`$<TARGET_FILE_DIR:roadmaker_editor_tests>/platforms`
**przed automatycznym wykrywaniem GTest**. Wymagany
plugin jest sprawdzany jako target Qt (brak -> błąd konfiguracji).
Zweryfikowano bez specjalnych `QT_PLUGIN_PATH`
ani `QT_QPA_PLATFORM_PLUGIN_PATH`: testy
`DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity`
oraz `MeshParity.*` — **5/5 PASS**. Ręcznie skopiowany
plugin kontrolny usunięto przed przebudową; automatyczna
kompilacja `roadmaker_editor_tests` zakończyła się
`EDITOR_TEST_DISCOVERY_BUILD_PASS` i sama odtworzyła
`qoffscreen.dll`. Pełna binarka edytora:
**1285 przypadków, 1284 PASS, 1 SKIPPED, 0 FAIL**, exit 0.
Log lokalny:
`archive/visual-regression/s2.2b-qt-full-tests.log`.
Dawnych prób diagnostycznych nie utrzymywano w kodzie.
Osobny Quality Gate jądra: 2059 testów C++ i 6 agenta PASS.
**Nie mylić testów Qt z odbiorem estetyki renderingu.**

**S2.2c — etap kodu i testów (13.11.2026):** odzyskano istniejące lokalne zmiany bez ich nadpisania i dokończono kontrolę automatyczną. `junction_surface.cpp` wytwarza odsuniętą krawędź górnego chodnika i oddzielną rzeczywistą fazę krawężnika szerokości nominalnej 5 cm, zmniejszaną na końcach wlotów i przy zjazdach. Dla ciasnych trójkątów CDT przesunięcie jest ograniczane testem orientacji, aby nie odwrócić siatki. Górna krawędź fazy korzysta z pozycji siatki chodnika, dolna z krawędzi ściany. Dodane testy T/Y, nieodwracania trójkątów, wspólnych krawędzi i eksportu GLB. `gltf_exporter.cpp` zapisuje `TEXCOORD_0` dla dowolnej podsiatki z kompletnymi UV — poprzednio exporter je pomijał, mimo że miały je fazy.

**Potwierdzone 13.11:** `BUILD_PASS: tests`, `BUILD_PASS: editor`; C++ **2068 PASS, 10 DISABLED**, Python agent **6 PASS**, `QUALITY_GATE_PASS`; Qt edytora **1284 PASS, 1 SKIPPED, 0 FAIL** (1285 przypadków, exit 0). W czasie pracy żadne okno `roadmaker-editor` nie było otwarte, nie zmieniano zapisanych scen użytkownika. **BRAK odbioru wizualnego S2.2c z kamery kierowcy i zbliżeń T/Y, brak potwierdzonego oglądu GLB w zewnętrznym viewerze i profilowania czasu remesh** — nie deklarować pełnego ukończenia etapu wizualnego. Najbliższa praca: osobna kopia sceny testowej, inspekcja krawężników w GUI pod małym kątem (ciasne T/Y, przejścia, nachylenia), sprawdzenie glTF w viewerze i kosztu remesh_objects; dopiero po tym ocena geometrii jako gotowej do PBR.

**S2.2c — niezależny ogląd GLB i naprawa eksportu agenta (13.11.2026):**
Wykryto rozjazd skompilowanego `build/dev-windows/python/_roadmaker*.pyd`
i ignorowanego przez Git `agent/vendor/roadmaker/_roadmaker*.pyd` (inne SHA-256).
Stary agent eksportował płaskie narożniki bez faz: GLB 127232 B, 41 meshy;
po synchronizacji z aktualnym modułem ten sam XODR eksportuje GLB
157824 B, 49 meshy, 4 ściany i 4 fazy narożników z UV oraz 0–0,15 m
wysokości. To był błąd ścieżki CLI, nie generatora C++.
`scripts/prawko-build-win.ps1` dla targetów editor/python/all teraz
kopiuje moduł do agenta i kontroluje SHA-256; `prawko-verify-win.ps1`
blokuje PASS przy stale binding; `agent/roadctl.py` wymaga
`raised_sidewalks` i nie może już po cichu użyć starej geometrii.
Agent ma nowy test end-to-end: prawdziwy GLB, 4 fazy, atrybuty
POSITION/NORMAL/TEXCOORD_0, dostępny VEC2 accessor. Repo agenta:
`02cbdeb5e943f7a710d3292a9fbc5e85d3cc95fc` na `origin/main`.

**Rzeczywista ocena 3D:** Blender 5.3 bezbłędnie zaimportował GLB
skrzyżowania X, nowo utworzonego miejskiego T i niestandardowego Y
(z zapisanych XODR z `agent/projects/visual_s2_2c_{t,y}`).
X: 49 meshy, 4 fazy; T: 14 meshy, 3 fazy; Y: 13 meshy,
3 fazy. Na diagnostycznych renderach z góry, z poziomu kierowcy
i na wyizolowanym zbliżeniu widać ciągłą wzniesioną bryłę,
oddzielną ścianę oraz fizyczne połączenie fazy z chodnikiem.
Kolor pomarańczowy faz na renderach jest **wyłącznie diagnostyczny**,
nie jest nowym materiałem produkcyjnym.
Rzeczywista aplikacja Qt otworzyła oddzielną scenę testową X bez awarii:
`archive/visual-regression/s2.2c-current-editor-gui.png`; proces
zamknięto bez zapisywania zmian. Lokalne dowody w
`archive/visual-regression/`: `s2.2c-independent-review-current.glb`,
`s2.2c-t-urban-current.glb`, `s2.2c-y-urban-current.glb`,
`s2.2c-glb-geometry-top.png`, `s2.2c-glb-corner-isolated.png`,
`s2.2c-glb-corner-driver.png`, `s2.2c-y-glb-oblique.png`.
Pliki diagnostyczne są ignorowane przez Git i **nie są** zdalnym backupem.

**Granice odbioru:** zakładany profil 15 cm / 5 cm jest widoczny,
a w pokazanych ujęciach nie widać przerw w fazie. Łuk pozostaje
widocznie segmentowy; nie ma jeszcze PBR, realnych tekstur ani
kompletnego odbioru estetyki z kamery kierowcy w samym edytorze.
Nie wykonano pomiaru czasu `remesh_objects`. **Odbiór realizmu
materiałowego i finalny odbiór wizualny produktu NIE SĄ PASS.**

**Testy końcowe:** C++ 2068 PASS (10 DISABLED), Qt 1284 PASS,
1 SKIPPED / 0 FAIL; agent 7 PASS wraz z nową regresją GLB.
`AGENT_BINDING_PARITY_PASS`, `BUILD_PASS: python`, kompilacja
edytora i pełny Quality Gate (aktualny log lokalny w
`archive/visual-regression/s2.2c-final-quality-gate.log`).

**S2.2d — korekta wizualna normalnych i betonu (13.11.2026):**
Analiza scen X/T/Y w Blenderze potwierdziła, że na fazie krawężnika
każdy trójkąt miał własną płaską normalną. W `junction_surface.cpp`
dodano deterministyczne, ważone polem uśrednianie normalnych we
wspólnych punktach **wyłącznie tej samej fazy**. Jej górna granica
z chodnikiem i dolna granica z pionową ścianą pozostają ostre, a
współrzędne wierzchołków, trójkąty i format OpenDRIVE są niezmienione.
Na podstawie surowych danych GLB X: 136 niezgodności normalnych >0,05°
na zduplikowanych wierzchołkach -> 0; maks. rozbieżność 6,7358°
-> 0,0165° (float32), 72 trójkąty faz przed/po. T: 210 wspólnych
wierzchołków fazy i 0 niezgodności; Y: 138 i 0 niezgodności.

Żółtawo-zielony odcień betonu krawężnika (`0.55/0.55/0.50`)
zastąpiono neutralnym `0.61/0.61/0.59` w GUI oraz w eksporcie.
Dla typów `Curb/Sidewalk/Border` ustawiono wspólną chropowatość
`0.78` (pozostałe pasy nadal `0.95`) w glTF, USD i podglądzie
manifestu; test kontroluje zgodność. **To nie są materiały PBR
z teksturami!** GLB nadal ma dla pasa krawężnika wyłącznie
`baseColorFactor`, `roughnessFactor` i UV; brak obrazów albedo,
normal i roughness w jego materiale. Istniejące katalogowe tekstury
edytora nie są jeszcze osadzane w GLB.

Próba globalnego zagęszczenia łuków (sagitta 8->4 mm,
`kBoundarySimplify` 5->1 mm, próg tangencji 8->4 cm)
**została odrzucona i w całości wycofana**: pełne testy
wykryły 6 i 4 wąskie trójkąty przy dopuszczalnych 2
na wlotach T oraz niezgodność z goldenem prostych skrzyżowań.
Przywrócono dotychczasową geometrię i wszystkie te testy znów PASS.
Nie podnosić gęstości globalnej ani nie aktualizować złotego wzorca
bez odrębnie zweryfikowanego rozwiązania. Segmentowana **sylwetka**
łuku nie jest usunięta (zmiana normalnych koryguje oświetlenie,
nie obrys).

**Niezależny GLB/Blender 5.3:** aktualne `s2.2d-x.glb`,
`s2.2d-t.glb`, `s2.2d-y.glb` wyeksportowane przez agenta,
zaimportowane do Blender 5.3; X: 4 fazy i 4 ściany,
T/Y: po 3 fazy i 3 ściany, UV dostępne, wszystkie trójkąty
niezerowe, wysokie części 0,15 m, roughness 0,78. Render X
przed/po z tym samym ustawieniem kamery oraz widoki T/Y:
`archive/visual-regression/s2.2d-before-x-*.png`,
`s2.2d-after-x-*.png`, `s2.2d-after-t-*.png`,
`s2.2d-after-y-*.png`. Dodatkowo włączono realne GUI
Windows na osobnej scenie testowej X, zapisano zrzut
`s2.2d-current-editor-gui.png` i zamknięto okno bez zapisu.
Pliki obrazów/GLB w `archive` są lokalne i ignorowane przez Git.
Zgoda użytkownika na udoskonalanie zapisanych scen jest aktualna,
ale w tym etapie nie trzeba było modyfikować żadnego XODR: wygląd
zmienia się globalnie po przebudowie edytora.

**Wyniki:** `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`,
`AGENT_BINDING_PARITY_PASS`, C++ 2068 PASS / 10 DISABLED,
Qt 1284 PASS / 1 SKIPPED / 0 FAIL, 7 testów agenta PASS;
`QUALITY_GATE_PASS`. Logi: `s2.2d-final-build.log`,
`s2.2d-final-gate.log`, `s2.2d-final-qt.log` w lokalnym
`archive/visual-regression/`. Nie wykonano pomiaru kosztu
`remesh_objects` ani zatwierdzenia realizmu z kamery kierowcy
w samym edytorze. **PBR i finalny realizm — OTWARTE.**

**S2.2e — remesh i rzeczywiste tekstury w GLB (13.11.2026):**

**Wydajność geometrii:** dodano trwały, replikowalny test w
`core/tests/test_prawko_remesh_budget.cpp`. Buduje X (4 ramiona, 4
przejścia), T (3, 3) i Y (3, 3) z `raised_sidewalks=true`,
porównuje `remesh_objects` do pełnej siatki (pozycje, indeksy pasów,
narożników) i mierzy 41 iteracji. Pierwszy pomiar na tym PC:
X p50=9,026 ms, p95=9,598 ms;
T p50=4,941 ms, p95=5,417 ms;
Y p50=5,410 ms, p95=5,959 ms. W końcowym Quality Gate po PBR:
X 9,180/9,976 ms, T 5,329/5,622 ms, Y 5,899/6,159 ms.
Pomiar ma typową zmienność zależną od obciążenia komputera.
Wszystkie trzy testy PASS. **To tylko wywołanie C++** bez uploadu GPU
i prezentacji klatki. Nie nazywać tego FPS ani SLA 16 ms GUI.
Pierwszy osobny commit: `f33cc89`.

**GLB/PBR:** `core/src/io/gltf_exporter.cpp` zapisuje teraz
osadzone (w BIN) obrazy CC0 Poly Haven JPEG albedo, PNG normal OpenGL
oraz JPEG roughness dla `Driving` (asphalt) i
`Sidewalk/Border/Curb` (brushed concrete); źródła już były w
`assets/textures/`, z wpisami w `ASSETS_LICENSES.md`. Deduplicacja
trzech obrazów na rodzinę materiału, REPEAT sampler, metrystyczne UV
`0.25 powtórzenia/metr` = płytka 4 m. Dla pasów drogowych używa
`road.uvs`, dla submeshy zachowuje UV, a dla CDT bez UV stosuje
planarną projekcję XY. Normal i roughness texture są podpięte
do materiału, `metallicFactor=0`. JPEG roughness ma identyczne
kanały RGB (1024 próbki na każdą z obu map bez różnic), więc jego G
jest prawidłowy. `export_preview.cpp` raportuje liczbę obrazów
i textured flags zgodne z faktycznym GLB; testy `ExportPreview` i
`GltfExport` obejmują osadzone mapy, UV, współistnienie z sygnalizacją
i strukturalny reload mimo braku dekodera obrazów w binarce testowej.
**Instalowany edytor poza tym drzewem:** źródłowa ścieżka zasobów jest
zdefiniowana przez CMake; gdy plików brakuje, GLB może użyć bazowych
kolorów. Dostarczanie tekstur z pakietem dystrybucyjnym pozostaje
odrębnym zadaniem; wyeksportowany GLB jest samowystarczalny.

**Blender 5.3 / X/T/Y:** `archive/visual-regression/s2.2e-pbr-x.glb`
(1 380 340 B), `s2.2e-pbr-t.glb` (1 314 116 B),
`s2.2e-pbr-y.glb` (1 289 912 B); wszystkie trzy ponownie
zaimportowane i faktycznie zdekodowane: X 49 meshy, po 4 bevel/wall,
T 14 meshy, Y 13 meshy, po 3 bevel/wall, poprawne UV,
niezdegenerowane powierzchnie, 3 image texture nodes w materiałach
`lane_0`, `lane_4`, `lane_9`. Wynik
`PBR_GLTF_IMPORT_PASS` X/T/Y. EEVEE render produkcyjnego GLB:
`archive/visual-regression/s2.2e-pbr-x-eevee.png`. To pliki
**lokalne ignorowane przez Git**; nie traktować jako backupu poza F:.
Na renderze wyraźnie widać powtarzalny wzór spękań asfaltu i
ciemne, monotonne bloki betonu — **realizm NIE odebrany**.
Utrzymujemy bazową kompatybilność OpenDRIVE i siatkę narożników.

**Łuki / analiza i decyzja:** X ma 18 trójkątów na fazę
narożnika (~9 odcinków), więc widoczna jest segmentacja zarysu.
W S2.2d zagęszczanie globalne naruszało dozwolony budżet
`sliver <=2` na ciasnych T (osiągało 4–6) i złoty wzorzec
prostopadłych wlotów. **W S2.2e nie zmieniono obrysów** ani
współrzędnych nawierzchni. Rozwiązanie wymaga odseparowanego
LOD tylko renderowania albo lokalnej retriangulacji naroża
z nowymi testami i nienaruszonym goldenem; w tym etapie
nie ma potwierdzonej bezpiecznej poprawki sylwetki.

**Weryfikacja:** `BUILD_PASS: all`,
`AGENT_BINDING_SYNC_PASS`, pełny Quality Gate
`2071/2071 C++ PASS` (10 testów DISABLED), agent
`7/7 PASS`, edytor Qt `1284 PASS + 1 SKIPPED`,
`QUALITY_GATE_PASS`. Logi:
`archive/visual-regression/s2.2e-final-gate.log`,
`s2.2e-pbr-qt.log`, `s2.2e-pbr-build-all.log`,
`s2.2e-blender-x.log`, `s2.2e-blender-t.log`,
`s2.2e-blender-y.log`.
**Otwarte:** zagęszczenie sylwetki bez psucia CDT,
ograniczenie tilingu/odcienia, realne światło oraz pełna ocena
z kamery kierowcy w GUI (nie tylko Blender). Pomiar i odbiór
GPU upload+FPS również nadal otwarte. **Potwierdzenie aplikacji Qt:**
aktualny `roadmaker-editor.exe` otworzył odrębną scenę X bez awarii,
zrzut: `archive/visual-regression/s2.2e-current-editor-gui.png`.
To był widok ogólny; nadal nie jest odbiorem z wysokości kierowcy.
Okno testowe zamknięte bez zapisu. **GUI nie korzysta jeszcze ze
wszystkich map PBR tak samo jak eksport GLB**, czego dowodzą
różnice w renderze Blendera i neutralnym obrazie GUI.

**S2.2f — lokalny LOD faz krawężników bez naruszenia CDT (13.11.2026):**

- W `core/src/mesh/junction_curb_refinement.hpp` wprowadzono **opcjonalny**
  podział trójkątów na wspólnej zakrzywionej krawędzi jezdnia/chodnik.
  Działa PO pierwotnej triangulacji CDT i klasyfikacji pasów. Tę samą
  nową pozycję wierzchołka dzielą OBA sąsiednie trójkąty, a ściana fazy
  i górna płyta podążają za granicą, więc nie ma T-seam w nowym detalu.
  Korekta Catmull-Rom jest tylko prostopadła do cięciwy, ograniczona
  do 8 mm / 1% cięciwy. Dzielone są tylko odcinki 0,45–5,0 m
  z dwoma dobrze uwarunkowanymi sąsiadami (>=8 stopni, >=22% pola).
  Klasyfikacja, oryginalne wejściowe obrysy, historyczne goldeny
  oraz zapis ASAM OpenDRIVE są bez zmian.
- Nowa opcja `MeshOptions.curb_render_lod` domyślnie = **false**
  w jądrze; działa wyłącznie razem z `raised_sidewalks=true`.
  Chroni zgodność oryginalnych siatek zwykłych i wyniesionych.
  `studio_mesh_options()` Qt włącza ją do rzeczywistego GUI, także
  dla `remesh_junctions` i `remesh_objects`; Python agent/GLB również
  włącza ją jawnie, a regresja agenta porównuje liczbę trójkątów
  GLB względem domyślnej wersji bez LOD.
- Test `JunctionSidewalkBands.VisualCurbArcLODAddsSharedVerticesWithoutSlivers`
  porównuje X/T/Y: trójkąty jezdnia+chodniki 676->828,
  469->555, 522->662; slivery <5 stopni odpowiednio 16->16,
  5->5, 18->18 (liczone na całych tych powierzchniach,
  nie mylić z osobnym budżetem CDT T-junction!). Dodatkowo
  test sprawdza deterministyczne bufory, orientację/niedegenerację,
  exact-match dolnych krawędzi ścian z wierzchołkami jezdni.
- Rzeczywisty agent GLB X/T/Y: triangulacja **faz** X 72->120,
  T 108->174, Y 72->116; ściany również X 72->120,
  T 108->174, Y 72->116. Podział jest zapisany w GLB.
  Blender 5.3 ponownie importował/działające PBR X/T/Y: PASS.
  Wyeksportowane pliki: `archive/visual-regression/s2.2f-opt-{x,t,y}.glb`.
  Logi: `s2.2f-opt-blender-{x,t,y}.log`, render EEVEE
  `s2.2f-opt-x-eevee.png`. Geometria before/after w tych samych
  kamerach: `s2.2f-before-x-{top,driver,isolated}.png`,
  `s2.2f-after-x-{top,driver,isolated}.png`,
  `s2.2f-after-{t,y}-{top,driver}.png`. To lokalne, ignorowane
  przez Git dowody; nie są kopią poza dyskiem F:.
- Test wydajności `PrawkoRemeshBudget` uruchamia teraz **optyczną
  wersję LOD**, a nie tylko stary kształt: podczas focused Qt+core
  X mediana/p95 ~10,37/10,90 ms, T 6,21/7,17 ms,
  Y 7,00/7,70 ms (zmienne wraz z obciążeniem PC).
  Nie są to wyniki uploadu GPU ani FPS.
- `BUILD_PASS: all`, `AGENT_BINDING_SYNC_PASS`,
  `AGENT_BINDING_PARITY_PASS`, pełny `QUALITY_GATE_PASS`:
  C++ **2072/2072 PASS** (10 DISABLED), Python **7/7 PASS**;
  testy Qt Windows **1284 PASS, 1 SKIPPED, 0 FAIL**.
  W Qt `DocumentCommands.NearJunctionCrosswalkDeleteAndUndoKeepRaisedMeshParity`
  nadal rygorystycznie porównuje bufory pełnego i inkrementalnego
  budowania, ale od S2.2f używa tej samej opcji LOD co GUI.
  Logi: `s2.2f-opt-final-gate.log`, `s2.2f-opt-qt-final.log`,
  `s2.2f-isolated-build-all.log`, `s2.2f-opt-qt-fix-build.log`.
  Realny edytor Windows otworzył testową kopię X (bez awarii),
  screenshot: `s2.2f-current-editor-gui.png`; okno zamknięto
  bez zapisywania scen. Żadna scena użytkownika nie została zmieniona.
- **Ocena estetyczna:** z bliskiej kamery geometria jest lepiej
  podzielona, lecz w statycznych renderach różnica sylwetki jest
  subtelna (przesunięcie geometrii <=8 mm). Nie deklarować
  docelowej płynności, fotorealizmu, odbioru z kamery kierowcy
  w SAMYM edytorze ani GPU/FPS. Trzeba sprawdzić większe skosy,
  z-fighting, powtarzanie tekstur, światło i detale.
- Osobne repo agenta po zmianie: `35c48c11cdb7efca8e08d1be731463657bbe7a32`
  (origin/main). Należy zawsze sprawdzić aktualny HEAD obu repo.

**S2.2g — pomiar karty GPU i pięć widoków X/T/Y (13.11.2026):**

- Dodano **oddzielny, ręcznie uruchamiany** target
  `prawko_gpu_probe` (`editor/tests/prawko_gpu_probe.cpp`;
  `EXCLUDE_FROM_ALL` w `editor/tests/CMakeLists.txt`). Korzysta z tej
  samej implementacji `GLRenderer`, shaderów, materiałów i budowania
  siatki co PrawkoRoadStudio; sprzętowy OpenGL 3.3 przez Qt offscreen
  na **NVIDIA GeForce GTX 1050 Ti (4096 MiB, sterownik 582.78)**.
  Próba jest pomiarem sprzętowym, nie Mesa/llvmpipe.
- Metodologia: ten sam zapisany X/T/Y, `raised_sidewalks=true`,
  LOD wyłączony/włączony; 1280×720, 5 kamer: z góry, ukośnie,
  kamera kierowcy (około **1,55 m**), z przeciwnej strony (1,55 m)
  i zbliżenie (około **0,60 m**). Dla T/Y kamera poziomu drogi
  celuje w centroid rzeczywiście zbudowanego najdłuższego
  krawężnika, zamiast w narożnik X. Dla każdego punktu 15 warm-up,
  65 pomiarów czasu `GL_TIME_ELAPSED` przez natywny GPU timer query.
  Mierzone są osobno CPU submission i oczekiwanie na wykonanie GPU;
  `QPainter`, Qt `QOpenGLWidget`, prezentacja klatki, synchronizacja
  monitora i kompozytor Windows NIE są objęte testem.
- W **finalnej, skorygowanej serii S2.2g-v2** mediana GPU draw:
  X góra **0,383 ms**, ukos **0,423 ms**, kierowca **0,292 ms**;
  T **0,348 / 0,373 / 0,258 ms**;
  Y **0,340 / 0,372 / 0,278 ms**. CPU draw submission
  ok. **0,008–0,025 ms**; CPU do odczytu zakończenia GPU
  ok. **0,4–0,65 ms**. Włączenie LOD zmienia mediany GPU
  o około 0–0,002 ms (zależne od kamery); bez widocznej
  regresji renderingowej w tych scenach.
- Oddzielono koszt wczytania PBR tekstur od siatek. Powtarzane
  synchronizowane `GLRenderer::upload` całej siatki+GPU `glFinish`
  (10 rozgrzanych próbek): mediana X **2,56 ms** (bez LOD 2,74),
  T **1,57 ms** (bez 1,54), Y **1,36 ms** (bez 1,51).
  Obejmuje CPU driver, tworzenie VAO/VBO/EBO i GPU fence, a NIE
  sam transfer DMA. Początkowe dekodowanie+upload sześciu
  obrazów PBR ok. **42–56 ms**, zależne od cache; wartości
  baseline/LOD nieporównywalne na zimno. Rozmiary buforów
  geometrii X 0,281→0,290 MiB, T 0,267→0,278,
  Y 0,230→0,237. Odpowiednio 1620→1764, 1423→1621
  i 1047→1179 trójkątów całego renderowanego obszaru
  (to inne liczniki niż test samej jezdni/chodników S2.2f).
- Obiektywne porównanie 15 par kadrów przy progu max RGB
  >=8/255: zmiana X góra **0,0125%**, kierowca **0,0248%**,
  macro **0,1518%**; T **0,0284 / 0,1341 / 0,0267%**;
  Y **0,0160 / 0,0627 / 0,1214%**. Na mapach różnic
  zmiany ograniczają się do linii łuków. Odbiór estetyczny:
  brak widocznych otwartych dziur w tych kadrach, ale
  **przyrost płynności bardzo subtelny**; miejscami wyraźny
  ciemny/czarny pas ściany krawężnika, nadal segmentowane
  cieniowanie i gwałtowne przejście teksturowanej jezdni
  do jednolitej posadzki junction. Nie ogłaszać
  braku z-fightingu w animacji ani realizmu.
- Źródła pomiarów (lokalne, ignorowane przez Git):
  `archive/visual-regression/s2.2g-v2/{x,t,y}-gpu-benchmark.json`,
  `{x,t,y}-{base,lod}-{top,oblique,driver,opposite,macro}.png`,
  `{x,t,y}-diff-*.png` + `s2.2g-v2-pixel-diff.ps1`.
  Nie używać wcześniejszej próbnej serii `s2.2g/`
  jako finalnej: T/Y kamery nie zawsze pokazywały krawężnik;
  **v2** ma dynamiczne celowanie w rzeczywisty narożnik.
- `BUILD_PASS: all`, `QUALITY_GATE_PASS`:
  **2072/2072 C++ PASS** (10 DISABLED), **7/7 agent PASS**;
  testy Qt **1284 PASS, 1 SKIPPED**. Logi `s2.2g-build-all.log`,
  `s2.2g-final-gate.log`, `s2.2g-qt.log`.
  Testowe okno prawdziwego edytora otwarto ze sceną X
  i bezpiecznie zamknięto bez zapisu, obraz:
  `s2.2g-current-editor-gui.png`. Nie modyfikowano
  scen użytkownika ani geometrii, agent repo bez zmian.

**S2.2h — widoczna aplikacja Qt, spójne UV i oświetlenie betonu (13.11.2026):**

- Dodano opcjonalny `prawko_qt_fps_probe` (`editor/tests/prawko_qt_fps_probe.cpp`, `EXCLUDE_FROM_ALL`). Tworzy prawdziwe widoczne `MainWindow` z teksturami, HUD, rzeczywistym `QOpenGLWidget` i `frameSwapped` (bez zapisu scen, `Settings::set_read_only(true)`). Symuluje `Alt+LMB` orbit przez normalne zdarzenia myszy Qt. 1600x900, GTX 1050 Ti, zegar Qt 16 ms, po 30 rozgrzewkowych i 180 mierzonych odstępów między `frameSwapped`; Windows Qt, nie `QT_QPA_PLATFORM=offscreen`.
- **Powtórka już po przebudowie materiałów** `archive/visual-regression/s2.2h-final-fps-{x,t,y}.json`: spoczynek X/T/Y odpowiednio **60,00 / 59,99 / 60,00 FPS**; obrót **59,97 / 60,01 / 60,00 FPS**. P95 obrotu odpowiednio **17,35 / 17,30 / 17,29 ms**, P99 **17,52 / 17,54 / 17,61 ms**, **0 próbek >33,33 ms** na scenę. To pomiar częstotliwości sygnału `frameSwapped` (Windows+Qt) przy sztucznym odświeżaniu co 16 ms; nie jest niezależnym pomiarem maksymalnej przepustowości GPU ani fizycznego momentu pojawienia się fotonu na ekranie. `viewport->format().samples()` zgłasza **0**, choć aplikacja prosi o 4x MSAA — nadal do weryfikacji. **Jank przy edycji węzłów i przebudowie siatki NIE sprawdzony.**
- **Naprawa ciemnego, jednolitego asfaltu w skrzyżowaniach:** `editor/src/render/scene_builder.cpp` generuje metryczne world-XY UV z dokładnie tych samych wierzchołków co eksporter `gltf_exporter.cpp` dla CDT floors i ich betonowych narożników. Szanuje jawne UV; materializowany asfalt/beton w `SurfaceMesh` ma UV, natomiast trawa pozostaje bez UV. Geometria, topologia, OpenDRIVE i model C++ niezmienione. Dodano testy Qt `BuildScene.JunctionAndCornerPavementUseWorldPlanarTextureCoordinates` oraz `BuildScene.MaterializedGroundSurfaceGetsPlanarUvButGrassDoesNot`.
- **Doświetlenie krawężników:** `Material.vertical_fill` domyślnie **0.0**, `ViewportWidget::material_for` przypisuje **0.38** tylko `SurfaceKind::Concrete` w trybie Textured, a GLSL dodaje rozproszone niebieskoszare odbicie nieba wyłącznie przy niemal pionowej normalnej geometrycznej. Bez globalnego podbijania asfaltu, trawy, malowania czy trybu Sober. Przy pionowych ścianach world-XY UV bywają singularne — ochrona wyznacznika ramki normal mapy w `GLRenderer` zachowuje wtedy normalną geometryczną (bez NaN i smug). Zaktualizowano ręczny `prawko_gpu_probe` do nowego parametru `vertical_fill`.
- **Kontrola wizualna:** 30 nowo wygenerowanych kadrów GLRenderer X/T/Y `archive/visual-regression/s2.2h-material-after/` (5 kamer × base/LOD na scenę) i rzeczywisty screenshot uruchomionego `roadmaker-editor.exe --screenshot --textured`: `s2.2h-gui-x-orbit.png`. Wszystkie X/T/Y pomiary wykonane z RC 0. Widać fakturę asfaltu i spękania przez całą powierzchnię skrzyżowania zamiast jednolitej szarej łaty, oraz bardziej czytelną, nie całkiem czarną ścianę krawężnika. Nadal widoczne **powtarzanie spękań co 4 m**, ostre przejścia niektórych fragmentów betonu i brak szczegółów/światła cieniowania — nie oznaczać pełnego fotorealizmu jako ukończonego. Nie zmieniano żadnej sceny użytkownika.
- **Testy:** `BUILD_PASS: all` + `AGENT_BINDING_SYNC_PASS`; `QUALITY_GATE_PASS` — C++ **2072/2072 PASS**, agent **7/7 PASS**; finalny Qt **1286 PASS, 1 SKIPPED, 0 FAIL** (1287 uruchomionych). Pierwsze uruchomienie Qt ujawniło jeden stary test wymagający `junction UV==empty`; w tym samym etapie poprawiono jego oczekiwania, zachowano guard zabezpieczający, ponowny test `QT_FINAL_RC=0`. Dowody: `archive/visual-regression/s2.2h-full-build.log`, `s2.2h-final-gate.log`, `s2.2h-qt-pass.log`, `s2.2h-final-fps-{x,t,y}.json`. Nie potwierdzaj niezweryfikowanych funkcji.

**S2.2i — lepsze materiały i kontrola FPS PO zmianach (13.11.2026):**

- Zostawiono wszystkie oryginalne pliki Poly Haven CC0 bez zmian. `scripts/build_pbr_atlas.py` (Pillow + NumPy) produkuje deterministyczne, powtarzalne **4×4 atlas PBR 2048²** z dopasowanych przesunięć tych samych map albedo/nor_gl/rough; lekko osłabia dominujące pęknięcie w asfalcie, neutralizuje brąz betonu, wygładza mikro-normal i reguluje roughness. Wyjście `assets/textures/prawko_atlas/` (6 plików, zapisywane w Git). Kontrola `scripts/test_pbr_atlas.py`: **PBR_ATLAS_QA_PASS** (6/6: rozdzielczość, różnorodność kafelków, seamy <25/255). `scripts/check_asset_licenses.py`: **PASS**; sześć wpisów w `ASSETS_LICENSES.md`.
- Qt `editor/resources/resources.qrc` używa nowego atlasu, `MaterialCatalog` dla `asphalt` i `concrete` ma `uv_scale=0.0625` (pełna faza 16 m; wewnętrzne kafelki 4 m), roughness scalar 0.87 / 0.93, normal_strength 0.85 / 0.75. `asphalt_worn` oraz materiały użytkownika pozostawione bez zmian. Eksporter glTF `mesh_export_common.hpp` i `gltf_exporter.cpp` odczytuje dokładnie te same nowe PBR i UV 0.0625; format scen/triangulacja bez zmian. Test Qt `SurfaceTextures.PrawkoAtlasKeepsFourMeterDetailButVariesOverSixteenMeters`.
- Odbiór obrazu: 30 aktualnych kadrów sprzętowego GLRenderer `archive/visual-regression/s2.2i-material-after/` (X/T/Y, 5 kamer, base + LOD), bezpośredni screenshot uruchomionego `roadmaker-editor.exe`: `s2.2i-gui-x-orbit.png`. W porównaniu z S2.2h spękania są mniej dominujące i nie powracają w identycznej konfiguracji co 4 m; beton mniej brązowy, przejście ulicy przez junction nadal poprawne. **Nie mylić z końcowym realizmem** — miejscami widoczne powtarzalne pęknięcia w skali 16 m, brak pełnego oświetlenia i PBR.
- GLB wykonany dla X/T/Y przez aktualny agent (`s2.2i-pbr-{x,t,y}.glb`, odpowiednio 16,75 / 16,69 / 16,66 MB); Blender 5.3 niezależnie zaimportował nowe materiały o obrazach **2048²**, obecne 3 mapy PBR, UV fazy i czoła, geometria niezerowego pola; `PBR_ATLAS_GLTF_IMPORT_PASS` dla X/T/Y. Render EEVEE X: `s2.2i-pbr-x-eevee.png` / logi `s2.2i-blender-{x,t,y}.log`. Blender ciemniejszy od Qt z powodu niezależnego oświetlenia — nie uznawać fotometrycznego parytetu.
- **GPU after** GTX 1050 Ti: mediana GL_TIME_ELAPSED LOD ukośnie X **0,493 ms**, T **0,473 ms**, Y **0,430 ms** (wzrost względem S2.2g ~0,07–0,10 ms); `texture_decode_upload_sync_ms` zimny **612/626/551 ms** X/T/Y vs wcześniejsze ~42–56 ms. Jest to **istotny koszt pierwszego wczytania**; nie występuje co klatkę. Dane `s2.2i-material-after/{x,t,y}-gpu-benchmark.json`.
- **Widoczny Qt FPS after**: 1600×900, po 180 próbkach `frameSwapped`, sztuczny timer 16 ms, symulowany gest `Alt+LMB`, X **60,00 FPS, p95 17,19 ms**; T **60,00, p95 17,36 ms**; Y **60,01, p95 17,11 ms**. We wszystkich **0 klatek >33,33 ms**. JSON `archive/visual-regression/s2.2i-fps-{x,t,y}.json`. Nadal `msaa_samples=0` mimo ustawienia 4x w żądanym formacie. To NIE jest pomiar frametime przeciągania węzłów ani niezależnego maksimum FPS bez VSYNC.
- **Build/test:** pierwszy `prawko-build-win.ps1 -Target all` trafił na stary uszkodzony artefakt CMake `junction_surface.cpp.obj` (LNK1236; kod tego pliku NIENARUSZONY). Usunięto **wyłącznie** ten wygenerowany `.obj`, ponowny `BUILD_PASS: all`. `s2.2i-quality-gate.log`: `QUALITY_GATE_PASS` (C++ 2072 PASS, Python 7 PASS); `s2.2i-qt.log`: **1287 PASS, 1 SKIPPED, 0 FAIL**; nie usuwano scen, nie zmieniano oryginalnego RoadMaker. Agenta Git bez zmian.

**S2.2j — optymalizacja PBR, MSAA, FPS podczas remesh, kontrola Blender (13.11.2026):**

- `scripts/build_pbr_atlas.py` zachowuje **albedo 2048²**, ale po filtrowaniu Lanczos i ponownej normalizacji zapisuje `*_nor_gl_1024.png` i `*_rough_1024.jpg`. Qt qrc i `core/src/io/mesh_export_common.hpp` odczytują dokładnie te same nowe źródła; `uv_scale=0.0625` bez zmian, poprzednie 2048² normal/rough usunięte tylko z bieżącego zestawu wygenerowanych pochodnych (historia Git i oryginalne Poly Haven CC0 nietknięte). `scripts/test_pbr_atlas.py`: **PBR_ATLAS_QA_PASS** (6 plików, ciągłość brzegów, różne kafelki, normalizacja normalnych). `check_asset_licenses.py` **PASS**, rejestr uaktualniony.
- Trzy nowe GLB przez aktualnego agenta: `archive/visual-regression/s2.2j-{x,t,y}.glb` **5 486 412 / 5 424 288 / 5 395 088 B** względem 16 748 684 / 16 686 448 / 16 657 240 B w S2.2i (redukcja ~67%). Blender 5.3 **S2.2J_BLENDER_GLTF_PASS X/T/Y**: dwie mapy albedo 2048², normal i rough 1024², niezerowe pola faz i czoła krawężnika, kompletne UV i materiały. Logi `s2.2j-blender-{x,t,y}.log`. Blender X z kontrolowanym standardowym światłem SUN/ambient i standardowym view transform: `s2.2j-x-blender-daylight.png`. **Nie istnieje pikselowy parytet Qt vs Blender**: dwa różne modele światła, tonemapping i oświetlenie czoła; nie markować jako fotorealizm.
- Rzeczywisty GLRenderer NVIDIA GTX 1050 Ti: `archive/visual-regression/s2.2j-gpu/{x,t,y}-gpu-benchmark.json` oraz 30 zrzutów. Zimny decode+upload (z synchronizacją) **X 267,5 / T 272,6 / Y 279,6 ms**, wobec S2.2i **612,3 / 625,8 / 550,5 ms**. Przy RGBA8 oszacowana pamięć 6 map bez mips spadła ~96 MiB → ~48 MiB (to **obliczenie z rozdzielczości**, nie odczyt rzeczywistego VRAM). GPU p50 LOD oblique **0,583 / 0,568 / 0,558 ms**, względem wcześniejszych 0,493 / 0,473 / 0,430 ms — porównanie serii nie jest odizolowaną ablacją, nie przypisywać różnicy tylko MSAA ani teksturom. `s2.2j-pixel-compare.py` porównał **30 identycznych kadrów** z S2.2i: maks. średnia różnica RGB **0,1741 / 255**, żadnych pikseli różniących się o ponad 16 poziomów; **S2.2J_PBR_IMAGE_REGRESSION_PASS**, zapis `s2.2j-pixel-diff.json`.
- **MSAA diagnoza (bez zmiany kodu produkcyjnego):** pierwotne `viewport->format().samples()==0` w `prawko_qt_fps_probe` NIE mierzyło rzeczywistego antyaliasingu. Po dodaniu diagnostyki `QOpenGLWidget::makeCurrent` i `glGetIntegerv(GL_SAMPLE_BUFFERS / GL_SAMPLES)`, próbka rzeczywistego FBO **bufory=1, próbki=4** dla X/T/Y. Potwierdzono **także po cofnięciu eksperymentalnego `ViewportWidget::setFormat(4)`**: `s2.2j-msaa-no-explicit-viewport-format.json` — nadal 4 próbki. **Nie było udowodnionej usterki MSAA.** Koniec etapu pozostawia produkcyjny `ViewportWidget` bez niepotrzebnej ingerencji.
- `prawko_qt_edit_probe` (manualny `EXCLUDE_FROM_ALL`) używa widocznego Windows Qt `ViewportWidget`, prawdziwego `Document` oraz produkcyjnego `update_node_drag` / `begin_preview` / `update_preview` / remesh / upload GPU, bez zapisu plików scen. `archive/visual-regression/s2.2j-edit-fps-{x,t,y}.json`: **X 60,02 FPS p95 17,27 ms, CPU edycji p95 7,89 ms, 205 zmian siatki; T 60,00 FPS p95 17,23 ms, CPU 6,82 ms, 206 zmian; Y 60,01 FPS p95 17,37 ms, CPU 5,21 ms, 206 zmian; wszystkie 0 klatek >33,33 ms**. `real_edit_verified=true` i aktywna sesja preview. Skrypt wymusza pozycje węzłów timerem 16 ms — NIE jest testem fizycznej myszy ani przeciążonej kilkunastotysięcznej sceny. Dodatkowo pełne GUI `prawko_qt_fps_probe` `s2.2j-qt-fps-{x,t,y}.json`: ~60 FPS, p95 obrotu X 17,20, T 17,23, Y 17,23 ms, 0 jank.
- Kompilacja `s2.2j-final-build.log`: **BUILD_PASS all**, synchronizacja agenta PASS; `s2.2j-final-gate.log`: **QUALITY_GATE_PASS** (C++ 2072 PASS, Python 7 PASS), `s2.2j-final-qt.log`: **1287 PASS + 1 SKIPPED**, 0 FAIL. Po cofnięciu redundantnego `setFormat` ponowiono pełny build i testy. **Uwaga na żywą pracę użytkownika:** od 13:36 `roadmaker-editor.exe` PID 4744, tytuł `scene.xodr* - PrawkoRoadStudio...` (**asterysk = niezapisane zmiany**). Nie ubijać, nie przeładowywać i nie nadpisywać tej sceny. Ponowiony opt-in benchmark edycji dla ostatecznej wersji binarnej celowo **wstrzymano**, żeby nie obciążać aktywnej sesji graficznej; powyższy benchmark edycji był wykonany wcześniej z jedyną eksperymentalną zmianą `setFormat`, potem cofniętą po ablacjach FBO. Nie uznawać braku końcowego powtórzenia za PASS tego konkretnego porównania.

**Historyczny plan po S2.2l (częściowo zrealizowany w S2.2m; aktualny krok S2.2n jest na początku HANDOFF):** S2.2l (instrukcje i obsługa stylów przez przeciąganie i dwuklik) zakończony — wynik i dowody na początku HANDOFF. Rozszerzyć testy fizycznego drag & drop na obiekty, sygnały, przejścia, materiały i obszary niedozwolone, ustalić spójny model edycji po upuszczeniu. Następnie wdrożyć parametryczny krawężnik obniżony/rampę oraz wysepkę jako samodzielnie przeciągany obiekt z zachowaniem siatki i OpenDRIVE. Powtórzyć test obciążeniowy większych scen (wiele dróg/props, ruch myszy, drag/remesh, p95/p99, VRAM). Oddzielnie wykończyć oświetlenie Qt/Blender oraz widoczne długie spękania. Nie uznawać fotorealizmu za ukończony. Nie zmieniać CDT/OpenDRIVE/goldenów bez uzasadnienia.





**Historyczny plan S2.2c:** dokończyć 5 cm fazę krawężnika
przez cały łuk narożnika (obecna ściana narożnika jest
pionowa, a faza drogi zwęża się przed wlotem), zszyć
górną krawędź i sprawdzić profile z nachyleniem oraz
przejścia na ciasnych T/Y. Następnie eksport glTF,
test wizualny z kamery kierowcy i dopiero później PBR.
Nie uznawać obecnych materiałów za realistycznie odebrane.

**Historyczna lista odbiorów geometrii po S2.2j i S2.2k.**

1. Uruchomić `scripts/prawko-resume.ps1` i sprawdzić oba repozytoria oraz otwarte sceny. Nie przeładowywać niezapisanych dokumentów.
2. **Testy Qt Windows:** ostatnia pełna sesja **S2.2k 1292 PASS, 1 SKIPPED, 0 FAIL**, `archive/visual-regression/s2.2k-qt-full-final.log`; `qoffscreen.dll` w `build/dev-windows/editor/platforms/`. Nowe 12 pozycji biblioteki są wbudowane w exe i dostępne od startu, nie trzeba ich importować.
3. **S2.2c — odbiór bazowej geometrii X/T/Y i niezależnego GLB wykonany.** Zrzut bieżącego GUI, kamera kierowcy i izolat narożnika w Blenderze oraz import GLB są w lokalnym `archive/visual-regression/`. Testy i dowody wyżej. Zwróć uwagę na segmentację łuku — nie mylić z ostatecznym wyglądem.
4. **S2.2j edycja geometrii i MSAA:** 205–206 aktualizacji siatki na scenę X/T/Y z prawdziwą sesją preview, 60 FPS, p95 17,23–17,37 ms (timer testowy). GL_SAMPLE_BUFFERS=1 i GL_SAMPLES=4 — wcześniejsze `format().samples()==0` było niemiarodajne. **Następne:** test pełnej sceny pod obciążeniem i fizycznej myszy, gdy użytkownik zamknie lub zapisze otwarty niezapisany dokument. Nie zmieniać CDT, OpenDRIVE ani goldenów.
5. **S2.2j odciążenie materiałów wykonane:** zimny upload 0,268–0,280 s, GLB 5,4–5,5 MB, 30 widoków RGB regresja PASS, Blender X/T/Y PASS. Światło Qt/Blender porównano z kontrolowanym ambient+SUN, ale wynik nadal nie jest fotometrycznie równoważny, krawężnik i długie spękania w 16 m atlasie nadal potrzebują dopracowania.

**Kryterium ukończenia:** kod + testy + rzeczywista ocena GUI; potem commity, push i aktualizacja TEGO pliku. Bez zgadywania stanu na podstawie dawnych raportów.

## Jak rozpocząć i zweryfikować

W PowerShell (z dowolnego miejsca):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1 -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-build-win.ps1 -Target editor -Jobs 3
```

- `prawko-resume.ps1` czyta **bieżący Git** i ten dokument, sygnalizuje odchylenia względem `origin`.
- `-Verify` obejmuje Quality Gate jądra i agenta. Testy Qt uruchom oddzielnie po `prawko-build-win.ps1 -Target all`, z `QT_QPA_PLATFORM=offscreen` i Qt `bin` w `PATH`. Plik wykonywalny to `build/dev-windows/editor/roadmaker_editor_tests.exe`. Aktualne wyniki zapisuj w sekcji **Stan bieżący**.
- Start programu: `build/dev-windows/editor/roadmaker-editor.exe`.
- Agent: `scripts/prawko-agent.cmd`.
- Główne wymagania: `docs/PRAWKO-ROAD-STUDIO.md`.
- Reguły współpracy: `AGENTS.md`.

## Reguła przekazywania stanu — po każdym ważnym etapie i na końcu

**Zapisuj postęp po każdym ukończonym etapie.** Wykonaj właściwe testy, zaktualizuj sekcje **Stan bieżący** i **Aktywny plan** na górze, zacommituj zmiany i wypchnij je na GitHub.

1. Nie zostawiaj rozpoczętych zmian bez informacji. Jeśli praca została przerwana, wpisz do tego pliku **co zmieniono, co nie zostało przetestowane i co powinno być zrobione jako pierwsze**. Nie nazywaj takiego kroku PASS.
2. Wpisz aktualny stan i następny konkretny krok do **tej samej sekcji** (bez mnożenia notatek). Zmień znacznik `agent_head` poniżej po ostatnim commicie agenta.
3. Zacommituj zmiany w odpowiednich repozytoriach; ostatni commit edytora powinien zawierać aktualizację `HANDOFF.md`. Wypchnij do `origin` i sprawdź SHA zdalnych gałęzi.
4. Wykonaj `scripts/prawko-git.ps1 -Action finish` (sprawdzenie czystości, Quality Gate i backup). Przekaż użytkownikowi oba SHA, wyniki, ewentualne blokady.
5. **Nie nadpisuj otwartej niezapisanej sceny.** Pliki `agent/projects/` nie są objęte Git; backupują się lokalnie, ale niezapisanej sceny GUI nie da się odzyskać z tych kopii.
6. Jeśli nie ma dostępu do komputera lub testów, opisz ograniczenie, zamiast twierdzić, że praca została wykonana.

<!-- agent_head: 67a92fa24b4054b7f094c9b667f3634c35ad9210 -->
