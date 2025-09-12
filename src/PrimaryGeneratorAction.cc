#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UniformRandPool.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "G4RotationMatrix.hh"
#include "DetectorConstruction.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(){
	fParticleGun = new G4ParticleGun(1); 
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron")); 
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
	delete fParticleGun; // Clean up the particle gun
}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
        auto det = static_cast<DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
        G4ThreeVector srcCenter = det->GetSourcePosition();
        G4double screenHalfLength = 70 * mm;
        G4double yLocal = screenHalfLength * (0.5 - G4UniformRand());
        G4double zLocal = screenHalfLength * (0.5 - G4UniformRand());
        G4RotationMatrix rotZ;
        rotZ.rotateZ(det->GetCurrentAngle());
        G4ThreeVector offset = rotZ * G4ThreeVector(0, yLocal, zLocal);
        G4ThreeVector srcPos = srcCenter + offset;
        G4ThreeVector momDir = -srcPos.unit();
        fParticleGun->SetParticlePosition(srcPos);
        fParticleGun->SetParticleMomentumDirection(momDir);
        fParticleGun->SetParticleEnergy(4.05 * CLHEP::MeV);
        fParticleGun->GeneratePrimaryVertex(event);
}