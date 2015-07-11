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
* FIRwindow
* Computes FIR filter coefficients using the window method and does straight convolution with an input signal.
*
*/

#include <fir-window.h>
#include <main_window.h>
#include <rtfile.h>
#include <math.h>
#include <algorithm>
#include <numeric>
#include <time.h>

#include <QtGui>
#include <sys/stat.h>

//create plug-in
extern "C" Plugin::Object *
createRTXIPlugin(void) {
	return new FIRwindow(); // Change the name of the plug-in here
}

//set up parameteinputs/outputs derived from defaultGUIModel, calls for initialization, creation, update, and refresh of GUI
static DefaultGUIModel::variable_t vars[] = {
	{ "Input", "Input to Filter", DefaultGUIModel::INPUT, },
	{ "Output", "Output of Filter", DefaultGUIModel::OUTPUT, },
	{ "# Taps", "Number of Filter Taps (Odd Number)", DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
	{ "Frequency 1 (Hz)", "Cut off Frequency #1 as Fraction of Pi, used for Lowpass/Highpass", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Frequency 2 (Hz)", "Cut off Frequency #1 as Fraction of Pi, not used for Lowpass/Highpass", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Chebyshev (dB)", "Attenuation Parameter for Chebyshev Window", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Kaiser Alpha", "Attenuation Parameter for Kaiser Window", DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
	{ "Time (s)", "Time (s)", DefaultGUIModel::STATE, },
};

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

FIRwindow::FIRwindow(void) : DefaultGUIModel("FIR Window", ::vars, ::num_vars) {
	setWhatsThis(
	"<p><b>FIR Window:</b><br>This plugin computes FIR filter coefficients using the window method "
	"given the number of taps desired and the cutoff frequencies. For a lowpass or highpass filter, use the"
	"Freq 1 parameter. For a bandpass or bandstop filter, use both frequencies to define the frequency band."
	"Since this plug-in computes new filter coefficients whenever you change the parameters, you should not"
	"change any settings during real-time.</p>");
	
	initParameters();
	DefaultGUIModel::createGUI(vars, num_vars);
	customizeGUI();
	update(INIT);
	refresh(); // refresh the GUI
	QTimer::singleShot(0, this, SLOT(resizeMe()));
}

FIRwindow::~FIRwindow(void) {}

//execute, the code block that actually does the signal processing
void FIRwindow::execute(void) {
	signalin.push_back(input(0));
	systime = count * dt; // current time, s
	out = 0;
	for (n = num_taps; n < 2 * num_taps; n++) out += h3[n] * signalin[n];

	output(0) = out;
	
	count++; // increment count to measure time
	return;
}

void FIRwindow::update(DefaultGUIModel::update_flags_t flag) {
	switch (flag) {
		case INIT:
			setParameter("# Taps", QString::number(num_taps));
			setParameter("Frequency 1 (Hz)", QString::number(lambda1));
			setParameter("Frequency 2 (Hz)", QString::number(lambda2));
			setParameter("Kaiser Alpha", QString::number(Kalpha));
			setParameter("Chebyshev (dB)", QString::number(Calpha));
			setState("Time (s)", systime);
			windowShape->setCurrentIndex(window_shape);
			filterType->setCurrentIndex(filter_type);
			break;

		case MODIFY:
			num_taps = int(getParameter("# Taps").toDouble());
			if (num_taps % 2 == 0) num_taps = num_taps + 1;

			setParameter("# Taps", QString::number(num_taps));
			lambda1 = getParameter("Frequency 1 (Hz)").toDouble();
			lambda2 = getParameter("Frequency 2 (Hz)").toDouble();
			Kalpha = getParameter("Kaiser Alpha").toDouble();
			Calpha = getParameter("Chebyshev (dB)").toDouble();
			window_shape = window_t(windowShape->currentIndex());
			filter_type = filter_t(filterType->currentIndex());
			bookkeep();
			makeFilter();
			break;
	
		case PAUSE:
			output(0) = 0; // stop command in case pause occurs in the middle of command
			break;
	
		case UNPAUSE:
			bookkeep();
			break;

		case PERIOD:
			dt = RT::System::getInstance()->getPeriod() * 1e-9;
			break;

		default:
			break;
	}
}

// custom functions, as defined in the header file

void FIRwindow::initParameters() {
	dt = RT::System::getInstance()->getPeriod() * 1e-9; // s
	signalin.clear();
	assert(signalin.size() == 0);
	window_shape = HAMM;
	filter_type = BANDPASS;
	num_taps = 9;
	lambda1 = 0.1; // Hz / pi
	lambda2 = 0.6; // Hz / pi
	Calpha = 70; // dB
	Kalpha = 1.5;
	makeFilter();
	bookkeep();
}

void FIRwindow::bookkeep() {
	count = 0;
	systime = 0;
	signalin.set_capacity(2 * num_taps);
	convolution = new double[num_taps];
	for (int i = 0; i < 2 * num_taps; i++)	signalin.push_back(0);// pad with zeros

	assert(signalin.full()); // check size and padding
	assert(signalin.size() == 2*num_taps);
	assert(signalin.capacity() == 2*num_taps);
}

void FIRwindow::updateWindow(int index) {
	if (index == 0) {
		window_shape = RECT;
		makeFilter();
	} else if (index == 1) {
		window_shape = TRI;
		makeFilter();
	} else if (index == 2) {
		window_shape = HAMM;
		makeFilter();
	} else if (index == 3) {
		window_shape = HANN;
		makeFilter();
	} else if (index == 4) {
		window_shape = CHEBY;
		makeFilter();
	} else if (index == 5) {
		window_shape = KAISER;
		makeFilter();
	}
}

void FIRwindow::updateFilterType(int index) {
	if (index == 0) {
		filter_type = LOWPASS;
		makeFilter();
	} else if (index == 1) {
		filter_type = HIGHPASS;
		makeFilter();
	} else if (index == 3) {
		filter_type = BANDSTOP;
		makeFilter();
	}
}

void FIRwindow::makeFilter() {
	switch (window_shape) {
		case RECT: // rectangular
			disc_window = new RectangularWindow(num_taps);
			break;
		
		case TRI: // triangular
			disc_window = new TriangularWindow(num_taps, 1);
			break;
		
		case HAMM: // Hamming
			disc_window = new HammingWindow(num_taps);
			break;
	
		case HANN: // Hann
			disc_window = new HannWindow(num_taps, 1);
			break;

		case CHEBY: // Dolph-Chebyshev
			disc_window = new DolphChebyWindow(num_taps, Calpha);
			break;

		case KAISER:
			disc_window = new KaiserWindow(num_taps, Kalpha);
			break;
	} // end of switch on window_shape
	
	h1 = disc_window->GetDataWindow();
	filter_design = new FirIdealFilter(num_taps, lambda1, lambda2, filter_type);
	//	h2 = filter_design->GetCoefficients();
	filter_design->ApplyWindow(disc_window);
	h3 = filter_design->GetCoefficients();
}

void FIRwindow::saveFIRData() {
	QFileDialog* fd = new QFileDialog(this, "Save File As");//, TRUE);
	fd->setFileMode(QFileDialog::AnyFile);
	fd->setViewMode(QFileDialog::Detail);
	QString fileName;
	if (fd->exec() == QDialog::Accepted) {
		QStringList files = fd->selectedFiles();
		if (!files.isEmpty()) fileName = files.takeFirst();
		
		if (OpenFile(fileName))	{
			//			stream.setPrintableData(true);
			switch (filter_type) {
				case LOWPASS:
					stream << QString("LOWPASS lambda1=") << (double) lambda1;
					break;
		
				case HIGHPASS:
					stream << QString("HIGHPASS lambda1=") << (double) lambda1;
					break;
				
				case BANDPASS:
					stream << QString("BANDPASS lambda1=") << (double) lambda1
					<< " lambda2= " << (double) lambda2;
					break;
			
				case BANDSTOP:
					stream << QString("BANDSTOP lambda1=") << (double) lambda1
					<< " lambda2= " << (double) lambda2;
					break;
			}
			
			switch (window_shape) {
				case RECT: // rectangular
					stream << QString(" RECT taps:") << num_taps << "\n";
					break;
		
				case TRI: // triangular
					stream << QString(" TRI taps:") << num_taps << "\n";
					break;

				case HAMM: // Hamming
					stream << QString(" HAMM taps:") << num_taps << "\n";
					break;
		
				case HANN: // Hann
					stream << QString(" HANN taps:") << num_taps << "\n";
					break;
				
				case CHEBY: // Dolph-Chebyshev
					stream << QString(" CHEBY taps:") << num_taps << " alpha:" << Calpha	<< "\n";
					break;
	
				case KAISER:
					stream << QString(" KAISER taps:") << num_taps << " alpha:"	<< Kalpha << "\n";
					break;
			}

			for (int i = 0; i < num_taps; i++) {
				stream << QString("h[") << i << "] = " << (double) h3[i] << "\n";
			}
			dataFile.close();
		} else {
			QMessageBox::information(this, "FIR filter: Save filter parameters",
			"There was an error writing to this file. You can view\n"
			"the parameters in the terminal.\n");
		}
	}
}

bool FIRwindow::OpenFile(QString FName) {
	dataFile.setFileName(FName);
	if (dataFile.exists()) {
		switch (QMessageBox::warning(this, "FIR filter", tr(
		"This file already exists: %1.\n").arg(FName), "Overwrite", "Append",
		"Cancel", 0, 2)) {
			case 0: // overwrite
				dataFile.remove();
				if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly)) return false;
				break;
	
			case 1: // append
				if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly | QIODevice::Append)) return false;
				break;
		
			case 2: // cancel
				return false;
				break;
		}
	} else {
		if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly)) return false;
	}
	stream.setDevice(&dataFile);
	//	stream.setPrintableData(false); // write binary
	return true;
}

