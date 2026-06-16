#include "FastNeutronTransmissionSD.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4ThreeVector.hh"
#include "PGAIConfig.hh"
#include "PGAITrackInfo.hh"
#include <cmath>

FastNeutronTransmissionSD::FastNeutronTransmissionSD(const G4String& name)
	: G4VSensitiveDetector(name) {
	collectionName.insert("TransmissionHC");
}

void FastNeutronTransmissionSD::Initialize(G4HCofThisEvent* hce) {
	fHC = new TransmissionHitsCollection(SensitiveDetectorName, collectionName[0]);
	if (fHCID < 0) {
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHC);
	}
	hce->AddHitsCollection(fHCID, fHC);
}

G4bool FastNeutronTransmissionSD::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/) {
	// 仅记录有能量沉积的 step (闪烁体通过反冲带电粒子沉积)
	G4double edep = step->GetTotalEnergyDeposit() / keV;
	auto track = step->GetTrack();
	auto pre = step->GetPreStepPoint();

	auto hit = new ImagingHit();
	hit->edep = edep;
	hit->time = pre->GetGlobalTime() / ns;
	hit->particleName = track->GetParticleDefinition()->GetParticleName();
	hit->trackID = track->GetTrackID();
	hit->parentID = track->GetParentID();

	const G4VProcess* creator = track->GetCreatorProcess();
	hit->creatorProcess = creator ? creator->GetProcessName() : "primary";

	hit->ekin = pre->GetKineticEnergy() / MeV;

	// 连续屏: 像素索引由局部坐标算 (模拟 CCD 读出像素化), 局部 y-z 为成像面
	auto touch = pre->GetTouchableHandle();
	G4ThreeVector local = touch->GetHistory()->GetTopTransform().TransformPoint(pre->GetPosition());
	hit->localPos = local;

	G4double detSize = gConfig.detectorSize;
	G4double half = detSize * 0.5;
	G4int nx = gConfig.pixelsX;
	G4int ny = gConfig.pixelsY;
	G4double dx = detSize / nx;
	G4double dy = detSize / ny;
	G4int px = (G4int)std::floor((local.y() + half) / dx);
	G4int py = (G4int)std::floor((local.z() + half) / dy);
	if (px < 0) px = 0; else if (px >= nx) px = nx - 1;
	if (py < 0) py = 0; else if (py >= ny) py = ny - 1;
	hit->pixelX = px;
	hit->pixelY = py;

	// 未散射/散射中子标记 (基于 TrackInfo, 不再用 parentID==0 误判)
	// uncollided primary = 中子 && parentID==0 && 未在 phantom 内发生强子相互作用
	G4bool isNeutron = (hit->particleName == "neutron");
	auto info = static_cast<PGAITrackInfo*>(track->GetUserInformation());
	G4bool interacted = info && info->interactedInPhantom;
	hit->isPrimaryNeutron = isNeutron && (hit->parentID == 0) && !interacted;
	hit->isScatteredNeutron = isNeutron && interacted;

	// 方向 + 严格未碰撞门控 (前向 + 近源能量)
	G4ThreeVector dir = pre->GetMomentumDirection();
	hit->dirX = dir.x();
	hit->dirY = dir.y();
	hit->dirZ = dir.z();
	G4bool forward = dir.x() > 0.999;
	G4bool nearE = std::abs(hit->ekin - gConfig.energy / MeV) < 0.05;
	hit->isUncollidedPrimary = hit->isPrimaryNeutron && forward && nearE;

	fHC->insert(hit);
	return true;
}

void FastNeutronTransmissionSD::EndOfEvent(G4HCofThisEvent* /*hce*/) {}
