#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Cons.hh"
#include "G4SubtractionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4RotationMatrix.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "G4ThreeVector.hh"
#include "G4SDManager.hh"

#include "FastNeutronTransmissionSD.hh"
#include "HPGeSpectrometerSD.hh"
#include "PGAIConfig.hh"

#include <cmath>

DetectorConstruction::DetectorConstruction() {
	DefineMaterials();
}

void DetectorConstruction::DefineMaterials() {
	auto nist = G4NistManager::Instance();
	nist->FindOrBuildMaterial("G4_AIR");
	nist->FindOrBuildMaterial("G4_Galactic");
	nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	nist->FindOrBuildMaterial("G4_WATER");
	nist->FindOrBuildMaterial("G4_Al");
	nist->FindOrBuildMaterial("G4_Fe");
	nist->FindOrBuildMaterial("G4_Cu");
	nist->FindOrBuildMaterial("G4_Pb");
	nist->FindOrBuildMaterial("G4_Ni");
	nist->FindOrBuildMaterial("G4_Ge");
	nist->FindOrBuildMaterial("G4_PLEXIGLASS");  // PMMA (CT 测试件背景)
	// 塑料闪烁体 (EJ200/BC408 近似) — 透射探测器主材料
	nist->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
}

G4Material* DetectorConstruction::GetPhantomMaterial(const G4String& name) {
	auto nist = G4NistManager::Instance();
	if (name == "PE")  return nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	if (name == "Al")  return nist->FindOrBuildMaterial("G4_Al");
	if (name == "Fe")  return nist->FindOrBuildMaterial("G4_Fe");
	if (name == "Cu")  return nist->FindOrBuildMaterial("G4_Cu");
	if (name == "Pb")  return nist->FindOrBuildMaterial("G4_Pb");
	if (name == "Ni")  return nist->FindOrBuildMaterial("G4_Ni");
	if (name == "water") return nist->FindOrBuildMaterial("G4_WATER");
	return nist->FindOrBuildMaterial("G4_AIR");  // air / void
}

G4RotationMatrix* DetectorConstruction::BeamRotation() {
	auto rot = new G4RotationMatrix();
	rot->rotateZ(gConfig.angleDeg * CLHEP::deg);
	return rot;
}

G4VPhysicalVolume* DetectorConstruction::Construct() {
	auto Air = G4Material::GetMaterial("G4_AIR");

	// ---- 世界 ----
	G4double worldSize = 1.5 * m;
	auto solidWorld = new G4Box("World", worldSize, worldSize, worldSize);
	auto logicWorld = new G4LogicalVolume(solidWorld, Air, "World");
	auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);

	// ---- 样品台 (绕 z 轴旋转 = tomography 投影角度) ----
	// x 半长 32 (容纳 PE 60mm 厚 x±30, 且 < detectorDistance 40 避透射屏); y/z 50 容样品
	auto solidStage = new G4Box("SampleStage", 32 * mm, 50 * mm, 50 * mm);
	auto logicStage = new G4LogicalVolume(solidStage, Air, "SampleStageLV");
	logicStage->SetVisAttributes(G4VisAttributes::GetInvisible());
	auto stageRot = new G4RotationMatrix();
	stageRot->rotateZ(gConfig.angleDeg * CLHEP::deg);
	new G4PVPlacement(stageRot, G4ThreeVector(), logicStage, "SampleStage", logicWorld, false, 0);

	BuildPhantom(logicStage);   // phantom 随样品台一起旋转

	// ---- 双模态探测器 (固定, 不随角度转) ----
	BuildTransmissionDetector(logicWorld);
	BuildHPGeDetector(logicWorld);

	logicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());
	return physWorld;
}

