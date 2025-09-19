#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(100 * CLHEP::um);

	DumpCutValuesTable();
}
