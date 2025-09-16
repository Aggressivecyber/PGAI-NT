#include "SensitiveDetector.hh"

#include <G4Run.hh>

#include "DetectorConstruction.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "RunAction.hh"
#include "VoxelNum.hh"




SensitiveDetector::SensitiveDetector(const G4String& name,G4int fSDtag) : G4VSensitiveDetector(name),SDtag(fSDtag),fHitsCollection(nullptr),fHCID(-1){
	collectionName.insert("MyHitsCollection");
}
SensitiveDetector::~SensitiveDetector() {}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	G4cout << "SensitiveDetector::Initialize called, this=" << this << G4endl;
	fHitsCollection = new MyHitsCollection(this->GetName(), collectionName[0]);
	if (fHCID<0)
	{
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
	}
	hce->AddHitsCollection(fHCID, fHitsCollection);
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	auto hit = new myHit();
	auto track = step->GetTrack();
	if (!track) return false;
	auto preStep = step->GetPreStepPoint();
	if (!preStep) return false;
	if (preStep->GetStepStatus()==fGeomBoundary)
	{
		track->SetTrackStatus(fStopAndKill);
	}
	auto postStep = step->GetPostStepPoint();
	if (!postStep) return false;
	auto touchable = postStep->GetTouchableHandle();
	if (!touchable) return false;
	auto history1 = touchable->GetHistory();
	if (!history1) return false;
	int nem_copyNo = 0;
	if (touchable)
		nem_copyNo = touchable->GetCopyNumber();
	hit->edep = step->GetTotalEnergyDeposit()/keV;
	hit->pos = preStep->GetPosition();
	hit->particleName = track->GetParticleDefinition()->GetParticleName();
	hit->globalTime =preStep->GetGlobalTime();
	hit->copyNo = nem_copyNo;
	G4ThreeVector local = history1->GetTopTransform().TransformPoint(hit->pos);
	hit->ux = local.x();
	hit->uy = local.y();
	hit->uz = local.z();
	G4cout << hit->edep << " keV " << hit->pos << " " << hit->particleName << " " << hit->globalTime << " ns " << hit->copyNo << " " << hit->ux << " " << hit->uy << " " << hit->uz << G4endl;
	fHitsCollection->insert(hit);
	return true;
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
		if (!hce)
	{
			G4cout << "hce is nullptr!" << G4endl;
		return;
	}
	G4cout << "EndOfEvent entered, fHitsCollection=" << fHitsCollection << G4endl;
	auto man = G4AnalysisManager::Instance();
	auto evt = G4RunManager::GetRunManager()->GetCurrentEvent();
	int evtID = evt->GetEventID();
		if (!fHitsCollection) {
    G4cout << "fHitsCollection is nullptr!" << G4endl;
    return;
}
	G4int nHits = fHitsCollection->entries();
	if (nHits == 0) {
		G4cout << "No hits in this event." << G4endl;
		return;
	}
			for (G4int i = 0; i < nHits; i++)
		{
			auto hit = (*fHitsCollection)[i];
			G4cout << hit << G4endl;
				G4cout << "EndodEventhit" << G4endl;
				G4cout <<" SDtag"<< SDtag << G4endl;
				man->FillNtupleIColumn(0, 0, SDtag);
				G4cout << "evtID" << evtID << G4endl;
				man->FillNtupleIColumn(0, 1, evtID);
				G4cout << "edep" << hit->edep << G4endl;
				man->FillNtupleDColumn(0, 2, hit->edep);
				G4cout << "edep" << hit->edep << G4endl;
				man->FillNtupleDColumn(0, 3, hit->ux);
				G4cout << "ux" << hit->ux << G4endl;
				man->FillNtupleDColumn(0, 4, hit->uy);
				G4cout << "uy" << hit->uy << G4endl;
				man->FillNtupleDColumn(0, 5, hit->uz);
				G4cout << "uz" << hit->uz << G4endl;
				man->FillNtupleIColumn(0, 6, hit->copyNo);
				G4cout << "copyNo" << hit->copyNo << G4endl;
				man->FillNtupleDColumn(0, 7, hit->globalTime);
				G4cout << "hit->globalTime" << hit->globalTime << G4endl;
				man->FillNtupleSColumn(0, 8, hit->particleName);
				G4cout << "hit->particleName" << hit->particleName << G4endl;
				man->AddNtupleRow(0);
			}
	}

