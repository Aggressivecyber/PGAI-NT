#ifndef PRIMARYGENERATORMESSENGER_HH
#define PRIMARYGENERATORMESSENGER_HH 1

#include "G4UImessenger.hh"

class G4UIdirectory;
class G4UIcmdWithADoubleAndUnit;
class G4UIcmdWithAnInteger;
class PrimaryGeneratorAction;

class PrimaryGeneratorMessenger : public G4UImessenger {
public:
    PrimaryGeneratorMessenger(PrimaryGeneratorAction* act);
    ~PrimaryGeneratorMessenger() override;

    void SetNewValue(G4UIcommand* cmd, G4String val) override;

private:
    PrimaryGeneratorAction* fAction;
    G4UIdirectory* fDir = nullptr;
    G4UIcmdWithADoubleAndUnit* fPhi0Cmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fDphiCmd = nullptr;
    G4UIcmdWithADoubleAndUnit* fRadiusCmd = nullptr;
    G4UIcmdWithAnInteger* fNperEvtCmd = nullptr;
};

#endif