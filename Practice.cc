#include "G4RunManager.hh"
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

    G4UIExecutive* ui = (argc == 1) ? new G4UIExecutive(argc, argv) : nullptr;

    auto runManager = new G4RunManager();

    auto det = new DetectorConstruction();
    runManager->SetUserInitialization(new MyPhysicsList());
    runManager->SetUserInitialization(new ActionInitialization());
    runManager->SetUserInitialization(det);
    runManager->Initialize();

    auto uiManager = G4UImanager::GetUIpointer();

    if (!ui) {
        const G4String fileName = argv[1];
        G4cout << "Executing macro (batch): " << fileName << G4endl;
        uiManager->ApplyCommand("/control/execute " + fileName);
    }
    else {
        auto visManager = new G4VisExecutive();
        visManager->Initialize();
        uiManager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
        delete visManager;
        delete ui;
    
    }

    delete runManager;
    return 0;
}