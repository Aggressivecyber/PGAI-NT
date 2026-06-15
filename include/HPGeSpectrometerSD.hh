#ifndef HPGEO_SPECTROMETER_SD_HH
#define HPGEO_SPECTROMETER_SD_HH

#include "G4VSensitiveDetector.hh"
#include "HPGeHit.hh"

// Channel B: HPGe 瞬发伽马谱探测器
// 每个 step 记录到 HPGeHit collection, 由 EventAction 在 event 末尾合并为一条事件级能谱记录
class HPGeSpectrometerSD : public G4VSensitiveDetector {
public:
	explicit HPGeSpectrometerSD(const G4String& name);
	~HPGeSpectrometerSD() override = default;

	void Initialize(G4HCofThisEvent* hce) override;
	G4bool ProcessHits(G4Step* step, G4TouchableHistory* history) override;
	void EndOfEvent(G4HCofThisEvent* hce) override;

private:
	HPGeHitsCollection* fHC{nullptr};
	G4int fHCID{-1};
};

#endif
