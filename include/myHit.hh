# ifndef MYHIT_HH
# define MYHIT_HH

#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

class myHit : public G4VHit{
public:
	myHit();
	virtual ~myHit() override;
	myHit(const myHit&) = default;
	void Draw() override;
	void Print() override;

	myHit& operator=(const myHit&) = default;
	G4bool operator==(const myHit&) const;

	inline void* operator new(size_t);
	inline void operator delete(void*);

	inline G4double Getedep() {
		return edep;
	}

	inline G4int GetcopyNo() { return copyNo; }

	inline G4String GetParticlename()
	{
		return particleName;
	}

	inline G4double GetGlobalTime()
	{
		return globalTime;
	}

	inline G4ThreeVector GetPos()
	{
		return pos;
	}

	inline void Setedep(G4double nedep)
	{
		edep = nedep;
	}

	inline void Setpos(G4ThreeVector npos)
	{
		pos = npos;
	}
	
	inline void SetParticleName(G4String nname)
	{
		particleName = nname;
	}

	inline void SetCopyNo(G4int ncopyn)
	{
		copyNo = ncopyn;
	}

	inline void SetTime(G4double nglobalTime)
	{
		globalTime = nglobalTime;
	}


private:
	G4double edep{};
	G4ThreeVector pos{};
	G4String particleName{};
	G4double globalTime{};
	G4int copyNo{};

};

using MyHitsCollection = G4THitsCollection<myHit>;

extern G4ThreadLocal G4Allocator<myHit>* myHitAllocator;

inline void* myHit::operator new(size_t)
{
	if (!myHitAllocator)
		myHitAllocator = new G4Allocator<myHit>;
	return (void*)myHitAllocator->MallocSingle();
}

inline void myHit::operator delete(void* hit)
{
	myHitAllocator->FreeSingle((myHit*)hit);
}



#endif
