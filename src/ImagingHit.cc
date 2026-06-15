#include "ImagingHit.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

G4ThreadLocal G4Allocator<ImagingHit>* ImagingHitAllocator = nullptr;

void ImagingHit::Print() {
	G4cout << "ImagingHit pixel(" << pixelX << "," << pixelY
	       << ") edep=" << edep << " keV particle=" << particleName
	       << " ekin=" << ekin << " MeV" << G4endl;
}
