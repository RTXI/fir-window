/*
Copyright (C) 2011 Georgia Institute of Technology

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/*
* Computes FIR filter coefficients using the window method.
*/

#include <boost/circular_buffer.hpp>
#include <DSP/gen_win.h>
#include <DSP/rectnglr.h>
#include <DSP/trianglr.h>
#include <DSP/hamming.h>
#include <DSP/hann.h>
#include <DSP/dolph.h>
#include <DSP/kaiser.h>
#include <DSP/fir_dsgn.h>
#include <DSP/lin_dsgn.h>
#include <DSP/firideal.h>

#include <default_gui_model.h>
#include <settings.h>
#include <cstdlib>

class FIRwindow : public DefaultGUIModel {
	
	Q_OBJECT
	
	public:
		FIRwindow(void);
		virtual ~FIRwindow(void);
	
		void execute(void);
		//  void createGUI(DefaultGUIModel::variable_t *, int);
		void customizeGUI(void);
	
		enum window_t	{
			RECT=0, TRI, HAMM, HANN, CHEBY, KAISER
		};
	
		enum filter_t {
			LOWPASS=0, HIGHPASS, BANDPASS, BANDSTOP
		};
	
	protected:
		virtual void update(DefaultGUIModel::update_flags_t);
	
	private:
		// inputs, states
		boost::circular_buffer<double> signalin;
		double* convolution;
		double out;
		double dt;
		int n;

		// filter parameters
		double* h1; // left for debugging purposes
		//	double* h2;
		double* h3; // filter coefficients
		window_t window_shape;
		filter_t filter_type;
		int num_taps;
		double lambda1; // cutoff frequencies
		double lambda2;
		double Kalpha; // Kaiser window sidelobe attenuation parameter
		double Calpha; // Chebyshev window sidelobe attenuation parameter
	
		// FIRwindow functions
		void initParameters();
		void bookkeep();
		void makeFilter();
		GenericWindow *disc_window;
		FirIdealFilter *filter_design;
		QComboBox *windowShape;
		QComboBox *filterType;

		// Saving FIR filter data to file without Data Recorder
		bool OpenFile(QString);
		QFile dataFile;
		QTextStream stream;

		void doSave(Settings::Object::State &) const;
		void doLoad(const Settings::Object::State &);

	private slots:
		// all custom slots
		void saveFIRData(); // write filter parameters to a file
		void updateWindow(int);
		void updateFilterType(int);
};
