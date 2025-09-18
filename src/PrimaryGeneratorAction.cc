#include "PrimaryGeneratorAction.hh"
#include "G4RunManager.hh"
#include "G4Neutron.hh"
#include "G4ParticleGun.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <cmath>
#include "PrimaryGeneratorMessenger.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction() {
    fParticlegun = new G4ParticleGun(1);
    fParticlegun->SetParticleDefinition(G4Neutron::Definition());
    fParticlegun->SetParticleEnergy(fEk);
    fMessenger = new PrimaryGeneratorMessenger(this);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() { 
    delete fParticlegun;  
    delete fMessenger;
}


G4double PrimaryGeneratorAction::CurrentPhiCenter() const {
    return fPhiCenter; 
}
G4ThreeVector PrimaryGeneratorAction::RadialDir(G4double phi) const
{
	return G4ThreeVector(-std::cos(phi), -std::sin(phi), 0.);
}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {

G4ThreeVector PrimaryGeneratorAction::SamplePosOnRing(G4double phi) const {
    const G4double fRlth = 2*(fRlength) *( 0.5-G4UniformRand());
    return { fRsrc * std::cos(phi)+ fRlth *std::sin(-phi), fRsrc * std::sin(phi)+ fRlth * std::cos(-phi), 0};
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* evt) {
        fParticlegun->SetParticleEnergy(fEk);
        const G4double phi = CurrentPhiCenter();
        const auto dir = RadialDir(phi);
        const auto pos = SamplePosOnRing(phi);
        fParticlegun->SetParticleMomentumDirection(dir);
        fParticlegun->SetParticlePosition(pos);
        fParticlegun->GeneratePrimaryVertex(evt);
}
