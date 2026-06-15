#ifndef FAST_NEUTRON_TRANSMISSION_SD_HH
#define FAST_NEUTRON_TRANSMISSION_SD_HH

#include "G4VSensitiveDetector.hh"
#include "ImagingHit.hh"

// Channel A: 快中子透射探测器
// 单个塑料闪烁体逻辑体, 像素索引在 ProcessHits 中根据局部坐标计算 (不建实体 voxel 阵列)
class FastNeutronTransmissionSD : public G4VSensitiveDetector {
public:
	explicit FastNeutronTransmissionSD(const G4String& name);
	~FastNeutronTransmissionSD() override = default;

	void Initialize(G4HCofThisEvent* hce) override;
	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	void EndOfEvent(G4HCofThisEvent* hce) override;

private:
	TransmissionHitsCollection* fHC{nullptr};
	G4int fHCID{-1};
};

#endif
