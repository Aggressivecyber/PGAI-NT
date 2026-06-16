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
	G4double sourceCenterY = 0.0 * mm;
	G4double sourceCenterZ = 0.0 * mm;
	G4double divergence = 0.0 * deg; // 角发散 1-sigma
	G4double sourceDistance = 800.0 * mm;

	// ---- 快中子透射探测器 (多层屏结构) ----
	G4int pixelsX = 128;
	G4int pixelsY = 128;
	G4double scintThickness = 10.0 * mm;
	G4double detectorSize = 70.0 * mm;   // 正方形平面全边长 (样品占 ~43% FOV, 避免截断伪影)
	G4double detectorDistance = 55.0 * mm;
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
	G4double collimLen = 74.0 * mm;      // 长 Pb 准直器: 前口贴近样品外侧
	G4double collimRout = 45.0 * mm;     // 加厚外径, 降低旁路伽马漏入
	G4double collimHoleFront = 1.5 * mm; // 样品侧小孔, 限定局部有效体积
	G4double collimHoleBack = 5.0 * mm;  // HPGe 侧孔, 维持窄视锥
	G4bool smearHPGe = false;            // 高斯展宽开关
	// 扫描式 PGAI: 准直器视场中心 (y,z), 整体平移 HPGe+准直器看样品不同体素列
	G4double hpgeCenterY = 0.0 * mm;
	G4double hpgeCenterZ = 0.0 * mm;
	// FWHM(E) = sqrt(a^2 + b*E + c*E^2)  [能量单位 keV]
	G4double resA = 1.0 * keV;
	G4double resB = 0.0;
	G4double resC = 0.0;

	// ---- Phantom ----
	// empty | single | calibration_block | degeneracy | steel | cttest | gradient_cylinder
	G4String phantomMode = "empty";
	G4String singleMaterial = "PE";      // PE|Al|Fe|Cu|Pb|Ni|air
	G4double singleThickness = 20.0 * mm;

	// ---- 投影角度 (tomography) ----
	G4double angleDeg = 0.0;
};

extern PGAIConfig gConfig;

#endif
