#ifndef ACTIONINITIALIZATION
#define ACTIONINITIALIZATION 1

#include "DetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"

class ActionInitialization : public G4VUserActionInitialization {
public:
	inline ActionInitialization() {}
	~ActionInitialization() override = default;
	void Build() const override;
	void BuildForMaster() const override;
};

#endif