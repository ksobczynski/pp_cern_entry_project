# Rozkłady cząstek w zderzeniach pp przy √s = 13.6 TeV (Pythia 8 + ROOT)

Generacja 100 000 zderzeń pp typu *minimum bias* w Pythii 8,
analiza cząstek stanu końcowego w C++/ROOT (krotność, pT, η, φ dla 8 kategorii
cząstek) i makro ROOT rysujące wykresy do PNG i PDF.

## Wymagania

- ROOT 6 (`root-config` w PATH)
- Pythia 8 (`pythia8-config` w PATH)
- kompilator C++ i `make`

Testowane z ROOT 6.40.02 i Pythią 8.312 na macOS (Apple Silicon).

Kod jest zgodny z C++17. Domyślnie kompilowany jest w standardzie, w którym
zbudowany jest ROOT z conda-forge (`gnu++20`), bo ROOT wymaga zgodności
standardów. Z ROOT-em zbudowanym w C++17: `make CXXSTD=c++17`. Build w C++17 daje identyczne histogramy.

Pakiety można zainstalować jednym poleceniem przez condę
(`environment.yml` zawiera dokładnie te wersje):

```bash
conda env create -f environment.yml && conda activate pp
```

## Szybki start

```bash
make run    # kompilacja → generacja → analiza → wykresy
```

`make run` tworzy:

- `events.root`: cząstki stanu końcowego ze 100 000 zderzeń (~235 MB),
- `analysis.root`: 32 histogramy,
- `plots/`: wykresy PNG i PDF oraz zbiorczy `plots/all_plots.pdf`.

## Cele `make`

| polecenie | co robi |
|---|---|
| `make` | kompiluje `bin/generate` i `bin/analysis` |
| `make run` | cały łańcuch; wykonuje tylko kroki, których wynik jest nieaktualny |
| `make run EVENTS=1000` | to samo na mniejszej próbce (szybki test) |
| `make run SEED=42` | inny seed generatora |
| `make clean` | usuwa skompilowane programy (`bin/`) |
| `make distclean` | usuwa wszystko, co da się odtworzyć: `bin/`, `events.root`, `analysis.root`, `plots/` |

`make` porównuje daty plików, a nie wartości zmiennych. Po zmianie `EVENTS`
albo `SEED` trzeba najpierw wykonać `make distclean`.

## Uruchamianie krok po kroku

```bash
make                                         # kompilacja
./bin/generate 100000 123456 events.root     # [liczba zdarzeń] [seed] [plik wyjściowy]
./bin/analysis events.root analysis.root     # [plik wejściowy] [plik wyjściowy]
root -l -b -q draw_plots.C                   # wykresy z analysis.root do plots/
```

Wszystkie argumenty są opcjonalne; powyżej podano wartości domyślne.
Makro przyjmuje też argumenty: `root -l -b -q 'draw_plots.C("analysis.root","plots")'`.

## Struktura projektu

```
.
├── generation/generate.cpp   # generator: Pythia 8 → events.root (TTree)
├── analysis/analysis.cpp     # analiza: events.root → analysis.root (histogramy)
├── draw_plots.C              # makro ROOT: analysis.root → plots/
├── Makefile                  # kompilacja i cały łańcuch (make run)
├── environment.yml           # środowisko conda (ROOT, Pythia, make)
├── analysis.root             # wynik analizy
├── presentation.pdf          # prezentacja ze wszystkimi wykresami
├── README.md
└── plots/                    # wykresy
    ├── png/
    ├── pdf/
    └── all_plots.pdf
```

## Co robi każdy etap

### 1. Generacja (`generation/generate.cpp`)

- zderzenia p + p przy `Beams:eCM = 13600` GeV,
- `SoftQCD:inelastic = on`, czyli *minimum bias*: zderzenia niedyfrakcyjne
  oraz pojedynczo i podwójnie dyfrakcyjne, bez rozpraszania elastycznego,
- stały seed (`Random:setSeed = on`, domyślnie 123456), dzięki czemu każde
  uruchomienie generuje dokładnie te same zdarzenia,