// ===================== Channel A: 多层闪烁屏 (EJ-200 风格) =====================
// 结构: [Al 入射窗] [塑料闪烁体(灵敏)] [Al 出射窗/反光层] + Al 框架
// 真实快中子成像屏: Al 保护窗 + 闪烁体 + 反光层, 光学读出在外部(CCD)
void DetectorConstruction::BuildTransmissionDetector(G4LogicalVolume* world) {
	auto Plastic = G4Material::GetMaterial("G4_PLASTIC_SC_VINYLTOLUENE");
	auto Al = G4Material::GetMaterial("G4_Al");
	auto Air = G4Material::GetMaterial("G4_AIR");

	G4double size = gConfig.detectorSize;
	G4double thk = gConfig.scintThickness;
	G4double wThk = gConfig.screenWindowThk;
	G4double frame = gConfig.screenFrame;

	// 灵敏闪烁体 (SD 绑定目标)
	auto solidScint = new G4Box("Scintillator", thk * 0.5, size * 0.5, size * 0.5);
	logicTransmissionScreen = new G4LogicalVolume(solidScint, Plastic, "ScintillatorLV");
	{
		auto v = new G4VisAttributes(G4Colour(0.2, 0.85, 0.85, 0.65));
		v->SetForceSolid(true);
		logicTransmissionScreen->SetVisAttributes(v);
	}

	// Al 入射窗 / 出射窗 (非灵敏, 模拟封装与反光层)
	auto solidWin = new G4Box("ScreenWindow", wThk * 0.5, size * 0.5, size * 0.5);
	auto logicWinF = new G4LogicalVolume(solidWin, Al, "ScreenWindowFrontLV");
	auto logicWinB = new G4LogicalVolume(solidWin, Al, "ScreenWindowBackLV");
	auto visAl = new G4VisAttributes(G4Colour(0.75, 0.75, 0.78, 0.8));
	visAl->SetForceSolid(true);
	logicWinF->SetVisAttributes(visAl);
	logicWinB->SetVisAttributes(visAl);

	// Al 框架母体 (容纳窗+闪烁体)
	G4double totalThk = thk + 2 * wThk;
	auto solidCarrier = new G4Box("ScreenCarrier", totalThk * 0.5 + frame,
	                              size * 0.5 + frame, size * 0.5 + frame);
	auto logicCarrier = new G4LogicalVolume(solidCarrier, Air, "ScreenCarrierLV");
	logicCarrier->SetVisAttributes(G4VisAttributes::GetInvisible());

	new G4PVPlacement(nullptr, G4ThreeVector(-thk * 0.5 - wThk * 0.5, 0, 0),
	                  logicWinF, "ScreenWindowFront", logicCarrier, false, 0);
	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0),
	                  logicTransmissionScreen, "Scintillator", logicCarrier, false, 0);
	new G4PVPlacement(nullptr, G4ThreeVector(thk * 0.5 + wThk * 0.5, 0, 0),
	                  logicWinB, "ScreenWindowBack", logicCarrier, false, 0);

	// 整体固定在束流下游 +x
	new G4PVPlacement(nullptr, G4ThreeVector(gConfig.detectorDistance, 0, 0),
	                  logicCarrier, "TransmissionScreen", world, false, 0);
}

