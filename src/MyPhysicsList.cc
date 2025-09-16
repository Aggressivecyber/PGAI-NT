#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(1 * CLHEP::mm);

	SetCutValue(1 * CLHEP::mm, "gamma");
	SetCutValue(0.5 * CLHEP::mm, "e-");
	SetCutValue(0.5 * CLHEP::mm, "e+");
	SetCutValue(0.5 * CLHEP::mm, "photon");

	DumpCutValuesTable();
}
