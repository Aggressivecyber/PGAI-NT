#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"


RunAction::RunAction() : G4UserRunAction() {
	auto man = G4AnalysisManager::Instance();
	man->SetNtupleMerging(1);
	man->SetVerboseLevel(1);
}
RunAction::~RunAction() {
	delete G4AnalysisManager::Instance();
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	G4cout << "Run Start" << G4endl;
	auto man = G4AnalysisManager::Instance();
	G4int runID = aRun->GetRunID();
	man->SetFileName("hits"+std::to_string(runID));
	man->SetDefaultFileType("csv");
	man->OpenFile();
	man->CreateNtuple("hits", "SD hits table");
	man->CreateNtupleIColumn("sd");
	man->CreateNtupleIColumn("event");
	man->CreateNtupleDColumn("edep_keV");
	man->CreateNtupleDColumn("pos_X_mm");
	man->CreateNtupleDColumn("pos_Y_mm");
	man->CreateNtupleDColumn("pos_Z_mm");
	man->CreateNtupleIColumn("num_X");
	man->CreateNtupleIColumn("num_Y");
	man->CreateNtupleDColumn("Time_ns");
	man->CreateNtupleSColumn("pname");
	man->FinishNtuple();

}
void RunAction::EndOfRunAction(const G4Run* aRun) {
	G4cout << "Run End" << G4endl;
	auto man = G4AnalysisManager::Instance();
	man->Write();
	man->CloseFile();
}
