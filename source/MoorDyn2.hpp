/*
 * Copyright (c) 2022, Matt Hall
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/** @file Moordyn2.hpp
 * C++ API for the moordyn::MoorDyn object, which is the main simulation handler
 */

#pragma once

#include "MoorDynAPI.h"
#include "IO.hpp"
#include "Misc.hpp"

#include "Time.hpp"
#include "Waves.hpp"
#include "MoorDyn.h"
#include "Line.hpp"
#include "Connection.hpp"
#include "Rod.hpp"
#include "Body.hpp"

#ifdef USE_VTK
#include <vtkSmartPointer.h>
#include <vtkMultiBlockDataSet.h>
#endif

namespace moordyn {

/** @class MoorDyn
 * @brief A Mooring system
 *
 * This class contains everything required to hold a whole mooring system,
 * making everything thread-friendly easier
 */
class MoorDyn : public io::IO
{
  public:
	/** @brief Constructor
	 *
	 * Remember to call Init() to initialize the mooring system
	 *
	 * @param infilename The input file, if either NULL or "", then
	 * "Mooring/lines.txt" will be considered
	 */
	MoorDyn(const char* infilename = NULL);

	/** @brief Destuctor
	 */
	~MoorDyn();

	/** @brief Initializes Moordyn, reading the input file and setting up the
	 * mooring lines
	 * @param x Position vector
	 * @param xd Velocity vector
	 * @param skip_ic true to skip computing the initial condition, false
	 * otherwise. You might be insterested on skipping the initial condition
	 * computation if for install you plan to load a previously saved simulation
	 * state
	 * @note You can know the number of components required for \p x and \p xd
	 * with the function MoorDyn::NCoupledDOF()
	 * @return MOORDYN_SUCCESS If the mooring system is correctly initialized,
	 * an error code otherwise (see @ref moordyn_errors)
	 */
	moordyn::error_id Init(const double* x,
	                       const double* xd,
	                       bool skip_ic = false);

	/** @brief Runs a time step of the MoorDyn system
	 * @param x Position vector
	 * @param xd Velocity vector
	 * @param f Output forces
	 * @param t Simulation time
	 * @param dt Time step
	 * @return MOORDYN_SUCCESS If the mooring system is correctly evolved,
	 * an error code otherwise (see @ref moordyn_errors)
	 * @note You can know the number of components required for \p x, \p xd and
	 * \p f with the function MoorDyn::NCoupledDOF()
	 */
	moordyn::error_id Step(const double* x,
	                       const double* xd,
	                       double* f,
	                       double& t,
	                       double& dt);

	/** @brief Get the connections
	 */
	inline vector<Body*> GetBodies() const { return BodyList; }

	/** @brief Get the connections
	 */
	inline vector<Rod*> GetRods() const { return RodList; }

	/** @brief Get the connections
	 */
	inline vector<Connection*> GetConnections() const { return ConnectionList; }

	/** @brief Get the lines
	 */
	inline vector<Line*> GetLines() const { return LineList; }

	/** @brief Return the number of coupled Degrees Of Freedom (DOF)
	 *
	 * This should match with the number of components of the positions and
	 * velocities passed to MoorDyn::Init() and MoorDyn::Step()
	 *
	 * The number of coupled DOF is computed as
	 *
	 * \f$n_{dof} = 6 * n_{body} + 3 * n_{conn} + 6 * n_{rod} + 3 * n_{pinn}\f$
	 *
	 * with \f$n_{body}\f$ the number of coupled bodies, \f$n_{conn}\f$ the
	 * number of coupled connections, \f$n_{rod}\f$ the number of cantilevered
	 * coupled rods, and \f$n_{pinn}\f$ the number of pinned coupled rods
	 *
	 * @return The number of coupled DOF
	 */
	inline unsigned int NCoupledDOF() const
	{
		unsigned int n = 6 * CpldBodyIs.size() + 3 * CpldConIs.size();
		for (auto rodi : CpldRodIs) {
			if (RodList[rodi]->type == Rod::COUPLED)
				n += 6; // cantilevered rods
			else
				n += 3; // pinned rods
		}
		return n;
	}

