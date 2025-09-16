#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include <atomic>

RunAction::RunAction(bool pMaster):isMaster(pMaster) {

}
RunAction::~RunAction() {
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	auto man = G4AnalysisManager::Instance();
	man->SetVerboseLevel(1);
	man->SetDefaultFileType("csv");
	static std::atomic<int> runCounter{ 0 };
	G4int currentRunNumber = runCounter.load();
	if (isMaster) {
		currentRunNumber = runCounter.fetch_add(1) + 1;
	}
	else {
		currentRunNumber = runCounter.load();
	}
	G4cout << "Run Start" << G4endl;
	man->SetFileName("hits" + std::to_string(currentRunNumber));
	man->OpenFile();
	if (man->GetNtuple(0) == nullptr) {
		man->SetNtupleMerging(true);
		man->SetVerboseLevel(1);
		man->SetDefaultFileType("csv");
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
