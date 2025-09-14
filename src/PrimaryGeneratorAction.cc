#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UniformRandPool.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* fdec):temp_dec(fdec){
	fParticleGun = new G4ParticleGun(1); 
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron"));
	
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
	delete fParticleGun; // Clean up the particle gun

}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {

		G4double Screen_L = 70 * mm;
		fParticleGun->SetParticlePosition(G4ThreeVector((-800)*std::cos(temp_dec->getDeg()*CLHEP::pi/180), (((Screen_L)*(0.5-G4UniformRand())-800)*std::sin(temp_dec->getDeg() * CLHEP::pi / 180)), (Screen_L)*(0.5-G4UniformRand())));
		fParticleGun->SetParticleMomentumDirection(G4ThreeVector(std::cos(temp_dec->getDeg() * CLHEP::pi / 180), std::sin(temp_dec->getDeg() * CLHEP::pi / 180), 0));
		fParticleGun->SetParticleEnergy(4.05*CLHEP::MeV);
		fParticleGun->GeneratePrimaryVertex(event);

}