# PrawkoRoadStudio — PRZEKAZANIE PRACY / START TUTAJ

> **Jedyny aktualizowany dziennik przekazania między rozmowami.**
> Kolejny agent ma czytać ten plik **przed edycją kodu**.
> Stan dokumentu: 2026-11-12, po konfiguracji publicznych repozytoriów GitHub.
> Poniższe SHA to punkt kontrolny, nie zamiennik aktualnego `git status`.

## Gotowe polecenie do nowej rozmowy

> **Kontynuuj PrawkoRoadStudio od miejsca przerwania. Połącz się z moim komputerem przez Desktop Commander. Zacznij od `F:\PrawkoRoadStudio\HANDOFF.md` i `AGENTS.md`; uruchom `scripts/prawko-resume.ps1`, sprawdź oba repozytoria Git oraz testy. Nie zgaduj stanu z poprzedniej rozmowy. Dokończ pierwsze niewykonane zadanie z HANDOFF.md, a na koniec zaktualizuj HANDOFF.md, wykonaj commity, push i Quality Gate. Nie nadpisuj niezapisanych scen.**

## Gdzie pracować

- Komputer przez Desktop Commander: `DESKTOP-G2FV0GJ`.
- **Aktywny katalog:** `F:\PrawkoRoadStudio` (C++20, Qt 6).
- **Agent AI — osobne repo Git:** `F:\PrawkoRoadStudio\agent`.
- Repo publiczne edytora: https://github.com/prawkonaraz100/PrawkoRoadStudio
- Repo publiczne agenta: https://github.com/prawkonaraz100/PrawkoRoadStudio-Agent
- Obie gałęzie edytora (`main`, `feature/realistic-road-surfaces`) wskazywały ten sam kod przed utworzeniem tego przekazania.
- Kopia oryginału `reference/RoadMaker/` jest **tylko referencyjna** — nie edytuj, nie wysyłaj zmian do upstream.
- `toolchain/`, `.qt/`, `build/`, `agent/projects/` i `archive/` pozostają na dysku; część jest ignorowana przez Git i nie występuje w publicznych repozytoriach.

## Ostatnie potwierdzone wykonanie

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

## Co robimy dalej — najbliższe zadanie

**Etap S2 — po audycie kodu w bieżącej sesji (12.11.2026).**
Pracę nad S2 rozpoczęto od odczytu aktualnych źródeł. Nie wykonano jeszcze
zmian w geometrii ani nowego wizualnego testu GUI, więc S2 nie jest ukończony.

**Konkretna przyczyna wykrytej różnicy wysokości:**
- `core/src/mesh/mesh_builder.cpp` w `build_one_road()`: chodnik jezdni ma osobną
  siatkę z `defaults::kCurbHeight` (0,15 m), ścianą i fazą krawężnika.
- Ten sam moduł przy końcu drogi prowadzącym do skrzyżowania stosuje rampę
  `kJoinRampM = 4.0` i wygasza wysokość do zera, bo na skrzyżowaniu nie ma
  jeszcze prawdziwego krawężnika.
- `core/src/mesh/junction_surface.cpp` w `build_junction_floor_split()`:
  chodniki na skrzyżowaniach są tylko klasyfikacją trójkątów bazowego
  `SubMesh` jako `LaneType::Sidewalk` — bez podniesienia ich geometrii i
  bez siatki pionowej ściany.
- `core/src/mesh/junction_sidewalk_bands.cpp` dostarcza istniejące obrysy
  naroży i ścieżki materiału; należy je **wykorzystać**, nie budować osobnego
  generatora OpenDRIVE.
- `core/tests/test_junction_sidewalk_bands.cpp` ma macierz T/X/Y, kąty,
  profile mieszane i chodnik za poboczem; nowe testy 3D muszą korzystać z tych
  przypadków i sprawdzać ciągłość wysokości na styku.
- **Edytor jest uruchomiony z tytułem `scene.xodr*`**: gwiazdka oznacza
  niezapisane zmiany. Nie zamykaj, nie nadpisuj i nie uruchamiaj automatycznego
  przeładowania tej sceny. Zrzut całego ekranu pokazuje okno przeglądarki na
  pierwszym planie, zatem nie potwierdza jeszcze wyglądu sceny w edytorze.

**Następny krok kodowy:** opracować i przetestować wspólną siatkę wyniesionych
narożników i ścian krawężników na skrzyżowaniach, zachowując stary płaski
tryb `raised_sidewalks=false`; dopiero po zszyciu topów i ścian usunąć
dotychczasowe wygaszenie przy wlotach. Weryfikować na T, X, ostrych kątach,
łukach i profilach mieszanych. Oceny GUI dokonać bez ingerencji w niezapisaną
scenę, np. w oddzielnej kopii testowej.

