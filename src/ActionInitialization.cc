#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"

void ActionInitialization::BuildForMaster() const {
	SetUserAction(new RunAction());
}
void ActionInitialization::Build() const {
	SetUserAction(new PrimaryGeneratorAction(fDet));
	SetUserAction(new RunAction());

}