//create the GUI components
void FIRwindow::customizeGUI(void) {
	QGridLayout *customlayout = DefaultGUIModel::getLayout();
	QGroupBox *box = new QGroupBox;
	QVBoxLayout *boxLayout = new QVBoxLayout;
	box->setLayout(boxLayout);
	QPushButton *saveDataButton = new QPushButton("Save FIR Parameters");
	boxLayout->addWidget(saveDataButton);
	QObject::connect(saveDataButton, SIGNAL(clicked()), this, SLOT(saveFIRData()));
	saveDataButton->setToolTip("Save filter parameters and coefficients to a file");
	
	QWidget *optionBox = new QWidget;
	QGridLayout *optionBoxLayout = new QGridLayout;
	optionBox->setLayout(optionBoxLayout);
	boxLayout->addWidget(optionBox);
	
	QLabel *windowLabel = new QLabel("Window Shape:");
	windowShape = new QComboBox;
	windowShape->insertItem(1, "Rectangular");
	windowShape->insertItem(2, "Triangular (Bartlett)");
	windowShape->insertItem(3, "Hamming");
	windowShape->insertItem(4, "Hann");
	windowShape->insertItem(5, "Chebyshev");
	windowShape->insertItem(6, "Kaiser");
	windowShape->setToolTip("Choose a window to apply. For no window, choose Rectangular.");
	optionBoxLayout->addWidget(windowLabel, 0, 0);
	optionBoxLayout->addWidget(windowShape, 0, 1);
	QObject::connect(windowShape,SIGNAL(activated(int)), this, SLOT(updateWindow(int)));
	
	QLabel *filterLabel = new QLabel("Type of Filter:");
	filterType = new QComboBox;
	filterType->setToolTip("A Type 1 FIR filter.");
	filterType->insertItem(1, "Lowpass");
	filterType->insertItem(2, "Highpass");
	filterType->insertItem(3, "Bandpass");
	filterType->insertItem(4, "Bandstop");
	optionBoxLayout->addWidget(filterLabel, 1, 0);
	optionBoxLayout->addWidget(filterType, 1, 1);
	QObject::connect(filterType,SIGNAL(activated(int)), this, SLOT(updateFilterType(int)));
	
	customlayout->addWidget(box, 0, 0);
	setLayout(customlayout);

	QObject::connect(DefaultGUIModel::pauseButton, SIGNAL(toggled(bool)), saveDataButton, SLOT(setEnabled(bool)));
	QObject::connect(DefaultGUIModel::pauseButton, SIGNAL(toggled(bool)), modifyButton, SLOT(setEnabled(bool)));
	DefaultGUIModel::pauseButton->setToolTip("Start/Stop filter");
	DefaultGUIModel::modifyButton->setToolTip("Commit changes to parameter values");
	DefaultGUIModel::unloadButton->setToolTip("Close plug-in");
}


