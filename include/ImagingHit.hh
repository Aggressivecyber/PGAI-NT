#ifndef IMAGING_HIT_HH
#define IMAGING_HIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4String.hh"

// 快中子透射探测器单 step 命中 (Channel A)
class ImagingHit : public G4VHit {
public:
	ImagingHit() = default;
	~ImagingHit() override = default;

	void Draw() override {}
	void Print() override;

	inline void* operator new(size_t);
	inline void  operator delete(void*);

	G4double edep{0};          // keV
	G4ThreeVector localPos;    // 闪烁体内局部坐标 (mm)
	G4int pixelX{0};
	G4int pixelY{0};
	G4double time{0};          // ns
	G4String particleName;
	G4int trackID{0};
	G4int parentID{0};
	G4String creatorProcess;
	G4double ekin{0};          // MeV (step 中点)
	G4bool isPrimaryNeutron{false};
	G4bool isScatteredNeutron{false};
};

using TransmissionHitsCollection = G4THitsCollection<ImagingHit>;

extern G4ThreadLocal G4Allocator<ImagingHit>* ImagingHitAllocator;

inline void* ImagingHit::operator new(size_t) {
	if (!ImagingHitAllocator) ImagingHitAllocator = new G4Allocator<ImagingHit>;
	return (void*)ImagingHitAllocator->MallocSingle();
}
inline void ImagingHit::operator delete(void* hit) {
	ImagingHitAllocator->FreeSingle((ImagingHit*)hit);
}

#endif