- zapisywane są **tylko cząstki stanu końcowego** (`Particle::isFinal()`).

Plik `events.root` zawiera drzewo `events`, w którym jeden wpis to jedno zderzenie:

| gałąź | typ | opis |
|---|---|---|
| `code` | `int` | kod procesu Pythii (101 ND, 103/104 SD, 105 DD) |
| `id` | `vector<int>` | kod PDG cząstki |
| `charge3` | `vector<short>` | 3 × ładunek (konwencja Pythii `chargeType()`) |
| `pt`, `eta`, `phi` | `vector<float>` | pęd poprzeczny [GeV], pseudorapidity, kąt azymutalny [rad] |

### 2. Analiza (`analysis/analysis.cpp`)

Dla 8 kategorii cząstek tworzy po 4 histogramy (razem 32) i zapisuje je do `analysis.root`:

| kategoria | wybór |
|---|---|
| `all` | wszystkie cząstki stanu końcowego |
| `ch` | cząstki naładowane (ładunek ≠ 0) |
| `pip`, `pim` | π⁺ (PDG 211), π⁻ (−211) |
| `Kp`, `Km` | K⁺ (321), K⁻ (−321) |
| `p`, `pbar` | p (2212), p̄ (−2212) |

| histogram | wielkość | binowanie | wypełnienie |
|---|---|---|---|
| `hN_<kat>` | krotność N | 400 binów, 0–400 | raz na zderzenie |
| `hPt_<kat>` | pT [GeV] | 100 binów, 0–10 | raz na cząstkę |
| `hEta_<kat>` | η | 100 binów, −5–5 | raz na cząstkę |
| `hPhi_<kat>` | φ [rad] | 64 biny, 0–π | raz na cząstkę |

### 3. Wykresy (`draw_plots.C`)

- **32 wykresy indywidualne**: każdy histogram na osobnym canvasie, z tytułem
  i podpisami osi. pT i N mają oś Y w skali logarytmicznej dla lepszej widoczności.
- **16 wykresów porównawczych** (`cmp_*`) z legendą:
  cząstka vs antycząstka (π⁺/π⁻, K⁺/K⁻, p/p̄) oraz porównanie gatunków
  (naładowane, π⁺, K⁺, p) dla każdej z 4 wielkości. Histogramy są podzielone
  przez liczbę zderzeń: dla N oś Y to ułamek zderzeń, dla pT, η i φ średnia
  liczba cząstek na zderzenie w danym binie. Legenda podaje ⟨N⟩ albo ⟨pT⟩.
- każdy wykres zapisany jako PNG (`plots/png/`) i PDF (`plots/pdf/`),
  wszystkie razem w `plots/all_plots.pdf`.

## Uwagi

**φ złożone do zakresu 0–π przez |φ|.** Pythia zwraca φ ∈ (−π, π], a specyfikacja
wymaga histogramu 0–π. Użyto złożenia |φ|, które nie zmienia kształtu rozkładu.

**„Mean” w ramce statystyk `hN_all` jest zaniżony.** Zakres 0–400 jest za mały dla
kategorii `all` (zawiera też fotony): 12.5% zderzeń ma ponad 400 cząstek stanu
końcowego i trafia do binu overflow. ROOT liczy średnią bez overflow, więc ramka
pokazuje ~145, a prawdziwa średnia to ⟨N_all⟩ ≈ 192. Dla `ch` overflow wynosi
0.1% (ramka 91.4, prawdziwa średnia 91.7), dla π, K i p jest zerowy, więc tam
„Mean” jest dokładny.


## Odtwarzalność

Te same wersje pakietów (ROOT 6.40.02, Pythia 8.312) i ten sam seed dają te same
zdarzenia w `events.root`, a w konsekwencji te same histogramy i wykresy
(same pliki `.root` różnią się tylko znacznikami czasu). Aby odtworzyć
wszystkie wyniki od zera:

```bash
make distclean && make run
```

## Autor
Konstanty Sobczyński