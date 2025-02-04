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

/** @file Connection.hpp
 * C++ API for the moordyn::Connection object
 */

#pragma once

#include "Misc.hpp"
#include "IO.hpp"
#include <utility>

#ifdef USE_VTK
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#endif

using namespace std;

namespace moordyn {

class Line;
class Waves;

/** @class Connection Connection.hpp
 * @brief A connection for a line endpoint
 *
 * Each line must have 2 connections at each endpoint, which are used to define
 * how those points are moving. There are 3 basic types of connections:
 *
 *  - Fixed: The point is indeed fixed, either to a unmovable point (i.e. an
 *           anchor) or to a Body
 *  - Free: The point freely moves, with its own translation degrees of freedom,
 *          to provide a connection point between multiple mooring lines or an
 *          unconnected termination point of a Line, which could have a clump
 *          weight or float via the point's mass and volume parameters
 *  - Coupled: The connection position and velocity is externally imposed
 */
class Connection : public io::IO
{
  public:
	/** @brief Costructor
	 * @param log Logging handler
	 */
	Connection(moordyn::Log* log);

	/** @brief Destructor
	 */
	~Connection();

	/// Attached lines to the connection
	typedef struct _attachment
	{
		/// The attached line
		Line* line;
		/// The attachment end point
		EndPoints end_point;
	} attachment;

  private:
	// ENVIRONMENTAL STUFF
	/// Global struct that holds environmental settings
	EnvCond* env;
	/// global Waves object
	moordyn::Waves* waves;

	/// Lines attached to this connection node
	std::vector<attachment> attached;

	/** @defgroup conn_constants Constants set at startup from input file
	 * @{
	 */

	/// Mass [kg]
	real conM;
	/// Volume [m3]
	real conV;
	/// Force [N]
	vec conF;
	/// Drag coefficient
	real conCdA;
	/// Added mass coefficient
	real conCa;

	/**
	 * @}
	 */

	/** @defgroup conn_common_line Common properties with line internal nodes
	 * @{
	 */

	/// node position [x/y/z]
	vec r;
	/// node velocity[x/y/z]
	vec rd;

	/**
	 * @}
	 */

	/// fairlead position for vessel/coupled node types [x/y/z]
	vec r_ves;
	/// fairlead velocity for vessel/coupled node types [x/y/z]
	vec rd_ves;

	/// total force on node
	vec Fnet;

	/// node mass + added mass matrices
	mat M;

	/** @defgroup conn_wave Wave data
	 *  @{
	 */

	/// free surface elevation
	real zeta;
	/// dynamic pressure
	real PDyn;
	/// wave velocities
	vec U;
	/// wave accelerations
	vec Ud;

	/**
	 * @}
	 */

  public:
	/** @brief Types of connections
	 */
	typedef enum
	{
		/// Is coupled, i.e. is controlled by the user
		COUPLED = -1,
		/// Is free to move, controlled by MoorDyn
		FREE = 0,
		/// Is fixed, either to a location or to another moving entity
		FIXED = 1,
		// Some aliases
		VESSEL = COUPLED,
		FAIRLEAD = COUPLED,
		CONNECT = FREE,
		ANCHOR = FIXED,
	} types;

	/** @brief Return a string with the name of a type
	 *
	 * This tool is useful mainly for debugging
	 */
	static string TypeName(types t)
	{
		switch (t) {
			case COUPLED:
				return "COUPLED";
			case FREE:
				return "FREE";
			case FIXED:
				return "FIXED";
		}
		return "UNKNOWN";
	}

	/// Connection ID
	int number;
	/// Connection type
	types type;

	/** @brief flag indicating whether wave/current kinematics will be
	 * considered for this linec
	 *
	 * - 0: none, or use value set externally for each node of the object
	 * - 1: interpolate from stored
	 * - 2: call interpolation function from global Waves grid
	 */
	int WaterKin;

	/** @brief Setup the connection
	 *
	 * Always call this function after the construtor
	 * @param number_in The connection identifier. The identifiers starts at 1,
	 * not at 0.
	 * @param type_in One of COUPLED, FREE or FIXED
	 * @param r0_in The initial position
	 * @param M_in The mass
	 * @param V_in The volume
	 * @param F_in The initial force on the node
	 * @param CdA_in Product of drag coefficient and projected area
	 * @param Ca_in Added mass coefficient used along with V to calculate added
	 * mass on node
	 */
	void setup(int number_in,
	           types type_in,
	           vec r0_in,
	           double M_in,
	           double V_in,
	           vec F_in,
	           double CdA_in,
	           double Ca_in);

	/** @brief Attach a line endpoint to this connection
	 * @param theLine The line to be attached
	 * @param end_point The line endpoint
	 */
	void addLine(moordyn::Line* theLine, EndPoints end_point);