// ===================== Channel B: 真实 HPGe (杜瓦+真空+死层+灵敏芯) + Pb 锥孔准直器 =====================
void DetectorConstruction::BuildHPGeDetector(G4LogicalVolume* world) {
	auto Ge = G4Material::GetMaterial("G4_Ge");
	auto Pb = G4Material::GetMaterial("G4_Pb");
	auto Al = G4Material::GetMaterial("G4_Al");
	auto Vac = G4Material::GetMaterial("G4_Galactic");  // 真空近似

	G4double R = gConfig.hpgeR;
	G4double H = gConfig.hpgeH;
	G4double dist = gConfig.hpgeDistance;
	G4double dead = gConfig.hpgeDeadLayer;
	G4double vac = gConfig.hpgeVacuumGap;
	G4double housing = gConfig.hpgeHousingThk;

	// --- Ge 灵敏芯 (SD 绑定目标) ---
	G4double Rcore = std::max(R - dead, 0.1 * mm);
	auto solidCore = new G4Tubs("HPGeCore", 0, Rcore, H * 0.5, 0, 2 * CLHEP::pi);
	logicHPGe = new G4LogicalVolume(solidCore, Ge, "HPGeCoreLV");
	{
		auto v = new G4VisAttributes(G4Colour(0.5, 0.5, 0.95, 0.9));
		v->SetForceSolid(true);
		logicHPGe->SetVisAttributes(v);
	}

	// --- Ge 死层 (外环, 非灵敏, 粒子穿越损失能量) ---
	auto solidDead = new G4Tubs("HPGeDeadLayer", Rcore, R, H * 0.5, 0, 2 * CLHEP::pi);
	auto logicDead = new G4LogicalVolume(solidDead, Ge, "HPGeDeadLayerLV");
	{
		auto v = new G4VisAttributes(G4Colour(0.35, 0.35, 0.7, 0.5));
		v->SetForceSolid(true);
		logicDead->SetVisAttributes(v);
	}

	// --- 真空层 (杜瓦内壁与 Ge 之间) ---
	G4double RvacOut = R + vac;
	G4double Hvac = H * 0.5 + vac;
	auto solidVacuum = new G4Tubs("HPGeVacuum", R, RvacOut, Hvac, 0, 2 * CLHEP::pi);
	auto logicVacuum = new G4LogicalVolume(solidVacuum, Vac, "HPGeVacuumLV");
	logicVacuum->SetVisAttributes(G4VisAttributes::GetInvisible());

	// --- Al 杜瓦外壳 ---
	G4double RhouseOut = RvacOut + housing;
	auto solidHousing = new G4Tubs("HPGeHousing", RvacOut, RhouseOut, Hvac, 0, 2 * CLHEP::pi);
	auto logicHousing = new G4LogicalVolume(solidHousing, Al, "HPGeHousingLV");
	{
		auto v = new G4VisAttributes(G4Colour(0.8, 0.8, 0.85, 0.9));
		v->SetForceSolid(true);
		logicHousing->SetVisAttributes(v);
	}

	// 同心多层直接放 world (轴沿 +y), 不嵌套避免越界
	G4ThreeVector perpDir(0, 1, 0);
	// 扫描 PGAI: 整体 y 平移使准直器视场对准样品 y=hpgeCenterY 条带
	G4ThreeVector scanOffset(0, gConfig.hpgeCenterY, 0);
	G4ThreeVector hpgePos = perpDir * dist + scanOffset;
	auto rotDet = new G4RotationMatrix();
	rotDet->rotateX(90 * CLHEP::deg);  // Tubs z 轴 -> y
	new G4PVPlacement(rotDet, hpgePos, logicHousing, "HPGeHousing", world, false, 0);
	new G4PVPlacement(new G4RotationMatrix(*rotDet), hpgePos, logicVacuum, "HPGeVacuum", world, false, 0);
	new G4PVPlacement(new G4RotationMatrix(*rotDet), hpgePos, logicDead, "HPGeDeadLayer", world, false, 0);
	new G4PVPlacement(new G4RotationMatrix(*rotDet), hpgePos, logicHPGe, "HPGe", world, false, 0);

	// --- Pb 锥孔准直器 (前端小孔对样品, 后端大孔对 HPGe) ---
	G4double cLen = gConfig.collimLen;
	G4double eps = 0.2 * mm;
	auto solidCollimBody = new G4Tubs("CollimBody", 0, gConfig.collimRout, cLen * 0.5, 0, 2 * CLHEP::pi);
	// G4Cons(Rmin1,Rmax1,Rmin2,Rmax2,Dz): -z端 Rmax1(样品侧小孔), +z端 Rmax2(HPGe侧大孔)
	auto solidCollimHole = new G4Cons("CollimHole", 0, gConfig.collimHoleFront,
	                                  0, gConfig.collimHoleBack, cLen * 0.5 + eps,
	                                  0, 2 * CLHEP::pi);
	auto solidCollim = new G4SubtractionSolid("Collimator", solidCollimBody, solidCollimHole);
	auto logicCollim = new G4LogicalVolume(solidCollim, Pb, "CollimatorLV");
	{
		auto v = new G4VisAttributes(G4Colour(0.35, 0.35, 0.4, 0.92));
		v->SetForceSolid(true);
		logicCollim->SetVisAttributes(v);
	}
	auto rotCollim = new G4RotationMatrix();
	rotCollim->rotateX(-90 * CLHEP::deg);  // Cons +z(大孔,HPGe侧) -> 世界 +y(HPGe)
	// 紧贴 HPGe 杜瓦前端 (朝样品)
	G4double housingFront = gConfig.hpgeDistance - Hvac * 0.5;
	G4ThreeVector collimPos(0, housingFront - cLen * 0.5, 0);
	new G4PVPlacement(rotCollim, collimPos, logicCollim, "Collimator", world, false, 0);
}

