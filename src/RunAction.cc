#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"

RunAction::RunAction() : G4UserRunAction() {
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

}
RunAction::~RunAction() {
	delete analysisManager;
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	G4int runID = run->GetRunID();
	G4cout << "### Run " << runID << " start." << G4endl;
}
void RunAction::EndOfRunAction(const G4Run* aRun) {
	G4int nEvents = run->GetNumberOfEvent();
	G4cout << "### Run " << run->GetRunID()
		<< " finished with " << nEvents << " events." << G4endl;
}
