
#ifndef MyPhysicsList_h
#define MyPhysicsList_h 1

#include "Shielding.hh"
#include "G4OpticalPhysics.hh"
#include "G4EmStandardPhysics_option4.hh"


class MyPhysicsList : public Shielding {
public:
    MyPhysicsList(): Shielding()                  
    {
        auto opticalPhysics = new G4OpticalPhysics();
        RegisterPhysics(opticalPhysics);
        ReplacePhysics(new G4EmStandardPhysics_option4());
    }
    ~MyPhysicsList() override = default;
};

#endif
