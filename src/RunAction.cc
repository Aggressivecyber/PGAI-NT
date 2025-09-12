#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include "DetectorConstruction.hh"

RunAction::RunAction(DetectorConstruction* det, G4double step)
    : G4UserRunAction(), fDetector(det), fStep(step) {
        analysisManager = G4AnalysisManager::Instance();
}

RunAction::~RunAction() {
        delete analysisManager;
}

void RunAction::BeginOfRunAction(const G4Run* aRun) {
        G4int runID = aRun->GetRunID();
        G4cout << "### Run " << runID << " start." << G4endl;
}

void RunAction::EndOfRunAction(const G4Run* aRun) {
        G4int nEvents = aRun->GetNumberOfEvent();
        G4cout << "### Run " << aRun->GetRunID()
                << " finished with " << nEvents << " events." << G4endl;
        if (fDetector) {
                fDetector->RotateRig(fStep);
        }
}
