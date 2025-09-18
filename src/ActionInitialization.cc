#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"


void ActionInitialization::Build() const {
	auto pri_worker = new PrimaryGeneratorAction();
	SetUserAction(new RunAction(pri_worker));
	SetUserAction(pri_worker);


}

void ActionInitialization::BuildForMaster() const
{
	auto pri_master = new PrimaryGeneratorAction();
	SetUserAction(new RunAction(pri_master));
}
