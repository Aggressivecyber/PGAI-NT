#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UniformRandPool.hh"
#include "G4SystemOfUnits.hh"
#include "G4RotationMatrix.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(){
	fParticleGun = new G4ParticleGun(1); 
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron")); 
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
        delete fParticleGun; // Clean up the particle gun
}
void PrimaryGeneratorAction::SetParticleGun(G4ParticleGun* gun) {
        fParticleGun = gun;
}
G4ParticleGun* PrimaryGeneratorAction::GetParticleGun() const {
        return fParticleGun;
}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
        G4double screenSize = 70 * mm;
        G4double y = screenSize * (0.5 - G4UniformRand());
        G4double z = screenSize * (0.5 - G4UniformRand());
        G4ThreeVector position(-fSourceRadius, y, z);
        G4RotationMatrix rot;
        rot.rotateZ(fAngle);
        position = rot * position;
        G4ThreeVector direction = (-position).unit();
        fParticleGun->SetParticlePosition(position);
        fParticleGun->SetParticleMomentumDirection(direction);
        fParticleGun->SetParticleEnergy(4.05 * CLHEP::MeV);
        fParticleGun->GeneratePrimaryVertex(event);
}

void PrimaryGeneratorAction::SetAngle(G4double angle) {
        fAngle = angle;
}
