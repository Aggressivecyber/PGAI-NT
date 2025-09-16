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
	G4LogicalVolume* logicVoxel = nullptr;
	inline G4LogicalVolume* getMatrix() const
	{
		return logicMatrixVoxel;
	}
	void setDeg(double);
	double getDeg();

private:
	G4LogicalVolume* logicMatrixVoxel;
	G4LogicalVolume* logicHPGe;
	G4bool  fCheckOverlaps;
	virtual void ConstructSDandField();
	G4UserLimits* fStepLimit;
	G4double deg{ 0.};
	G4VSensitiveDetector* HPGEsd = nullptr;
	G4VSensitiveDetector* CMOSsd = nullptr;
};




#endif