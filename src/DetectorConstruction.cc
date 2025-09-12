#include "DetectorConstruction.hh"

#include <G4RunManager.hh>

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserLimits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4VisAttributes.hh"
#include "G4Colour.hh"
#include "CADMesh.hh"
#include "G4Tubs.hh"
#include "G4RotationMatrix.hh"
#include <vector>
#include <string>
#include "G4String.hh"
#include "SensitiveDetector.hh"
#include "G4SDManager.hh"

using namespace voxel_num;
G4VPhysicalVolume* DetectorConstruction::Construct() {
	//材料定义
	G4NistManager* nist = G4NistManager::Instance();
	G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
	G4Material* Pb = nist->FindOrBuildMaterial("G4_Pb");
	G4Material* Fe = nist->FindOrBuildMaterial("G4_Fe");
	G4Material* Cu = nist->FindOrBuildMaterial("G4_Cu");
	G4Material* Si = nist->FindOrBuildMaterial("G4_Si");
	G4Material* Al = nist->FindOrBuildMaterial("G4_Al");
	G4Material* Ge = nist->FindOrBuildMaterial("G4_Ge");
	//闪烁体材料定义
	std::vector<G4double> energy;
	std::vector<G4double> spectrum;
	G4double eMin = 1240.0 / 550.0 * eV; // 2.25 eV
	G4double eMax = 1240.0 / 380.0 * eV; // 3.26 eV
	G4int nPoints = 20;
	for (int i = 0; i < nPoints; i++) {
		G4double eVal = eMin + i * (eMax - eMin) / (nPoints - 1);
		energy.push_back(eVal);
		spectrum.push_back(1.0);
	}
	auto siMpt = new G4MaterialPropertiesTable();
	siMpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 3.5));
	Si->SetMaterialPropertiesTable(siMpt);
	G4Material* Ni = nist->FindOrBuildMaterial("G4_Ni");
	G4Material* PE = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	G4Material* Galactic = nist->FindOrBuildMaterial("G4_Galactic");
	auto mptGalactic = new G4MaterialPropertiesTable();
	std::vector<G4double> GalRindex(nPoints, 1.0);
	mptGalactic->AddProperty("RINDEX", energy, GalRindex);
	Galactic->SetMaterialPropertiesTable(mptGalactic);
	G4Element* Li = new G4Element("Lithium-6", "Li6", 3., 6.015 * g / mole);
	G4Element* F = nist->FindOrBuildElement("F");
	G4Element* Zn = nist->FindOrBuildElement("Zn");
	G4Element* S = nist->FindOrBuildElement("S");
	G4Element* Ag = nist->FindOrBuildElement("Ag");
	G4double density_LiF = 2.64 * g / cm3;
	G4double density_ZnS_Ag = 4.10 * g / cm3;
	G4Material* LiF = new G4Material("LiF", density_LiF, 2);
	LiF->AddElement(Li, 1);
	LiF->AddElement(F, 1);
	G4Material* ZnS_Ag = new G4Material("ZnS_Ag", density_ZnS_Ag, 3);
	ZnS_Ag->AddElement(Zn, 0.653);
	ZnS_Ag->AddElement(S, 0.3465);
	ZnS_Ag->AddElement(Ag, 0.0005);
	// Silver is 0.05wt% in ZnS:Ag, Abdalla et al., J Mater Sci: Mater Electron, 2022
	G4Material* ImageLayer = new G4Material("ImageLayer", 0.323 * density_LiF + 0.647 * density_ZnS_Ag + 0.03 * PE->GetDensity(), 3);
	ImageLayer->AddMaterial(LiF, 0.323);
	ImageLayer->AddMaterial(ZnS_Ag, 0.647);
	ImageLayer->AddMaterial(PE, 0.03); //Eljen Technology – EJ-426 Specification Sheet
	std::vector<G4double> rindex(nPoints, 1.);
	std::vector<G4double> absLength(nPoints, 40 * CLHEP::cm);
	G4double FastTimeConst = 200 * ns;
	G4double SlowTimeConst = 2000 * ns;
	G4double YieldRatio = 0.6;
	G4double ScintYield = 1000. / MeV;
	auto mpt = new G4MaterialPropertiesTable(); 
	mpt->AddProperty("RINDEX", energy,rindex);
	mpt->AddProperty("ABSLENGTH", energy,absLength);  
	mpt->AddConstProperty("SCINTILLATIONYIELD", ScintYield);
	mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
	mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", FastTimeConst);
	mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT2", SlowTimeConst);
	mpt->AddProperty("SCINTILLATIONCOMPONENT1", energy, spectrum);
	mpt->AddProperty("SCINTILLATIONCOMPONENT2", energy, spectrum);
	mpt->AddConstProperty("SCINTILLATIONYIELD1", ScintYield * YieldRatio);
	mpt->AddConstProperty("SCINTILLATIONYIELD2", ScintYield * (1.0 - YieldRatio));
	ImageLayer->SetMaterialPropertiesTable(mpt);


	//几何体定义
	HPGe_H = 60* mm;
	G4double HPGe_R = 20* mm;
	G4double Screen_L = 20 * mm;
	G4double Screen_H = 1 * mm;
	G4double film_Scintillator_T = 250 * um;
	G4double Tubs_H = 30 / 2 * mm;
	G4double Tubs_R = 2.9 * mm;
	G4double worldSize = 1 * m;
	Voxel_H = 1 * mm;
	G4Box* solidWorld = new G4Box("World", worldSize, worldSize, worldSize);
	G4Tubs* HPGe = new G4Tubs("HPGe", 0, HPGe_R, HPGe_H, 0, 2 * CLHEP::pi);
	//auto soildScreen = new G4Box("soildScreen",Screen_H,Screen_L, Screen_L);
	auto soildScintillator = new G4Box("soildScintillator", film_Scintillator_T,Screen_L, Screen_L);
	auto soildTubs_Pb = new G4Tubs("soildTubs_Pb", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
	auto soildTubs_Al = new G4Tubs("soildTubs_Al", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
	auto soildTubs_Cu = new G4Tubs("soildTubs_Cu", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
	auto soildTubs_Fe = new G4Tubs("soildTubs_Fe", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
	auto soildTubs_PE = new G4Tubs("soildTubs_PE", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
	auto soildTubs_Ni = new G4Tubs("soildTubs_Ni", 0, Tubs_R, Tubs_H, 0, 2 * CLHEP::pi);
    voxelNx = 10;
    voxelNy = 10;
	auto soildVoxel = new G4Box("soildVoxel", Voxel_H, (Screen_L*0.98)/voxelNx, (Screen_L*0.98)/voxelNy);
	auto soildMatrixVoxel = new G4Box("soildMatrixVoxel", Voxel_H, Screen_L, Screen_L);
	logicVoxel = new G4LogicalVolume(soildVoxel, Si, "logicVoxel");
	logicMatrixVoxel = new G4LogicalVolume(soildMatrixVoxel, Galactic, "logicMatrixVoxel");
	G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, Galactic, "World");
	G4VPhysicalVolume* physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);
	logicHPGe = new G4LogicalVolume(HPGe, Ge, "logicHPGe");
	//CMOS阵列
	for (G4int i = 0; i < voxelNx; i++) {
		for (G4int j = 0; j < voxelNy; j++) {
			G4double xPos = (i - (voxelNx - 1) / 2.0) * (2*Screen_L/voxelNx);
			G4double yPos = (j - (voxelNy - 1) / 2.0) * (2*Screen_L/voxelNy);
			G4VPhysicalVolume* physVoxel = new G4PVPlacement(nullptr, G4ThreeVector(0, xPos, yPos), logicVoxel, "Voxel", logicMatrixVoxel, false, i * voxelNy + j);
		}
	}
	fCMOSPV= new G4PVPlacement(nullptr, fCMOSPos0,logicMatrixVoxel, "MatrixVoxel", logicWorld, false, 0);
	G4VPhysicalVolume* physMatrixVoxel = fCMOSPV;
	G4VisAttributes* visAttributesVoxel = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8));
	visAttributesVoxel->SetForceSolid(true);
	logicVoxel->SetVisAttributes(visAttributesVoxel);
	logicMatrixVoxel->SetVisAttributes(G4VisAttributes::GetInvisible());
	auto soildcollimator = new G4Tubs("collimator", 2*mm, 4 * mm, 20 * mm, 0, 2 * CLHEP::pi);
	auto test = CADMesh::TessellatedMesh::FromSTL("./test3.stl");
	test->SetScale(1.);
	test->SetOffset(-15, -15, -15);
	//auto logicalScreen = new G4LogicalVolume(soildScreen, Al, "logicalScreen");
	//测试材料
	auto logicalScintillator = new G4LogicalVolume(soildScintillator, ImageLayer, "logicalScintillator");
	auto logicalTubs_Pb = new G4LogicalVolume(soildTubs_Pb, Pb, "logicalTubs_Pb");
	auto logicalTubs_Al = new G4LogicalVolume(soildTubs_Al, Al, "logicalTubs_Al");
	auto logicalTubs_Cu = new G4LogicalVolume(soildTubs_Cu, Cu, "logicalTubs_Cu");
	auto logicalTubs_Fe = new G4LogicalVolume(soildTubs_Fe, Fe, "logicalTubs_Fe");
	auto logicalTubs_PE = new G4LogicalVolume(soildTubs_PE, PE, "logicalTubs_PE");
	auto logicalTubs_Ni = new G4LogicalVolume(soildTubs_Ni, Ni, "logicalTubs_Ni");
	//auto physScreen = new G4PVPlacement(nullptr, G4ThreeVector(40,0,0),logicalScreen, "Screen", logicWorld, 1, 0);
	auto physScintillator = new G4PVPlacement(nullptr, G4ThreeVector((-film_Scintillator_T / 2) + (40), 0, 0), logicalScintillator, "Scintillator", logicWorld, 1, 0);
	auto logicaltest = new G4LogicalVolume(test->GetSolid(), Al, "logical");
	auto logicalcollimator = new G4LogicalVolume(soildcollimator, Pb, "logicalcollimator");
	new G4PVPlacement(0, G4ThreeVector(), logicaltest, "test", logicWorld, false, 0);
	auto pRot= new G4RotationMatrix();
	pRot->rotateX(90 * CLHEP::deg);
	fHPGePV = new G4PVPlacement(nullptr, fHPGePos0, logicalcollimator, "collimator", logicHPGe, false, 0);
	G4VPhysicalVolume* physHPGe = fHPGePV;
	//可视化属性设置
	G4VisAttributes* visAttributesCollimator = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	G4VisAttributes* visAttributesHPGe = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	visAttributesCollimator->SetForceWireframe(true);
	visAttributesHPGe->SetForceSolid(true);
	logicHPGe->SetVisAttributes(visAttributesHPGe);
	logicalcollimator->SetVisAttributes(visAttributesCollimator);
	std::vector<G4LogicalVolume*> logicalTubs;
	logicalTubs.push_back(logicalTubs_Pb);
	logicalTubs.push_back(logicalTubs_Al);
	logicalTubs.push_back(logicalTubs_Cu);
	logicalTubs.push_back(logicalTubs_Fe);
	logicalTubs.push_back(logicalTubs_PE);
	logicalTubs.push_back(logicalTubs_Ni);
	G4int i = 0;
	G4double R = 10 * mm;
	for (std::vector<G4LogicalVolume*>::iterator it = logicalTubs.begin(); it != logicalTubs.end(); it++) {
		G4double theta = i * ((1. / 3.) * CLHEP::pi);
		std::cout << "i = " << i << std::endl;
		std::string name = ("tubs");
		std::string temp = name;
		name += "_";
		name += (*it)->GetName();
		G4ThreeVector pos(R * std::sin(theta),
			R * std::cos(theta),
			0);
		new G4PVPlacement(0, pos, *it, name, logicaltest, false, 1);
		i++;
		G4VisAttributes* visAttributes1 = nullptr;
		if ((*it)->GetMaterial() == Pb) {visAttributes1 = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0));}
		else if ((*it)->GetMaterial() == Al) {visAttributes1 = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));}
		else if ((*it)->GetMaterial() == Cu) {visAttributes1 = new G4VisAttributes(G4Colour(1.0, 0.5, 0.5));}
		else if ((*it)->GetMaterial() == Fe) {visAttributes1 = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));}
		else if ((*it)->GetMaterial() == PE) {visAttributes1 = new G4VisAttributes(G4Colour(0.5, 1.0, 0.5));}
		else if ((*it)->GetMaterial() == Ni) {visAttributes1 = new G4VisAttributes(G4Colour(1.0, 1.0, 0.5));}
		else {visAttributes1 = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0));}
		visAttributes1->SetForceSolid(true);
		(*it)->SetVisAttributes(visAttributes1);
		name = temp;
	}
	G4VisAttributes* visAttributesScreen = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	G4VisAttributes* visAttributesScintillator = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0));
	visAttributesScintillator->SetForceSolid(true);
	//logicalScreen->SetVisAttributes(visAttributesScreen);
	logicalScintillator->SetVisAttributes(visAttributesScintillator);
	G4VisAttributes* vis1Attributes = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	logicaltest->SetVisAttributes(vis1Attributes);
	G4UserLimits* userLimits1 = new G4UserLimits(5 * CLHEP::mm);
	G4UserLimits* userLimits2 = new G4UserLimits(5 * CLHEP::um);
	logicWorld->SetUserLimits(userLimits1);
	logicalScintillator->SetUserLimits(userLimits2);
	G4VisAttributes* visAttributes = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	logicWorld->SetVisAttributes(visAttributes);
	return physWorld;
}

