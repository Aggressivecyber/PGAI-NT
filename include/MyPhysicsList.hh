
#ifndef MyPhysicsList_h
#define MyPhysicsList_h 1

#include "FTFP_BERT.hh"
#include "G4OpticalPhysics.hh"

class MyPhysicsList : public FTFP_BERT {
public:
    MyPhysicsList(): FTFP_BERT()                  
    {
        auto opticalPhysics = new G4OpticalPhysics();
        RegisterPhysics(opticalPhysics);
    }
    ~MyPhysicsList() override = default;
};

#endif