	/** @brief Get the wave kinematics instance
	 *
	 * The wave kinematics instance is used if env.WaveKin is one of
	 * WAVES_FFT_GRID, WAVES_GRID, WAVES_FFT_NODE, WAVES_NODE or if
	 * env.Currents is not CURRENTS_NONE
	 * @return The wave knematics instance
	 */
	inline moordyn::Waves* GetWaves() const { return waves; }

	/** @brief Initializes the external Wave kinetics
	 *
	 * This is only used if env.WaveKin = WAVES_EXTERNAL
	 * @return The number of points where the wave kinematics shall be provided
	 */
	inline unsigned int ExternalWaveKinInit()
	{
		npW = 0;

		for (auto line : LineList)
			npW += line->getN() + 1;

		// allocate arrays to hold data that could be passed in
		U_1.clear();
		Ud_1.clear();
		U_2.clear();
		Ud_2.clear();
		U_1.assign(npW, vec::Zero());
		U_1.assign(npW, vec::Zero());
		U_1.assign(npW, vec::Zero());
		U_1.assign(npW, vec::Zero());
		tW_1 = 0.0;
		tW_2 = 0.0;

		return npW;
	}

	/** @brief Get the number of points where the waves kinematics shall be
	 * provided
	 *
	 * If env.WaveKin is not WAVES_EXTERNAL, this will return 0
	 * @return The number of evaluation points
	 * @see MoorDyn::ExternalWaveKinInit()
	 * @see MoorDyn::GetWaves()
	 */
	inline unsigned int ExternalWaveKinGetN() const { return npW; }

	/** @brief Get the points where the waves kinematics shall be provided
	 *
	 * The kinematics are provided in those points just if env.WaveKin is
	 * WAVES_EXTERNAL. Otherwise moordyn::Waves is used
	 * @param r The output coordinates
	 * @return MOORDYN_SUCCESS If the data is correctly set, an error code
	 * otherwise  (see @ref moordyn_errors)
	 * @deprecated Use MoorDyn::GetWaveKinPoints() instead
	 * @see MoorDyn::ExternalWaveKinInit()
	 * @see MoorDyn::GetWaves()
	 */
	inline moordyn::error_id DEPRECATED GetWaveKinCoordinates(double* r) const
	{
		unsigned int i = 0;
		for (auto line : LineList) {
			std::vector<vec> rvec = line->getNodeCoordinates();
			for (unsigned int j = 0; j < rvec.size(); j++)
				moordyn::vec2array(rvec[j], &(r[3 * (i + j)]));
			i += line->getN() + 1;
		}
		return MOORDYN_SUCCESS;
	}

	/** @brief Get the points where the waves kinematics shall be provided
	 *
	 * The kinematics are provided in those points just if env.WaveKin is
	 * WAVES_EXTERNAL. Otherwise moordyn::Waves is used
	 * @return The points where the wave kinematics shall be evaluated
	 * @see MoorDyn::ExternalWaveKinInit()
	 * @see MoorDyn::GetWaves()
	 */
	inline std::vector<vec> ExternalWaveKinGetPoints() const
	{
		std::vector<vec> rout;
		for (auto line : LineList) {
			std::vector<vec> rline = line->getNodeCoordinates();
			vector_extend(rout, rline);
		}
		return rout;
	}

	/** @brief Set the kinematics of the waves
	 *
	 * Use this function if env.WaveKin = 1
	 * @param U The velocities at the points
	 * @param Ud The accelerations at the points
	 * @param t Simulation time
	 * @throw moordyn::invalid_value_error If the size of @p U and @p Ud is not
	 * the same than the one reported by MoorDyn::ExternalWaveKinInit()
	 * @see MoorDyn_InitExtWaves()
	 * @see MoorDyn_GetWavesCoords()
	 */
	inline void DEPRECATED SetWaveKin(std::vector<vec> const& U,
	                                  std::vector<vec> const& Ud,
	                                  double t)
	{
		ExternalWaveKinSet(U, Ud, t);
	}

