#include "Pythia8/Pythia.h"
#include "TFile.h"
#include "TTree.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace Pythia8;

int main(int argc, char *argv[]) {

  const long nEvents = (argc > 1) ? std::atol(argv[1]) : 100000;
  const int seed = (argc > 2) ? std::atoi(argv[2]) : 123456;
  const std::string outName = (argc > 3) ? argv[3] : "events.root";

  Pythia pythia;
  pythia.readString("Beams:eCM = 13600.");
  pythia.readString("SoftQCD:inelastic = on");
  pythia.readString("Random:setSeed = on");
  pythia.readString("Random:seed = " + std::to_string(seed));
  pythia.readString("Next:numberCount = 10000");
  pythia.readString("Next:numberShowEvent = 0");

  if (!pythia.init()) {
    std::cerr << "Error initializing Pythia." << std::endl;
    return 1;
  }

  TFile *fout = TFile::Open(outName.c_str(), "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "Error creating output file: " << outName << std::endl;
    return 1;
  }

  TTree *tree =
      new TTree("events", "Final-state particles, pp @ 13.6 TeV, SoftQCD:inelastic");
  int code = 0;
  std::vector<int> id;
  std::vector<short> charge3;
  std::vector<float> pt, eta, phi;

  tree->Branch("code", &code);
  tree->Branch("id", &id);
  tree->Branch("charge3", &charge3);
  tree->Branch("pt", &pt);
  tree->Branch("eta", &eta);
  tree->Branch("phi", &phi);

  long nFailed = 0;
  for (long iEvent = 0; iEvent < nEvents; ++iEvent) {
    if (!pythia.next()) {
      ++nFailed;
      continue;
    }

    id.clear();
    charge3.clear();
    pt.clear();
    eta.clear();
    phi.clear();

    for (int i = 0; i < pythia.event.size(); ++i) {
      const Particle &p = pythia.event[i];
      if (!p.isFinal())
        continue;
      id.push_back(p.id());
      charge3.push_back(static_cast<short>(p.chargeType()));
      pt.push_back(static_cast<float>(p.pT()));
      eta.push_back(static_cast<float>(p.eta()));
      phi.push_back(static_cast<float>(p.phi()));
    }

    code = pythia.info.code();
    tree->Fill();
  }

  const long nSaved = tree->GetEntries();
  fout->cd();
  tree->Write();
  fout->Close();
  delete fout;

  pythia.stat();
  std::cout << "Saved " << nSaved << " events to " << outName
            << " (failed next(): " << nFailed << ")\n";
  return 0;
}