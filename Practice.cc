#include "G4MTRunManager.hh"
#include "G4UIExecutive.hh"
#include "DetectorConstruction.hh"
#include "FTFP_BERT.hh"
#include "ActionInitialization.hh"
#include "G4VisExecutive.hh"
#include "G4UImanager.hh"
#include "G4SteppingVerbose.hh"
#include "MyPhysicsList.hh"
#include "PrimaryGeneratorAction.hh"
#include "G4SystemOfUnits.hh"

int main(int argc, char** argv) {
	std::cout << "Program Start" << std::endl;
	G4UIExecutive* ui = nullptr;
	if (argc == 1) { ui = new G4UIExecutive(argc, argv); }
	G4int precision = 4;
	G4SteppingVerbose::UseBestUnit(precision);
	auto runManager = new G4MTRunManager();
	auto det = new DetectorConstruction();
	runManager->SetUserInitialization(det);
	runManager->SetUserInitialization(new MyPhysicsList());
	runManager->SetUserInitialization(new ActionInitialization(det));
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

		for (int i = 0; i < 3; i++)
		{
	
			det->setDeg(i * 10.);
			G4RunManager::GetRunManager()->ReinitializeGeometry();
			runManager->BeamOn(10);
		}
		uiManager->ApplyCommand("/control/execute vis.mac");
		uiManager->ApplyCommand("/vis/verbose 1");
		ui->SessionStart();
		delete ui;
	}
=======
                ui->SessionStart();
                delete ui;
        }

        G4double rotateAngle = 10 * CLHEP::deg;
        det->RotateRig(rotateAngle);
        auto generator = const_cast<PrimaryGeneratorAction*>(static_cast<const PrimaryGeneratorAction*>(runManager->GetUserPrimaryGeneratorAction()));
        if (generator) {
                generator->SetAngle(rotateAngle);
        }
        runManager->ReinitializeGeometry();
	
>>>>>>> refs/remotes/origin/main
	delete runManager;
	delete visManager;
	return 0;
}
