#include "SensitiveDetector.hh"

#include <G4Run.hh>

#include "DetectorConstruction.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "VoxelNum.hh"




SensitiveDetector::SensitiveDetector(const G4String& name,G4int fSDtag) : G4VSensitiveDetector(name),SDtag(fSDtag),fHitsCollection(nullptr),fHCID(-1){
	collectionName.insert("MyHitsCollection");
}
SensitiveDetector::~SensitiveDetector() {}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	G4cout << "Init" << this->GetName() << G4endl;
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
	if (step->GetPostStepPoint()->GetStepStatus()==fGeomBoundary&&step->GetTrack()->GetDefinition()->GetParticleName()=="photon")
	{
		track->SetTrackStatus(fStopAndKill);
	}
	auto postStep = step->GetPostStepPoint();
	if (!postStep) return false;
	auto touchable = postStep->GetTouchableHandle();
	if (!touchable) return false;
	auto history1 = touchable->GetHistory();
	if (!history1) return false;
	int nem_copyNo{};
	if (touchable->GetCopyNumber())
{nem_copyNo = step->GetPostStepPoint()->GetTouchableHandle()->GetCopyNumber();}
	hit->edep = step->GetTotalEnergyDeposit()/keV;
	hit->pos = step->GetPreStepPoint()->GetPosition();
	hit->particleName = step->GetTrack()->GetParticleDefinition()->GetParticleName();
	hit->globalTime = step->GetPreStepPoint()->GetGlobalTime();
	hit->copyNo = nem_copyNo;
	G4ThreeVector local = step->GetPostStepPoint()->GetTouchableHandle()->GetHistory()->GetTopTransform().TransformPoint(hit->pos);
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
		return;
	}
	G4cout << "EndodEvent" << G4endl;
	auto man = G4AnalysisManager::Instance();
	auto evt = G4RunManager::GetRunManager()->GetCurrentEvent();
	int evtID = evt->GetEventID();
		G4int nHits = fHitsCollection->entries();
	if (!fHitsCollection)
	{
		return;
	}
		for (G4int i = 0; i < nHits; i++)
		{
			auto hit = (*fHitsCollection)[i];
			G4cout << hit << G4endl;
			G4cout << "EndodEventhit" << G4endl;
			man->FillNtupleIColumn(0, 0, SDtag);
			man->FillNtupleIColumn(0, 1, evtID);
			man->FillNtupleDColumn(0, 2, hit->edep);
			man->FillNtupleDColumn(0, 3, hit->ux);
			man->FillNtupleDColumn(0, 4, hit->uy);
			man->FillNtupleDColumn(0, 5, hit->uz);
			man->FillNtupleIColumn(0, 6, hit->copyNo);
			man->FillNtupleDColumn(0, 7, hit->globalTime);
			man->FillNtupleSColumn(0, 8, hit->particleName);
			man->AddNtupleRow(0);
		}
}