#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH 1
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "DetectorConstruction.hh"

class G4Event;
class G4ParticleGun;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
	PrimaryGeneratorAction(DetectorConstruction*);
	~PrimaryGeneratorAction() override;
	void GeneratePrimaries(G4Event* event) override;
	void SetParticleGun(G4ParticleGun* gun);
	G4ParticleGun* GetParticleGun() const;
	DetectorConstruction* temp_dec;
private:

	G4ParticleGun* fParticleGun; // Particle gun for generating primary particles
};

#endif