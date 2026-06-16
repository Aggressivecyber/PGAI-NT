#include "SteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4ParticleDefinition.hh"
#include "G4Material.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "Randomize.hh"
#include "PGAIConfig.hh"
#include "PGAITrackInfo.hh"

#include <algorithm>
#include <cmath>

static G4bool HasText(const G4String& text, const char* needle) {
	return text.find(needle) != std::string::npos;
}

static G4double ConeSolidAngleFraction(G4double halfAngle) {
	G4double a = std::clamp(halfAngle, 0.0, CLHEP::pi);
	return 0.5 * (1.0 - std::cos(a));
}

static G4ThreeVector RandomDirectionInCone(const G4ThreeVector& axis, G4double halfAngle) {
	G4double a = std::clamp(halfAngle, 0.0, CLHEP::pi);
	G4double cosMin = std::cos(a);
	G4double cosTheta = 1.0 - G4UniformRand() * (1.0 - cosMin);
	G4double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));
	G4double phi = CLHEP::twopi * G4UniformRand();
	G4ThreeVector dir(sinTheta * std::cos(phi), sinTheta * std::sin(phi), cosTheta);
	dir.rotateUz(axis.unit());
	return dir.unit();
}

// 判断是否为样品体积。不能只按材料判断, 否则 Pb 准直器 / Al 杜瓦也会被算成样品。
static G4bool IsPhantomVolume(const G4String& v) {
	return v == "SinglePhantom"
	    || v == "CalibrationBlock"
	    || v == "SteelShell"
	    || v == "CTPhantom"
	    || HasText(v, "GradientCylinder_")
	    || HasText(v, "Deg_")
	    || HasText(v, "Fill_")
	    || HasText(v, "CTRod_");
}

static void CopyInfoToTrack(G4Track* track, const PGAITrackInfo* src) {
	if (!track || !src) return;
	auto* dst = static_cast<PGAITrackInfo*>(track->GetUserInformation());
	if (!dst) {
		dst = new PGAITrackInfo();
		track->SetUserInformation(dst);
	}
	*dst = *src;
}

static void PropagateSampleGammaInfo(const G4Step* step, const PGAITrackInfo* info) {
	if (!info || !info->bornInPhantom) return;
	auto secondaries = step->GetSecondaryInCurrentStep();
	if (!secondaries) return;
	for (const auto* secondaryConst : *secondaries) {
		auto* secondary = const_cast<G4Track*>(secondaryConst);
		CopyInfoToTrack(secondary, info);
	}
}

static void ApplyPromptGammaConeBias(G4Track* secondary, PGAITrackInfo& gammaInfo) {
	if (!secondary || !gConfig.gammaConeBias || gConfig.gammaConeBiasAngle <= 0) return;

	G4ThreeVector hpgeTarget(0.0, gConfig.hpgeCenterY + gConfig.hpgeDistance, gConfig.hpgeCenterZ);
	G4ThreeVector axis = hpgeTarget - secondary->GetPosition();
	if (axis.mag2() <= 0) return;

	secondary->SetMomentumDirection(RandomDirectionInCone(axis, gConfig.gammaConeBiasAngle));
	gammaInfo.sourceBiasWeight = ConeSolidAngleFraction(gConfig.gammaConeBiasAngle);
	secondary->SetWeight(secondary->GetWeight() * gammaInfo.sourceBiasWeight);
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
	auto track = step->GetTrack();
	auto* existingInfo = static_cast<PGAITrackInfo*>(track->GetUserInformation());
	PropagateSampleGammaInfo(step, existingInfo);

	if (track->GetParticleDefinition()->GetParticleName() != "neutron") return;

	auto pre = step->GetPreStepPoint();
	auto mat = pre->GetMaterial();
	auto volume = pre->GetTouchableHandle() ? pre->GetTouchableHandle()->GetVolume() : nullptr;
	if (!mat || !volume || !IsPhantomVolume(volume->GetName())) return;

	auto post = step->GetPostStepPoint();
	auto proc = post->GetProcessDefinedStep();
	if (!proc) return;

	G4String pn = proc->GetProcessName();
	// 强子相互作用 (中子散射/俘获/裂变)
	G4bool hadronic = HasText(pn, "Elastic") || HasText(pn, "Inelastic")
	               || HasText(pn, "Capture") || HasText(pn, "Fission");
	if (!hadronic) return;

	auto info = const_cast<PGAITrackInfo*>(
		static_cast<const PGAITrackInfo*>(track->GetUserInformation()));
	if (!info) {
		info = new PGAITrackInfo();
		const_cast<G4Track*>(track)->SetUserInformation(info);
	}
	info->interactedInPhantom = true;
	info->lastPhantomProcess = pn;

	auto secondaries = step->GetSecondaryInCurrentStep();
	if (!secondaries) return;
	for (const auto* secondaryConst : *secondaries) {
		auto* secondary = const_cast<G4Track*>(secondaryConst);
		if (secondary->GetParticleDefinition()->GetParticleName() != "gamma") continue;

		PGAITrackInfo gammaInfo;
		gammaInfo.interactedInPhantom = true;
		gammaInfo.lastPhantomProcess = pn;
		gammaInfo.bornInPhantom = true;
		gammaInfo.sourceMaterial = mat->GetName();
		gammaInfo.sourceProcess = pn;
		gammaInfo.sourceGammaEnergyKeV = secondary->GetKineticEnergy() / keV;
		ApplyPromptGammaConeBias(secondary, gammaInfo);
		CopyInfoToTrack(secondary, &gammaInfo);
	}
}
