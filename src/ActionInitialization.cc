#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"


void ActionInitialization::Build() const {
	SetUserAction(new RunAction());
	SetUserAction(new PrimaryGeneratorAction(fDet));


}

void ActionInitialization::BuildForMaster() const
{
	SetUserAction(new RunAction());
}
