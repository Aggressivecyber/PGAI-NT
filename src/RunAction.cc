#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"

RunAction::RunAction(DetectorConstruction* det,
                     PrimaryGeneratorAction* gen,
                     G4double step)
  : G4UserRunAction(), fDetector(det), fGenerator(gen), fStep(step) {
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

  // Rotate geometry and source for the next run
  G4int nextRun = aRun->GetRunID() + 1;
  G4double angle = nextRun * fStep;
  if (fDetector) {
    fDetector->RotateRig(angle);
  }
  if (fGenerator) {
    fGenerator->SetAngle(angle);
  }
}
