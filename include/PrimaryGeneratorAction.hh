#ifndef PRIMARYGENERATORACTION_HH
#define PRIMARYGENERATORACTION_HH 1
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "DetectorConstruction.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
class PrimaryGeneratorMessenger;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction {
public:
	PrimaryGeneratorAction();
	~PrimaryGeneratorAction() override;

	void GeneratePrimaries(G4Event* evt) override;

	G4double GetPhi0() { return fPhi0; }
	G4double GetDphi() { return fDphi; }

	void SetRadius(G4double r) { fRsrc = r; }
	void SetNPerEvent(G4int n) { fNperEvt = n; }
	void SetPhi0(G4double v) { fPhi0 = v; }
	void SetDphi(G4double v) { fDphi = v; }
	void SetPhiCenter(G4double v) { fPhiCenter = v; }

private:
	G4double fRsrc = 20. * CLHEP::mm;  
	G4int    fNperEvt = 1;            
	G4double fPhiCenter = 20. * CLHEP::deg; 
	G4double fRlength = 20*CLHEP::mm; 


	G4ParticleGun* fParticlegun = nullptr;
	G4double fEk = 1.8*0.001 * CLHEP::eV;
	G4double fPhi0 = 0. * CLHEP::deg;
	G4double fDphi = 5. * CLHEP::deg;
	PrimaryGeneratorMessenger* fMessenger = nullptr;

	G4double CurrentPhiCenter() const;
	G4ThreeVector RadialDir(G4double phi) const;
	G4ThreeVector SamplePosOnRing(G4double phi) const;
};
#endif