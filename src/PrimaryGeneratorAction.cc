#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4UniformRandPool.hh"
#include "G4SystemOfUnits.hh"

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* fdec):temp_dec(fdec){
	fParticleGun = new G4ParticleGun(1); 
	auto table = G4ParticleTable::GetParticleTable();
	fParticleGun->SetParticleDefinition(table->FindParticle("neutron"));
	
}
PrimaryGeneratorAction::~PrimaryGeneratorAction() {
	delete fParticleGun; // Clean up the particle gun

}
void PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) {

	// 角度/方向
	const G4double theta = temp_dec->getDeg() * CLHEP::pi / 180.0;
	const G4ThreeVector dir(std::cos(theta), std::sin(theta), 0.0);         // 束流方向
	const G4ThreeVector n1(-std::sin(theta), std::cos(theta), 0.0);         // XY 面内、与 dir 垂直
	const G4ThreeVector n2(0.0, 0.0, 1.0);                                   // Z 方向

	// 源面尺寸与距离
	const G4double Screen_L = 70.0 * mm;                                     // 源面方形半边长=Screen_L/2
	const G4double R = 800.0 * mm;                                           // 源到原点的距离（沿 -dir）

	// 在与 dir 垂直的平面内做均匀随机（方形分布）
	const G4double dy = (G4UniformRand() - 0.5) * Screen_L;                  // [-L/2, L/2]
	const G4double dz = (G4UniformRand() - 0.5) * Screen_L;                  // [-L/2, L/2]

	// 源面中心放在 -R * dir 处，再加横向偏移 dy*n1 + dz*n2
	const G4ThreeVector center = -R * dir;
	const G4ThreeVector pos = center + dy * n1 + dz * n2;

	// 设置粒子
	fParticleGun->SetParticlePosition(pos);
	fParticleGun->SetParticleMomentumDirection(dir);
	fParticleGun->SetParticleEnergy(4.05 * CLHEP::MeV);
	fParticleGun->GeneratePrimaryVertex(event);

}