// ===================== Phantom 模式 =====================

void DetectorConstruction::BuildPhantom(G4LogicalVolume* world) {
	const G4String& mode = gConfig.phantomMode;
	if (mode == "single")             BuildSingleMaterialPhantom(world);
	else if (mode == "degeneracy")    BuildMaterialDegeneracyPhantom(world);
	else if (mode == "steel")         BuildSteelShellHydrogenPhantom(world);
	else if (mode == "cttest")        BuildCTTestPhantom(world);
	else                              BuildEmptyPhantom(world);  // empty / default
}

void DetectorConstruction::BuildEmptyPhantom(G4LogicalVolume* /*world*/) {
	// 空场: 不放任何样品 (用于 I0)
}

void DetectorConstruction::BuildSingleMaterialPhantom(G4LogicalVolume* world) {
	// 单材料圆柱 (标定), 轴沿 z, 厚度沿 x 方向 (束流方向) 不对 — 这里用轴沿 x 的圆柱
	auto mat = GetPhantomMaterial(gConfig.singleMaterial);
	G4double thick = gConfig.singleThickness;
	G4double radius = 15 * mm;
	// 用 G4Tubs, 轴沿 x: rotateY(90)
	auto solid = new G4Tubs("SinglePhantom", 0, radius, thick * 0.5, 0, 2 * CLHEP::pi);
	auto logic = new G4LogicalVolume(solid, mat, "SinglePhantomLV");
	auto rot = new G4RotationMatrix();
	rot->rotateY(90 * CLHEP::deg);
	new G4PVPlacement(rot, G4ThreeVector(), logic, "SinglePhantom", world, false, 0);
	logic->SetVisAttributes(new G4VisAttributes(G4Colour(0.9, 0.6, 0.2, 0.7)));
}

void DetectorConstruction::BuildMaterialDegeneracyPhantom(G4LogicalVolume* world) {
	// 6 个材料块, 不同厚度, 设计相近 4.05MeV 中子透射衰减 (演示材料退化)
	// 块为小圆柱 (轴沿 x/束流), 在 y 方向并排, z=0
	struct Block { const char* mat; double thick_mm; double y_mm; };
	// 厚度选择使宏观衰减量级相近 (粗略, 中子 4MeV)
	Block blocks[] = {
		{"PE",  60.0, -18.0},
		{"Al",  40.0, -10.8},
		{"Fe",  15.0,  -3.6},
		{"Cu",  12.0,   3.6},
		{"Pb",  10.0,  10.8},
		{"air", 40.0,  18.0},
	};
	G4double radius = 3.0 * mm;
	for (const auto& b : blocks) {
		auto mat = GetPhantomMaterial(b.mat);
		G4double thick = b.thick_mm * mm;
		auto solid = new G4Tubs(G4String("Deg_") + b.mat, 0, radius, thick * 0.5, 0, 2 * CLHEP::pi);
		auto logic = new G4LogicalVolume(solid, mat, G4String("Deg_") + b.mat + "LV");
		auto rot = new G4RotationMatrix();
		rot->rotateY(90 * CLHEP::deg);
		new G4PVPlacement(rot, G4ThreeVector(0, b.y_mm * mm, 0), logic,
		                  G4String("Deg_") + b.mat, world, false, 0);
		G4Colour c = (G4String(b.mat) == "air") ? G4Colour(0.9, 0.9, 0.9, 0.2)
		           : (G4String(b.mat) == "PE")  ? G4Colour(0.3, 0.9, 0.4, 0.7)
		           : (G4String(b.mat) == "Pb")  ? G4Colour(0.3, 0.3, 0.9, 0.7)
		           : G4Colour(0.8, 0.5, 0.3, 0.7);
		logic->SetVisAttributes(new G4VisAttributes(c));
	}
}

