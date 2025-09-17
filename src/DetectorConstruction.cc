#include "DetectorConstruction.hh"
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
#include "G4StepLimiter.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include  "G4OpticalPhoton.hh"
#include  "G4MaterialPropertiesTable.hh"
#include "G4Exception.hh"
#include  "G4SubtractionSolid.hh"
#include "G4OpticalSurface.hh"
#include "G4Trd.hh"
#include "G4PhysicalVolumeStore.hh"
#include  <memory>
G4VPhysicalVolume* DetectorConstruction::Construct() {
	//材料定义
	G4NistManager* nist = G4NistManager::Instance();
	G4Material* PMMA = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
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
	G4int nPoints = 4;
	for (int i = 0; i < nPoints; i++) {
		G4double eVal = eMin + i * (eMax - eMin) / (nPoints - 1);
		energy.push_back(eVal);
		spectrum.push_back(1.0);
	}
	std::vector<G4double> ref1(nPoints,0.92);

	auto almpt = new G4MaterialPropertiesTable();
	almpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 0.85));
	auto surfAl = new G4OpticalSurface("surfAl");
	surfAl->SetType(dielectric_metal);
	surfAl->SetFinish(polished);
	surfAl->SetModel(unified);
	almpt->AddProperty("REFLECTIVITY", energy, ref1);
	surfAl->SetMaterialPropertiesTable(almpt);

	auto siMpt = new G4MaterialPropertiesTable();
	siMpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 1.5));
	siMpt->AddProperty("ABSLENGTH", energy, std::vector<G4double>(nPoints, 10*mm));
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
	std::vector<G4double> rindex(nPoints, 1.49);
	std::vector<G4double> absLength(nPoints, 0.5*cm);
	G4double FastTimeConst = 200. * ns;
	G4double ScintYield = 60000.*(0.5)/ MeV;
	auto mpt = new G4MaterialPropertiesTable();
	mpt->AddProperty("RINDEX", energy, rindex);
	mpt->AddProperty("ABSLENGTH", energy, absLength);
	mpt->AddConstProperty("SCINTILLATIONYIELD", ScintYield);
	mpt->AddConstProperty("RESOLUTIONSCALE", 1.0);
	mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", FastTimeConst);
	mpt->AddProperty("SCINTILLATIONCOMPONENT1", energy, spectrum);
	ImageLayer->SetMaterialPropertiesTable(mpt);


	//几何体定义
	G4double HPGe_H = 60 * mm;
	G4double HPGe_R = 20 * mm;
	G4double Screen_L = 20 * mm;
	G4double Screen_H = 1.1 * mm;
	G4double CMOS_L = 18 * mm;
	G4double film_Scintillator_T = 1 * mm;
	G4double Tubs_H = 30 / 2 * mm;
	G4double Tubs_R = 2.9 * mm;
	G4double worldSize = 1 * m;
	G4double Voxel_H = 1 * mm;
	G4Box* solidWorld = new G4Box("World", worldSize, worldSize, worldSize);
	G4Tubs* HPGe = new G4Tubs("HPGe", 0, HPGe_R, HPGe_H, 0, 2 * CLHEP::pi);
	auto soildScreen = new G4Box("soildScreen", Screen_H,Screen_L+5*mm, Screen_L+5 * mm);
	G4ThreeVector holePos(0, 0, 0);
	auto soildScintillator = new G4Box("soildScintillator", film_Scintillator_T, Screen_L, Screen_L);
	auto soildTubs_Pb = new G4Tubs("soildTubs_Pb", 0, Tubs_R, Tubs_H-0.2*mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Al = new G4Tubs("soildTubs_Al", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Cu = new G4Tubs("soildTubs_Cu", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Fe = new G4Tubs("soildTubs_Fe", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_PE = new G4Tubs("soildTubs_PE", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Ni = new G4Tubs("soildTubs_Ni", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	voxelNum vnum;
	vnum.setNumX(10);
	vnum.setNumY(10);
	auto soildScintillator1 = new G4Box("soildScintillator", Screen_H+0.3*mm, Screen_L + 0.2* mm, Screen_L + 0.2 * mm);
	auto soildVoxel = new G4Box("soildVoxel", Voxel_H/2, CMOS_L  / vnum.GetNx(), CMOS_L  / vnum.GetNy());
	auto soildMatrixVoxel = new G4Box("soildMatrixVoxel", Voxel_H/2, CMOS_L, CMOS_L);
	auto soildSupport = new G4SubtractionSolid("Support", soildScreen, soildScintillator1);
	logicVoxel = new G4LogicalVolume(soildVoxel, Si, "logicVoxel");
	logicMatrixVoxel = new G4LogicalVolume(soildMatrixVoxel, Si, "logicMatrixVoxel");
	auto logicSupport = new G4LogicalVolume(soildSupport, Al, "logicSupport");
	G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, Galactic, "World");
	G4VPhysicalVolume* physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);
	logicHPGe = new G4LogicalVolume(HPGe, Ge, "logicHPGe");
	//CMOS阵列
	for (G4int i = 0; i < vnum.GetNx(); i++) {
		for (G4int j = 0; j < vnum.GetNy(); j++) {
			G4double xPos = (i - (vnum.GetNx() - 1) / 2.0) * (2 * CMOS_L / vnum.GetNx());
			G4double yPos = (j - (vnum.GetNy() - 1) / 2.0) * (2 * CMOS_L / vnum.GetNy());
			new G4PVPlacement(nullptr, G4ThreeVector(0, xPos, yPos), logicVoxel, "Voxel", logicMatrixVoxel, false, i * vnum.GetNy() + j);
		}
	}
	auto pRot1 = new G4RotationMatrix();
	pRot1->rotateZ(-getDeg() * CLHEP::deg);
	G4VPhysicalVolume* physMatrixVoxel = new G4PVPlacement(pRot1, G4ThreeVector((43 + Voxel_H/2) * std::cos(getDeg() * CLHEP::pi / 180), (43 + Voxel_H/2) * std::sin(getDeg() * CLHEP::pi / 180), 0), logicMatrixVoxel, "MatrixVoxel", logicWorld, false, 0,1);

	G4VisAttributes* visAttributesVoxel = new G4VisAttributes(G4Colour(0.8, 0.8, 0.8));

	visAttributesVoxel->SetForceSolid(true);
	logicVoxel->SetVisAttributes(visAttributesVoxel);
	logicMatrixVoxel->SetVisAttributes(G4VisAttributes::GetInvisible());
	auto test = CADMesh::TessellatedMesh::FromSTL("./test3.stl");
	test->SetScale(1.);
	test->SetOffset(-15, -15, -15);
	auto logicalScreen = new G4LogicalVolume(soildScreen, Al, "logicalScreen");
	//测试材料
	auto logicalScintillator = new G4LogicalVolume(soildScintillator, ImageLayer, "logicalScintillator");
	auto logicalTubs_Pb = new G4LogicalVolume(soildTubs_Pb, Pb, "logicalTubs_Pb");
	auto logicalTubs_Al = new G4LogicalVolume(soildTubs_Al, Al, "logicalTubs_Al");
	auto logicalTubs_Cu = new G4LogicalVolume(soildTubs_Cu, Cu, "logicalTubs_Cu");
	auto logicalTubs_Fe = new G4LogicalVolume(soildTubs_Fe, Fe, "logicalTubs_Fe");
	auto logicalTubs_PE = new G4LogicalVolume(soildTubs_PE, PE, "logicalTubs_PE");
	auto logicalTubs_Ni = new G4LogicalVolume(soildTubs_Ni, Ni, "logicalTubs_Ni");
	auto phyScint= new G4PVPlacement(pRot1, G4ThreeVector(((-film_Scintillator_T / 2) + (40)) * std::cos(getDeg() * CLHEP::pi / 180), ((-film_Scintillator_T / 2) + (40)) * std::sin(getDeg() * CLHEP::pi / 180), 0), logicalScintillator, "Scintillator", logicWorld, false, 0,1);
	auto logicaltest = new G4LogicalVolume(test->GetSolid(), Al, "logical");

	new G4PVPlacement(0, G4ThreeVector(), logicaltest, "test", logicWorld, false, 0);
	auto pRot2 = new G4RotationMatrix();
	pRot2->rotateZ(-getDeg() * CLHEP::deg);
	pRot2->rotateX(90 * CLHEP::deg);
	G4VPhysicalVolume* physHPGe = new G4PVPlacement(pRot2, G4ThreeVector(250 * std::sin(-getDeg() * CLHEP::pi / 180), 250 * std::cos(-getDeg() * CLHEP::pi / 180), 0), logicHPGe, "HPGe", logicWorld, false, 0);
	//可视化属性设置
	G4VisAttributes* visAttributesCollimator = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	G4VisAttributes* visAttributesHPGe = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	visAttributesCollimator->SetForceWireframe(true);
	visAttributesHPGe->SetForceSolid(true);
	logicHPGe->SetVisAttributes(visAttributesHPGe);
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
		if ((*it)->GetMaterial() == Pb) { visAttributes1 = new G4VisAttributes(G4Colour(0.0, 0.0, 1.0)); }
		else if ((*it)->GetMaterial() == Al) { visAttributes1 = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5)); }
		else if ((*it)->GetMaterial() == Cu) { visAttributes1 = new G4VisAttributes(G4Colour(1.0, 0.5, 0.5)); }
		else if ((*it)->GetMaterial() == Fe) { visAttributes1 = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0)); }
		else if ((*it)->GetMaterial() == PE) { visAttributes1 = new G4VisAttributes(G4Colour(0.5, 1.0, 0.5)); }
		else if ((*it)->GetMaterial() == Ni) { visAttributes1 = new G4VisAttributes(G4Colour(1.0, 1.0, 0.5)); }
		else { visAttributes1 = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0)); }
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
	new G4PVPlacement(pRot1, G4ThreeVector((-film_Scintillator_T / 2 + 40) * std::cos(getDeg() * CLHEP::pi / 180), ((-film_Scintillator_T / 2) + (40))* std::sin(getDeg() * CLHEP::pi / 180),0) ,logicSupport, "Support", logicWorld, false, 0,1);
	new G4LogicalSkinSurface("SurfaceSupport", logicSupport, surfAl);
	G4VisAttributes* visAttributesSupport = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0));
	visAttributesSupport->SetForceWireframe(true);
	logicSupport->SetVisAttributes(visAttributesSupport);
	G4double PMMA_H =( 1 +film_Scintillator_T / 4) * mm;
	auto solidPMMA = new G4Trd("PMMA", CMOS_L, Screen_L, CMOS_L, Screen_L, PMMA_H);
	auto logicPMMA = new G4LogicalVolume(solidPMMA, PMMA, "logicPMMA");
	auto pRot3 = new G4RotationMatrix();
	pRot3->rotateZ(-getDeg() * CLHEP::deg);
	pRot3->rotateY(90* CLHEP::deg);
	auto phyPMMA= new G4PVPlacement(pRot3, G4ThreeVector((42 - film_Scintillator_T / 4) * std::cos(getDeg() * CLHEP::pi / 180), (42 - film_Scintillator_T / 4) * std::sin(getDeg() * CLHEP::pi / 180), 0), logicPMMA, "PMMA", logicWorld, false , 0,1);
	G4VisAttributes* visAttributesPMMA = new G4VisAttributes(G4Colour(0.8, 0.2, 0.0,0.4));
	visAttributesPMMA->SetForceSolid(1);
	logicPMMA->SetVisAttributes(visAttributesPMMA);
	auto pmmaMpt = new G4MaterialPropertiesTable();
	pmmaMpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 1.49));
	pmmaMpt->AddProperty("ABSLENGTH", energy, std::vector<G4double>(nPoints, 1 * m));
	PMMA->SetMaterialPropertiesTable(pmmaMpt);
	auto surfpmma = new G4OpticalSurface("surfpmma");
	surfpmma->SetType(dielectric_metal);
	surfpmma->SetFinish(groundfrontpainted);
	surfpmma->SetModel(unified);
	auto mptSide = new G4MaterialPropertiesTable();
	mptSide->AddProperty("REFLECTIVITY", energy, std::vector<G4double>(nPoints, 0.03));
	surfpmma->SetMaterialPropertiesTable(mptSide);
	new G4LogicalSkinSurface("SurfacePMMA", logicPMMA, surfpmma);

	auto surfIdeal1 = new G4OpticalSurface("Scint_PMMA_Ideal");
	surfIdeal1->SetType(dielectric_dielectric);
	surfIdeal1->SetModel(unified);
	surfIdeal1->SetFinish(polished);


	auto surfIdeal2 = new G4OpticalSurface("Scint_PMMA_Ideal");
	surfIdeal2->SetType(dielectric_dielectric);
	surfIdeal2->SetModel(unified);
	surfIdeal2->SetFinish(polished);

	auto surfIdeal3 = new G4OpticalSurface("Scint_PMMA_Ideal");
	surfIdeal3->SetType(dielectric_dielectric);
	surfIdeal3->SetModel(unified);
	surfIdeal3->SetFinish(polished);

	auto surfIdeal4 = new G4OpticalSurface("Scint_PMMA_Ideal");
	surfIdeal4->SetType(dielectric_dielectric);
	surfIdeal4->SetModel(unified);
	surfIdeal4->SetFinish(polished);


	new G4LogicalBorderSurface("pmmaToScint", phyPMMA, phyScint, surfIdeal1);
	new G4LogicalBorderSurface("scintToPMMA", phyScint, phyPMMA, surfIdeal2);
	new G4LogicalBorderSurface("PMMA_to_Martrix", phyPMMA, physMatrixVoxel, surfIdeal3);
	new G4LogicalBorderSurface("Martrix_to_PMMA", physMatrixVoxel, phyPMMA, surfIdeal4);
	for (auto* pv : *G4PhysicalVolumeStore::GetInstance())
		pv->CheckOverlaps(1000, 0., true);
	return physWorld;
}

void DetectorConstruction::ConstructSDandField() {
	auto sdManager = G4SDManager::GetSDMpointer();

	if (!CMOSsd)
	{
		G4cout << "THis a random num" << G4Random() << G4endl << G4endl << G4endl << G4endl << G4endl << G4endl << G4endl << G4endl;
		CMOSsd = new SensitiveDetector(G4String("CMOS"), 0);
		sdManager->AddNewDetector(CMOSsd);
	}
	SetSensitiveDetector(logicVoxel, CMOSsd);

	/*if (!HPGEsd)
	{
		HPGEsd = new SensitiveDetector(G4String("HPGE"), 1);
		sdManager->AddNewDetector(HPGEsd);
	}
	SetSensitiveDetector(logicHPGe, HPGEsd);
	*/
}


void DetectorConstruction::setDeg(double nDeg)
{
	deg = nDeg;
}

double DetectorConstruction::getDeg()
{
	return deg;
}
