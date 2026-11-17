# AGENTS.md — PrawkoRoadStudio

To jest aktywny projekt rozwoju edytora dróg PrawkoRoadStudio, oparty na
oryginalnym RoadMaker (licencja Apache-2.0). Projekt służy budowaniu scen
drogowych do nauki jazdy oraz poprawie jakości wizualnej siatki 3D.

**Źródła projektu:** `F:\PrawkoRoadStudio`.
**Gałąź funkcjonalna:** `feature/realistic-road-surfaces`.
**Dokument nadrzędny (wymagania):** `docs/PRAWKO-ROAD-STUDIO.md`.
**Ostatni stan i następne zadanie (START TUTAJ):** `HANDOFF.md`.

**JEDEN AKTYWNY PROJEKT (17.11.2026):** Edytor rozwijaj wyłącznie w
`F:\PrawkoRoadStudio`, na `feature/realistic-road-surfaces`. Zwykła
aplikacja to `F:\PrawkoRoadStudio\build\dev-windows\editor\roadmaker-editor.exe`,
a sceny to `F:\PrawkoRoadStudio\projects`. `archive/render-opt` i
`archive/render-integration` są WYŁĄCZNIE historycznymi worktree do odczytu
starych dowodów; NIE uruchamiaj ich aplikacji, NIE edytuj tam kodu ani NIE
przenoś tam nowych scen. Po R6b2k wszystko potrzebne jest już w aktywnej
gałęzi (`21ac5d9`). Agenta rozwijaj nadal w jego przewidzianym repo `agent/`:
jest modułem TEGO SAMEGO produktu, nie drugim edytorem. `main` edytora
pozostaje starszą gałęzią, NIE osobnym projektem; nie zmieniaj strategii
scalania bez zgody użytkownika. Standardowe Git hooks (pre-commit/pre-push)
blokują pracę/publikację z archiwalnych kopii; po nowym klonie odtwórz
lokalnie `git config core.hooksPath F:/PrawkoRoadStudio/scripts/git-hooks`.

**JEDEN SKRÓT I JEDNA GLOBALNA BIBLIOTEKA (17.11.2026):** na pulpicie
tylko `PrawkoRoadStudio.lnk`, bez argumentu `.xodr`. START ZAWSZE pokazuje
ekran wyboru projektu, NIE ładuje ostatniego automatycznie. Kliknięcie w liście
ostatnich projektów tylko zaznacza; `Otwórz wybrany` otwiera. `Usuń z listy`
usuwa jedynie wpis w Settings, **NIGDY pliki na dysku**. `Plik` ma
`Nowy projekt`, `Otwórz projekt`, `Ostatnie projekty`, `Zamknij projekt`
oraz osobne `Zamknij scenę`. Nowe projekty trafiają do osobnych folderów,
a nie do katalogu nadrzędnego. **Jedyna wspólna biblioteka assetów wszystkich
projektów** jest w `F:\PrawkoRoadStudio\assets/library/manifest.json`,
modele w `assets/props/`, miniatury w `assets/library/thumbnails/`:
12 gotowych nawierzchni + minimum 19 własnych GLB. Wbudowane statyczne
12 pozycji dla zasobów Qt to `assets/library/bundled-defaults.json`, nie
druga biblioteka użytkownika. Nowy import GLB i materiałów do wspólnego
katalogu, nie do projektowego. Stare kopie GLB w `projects/Prawko-Scenariusze/assets`
pozostały wyłącznie dla zgodności ze starymi wersjami; manifest tego projektu
nie rejestruje już 19 oddzielnych modeli. Nie przywracaj automatycznego
otwierania projektu ani odrębnych bibliotek dla nowych projektów.

## Pierwsze kroki

1. Połącz się z Windows przez **PrawkoBridge-Control**: prywatne repozytorium `prawkonaraz100/PrawkoBridge-Control`, Issue #353 i podłączony konektor GitHub. Wysyłaj tam polecenia `/prawko status`, `/prawko dev inspect`, `/prawko dev read HANDOFF.md` i odbieraj potwierdzone wyniki z GitHub Actions. Lokalny self-hosted runner `prawko-road` musi działać w sesji użytkownika. Instrukcje połączenia są w README prywatnego repo. Nie wymagaj Desktop Commandera ani dodatkowego tunelu, jeśli PrawkoBridge działa.
2. **Najpierw przeczytaj `F:\PrawkoRoadStudio\HANDOFF.md`.**
   Uruchom `scripts/prawko-resume.ps1` — wykrywa live SHA obu repo, status
   zmian i zgodność z GitHub. Bez tego nie podejmuj pracy z pamięci rozmowy.
3. Sprawdź wymagania nadrzędne i czytaj tylko aktywny projekt
   `F:\PrawkoRoadStudio` oraz jego osobne repo `agent/`.
4. Buduj przez `scripts/prawko-build-win.ps1` przy użyciu lokalnego
   MSVC z `F:\PrawkoRoadStudio\toolchain`.
5. Funkcja jest ukończona tylko po testach C++, walidacji scen i realnym
   sprawdzeniu programu Windows. Nie przedstawiaj zamierzeń jako ukończonych.

## Reguły projektu

- Twórz pełne trójwymiarowe drogi, krawężniki, chodniki, narożniki i pobocza,
  zamiast jedynie zmieniać kolory płaskich powierzchni.
- Geometria musi działać na prostych, łukach i skrzyżowaniach.
- Każdą funkcję wymaganą przez agenta zaimplementuj także w edytorze
  obsługiwanym **myszką**. CLI i GUI dzielą jeden model C++.
- Utrzymuj formaty OpenDRIVE (.xodr) i OpenSCENARIO (.xosc),
  trwałe identyfikatory, Undo/Redo i zapis/odczyt.
