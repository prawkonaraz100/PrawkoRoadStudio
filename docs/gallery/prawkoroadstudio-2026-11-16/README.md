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