	/** @brief Dettach a line
	 * @param line The line
	 * @return The line end point that was attached to the connection
	 * @throws moordyn::invalid_value_error If there is no an attached line
	 * with the provided @p lineID
	 */
	EndPoints removeLine(Line* line);

	/** @brief Get the list of attachments
	 * @return The list of attachments
	 */
	inline std::vector<attachment> getLines() const { return attached; }

	/** @brief Initialize the FREE connection state
	 * @return The position (first) and the velocity (second)
	 * @throws moordyn::invalid_value_error If it is not a FREE connection
	 */
	std::pair<vec, vec> initialize();

	/** @brief Get the connection state
	 * @param r_out The output position [x,y,z]
	 * @param rd_out The output velocity [x,y,z]
	 */
	inline void getState(vec& r_out, vec& rd_out)
	{
		r_out = r;
		rd_out = rd;
	};

	/** @brief Get the connection state
	 * @param r_out The output position [x,y,z]
	 * @param rd_out The output velocity [x,y,z]
	 */
	inline std::pair<vec, vec> getState() { return std::make_pair(r, rd); }

	/** @brief Get the force on the connection
	 * @param Fnet_out The output force [x,y,z]
	 */
	void getFnet(vec& Fnet_out);

	/** @brief Get the mass matrix
	 * @param M_out The output mass matrix
	 */
	void getM(mat& M_out);

	/** @brief Get the output
	 * @param outChan The query
	 * @return The data, 0.0 if no such data can be found
	 */
	real GetConnectionOutput(OutChanProps outChan);

	/** @brief Set the environmental data
	 * @param env_in Global struct that holds environmental settings
	 * @param waves_in Global Waves object
	 */
	void setEnv(EnvCond* env_in, moordyn::Waves* waves_in);

	/** @brief Multiply the drag by a factor
	 *
	 * function for boosting drag coefficients during IC generation
	 * @param scaler Drag factor
	 */
	inline void scaleDrag(real scaler) { conCdA *= scaler; }

	/** @brief Initialize the time step integration
	 *
	 * Called at the beginning of each coupling step to update the boundary
	 * conditions (fairlead kinematics) for the proceeding line time steps
	 * @param rFairIn Fairlead position
	 * @param rdFairIn Fairlead velocity
	 * @param time Simulation time
	 * @throws moordyn::invalid_value_error If it is not a COUPLED connection
	 */
	void initiateStep(vec rFairIn, vec rdFairIn);

	/** @brief Take the kinematics from the fairlead information
	 *
	 * Sets Connection states and ends of attached lines ONLY if this Connection
	 * is driven externally, i.e. type = COUPLED (otherwise shouldn't be called)
	 * @param time Local time within the time step (from 0 to dt)
	 * @throws moordyn::invalid_value_error If it is not a COUPLED connection
	 */
	void updateFairlead(real time);

	/** @brief Take the kinematics from the fairlead information
	 *
	 * sets Connection states and ends of attached lines ONLY if this Connection
	 * is attached to a body, i.e. type = FIXED (otherwise shouldn't be called)
	 * @param r_in Position
	 * @param rd_in Velocity
	 * @throws moordyn::invalid_value_error If it is not a FIXED connection
	 */
	void setKinematics(vec r_in, vec rd_in);

	/** @brief Set the state variables
	 *
	 * sets Connection states and ends of attached lines ONLY if this Connection
	 * is free, i.e. type = FREE (otherwise shouldn't be called)
	 * @param pos Position
	 * @param vel Velocity
	 * @throws moordyn::invalid_value_error If it is not a FREE connection
	 */
	void setState(vec pos, vec vel);

	/** @brief Calculate the forces and state derivatives of the connection
	 * @param return The states derivatives, i.e. the velocity (first) and the
	 * acceleration (second)
	 * @throws moordyn::invalid_value_error If it is not a FREE connection
	 */
	std::pair<vec, vec> getStateDeriv();

	/** @brief Calculate the force and mass contributions of the connect on the
	 * parent body
	 * @param Fnet_out Output Force about body ref point
	 * @param M_out Output Mass matrix about body ref point
	 * @param rBody The body position. If NULL, {0, 0, 0} is considered
	 */
	void getNetForceAndMass(vec6& Fnet_out,
	                        mat6& M_out,
	                        vec rBody = vec::Zero());

	/** @brief Calculates the forces and mass on the connection, including from
	 * attached lines
	 *
	 * @return MOORDYN_SUCCESS upon success, an error code otherwise
	 */
	moordyn::error_id doRHS();

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
	/** @brief Produce a VTK object
	 * @return The new VTK object
	 */
	vtkSmartPointer<vtkPolyData> getVTK() const;

	/** @brief Save the connection on a VTK (.vtp) file
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
	 */
	void saveVTK(const char* filename) const;
#endif

#ifdef USEGL
	void drawGL(void);
#endif
};

} // ::moordyn
