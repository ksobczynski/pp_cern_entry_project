
CXXSTD   ?= gnu++20
CXXFLAGS := -O2 -Wall -std=$(CXXSTD)
PYTHIA_FLAGS := $(shell pythia8-config --cxxflags --libs)
ROOT_FLAGS   := $(shell root-config --cflags --libs)

EVENTS ?= 100000
SEED   ?= 123456

all: bin/generate bin/analysis

bin/generate: generation/generate.cpp | bin
	$(CXX) $< -o $@ $(PYTHIA_FLAGS) $(ROOT_FLAGS) $(LDFLAGS) $(CXXFLAGS)

bin/analysis: analysis/analysis.cpp | bin
	$(CXX) $< -o $@ $(ROOT_FLAGS) $(LDFLAGS) $(CXXFLAGS)

bin:
	mkdir -p bin

events.root: bin/generate
	./bin/generate $(EVENTS) $(SEED) $@

analysis.root: bin/analysis events.root
	./bin/analysis events.root $@

plots/all_plots.pdf: draw_plots.C analysis.root
	root -l -b -q draw_plots.C

run: plots/all_plots.pdf

clean:
	rm -rf bin

distclean: clean
	rm -rf events.root analysis.root plots

.PHONY: all clean run distclean
