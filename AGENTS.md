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

## Rytual Git dla kazdej sesji (obowiazkowy)

W folderze glownym sa **dwa oddzielne repozytoria Git**:
- edytor: `F:\PrawkoRoadStudio` (aktywny branch: `feature/realistic-road-surfaces`),
- agent AI: `F:\PrawkoRoadStudio\agent` (branch: `main`).

Katalog `agent/` jest celowo ignorowany przez Git edytora. Nie oznacza to,
ze kod agenta jest zapisywany w historii edytora. Kazde repo wymaga
**wlasnego commita i wlasnego backupu**.

1. **START:** `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action status`.
   Odczytaj oba HEAD, branch, dirty paths i remote. Nie nadpisuj cudzych zmian.
2. **PRACA:** jedna funkcja = jedna galaz zadaniowa lub logiczny commit.
   Przed zmiana sprawdz `git diff`, po zmianie kompilacje i testy
   stosowne do zakresu. Sceny z otwartego GUI zapisuj za zgoda uzytkownika.
3. **COMMIT:** dla obu repozytoriow osobno: `git diff --check`,
   przeglad `git status`, selektywne `git add <pliki>`, opisowy commit.
   Nie uzywaj `git add -A` bez audytu, nie uzywaj `git reset --hard`,
   `git clean -fdx`, force-push ani `git push upstream`.
4. **BACKUP:** `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action backup`.
   Tworzy i weryfikuje osobne `git bundle` + SHA256/manifest w
   `archive/git-backups/` (katalog poza historia repo, ale na tym samym dysku).
5. **KONIEC:** po doprowadzeniu obu repo do czystego stanu uruchom
   `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/prawko-git.ps1 -Action finish`.
   Przeprowadzi Quality Gate i kopie Git, odmowi PASS gdy sa zmiany bez commita.
6. **PRZEKAZANIE:** podaj dokladne SHA obu HEAD, wyniki build/test,
   lokalizacje backupow, istniejace niezapisane sceny i otwarte zadania.

`git bundle` NIE obejmuje niezacommitowanych zmian, ignorowanych `agent/projects/`,
zaleznosci ani scen niezapisanych w GUI. Sceny wymagaja osobnej kopii katalogu.
Backup `archive/git-backups/` jest lokalny; utrata dysku F: nadal oznacza ryzyko.
Trwaly backup wymaga prywatnych repozytoriow zdalnych (osobno dla edytora
i agenta) oraz potwierdzonego `git push` kazdego brancha. Nigdy nie publikuj
naszego kodu na zewnetrznych `upstream` bez wyraznej zgody.
