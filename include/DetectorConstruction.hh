#ifndef DETECTORCONSTRUCTION
#define DETECTORCONSTRUCTION 1
#include "G4VUserDetectorConstruction.hh"
#include "G4UserLimits.hh"
#include "globals.hh"
#include "G4LogicalVolume.hh"
<<<<<<< HEAD

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
	virtual G4VPhysicalVolume* Construct() override;
	G4LogicalVolume* logicVoxel = nullptr;
	G4int voxelNx = 0;
	G4int voxelNy = 0;
private:
	G4LogicalVolume* logicMatrixVoxel;
	G4LogicalVolume* logicHPGe;
	G4bool  fCheckOverlaps;
	virtual void ConstructSDandField();
	G4UserLimits* fStepLimit;
=======
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"

namespace voxel_num
{
	inline G4int voxelNx = 0;
	inline G4int voxelNy = 0;
	inline G4double Voxel_H{ 0 };
	inline G4double HPGe_H{ 0 };
}

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
        void RotateRig(G4double angle);
        void SetRadius(G4double hpgeRadius, G4double cmosRadius, G4double sourceRadius);
        G4VPhysicalVolume* Construct() override;
        G4LogicalVolume* logicVoxel = nullptr;

private:
        void ConstructSDandField() override;
        G4LogicalVolume* logicMatrixVoxel;
        G4LogicalVolume* logicHPGe;

        G4PVPlacement* fHPGePhys = nullptr;
        G4PVPlacement* fCmosPhys = nullptr;
        G4PVPlacement* fSourcePhys = nullptr;

        G4double fHPGeRadius = voxel_num::HPGe_H;
        G4double fCmosRadius = 40 + voxel_num::Voxel_H;
        G4double fSourceRadius = 800 * CLHEP::mm;

        G4ThreeVector fHPGeStartPos{0, 0, -voxel_num::HPGe_H};
        G4ThreeVector fCmosStartPos{40 + voxel_num::Voxel_H, 0, 0};
        G4ThreeVector fSourceStartPos{-800 * CLHEP::mm, 0, 0};

G4RotationMatrix fHPGeStartRot, fCmosStartRot, fSourceStartRot;
>>>>>>> refs/remotes/origin/main
};




#endif
