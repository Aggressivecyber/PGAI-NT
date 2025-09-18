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


DetectorConstruction::DetectorConstruction()
{
	DefinitionMatertial();
}


void DetectorConstruction::DefinitionMatertial()
{
	G4NistManager* nist = G4NistManager::Instance();
	G4Material* PMMA = nist->FindOrBuildMaterial("G4_PLEXIGLASS");
	G4Material* air = nist->FindOrBuildMaterial("G4_AIR");
	G4Material* Pb = nist->FindOrBuildMaterial("G4_Pb");
	G4Material* Fe = nist->FindOrBuildMaterial("G4_Fe");
	G4Material* Cu = nist->FindOrBuildMaterial("G4_Cu");
	G4Material* Si = nist->FindOrBuildMaterial("G4_Si");
	G4Material* Al = nist->FindOrBuildMaterial("G4_Al");
	G4Material* Ge = nist->FindOrBuildMaterial("G4_Ge");
	G4Material* Ni = nist->FindOrBuildMaterial("G4_Ni");
	G4Material* PE = nist->FindOrBuildMaterial("G4_POLYETHYLENE");
	G4Material* Galactic = nist->FindOrBuildMaterial("G4_Galactic");
	G4Element* Li = new G4Element("Lithium-6", "Li6", 3., 6.015 * g / mole);
	G4Element* F = nist->FindOrBuildElement("F");
	G4Element* Zn = nist->FindOrBuildElement("Zn");
	G4Element* S = nist->FindOrBuildElement("S");
	G4Element* Ag = nist->FindOrBuildElement("Ag");


	G4double density_LiF = 2.64 * g / cm3;
	G4Material* LiF = new G4Material("LiF", density_LiF, 2);
	LiF->AddElement(Li, 1);
	LiF->AddElement(F, 1);


	G4double density_ZnS_Ag = 4.10 * g / cm3;
	G4Material* ZnS_Ag = new G4Material("ZnS_Ag", density_ZnS_Ag, 3);
	ZnS_Ag->AddElement(Zn, 0.653);
	ZnS_Ag->AddElement(S, 0.3465);
	ZnS_Ag->AddElement(Ag, 0.0005);// Silver is 0.05wt% in ZnS:Ag, Abdalla et al., J Mater Sci: Mater Electron, 2022


	G4Material* ImageLayer = new G4Material("ImageLayer", 0.323 * density_LiF + 0.647 * density_ZnS_Ag + 0.03 * PE->GetDensity(), 3);
	ImageLayer->AddMaterial(LiF, 0.323);
	ImageLayer->AddMaterial(ZnS_Ag, 0.647);
	ImageLayer->AddMaterial(PE, 0.03); //Eljen Technology – EJ-426 Specification Sheet

	
	
}

