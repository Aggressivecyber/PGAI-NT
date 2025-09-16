#ifndef RUNACTION_HH
#define RUNACTION_HH 1
#include "G4UserRunAction.hh"
#include "G4SystemOfUnits.hh"

class RunAction :public G4UserRunAction {
public:
	RunAction(bool pMaster);
	virtual ~RunAction();
	virtual void BeginOfRunAction(const G4Run*);
	virtual void EndOfRunAction(const G4Run*);
private:
	bool isMaster=false;
};




#endif