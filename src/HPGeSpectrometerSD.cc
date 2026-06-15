#include "HPGeSpectrometerSD.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4Track.hh"
#include "G4VProcess.hh"
#include "G4SystemOfUnits.hh"

HPGeSpectrometerSD::HPGeSpectrometerSD(const G4String& name)
	: G4VSensitiveDetector(name) {
	collectionName.insert("HPGeHC");
}

void HPGeSpectrometerSD::Initialize(G4HCofThisEvent* hce) {
	fHC = new HPGeHitsCollection(SensitiveDetectorName, collectionName[0]);
	if (fHCID < 0) {
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHC);
	}
	hce->AddHitsCollection(fHCID, fHC);
}

G4bool HPGeSpectrometerSD::ProcessHits(G4Step* step, G4TouchableHistory* /*history*/) {
	G4double edep = step->GetTotalEnergyDeposit() / keV;
	if (edep <= 0) return false;  // HPGe 只关心实际能量沉积

	auto track = step->GetTrack();
	auto pre = step->GetPreStepPoint();

	auto hit = new HPGeHit();
	hit->edep = edep;
	hit->particleName = track->GetParticleDefinition()->GetParticleName();
	hit->ekin = pre->GetKineticEnergy() / MeV;
	const G4VProcess* creator = track->GetCreatorProcess();
	hit->creatorProcess = creator ? creator->GetProcessName() : "primary";

	fHC->insert(hit);
	return true;
}

void HPGeSpectrometerSD::EndOfEvent(G4HCofThisEvent* /*hce*/) {}
