#include "TCanvas.h"
#include "TFile.h"
#include "TH1.h"
#include "TLegend.h"
#include "TROOT.h"
#include "TString.h"
#include "TStyle.h"
#include "TSystem.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {
struct Cat {
  const char *name;
  const char *label;
  int color;
  int lineStyle;
};
const std::vector<Cat> kCats = {
    {"all", "all final-state particles", kBlack, 1},
    {"ch", "charged particles", kGray + 2, 1},
    {"pip", "#pi^{+}", kBlue + 1, 1},
    {"pim", "#pi^{-}", kAzure + 7, 2},
    {"Kp", "K^{+}", kRed + 1, 1},
    {"Km", "K^{-}", kOrange + 7, 2},
    {"p", "p", kGreen + 2, 1},
    {"pbar", "#bar{p}", kTeal - 5, 2},
};
struct Var {
  const char *key;
  const char *title;
  const char *yPerEvent;
};
const std::vector<Var> kVars = {
    {"N", "Multiplicity N", "Fraction of events"},
    {"Pt", "Transverse momentum p_{T}", "Particles per event (bin width 0.1 GeV)"},
    {"Eta", "Pseudorapidity #eta", "Particles per event (bin width 0.1)"},
    {"Phi", "Azimuthal angle #phi", "Particles per event (bin width #pi/64 rad)"},
};

TFile *gIn = nullptr;
TString gOutDir;
TString gAllPdf;

const Cat *findCat(const char *name) {
  for (const auto &cat : kCats) {
    if (TString(cat.name) == name)
      return &cat;
  }
  return nullptr;
}

TH1 *getHist(const TString &name) {
  auto *h = dynamic_cast<TH1 *>(gIn->Get(name));
  if (!h)
    std::cerr << "draw_plots: missing histogram " << name << std::endl;
  return h;
}

void save(TCanvas *c, const TString &name) {
  c->SaveAs(gOutDir + "/png/" + name + ".png");
  c->SaveAs(gOutDir + "/pdf/" + name + ".pdf");
  c->SaveAs(gAllPdf, "Title:" + name);
}

TH1 *perEvent(TH1 *h, const Var &v, double nEv) {
  auto *c = static_cast<TH1 *>(h->Clone(TString(h->GetName()) + "_norm"));
  c->SetDirectory(nullptr);
  c->Scale(1.0 / nEv);
  c->GetYaxis()->SetTitle(v.yPerEvent);
  c->GetYaxis()->SetTitleSize(0.04);
  return c;
}

double lastFilledEdge(const std::vector<TH1 *> &hs) {
  int last = 1;
  for (auto *h : hs)
    last = std::max(last, h->FindLastBinAbove(0));
  return hs.front()->GetXaxis()->GetBinUpEdge(last);
}

void drawSingle(const Cat &cat, const Var &v) {
  const TString name = TString::Format("h%s_%s", v.key, cat.name);
  TH1 *h = getHist(name);
  if (!h)
    return;
  auto *c = new TCanvas("c_" + name, name, 800, 600);
  h->SetLineColor(cat.color);
  h->SetLineWidth(2);
  h->SetTitle(TString::Format("%s of %s", v.title, cat.label));

  const TString key = v.key;
  const bool logY = (key == "Pt" || key == "N");
  if (logY) {
    c->SetLogy();
    h->SetMinimum(0.5);
  } else {
    h->SetMinimum(0.);
  }
  h->Draw("HIST");

  save(c, name);
  delete c;
}

