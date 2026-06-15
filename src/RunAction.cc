#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"


RunAction::RunAction() : G4UserRunAction() {
	auto man = G4AnalysisManager::Instance();
	// 注: csv 输出不支持 MT ntuple merging, 各 worker 分片写 hits{runID}_nt_hits_t{N}.csv
	man->SetVerboseLevel(0);
	man->SetDefaultFileType("csv");

	// ntuple 在构造时创建一次, 避免多 run 重复创建报错
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

RunAction::~RunAction() {
	// 不手动 delete AnalysisManager 单例 (MT 下会重复释放, 由 RunManager 统一清理)
}

void RunAction::BeginOfRunAction(const G4Run* aRun) {
	auto man = G4AnalysisManager::Instance();
	G4int runID = aRun->GetRunID();
	man->SetFileName("hits" + std::to_string(runID));
	man->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run* /*aRun*/) {
	auto man = G4AnalysisManager::Instance();
	man->Write();
	man->CloseFile();
}
