#ifndef SENSITIVEDETECTOR
#define SENSITIVEDETECTOR 1

#include "G4VSensitiveDetector.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4String.hh"
#include "G4HCofThisEvent.hh"
#include "G4THitsCollection.hh"


class SensitiveDetector : public G4VSensitiveDetector {
public:
	explicit SensitiveDetector(G4String name);
	~SensitiveDetector();

	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	virtual void Initialize(G4HCofThisEvent*);
	virtual void EndOfEvent(G4HCofThisEvent*);
private:
	std::ofstream ofs;
	G4String sdName;
	bool wroteHeader = false;
	void WriteHeaderIfNeeded();
	G4int eventID{0};
	G4int copyNo{999};
	G4double edep_keV{ 0.0 };
	G4double Ekin_keV{ 0.0 };
	G4double globalTime_ns{ 0.0 };

};

#endif // !SENSITIVEDETECTOR
