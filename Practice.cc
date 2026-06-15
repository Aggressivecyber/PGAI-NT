#include "G4MTRunManager.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4SteppingVerbose.hh"
#include "G4Threading.hh"

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "MyPhysicsList.hh"
#include "PGAIMessenger.hh"

// PGAI-NT — 双模态快中子材料识别仿真系统
//   Channel A: 4.05 MeV 快中子透射成像 (塑料闪烁体)
//   Channel B: HPGe 瞬发伽马能谱
//
// 用法:
//   ./build/NT <macro>     # 批处理 (推荐)
//   ./build/NT             # 交互式 + 可视化
//
// 关键宏命令见 /pgai/*  (PGAIMessenger)

int main(int argc, char** argv) {
	G4cout << "=== PGAI-NT (Dual-modal Neutron Material ID) Start ===" << G4endl;

	G4int precision = 4;
	G4SteppingVerbose::UseBestUnit(precision);

	auto runManager = new G4MTRunManager();
	runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());

	auto det = new DetectorConstruction();
	runManager->SetUserInitialization(det);
	runManager->SetUserInitialization(new MyPhysicsList());
	runManager->SetUserInitialization(new ActionInitialization(det));

	// 注册宏命令 (/pgai/...)
	auto messenger = new PGAIMessenger(det);

	auto uiManager = G4UImanager::GetUIpointer();

	if (argc > 1) {
		// 批处理: 执行 macro
		G4String command = "/control/execute ";
		G4cout << "Executing macro: " << argv[1] << G4endl;
		uiManager->ApplyCommand(command + argv[1]);
	} else {
		// 交互式 + 可视化
		auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
		visManager->Initialize();
		G4UIExecutive* ui = new G4UIExecutive(argc, argv);
		uiManager->ApplyCommand("/control/execute vis.mac");
		ui->SessionStart();
		delete ui;
		delete visManager;
	}

	delete runManager;
	return 0;
}