/*
Overloaded the doSave function to save the window_shape and filter_type
*/
void FIRwindow::doSave(Settings::Object::State &s) const {
	s.saveInteger("paused", pauseButton->isChecked());
	if (isMaximized()) s.saveInteger("Maximized", 1);
	else if (isMinimized()) s.saveInteger("Minimized", 1);

	QPoint pos = parentWidget()->pos();
	s.saveInteger("X", pos.x());
	s.saveInteger("Y", pos.y());
	s.saveInteger("W", width());
	s.saveInteger("H", height());

	s.saveInteger("Window Shape", window_shape); //Save window shape
	s.saveInteger("Filter Type", filter_type); //Save filter type

	for (std::map<QString, param_t>::const_iterator i = parameter.begin(); i != parameter.end(); ++i) {
		s.saveString((i->first).toStdString(), (i->second.edit->text()).toStdString());
	}
}

/*
Overloaded the doLoad function to load the window/filter settings saved in doSave
*/
void FIRwindow::doLoad(const Settings::Object::State &s) {
	for (std::map<QString, param_t>::iterator i = parameter.begin(); i != parameter.end(); ++i)
		i->second.edit->setText(QString::fromStdString(s.loadString((i->first).toStdString())));

	if (s.loadInteger("Maximized")) showMaximized();
	else if (s.loadInteger("Minimized")) showMinimized();

	// this only exists in RTXI versions >1.3
	if (s.loadInteger("W") != NULL) {
		resize(s.loadInteger("W"), s.loadInteger("H"));
		parentWidget()->move(s.loadInteger("X"), s.loadInteger("Y"));
	}

	FIRwindow::window_shape = window_t(s.loadInteger("Window Shape")); //Load window_shape
	FIRwindow::filter_type = filter_t(s.loadInteger("Filter Type")); //Load filter_type
	FIRwindow::windowShape->setCurrentIndex(window_shape); //sets GUI to display current window_type
	FIRwindow::filterType->setCurrentIndex(filter_type); //same, but for filter_type

	pauseButton->setChecked(s.loadInteger("paused"));
	modify();
}
