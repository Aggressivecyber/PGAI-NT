#include "RunAction.hh"
#include "G4Run.hh"
#include "G4AnalysisManager.hh"

RunAction::RunAction() : G4UserRunAction() {
	auto man = G4AnalysisManager::Instance();
	man->SetVerboseLevel(0);
	man->SetDefaultFileType("csv");

	// ntuple 0: 快中子透射 (per-step)
	man->CreateNtuple("transmission", "fast neutron transmission hits");
	man->CreateNtupleIColumn("run_id");
	man->CreateNtupleIColumn("event_id");
	man->CreateNtupleDColumn("angle_deg");
	man->CreateNtupleIColumn("pixel_x");
	man->CreateNtupleIColumn("pixel_y");
	man->CreateNtupleDColumn("edep_keV");
	man->CreateNtupleDColumn("time_ns");
	man->CreateNtupleSColumn("particle_name");
	man->CreateNtupleIColumn("track_id");
	man->CreateNtupleIColumn("parent_id");
	man->CreateNtupleSColumn("creator_process");
	man->CreateNtupleDColumn("kinetic_energy_MeV");
	man->CreateNtupleDColumn("local_x_mm");
	man->CreateNtupleDColumn("local_y_mm");
	man->CreateNtupleDColumn("local_z_mm");
	man->CreateNtupleIColumn("is_primary_neutron");
	man->CreateNtupleIColumn("is_scattered_neutron");
	man->FinishNtuple();

	// ntuple 1: HPGe event-level 能谱
	man->CreateNtuple("hpge_events", "HPGe event-level spectrum");
	man->CreateNtupleIColumn("run_id");
	man->CreateNtupleIColumn("event_id");
	man->CreateNtupleDColumn("angle_deg");
	man->CreateNtupleDColumn("total_edep_keV");
	man->CreateNtupleIColumn("n_steps");
	man->CreateNtupleDColumn("first_gamma_energy_keV");
	man->CreateNtupleSColumn("particle_names");
	man->CreateNtupleSColumn("dominant_creator_process");
	man->CreateNtupleSColumn("detector_name");
	man->FinishNtuple();
}

RunAction::~RunAction() {
	// 不 delete AnalysisManager 单例 (MT 下由 RunManager 统一清理)
}

void RunAction::BeginOfRunAction(const G4Run* aRun) {
	auto man = G4AnalysisManager::Instance();
	man->SetFileName("pgai_run" + std::to_string(aRun->GetRunID()));
	man->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run* /*aRun*/) {
	auto man = G4AnalysisManager::Instance();
	man->Write();
	man->CloseFile();
}
