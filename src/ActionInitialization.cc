#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"

void ActionInitialization::BuildForMaster() const {
}
void ActionInitialization::Build() const {
	SetUserAction(new PrimaryGeneratorAction());
}