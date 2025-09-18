#ifndef SENSITIVEDETECTOR
#define SENSITIVEDETECTOR 1

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4String.hh"
#include "G4HCofThisEvent.hh"
#include "G4THitsCollection.hh"
#include "myHit.hh"

class SensitiveDetector : public G4VSensitiveDetector {
public:
	explicit SensitiveDetector(const G4String& name, G4String nfcollectionName);
	~SensitiveDetector() override;
	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	void Initialize(G4HCofThisEvent*) override;
	void EndOfEvent(G4HCofThisEvent*) override;
private:
	G4String fcollectionName;
	MyHitsCollection* fHitsCollection;
	G4int fHCID{};
	G4int SDtag{};
};

#endif // !SENSITIVEDETECTOR
