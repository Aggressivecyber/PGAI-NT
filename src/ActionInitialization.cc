#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "DetectorConstruction.hh"

ActionInitialization::ActionInitialization(DetectorConstruction* det)
    : G4VUserActionInitialization(), fDetector(det) {}

void ActionInitialization::BuildForMaster() const {}

void ActionInitialization::Build() const {
        SetUserAction(new PrimaryGeneratorAction());
        SetUserAction(new RunAction(fDetector));
}