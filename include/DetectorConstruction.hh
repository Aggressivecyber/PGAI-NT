#ifndef DETECTORCONSTRUCTION
#define DETECTORCONSTRUCTION 1
#include "G4VUserDetectorConstruction.hh"
#include "G4UserLimits.hh"
#include "globals.hh"
#include "G4LogicalVolume.hh"

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
	virtual G4VPhysicalVolume* Construct() override;
private:
	G4LogicalVolume* logicMatrixVoxel;
	G4LogicalVolume* logicHPGe;
	G4bool  fCheckOverlaps;
	virtual void ConstructSDandField();
	G4UserLimits* fStepLimit;
};




#endif