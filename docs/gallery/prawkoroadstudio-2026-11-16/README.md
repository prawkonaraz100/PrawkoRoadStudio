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