	/** @brief Set the kinematics of the waves
	 *
	 * Use this function if env.WaveKin = 1
	 * @param U The velocities at the points
	 * @param Ud The accelerations at the points
	 * @param t Simulation time
	 * @throw moordyn::invalid_value_error If the size of @p U and @p Ud is not
	 * the same than the one reported by MoorDyn::ExternalWaveKinInit()
	 * @see MoorDyn_InitExtWaves()
	 * @see MoorDyn_GetWavesCoords()
	 */
	inline void ExternalWaveKinSet(std::vector<vec> const& U,
	                               std::vector<vec> const& Ud,
	                               double t)

	{
		if ((U.size() != Ud.size()) || (U.size() != U_1.size())) {
			LOGERR << "Invalid input size."
			       << "Have you called MoorDyn::ExternalWaveKinInit()?"
			       << std::endl;
			throw moordyn::invalid_value_error("Invalid input size");
		}

		// shift the data
		tW_2 = tW_1;
		U_2 = U_1;
		Ud_2 = Ud_1;

		// Set the new info
		tW_1 = t;
		U_1 = U;
		Ud_1 = Ud;
	}

	/** @brief Produce the packed data to be saved
	 *
	 * The produced data can be used afterwards to restore the saved information
	 * afterwards calling Deserialize(void).
	 *
	 * Thus, this function is not processing the information that is extracted
	 * from the definition file
	 * @return The packed data
	 */
	virtual std::vector<uint64_t> Serialize(void);

	/** @brief Unpack the data to restore the Serialized information
	 *
	 * This is the inverse of Serialize(void)
	 * @param data The packed data
	 * @return A pointer to the end of the file, for debugging purposes
	 */
	virtual uint64_t* Deserialize(const uint64_t* data);

#ifdef USE_VTK
	/** @brief Produce a VTK object of the whole system
	 * @return The new VTK object
	 * @see moordyn::Line::getVTK()
	 * @see moordyn::Rod::getVTK()
	 */
	vtkSmartPointer<vtkMultiBlockDataSet> getVTK() const;

	/** @brief Save the whole system on a VTK (.vtm) file
	 *
	 * Many times, it is more convenient for the user to save each instance in
	 * a separate file. However, if the number of subinstances is too large,
	 * that would not be an option anymore. Then you can use this function to
	 * save the whole system in a multiblock VTK file
	 *
	 * @param filename The output file name
	 * @throws output_file_error If VTK reports
	 * vtkErrorCode::FileNotFoundError, vtkErrorCode::CannotOpenFileError
	 * or vtkErrorCode::NoFileNameError
	 * @throws invalid_value_error If VTK reports
	 * vtkErrorCode::UnrecognizedFileTypeError or vtkErrorCode::FileFormatError
	 * @throws mem_error If VTK reports
	 * vtkErrorCode::OutOfDiskSpaceError
	 * @throws unhandled_error If VTK reports
	 * any other error
	 * @see moordyn::Line::saveVTK()
	 * @see moordyn::Rod::saveVTK()
	 */
	void saveVTK(const char* filename) const;
#endif

  protected:
	/** @brief Read the input file, setting up all the requird objects and their
	 * relationships
	 *
	 * This function is called from the constructor, so this information is
	 * ready when MoorDyn::Init() is called
	 *
	 * @return MOORDYN_SUCCESS If the input file is correctly loaded and all
	 * the objects are consistently set, an error code otherwise
	 * (see @ref moordyn_errors)
	 */
	moordyn::error_id ReadInFile();

