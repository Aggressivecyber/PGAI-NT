#include "EventAction.hh"
#include "G4Event.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include "ImagingHit.hh"
#include "HPGeHit.hh"
#include "PGAIConfig.hh"

#include <map>
#include <set>
#include <cmath>

EventAction::EventAction() : G4UserEventAction() {}

G4double EventAction::SmearEnergy(G4double e_keV) const {
	if (!gConfig.smearHPGe || e_keV <= 0) return e_keV;
	// FWHM(E) = sqrt(a^2 + b*E + c*E^2), 单位 keV
	G4double a = gConfig.resA / keV;
	G4double b = gConfig.resB;   // b 单位 keV^-1 * (keV^2)?
	G4double c = gConfig.resC;
	G4double fwhm = std::sqrt(a * a + b * e_keV + c * e_keV * e_keV);
	G4double sigma = fwhm / 2.35482;
	return e_keV + G4RandGauss::shoot(0.0, sigma);
}

void EventAction::BeginOfEventAction(const G4Event* /*event*/) {
	auto run = G4RunManager::GetRunManager()->GetCurrentRun();
	if (run) fRunID = run->GetRunID();
}

void EventAction::EndOfEventAction(const G4Event* event) {
	auto man = G4AnalysisManager::Instance();
	auto hce = event->GetHCofThisEvent();
	G4int evtID = event->GetEventID();
	G4double angle = gConfig.angleDeg;

	// ---- Channel A: 透射 — 每个 step 一行 ----
	if (hce && fTransHCID < 0) {
		fTransHCID = G4SDManager::GetSDMpointer()->GetCollectionID("TransmissionSD/TransmissionHC");
	}
	if (hce && fTransHCID >= 0 && fTransHCID < hce->GetNumberOfCollections()) {
		auto hc = static_cast<TransmissionHitsCollection*>(hce->GetHC(fTransHCID));
		if (hc) {
			G4int n = hc->entries();
			for (G4int i = 0; i < n; i++) {
				auto h = (*hc)[i];
				man->FillNtupleIColumn(0, 0, fRunID);
				man->FillNtupleIColumn(0, 1, evtID);
				man->FillNtupleDColumn(0, 2, angle);
				man->FillNtupleIColumn(0, 3, h->pixelX);
				man->FillNtupleIColumn(0, 4, h->pixelY);
				man->FillNtupleDColumn(0, 5, h->edep);
				man->FillNtupleDColumn(0, 6, h->time);
				man->FillNtupleSColumn(0, 7, h->particleName);
				man->FillNtupleIColumn(0, 8, h->trackID);
				man->FillNtupleIColumn(0, 9, h->parentID);
				man->FillNtupleSColumn(0, 10, h->creatorProcess);
				man->FillNtupleDColumn(0, 11, h->ekin);
				man->FillNtupleDColumn(0, 12, h->localPos.x());
				man->FillNtupleDColumn(0, 13, h->localPos.y());
				man->FillNtupleDColumn(0, 14, h->localPos.z());
				man->FillNtupleIColumn(0, 15, h->isPrimaryNeutron ? 1 : 0);
				man->FillNtupleIColumn(0, 16, h->isScatteredNeutron ? 1 : 0);
				man->FillNtupleDColumn(0, 17, h->dirX);
				man->FillNtupleDColumn(0, 18, h->dirY);
				man->FillNtupleDColumn(0, 19, h->dirZ);
				man->FillNtupleIColumn(0, 20, h->isUncollidedPrimary ? 1 : 0);
				man->AddNtupleRow(0);
			}
		}
	}

	// ---- Channel B: HPGe — event 级合并 ----
	if (hce && fHPGeHCID < 0) {
		fHPGeHCID = G4SDManager::GetSDMpointer()->GetCollectionID("HPGeSD/HPGeHC");
	}
	if (hce && fHPGeHCID >= 0 && fHPGeHCID < hce->GetNumberOfCollections()) {
		auto hc = static_cast<HPGeHitsCollection*>(hce->GetHC(fHPGeHCID));
		if (hc && hc->entries() > 0) {
			G4double totalEDep = 0;
			G4int nSteps = hc->entries();
			G4double firstGammaE = -1;
			std::set<G4String> particles;
			std::map<G4String, G4int> creatorCount;

			for (G4int i = 0; i < nSteps; i++) {
				auto h = (*hc)[i];
				totalEDep += h->edep;
				particles.insert(h->particleName);
				creatorCount[h->creatorProcess]++;
				if (h->particleName == "gamma" && firstGammaE < 0) {
					firstGammaE = h->ekin;
				}
			}
			G4double smeared = SmearEnergy(totalEDep);

			// 拼接粒子名
			G4String pnames;
			for (const auto& p : particles) {
				if (!pnames.empty()) pnames += "|";
				pnames += p;
			}
			// 主导 creator process
			G4String dominant = "none";
			G4int maxCount = -1;
			for (const auto& kv : creatorCount) {
				if (kv.second > maxCount) { maxCount = kv.second; dominant = kv.first; }
			}

			man->FillNtupleIColumn(1, 0, fRunID);
			man->FillNtupleIColumn(1, 1, evtID);
			man->FillNtupleDColumn(1, 2, angle);
			man->FillNtupleDColumn(1, 3, smeared);            // keV (含可选展宽)
			man->FillNtupleIColumn(1, 4, nSteps);
			man->FillNtupleDColumn(1, 5, firstGammaE * 1000); // keV
			man->FillNtupleSColumn(1, 6, pnames);
			man->FillNtupleSColumn(1, 7, dominant);
			man->FillNtupleSColumn(1, 8, "HPGe");
			man->AddNtupleRow(1);
		}
	}
}
