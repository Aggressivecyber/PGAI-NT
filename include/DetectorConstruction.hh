#ifndef DETECTORCONSTRUCTION
#define DETECTORCONSTRUCTION 1
#include "G4VUserDetectorConstruction.hh"
#include "G4UserLimits.hh"
#include "globals.hh"
#include "G4LogicalVolume.hh"
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
        void RotateRig(G4double degZ);
        void SetRadius(G4double r_hpge, G4double r_cmos, G4double r_src);
	virtual G4VPhysicalVolume* Construct() override;
	G4LogicalVolume* logicVoxel = nullptr;
private:
	G4LogicalVolume* logicMatrixVoxel;
	G4LogicalVolume* logicHPGe;
	G4bool  fCheckOverlaps;
	void ConstructSDandField() override;
	G4UserLimits* fStepLimit;
	G4PVPlacement* fHPGePV = nullptr;
        G4PVPlacement* fCMOSPV = nullptr;
        G4PVPlacement* fSourcePV = nullptr;
        G4double fRhpge = voxel_num::HPGe_H, fRcmos = 40 + voxel_num::Voxel_H,
                 fRsrc = 800 * CLHEP::mm;
        G4ThreeVector fHPGePos0{ voxel_num::HPGe_H,0,-voxel_num::HPGe_H },
                     fCMOSPos0{ 40+voxel_num::Voxel_H,0,0};
        G4RotationMatrix fHPGeRot0, fCMOSRot0;
};




#endif