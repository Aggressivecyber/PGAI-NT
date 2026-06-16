#ifndef HPGEO_HIT_HH
#define HPGEO_HIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4String.hh"

// HPGe 探测器单 step 命中 (Channel B) — event 级合并后形成能谱
class HPGeHit : public G4VHit {
public:
	HPGeHit() = default;
	~HPGeHit() override = default;

	void Draw() override {}
	void Print() override;

	inline void* operator new(size_t);
	inline void  operator delete(void*);

	G4double edep{0};     // keV
	G4String particleName;
	G4double ekin{0};     // MeV
	G4String creatorProcess;
	G4bool fromPhantomGamma{false};
	G4double sampleGammaEdep{0};          // keV
	G4double sourceGammaEnergyKeV{-1.0};  // keV
	G4double sampleGammaBiasWeight{1.0};
	G4String sourceMaterial;
	G4String sourceProcess;
};

using HPGeHitsCollection = G4THitsCollection<HPGeHit>;

extern G4ThreadLocal G4Allocator<HPGeHit>* HPGeHitAllocator;

inline void* HPGeHit::operator new(size_t) {
	if (!HPGeHitAllocator) HPGeHitAllocator = new G4Allocator<HPGeHit>;
	return (void*)HPGeHitAllocator->MallocSingle();
}
inline void HPGeHit::operator delete(void* hit) {
	HPGeHitAllocator->FreeSingle((HPGeHit*)hit);
}

#endif