G4VPhysicalVolume* DetectorConstruction::Construct() {


	auto ImageLayer = G4Material::GetMaterial("ImageLayer");     
	auto PMMA = G4Material::GetMaterial("G4_PLEXIGLASS"); 
	auto Al = G4Material::GetMaterial("G4_Al");           
	auto Ge = G4Material::GetMaterial("G4_Ge");         
	auto Si = G4Material::GetMaterial("G4_Si");          
	auto Pb = G4Material::GetMaterial("G4_Pb");            
	auto Fe = G4Material::GetMaterial("G4_Fe");
	auto Cu = G4Material::GetMaterial("G4_Cu");
	auto Ni = G4Material::GetMaterial("G4_Ni");
	auto PE = G4Material::GetMaterial("G4_POLYETHYLENE");
	auto Galactic = G4Material::GetMaterial("G4_Galactic");


	std::vector<G4double> energy;
	std::vector<G4double> spectrum;
	G4int nPoints = 4;
	std::vector<G4double> ref1(nPoints, 0.02);
	std::vector<G4double> rindex(nPoints, 1.49);
	std::vector<G4double> absLength(nPoints, 0.5 * cm);
	G4double eMin = 1240.0 / 550.0 * eV; // 2.25 eV
	G4double eMax = 1240.0 / 380.0 * eV; // 3.26 eV

	for (int i = 0; i < nPoints; i++) {
		G4double eVal = eMin + i * (eMax - eMin) / (nPoints - 1);
		energy.push_back(eVal);
		spectrum.push_back(1.0);
	}

	auto almpt = new G4MaterialPropertiesTable();
	auto surfAl = new G4OpticalSurface("surfAl");
	surfAl->SetType(dielectric_metal);
	surfAl->SetFinish(groundfrontpainted);
	surfAl->SetModel(unified);
	almpt->AddProperty("REFLECTIVITY", energy, ref1);
	surfAl->SetMaterialPropertiesTable(almpt);


	auto siMpt = new G4MaterialPropertiesTable();
	siMpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 1.5));
	siMpt->AddProperty("ABSLENGTH", energy, std::vector<G4double>(nPoints, 0.05 * mm));
	Si->SetMaterialPropertiesTable(siMpt);


	auto mptGalactic = new G4MaterialPropertiesTable();
	std::vector<G4double> GalRindex(nPoints, 1.0);
	mptGalactic->AddProperty("RINDEX", energy, GalRindex);
	Galactic->SetMaterialPropertiesTable(mptGalactic);



	G4double FastTimeConst = 20. * ns;
	G4double ScintYield = 60000. * (0.1) / MeV;
	auto mpt = new G4MaterialPropertiesTable();
	mpt->AddProperty("RINDEX", energy, rindex);
	mpt->AddProperty("ABSLENGTH", energy, absLength);
	mpt->AddConstProperty("SCINTILLATIONYIELD", ScintYield);
	mpt->AddConstProperty("RESOLUTIONSCALE", 10);
	mpt->AddConstProperty("SCINTILLATIONTIMECONSTANT1", FastTimeConst);
	mpt->AddProperty("SCINTILLATIONCOMPONENT1", energy, spectrum);
	ImageLayer->SetMaterialPropertiesTable(mpt);



	auto pmmaMpt = new G4MaterialPropertiesTable();
	pmmaMpt->AddProperty("RINDEX", energy, std::vector<G4double>(nPoints, 1.49));
	pmmaMpt->AddProperty("ABSLENGTH", energy, std::vector<G4double>(nPoints, 100 * mm));
	PMMA->SetMaterialPropertiesTable(pmmaMpt);

	//几何体定义
	G4double HPGe_H = 60 * mm;
	G4double HPGe_R = 20 * mm;
	G4double Screen_L = 20 * mm;
	G4double Screen_H = 1.1 * mm;
	G4double CMOS_L = 18 * mm;
	
	G4double Tubs_H = 30 / 2 * mm;
	G4double Tubs_R = 2.9 * mm;
	G4double worldSize = 1 * m;
	G4double Voxel_H = 1 * mm;
	G4Box* solidWorld = new G4Box("World", worldSize, worldSize, worldSize);
	G4Tubs* HPGe = new G4Tubs("HPGe", 0, HPGe_R, HPGe_H, 0, 2 * CLHEP::pi);



	G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, Galactic, "World");
	G4VPhysicalVolume* physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);
