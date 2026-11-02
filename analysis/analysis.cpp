#include "TFile.h"
#include "TH1D.h"
#include "TMath.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct Category {
  std::string name;
  std::string label;
  std::function<bool(int id, int charge3)> select;

  TH1D *hN = nullptr;
  TH1D *hPt = nullptr;
  TH1D *hEta = nullptr;
  TH1D *hPhi = nullptr;
  int count = 0;
};

int main(int argc, char *argv[]) {
  const std::string inName = (argc > 1) ? argv[1] : "events.root";
  const std::string outName = (argc > 2) ? argv[2] : "analysis.root";

  TFile *fin = TFile::Open(inName.c_str(), "READ");
  if (!fin || fin->IsZombie()) {
    std::cerr << "Error opening input file: " << inName << std::endl;
    return 1;
  }
  TTreeReader reader("events", fin);
  TTreeReaderValue<std::vector<int>> id(reader, "id");
  TTreeReaderValue<std::vector<short>> charge3(reader, "charge3");
  TTreeReaderValue<std::vector<float>> pt(reader, "pt");
  TTreeReaderValue<std::vector<float>> eta(reader, "eta");
  TTreeReaderValue<std::vector<float>> phi(reader, "phi");

  std::vector<Category> cats = {
      {"all", "all final-state", [](int, int) { return true; }},
      {"ch", "charged", [](int, int q3) { return q3 != 0; }},
      {"pip", "#pi^{+}", [](int id, int) { return id == 211; }},
      {"pim", "#pi^{-}", [](int id, int) { return id == -211; }},
      {"Kp", "K^{+}", [](int id, int) { return id == 321; }},
      {"Km", "K^{-}", [](int id, int) { return id == -321; }},
      {"p", "p", [](int id, int) { return id == 2212; }},
      {"pbar", "#bar{p}", [](int id, int) { return id == -2212; }},
  };
  TFile *fout = TFile::Open(outName.c_str(), "RECREATE");
  if (!fout || fout->IsZombie()) {
    std::cerr << "Error creating output file: " << outName << std::endl;
    return 1;
  }
  TH1::SetDefaultSumw2();

  for (auto &c : cats) {
    const std::string n = c.name, l = c.label;
    c.hN = new TH1D(("hN_" + n).c_str(), (l + ";Multiplicity N;Events").c_str(),
                    400, 0., 400.);
    c.hPt = new TH1D(("hPt_" + n).c_str(),
                     (l + ";p_{T} [GeV];Particles").c_str(), 100, 0., 10.);
    c.hEta = new TH1D(("hEta_" + n).c_str(), (l + ";#eta;Particles").c_str(),
                      100, -5., 5.);
    c.hPhi =
        new TH1D(("hPhi_" + n).c_str(), (l + ";#phi [rad];Particles").c_str(),
                 64, 0., TMath::Pi());
  }

  long nEvents = 0;
  while (reader.Next()) {
    ++nEvents;
    for (auto &c : cats)
      c.count = 0;
    const size_t nPart = id->size();
    for (size_t i = 0; i < nPart; ++i) {
      const int pid = (*id)[i];
      const int q3 = (*charge3)[i];
      const double aphi = std::min(std::fabs(static_cast<double>((*phi)[i])),
                                   TMath::Pi() - 1e-9);

      for (auto &c : cats) {
        if (!c.select(pid, q3))
          continue;
        ++c.count;
        c.hPt->Fill((*pt)[i]);
        c.hEta->Fill((*eta)[i]);
        c.hPhi->Fill(aphi);
      }
    }
    for (auto &c : cats)
      c.hN->Fill(c.count);
  }

  if (reader.GetEntryStatus() != TTreeReader::kEntryBeyondEnd) {
    std::cerr << "Error reading TTree: " << reader.GetCurrentEntry()
              << std::endl;
    return 1;
  }

  fout->cd();
  fout->Write();
  fout->Close();
  delete fout;
  fin->Close();
  delete fin;
  std::cout << "Processed " << nEvents << " events from " << inName
            << ", saved histograms to " << outName << std::endl;
  return 0;
}