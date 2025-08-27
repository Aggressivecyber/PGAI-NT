#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"

RunAction::RunAction() : G4UserRunAction() {
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();

}
RunAction::~RunAction() {
	delete G4AnalysisManager::Instance();
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
	analysisManager->OpenFile("NeutronSim");
}
void RunAction::EndOfRunAction(const G4Run* aRun) {
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
	analysisManager->Write();
	analysisManager->CloseFile();
}
