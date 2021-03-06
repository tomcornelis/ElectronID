#include <iostream>
#include <fstream>
#include "TSystem.h"
#include "TStyle.h"
#include "TF1.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TFile.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TTree.h"
#include "TList.h"
#include "TString.h"
#include "TLatex.h"
#include "TLorentzVector.h"
#include "TLegend.h"
#include "TInterpreter.h"
#include "TCut.h"
#include "TBenchmark.h"
#include <signal.h>
#include "TMath.h"
#include <cassert>
#include <TROOT.h>
#include <vector>

#include "OptimizationConstants.hh"
#include <stdio.h>
#include <stdlib.h>

enum MatchType  {MATCH_TRUE, MATCH_FAKE, MATCH_ANY};
enum SampleType {SAMPLE_UNDEF, SAMPLE_DY, SAMPLE_TT, SAMPLE_GJ, SAMPLE_DoubleEle1to300, SAMPLE_DoubleEle300to6500};
enum EtaRegion  {ETA_EB, ETA_EE, ETA_FULL};

const float C_e_barrel   = 1.12;
const float C_rho_barrel = 0.0368;
const float C_e_endcap   = 2.35;
const float C_rho_endcap = 0.201;


const TString tagDir = "2019-08-23";
const TString getFileName(TString type){
  return "/user/tomc/eleIdTuning/tuples/" + type + ".root";
}

// Preselection cuts: must match or be looser than
// cuts in OptimizatioConstants.hh
const float ptMin = 20;
const float etaMax = 2.5;
const float dzMax = 1.0;
// Note: passConversionVeto is also applied
const float boundaryEBEE = 1.479;

//====================================================
const bool talkativeRegime = true;
const bool smallEventCount = false;
const int maxEventsSmall = 20000000;
// output dir of tuples

// Tree name input
const TString treeName = "ntupler/ElectronTree";
// File and histogram with kinematic weights
const TString fileNameWeights = "kinematicWeights.root";
const TString histNameWeights = "hKinematicWeights";

// Effective areas for electrons derived by Ilya for Fall17
//  https://indico.cern.ch/event/662749/contributions/2763091/attachments/1545124/2424854/talk_electron_ID_fall17.pdf
namespace EffectiveAreas {
  const int nEtaBins = 7;
  const float etaBinLimits[nEtaBins+1] = {
    0.0, 1.0, 1.479, 2.0,
    2.2, 2.3, 2.4, 2.5
  };
  const float effectiveAreaValues[nEtaBins] = {
    0.0978,
    0.1033,
    0.0552,
    0.0247,
    0.0255,
    0.0208,
    0.0960,
  };
}


void bazinga (std::string mes){
  if(talkativeRegime) std::cout<<"\n"<<mes<<std::endl;
}

// Forward declarations
float   findKinematicWeight(TH2D *hist, float pt, float etaSC);
bool    passPreselection(int isTrue, float pt, float eta, int passConversionVeto, float dz, MatchType matchType, EtaRegion etaRegion);
TString eventCountString();
void    drawProgressBar(float progress);

float getEntries(TString inputFileName){
  TFile *file   = new TFile(inputFileName);
  TTree *tree   = (TTree*)file->Get(treeName);
  float entries = tree->GetEntries();
  delete tree;
  delete file;
  return entries;
}