**Kontynuować przebudowę realistycznej geometrii dróg i chodników.** Nie zaczynać ponownie od konfiguracji Git ani migracji folderów.

1. Sprawdzić aktualny stan **obu** repozytoriów, obu zdalnych gałęzi, otwartych procesów i scen. Uruchomić `scripts/prawko-resume.ps1`.
2. Obejrzeć w GUI drogę na prostej, łuku i skrzyżowaniach T/X. Ustalić, które krawężniki/narożniki nadal tworzą płaskie pasma, szczeliny, nakładanie powierzchni lub z-fighting. **Brak nowego potwierdzonego przeglądu GUI wszystkich tych przypadków**.
3. Rozpocząć od kodu `core/src/mesh/junction_sidewalk_bands.cpp`, `junction_corners.cpp`, `junction_corner_detail.cpp`, `mesh_builder.cpp` oraz odpowiednich testów. Najpierw przeanalizować istniejące algorytmy i testy, nie przepisywać ich w ciemno.
4. Zaimplementować **ciągłą przestrzenną geometrię wyniesionych naroży chodnika i obniżeń przy przejściach**, spójną na łukach i skrzyżowaniach. Zrobić to w tym samym modelu C++, którego używa GUI oraz Python/agent. Nie zastępować geometrii samą teksturą.
5. Weryfikacja: kompilacja Windows, testy C++ i agenta, zapis/odczyt OpenDRIVE, Undo/Redo, walidacja scen, ręczna kontrola GUI na prostej, łuku, T i X, brak widocznych przerw/kolizji siatki.
6. Przegląd wizualny nawierzchni (materiały, normal/roughness, skala tekstur i oświetlenie) jest kolejnym etapem po geometrii. Nie raportować tego jako wykonane.

**Kryterium ukończenia:** kod + testy + rzeczywista ocena GUI; potem commity, push i aktualizacja TEGO pliku. Bez zgadywania stanu na podstawie dawnych raportów.

## Jak rozpocząć i zweryfikować

W PowerShell (z dowolnego miejsca):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-resume.ps1 -Verify
powershell -NoProfile -ExecutionPolicy Bypass -File F:\PrawkoRoadStudio\scripts\prawko-build-win.ps1 -Target editor -Jobs 3
```

- `prawko-resume.ps1` czyta **bieżący Git** i ten dokument, sygnalizuje odchylenia względem `origin`.
- `-Verify` uruchamia Quality Gate. Pełna kompilacja i **ręczny test GUI** są osobną kontrolą.
- Start programu: `build/dev-windows/editor/roadmaker-editor.exe`.
- Agent: `scripts/prawko-agent.cmd`.
- Główne wymagania: `docs/PRAWKO-ROAD-STUDIO.md`.
- Reguły współpracy: `AGENTS.md`.

## Reguła przekazywania stanu — po każdym ważnym etapie i na końcu

**Nie czekaj na zakończenie rozmowy.** Po każdym ukończonym kroku aktualizuj poniższy stan, zapisuj commity i wypychaj je na GitHub. Nagłe urwanie rozmowy nie powinno oznaczać utraty poprzednich punktów kontrolnych.

1. Nie zostawiaj rozpoczętych zmian bez informacji. Jeśli praca została przerwana, wpisz do tego pliku **co zmieniono, co nie zostało przetestowane i co powinno być zrobione jako pierwsze**. Nie nazywaj takiego kroku PASS.
2. Wpisz aktualny stan i następny konkretny krok do **tej samej sekcji** (bez mnożenia notatek). Zmień znacznik `agent_head` poniżej po ostatnim commicie agenta.
3. Zacommituj zmiany w odpowiednich repozytoriach; ostatni commit edytora powinien zawierać aktualizację `HANDOFF.md`. Wypchnij do `origin` i sprawdź SHA zdalnych gałęzi.
4. Wykonaj `scripts/prawko-git.ps1 -Action finish` (sprawdzenie czystości, Quality Gate i backup). Przekaż użytkownikowi oba SHA, wyniki, ewentualne blokady.
5. **Nie nadpisuj otwartej niezapisanej sceny.** Pliki `agent/projects/` nie są objęte Git; backupują się lokalnie, ale niezapisanej sceny GUI nie da się odzyskać z tych kopii.
6. Jeśli nie ma dostępu do komputera lub testów, opisz ograniczenie, zamiast twierdzić, że praca została wykonana.

<!-- agent_head: 147a08d537530a4b07b0bd7840f6197dc9f62512 -->
