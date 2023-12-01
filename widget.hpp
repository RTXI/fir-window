
#include <QComboBox>
#include <QFile>
#include <QTextStream>

#include <boost/circular_buffer.hpp>
#include <rtxi/dsp/dolph.h>
#include <rtxi/dsp/fir_dsgn.h>
#include <rtxi/dsp/firideal.h>
#include <rtxi/dsp/gen_win.h>
#include <rtxi/dsp/hamming.h>
#include <rtxi/dsp/hann.h>
#include <rtxi/dsp/kaiser.h>
#include <rtxi/dsp/lin_dsgn.h>
#include <rtxi/dsp/rectnglr.h>
#include <rtxi/dsp/trianglr.h>
#include <rtxi/widgets.hpp>

// This is an generated header file. You may change the namespace, but
// make sure to do the same in implementation (.cpp) file
namespace fir_window
{

constexpr std::string_view MODULE_NAME = "fir-window";

enum window_t : int64_t
{
  RECT = 0,
  TRI,
  HAMM,
  HANN,
  CHEBY,
  KAISER
};

enum filter_t : int64_t
{
  LOWPASS = 0,
  HIGHPASS,
  BANDPASS,
  BANDSTOP
};

enum PARAMETER : Widgets::Variable::Id
{
  // set parameter ids here
  WINDOW_TYPE = 0,
  FILTER_TYPE,
  TAPS,
  FREQUENCY_1,
  FREQUENCY_2,
  CHEBYSHEV_ATTENUATION,
  KAISER_ALPHA_ATTENUATION
};

inline std::vector<Widgets::Variable::Info> get_default_vars()
{
  return {
      {PARAMETER::WINDOW_TYPE,
       "Window Type",
       "Type of window to use in the filter",
       Widgets::Variable::INT_PARAMETER,
       fir_window::RECT},
      {PARAMETER::FILTER_TYPE,
       "Filter Type",
       "Type of filter to use",
       Widgets::Variable::INT_PARAMETER,
       fir_window::LOWPASS},
      {PARAMETER::TAPS,
       "# Taps",
       "Number of Filter Taps (Odd Number)",
       Widgets::Variable::INT_PARAMETER,
       int64_t {9}},
      {
          PARAMETER::FREQUENCY_1,
          "Frequency 1 (Hz)",
          "Cut off Frequency #1 as Fraction of Pi, used for Lowpass/Highpass",
          Widgets::Variable::DOUBLE_PARAMETER,
          0.1,
      },
      {
          PARAMETER::FREQUENCY_2,
          "Frequency 2 (Hz)",
          "Cut off Frequency #1 as Fraction of Pi, not used for "
          "Lowpass/Highpass",
          Widgets::Variable::DOUBLE_PARAMETER,
          0.6,
      },
      {
          PARAMETER::CHEBYSHEV_ATTENUATION,
          "Chebyshev (dB)",
          "Attenuation Parameter for Chebyshev Window",
          Widgets::Variable::DOUBLE_PARAMETER,
          70.0,
      },
      {PARAMETER::KAISER_ALPHA_ATTENUATION,
       "Kaiser Alpha",
       "Attenuation Parameter for Kaiser Window",
       Widgets::Variable::DOUBLE_PARAMETER,
       1.5}};
}

inline std::vector<IO::channel_t> get_default_channels()
{
  return {{
              "Input",
              "Input to Filter",
              IO::INPUT,
          },
          {
              "Output",
              "Output of Filter",
              IO::OUTPUT,
          }};
}

class Panel : public Widgets::Panel
{
  Q_OBJECT
public:
  Panel(QMainWindow* main_window, Event::Manager* ev_manager);
  void customizeGUI();

private:
  // FIRwindow functions
  QComboBox* windowShape;
  QComboBox* filterType;

  // Saving FIR filter data to file without Data Recorder
  bool OpenFile(QString);
  QFile dataFile;
  QTextStream stream;

private slots:
  // all custom slots
  void saveFIRData();  // write filter parameters to a file
  void updateWindow(int);
  void updateFilterType(int);

  // Any functions and data related to the GUI are to be placed here
};

class Component : public Widgets::Component
{
public:
  explicit Component(Widgets::Plugin* hplugin);
  void execute() override;

private:
  boost::circular_buffer<double> signalin;
  double* convolution;
  double out;
  double dt;
  int n;

  void initParameters();
  void bookkeep();
  void makeFilter();

  //	double* h2;
  double* h3;  // filter coefficients
  window_t window_shape;
  filter_t filter_type;
  int64_t num_taps;
  double lambda1;  // cutoff frequencies
  double lambda2;
  double Kalpha;  // Kaiser window sidelobe attenuation parameter
  double Calpha;  // Chebyshev window sidelobe attenuation parameter

private:
  GenericWindow* disc_window;
  FirIdealFilter* filter_design;
};

class Plugin : public Widgets::Plugin
{
public:
  explicit Plugin(Event::Manager* ev_manager);
};

}  // namespace fir_window
