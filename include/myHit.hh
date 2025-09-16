# ifndef MYHIT_HH
# define MYHIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

class myHit : public G4VHit{
public:
	myHit();
	virtual ~myHit() override;

	void Draw() override;
	void Print() override;

	G4double edep{};
	G4ThreeVector pos{};
	G4String particleName{};
	G4double globalTime{};
	G4int copyNo{};
	G4double ux{};
	G4double uy{};
	G4double uz{};

};

using MyHitsCollection = G4THitsCollection<myHit>;




#endif
