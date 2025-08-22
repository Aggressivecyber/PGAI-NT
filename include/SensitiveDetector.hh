#ifndef SENSITIVEDETECTOR
#define SENSITIVEDETECTOR 1

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "DetectorConstruction.hh"
#include "G4String.hh"
#include "G4HCofThisEvent.hh"
#include "G4THitsCollection.hh"
#include "Hit.hh"

class SensitiveDetector : public G4VSensitiveDetector {
public:
	SensitiveDetector(G4String name):G4VSensitiveDetector(name) {};
	~SensitiveDetector() {};

	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	virtual void Initialize(G4HCofThisEvent*);
	virtual void EndOfEvent(G4HCofThisEvent*);
};

#endif // !SENSITIVEDETECTOR
