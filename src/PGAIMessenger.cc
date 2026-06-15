#include "PGAIMessenger.hh"
#include "DetectorConstruction.hh"
#include "PGAIConfig.hh"

#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWith3Vector.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

PGAIMessenger::PGAIMessenger(DetectorConstruction* det) : fDet(det) {
	fDir = new G4UIdirectory("/pgai/");
	fDir->SetGuidance("PGAI-NT dual-modal simulation control");

	fDirSource = new G4UIdirectory("/pgai/source/");
	fDirDet    = new G4UIdirectory("/pgai/detector/");
	fDirHPGe   = new G4UIdirectory("/pgai/hpge/");
	fDirPhantom= new G4UIdirectory("/pgai/phantom/");
	fDirRun    = new G4UIdirectory("/pgai/run/");

	fCmdEnergy = new G4UIcmdWithADoubleAndUnit("/pgai/source/energy", this);
	fCmdEnergy->SetGuidance("Mean neutron energy");
	fCmdEnergy->SetParameterName("energy", false);
	fCmdEnergy->SetDefaultUnit("MeV");

	fCmdEnergySpread = new G4UIcmdWithADouble("/pgai/source/energySpread", this);
	fCmdEnergySpread->SetGuidance("Fractional energy spread (1-sigma), e.g. 0.03");
	fCmdEnergySpread->SetParameterName("spread", false);

	fCmdSpotSize = new G4UIcmdWithADoubleAndUnit("/pgai/source/spotSize", this);
	fCmdSpotSize->SetGuidance("Source spot size (square full width)");
	fCmdSpotSize->SetParameterName("spot", false);
	fCmdSpotSize->SetDefaultUnit("mm");

	fCmdDivergence = new G4UIcmdWithADoubleAndUnit("/pgai/source/divergence", this);
	fCmdDivergence->SetGuidance("Angular divergence (1-sigma)");
	fCmdDivergence->SetParameterName("div", false);
	fCmdDivergence->SetDefaultUnit("deg");

	fCmdPixelsX = new G4UIcmdWithAnInteger("/pgai/detector/pixelsX", this);
	fCmdPixelsX->SetParameterName("nx", false);
	fCmdPixelsY = new G4UIcmdWithAnInteger("/pgai/detector/pixelsY", this);
	fCmdPixelsY->SetParameterName("ny", false);

	fCmdScintThickness = new G4UIcmdWithADoubleAndUnit("/pgai/detector/scintThickness", this);
	fCmdScintThickness->SetParameterName("thk", false);
	fCmdScintThickness->SetDefaultUnit("mm");

	fCmdSmearHPGe = new G4UIcmdWithABool("/pgai/hpge/smear", this);
	fCmdSmearHPGe->SetGuidance("Enable Gaussian energy resolution smearing on HPGe");

	fCmdPhantomMode = new G4UIcmdWithAString("/pgai/phantom/mode", this);
	fCmdPhantomMode->SetGuidance("empty | single | degeneracy | steel");
	fCmdPhantomMode->SetParameterName("mode", false);
	fCmdPhantomMode->SetCandidates("empty single degeneracy steel cttest");

	fCmdSingleMaterial = new G4UIcmdWithAString("/pgai/phantom/singleMaterial", this);
	fCmdSingleMaterial->SetParameterName("mat", false);
	fCmdSingleMaterial->SetCandidates("PE Al Fe Cu Pb Ni air water");

	fCmdSingleThickness = new G4UIcmdWithADoubleAndUnit("/pgai/phantom/singleThickness", this);
	fCmdSingleThickness->SetParameterName("thk", false);
	fCmdSingleThickness->SetDefaultUnit("mm");

	fCmdAngle = new G4UIcmdWithADoubleAndUnit("/pgai/run/angle", this);
	fCmdAngle->SetGuidance("Projection angle (tomography)");
	fCmdAngle->SetParameterName("angle", false);
	fCmdAngle->SetDefaultUnit("deg");
}

PGAIMessenger::~PGAIMessenger() {
	delete fCmdEnergy; delete fCmdEnergySpread; delete fCmdSpotSize; delete fCmdDivergence;
	delete fCmdPixelsX; delete fCmdPixelsY; delete fCmdScintThickness;
	delete fCmdSmearHPGe;
	delete fCmdPhantomMode; delete fCmdSingleMaterial; delete fCmdSingleThickness;
	delete fCmdAngle;
	delete fDir; delete fDirSource; delete fDirDet; delete fDirHPGe; delete fDirPhantom; delete fDirRun;
}

void PGAIMessenger::SetNewValue(G4UIcommand* cmd, G4String value) {
	if (cmd == fCmdEnergy)           gConfig.energy = fCmdEnergy->GetNewDoubleValue(value);
	else if (cmd == fCmdEnergySpread) gConfig.energySpread = fCmdEnergySpread->GetNewDoubleValue(value);
	else if (cmd == fCmdSpotSize)    gConfig.spotSize = fCmdSpotSize->GetNewDoubleValue(value);
	else if (cmd == fCmdDivergence)  gConfig.divergence = fCmdDivergence->GetNewDoubleValue(value);
	else if (cmd == fCmdPixelsX)     gConfig.pixelsX = fCmdPixelsX->GetNewIntValue(value);
	else if (cmd == fCmdPixelsY)     gConfig.pixelsY = fCmdPixelsY->GetNewIntValue(value);
	else if (cmd == fCmdScintThickness) gConfig.scintThickness = fCmdScintThickness->GetNewDoubleValue(value);
	else if (cmd == fCmdSmearHPGe)   gConfig.smearHPGe = fCmdSmearHPGe->GetNewBoolValue(value);
	else if (cmd == fCmdPhantomMode) gConfig.phantomMode = value;
	else if (cmd == fCmdSingleMaterial) gConfig.singleMaterial = value;
	else if (cmd == fCmdSingleThickness) gConfig.singleThickness = fCmdSingleThickness->GetNewDoubleValue(value);
	else if (cmd == fCmdAngle) {
		G4double a = fCmdAngle->GetNewDoubleValue(value);
		gConfig.angleDeg = a / deg;
		if (fDet) fDet->setDeg(gConfig.angleDeg);
	}
}
