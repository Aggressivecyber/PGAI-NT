#ifndef DETECTORCONSTRUCTION
#define DETECTORCONSTRUCTION
#include "G4VUserDetectorConstruction.hh"
#include "G4UserLimits.hh"
#include "G4LogicalVolume.hh"
#include "VoxelNum.hh"
;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
	virtual G4VPhysicalVolume* Construct() override;

	DetectorConstruction() ;



	void setDeg(double) ;

	double getDeg() const;

	

private:

	void DefinitionMatertial();
	G4LogicalVolume* logicPixel = nullptr;
	G4LogicalVolume* logicHPGe = nullptr;
	G4bool  fCheckOverlaps = false;
	void ConstructSDandField() override;
	G4UserLimits* fStepLimit = nullptr;
	G4VSensitiveDetector* HPGEsd = nullptr;
	G4VSensitiveDetector* CMOSsd = nullptr;
};




#endif
