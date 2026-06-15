#include "HPGeHit.hh"
#include "G4SystemOfUnits.hh"

G4ThreadLocal G4Allocator<HPGeHit>* HPGeHitAllocator = nullptr;

void HPGeHit::Print() {
	G4cout << "HPGeHit edep=" << edep << " keV particle=" << particleName
	       << " ekin=" << ekin << " MeV" << G4endl;
}
