#ifndef PGAI_MESSENGER_HH
#define PGAI_MESSENGER_HH

#include "G4UImessenger.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"

class DetectorConstruction;
class G4UIdirectory;

// /pgai/... 宏命令: 配置源、探测器、phantom、角度
class PGAIMessenger : public G4UImessenger {
public:
	explicit PGAIMessenger(DetectorConstruction* det);
	~PGAIMessenger() override;
	void SetNewValue(G4UIcommand* cmd, G4String value) override;

private:
	DetectorConstruction* fDet;

	G4UIdirectory* fDir;
	G4UIdirectory* fDirSource;
	G4UIdirectory* fDirDet;
	G4UIdirectory* fDirHPGe;
	G4UIdirectory* fDirPhantom;
	G4UIdirectory* fDirRun;

	G4UIcmdWithADoubleAndUnit* fCmdEnergy;
	G4UIcmdWithADouble* fCmdEnergySpread;
	G4UIcmdWithADoubleAndUnit* fCmdSpotSize;
	G4UIcmdWithADoubleAndUnit* fCmdDivergence;

	G4UIcmdWithAnInteger* fCmdPixelsX;
	G4UIcmdWithAnInteger* fCmdPixelsY;
	G4UIcmdWithADoubleAndUnit* fCmdScintThickness;

	G4UIcmdWithABool* fCmdSmearHPGe;

	G4UIcmdWithAString* fCmdPhantomMode;
	G4UIcmdWithAString* fCmdSingleMaterial;
	G4UIcmdWithADoubleAndUnit* fCmdSingleThickness;

	G4UIcmdWithADoubleAndUnit* fCmdAngle;
};

#endif
