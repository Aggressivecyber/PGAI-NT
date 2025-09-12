#ifndef SENSITIVEDETECTOR
#define SENSITIVEDETECTOR 1

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "DetectorConstruction.hh"
#include "G4String.hh"
#include "G4HCofThisEvent.hh"
#include "G4THitsCollection.hh"

class SensitiveDetector : public G4VSensitiveDetector {
public:
	explicit SensitiveDetector(G4String name,G4int nx=1,G4int ny=1);
	~SensitiveDetector();

	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	virtual void Initialize(G4HCofThisEvent*);
	virtual void EndOfEvent(G4HCofThisEvent*);
private:
	std::ofstream ofs;
	G4String sdName;
	G4int mNx = 1;
	G4int mNy = 1;
	bool wroteHeader = false;
	void WriteHeaderIfNeeded();
};

#endif // !SENSITIVEDETECTOR