void DetectorConstruction::BuildSteelShellHydrogenPhantom(G4LogicalVolume* world) {
	// Fe 外壳圆筒 (轴沿 x), 内部含 PE / water / void / Al / Cu 区块
	auto Fe = GetPhantomMaterial("Fe");
	G4double rIn = 10 * mm, rOut = 14 * mm, lenX = 40 * mm;

	// 外壳: 大 Fe 圆柱减内孔
	auto solidShellOuter = new G4Tubs("SteelShellOuter", 0, rOut, lenX * 0.5, 0, 2 * CLHEP::pi);
	auto solidShellInner = new G4Tubs("SteelShellInner", 0, rIn, lenX * 0.5, 0, 2 * CLHEP::pi);
	auto solidShell = new G4SubtractionSolid("SteelShell", solidShellOuter, solidShellInner);
	auto logicShell = new G4LogicalVolume(solidShell, Fe, "SteelShellLV");
	auto rotShell = new G4RotationMatrix();
	rotShell->rotateY(90 * CLHEP::deg);
	new G4PVPlacement(rotShell, G4ThreeVector(), logicShell, "SteelShell", world, false, 0);
	logicShell->SetVisAttributes(new G4VisAttributes(G4Colour(0.5, 0.5, 0.55, 0.85)));

	// 内部填充 (沿 y 方向排布, 半径 < rIn)
	struct Fill { const char* mat; double y_mm; };
	Fill fills[] = {
		{"PE",    -6.0},
		{"water",  -2.0},
		{"air",    2.0},
		{"Al",     6.0},
		{"Cu",     0.0},  // 居中铜芯 (小)
	};
	for (const auto& f : fills) {
		auto mat = GetPhantomMaterial(f.mat);
		G4double r = (G4String(f.mat) == "Cu") ? 2.0 * mm : 2.5 * mm;
		G4double flen = (lenX - 4 * mm);  // 略短于外壳
		auto solid = new G4Tubs(G4String("Fill_") + f.mat, 0, r, flen * 0.5, 0, 2 * CLHEP::pi);
		auto logic = new G4LogicalVolume(solid, mat, G4String("Fill_") + f.mat + "LV");
		auto rot = new G4RotationMatrix();
		rot->rotateY(90 * CLHEP::deg);
		new G4PVPlacement(rot, G4ThreeVector(0, f.y_mm * mm, 0), logic,
		                  G4String("Fill_") + f.mat, world, false, 0);
		G4Colour c = (G4String(f.mat) == "PE")    ? G4Colour(0.3, 0.9, 0.4, 0.8)
		           : (G4String(f.mat) == "water") ? G4Colour(0.2, 0.5, 0.9, 0.8)
		           : (G4String(f.mat) == "Cu")    ? G4Colour(0.9, 0.5, 0.2, 0.8)
		           : (G4String(f.mat) == "Al")    ? G4Colour(0.8, 0.8, 0.85, 0.8)
		           : G4Colour(0.9, 0.9, 0.9, 0.3);
		logic->SetVisAttributes(new G4VisAttributes(c));
	}
}

