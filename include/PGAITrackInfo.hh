#ifndef PGAI_TRACK_INFO_HH
#define PGAI_TRACK_INFO_HH

#include "G4VUserTrackInformation.hh"
#include "globals.hh"

// 跟踪中子是否在样品区发生过强子相互作用。
// 修复核心硬伤: parentID==0 不等于"未散射" — 散射后的初级中子仍是同一 track。
// 只有 parentID==0 且未在 phantom 内相互作用, 才是真"未碰撞透射初级中子"。
class PGAITrackInfo : public G4VUserTrackInformation {
public:
	G4bool interactedInPhantom{false};
	G4String lastPhantomProcess{""};
	G4bool bornInPhantom{false};
	G4String sourceMaterial{""};
	G4String sourceProcess{""};
	G4double sourceGammaEnergyKeV{-1.0};
	G4double sourceBiasWeight{1.0};
	void Print() const override {}
};

#endif
