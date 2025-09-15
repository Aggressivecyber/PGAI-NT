#include "myHit.hh"

myHit::myHit():edep(0.),pos(0.,0.,0.),particleName(""),globalTime(0.)
{}
myHit:: ~myHit(){}

void myHit::Draw()
{
}

void myHit::Print()
{
	G4cout << "edep: " << edep / CLHEP::keV << " keV, pos: " << pos << ", particle: " << particleName << ", time: " << globalTime / CLHEP::ns << " ns" << G4endl;
}
