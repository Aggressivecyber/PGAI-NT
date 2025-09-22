#include "myHit.hh"
#include "G4VVisManager.hh"
#include "G4Circle.hh"
#include "G4Colour.hh"
#include "G4VisAttributes.hh"

G4ThreadLocal G4Allocator<myHit>* myHitAllocator = nullptr;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4bool myHit::operator==(const myHit& right) const
{
	return (this == &right) ? true : false;
}

myHit::myHit():edep(0.),pos(0.,0.,0.),particleName(""),globalTime(0.)
{}
myHit:: ~myHit(){}

void myHit::Draw()
{
	G4VVisManager* pVVismanager = G4VVisManager::GetConcreteInstance();
	if (pVVismanager)
	{
		G4Circle circle(pos);
		circle.SetScreenSize(4.);
		circle.SetFillStyle(G4Circle::filled);
		G4VisAttributes attribs(G4Colour::Red());
		circle.SetVisAttributes(attribs);
		pVVismanager->Draw(circle);
	}
}

void myHit::Print()
{
	G4cout << "edep: " << edep / CLHEP::keV << " keV, pos: " << pos << ", particle: " << particleName << ", time: " << globalTime / CLHEP::ns << " ns" << G4endl;
}