- Nie zapisuj nad otwartą, zmodyfikowaną sceną użytkownika.
- Zmiany w kodzie przez osobne commity i testy, bez zbędnego duplikowania
  dokumentacji.
- Nie zmieniaj oryginalnego `F:\PrawkoRoadStudio\reference\RoadMaker`, który pozostaje
  źródłem Qt, Pythona, CMake i kopią porównawczą.
- Interfejs AI i generowania scen: `F:\PrawkoRoadStudio\agent`.
  Wywołuj go przez `F:\PrawkoRoadStudio\scripts\prawko-agent.cmd`,
  które wybiera nowy edytor i wspólne OpenDRIVE/OpenSCENARIO.
  Moduł Python należy zaktualizować po kompilacji nowego forka.
- Stary `F:\RoadSceneEditor` Unreal został usunięty 12.11.2026,
  więc nie zakładaj jego istnienia ani nie przywracaj go.
- Historyczny Git bundle/patch: `F:\PrawkoRoadStudio\archive\legacy-backup\2026-11-12`.

## Przekazanie stanu

Weryfikuj najnowszy kod i procesy: nie zakładaj, że build już przeszedł,
jeżeli nie widzisz wyjścia PASS. Przed końcem podaj sprawdzone artefakty,
wersję Git i ewentualne ograniczenia.

## Procedura Git dla każdej sesji (obowiązkowa)

W folderze głównym znajdują się **dwa niezależne repozytoria Git**:
- edytor: `F:\PrawkoRoadStudio` (gałąź `feature/realistic-road-surfaces`),
- agent AI: `F:\PrawkoRoadStudio\agent` (gałąź `main`).

`agent/` jest celowo ignorowany przez Git edytora. **Kod i historia agenta
nie są częścią commitów edytora.** Oba repozytoria wymagają osobnych
commitów, testów i kopii historii.

1. **START:** z katalogu głównego uruchom
   `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action status`.
   Sprawdź oba HEAD, gałęzie, zmienione pliki oraz remotes. Nie nadpisuj
   nieswoich zmian ani niezapisanej sceny.
2. **PRACA:** jedna funkcja lub logiczny krok = osobny commit (w razie potrzeby
   osobna gałąź). Przed edycją sprawdź `git diff`. Po zmianie sprawdź
   kompilację, właściwe testy i działanie GUI, jeżeli funkcja dotyczy edytora.
3. **COMMIT:** dla każdego zmienionego repozytorium osobno uruchom
   `git diff --check`, przejrzyj `git status`, użyj selektywnego
   `git add <pliki>` i opisz commit. Gdy brak konfiguracji autora,
   techniczny commit agenta wykonaj z lokalnymi parametrami
   `git -c user.name="PrawkoRoadStudio Agent" -c user.email="agent@prawkoroadstudio.invalid" commit -m "..."`.
   Nie zgaduj tożsamości użytkownika ani nie zmieniaj globalnego Git.
   Bez `git reset --hard`, `git clean -fdx`, `git push --force`
   i automatycznego `git add -A`.
4. **BACKUP:** po ważnych commitach uruchom
   `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action backup`.
   Skrypt tworzy osobne `git bundle`, testuje ich odtworzenie w pustym
   repozytorium, zapisuje SHA-256 i manifest. Kopiuje też **zapisane pliki**
   z `agent/projects/` do `archive/git-backups/`.
5. **KONIEC:** oba repozytoria muszą być czyste. Uruchom
   `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action finish`.
   Skrypt blokuje zakończenie przy niezacommitowanych zmianach, wykonuje
   Quality Gate oraz kontrolę archiwów. Osobno potwierdź pełny build/GUI,
   jeśli był wymagany.
6. **PRZEKAZANIE — OBOWIĄZKOWE, nie tylko na końcu rozmowy:** po każdym
   potwierdzonym etapie zaktualizuj `HANDOFF.md` w głównym repo
   (ostatnia potwierdzona zmiana, aktualne wyniki, blokady, dokładny kolejny
   krok, znacznik `agent_head`), a następnie wykonaj ostatni commit edytora
   **zawierający `HANDOFF.md`**. Wyślij commity obu repo na GitHub.
   `-Action finish` odmówi PASS, jeśli handoff jest nieaktualny lub SHA w
   GitHub nie zgadzają się z lokalnymi. Podaj pełne SHA obu HEAD, wyniki
   testów, backup i stan otwartych scen.

Archiwa Git **nie zawierają niezacommitowanych zmian** ani ignorowanych
narzędzi i bibliotek; zapisy scen z `agent/projects/` są kopiowane osobno.
Sceny niezapisanej w GUI nie da się zabezpieczyć bez jej wcześniejszego
zapisu. Backupy lokalne na F: nie chronią przed awarią tego dysku.

**GitHub (PUBLIC, stan potwierdzony 12.11.2026):**
- edytor: `https://github.com/prawkonaraz100/PrawkoRoadStudio`, `origin`, gałęzie `main` i `feature/realistic-road-surfaces`;
- agent: `https://github.com/prawkonaraz100/PrawkoRoadStudio-Agent`, `origin`, gałąź `main`.

Każdy nowy commit należy wysłać na odpowiednią gałąź `origin` i sprawdzić
zgodność SHA z GitHub przed zgłoszeniem pełnej synchronizacji. Repozytoria
są **publiczne**: nie zapisuj w Git haseł, tokenów, prywatnych kluczy ani
danych użytkowników. Druga kopia scen poza dyskiem F: nadal jest potrzebna.
`source-local` to tylko lokalna kopia referencyjna, `upstream` to oryginalny
publiczny RoadMaker; **nie wysyłaj tam zmian naszego projektu**.
Nie usuwaj lokalnych archiwów przed potwierdzeniem zewnętrznej kopii.
