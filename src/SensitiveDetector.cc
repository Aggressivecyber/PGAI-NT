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




SensitiveDetector::SensitiveDetector(G4String& name,G4int fSDtag) : G4VSensitiveDetector(name),SDtag(fSDtag),fHitsCollection(nullptr),fHCID(-1){
	collectionName.insert("MyHitsCollection");
}
SensitiveDetector::~SensitiveDetector() {}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	fHitsCollection = new MyHitsCollection(SensitiveDetectorName, collectionName[0]);
	if (fHCID<0)
	{
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection);
	}
	hce->AddHitsCollection(fHCID, fHitsCollection);
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	auto hit = new myHit();
	auto track = step->GetTrack();
	if (step->GetPostStepPoint()->GetStepStatus()==fGeomBoundary)
	{
		track->SetTrackStatus(fStopAndKill);
	}
	int copyNo = step->GetPostStepPoint()->GetTouchableHandle()->GetCopyNumber();
	hit->edep = step->GetTotalEnergyDeposit()/keV;
	hit->pos = step->GetPreStepPoint()->GetPosition();
	hit->particleName = step->GetTrack()->GetParticleDefinition()->GetParticleName();
	hit->globalTime = step->GetPreStepPoint()->GetGlobalTime();
	voxelNum vnum;
	hit->num_X =vnum.GetNumX(copyNo);
	hit->num_Y =vnum.GetNumY(copyNo);
	G4cout << vnum.GetNumX(copyNo) << G4endl;
	G4cout << vnum.GetNumY(copyNo) << G4endl;
	G4ThreeVector local = step->GetPostStepPoint()->GetTouchableHandle()->GetHistory()->GetTopTransform().TransformPoint(hit->pos);
	hit->ux = local.x();
	hit->uy = local.y();
	hit->uz = local.z();
	fHitsCollection->insert(hit);
	return true;
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
	auto man = G4AnalysisManager::Instance();
	auto evt = G4RunManager::GetRunManager()->GetCurrentEvent();
	int evtID = evt->GetEventID();
	if (fHitsCollection)
	{
		G4int nHits = fHitsCollection->entries();
		for (G4int i = 0; i < nHits; i++)
		{
			auto hit = (*fHitsCollection)[i];
			man->FillNtupleIColumn(0, 0, SDtag);
			man->FillNtupleIColumn(0, 1, evtID);
			man->FillNtupleDColumn(0, 2, hit->edep);
			man->FillNtupleDColumn(0, 3, hit->ux);
			man->FillNtupleDColumn(0, 4, hit->uy);
			man->FillNtupleDColumn(0, 5, hit->uz);
			man->FillNtupleIColumn(0, 6, hit->num_X);
			man->FillNtupleIColumn(0, 7, hit->num_Y);
			man->FillNtupleDColumn(0, 8, hit->globalTime);
			man->FillNtupleSColumn(0, 9, hit->particleName);
			man->AddNtupleRow(0);
		}
	}
}