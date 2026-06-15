#ifndef EVENT_ACTION_HH
#define EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "globals.hh"

class EventAction : public G4UserEventAction {
public:
	EventAction();
	~EventAction() override = default;
	void BeginOfEventAction(const G4Event* event) override;
	void EndOfEventAction(const G4Event* event) override;

private:
	G4int fTransHCID{-1};
	G4int fHPGeHCID{-1};
	G4int fRunID{0};

	G4double SmearEnergy(G4double e_keV) const;
};

#endif
