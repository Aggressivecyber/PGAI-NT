#include "MyPhysicsList.hh"

void MyPhysicsList::SetCuts()
{
	SetDefaultCutValue(0.7* CLHEP::mm);
	SetParticleCuts(100 * CLHEP::um, "opticalphoton");
	DumpCutValuesTable();
}