//
// Main program
//
void convert_EventStrNtuple_To_FlatNtuple(SampleType sample, MatchType matchType, EtaRegion etaRegion){
  int N_1to300    = -1;
  int N_300to6500 = -1;
  if(sample == SAMPLE_DoubleEle1to300 or sample == SAMPLE_DoubleEle300to6500){
    N_1to300    = getEntries(getFileName("DoubleEleFlat1to300"));
    N_300to6500 = getEntries(getFileName("DoubleEleFlat300to6500"));
  }


  bazinga("Start main function");

  // General settings in case one adds drawing of histograms etc
  gStyle->SetOptFit();
  gStyle->SetOptStat(0);

  // ======================================================================
  // Set up input/output files and find/create trees
  // ======================================================================
  bazinga("Set up input/output files\n");

  // Input/output file names
  TString inputFileName = "";
  TString flatNtupleFileNameBase = "Undefined";
  if(sample == SAMPLE_DY)                        inputFileName = "DY";
  else if( sample == SAMPLE_TT )                 inputFileName = "TT";
  else if( sample == SAMPLE_GJ )                 inputFileName = "GJ";
  else if( sample == SAMPLE_DoubleEle1to300 )    inputFileName = "DoubleEleFlat1to300";
  else if( sample == SAMPLE_DoubleEle300to6500 ) inputFileName = "DoubleEleFlat300to6500";
  else {
    printf("Unknown sample requested\n");
    assert(0);
  }

  flatNtupleFileNameBase = inputFileName + "_flat_ntuple";
  inputFileName          = getFileName(inputFileName);

  TFile *inputFile = new TFile(inputFileName);
  if(!inputFile ){
    printf("Failed to open input file %s\n", inputFileName.Data());
    assert(0);
  }
  TTree *treeIn = (TTree*)inputFile->Get(treeName);
  if( !treeIn ){
    printf("Failed to find tree %s in the file %s\n", treeName.Data(), inputFileName.Data());
    assert(0);
  }

  TString flatNtupleFileNameTruth = "";
  if(matchType == MATCH_TRUE)      flatNtupleFileNameTruth = "_true";
  else if(matchType == MATCH_FAKE) flatNtupleFileNameTruth = "_fake";
  else if(matchType == MATCH_ANY)  flatNtupleFileNameTruth = "_trueAndFake";

  TString flatNtupleFileNameEtas = "";
  if(etaRegion == ETA_EB)        flatNtupleFileNameEtas = "_barrel";
  else if(etaRegion == ETA_EE)   flatNtupleFileNameEtas = "_endcap";
  else if(etaRegion == ETA_FULL) flatNtupleFileNameEtas = "_alleta";

  TString flatNtupleFileNameEvents = eventCountString();
  TString flatNtupleFileNameEnding = ".root";

  system("mkdir -p " + tagDir);
  TString flatNtupleFileName = tagDir + "/" + flatNtupleFileNameBase + flatNtupleFileNameTruth
    + flatNtupleFileNameEtas + flatNtupleFileNameEvents + flatNtupleFileNameEnding;


  // Open output file
  TFile *fileOut = new TFile(flatNtupleFileName, "recreate");
  TTree *treeOut = new TTree("electronTree","Flat_ntuple");

  // ======================================================================
  // Set up all variables and branches for the input tree
  // ======================================================================
  //
  // Declare variables and branches
  // Event-level variables:
  int eleNEle; // the number of reconstructed electrons in the event
  float eleRho;
  float genWeight;
  int nPV;
  // Per-eletron variables
  // Kinematics
  std::vector <float> *elePt = 0;         // electron PT
  std::vector <float> *eleGenPt = 0;         // electron PT
  std::vector <float> *eleESC = 0;        // supercluser energy
  std::vector <float> *eleEtaSC = 0;      // supercluser eta
  std::vector <float> *elePhiSC = 0;      // supercluser phi
  // Variables for analysis
  std::vector <float> *isoChargedHadrons = 0;
  std::vector <float> *isoNeutralHadrons = 0;
  std::vector <float> *eleIsoPhotons = 0;
  std::vector <int> *eleIsTrueElectron = 0;
  // Other vars
  // Impact parameters
  std::vector <float> *eleD0 = 0;      // r-phi plane impact parameter
  std::vector <float> *eleDZ = 0;      // r-z plane impact parameter
  // Matching track-supercluster
  std::vector <float> *eleDEtaSeed = 0;  // deltaEtaIn
  std::vector <float> *eleDPhiIn = 0;  // deltaPhiIn
  // Misc ID variables
  std::vector <float> *eleHoverE = 0;  // H/E
  std::vector <float> *eleFull5x5SigmaIEtaIEta = 0;
  std::vector <float> *eleOOEMOOP = 0; // |1/E - 1/p|
  // Conversion rejection
  std::vector <int> *eleExpectedMissingInnerHits = 0;
  std::vector <int> *electronPassConversionVeto = 0;

  // Declare branches
  TBranch *b_eleNEle = 0;
  TBranch *b_nPV = 0;
  TBranch *b_genWeight = 0;
  TBranch *b_eleRho = 0;
  TBranch *b_elePt = 0;
  TBranch *b_eleGenPt = 0;
  TBranch *b_eleESC = 0;
  TBranch *b_eleEtaSC = 0;
  TBranch *b_elePhiSC = 0;
  TBranch *b_isoChargedHadrons = 0;
  TBranch *b_isoNeutralHadrons = 0;
  TBranch *b_eleIsoPhotons = 0;
  TBranch *b_eleIsTrueElectron;

  // Other vars
  TBranch *b_eleD0 = 0;
  TBranch *b_eleDZ = 0;
  TBranch *b_eleDEtaSeed = 0;
  TBranch *b_eleDPhiIn = 0;
  TBranch *b_eleHoverE = 0;
  TBranch *b_eleFull5x5SigmaIEtaIEta = 0;
  TBranch *b_eleOOEMOOP = 0;
  TBranch *b_eleExpectedMissingInnerHits = 0;
  TBranch *b_electronPassConversionVeto = 0;

  // Connect variables and branches to the tree with the data
  treeIn->SetBranchAddress("nEle",                     &eleNEle,                     &b_eleNEle);
  treeIn->SetBranchAddress("nPV",                      &nPV,                         &b_nPV);
  treeIn->SetBranchAddress("genWeight",                &genWeight,                   &b_genWeight);
  treeIn->SetBranchAddress("rho",                      &eleRho,                      &b_eleRho);
  treeIn->SetBranchAddress("pt",                       &elePt,                       &b_elePt);
  treeIn->SetBranchAddress("genPt",                    &eleGenPt,                    &b_eleGenPt);
  treeIn->SetBranchAddress("eSC",                      &eleESC,                      &b_eleESC);
  treeIn->SetBranchAddress("etaSC",                    &eleEtaSC,                    &b_eleEtaSC);
  treeIn->SetBranchAddress("phiSC",                    &elePhiSC,                    &b_elePhiSC);
  treeIn->SetBranchAddress("isoChargedHadrons",        &isoChargedHadrons,           &b_isoChargedHadrons);
  treeIn->SetBranchAddress("isoNeutralHadrons",        &isoNeutralHadrons,           &b_isoNeutralHadrons);
  treeIn->SetBranchAddress("isoPhotons",               &eleIsoPhotons,               &b_eleIsoPhotons);
  treeIn->SetBranchAddress("isTrue",                   &eleIsTrueElectron,           &b_eleIsTrueElectron);
  treeIn->SetBranchAddress("d0",                       &eleD0,                       &b_eleD0);
  treeIn->SetBranchAddress("dz",                       &eleDZ,                       &b_eleDZ);
  treeIn->SetBranchAddress("dEtaSeed",                 &eleDEtaSeed,                 &b_eleDEtaSeed);
  treeIn->SetBranchAddress("dPhiIn",                   &eleDPhiIn,                   &b_eleDPhiIn);
  treeIn->SetBranchAddress("hOverE",                   &eleHoverE,                   &b_eleHoverE);
  treeIn->SetBranchAddress("full5x5_sigmaIetaIeta",    &eleFull5x5SigmaIEtaIEta,     &b_eleFull5x5SigmaIEtaIEta);
  treeIn->SetBranchAddress("ooEmooP",                  &eleOOEMOOP,                  &b_eleOOEMOOP);
  treeIn->SetBranchAddress("expectedMissingInnerHits", &eleExpectedMissingInnerHits, &b_eleExpectedMissingInnerHits);
  treeIn->SetBranchAddress("passConversionVeto",       &electronPassConversionVeto,  &b_electronPassConversionVeto);


  //
  // Get the histogram with kinematic weights
  //
  TFile *fweights = new TFile(tagDir + "/" + fileNameWeights);
  if( !fweights or fweights->IsZombie() ){
    std::cout << ">>>>" << std::endl;
    system("./compileAndRun.sh computeKinematicWeights " + tagDir);
    fweights = new TFile(tagDir + "/" + fileNameWeights);
  }
  TH2D *hKinematicWeights = (TH2D*)fweights->Get("hKinematicWeights");
  if( !hKinematicWeights ){
    printf("The histogram %s is not found in file %s\n", histNameWeights.Data(), tagDir + "/" + fileNameWeights.Data());
    assert(0);
  }

  // ======================================================================
  // Set up all variables and branches for the input tree
  // ======================================================================

  //bazinga("Set up output tree\n");
  fileOut->cd();  //?????????

  Int_t nPV_ =0;        // number of reconsrtucted primary vertices

  Float_t gweight_ = 0.0;      // gen Weight
  Float_t kweight_ = 0.0;      // kinematic Weight

  // all electron variables
  Float_t pt_ = 0.0;
  Float_t genPt_ = 0.0;
  Float_t rho_ = 0.0;
  Float_t etaSC_= 0.0;
  Float_t eSC_= 0.0;
  Float_t dEtaSeed_= 0.0;
  Float_t dPhiIn_= 0.0;
  Float_t hOverE_= 0.0;
  Float_t hOverEscaled_= 0.0;
  Float_t full5x5_sigmaIetaIeta_= 0.0;
  Float_t isoChargedHadrons_= 0.0;
  Float_t isoNeutralHadrons_= 0.0;
  Float_t isoPhotons_= 0.0;
  Float_t isoChargedFromPU_= 0.0;
  Float_t relIsoWithEA_= 0.0;
  Float_t relIsoWithDBeta_= 0.0;
  Float_t ooEmooP_= 0.0;
  Float_t d0_= 0.0;
  Float_t dz_= 0.0;
  Int_t   expectedMissingInnerHits_= 0;
  Int_t   passConversionVeto_= 0;
  Int_t   isTrueEle_= 0;

  treeOut->Branch("nPV",                      &nPV_,                      "nPV/I");

  treeOut->Branch("genWeight",                &gweight_,                  "gweight/F");
  treeOut->Branch("kinWeight",                &kweight_,                  "kweight/F");

  treeOut->Branch("rho",                      &rho_,                      "rho/F");
  treeOut->Branch("pt" ,                      &pt_,                       "pt/F");
  treeOut->Branch("genPt" ,                   &genPt_,                    "genPt/F");
  treeOut->Branch("eSC",                      &eSC_,                      "eSC/F");
  treeOut->Branch("etaSC",                    &etaSC_,                    "etaSC/F");
  treeOut->Branch("dEtaSeed",                 &dEtaSeed_,                 "dEtaSeed/F");
  treeOut->Branch("dPhiIn",                   &dPhiIn_,                   "dPhiIn/F");
  treeOut->Branch("hOverE",                   &hOverE_,                   "hOverE/F");
  treeOut->Branch("hOverEscaled",             &hOverEscaled_,             "hOverEscaled/F");
  treeOut->Branch("full5x5_sigmaIetaIeta",    &full5x5_sigmaIetaIeta_,    "full5x5_sigmaIetaIeta/F");
  treeOut->Branch("relIsoWithEA",             &relIsoWithEA_,             "relIsoWithEA/F");
  treeOut->Branch("ooEmooP",                  &ooEmooP_,                  "ooEmooP/F");
  treeOut->Branch("d0",                       &d0_,                       "d0/F");
  treeOut->Branch("dz",                       &dz_,                       "dz/F");
  treeOut->Branch("expectedMissingInnerHits", &expectedMissingInnerHits_, "expectedMissingInnerHits/I");
  treeOut->Branch("passConversionVeto",       &passConversionVeto_,       "passConversionVeto/I");
  treeOut->Branch("isTrueEle",                &isTrueEle_,                "isTrueEle/I");

  //
  // Loop over events
  //
  UInt_t maxEvents = treeIn->GetEntries();
  UInt_t maxEventsOver10000 =   maxEvents/10000.;

  if(smallEventCount and maxEventsSmall < maxEvents) maxEvents = maxEventsSmall;

  printf("\nStart processing events, will run on %u events\n", maxEvents );

  for(UInt_t ievent = 0; ievent < maxEvents; ievent++){

    Long64_t tentry = treeIn->LoadTree(ievent);
    // Load the value of the number of the electrons in the event
    b_eleNEle->GetEntry(tentry);

    if(ievent%100000 == 0 || ievent == maxEvents-1) drawProgressBar( (1.0*ievent+1)/maxEvents);

    // Get data for all electrons in this event, only vars of interest
    b_eleRho->GetEntry(tentry);
    b_genWeight->GetEntry(tentry);
    b_elePt->GetEntry(tentry);
    b_eleGenPt->GetEntry(tentry);
    b_eleESC->GetEntry(tentry);
    b_eleEtaSC->GetEntry(tentry);
    b_elePhiSC->GetEntry(tentry);
    b_isoChargedHadrons->GetEntry(tentry);
    b_isoNeutralHadrons->GetEntry(tentry);
    b_eleIsoPhotons->GetEntry(tentry);
    b_eleIsTrueElectron->GetEntry(tentry);

    // // Other vars
    b_eleD0->GetEntry(tentry);
    b_nPV->GetEntry(tentry);
    b_eleDZ->GetEntry(tentry);
    b_eleDEtaSeed->GetEntry(tentry);
    b_eleDPhiIn->GetEntry(tentry);
    b_eleHoverE->GetEntry(tentry);
    b_eleFull5x5SigmaIEtaIEta->GetEntry(tentry);
    b_eleOOEMOOP->GetEntry(tentry);
    b_eleExpectedMissingInnerHits->GetEntry(tentry);
    b_electronPassConversionVeto->GetEntry(tentry);

    // Loop over the electrons
    for(int iele = 0; iele < eleNEle; iele++){

      gweight_ = genWeight;
      pt_ = elePt->at(iele);
      genPt_ = eleGenPt->at(iele);
      rho_ = eleRho;
      eSC_ = eleESC->at(iele);
      etaSC_ = eleEtaSC->at(iele);
      // Reweight only signal electron of the DY sample
      if(sample == SAMPLE_DY && matchType == MATCH_TRUE) kweight_ = findKinematicWeight(hKinematicWeights, pt_, etaSC_);
      else if(sample == SAMPLE_DoubleEle300to6500)       kweight_ = (6500 - 300)/(300 - 1)*N_1to300/N_300to6500;
      else                                               kweight_ = 1;

      float C_e                 = fabs(etaSC_) < 1.4442 ? C_e_barrel   : C_e_endcap;
      float C_rho               = fabs(etaSC_) < 1.4442 ? C_rho_barrel : C_rho_endcap;
      dEtaSeed_                 = eleDEtaSeed->at(iele);
      dPhiIn_                   = eleDPhiIn->at(iele);
      full5x5_sigmaIetaIeta_    = eleFull5x5SigmaIEtaIEta->at(iele);
      hOverE_                   = eleHoverE->at(iele);
      hOverEscaled_             = hOverE_ - C_e/eSC_ - C_rho*eleRho/eSC_;
      d0_                       = eleD0->at(iele);
      dz_                       = eleDZ->at(iele);
      isoChargedHadrons_        = isoChargedHadrons->at(iele);
      isoNeutralHadrons_        = isoNeutralHadrons->at(iele);
      isoPhotons_               = eleIsoPhotons->at(iele);
      expectedMissingInnerHits_ = eleExpectedMissingInnerHits->at(iele);
      nPV_                      = nPV;
      ooEmooP_                  = eleOOEMOOP->at(iele);
      passConversionVeto_       = electronPassConversionVeto->at(iele);
      isTrueEle_                = eleIsTrueElectron->at(iele);

      // Compute isolation with effective area correction for PU
      // Find eta bin first. If eta>2.5, the last eta bin is used.
      int etaBin = 0;
      while(etaBin < EffectiveAreas::nEtaBins-1 && abs(etaSC_) > EffectiveAreas::etaBinLimits[etaBin+1]) ++etaBin;
      double area = EffectiveAreas::effectiveAreaValues[etaBin];
      relIsoWithEA_ = (isoChargedHadrons_ + std::max(0.0, isoNeutralHadrons_+isoPhotons_-eleRho*area))/pt_;
      if(!passPreselection(isTrueEle_, pt_, etaSC_, passConversionVeto_, dz_, matchType, etaRegion)) continue;
      treeOut->Fill();// IK will kill me next time, if this line is not at the right place!
    } // end loop over the electrons
  } // end loop over SIGNAL events

  bazinga("I'm here to write signal tree");
  treeOut->Write();
  fileOut->Write();
  fileOut->Close();
  delete fileOut;
  fileOut = nullptr;
  delete inputFile;
  inputFile = nullptr;

  bazinga("I'm finished with calculation, here is the info from dBenchmark:");
//  printf("\n");  gBenchmark->Show("Timing");  // get timing info
} // end of main fcn