	/** @brief Get the forces
	 * @param f The forces array
	 * @return MOORDYN_SUCCESS If the forces are correctly set, an error code
	 * otherwise (see @ref moordyn_errors)
	 * @note You can know the number of components required for \p f with the
	 * function MoorDyn::NCoupledDOF()
	 */
	inline moordyn::error_id GetForces(double* f) const
	{
		if (!NCoupledDOF()) {
			if (f)
				_log->Cout(MOORDYN_WRN_LEVEL)
				    << "Warning: Forces have been asked on "
				    << "the coupled entities, but there are no such entities"
				    << std::endl;
			return MOORDYN_SUCCESS;
		}
		if (NCoupledDOF() && !f) {
			_log->Cout(MOORDYN_ERR_LEVEL)
			    << "Error: " << __PRETTY_FUNC_NAME__
			    << " called with a NULL forces pointer, but there are "
			    << NCoupledDOF() << " coupled Degrees Of Freedom" << std::endl;
			return MOORDYN_INVALID_VALUE;
		}
		unsigned int ix = 0;
		for (auto l : CpldBodyIs) {
			// BUG: These conversions will not be needed in the future
			const vec6 f_i = BodyList[l]->getFnet();
			moordyn::vec62array(f_i, f + ix);
			ix += 6;
		}
		for (auto l : CpldRodIs) {
			const vec6 f_i = RodList[l]->getFnet();
			if (RodList[l]->type == Rod::COUPLED) {
				moordyn::vec62array(f_i, f + ix);
				ix += 6; // for cantilevered rods 6 entries will be taken
			} else {
				moordyn::vec2array(f_i(Eigen::seqN(0, 3)), f + ix);
				ix += 3; // for pinned rods 3 entries will be taken
			}
		}
		for (auto l : CpldConIs) {
			vec fnet;
			ConnectionList[l]->getFnet(fnet);
			moordyn::vec2array(fnet, f + ix);
			ix += 3;
		}
		return MOORDYN_SUCCESS;
	}

  private:
	/// The input file
	string _filepath;
	/// The input file basename
	string _basename;
	/// The input file directory
	string _basepath;

	// factor by which to boost drag coefficients during dynamic relaxation IC
	// generation
	real ICDfac;
	// convergence analysis time step for IC generation
	real ICdt;
	// max time for IC generation
	real ICTmax;
	// threshold for relative change in tensions to call it converged
	real ICthresh;
	// temporary wave kinematics flag used to store input value while keeping
	// env.WaveKin=0 for IC gen
	moordyn::waves_settings WaveKinTemp;
	/// (s) desired mooring line model time step
	real dtM0;
	/// (s) desired output interval (the default zero value provides output at
	/// every call to MoorDyn)
	real dtOut;

	/// The time integration scheme
	TimeScheme* _t_integrator;

	/// General options of the Mooryng system
	EnvCond env;
	/// The ground body, which is unique
	Body* GroundBody;
	/// Waves object that will be created to hold water kinematics info
	Waves* waves = NULL;

	/// array of pointers to hold line library types
	vector<LineProps*> LinePropList;
	/// array of pointers to hold rod library types
	vector<RodProps*> RodPropList;
	/// array of pointers to hold failure condition structs
	vector<FailProps*> FailList;
	/// array of pointers to connection objects (line joints or ends)
	vector<Body*> BodyList;
	/// array of pointers to Rod objects
	vector<Rod*> RodList;
	/// array of pointers to connection objects (line joints or ends)
	vector<moordyn::Connection*> ConnectionList;
	/// array of pointers to line objects
	vector<moordyn::Line*> LineList;

	/// array of starting indices for Lines in "states" array
	vector<unsigned int> LineStateIs;
	/// array of starting indices for indendent Connections in "states" array
	vector<unsigned int> ConnectStateIs;
	/// array of starting indices for independent Rods in "states" array
	vector<unsigned int> RodStateIs;
	/// array of starting indices for Bodies in "states" array
	vector<unsigned int> BodyStateIs;

	/// vector of free body indices in BodyList vector
	vector<unsigned int> FreeBodyIs;
	/// vector of coupled/fairlead body indices in BodyList vector
	vector<unsigned int> CpldBodyIs;

	/// vector of free rod indices in RodList vector (this includes pinned rods
	/// because they are partially free and have states)
	vector<unsigned int> FreeRodIs;
	/// vector of coupled/fairlead rod indices in RodList vector
	vector<unsigned int> CpldRodIs;