void drawCompare(const TString &tag, const TString &title,
                 const std::vector<const char *> &catNames, const Var &v,
                 double nEv) {
  std::vector<TH1 *> hs;
  std::vector<const Cat *> cs;
  for (const char *cn : catNames) {
    const Cat *cat = findCat(cn);
    TH1 *h = getHist(TString::Format("h%s_%s", v.key, cn));
    if (!cat || !h)
      return;
    hs.push_back(perEvent(h, v, nEv));
    cs.push_back(cat);
  }

  const TString key = v.key;
  const TString name = TString::Format("cmp_%s_%s", v.key, tag.Data());
  auto *c = new TCanvas("c_" + name, name, 800, 600);
  const bool logY = (key == "Pt" || key == "N");
  if (logY)
    c->SetLogy();

  double ymax = 0., ymin = 1e30;
  for (auto *h : hs) {
    ymax = std::max(ymax, h->GetMaximum());
    const double m = h->GetMinimum(0.);
    if (m > 0)
      ymin = std::min(ymin, m);
  }

  auto *leg = new TLegend(0.45, 0.68, 0.88, 0.88);
  leg->SetBorderSize(0);
  leg->SetFillStyle(0);
  leg->SetTextSize(0.035);

  for (size_t i = 0; i < hs.size(); ++i) {
    TH1 *h = hs[i];
    const double mean = h->GetMean();
    h->SetStats(false);
    h->SetLineColor(cs[i]->color);
    h->SetLineStyle(cs[i]->lineStyle);
    h->SetLineWidth(2);
    if (i == 0) {
      h->SetTitle(TString(v.title) + ": " + title);
      h->SetMaximum(logY ? ymax * 20. : ymax * 1.4);
      h->SetMinimum(logY ? std::max(ymin * 0.5, ymax * 1e-7) : 0.);
      if (key == "N")
        h->GetXaxis()->SetRangeUser(0., lastFilledEdge(hs));
      h->Draw("HIST");
    } else {
      h->Draw("HIST SAME");
    }
    TString entry = cs[i]->label;
    if (key == "N")
      entry += TString::Format("  #LTN#GT = %.2f", mean);
    if (key == "Pt")
      entry += TString::Format("  #LTp_{T}#GT = %.3f GeV", mean);
    leg->AddEntry(h, entry, "l");
  }
  leg->Draw();

  save(c, name);
  delete c;
  for (auto *h : hs)
    delete h;
}

} // namespace

void draw_plots(const char *inName = "analysis.root",
                const char *outDir = "plots") {
  gROOT->SetBatch(true);
  gStyle->SetOptStat("emr");
  gStyle->SetTitleFontSize(0.05);
  gStyle->SetPadLeftMargin(0.13);
  gStyle->SetPadBottomMargin(0.12);
  gStyle->SetTitleOffset(1.2, "Y");
  TH1::AddDirectory(false);

  gIn = TFile::Open(inName, "READ");
  if (!gIn || gIn->IsZombie()) {
    std::cerr << "draw_plots: cannot open " << inName << "\n";
    return;
  }

  gOutDir = outDir;
  gSystem->mkdir(gOutDir + "/png", true);
  gSystem->mkdir(gOutDir + "/pdf", true);
  gAllPdf = gOutDir + "/all_plots.pdf";

  TH1 *hNall = getHist("hN_all");
  if (!hNall)
    return;
  const double nEv = hNall->GetEntries();
  std::cout << "draw_plots: " << nEv << " events\n";

  TCanvas blank("blank", "", 800, 600);
  blank.Print(gAllPdf + "[");

  for (const auto &v : kVars)
    for (const auto &cat : kCats)
      drawSingle(cat, v);

  for (const auto &v : kVars) {
    drawCompare("pions", "#pi^{+} vs #pi^{-}", {"pip", "pim"}, v, nEv);
    drawCompare("kaons", "K^{+} vs K^{-}", {"Kp", "Km"}, v, nEv);
    drawCompare("protons", "p vs #bar{p}", {"p", "pbar"}, v, nEv);
    drawCompare("species", "charged particles, #pi^{+}, K^{+}, p",
                {"ch", "pip", "Kp", "p"}, v, nEv);
  }

  blank.Print(gAllPdf + "]");
  gIn->Close();
  std::cout << "draw_plots: done, output in " << gOutDir << "/\n";
}