// ===================== CT 测试件: PMMA 大圆柱 + 环绕 6 材料小圆柱 (全轴沿 z) =====================
// 标准 2D 平行束 CT 几何: 所有特征轴沿旋转轴 z, z 切片重建无模糊。
// 一次成像即可测试 PE/Al/Fe/Cu/Pb/Ni 多材料 + PGAI-NT 调试。
void DetectorConstruction::BuildCTTestPhantom(G4LogicalVolume* world) {
	auto PMMA = G4Material::GetMaterial("G4_PLEXIGLASS");

	G4double Rbig = 20 * mm;     // 大圆柱半径
	G4double H = 40 * mm;        // 高度 (z 方向)
	G4double Rring = 13 * mm;    // 小圆柱环绕半径
	G4double Rsmall = 4 * mm;    // 小圆柱半径

	// PMMA 背景大圆柱 (轴沿 z, G4Tubs 默认)
	auto solidBig = new G4Tubs("CTPhantom", 0, Rbig, H * 0.5, 0, 2 * CLHEP::pi);
	auto logicBig = new G4LogicalVolume(solidBig, PMMA, "CTPhantomLV");
	logicBig->SetVisAttributes(new G4VisAttributes(G4Colour(0.9, 0.9, 0.92, 0.5)));
	new G4PVPlacement(nullptr, G4ThreeVector(), logicBig, "CTPhantom", world, false, 0);

	// 6 种材料小圆柱环绕 (60° 间隔), 轴沿 z, 同高度
	const char* mats[6] = {"PE", "Al", "Fe", "Cu", "Pb", "Ni"};
	G4Colour cols[6] = {
		G4Colour(0.3, 0.9, 0.4, 0.95),   // PE
		G4Colour(0.8, 0.8, 0.85, 0.95),  // Al
		G4Colour(0.7, 0.4, 0.3, 0.95),   // Fe
		G4Colour(0.9, 0.5, 0.2, 0.95),   // Cu
		G4Colour(0.3, 0.3, 0.9, 0.95),   // Pb
		G4Colour(0.5, 0.9, 0.9, 0.95),   // Ni
	};
	for (G4int i = 0; i < 6; ++i) {
		G4double theta = i * 60.0 * CLHEP::deg;
		G4double x = Rring * std::cos(theta);
		G4double y = Rring * std::sin(theta);
		auto mat = GetPhantomMaterial(mats[i]);
		G4String nm = G4String("CTRod_") + mats[i];
		auto solidRod = new G4Tubs(nm, 0, Rsmall, H * 0.5, 0, 2 * CLHEP::pi);
		auto logicRod = new G4LogicalVolume(solidRod, mat, nm + "LV");
		auto va = new G4VisAttributes(cols[i]);
		va->SetForceSolid(true);
		logicRod->SetVisAttributes(va);
		new G4PVPlacement(nullptr, G4ThreeVector(x, y, 0), logicRod, nm, logicBig, false, i);
	}
}

void DetectorConstruction::ConstructSDandField() {
	// MT: 每个 worker 线程独立创建 SD + HC (不可 static 共享)
	auto sdMan = G4SDManager::GetSDMpointer();

	if (logicTransmissionScreen) {
		auto sd = new FastNeutronTransmissionSD("TransmissionSD");
		sdMan->AddNewDetector(sd);
		logicTransmissionScreen->SetSensitiveDetector(sd);
	}
	if (logicHPGe) {
		auto sd = new HPGeSpectrometerSD("HPGeSD");
		sdMan->AddNewDetector(sd);
		logicHPGe->SetSensitiveDetector(sd);
	}
}

void DetectorConstruction::setDeg(double d) { gConfig.angleDeg = d; }
double DetectorConstruction::getDeg() const { return gConfig.angleDeg; }