	/// vector of free connection indices in ConnectionList vector
	vector<unsigned int> FreeConIs;
	/// vector of coupled/fairlead connection indices in ConnectionList vector
	vector<unsigned int> CpldConIs;

	/// Number of used state vector components
	unsigned int nX;
	/// full size of state vector array including extra space for detaching up
	/// to all line ends, each which could get its own 6-state connect
	/// (nXtra = nX + 6 * 2 * LineList.size())
	unsigned int nXtra;

	/// number of points that wave kinematics are input at
	/// (if using env.WaveKin=1)
	unsigned int npW;
	/// time corresponding to the wave kinematics data
	real tW_1;
	/// array of wave velocity at each of the npW points at time tW_1
	std::vector<vec> U_1;
	/// array of wave acceleration at each of the npW points
	std::vector<vec> Ud_1;
	/// time corresponding to the wave kinematics data
	real tW_2;
	/// array of wave velocity at each of the npW points at time tW_2
	std::vector<vec> U_2;
	/// array of wave acceleration at each of the npW points
	std::vector<vec> Ud_2;

	/// main output file
	ofstream outfileMain;

	/// a vector to hold ofstreams for each body, line or rod
	vector<shared_ptr<ofstream>> outfiles;

	/// list of structs describing selected output channels for main out file
	vector<OutChanProps> outChans;

	/** @brief Create the log file if queried, close it otherwise
	 *
	 * Depending on the value of env.writeLog value, a Log file stream will
	 * be open or closed
	 */
	inline moordyn::error_id SetupLog()
	{
		// env.writeLog = 0 -> MOORDYN_NO_OUTPUT
		// env.writeLog = 1 -> MOORDYN_WRN_LEVEL
		// env.writeLog = 2 -> MOORDYN_MSG_LEVEL
		// env.writeLog >= 3 -> MOORDYN_DBG_LEVEL
		int log_level = MOORDYN_ERR_LEVEL - env.writeLog;
		if (log_level >= MOORDYN_ERR_LEVEL)
			log_level = MOORDYN_NO_OUTPUT;
		if (log_level < MOORDYN_DBG_LEVEL)
			log_level = MOORDYN_DBG_LEVEL;
		GetLogger()->SetLogLevel(log_level);

		if (env.writeLog > 0) {
			moordyn::error_id err = MOORDYN_SUCCESS;
			string err_msg;
			stringstream filepath;
			filepath << _basepath << _basename << ".log";
			try {
				GetLogger()->SetFile(filepath.str().c_str());
			}
			MOORDYN_CATCHER(err, err_msg);
			if (err != MOORDYN_SUCCESS)
				LOGERR << "Unable to create the log at '" << filepath.str()
				       << "': " << std::endl
				       << err_msg << std::endl;
			else
				LOGMSG << "MoorDyn v2 log file with output level "
				       << log_level_name(GetLogger()->GetLogLevel()) << " at '"
				       << filepath.str() << "'" << std::endl;
			return err;
		}

		return MOORDYN_SUCCESS;
	}