//	logicHPGe = new G4LogicalVolume(HPGe, Ge, "logicHPGe");
	//CMOS阵列



	const G4double R = 120. * mm ;
	const G4double pix_t = 1*mm;           
	const G4int    Nang = 10;               
	const G4int    Nz = 10;                 
	const G4double cellZ = (60./Nz) * mm;           
	const G4double gapT = 0.05 * mm;          
	const G4double gapZ = 0.02 * mm;            
	const G4bool   checkOL = true;


	const G4double Htot = Nz * cellZ + (Nz - 1) * gapZ;        
	const G4double dPhi = 2. * CLHEP::pi / Nang;
	const G4double chord = 2. * R * std::sin(0.5 * dPhi);
	const G4double cellT = std::max(0.010 * mm, chord - gapT);
	const G4double bow = (cellT * cellT) / (8. * R);   
	const G4double eps = 0.01 * mm;                  

	const G4double Rin_carrier = (R - 0.5 * pix_t) - bow - eps;
	const G4double Rout_carrier = (R + 0.5 * pix_t) + bow + eps;

	auto solidCarrier = new G4Tubs("CTRingCarrier",
		Rin_carrier, Rout_carrier,
		0.5 * Htot, 0., 2. * CLHEP::pi);
	auto logicCarrier = new G4LogicalVolume(solidCarrier,PMMA, "CTRingCarrierLV");
	new G4PVPlacement(nullptr, G4ThreeVector(), logicCarrier, "CTRingCarrier", logicWorld, false, 0, checkOL);

	auto solidPix = new G4Box("CTPixelSolid", 0.5 * pix_t, 0.5 * cellT, 0.5 * cellZ);
	logicPixel = new G4LogicalVolume(solidPix, Si, "CTPixelLV");
	auto v = new G4VisAttributes(G4Colour(0.9, 0.2, 0.2, 0.7));
	v->SetForceSolid(true);
	logicPixel->SetVisAttributes(v);

	for (G4int iz = 0; iz < Nz; ++iz) {
		const G4double zc = -0.5 * Htot + (iz + 0.5) * cellZ + iz * gapZ;
		for (G4int k = 0; k < Nang; ++k) {
			const G4double phi = k * dPhi;
			auto rot = new G4RotationMatrix();
			rot->rotateZ(-phi);
			const G4ThreeVector pos(R * std::cos(phi), R * std::sin(phi), zc);
			const G4int copyNo = iz * Nang + k;
			new G4PVPlacement(rot, pos,
				logicPixel, "Pixel",
				logicCarrier, false, copyNo, checkOL);
		}
	}





	// 环形闪烁体
	G4double film_Scintillator_T = 1.5 * mm;        
	const G4double R_sci = R - 8.0 * mm;       
	const G4double Rin_sci = R_sci - 0.5 * film_Scintillator_T;
	const G4double Rout_sci = R_sci + 0.5 * film_Scintillator_T;
	auto solidScintRing = new G4Tubs("ScintRing",
		Rin_sci, Rout_sci,
		0.5 * Htot,             
		0., 2. * CLHEP::pi);

	auto logicScintRing = new G4LogicalVolume(solidScintRing, ImageLayer, "ScintRingLV");

	new G4PVPlacement(nullptr, G4ThreeVector(),
		logicScintRing, "ScintRing",
		logicWorld, false, 0, true);  

	{
		auto visSc = new G4VisAttributes(G4Colour(0.0, 1.0, 0.0, 0.4)); 
		visSc->SetForceSolid(true);
		logicScintRing->SetVisAttributes(visSc);
		auto userLimitsSc = new G4UserLimits(400. * CLHEP::um);
		logicScintRing->SetUserLimits(userLimitsSc);
	}

	//吸收外溢光子
	const G4double capThick = 1 * mm;                       
	const G4double dz_gap = 0.20 * mm;                          
	const G4double rMargin = 5 * mm;                         
	const G4double capHalfZ = 0.5 * capThick;
	const G4double zTop = +(0.5 * Htot + dz_gap + capHalfZ); 
	const G4double zBot = -(0.5 * Htot + dz_gap + capHalfZ); 

	auto solidBlackCap = new G4Tubs("BlackCapSolid",
		0,          
		Rout_sci + rMargin,           
		capHalfZ, 0., 2. * CLHEP::pi);

	auto logicBlackCap = new G4LogicalVolume(solidBlackCap, Al, "BlackCapLV");


	{
		auto vis = new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.1));
		vis->SetForceSolid(true);
		logicBlackCap->SetVisAttributes(vis);
	}

	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zTop),
		logicBlackCap, "BlackCapTop",
		logicWorld, false, 0, true);

	new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zBot),
		logicBlackCap, "BlackCapBottom",
		logicWorld, false, 1, true);

	new G4LogicalSkinSurface("SurfBlackCap", logicBlackCap, surfAl);

	//吸收反射光子
	const G4double R_abs = R_sci - 2 * mm;
	const G4double Rin_abs = R_abs - 1 * mm;
	const G4double Rout_abs = R_abs + 1 * mm;

	auto solidAbs= new G4Tubs("AbsRing",
		Rin_abs, Rout_abs,
		0.5 * Htot,
		0., 2. * CLHEP::pi);

	auto logicAbsRing = new G4LogicalVolume(solidAbs, Galactic, "logicAbsRing");
	new G4PVPlacement(nullptr, G4ThreeVector(), logicAbsRing, "pAbsRing", logicWorld, false, 0, false);
	new G4LogicalSkinSurface("AbsRingSurf", logicAbsRing, surfAl);

	auto vis = new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.1));
	vis->SetForceWireframe(1);
	logicAbsRing->SetVisAttributes(vis);



	//导光板
	auto Vac = G4Material::GetMaterial("G4_Galactic");

	const G4double r_in_g = Rout_sci + 0.02 * mm;
	const G4double r_out_g = Rin_carrier - 0.02 * mm;

	const G4double t_radial = r_out_g - r_in_g;
	if (t_radial <= 0.) {
		G4Exception("DetectorConstruction", "GuideBand<=0", FatalException,
			"导光分隔带径向空间不足，请调整几何。");
	}

	auto solidWallHost = new G4Tubs("WallHost", r_in_g, r_out_g, 0.5 * Htot, 0., 2. * CLHEP::pi);
	auto logicWallHost = new G4LogicalVolume(solidWallHost, Vac, "WallHostLV");
	new G4PVPlacement(nullptr, {}, logicWallHost, "WallHost", logicWorld, false, 0, true);

	const G4double wallArcW = 0.02 * mm;                  
	const G4double dphiWall = std::max(1e-6, wallArcW / std::max((r_in_g + r_out_g) / 2., 1. * mm));

	auto solidWallPhi = new G4Tubs("WallPhiSolid",
		r_in_g, r_out_g,
		0.5 * Htot,
		-0.5 * dphiWall, dphiWall);
	auto logicWallPhi = new G4LogicalVolume(solidWallPhi, Al, "WallPhiLV");
	new G4LogicalSkinSurface("WallPhiSkin", logicWallPhi, surfAl);

	// 可视化（深灰）
	{
		auto v = new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.9));
		v->SetForceSolid(true); logicWallPhi->SetVisAttributes(v);
	}

	for (G4int k = 0; k < Nang; ++k) {
		const G4double phi_b = (k + 0.5) * dPhi;
		auto rot = new G4RotationMatrix(); rot->rotateZ(phi_b);
		new G4PVPlacement(rot, {}, logicWallPhi, "WallPhi",
			logicWallHost, false, k, true);
	}

	const G4double wallZ = std::min(0.8 * gapZ, 0.15 * mm);  
	auto solidWallZ = new G4Tubs("WallZSolid",
		r_in_g, r_out_g,
		0.5 * wallZ, 0., 2. * CLHEP::pi);
	auto logicWallZ = new G4LogicalVolume(solidWallZ, Al, "WallZLV");
	new G4LogicalSkinSurface("WallZSkin", logicWallZ, surfAl);

	{
		auto v = new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.9));
		v->SetForceSolid(true); logicWallZ->SetVisAttributes(v);
	}

	for (G4int iz = 0; iz < Nz - 1; ++iz) {
		const G4double zg = -0.5 * Htot + (iz + 1) * cellZ + iz * gapZ + 0.5 * gapZ;
		new G4PVPlacement(nullptr, G4ThreeVector(0, 0, zg),
			logicWallZ, "WallZ",
			logicWallHost, false, iz, true);
	}



	//测试材料
	auto test = CADMesh::TessellatedMesh::FromSTL("./test3.stl");
	test->SetScale(1.);
	test->SetOffset(-15, -15, -15);
	auto logicaltest = new G4LogicalVolume(test->GetSolid(), Al, "logical");
	new G4PVPlacement(nullptr, G4ThreeVector(), logicaltest, "test", logicWorld, false, 0);

	auto soildTubs_Pb = new G4Tubs("soildTubs_Pb", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Al = new G4Tubs("soildTubs_Al", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Cu = new G4Tubs("soildTubs_Cu", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Fe = new G4Tubs("soildTubs_Fe", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_PE = new G4Tubs("soildTubs_PE", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto soildTubs_Ni = new G4Tubs("soildTubs_Ni", 0, Tubs_R, Tubs_H - 0.2 * mm, 0, 2 * CLHEP::pi);
	auto logicalTubs_Pb = new G4LogicalVolume(soildTubs_Pb, Pb, "logicalTubs_Pb");
	auto logicalTubs_Al = new G4LogicalVolume(soildTubs_Al, Al, "logicalTubs_Al");
	auto logicalTubs_Cu = new G4LogicalVolume(soildTubs_Cu, Cu, "logicalTubs_Cu");
	auto logicalTubs_Fe = new G4LogicalVolume(soildTubs_Fe, Fe, "logicalTubs_Fe");
	auto logicalTubs_PE = new G4LogicalVolume(soildTubs_PE, PE, "logicalTubs_PE");
	auto logicalTubs_Ni = new G4LogicalVolume(soildTubs_Ni, Ni, "logicalTubs_Ni");

	G4VisAttributes* visAttributesHPGe = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	visAttributesHPGe->SetForceSolid(true);
	//logicHPGe->SetVisAttributes(visAttributesHPGe);
	std::vector<G4LogicalVolume*> logicalTubs;
	logicalTubs.push_back(logicalTubs_Pb);
	logicalTubs.push_back(logicalTubs_Al);
	logicalTubs.push_back(logicalTubs_Cu);
	logicalTubs.push_back(logicalTubs_Fe);
	logicalTubs.push_back(logicalTubs_PE);
	logicalTubs.push_back(logicalTubs_Ni);
	G4int i = 0;
	G4double R1 = 10 * mm;
	for (std::vector<G4LogicalVolume*>::iterator it = logicalTubs.begin(); it != logicalTubs.end(); it++) {
		G4double theta = i * ((1. / 3.) * CLHEP::pi);

		std::string name = ("tubs");
		std::string temp = name;
		name += "_";
		name += (*it)->GetName();
		G4ThreeVector pos(R1 * std::sin(theta),
			R1 * std::cos(theta),
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


	G4VisAttributes* vis1Attributes = new G4VisAttributes(G4Colour(0.5, 0.5, 0.5));
	logicaltest->SetVisAttributes(vis1Attributes);

	G4UserLimits* userLimits1 = new G4UserLimits(1 * CLHEP::mm);
	logicWorld->SetUserLimits(userLimits1);

	return physWorld;
}

void DetectorConstruction::ConstructSDandField() {
	auto SDman = G4SDManager::GetSDMpointer();
	static SensitiveDetector* mySD = nullptr;
	if (!mySD) {
		mySD = new SensitiveDetector("CMOS","CMOSHitCollection");
		SDman->AddNewDetector(mySD);
	}
	logicPixel->SetSensitiveDetector(mySD);


	/*if (!HPGEsd)
	{
		HPGEsd = new SensitiveDetector(G4String("HPGE"), 1);
		sdManager->AddNewDetector(HPGEsd);
	}
	SetSensitiveDetector(logicHPGe, HPGEsd);
	*/
}

