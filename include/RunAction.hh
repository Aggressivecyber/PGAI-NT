#ifndef RUNACTION_HH
#define RUNACTION_HH 1
#include "G4UserRunAction.hh"
#include "G4SystemOfUnits.hh"
#include "PrimaryGeneratorAction.hh"
#include <RunAction.hh>

class RunAction :public G4UserRunAction {
public:
	RunAction(PrimaryGeneratorAction* pga);
	virtual ~RunAction();
	virtual void BeginOfRunAction(const G4Run*);
	virtual void EndOfRunAction(const G4Run*);
private:
	PrimaryGeneratorAction* fPGA;
};




#endif