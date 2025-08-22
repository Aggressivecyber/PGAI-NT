#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UniformRandPool.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(){
	fParticleGun = new G4ParticleGun(1); 
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron")); 
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
	delete fParticleGun; // Clean up the particle gun
}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
	G4int nParticleGun = 1;
	for (G4int i = 0; i < nParticleGun; i++) {
		G4double Screen_L = 70 * mm;
		fParticleGun->SetParticlePosition(G4ThreeVector(-800, (Screen_L)*(0.5-G4UniformRand()), (Screen_L)*(0.5-G4UniformRand())));
		fParticleGun->SetParticleMomentumDirection(G4ThreeVector(1, 0, 0));
		//G4double minEnergy = 0.1 * CLHEP::MeV;
		//G4double maxEnergy = 10.0 * CLHEP::MeV;
		fParticleGun->SetParticleEnergy(4.05*CLHEP::MeV);
		fParticleGun->GeneratePrimaryVertex(event);
	}
}