	/** @brief Helper to read a nonlinear curve data from text
	 *
	 * This function can also read single values
	 *
	 * @param entry Either the value or the file where the curve can be read
	 *              from
	 * @param x Array of x values, 0 for single values
	 * @param y Array of y values, or the single value
	 * @return MOORDYN_SUCCESS if the value/curve is correctly read, an error
	 * code otherwise
	 */
	moordyn::error_id read_curve(const char* entry,
	                             vector<double>& x,
	                             vector<double>& y)
	{
		try {
			y.push_back((real)std::stold(entry));
			// If we reach this point, it is a valid number
			x.push_back(0.0);
			return MOORDYN_SUCCESS;
		} catch (std::out_of_range) {
			LOGERR << "" << std::endl;
			return MOORDYN_INVALID_INPUT;
		} catch (std::invalid_argument) {
			// Do nothing, just proceed to read the curve file
		}

		string fpath = _basepath + entry;
		LOGMSG << "Loading a curve from '" << fpath << "'..." << std::endl;
		ifstream f(fpath);
		if (!f.is_open()) {
			LOGERR << "Cannot read the file '" << fpath << "'" << std::endl;
			return MOORDYN_INVALID_INPUT_FILE;
		}

		vector<string> flines;
		while (f.good()) {
			string fline;
			getline(f, fline);
			flines.push_back(fline);
		}
		f.close();

		for (auto fline : flines) {
			vector<string> entries = moordyn::str::split(fline, ' ');
			if (entries.size() < 2) {
				LOGERR << "Error: Bad curve point" << std::endl
				       << "\t'" << fline << "'" << std::endl
				       << "\t2 fields required, but just " << entries.size()
				       << " are provided" << std::endl;
				return MOORDYN_INVALID_INPUT;
			}
			x.push_back(atof(entries[0].c_str()));
			y.push_back(atof(entries[0].c_str()));
			LOGDBG << "(" << x.back() << ", " << y.back() << ")" << std::endl;
		}

		LOGMSG << "OK" << std::endl;
		return MOORDYN_SUCCESS;
	}

	/** @brief Helper to read a nonlinear curve data from text
	 *
	 * This function can also read single values. This function is prepared
	 * for working with the old API
	 *
	 * @param entry Either the value or the file where the curve can be read
	 *              from
	 * @param c The value, if it is just a coefficient
	 * @param n Number of points in the curves
	 * @param x Array of x values
	 * @param y Array of y values
	 * @return MOORDYN_SUCCESS if the value/curve is correctly read, an error
	 * code otherwise
	 */
	moordyn::error_id read_curve(const char* entry,
	                             double* c,
	                             int* n,
	                             double* x,
	                             double* y)
	{
		vector<double> xv, yv;
		const moordyn::error_id error = read_curve(entry, xv, yv);
		if (error != MOORDYN_SUCCESS)
			return error;

		if (xv.size() == 1) {
			*c = yv.back();
			n = 0;
			return MOORDYN_SUCCESS;
		}

		if (xv.size() > nCoef) {
			_log->Cout(MOORDYN_ERR_LEVEL)
			    << "Error: Too much points in the curve" << std::endl
			    << "\t" << xv.size() << " points given, but just " << nCoef
			    << " are accepted" << std::endl;
			return MOORDYN_INVALID_INPUT;
		}

		*c = 0.0;
		*n = xv.size();
		memcpy(x, xv.data(), xv.size() * sizeof(double));
		memcpy(y, yv.data(), yv.size() * sizeof(double));

		return MOORDYN_SUCCESS;
	}

	/** @brief Get the value of a specific output channel
	 *
	 * This function might trhow moordyn::invalid_value_error exceptions
	 * @param channel Output channel
	 * @return The corresponding value
	 */
	inline double GetOutput(const OutChanProps channel) const
	{
		if (channel.OType == 1)
			return LineList[channel.ObjID - 1]->GetLineOutput(channel);
		else if (channel.OType == 2)
			return ConnectionList[channel.ObjID - 1]->GetConnectionOutput(
			    channel);
		else if (channel.OType == 3)
			return RodList[channel.ObjID - 1]->GetRodOutput(channel);
		stringstream s;
		s << "Error: output type of " << channel.Name
		  << " does not match a supported object type";
		MOORDYN_THROW(MOORDYN_INVALID_VALUE, s.str().c_str());
		return 0.0;
	}

	/** @brief Detach lines from a failed connection
	 * @param failure The failure structure
	 */
	void detachLines(FailProps* failure);

	/** @brief Print the output files
	 *
	 * Ths function is indeed chcking before that the output should be printed
	 * @param t Time instant
	 * @param dt Time step
	 * @return MOORDYN_SUCCESS if the output is correctly printed, an error
	 * code otherwise
	 */
	moordyn::error_id AllOutput(double t, double dt);
};

} // ::moordyn
