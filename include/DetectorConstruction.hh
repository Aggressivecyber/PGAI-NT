#ifndef DETECTORCONSTRUCTION_HH
#define DETECTORCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "globals.hh"

class G4Material;

class DetectorConstruction : public G4VUserDetectorConstruction {
public:
	DetectorConstruction();
	~DetectorConstruction() override = default;

	G4VPhysicalVolume* Construct() override;

	// 投影角度 (tomography)
	void setDeg(double d);
	double getDeg() const;

	// 双模态探测器逻辑体 (成员, 供 SD 绑定)
	G4LogicalVolume* logicTransmissionScreen = nullptr;  // 连续闪烁屏 (快中子照相)
	G4LogicalVolume* logicHPGe = nullptr;

private:
	void ConstructSDandField() override;
	void DefineMaterials();
	G4Material* GetPhantomMaterial(const G4String& name);

	// 探测器构建
	void BuildTransmissionDetector(G4LogicalVolume* world);
	void BuildHPGeDetector(G4LogicalVolume* world);
	G4LogicalVolume* logicPixelCarrier = nullptr;  // (保留兼容, 未使用)

	// phantom 构建器
	void BuildPhantom(G4LogicalVolume* world);
	void BuildEmptyPhantom(G4LogicalVolume* world);
	void BuildSingleMaterialPhantom(G4LogicalVolume* world);
	void BuildMaterialDegeneracyPhantom(G4LogicalVolume* world);
	void BuildSteelShellHydrogenPhantom(G4LogicalVolume* world);
	void BuildCTTestPhantom(G4LogicalVolume* world);

	// 束流系统绕 z 轴的旋转 (实现投影角度)
	G4RotationMatrix* BeamRotation();
};

#endif
