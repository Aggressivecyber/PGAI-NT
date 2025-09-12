#ifndef DETECTORCONSTRUCTION
#define DETECTORCONSTRUCTION 1
#include "G4VUserDetectorConstruction.hh"
#include "G4UserLimits.hh"
#include "globals.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"

namespace voxel_num
{
        inline G4int voxelNx = 0;
        inline G4int voxelNy = 0;
        // Half thickness of a single CMOS voxel
        inline G4double voxelHalfLength{ 0 };
        // Half length of the HPGe detector crystal
        inline G4double hpgeHalfLength{ 0 };
}

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
        void RotateRig(G4double degZ);
        G4ThreeVector GetSourcePosition() const;
        G4double GetCurrentAngle() const { return fCurrentAngle; }
        virtual G4VPhysicalVolume* Construct() override;
        G4LogicalVolume* logicVoxel = nullptr;
private:
        G4LogicalVolume* logicMatrixVoxel;
        G4LogicalVolume* logicHPGe;
        G4bool  fCheckOverlaps;
        void ConstructSDandField() override;
        G4UserLimits* fStepLimit;

        // Physical volumes for rotating components
        G4PVPlacement* fHPGePV = nullptr;
        G4PVPlacement* fCMOSPV = nullptr;
        G4PVPlacement* fSourcePV = nullptr;

        // Radii from sample to each component
        G4double fRhpge = voxel_num::hpgeHalfLength;
        G4double fRcmos = 40 + voxel_num::voxelHalfLength;
        G4double fRsrc  = 800 * CLHEP::mm;

        // Initial positions and rotations
        G4ThreeVector fHPGePos0{ 0,0,-voxel_num::hpgeHalfLength };
        G4ThreeVector fCMOSPos0{ 40 + voxel_num::voxelHalfLength,0,0 };
        G4ThreeVector fSourcePos0{ -fRsrc,0,0 };
        G4RotationMatrix fHPGeRot0, fCMOSRot0, fSourceRot0;

        // Current accumulated rotation angle (radians)
        G4double fCurrentAngle = 0.;
};




#endif