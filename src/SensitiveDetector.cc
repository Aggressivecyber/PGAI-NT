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




SensitiveDetector::SensitiveDetector(const G4String& name,G4String nfcollectionName) : G4VSensitiveDetector(name), fcollectionName(nfcollectionName),fHitsCollection(nullptr),fHCID(-1){
	collectionName.insert(fcollectionName);
}
SensitiveDetector::~SensitiveDetector()
{
}

void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	fHitsCollection = new MyHitsCollection(SensitiveDetectorName, collectionName[0]);
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
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
	hit->Setedep(step->GetTotalEnergyDeposit() / keV);
	hit->Setpos(preStep->GetPosition());
	hit->SetParticleName(track->GetParticleDefinition()->GetParticleName());
	hit->SetTime(preStep->GetGlobalTime());

	G4int nem_copyNo = -1;
	if (const auto* volume = touchable->GetVolume())
	{
		nem_copyNo = volume->GetCopyNo();
	}
	hit->SetCopyNo( nem_copyNo);

	fHitsCollection->insert(hit);
	return true;
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
		if (!hce)
	{
			G4cout << "hce is nullptr!" << G4endl;
		return;
	}
	auto man = G4AnalysisManager::Instance();
	auto evt = G4RunManager::GetRunManager()->GetCurrentEvent();
	int evtID = evt->GetEventID();
		if (!fHitsCollection) {
    G4cout << "fHitsCollection is nullptr!" << G4endl;
    return;
}
	G4int nHits = fHitsCollection->entries();
	if (nHits == 0) {
		return;
	}
			for (G4int i = 0; i < nHits; i++)
		{
			auto hit = (*fHitsCollection)[i];
				man->FillNtupleSColumn(0, 0, fcollectionName);
				man->FillNtupleIColumn(0, 1, evtID);
				man->FillNtupleDColumn(0, 2, hit->Getedep());
				if (hit->GetParticlename()=="opticalphoton")
				{
					man->FillNtupleIColumn(0, 6, hit->GetcopyNo());
				}
				else
				{
					man->FillNtupleIColumn(0, 6, -1);
				}
				man->FillNtupleDColumn(0, 7, hit->GetGlobalTime());
				man->FillNtupleSColumn(0, 8, hit->GetParticlename());
				man->AddNtupleRow(0);
			}
			fHitsCollection = nullptr;
			G4cout << evtID  << "k events processed in " << SensitiveDetectorName << G4endl;
	}

