#ifndef STEPPING_ACTION_HH
#define STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

class SteppingAction : public G4UserSteppingAction {
public:
	void UserSteppingAction(const G4Step* step) override;
};

#endif
