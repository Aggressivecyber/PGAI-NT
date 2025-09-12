#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"
#include "G4RunManager.hh"

void ActionInitialization::BuildForMaster() const {
        auto det = static_cast<DetectorConstruction*>(
                G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        SetUserAction(new RunAction(det, nullptr));
}

void ActionInitialization::Build() const {
        auto generator = new PrimaryGeneratorAction();
        SetUserAction(generator);
        auto det = static_cast<DetectorConstruction*>(
                G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        SetUserAction(new RunAction(det, generator));
}