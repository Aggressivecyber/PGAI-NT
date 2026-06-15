#ifndef MyPhysicsList_h
#define MyPhysicsList_h 1

#include "Shielding.hh"
#include "G4EmStandardPhysics_option4.hh"

// Shielding (含中子 HP) + EM option4。
// 不启用光学物理: 透射成像基于能量沉积 (反冲质子), 无需光学光子传输, 大幅加速仿真。
class MyPhysicsList : public Shielding {
public:
	MyPhysicsList() : Shielding() {
		ReplacePhysics(new G4EmStandardPhysics_option4());
	}
	~MyPhysicsList() override = default;
};

#endif
