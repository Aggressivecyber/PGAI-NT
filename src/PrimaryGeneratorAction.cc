#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "Randomize.hh"
#include "PGAIConfig.hh"

#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* /*det*/)
	: temp_dec(nullptr) {
	fParticleGun = new G4ParticleGun(1);
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron"));
}

PrimaryGeneratorAction::~PrimaryGeneratorAction() {
	delete fParticleGun;
}

void PrimaryGeneratorAction::SetParticleGun(G4ParticleGun* gun) { fParticleGun = gun; }
G4ParticleGun* PrimaryGeneratorAction::GetParticleGun() const { return fParticleGun; }

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {
	// 源固定在 -x, 束流沿 +x (样品台旋转产生投影角度, 源不动)
	G4ThreeVector beamDir(1, 0, 0);
	G4ThreeVector perpY(0, 1, 0);
	G4ThreeVector perpZ(0, 0, 1);

	// 源斑 (均匀方斑, 照射野)
	G4double half = gConfig.spotSize * 0.5;
	G4double dy = (G4UniformRand() - 0.5) * 2.0 * half;
	G4double dz = (G4UniformRand() - 0.5) * 2.0 * half;

	// 源位置: (-sourceDistance, centerY, centerZ) + 源斑偏移
	G4ThreeVector srcPos(-gConfig.sourceDistance, gConfig.sourceCenterY, gConfig.sourceCenterZ);
	srcPos += perpY * dy + perpZ * dz;

	// 角发散: 方向加高斯抖动
	G4double divSigma = gConfig.divergence;
	G4double dThetaY = (divSigma > 0) ? G4RandGauss::shoot(0.0, divSigma) : 0.0;
	G4double dThetaZ = (divSigma > 0) ? G4RandGauss::shoot(0.0, divSigma) : 0.0;
	G4ThreeVector dir = beamDir + perpY * std::tan(dThetaY) + perpZ * std::tan(dThetaZ);
	dir = dir.unit();

	// 能量: 准单能高斯展宽
	G4double ekin = gConfig.energy;
	if (gConfig.energySpread > 0) {
		G4double sigma = gConfig.energySpread * gConfig.energy;
		ekin = G4RandGauss::shoot(gConfig.energy, sigma);
		if (ekin < 0.01 * MeV) ekin = 0.01 * MeV;
	}

	fParticleGun->SetParticlePosition(srcPos);
	fParticleGun->SetParticleMomentumDirection(dir);
	fParticleGun->SetParticleEnergy(ekin);
	fParticleGun->GeneratePrimaryVertex(event);
}
