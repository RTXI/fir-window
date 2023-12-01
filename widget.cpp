#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include "widget.hpp"

#include <sys/stat.h>

fir_window::Plugin::Plugin(Event::Manager* ev_manager)
    : Widgets::Plugin(ev_manager, std::string(fir_window::MODULE_NAME))
{
}

fir_window::Panel::Panel(QMainWindow* main_window, Event::Manager* ev_manager)
    : Widgets::Panel(
        std::string(fir_window::MODULE_NAME), main_window, ev_manager)
{
  setWhatsThis(
      "<p><b>FIR Window:</b><br>This plugin computes FIR filter coefficients "
      "using the window method "
      "given the number of taps desired and the cutoff frequencies. For a "
      "lowpass or highpass filter, use the"
      "Freq 1 parameter. For a bandpass or bandstop filter, use both "
      "frequencies to define the frequency band."
      "Since this plug-in computes new filter coefficients whenever you change "
      "the parameters, you should not"
      "change any settings during real-time.</p>");
  createGUI(fir_window::get_default_vars(),
            {fir_window::WINDOW_TYPE,
             fir_window::FILTER_TYPE});  // this is required to create the GUI
  customizeGUI();
  QTimer::singleShot(0, this, SLOT(resizeMe()));
}

fir_window::Component::Component(Widgets::Plugin* hplugin)
    : Widgets::Component(hplugin,
                         std::string(fir_window::MODULE_NAME),
                         fir_window::get_default_channels(),
                         fir_window::get_default_vars())
{
}

void fir_window::Component::execute()
{
  // This is the real-time function that will be called
  switch (this->getState()) {
    case RT::State::EXEC:
      signalin.push_back(readinput(0));
      out = 0;
      for (auto n = num_taps; n < 2 * num_taps; n++)
        out += h3[n] * signalin[n];
      writeoutput(0, out);
      break;
    case RT::State::INIT:
      initParameters();
      setState(RT::State::EXEC);
      break;
    case RT::State::MODIFY:
      num_taps = getValue<int64_t>(PARAMETER::TAPS);
      if (num_taps % 2 == 0) {
        num_taps = num_taps + 1;
      }

      lambda1 = getValue<double>(PARAMETER::FREQUENCY_1);
      lambda2 = getValue<double>(PARAMETER::FREQUENCY_2);
      Kalpha = getValue<double>(PARAMETER::KAISER_ALPHA_ATTENUATION);
      Calpha = getValue<double>(PARAMETER::CHEBYSHEV_ATTENUATION);
      window_shape =
          static_cast<window_t>(getValue<int64_t>(PARAMETER::WINDOW_TYPE));
      filter_type =
          static_cast<filter_t>(getValue<int64_t>(PARAMETER::FILTER_TYPE));
      bookkeep();
      makeFilter();
      setState(RT::State::EXEC);
      break;
    case RT::State::PAUSE:
      writeoutput(0, 0);
      break;
    case RT::State::UNPAUSE:
      bookkeep();
      setState(RT::State::EXEC);
      break;
    case RT::State::PERIOD:
      dt = RT::OS::getPeriod() * 1e-9;
      setState(RT::State::EXEC);
      break;
    default:
      break;
  }
}

void fir_window::Component::initParameters()
{
  dt = RT::OS::getPeriod() * 1e-9;  // s
  signalin.clear();
  assert(signalin.empty());
  window_shape = HAMM;
  filter_type = BANDPASS;
  num_taps = 9;
  lambda1 = 0.1;  // Hz / pi
  lambda2 = 0.6;  // Hz / pi
  Calpha = 70;  // dB
  Kalpha = 1.5;
  makeFilter();
  bookkeep();
}

void fir_window::Component::bookkeep()
{
  signalin.set_capacity(2 * num_taps);
  convolution = new double[num_taps];
  for (int i = 0; i < 2 * num_taps; i++)
    signalin.push_back(0);  // pad with zeros

  assert(signalin.full());  // check size and padding
  assert(signalin.size() == 2 * num_taps);
  assert(signalin.capacity() == 2 * num_taps);
}

void fir_window::Panel::updateWindow(int index)
{
  if (index < 0) {
    return;
  }
  Widgets::Plugin* hplugin = getHostPlugin();
  hplugin->setComponentParameter(fir_window::WINDOW_TYPE,
                                 static_cast<int64_t>(index));
}

