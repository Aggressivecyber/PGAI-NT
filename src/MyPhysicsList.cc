#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(0.5 * CLHEP::mm);

	DumpCutValuesTable();
}
