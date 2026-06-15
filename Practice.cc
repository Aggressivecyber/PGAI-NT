#include "G4MTRunManager.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4SteppingVerbose.hh"
#include "G4Threading.hh"

#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"
#include "MyPhysicsList.hh"

// NT — Neutron Tomography 中子层析成像仿真
// 用法:
//   ./NT run.mac        # 批处理: 多线程快速生产跑 (推荐)
//   ./NT                # 交互式: 多角度扫描 + 可视化

int main(int argc, char** argv) {
	G4cout << "=== NT (Neutron Tomography) Start ===" << G4endl;

	G4int precision = 4;
	G4SteppingVerbose::UseBestUnit(precision);

	auto runManager = new G4MTRunManager();
	// 多线程: 默认使用全部物理核心以加速
	runManager->SetNumberOfThreads(G4Threading::G4GetNumberOfCores());

	auto det = new DetectorConstruction();
	runManager->SetUserInitialization(det);
	runManager->SetUserInitialization(new MyPhysicsList());
	runManager->SetUserInitialization(new ActionInitialization(det));
	runManager->Initialize();

	auto uiManager = G4UImanager::GetUIpointer();

	// 批处理模式: 执行 macro, 不启动可视化 (最快)
	if (argc > 1) {
		G4String command = "/control/execute ";
		G4cout << "Executing macro: " << argv[1] << G4endl;
		uiManager->ApplyCommand(command + argv[1]);
	}
	// 交互模式: 多角度扫描投影 + 可视化
	else {
		auto visManager = new G4VisExecutive(argc, argv, "OGL", "Quiet");
		visManager->Initialize();

		G4UIExecutive* ui = new G4UIExecutive(argc, argv);

		// 多角度扫描: 0°, 10°, 20° 三组投影
		for (G4int i = 0; i < 3; i++) {
			det->setDeg(i * 10.);
			G4RunManager::GetRunManager()->ReinitializeGeometry();
			runManager->BeamOn(10);
		}

		uiManager->ApplyCommand("/control/execute vis.mac");
		uiManager->ApplyCommand("/vis/verbose 1");
		ui->SessionStart();

		delete ui;
		delete visManager;
	}

	delete runManager;
	return 0;
}
