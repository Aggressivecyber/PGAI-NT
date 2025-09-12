#ifndef RUNACTION_HH
#define RUNACTION_HH 1
#include "G4UserRunAction.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

class RunAction :public G4UserRunAction {
public:
        RunAction(DetectorConstruction* det, PrimaryGeneratorAction* gen,
                  G4double step = 10*deg);
        ~RunAction();
        virtual void BeginOfRunAction(const G4Run*);
        virtual void EndOfRunAction(const G4Run*);
private:
        G4AnalysisManager* analysisManager = nullptr;
        DetectorConstruction* fDetector;
        PrimaryGeneratorAction* fGenerator;
        G4double fStep;
};




#endif