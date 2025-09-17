#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(0.2 * CLHEP::mm);

	DumpCutValuesTable();
}
