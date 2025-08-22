#include "SensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"
#include "Hit.hh"

SensitiveDetector::SensitiveDetector(G4String name) : G4VSensitiveDetector(name) {
	fEdep = 0;
}
SensitiveDetector::~SensitiveDetector() {
	// Destructor implementation if needed
}
G4int fHCID = -1;
void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	hitsCollection = new G4THitsCollection<Hit>("matrixSD", collectionName[0]);
	if (fHCID < 0) {
		auto fullname = "matrixSD" + "/" + collectionName[0];
		fHCID = G4SDManager::GetSDMpointer()->GetCollectionID(fullname);
                hce->AddHitsCollection(fHCID, hitsCollection);
        }
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
	// Code to execute at the end of the event
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
}
G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	G4double edep = step->GetTotalEnergyDeposit();
	if (edep == 0.) return false;

        Hit* myHit = new Hit();
        myHit->SetEdep(edep);
        myHit->SetPos(step->GetPreStepPoint()->GetPosition());

        hitsCollection->insert(myHit);
	return true; // Return true to indicate the hit was processed
}