float findKinematicWeight(TH2D *hist, float pt, float etaSC){

  // For signal electrons from Drell-Yan, use kinematic weights,
  // for background electrons from ttbar, do not use any weights
  float weight = 1.0;
  // Retrieve kinematic weight
  int ipt = hist->GetXaxis()->FindBin(pt);
  int npt = hist->GetNbinsX();
  int ieta = hist->GetYaxis()->FindBin(etaSC);
  int neta = hist->GetNbinsY();
  if( ipt < 1 || ipt > npt || ieta < 1 || ieta > neta ){
    // If pt and eta are outside of the limits of the weight histogram,
    // set the weight to the edge value
    if(ipt < 1)     ipt = 1;
    if(ipt > npt)   ipt = npt;
    if(ieta < 1)    ieta = 1;
    if(ieta > neta) ieta = neta;
    weight = hist->GetBinContent(ipt, ieta);
  } else {
    // Normal case, pt and eta are within limits of the weight histogram
    weight = hist->Interpolate(pt, etaSC);
  } // end is within hist limits

  return weight;
}

bool passPreselection(int isTrue, float pt, float eta, int passConversionVeto, float dz, MatchType matchType, EtaRegion etaRegion){
  if(matchType == MATCH_TRUE and !(isTrue==1))                                      return false;
  if(matchType == MATCH_FAKE and !(isTrue==0 || isTrue==3))                         return false;
  if(etaRegion == ETA_EB     and !(abs(eta) <= boundaryEBEE))                       return false;
  if(etaRegion == ETA_EE     and !(abs(eta) >= boundaryEBEE && abs(eta) <= etaMax)) return false;
  if(etaRegion == ETA_FULL   and !(abs(eta) < etaMax))                              return false;
  if(pt < ptMin)                                                                    return false;
  if(!passConversionVeto)                                                           return false;
  if(!(abs(dz) < dzMax))                                                            return false;
  return true;
}

