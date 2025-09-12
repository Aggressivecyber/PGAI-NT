#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH 1
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"

class G4Event;
class G4ParticleGun;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
        PrimaryGeneratorAction();
        ~PrimaryGeneratorAction() override;
        void GeneratePrimaries(G4Event* event) override;
        void SetParticleGun(G4ParticleGun* gun);
        G4ParticleGun* GetParticleGun() const;
        void SetAngle(G4double angle);
private:
        G4ParticleGun* fParticleGun; // Particle gun for generating primary particles
        G4double fAngle = 0.;
        G4double fSourceRadius = 800 * CLHEP::mm;
};

#endif
