#ifndef HIT_HH
#define HIT_HH 1

#include "G4VHit.hh"
#include "G4ThreeVector.hh"
#include "G4String.hh"
#include "G4Allocator.hh"
#include "G4SystemOfUnits.hh"



class Hit : public G4VHit {
public:
	Hit() :edep(0.) {};
	virtual ~Hit() {};
	void SetEdep(G4double e) { edep = e; }
	G4double GetEdep() const { return edep; }
	void SetPos(G4ThreeVector p) { pos = p; }
	G4ThreeVector GetPos() const { return pos; }
	void SetName(G4String n) { name = n; }
	G4String GetName() const { return name; }
private:
	G4double edep;
	G4ThreeVector pos;
	G4String name;
}

#endif