const int maxPowersBin = 6;
TString powers[maxPowersBin] = {"","K","M","G","T","P"};

TString eventCountString(){
  TString result = "_full";
  if(smallEventCount){
    int powerOfTen = log10(maxEventsSmall);
    int powersBin = powerOfTen/3;
    if(powersBin >= maxPowersBin) powersBin = maxPowersBin-1;
    int neventsShort = maxEventsSmall / TMath::Power(10, 3*powersBin);
    result = TString::Format("_%d%s", neventsShort, powers[powersBin].Data());
  }
  return result;
}

void drawProgressBar(float progress){

  const int barWidth = 70;

  std::cout << "[";
  int pos = barWidth * progress;
  for (int i = 0; i < barWidth; ++i) {
    if (i < pos)       std::cout << "=";
    else if (i == pos) std::cout << ">";
    else               std::cout << " ";
  }
  std::cout << "] " << int(progress * 100.0) << " %\r";
  std::cout.flush();

  if( progress >= 1.0 ) std::cout << std::endl;

  return;
}


// For electron ID tuning, use MATCH_TRUE with SAMPLE_DY
// and MATCH_FAKE with SAMPLE_TT.
// NOTE: kinematic weights are meaningful only for the combination DY/TRUE.
// for all other choices of flags kinematic weights are 1.0
int main(int argc, char *argv[]){
  gROOT->SetBatch();

  // For tuning
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DY, MATCH_TRUE, ETA_EB);
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DY, MATCH_TRUE, ETA_EE);
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_TT, MATCH_FAKE, ETA_EB);
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_TT, MATCH_FAKE, ETA_EE);

  // For plotting
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DY,                 MATCH_TRUE, ETA_FULL);
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DY,                 MATCH_ANY,  ETA_FULL);
  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_TT,                 MATCH_ANY,  ETA_FULL);
//  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_GJ,                 MATCH_ANY,  ETA_FULL);
//  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DoubleEle1to300,    MATCH_ANY,  ETA_FULL);
//  convert_EventStrNtuple_To_FlatNtuple(SAMPLE_DoubleEle300to6500, MATCH_ANY,  ETA_FULL);
}
