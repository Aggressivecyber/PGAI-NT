#include "PrimaryGeneratorMessenger.hh"
#include "PrimaryGeneratorAction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAnInteger.hh"

PrimaryGeneratorMessenger::PrimaryGeneratorMessenger(PrimaryGeneratorAction* act)
    : fAction(act)
{
    fDir = new G4UIdirectory("/src/");
    fDir->SetGuidance("Controls for primary generator");

    fPhi0Cmd = new G4UIcmdWithADoubleAndUnit("/src/phi0", this);
    fPhi0Cmd->SetUnitCategory("Angle");
    fPhi0Cmd->SetDefaultUnit("deg");

    fDphiCmd = new G4UIcmdWithADoubleAndUnit("/src/dphi", this);
    fDphiCmd->SetUnitCategory("Angle");
    fDphiCmd->SetDefaultUnit("deg");

    fRadiusCmd = new G4UIcmdWithADoubleAndUnit("/src/radius", this);
    fRadiusCmd->SetUnitCategory("Length");
    fRadiusCmd->SetDefaultUnit("mm");

    fNperEvtCmd = new G4UIcmdWithAnInteger("/src/nPerEvt", this);
}

PrimaryGeneratorMessenger::~PrimaryGeneratorMessenger() {
    delete fPhi0Cmd;
    delete fDphiCmd;
    delete fRadiusCmd;
    delete fNperEvtCmd;
    delete fDir;
}

void PrimaryGeneratorMessenger::SetNewValue(G4UIcommand* cmd, G4String val) {
    if (cmd == fPhi0Cmd) fAction->SetPhi0(fPhi0Cmd->GetNewDoubleValue(val));
    else if (cmd == fDphiCmd) fAction->SetDphi(fDphiCmd->GetNewDoubleValue(val));
    else if (cmd == fRadiusCmd) fAction->SetRadius(fRadiusCmd->GetNewDoubleValue(val));
    else if (cmd == fNperEvtCmd) fAction->SetNPerEvent(fNperEvtCmd->GetNewIntValue(val));
}