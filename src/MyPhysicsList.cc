#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(1 * CLHEP::mm);

	DumpCutValuesTable();
}
