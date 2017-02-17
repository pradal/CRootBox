#ifndef ROOTSYSTEM_H_
#define ROOTSYSTEM_H_

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <chrono>
#include <random>

#include "ModelParameter.h"
#include "Root.h"
#include "soil.h"

class Root;
class TropismFunction;



/**
 * RootSystem
 *
 * This class manages all model parameter, the simulation, stores the base roots, and offers utility functions
 *
 */
class RootSystem
{

	friend Root;  // obviously :-)

public:

	enum TropismTypes { tt_plagio=0, tt_gravi=1, tt_exo=2, tt_hydro=3 };  ///< root tropism
	enum GrowthFunctionTypes { gft_negexp=1, gft_linear=2 }; // root growth function
	enum OutputTypes { ot_segments=0, ot_polylines=1 }; ///< used for postprocessing
	enum ScalarTypes { st_type=0, st_radius=1, st_order=2,  st_time=3, st_length=4, st_surface=5, st_one=6, st_userdata1=7, st_userdata2=8, st_userdata3=9,
		st_length_times_ud1=10 }; ///< @see RootSystem::getScalar
	static const std::vector<std::string> scalarTypeNames; ///< the corresponding names

	RootSystem() { initRTP(); };
	virtual ~RootSystem();

	// Parameter input output
	void setRootTypeParameter(RootTypeParameter p) { rtparam.at(p.type-1) = p; } ///< set the root type parameter to the index type-1
	RootTypeParameter* getRootTypeParameter(int type) { return &rtparam.at(type-1); } ///< Returns the i-th root parameter set (i=1..n)
	void setRootSystemParameter(const RootSystemParameter& rsp) { rsparam = rsp; }; ///< sets the root system parameters
	RootSystemParameter* getRootSystemParameter() { return &rsparam; } ///< gets the root system parameters

	void openFile(std::string filename, std::string subdir="modelparameter/"); ///< Reads root paramter and plant parameter
	int readParameters(std::istream & cin); ///< Reads root parameters from an input stream
	void writeParameters(std::ostream & os) const; ///< Writes root parameters

	// Simulation
	void setGeometry(SignedDistanceFunction* geom) { geometry = geom; }; ///< Optionally, sets a confining geometry (call before RootSystem::initialize())
	void setSoil(SoilProperty* soil_) { soil = soil_; }; ///< Optionally sets a soil for hydro tropism (call before RootSystem::initialize())
	void reset(); ///< Resets the root class, keeps the root type parameters
	void initialize(int basal=4, int shootborne=5); ///< Creates the base roots, call before simulation and after setting the plant and root parameters
	void simulate(double dt); ///< Simulates root system growth for time span dt

	// TODO
	// dynamic information what happened last time step
	// getNodeUpdates
	// getNewNodes
	// getNewSegments

	// call back functions
	virtual Root* createRoot(int lt, Vector3d  h, double delay, Root* parent, double pbl, int pni);
	///< Creates a new lateral root, overwrite or change this method to use more spezialised root classes
	virtual TropismFunction* createTropismFunction(int tt, double N, double sigma);
	///< Creates the tropisms, overwrite or change this method to add more tropisms
	virtual GrowthFunction* createGrowthFunction(int gft);
	///< Creates the growth function per root type, overwrite or change this method to add more tropisms

	// Analysis of simulation results
	int getNumberOfNodes() const { return nid+1; } ///< Number of nodes of the root system
	std::vector<Root*> getRoots() const; ///< Represents the root system as sequential vector of roots
	std::vector<Root*> getBaseRoots() { return baseRoots; } ///< Base roots are tap root, basal roots, and shoot borne roots
	std::vector<Vector3d> getRootTips(std::vector<Root*> roots=std::vector<Root*>()) const; ///< Positions of the root tips TODO node or segment indices make more sense
    std::vector<Vector3d> getRootBases(std::vector<Root*> roots=std::vector<Root*>()) const; ///< Positions of the root bases TODO node or segment  indices make more sense
    std::vector<Vector3d> getNodes(int ot=RootSystem::ot_segments, std::vector<Root*> roots=std::vector<Root*>()) const; ///< Copies all root system nodes into a vector
	std::vector<Vector2i> getSegments(int ot=RootSystem::ot_segments, std::vector<Root*> roots=std::vector<Root*>()) const; ///< Copies all segments indices into a vector
	std::vector<Root*> getSegmentsOrigin(int ot=RootSystem::ot_segments, std::vector<Root*> roots=std::vector<Root*>()) const; ///< Copies a pointer to the root containing the segment
	std::vector<double> getNETimes(int ot=RootSystem::ot_segments, std::vector<Root*> roots=std::vector<Root*>()) const; ///< Copies all node emergence times into a vector
	std::vector<double> getScalar(int ot=RootSystem::ot_polylines, int stype=RootSystem::st_length, std::vector<Root*> roots=std::vector<Root*>()) const; ///< Copies a scalar root parameter that is constant per root to a vector

	// Output Simulation results
	void write(std::string name, int type = ot_polylines) const; /// writes simulation results (type is determined from file extension in name)
	void writeRSML(std::ostream & os) const; ///< Writes current simulation results as RSML
	void writeVTP(std::ostream & os, int type = ot_polylines) const; ///< Writes current simulation results as VTP (VTK polydata file)
	void writeGeometry(std::ostream & os) const; ///< Writes the current confining geometry (e.g. a plant container) as paraview python script
	void writeDGF(std::ostream & os) const; ///< Writes the segments of the root system in DGF format used in Dumux

	// void std::toString() const; TODO

	// random stuff
	void setSeed(double seed); ///< help fate (sets the seed of all random generators)
	double rand() { return UD(gen); } ///< Uniformly distributed random number (0,1)
	double randn() { return ND(gen); } ///< Normally distributed random number (0,1)

	int rsmlReduction = 5; ///< only each n-th node is written to the rsml file (to coarsely adjust axial resolution for output)

private:
	void initRTP();

	const int maxtypes = 100;
	std::vector<RootTypeParameter> rtparam; ///< Parameter set for each root type
	RootSystemParameter rsparam; ///< Plant parameter

	void writeRSMLMeta(std::ostream & os) const;
	void writeRSMLPlant(std::ostream & os) const;

	int getRootIndex() { rid++; return rid; } ///< returns next unique root id, called by the constructor of Root
	int getNodeIndex() { nid++; return nid; } ///< returns next unique node id, called by Root::addNode()

	std::vector<Root*> baseRoots;  ///< Base roots of the root system

	std::vector<GrowthFunction*> gf; ///< Growth function per root type
	std::vector<TropismFunction*> tf;  ///< Tropism per root type

	SignedDistanceFunction* geometry = new SignedDistanceFunction(); ///< Confining geometry (unconfined by default)
	SoilProperty* soil = nullptr; ///< callback for hydro, or chemo tropism (needs to set before initialize())

	double simtime = 0;
	int rid = -1; // unique root id counter
	int nid = -1; // unique root id counter

	std::mt19937 gen = std::mt19937(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_real_distribution<double> UD = std::uniform_real_distribution<double>(0,1);
	std::normal_distribution<double> ND = std::normal_distribution<double>(0,1);

};

#endif /* ROOTSYSTEM_H_ */