void fir_window::Panel::updateFilterType(int index)
{
  if (index < 0) {
    return;
  }
  Widgets::Plugin* hplugin = getHostPlugin();
  hplugin->setComponentParameter(fir_window::FILTER_TYPE,
                                 static_cast<int64_t>(index));
}

void fir_window::Component::makeFilter()
{
  switch (window_shape) {
    case RECT:  // rectangular
      disc_window = new RectangularWindow(num_taps);
      break;

    case TRI:  // triangular
      disc_window = new TriangularWindow(num_taps, 1);
      break;

    case HAMM:  // Hamming
      disc_window = new HammingWindow(num_taps);
      break;

    case HANN:  // Hann
      disc_window = new HannWindow(num_taps, 1);
      break;

    case CHEBY:  // Dolph-Chebyshev
      disc_window = new DolphChebyWindow(num_taps, Calpha);
      break;

    case KAISER:
      disc_window = new KaiserWindow(num_taps, Kalpha);
      break;
  }  // end of switch on window_shape

  filter_design = new FirIdealFilter(num_taps, lambda1, lambda2, filter_type);
  //	h2 = filter_design->GetCoefficients();
  filter_design->ApplyWindow(disc_window);
  h3 = filter_design->GetCoefficients();
}

void fir_window::Panel::saveFIRData()
{
  QFileDialog* fd = new QFileDialog(this, "Save File As");  //, TRUE);
  fd->setFileMode(QFileDialog::AnyFile);
  fd->setViewMode(QFileDialog::Detail);
  QString fileName;
  if (fd->exec() == QDialog::Accepted) {
    QStringList files = fd->selectedFiles();
    if (!files.isEmpty())
      fileName = files.takeFirst();

    if (OpenFile(fileName)) {
      Widgets::Plugin* hplugin = getHostPlugin();
      int64_t filter_type =
          hplugin->getComponentIntParameter(PARAMETER::FILTER_TYPE);
      double lambda1 =
          hplugin->getComponentDoubleParameter(PARAMETER::FREQUENCY_1);
      double lambda2 =
          hplugin->getComponentDoubleParameter(PARAMETER::FREQUENCY_2);
      switch (filter_type) {
        case LOWPASS:
          stream << QString("LOWPASS lambda1=") << (double)lambda1;
          break;

        case HIGHPASS:
          stream << QString("HIGHPASS lambda1=") << (double)lambda1;
          break;

        case BANDPASS:
          stream << QString("BANDPASS lambda1=") << (double)lambda1
                 << " lambda2= " << (double)lambda2;
          break;

        case BANDSTOP:
          stream << QString("BANDSTOP lambda1=") << (double)lambda1
                 << " lambda2= " << (double)lambda2;
          break;
      }
      int64_t window_shape =
          hplugin->getComponentIntParameter(PARAMETER::WINDOW_TYPE);
      int64_t num_taps = hplugin->getComponentIntParameter(PARAMETER::TAPS);
      double Calpha = hplugin->getComponentDoubleParameter(
          PARAMETER::CHEBYSHEV_ATTENUATION);
      double Kalpha = hplugin->getComponentDoubleParameter(
          PARAMETER::KAISER_ALPHA_ATTENUATION);
      switch (window_shape) {
        case RECT:  // rectangular
          stream << QString(" RECT taps:") << num_taps << "\n";
          break;

        case TRI:  // triangular
          stream << QString(" TRI taps:") << num_taps << "\n";
          break;

        case HAMM:  // Hamming
          stream << QString(" HAMM taps:") << num_taps << "\n";
          break;

        case HANN:  // Hann
          stream << QString(" HANN taps:") << num_taps << "\n";
          break;

        case CHEBY:  // Dolph-Chebyshev
          stream << QString(" CHEBY taps:") << num_taps << " alpha:" << Calpha
                 << "\n";
          break;

        case KAISER:
          stream << QString(" KAISER taps:") << num_taps << " alpha:" << Kalpha
                 << "\n";
          break;
      }

      dataFile.close();
    } else {
      QMessageBox::information(
          this,
          "FIR filter: Save filter parameters",
          "There was an error writing to this file. You can view\n"
          "the parameters in the terminal.\n");
    }
  }
}

