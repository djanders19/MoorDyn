/*
 * Copyright (c) 2022, Jose Luis Cercos-Pita <jlc@core-marine.com>
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

#include "Time.hpp"
#include <sstream>

using namespace std;

namespace moordyn {

EulerScheme::EulerScheme(moordyn::Log* log)
  : TimeSchemeBase(log)
{
	name = "1st order Euler";
}

void
EulerScheme::Step(real& dt)
{
	Update(0.0, 0);
	CalcStateDeriv(0);
	r[0] = r[0] + rd[0] * dt;
	t += dt;
	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

HeunScheme::HeunScheme(moordyn::Log* log)
  : TimeSchemeBase(log)
{
	name = "2nd order Heun";
}

void
HeunScheme::Step(real& dt)
{
	// Apply the latest knew derivative, as a predictor
	r[0] = r[0] + rd[0] * dt;
	rd[1] = rd[0];
	// Compute the new derivative
	Update(0.0, 0);
	CalcStateDeriv(0);
	// Correct the integration
	r[0] = r[0] + (rd[0] - rd[1]) * (0.5 * dt);

	t += dt;
	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

RK2Scheme::RK2Scheme(moordyn::Log* log)
  : TimeSchemeBase(log)
{
	name = "2nd order Runge-Kutta";
}

void
RK2Scheme::Step(real& dt)
{
	Update(0.0, 0);

	// Compute the intermediate state
	CalcStateDeriv(0);
	t += 0.5 * dt;
	r[1] = r[0] + rd[0] * (0.5 * dt);
	Update(0.5 * dt, 1);
	// And so we can compute the new derivative and apply it
	CalcStateDeriv(0);
	r[0] = r[0] + rd[0] * dt;

	t += 0.5 * dt;
	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

RK4Scheme::RK4Scheme(moordyn::Log* log)
  : TimeSchemeBase(log)
{
	name = "4th order Runge-Kutta";
}

void
RK4Scheme::Step(real& dt)
{
	Update(0.0, 0);

	// k1
	CalcStateDeriv(0);

	// k2
	t += 0.5 * dt;
	r[1] = r[0] + rd[0] * (0.5 * dt);
	Update(0.5 * dt, 1);
	CalcStateDeriv(1);

	// k3
	r[1] = r[0] + rd[1] * (0.5 * dt);
	Update(0.5 * dt, 1);
	CalcStateDeriv(2);

	// k4
	t += 0.5 * dt;
	r[2] = r[0] + rd[2] * dt;
	Update(dt, 2);
	CalcStateDeriv(3);

	// Apply
	r[0] = r[0] + (rd[0] + rd[3]) * (dt / 6.0) + (rd[1] + rd[2]) * (dt / 3.0);

	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

template<unsigned int order>
ABScheme<order>::ABScheme(moordyn::Log* log)
  : TimeSchemeBase(log)
  , n_steps(0)
{
	stringstream s;
	s << order << "th order Adam-Bashforth";
	name = s.str();
	if (order > 4) {
		LOGWRN << name
		       << " scheme queried, but 4th order is the maximum implemented"
		       << endl;
	}
}

template<unsigned int order>
void
ABScheme<order>::Step(real& dt)
{
	Update(0.0, 0);
	shift();

	// Get the new derivative
	CalcStateDeriv(0);

	// Apply different formulas depending on the number of derivatives available
	switch (n_steps) {
		case 0:
			r[0] = r[0] + rd[0] * dt;
			break;
		case 1:
			r[0] = r[0] + rd[0] * (dt * 1.5) - rd[1] * (dt * 0.5);
			break;
		case 2:
			r[0] = r[0] + rd[0] * (dt * 23.0 / 12.0) -
			       rd[1] * (dt * 4.0 / 3.0) + rd[2] * (dt * 5.0 / 12.0);
			break;
		case 3:
			r[0] = r[0] + rd[0] * (dt * 55.0 / 24.0) -
			       rd[1] * (dt * 59.0 / 24.0) + rd[2] * (dt * 37.0 / 24.0) -
			       rd[3] * (dt * 3.0 / 8.0);
			break;
		default:
			r[0] = r[0] + rd[0] * (dt * 1901.0 / 720.0) -
			       rd[1] * (dt * 1387.0 / 360.0) + rd[2] * (dt * 109.0 / 30.0) -
			       rd[3] * (dt * 637.0 / 360.0) + rd[4] * (dt * 251.0 / 720.0);
	}

	t += dt;
	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

ImplicitEulerScheme::ImplicitEulerScheme(moordyn::Log* log,
                                         unsigned int iters,
                                         real dt_factor)
  : TimeSchemeBase(log)
  , _iters(iters)
  , _dt_factor(dt_factor)
{
	stringstream s;
	s << "k=" << dt_factor << " implicit Euler (" << iters << " iterations)";
	name = s.str();
}

void
ImplicitEulerScheme::Step(real& dt)
{
	t += _dt_factor * dt;
	for (unsigned int i = 0; i < _iters; i++) {
		r[1] = r[0] + rd[0] * (_dt_factor * dt);
		Update(_dt_factor * dt, 1);
		CalcStateDeriv(0);
	}

	// Apply
	r[0] = r[0] + rd[0] * dt;
	t += (1.0 - _dt_factor) * dt;
	Update(dt, 0);
	TimeSchemeBase::Step(dt);
}

TimeScheme*
create_time_scheme(const std::string& name, moordyn::Log* log)
{
	TimeScheme* out = NULL;
	if (str::lower(name) == "euler") {
		out = new EulerScheme(log);
	} else if (str::lower(name) == "heun") {
		out = new HeunScheme(log);
	} else if (str::lower(name) == "rk2") {
		out = new RK2Scheme(log);
	} else if (str::lower(name) == "rk4") {
		out = new RK4Scheme(log);
	} else if (str::lower(name) == "ab2") {
		out = new ABScheme<2>(log);
	} else if (str::lower(name) == "ab3") {
		out = new ABScheme<3>(log);
	} else if (str::lower(name) == "ab4") {
		out = new ABScheme<4>(log);
	} else if (str::startswith(str::lower(name), "beuler")) {
		try {
			unsigned int iters = std::stoi(name.substr(6));
			out = new ImplicitEulerScheme(log, iters, 1.0);
		} catch (std::invalid_argument) {
			stringstream s;
			s << "Invalid Backward Euler name format '" << name << "'";
			throw moordyn::invalid_value_error(s.str().c_str());
		}
	} else if (str::startswith(str::lower(name), "midpoint")) {
		try {
			unsigned int iters = std::stoi(name.substr(8));
			out = new ImplicitEulerScheme(log, iters, 0.5);
		} catch (std::invalid_argument) {
			stringstream s;
			s << "Invalid Midpoint name format '" << name << "'";
			throw moordyn::invalid_value_error(s.str().c_str());
		}
	} else {
		stringstream s;
		s << "Unknown time scheme '" << name << "'";
		throw moordyn::invalid_value_error(s.str().c_str());
	}
	if (!out)
		throw moordyn::mem_error("Failure allocating the time scheme");
	return out;
}

} // ::moordyn
