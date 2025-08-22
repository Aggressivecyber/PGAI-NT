#include "G4RunManager.hh"
#include "G4UIExecutive.hh"
#include "G4RunManagerFactory.hh"
#include "DetectorConstruction.hh"
#include "FTFP_BERT.hh"
#include "ActionInitialization.hh"
#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "G4SteppingVerbose.hh"
#include "MyPhysicsList.hh"

int main(int argc, char** argv) {
	std::cout << "Program Start" << std::endl;
	G4UIExecutive* ui = nullptr;
	if (argc == 1) { ui = new G4UIExecutive(argc, argv); }
	G4int precision = 4;
	G4SteppingVerbose::UseBestUnit(precision);
	auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
	runManager->SetUserInitialization(new DetectorConstruction());
	runManager->SetUserInitialization(new MyPhysicsList());
	runManager->SetUserInitialization(new ActionInitialization());
	runManager->Initialize();
	auto visManager = new G4VisExecutive(argc,argv,"OGL","Quiet");
	visManager->Initialize();
	auto uiManager = G4UImanager::GetUIpointer();
	if (!ui) {
		G4String command = "/control/execute";
		G4String fileName = argv[1];
		G4cout << "Executing macro: " << fileName << G4endl;
		uiManager->ApplyCommand(command+ " "+fileName);
	}
	else {
		uiManager->ApplyCommand("/control/execute vis.mac");
		ui->SessionStart();
		delete ui;
	}
	delete runManager;
	delete visManager;
	return 0;
}