#ifndef ACTIONINITIALIZATION
#define ACTIONINITIALIZATION 1

#include "DetectorConstruction.hh"
#include "G4VUserActionInitialization.hh"

class ActionInitialization : public G4VUserActionInitialization {
public:
	inline ActionInitialization(DetectorConstruction* det) : fDet(det) {}
	~ActionInitialization() override = default;
	void Build() const override;
private:
	DetectorConstruction* fDet;
};

#endif