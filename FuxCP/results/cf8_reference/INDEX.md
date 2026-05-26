# cf8_reference

Contrepoints de référence sur le CF n°8 (Do Reference Grave), générés avec le
binaire **patché**. Preset `bryce`, objectif lex pondéré, timeout 180000 ms,
stagnation 0 ms.

Correctifs de consonance sur temps faibles disjoints (2026-05-26) :
- **3e espèce 2 voix** : `H2_3_..._multiVoice` réutilisé (consonance sur les 3 temps
  faibles, calculée depuis les notes vs la basse).
- **5e espèce (toutes voix)** : `SP5_H6` corrigé — sur chaque note de 3e espèce
  (`isThirdSpeciesArray`), les 3 temps faibles doivent être consonants ou note de
  passage (avant : seul le temps central, et exemption fautive des notes de 3e).
  Vérifié A/B (patch OFF : 3-6 dissonances illégales par config → patch ON : 0) et
  sur les 9 configs 5e résolues : **0** dissonance disjointe illégale sur cellules 3e.

Les fichiers de 5e espèce incluent une ligne `isThirdSpeciesArray (CPk)` : `1` = vraie
attaque de 3e espèce (consonance contrôlée), `0` = cellule fantôme (continuation/syncope,
rendue en noire dans le MIDI mais NON contrôlée — limitation pré-existante du rendu 5e).

Arborescence : `<N>voices/<espèce>/vt<vtypes>.{txt,mid}`.

