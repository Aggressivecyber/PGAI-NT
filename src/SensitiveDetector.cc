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
SensitiveDetector::~SensitiveDetector()
{
}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	auto man =G4SDManager::GetSDMpointer();
	fHitsCollection = new MyHitsCollection(this->GetName(), collectionName[0]);
	if (fHCID<0)
	{
		fHCID = man->GetCollectionID(collectionName[0]);
		if (fHCID < 0)
		{
			G4Exception("SensitiveDetector::Initialize", "SD001", JustWarning,
				"Failed to obtain collection ID for hits collection");
			return;
		}
	}
	hce->AddHitsCollection(fHCID, fHitsCollection);
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	auto track = step->GetTrack();
	auto preStep = step->GetPreStepPoint();
	if (!preStep)
	{
		return false;
	}

	auto touchable = preStep->GetTouchableHandle();
	if (!touchable)
	{
		return false;
	}

	auto history1 = touchable->GetHistory();
	if (!history1)
	{
		return false;
	}

	auto hit = new myHit();
	hit->edep = step->GetTotalEnergyDeposit() / keV;
	hit->pos = preStep->GetPosition();
	hit->particleName = track->GetParticleDefinition()->GetParticleName();
	hit->globalTime = preStep->GetGlobalTime();

	G4int nem_copyNo = -1;
	if (const auto* volume = touchable->GetVolume())
	{
		nem_copyNo = volume->GetCopyNo();
	}
	hit->copyNo = nem_copyNo;

	G4ThreeVector local = history1->GetTopTransform().TransformPoint(hit->pos);
	hit->ux = local.x();
	hit->uy = local.y();
	hit->uz = local.z();
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
				man->FillNtupleIColumn(0, 0, SDtag);
				man->FillNtupleIColumn(0, 1, evtID);
				man->FillNtupleDColumn(0, 2, hit->edep);
				man->FillNtupleDColumn(0, 3, hit->ux);
				man->FillNtupleDColumn(0, 4, hit->uy);
				man->FillNtupleDColumn(0, 5, hit->uz);
				if (hit->particleName=="opticalphoton")
				{
					man->FillNtupleIColumn(0, 6, hit->copyNo);
					G4cout << "copyNo" << hit->copyNo << G4endl;
				}
				else
				{
					man->FillNtupleIColumn(0, 6, -1);
				}
				man->FillNtupleDColumn(0, 7, hit->globalTime);
				man->FillNtupleSColumn(0, 8, hit->particleName);
				man->AddNtupleRow(0);
			}
	}

