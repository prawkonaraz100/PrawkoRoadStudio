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
   MSVC z `F:\PrawkoRoadStudio-toolchain`.
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
- Nie zmieniaj oryginalnego `F:\RoadMaker-evaluation`, który pozostaje
  źródłem Qt, Pythona, CMake i kopią porównawczą.
- Interfejs AI i generowania scen: `F:\RoadMakerAgent`.
  Wywołuj go przez `F:\PrawkoRoadStudio\scripts\prawko-agent.cmd`,
  które wybiera nowy edytor i wspólne OpenDRIVE/OpenSCENARIO.
  Moduł Python należy zaktualizować po kompilacji nowego forka.
- Stary `F:\RoadSceneEditor` Unreal został usunięty 12.11.2026,
  więc nie zakładaj jego istnienia ani nie przywracaj go.
- Historyczny Git bundle/patch: `F:\PrawkoRoadStudio-legacy-backup\2026-11-12`.

## Przekazanie stanu

Weryfikuj najnowszy kod i procesy: nie zakładaj, że build już przeszedł,
jeżeli nie widzisz wyjścia PASS. Przed końcem podaj sprawdzone artefakty,
wersję Git i ewentualne ograniczenia.