| Voix | Espèce | vtype | Statut | Coût | #sol | Fichier |
|---|---|---|---|---|---|---|
| 2 | 5th | 1 | NO_OUTPUT | — | 0 | — |
| 2 | 5th | 2 | NO_OUTPUT | — | 0 | — |
| 3 | 3rd | 3-1 | TIMEOUT_MAX | 13395.0 | 69 | 3voices/3rd/vt3-1.txt |
| 3 | 4th | 1-1 | NO_OUTPUT | — | 0 | — |
| 3 | 4th | 2-1 | NO_OUTPUT | — | 0 | — |
| 3 | 4th | 2-2 | NO_OUTPUT | — | 0 | — |
| 3 | 4th | 3-1 | NO_OUTPUT | — | 0 | — |
| 3 | 4th | 3-2 | NO_OUTPUT | — | 0 | — |
| 3 | 5th | 1-1 | NO_OUTPUT | — | 0 | — |
| 3 | 5th | 2-1 | NO_OUTPUT | — | 0 | — |
| 3 | 5th | 2-2 | NO_OUTPUT | — | 0 | — |
| 3 | 5th | 3-1 | NO_OUTPUT | — | 0 | — |
| 3 | 5th | 3-2 | NO_OUTPUT | — | 0 | — |
| 2 | 1st | 1 | EXHAUSTIVE | 696.0 | 16 | 2voices/1st/vt1.txt |
| 2 | 1st | 2 | EXHAUSTIVE | 656.0 | 8 | 2voices/1st/vt2.txt |
| 2 | 1st | 3 | EXHAUSTIVE | 656.0 | 15 | 2voices/1st/vt3.txt |
| 2 | 2nd | 1 | EXHAUSTIVE | 776.0 | 47 | 2voices/2nd/vt1.txt |
| 2 | 2nd | 2 | EXHAUSTIVE | 776.0 | 64 | 2voices/2nd/vt2.txt |
| 2 | 2nd | 3 | EXHAUSTIVE | 776.0 | 40 | 2voices/2nd/vt3.txt |
| 2 | 3rd | 1 | TIMEOUT_MAX | 4323.0 | 90 | 2voices/3rd/vt1.txt |
| 2 | 3rd | 2 | TIMEOUT_MAX | 3515.0 | 86 | 2voices/3rd/vt2.txt |
| 2 | 3rd | 3 | TIMEOUT_MAX | 1925.0 | 112 | 2voices/3rd/vt3.txt |
| 2 | 4th | 1 | TIMEOUT_MAX | 1759.0 | 7 | 2voices/4th/vt1.txt |
| 2 | 4th | 2 | NO_OUTPUT | — | 0 | — |
| 2 | 4th | 3 | EXHAUSTIVE | 1109.0 | 11 | 2voices/4th/vt3.txt |
| 2 | 5th | 3 | TIMEOUT_MAX | 2331.0 | 74 | 2voices/5th/vt3.txt |
| 2 | 5th | 4 | TIMEOUT_MAX | 10107.0 | 85 | 2voices/5th/vt4.txt |
| 3 | 1st | 1-1 | TIMEOUT_MAX | 2037.0 | 53 | 3voices/1st/vt1-1.txt |
| 3 | 1st | 1-2 | TIMEOUT_MAX | 1722.0 | 91 | 3voices/1st/vt1-2.txt |
| 3 | 1st | 1-3 | TIMEOUT_MAX | 1431.0 | 118 | 3voices/1st/vt1-3.txt |
| 3 | 1st | 2-1 | TIMEOUT_MAX | 1431.0 | 102 | 3voices/1st/vt2-1.txt |
| 3 | 1st | 2-2 | TIMEOUT_MAX | 1431.0 | 54 | 3voices/1st/vt2-2.txt |
| 3 | 1st | 2-3 | TIMEOUT_MAX | 1278.0 | 69 | 3voices/1st/vt2-3.txt |
| 3 | 1st | 3-1 | EXHAUSTIVE | 1071.0 | 111 | 3voices/1st/vt3-1.txt |
| 3 | 1st | 3-2 | TIMEOUT_MAX | 1071.0 | 95 | 3voices/1st/vt3-2.txt |
| 3 | 1st | 3-3 | TIMEOUT_MAX | 1701.0 | 92 | 3voices/1st/vt3-3.txt |
| 3 | 2nd | 1-1 | TIMEOUT_MAX | 3545.0 | 126 | 3voices/2nd/vt1-1.txt |
| 3 | 2nd | 1-2 | TIMEOUT_MAX | 3416.0 | 224 | 3voices/2nd/vt1-2.txt |
| 3 | 2nd | 1-3 | TIMEOUT_MAX | 3225.0 | 159 | 3voices/2nd/vt1-3.txt |
| 3 | 2nd | 2-1 | TIMEOUT_MAX | 4090.0 | 60 | 3voices/2nd/vt2-1.txt |
| 3 | 2nd | 2-2 | TIMEOUT_MAX | 3768.0 | 129 | 3voices/2nd/vt2-2.txt |
| 3 | 2nd | 2-3 | TIMEOUT_MAX | 3355.0 | 143 | 3voices/2nd/vt2-3.txt |
| 3 | 2nd | 3-1 | TIMEOUT_MAX | 4090.0 | 75 | 3voices/2nd/vt3-1.txt |
| 3 | 2nd | 3-2 | TIMEOUT_MAX | 4231.0 | 95 | 3voices/2nd/vt3-2.txt |
| 3 | 2nd | 3-3 | TIMEOUT_MAX | 4322.0 | 93 | 3voices/2nd/vt3-3.txt |
| 3 | 3rd | 1-1 | NO_SOLUTION | — | 0 | — |
| 3 | 3rd | 1-2 | TIMEOUT_MAX | 8054.0 | 121 | 3voices/3rd/vt1-2.txt |
| 3 | 3rd | 1-3 | TIMEOUT_MAX | 6277.0 | 98 | 3voices/3rd/vt1-3.txt |
| 3 | 3rd | 2-1 | NO_SOLUTION | — | 0 | — |
| 3 | 3rd | 2-2 | TIMEOUT_MAX | 8066.0 | 73 | 3voices/3rd/vt2-2.txt |
| 3 | 3rd | 2-3 | TIMEOUT_MAX | 4267.0 | 85 | 3voices/3rd/vt2-3.txt |
| 3 | 3rd | 3-2 | TIMEOUT_MAX | 11117.0 | 81 | 3voices/3rd/vt3-2.txt |
| 3 | 3rd | 3-3 | TIMEOUT_MAX | 5748.0 | 150 | 3voices/3rd/vt3-3.txt |
| 3 | 4th | 1-2 | TIMEOUT_MAX | 2638.0 | 36 | 3voices/4th/vt1-2.txt |
| 3 | 4th | 1-3 | TIMEOUT_MAX | 2638.0 | 40 | 3voices/4th/vt1-3.txt |
| 3 | 4th | 1-4 | TIMEOUT_MAX | 2638.0 | 43 | 3voices/4th/vt1-4.txt |
| 3 | 4th | 2-3 | TIMEOUT_MAX | 1854.0 | 59 | 3voices/4th/vt2-3.txt |
| 3 | 4th | 2-4 | TIMEOUT_MAX | 1991.0 | 59 | 3voices/4th/vt2-4.txt |
| 3 | 4th | 3-3 | TIMEOUT_MAX | 1720.0 | 53 | 3voices/4th/vt3-3.txt |
| 3 | 4th | 3-4 | TIMEOUT_MAX | 1720.0 | 30 | 3voices/4th/vt3-4.txt |
| 3 | 5th | 1-2 | TIMEOUT_MAX | 13816.0 | 9 | 3voices/5th/vt1-2.txt |
| 3 | 5th | 1-3 | TIMEOUT_MAX | 18955.0 | 13 | 3voices/5th/vt1-3.txt |
| 3 | 5th | 1-4 | TIMEOUT_MAX | 17956.0 | 16 | 3voices/5th/vt1-4.txt |
| 3 | 5th | 2-3 | TIMEOUT_MAX | 22681.0 | 24 | 3voices/5th/vt2-3.txt |
| 3 | 5th | 2-4 | TIMEOUT_MAX | 23919.0 | 12 | 3voices/5th/vt2-4.txt |
| 3 | 5th | 3-3 | TIMEOUT_MAX | 47254.0 | 4 | 3voices/5th/vt3-3.txt |
| 3 | 5th | 3-4 | TIMEOUT_MAX | 23568.0 | 17 | 3voices/5th/vt3-4.txt |

_Build terminé : 2026-05-25 22:39:44._
