#ifndef PGAI_CONFIG_HH
#define PGAI_CONFIG_HH

#include "globals.hh"
#include "G4SystemOfUnits.hh"

// 全局仿真配置 — 由 PGAIMessenger (宏命令) 或环境变量修改, 各模块读取。
struct PGAIConfig {
	// ---- 源 (4.05 MeV 准单能快中子) ----
	G4double energy = 4.05 * MeV;
	G4double energySpread = 0.0;      // 分数 1-sigma (0.03 = 3%)
	G4double spotSize = 1.0 * mm;    // 源斑直径 (均匀方斑半宽 = spotSize/2)
	G4double divergence = 0.0 * deg; // 角发散 1-sigma
	G4double sourceDistance = 800.0 * mm;

	// ---- 快中子透射探测器 (多层屏结构) ----
	G4int pixelsX = 128;
	G4int pixelsY = 128;
	G4double scintThickness = 10.0 * mm;
	G4double detectorSize = 60.0 * mm;   // 正方形平面全边长 (样品占 ~67% FOV, 避免截断)
	G4double detectorDistance = 40.0 * mm;
	G4double screenWindowThk = 1.0 * mm; // Al 入/出射窗厚度
	G4double screenFrame = 2.0 * mm;     // Al 框架边宽

	// ---- HPGe 瞬发伽马探测器 (真实高纯锗结构) ----
	G4double hpgeDistance = 150.0 * mm;  // 样品->杜瓦外壳距离
	G4double hpgeR = 25.0 * mm;          // Ge 晶体半径 (含死层)
	G4double hpgeH = 60.0 * mm;          // Ge 晶体厚度
	G4double hpgeDeadLayer = 0.7 * mm;   // Ge 死层 (非灵敏, Li 扩散层)
	G4double hpgeVacuumGap = 3.0 * mm;   // 真空层 (杜瓦内)
	G4double hpgeHousingThk = 2.0 * mm;  // Al 杜瓦外壳厚度
	// Pb 准直器 (锥形孔)
	G4double collimLen = 50.0 * mm;      // 准直器长度
	G4double collimRout = 35.0 * mm;     // 准直器外径 (Pb 屏蔽)
	G4double collimHoleFront = 5.0 * mm; // 样品侧孔半径 (小, 聚焦视场)
	G4double collimHoleBack = 12.0 * mm; // HPGe 侧孔半径 (大, 覆盖晶体)
	G4bool smearHPGe = false;            // 高斯展宽开关
	// 扫描式 PGAI: 准直器视场中心 y (mm), 整体平移 HPGe+准直器看样品不同条带
	G4double hpgeCenterY = 0.0 * mm;
	// FWHM(E) = sqrt(a^2 + b*E + c*E^2)  [能量单位 keV]
	G4double resA = 1.0 * keV;
	G4double resB = 0.0;
	G4double resC = 0.0;

	// ---- Phantom ----
	// empty | single | degeneracy | steel
	G4String phantomMode = "empty";
	G4String singleMaterial = "PE";      // PE|Al|Fe|Cu|Pb|Ni|air
	G4double singleThickness = 20.0 * mm;

	// ---- 投影角度 (tomography) ----
	G4double angleDeg = 0.0;
};

extern PGAIConfig gConfig;

#endif
