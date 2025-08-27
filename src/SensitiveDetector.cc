#include "SensitiveDetector.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"
#include "G4RunManager.hh"

#include <iomanip>
#include <sstream>

G4int temp_x, temp_y;

SensitiveDetector::SensitiveDetector(G4String name,G4int nx,G4int ny) : G4VSensitiveDetector(name), sdName(name), mNx(nx), mNy(ny) {
	G4String g4fn = "hits_" + sdName + ".csv";
	temp_x = nx;
	temp_y = ny;
	ofs.open(g4fn, std::ios::out);
}
SensitiveDetector::~SensitiveDetector() {
 if(ofs.is_open()) {
		ofs.close();
 }
}
void SensitiveDetector::WriteHeaderIfNeeded() {
	if (!ofs.is_open() || wroteHeader) return;
	ofs << "event,volume,copyNo,edep_keV,"
		"x_mm,y_mm,z_mm,px,py,pz,Ekin_MeV,globalTime_ns,particle,"	
		"pix_i,pix_j,u_local_mm,v_local_mm\n";
	wroteHeader = true;
}
void SensitiveDetector::Initialize(G4HCofThisEvent* hce) {
	WriteHeaderIfNeeded();
}

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory* history) {
	if (!ofs.is_open()) return true;
	const G4Event* event = G4RunManager::GetRunManager()->GetCurrentEvent();
	const G4int eventID = event->GetEventID();
	const G4String particle = step->GetTrack()->GetParticleDefinition()->GetParticleName();
	const bool isOptical = (particle == "opticalphoton");
	const auto pre = step->GetPreStepPoint();
	const bool entering = (pre->GetStepStatus() == fGeomBoundary);
	auto touchable = step->GetPreStepPoint()->GetTouchableHandle();
	auto vol = touchable->GetVolume();
	G4String volumeName = touchable->GetVolume()->GetName();
	G4int copyNo = touchable->GetCopyNumber();

	const G4double edep_keV = step->GetTotalEnergyDeposit()/CLHEP::keV;

	const auto pos_global = pre->GetPosition();
	const G4double x_mm = pos_global.x() / CLHEP::mm;
	const G4double y_mm = pos_global.y() / CLHEP::mm;
	const G4double z_mm = pos_global.z() / CLHEP::mm;

	const auto mom = step->GetPreStepPoint()->GetMomentum();
	const G4double px = mom.x();
	const G4double py = mom.y();
	const G4double pz = mom.z();

	const G4double Ekin_MeV = step->GetPreStepPoint()->GetKineticEnergy() / CLHEP::MeV;
	const G4double globalTime_ns = step->GetPreStepPoint()->GetGlobalTime() / CLHEP::ns;
	G4int pix_i, pix_j;
	G4int mNy = temp_y;
	if (mNy > 0) {
		pix_i = copyNo / mNy;
		pix_j = copyNo % mNy;
	}
	G4double u_local_mm = std::numeric_limits<double>::quiet_NaN();
	G4double v_local_mm = std::numeric_limits<double>::quiet_NaN();
	if (vol) {
		const G4AffineTransform& topTr = touchable->GetHistory()->GetTopTransform();
		const G4ThreeVector local = topTr.TransformPoint(pos_global);
		u_local_mm = local.x() / CLHEP::mm;
		v_local_mm = local.y() / CLHEP::mm;
	}
	if (sdName == "CMOS" && isOptical && entering) {
		step->GetTrack()->SetTrackStatus(fStopAndKill);
	}
	ofs << eventID << "," << volumeName << "," << copyNo << "," << std::fixed << std::setprecision(3) << edep_keV << ","
		<< std::fixed << std::setprecision(3) << x_mm << "," << std::fixed << std::setprecision(3) << y_mm << "," << std::fixed << std::setprecision(3) << z_mm << ","
		<< std::scientific << std::setprecision(6) << px << "," << std::scientific << std::setprecision(6) << py << "," << std::scientific << std::setprecision(6) << pz << ","
		<< std::fixed << std::setprecision(6) << Ekin_MeV << "," << std::fixed << std::setprecision(3) << globalTime_ns << "," << particle << ","
		<< pix_i << "," << pix_j << "," 
		<< std::fixed << std::setprecision(3) << u_local_mm  << "," 
		<< std::fixed << std::setprecision(3) << v_local_mm 
		<< "\n";


	return true;
}
void SensitiveDetector::EndOfEvent(G4HCofThisEvent* hce) {
	if (ofs.is_open()) ofs.flush();
}