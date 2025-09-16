#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include <atomic>

RunAction::RunAction(bool pMaster):isMaster(pMaster) {
	auto man = G4AnalysisManager::Instance();
	man->SetNtupleMerging(true);
	man->SetVerboseLevel(1);
	man->SetDefaultFileType("root");
	man->CreateNtuple("hits", "SD hits table");
	man->CreateNtupleIColumn("sd");
	man->CreateNtupleIColumn("event");
	man->CreateNtupleDColumn("edep_keV");
	man->CreateNtupleDColumn("pos_X_mm");
	man->CreateNtupleDColumn("pos_Y_mm");
	man->CreateNtupleDColumn("pos_Z_mm");
	man->CreateNtupleIColumn("copyNum");
	man->CreateNtupleDColumn("Time_ns");
	man->CreateNtupleSColumn("pname");
	man->FinishNtuple();
}
RunAction::~RunAction() {
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	if (isMaster)
	{
		static std::atomic<int> runCounter{ 0 };
		G4int currentRunNumber = runCounter.load();
		if (isMaster) {
			currentRunNumber = runCounter.fetch_add(1) + 1;
		}
		else {
			currentRunNumber = runCounter.load();
		}
		G4cout << "Run Start" << G4endl;
		auto man = G4AnalysisManager::Instance();
		man->SetFileName("hits"+std::to_string(currentRunNumber));
		man->OpenFile();
	}
	

}
void RunAction::EndOfRunAction(const G4Run* aRun) {
	G4cout << "Run End" << G4endl;
	if (isMaster)
	{
		auto man = G4AnalysisManager::Instance();
		man->Write();
		man->CloseFile();
	}
}
