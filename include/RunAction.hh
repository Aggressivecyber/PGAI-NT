#ifndef RUNACTION_HH
#define RUNACTION_HH 1
#include "G4UserRunAction.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

class DetectorConstruction;

class RunAction :public G4UserRunAction {
public:
        RunAction(DetectorConstruction* det, G4double step = 10 * CLHEP::deg);
        ~RunAction();
        void BeginOfRunAction(const G4Run*) override;
        void EndOfRunAction(const G4Run*) override;
private:
        G4AnalysisManager* analysisManager = nullptr;
        DetectorConstruction* fDetector;
        G4double fStep;
};




#endif