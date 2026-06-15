#include "SteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4StepPoint.hh"
#include "G4ParticleDefinition.hh"
#include "G4Material.hh"
#include "G4VProcess.hh"
#include "PGAITrackInfo.hh"

// 判断是否为样品材料 (排除空气/真空/探测材料)
static G4bool IsPhantomMaterial(const G4String& m) {
	return m != "G4_AIR" && m != "G4_Galactic"
	    && m != "G4_PLASTIC_SC_VINYLTOLUENE" && m != "G4_Ge";
}

void SteppingAction::UserSteppingAction(const G4Step* step) {
	auto track = step->GetTrack();
	if (track->GetParticleDefinition()->GetParticleName() != "neutron") return;

	auto pre = step->GetPreStepPoint();
	auto mat = pre->GetMaterial();
	if (!mat || !IsPhantomMaterial(mat->GetName())) return;

	auto post = step->GetPostStepPoint();
	auto proc = post->GetProcessDefinedStep();
	if (!proc) return;

	G4String pn = proc->GetProcessName();
	// 强子相互作用 (中子散射/俘获/裂变)
	G4bool hadronic = pn.contains("Elastic") || pn.contains("Inelastic")
	               || pn.contains("Capture") || pn.contains("Fission");
	if (!hadronic) return;

	auto info = const_cast<PGAITrackInfo*>(
		static_cast<const PGAITrackInfo*>(track->GetUserInformation()));
	if (!info) {
		info = new PGAITrackInfo();
		const_cast<G4Track*>(track)->SetUserInformation(info);
	}
	info->interactedInPhantom = true;
	info->lastPhantomProcess = pn;
}
