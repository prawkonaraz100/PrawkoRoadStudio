# AGENTS.md — PrawkoRoadStudio

To jest aktywny projekt rozwoju edytora dróg PrawkoRoadStudio, oparty na
oryginalnym RoadMaker (licencja Apache-2.0). Projekt służy budowaniu scen
drogowych do nauki jazdy oraz poprawie jakości wizualnej siatki 3D.

**Źródła projektu:** `F:\PrawkoRoadStudio`.
**Gałąź funkcjonalna:** `feature/realistic-road-surfaces`.
**Dokument nadrzędny:** `docs/PRAWKO-ROAD-STUDIO.md`.

## Pierwsze kroki

1. Połącz się z DESKTOP-G2FV0GJ przez Desktop Commander.
2. Sprawdź `git status`, `git log -1` i dokument nadrzędny.
3. Czytaj i modyfikuj tylko aktywne repo `F:\PrawkoRoadStudio`.
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
6. **PRZEKAZANIE:** odnotuj pełne SHA obu HEAD, wyniki kompilacji i testów,
   lokalizacje backupów, niewykonane kroki oraz stan otwartych scen.

Archiwa Git **nie zawierają niezacommitowanych zmian** ani ignorowanych
narzędzi i bibliotek; zapisy scen z `agent/projects/` są kopiowane osobno.
Sceny niezapisanej w GUI nie da się zabezpieczyć bez jej wcześniejszego
zapisu. Backupy lokalne na F: nie chronią przed awarią tego dysku.

**Docelowo:** dwa prywatne repozytoria `origin` na zewnętrznym serwerze
(np. GitHub), potwierdzony `git push` obu gałęzi oraz druga kopia scen poza
dyskiem F:. `source-local` to wyłącznie lokalna kopia referencyjna,
`upstream` to oryginalny publiczny RoadMaker; **nie wysyłaj tam zmian
naszego projektu**. Nie usuwaj lokalnych archiwów przed potwierdzeniem
zewnętrznej kopii.
