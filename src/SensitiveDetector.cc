#include "SensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"

SensitiveDetector::SensitiveDetector(G4String name) : G4VSensitiveDetector(name) {

}
SensitiveDetector::~SensitiveDetector() {}
void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	G4double edep = step->GetTotalEnergyDeposit();
	return true;
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
	G4AnalysisManager* analysisManager = G4AnalysisManager::Instance();
}