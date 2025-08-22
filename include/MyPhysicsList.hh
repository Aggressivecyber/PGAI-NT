
#ifndef MyPhysicsList_h
#define MyPhysicsList_h 1

#include "FTFP_BERT_HP.hh"
#include "G4OpticalPhysics.hh"

class MyPhysicsList : public FTFP_BERT_HP {
public:
    MyPhysicsList(): FTFP_BERT_HP()                  
    {
        auto opticalPhysics = new G4OpticalPhysics();
        RegisterPhysics(opticalPhysics);
    }
    ~MyPhysicsList() override = default;
};

#endif
