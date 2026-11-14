# PrawkoRoadStudio

**Edytor scen drogowych PrawkoNaRaz** — tworzenie dróg, chodników i scenariuszy ruchu w jednym widoku 3D, z zapisem OpenDRIVE (XODR) i OpenSCENARIO (XOSC).

> Kontynuujesz pracę po przerwaniu rozmowy? Przeczytaj [HANDOFF.md](HANDOFF.md) i [AGENTS.md](AGENTS.md), a następnie uruchom `scripts/prawko-resume.ps1`. Tylko potwierdzony kod i testy oznaczamy jako ukończone.

## Czysta biblioteka Prawko

Aktualnie w bibliotece udostępnione są wyłącznie trzy kategorie:

| Kategoria | Zawartość |
|---|---|
| **Prawko \| Gotowe drogi** | 6 własnych profili dróg |
| **Prawko \| Krawężniki i chodniki** | 6 własnych wariantów nawierzchni, krawężników i chodników |
| **Uczestnicy ruchu** | samochód, ciężarówka, autobus, motocykl, rower, pieszy |

**Razem: 18 elementów.** Pozostałe kategorie z bazowego RoadMaker zostały usunięte z aktywnego manifestu i zasobów graficznych palety. Nowe rodzaje obiektów będą tworzone i dodawane stopniowo jako własne komponenty PrawkoNaRaz.

Nową scenę rozpoczynamy przez **New Scene**, bez wczytywania przykładowego skrzyżowania. Aby dodać drugą lub trzecią drogę, przeciągamy wybrany szablon na scenę, wskazujemy kolejny punkt i zatwierdzamy **Enter**. Zaznaczonego uczestnika ruchu można przesunąć w **Select** lub **Move**.

## Projekt i pliki

- `editor/` — aplikacja Qt/C++20, narzędzia myszy i biblioteka.
- `core/` — geometria, formaty OpenDRIVE/OpenSCENARIO, eksporty i testy silnika.
- `agent/` — automatyzacja scen i polecenia agenta (osobne repozytorium Git).
- `assets/library/manifest.json` — **jedyna aktywna lista wbudowanych komponentów**.
- `core/tests/fixtures/legacy_scenes/`, `legacy_prop_sources/`, `legacy_sign_sources/` — historyczne dane do regresji formatów i geometrii, **nie są zasobami biblioteki ani przykładami na ekranie startowym**.
- `archive/` — lokalne raporty, zapisy odbiorów i kopie bezpieczeństwa (poza Git).

Testowe sceny historyczne wydzielono, aby zachować pokrycie testami bez zaśmiecania projektu użytkownika. Lokalne stare projekty scenariuszy usunięto z `agent/projects/` po zweryfikowanej kopii ZIP.

## Uruchomienie (Windows)

Po przygotowaniu lokalnego środowiska i skompilowaniu projektu:

```powershell
cd F:\PrawkoRoadStudio
powershell -ExecutionPolicy Bypass -File scripts\prawko-resume.ps1 -Verify
powershell -ExecutionPolicy Bypass -File scripts\prawko-build-win.ps1 -Target all -Jobs 3
.\build\dev-windows\editor\roadmaker-editor.exe
```

Nie nadpisuj niezapisanej sceny i nie uruchamiaj jednocześnie dwóch agentów edytujących te same pliki. Wdrożenia są wykonywane w gałęzi roboczej i zatwierdzane po zielonym Quality Gate.

## Dokumentacja i pochodzenie

- [HANDOFF.md](HANDOFF.md) — potwierdzony stan, testy, otwarte zadania i kolejny krok.
- [AGENTS.md](AGENTS.md) — zasady bezpieczeństwa zmian, kontroli jakości i Git.
- [Opis studia i operacji](docs/PRAWKO-ROAD-STUDIO.md).
- [Architektura silnika](docs/architecture/overview.md).

Silnik PrawkoRoadStudio wywodzi się z otwartego projektu [Robomous/RoadMaker](https://github.com/Robomous/RoadMaker). Zachowane pliki źródłowe i ich oznaczenia prawne podlegają odpowiednim warunkom [Apache 2.0](LICENSE). PrawkoNaRaz rozwija własny interfejs, geometrię i bibliotekę.