bool fir_window::Panel::OpenFile(QString FName)
{
  dataFile.setFileName(FName);
  if (dataFile.exists()) {
    switch (
        QMessageBox::warning(this,
                             "FIR filter",
                             tr("This file already exists: %1.\n").arg(FName),
                             "Overwrite",
                             "Append",
                             "Cancel",
                             0,
                             2))
    {
      case 0:  // overwrite
        dataFile.remove();
        if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly))
          return false;
        break;

      case 1:  // append
        if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly
                           | QIODevice::Append))
          return false;
        break;

      case 2:  // cancel
        return false;
        break;
    }
  } else {
    if (!dataFile.open(QIODevice::Unbuffered | QIODevice::WriteOnly))
      return false;
  }
  stream.setDevice(&dataFile);
  return true;
}

// create the GUI components
void fir_window::Panel::customizeGUI()
{
  // QGridLayout* customlayout = DefaultGUIModel::getLayout();
  auto* widget_layout = dynamic_cast<QVBoxLayout*>(this->layout());
  QWidget* box = new QWidget;
  QVBoxLayout* boxLayout = new QVBoxLayout;
  box->setLayout(boxLayout);
  QPushButton* saveDataButton = new QPushButton("Save FIR Parameters");
  boxLayout->addWidget(saveDataButton);
  QObject::connect(
      saveDataButton, SIGNAL(clicked()), this, SLOT(saveFIRData()));
  saveDataButton->setToolTip(
      "Save filter parameters and coefficients to a file");

  QWidget* optionBox = new QWidget;
  QGridLayout* optionBoxLayout = new QGridLayout;
  optionBox->setLayout(optionBoxLayout);
  boxLayout->addWidget(optionBox);

  QLabel* windowLabel = new QLabel("Window Shape:");
  windowShape = new QComboBox;
  windowShape->insertItem(1, "Rectangular");
  windowShape->insertItem(2, "Triangular (Bartlett)");
  windowShape->insertItem(3, "Hamming");
  windowShape->insertItem(4, "Hann");
  windowShape->insertItem(5, "Chebyshev");
  windowShape->insertItem(6, "Kaiser");
  windowShape->setToolTip(
      "Choose a window to apply. For no window, choose Rectangular.");
  optionBoxLayout->addWidget(windowLabel, 0, 0);
  optionBoxLayout->addWidget(windowShape, 0, 1);
  QObject::connect(
      windowShape, SIGNAL(activated(int)), this, SLOT(updateWindow(int)));

  QLabel* filterLabel = new QLabel("Type of Filter:");
  filterType = new QComboBox;
  filterType->setToolTip("A Type 1 FIR filter.");
  filterType->insertItem(1, "Lowpass");
  filterType->insertItem(2, "Highpass");
  filterType->insertItem(3, "Bandpass");
  filterType->insertItem(4, "Bandstop");
  optionBoxLayout->addWidget(filterLabel, 1, 0);
  optionBoxLayout->addWidget(filterType, 1, 1);
  QObject::connect(
      filterType, SIGNAL(activated(int)), this, SLOT(updateFilterType(int)));

  widget_layout->insertWidget(0, box);
  setLayout(widget_layout);
}

///////// DO NOT MODIFY BELOW //////////
// The exception is if your plugin is not going to need real-time functionality.
// For this case just replace the craeteRTXIComponent return type to nullptr.
// RTXI will automatically handle that case and won't attach a component to the
// real time thread for your plugin.

std::unique_ptr<Widgets::Plugin> createRTXIPlugin(Event::Manager* ev_manager)
{
  return std::make_unique<fir_window::Plugin>(ev_manager);
}

Widgets::Panel* createRTXIPanel(QMainWindow* main_window,
                                Event::Manager* ev_manager)
{
  return new fir_window::Panel(main_window, ev_manager);
}

std::unique_ptr<Widgets::Component> createRTXIComponent(
    Widgets::Plugin* host_plugin)
{
  return std::make_unique<fir_window::Component>(host_plugin);
}

Widgets::FactoryMethods fact;

extern "C"
{
Widgets::FactoryMethods* getFactories()
{
  fact.createPanel = &createRTXIPanel;
  fact.createComponent = &createRTXIComponent;
  fact.createPlugin = &createRTXIPlugin;
  return &fact;
}
};

//////////// END //////////////////////
