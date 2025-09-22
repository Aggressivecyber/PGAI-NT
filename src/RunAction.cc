#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"
#include <atomic>

RunAction::RunAction(PrimaryGeneratorAction* pga) : fPGA(pga) {
	auto man = G4AnalysisManager::Instance();
	man->SetVerboseLevel(1);
	man->SetDefaultFileType("csv");
	man->CreateNtuple("hits", "SD hits table");
	man->CreateNtupleSColumn("sdhc");
	man->CreateNtupleIColumn("event");
	man->CreateNtupleDColumn("edep_keV");
	man->CreateNtupleDColumn("pos_X_mm");
	man->CreateNtupleDColumn("pos_Y_mm");
	man->CreateNtupleDColumn("pos_Z_mm");
	man->CreateNtupleIColumn("copyNum");
	man->CreateNtupleDColumn("Time_ns");
	man->CreateNtupleSColumn("pname");
	man->FinishNtuple();
	man->SetVerboseLevel(1);
}

RunAction::~RunAction() {
}
void RunAction::BeginOfRunAction(const G4Run* aRun) {
	G4cout << "Run Start" << aRun->GetRunID() << G4endl;
	auto man = G4AnalysisManager::Instance();
	G4int runID = aRun->GetRunID();
	G4double phi = fPGA->GetPhi0() + runID * fPGA->GetDphi();
	fPGA->SetPhiCenter(phi);
	man->SetFileName("hits" +to_string(runID));
	man->OpenFile();

}
	

void RunAction::EndOfRunAction(const G4Run* aRun) {
		G4cout << "Run End"<<aRun->GetRunID() << G4endl;
		auto man = G4AnalysisManager::Instance();
		man->Write();
		man->CloseFile();
}