void DetectorConstruction::ConstructSDandField() {
	auto sdManager = G4SDManager::GetSDMpointer();
	SensitiveDetector* CMOSsd = new SensitiveDetector("CMOS");
	SensitiveDetector* HPGesd = new SensitiveDetector("HPGE");
	logicVoxel->SetSensitiveDetector(CMOSsd);
	logicHPGe->SetSensitiveDetector(HPGesd);
	sdManager->AddNewDetector(CMOSsd);
	sdManager->AddNewDetector(HPGesd);
}

void DetectorConstruction::RotateRig(G4double degZ)
{
	G4double phi = degZ * deg;
	G4RotationMatrix Ry;
	Ry.rotateY(phi);

	// HPGe: 位置=把初始位置投到半径上做绕Z旋，或直接按圆周设定
	G4ThreeVector posHPGe(fRhpge * std::cos(phi), fRhpge * std::sin(phi), fHPGePos0.z());
	auto rotHPGe = fHPGeRot0 * Ry;   // 机体也随之转向（如需保持朝向固定就别乘Rz）

	fHPGePV->SetTranslation(posHPGe);
	fHPGePV->SetRotation(new G4RotationMatrix(rotHPGe)); // 注意交给 Geant4 拷贝用 new

	// CMOS
	G4ThreeVector posCMOS(fRcmos * std::cos(phi), fRcmos * std::sin(phi), fCMOSPos0.z());
	auto rotCMOS = fCMOSRot0 * Ry;
	fCMOSPV->SetTranslation(posCMOS);
	fCMOSPV->SetRotation(new G4RotationMatrix(rotCMOS));

	G4RunManager::GetRunManager()->GeometryHasBeenModified();
}