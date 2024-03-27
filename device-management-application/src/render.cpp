/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#include "render.hpp"
std::vector<Color> colors{ftxui::Color::Red,          ftxui::Color::Green,       ftxui::Color::Yellow,
                          ftxui::Color::Blue,         ftxui::Color::Magenta,     ftxui::Color::Cyan,
                          ftxui::Color::GrayLight,    ftxui::Color::GrayDark,    ftxui::Color::RedLight,
                          ftxui::Color::GreenLight,   ftxui::Color::YellowLight, ftxui::Color::BlueLight,
                          ftxui::Color::MagentaLight, ftxui::Color::CyanLight,   ftxui::Color::White};

// Power
std::shared_ptr<PerfMeasure> pwrCard;
std::shared_ptr<PerfMeasure> pwrMinion;
std::shared_ptr<PerfMeasure> pwrSram;
std::shared_ptr<PerfMeasure> pwrNoc;
std::vector<std::shared_ptr<PerfMeasure>> powerItems;

// DDR BW
std::shared_ptr<PerfMeasure> ddrR;
std::shared_ptr<PerfMeasure> ddrW;
std::vector<std::shared_ptr<PerfMeasure>> ddrItems;

// SC BW
std::shared_ptr<PerfMeasure> scbR;
std::shared_ptr<PerfMeasure> scbW;
std::vector<std::shared_ptr<PerfMeasure>> scItems;

// PCI BW
std::shared_ptr<PerfMeasure> pciR;
std::shared_ptr<PerfMeasure> pciW;
std::vector<std::shared_ptr<PerfMeasure>> pciItems;

// Throughput
std::shared_ptr<PerfMeasure> tpt;
std::vector<std::shared_ptr<PerfMeasure>> throughputItems;

// Utilization
std::shared_ptr<PerfMeasure> uMinion;
std::shared_ptr<PerfMeasure> uDmaR;
std::shared_ptr<PerfMeasure> uDmaW;
std::vector<std::shared_ptr<PerfMeasure>> utilizationItems;

// Temperature
std::shared_ptr<PerfMeasure> tempMinion;
std::vector<std::shared_ptr<PerfMeasure>> tempItems;

// Frequency
std::shared_ptr<PerfMeasure> freqMinion;
std::shared_ptr<PerfMeasure> freqNoc;
std::shared_ptr<PerfMeasure> freqDdr;
std::shared_ptr<PerfMeasure> freqIoShire;
std::shared_ptr<PerfMeasure> freqMemShire;
std::shared_ptr<PerfMeasure> freqPcieShire;
std::vector<std::shared_ptr<PerfMeasure>> freqItems;

// Voltages
std::shared_ptr<PerfMeasure> voltMaxion;
std::shared_ptr<PerfMeasure> voltMinion;
std::shared_ptr<PerfMeasure> voltNoc;
std::shared_ptr<PerfMeasure> voltL2Cache;
std::shared_ptr<PerfMeasure> voltPShire;
std::shared_ptr<PerfMeasure> voltIoShire;
std::shared_ptr<PerfMeasure> voltVddq;
std::shared_ptr<PerfMeasure> voltVddqlp;
std::shared_ptr<PerfMeasure> voltDdr;
std::vector<std::shared_ptr<PerfMeasure>> voltItems;

/**
 * @brief Adjusts a value for graphical representation.
 * Scales the input value based on graph height and given range limits.
 * @param value Original value to be adjusted.
 * @param l Lower limit of the value range.
 * @param h Upper limit of the value range.
 * @param height Height of the graph.
 * @return Adjusted value for graph.
 */
float adjustValue(float value, float l, float h, float height) {
  if (h - l == 0) {
    return 0; // handling divide by 0 exception
  }
  float normalizedValue = (value - l) / (h - l);
  float adjustedValue = height - normalizedValue * (height);
  return (adjustedValue);
}

/**
 * @brief Retrieves half of the screen's height.
 * This function calculates and returns the height value corresponding
 * to half of the current screen's total height.
 * @return Half of the screen's height.
 */
int getHalfScreenHeight() {
  int height = -1;
  FILE* fp = popen("tput lines", "r");
  if (fp == NULL) {
    return height;
  }
  if (fscanf(fp, "%d", &height) != 1) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  // Performing linear regression in order to scale the value to represent
  // actual half screen height
  height = HS_HEIGHT_SLOPE * height + HS_HEIGHT_Y_INTERCEPT;
  return height;
}

/**
 * @brief Retrieves the screen's height.
 * This function calculates and returns the height value corresponding
 * to the current screen's total height.
 * @return The screen's height.
 */
int getFullScreenHeight() {
  int height = -1;
  FILE* fp = popen("tput lines", "r");
  if (fp == NULL) {
    return height;
  }
  if (fscanf(fp, "%d", &height) != 1) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  // Performing linear regression in order to scale the value to represent
  // actual full screen height
  height = FS_HEIGHT_SLOPE * height + FS_HEIGHT_Y_INTERCEPT;
  return height;
}

/**
 * @brief Retrieves the screen's width.
 * This function calculates and returns the width value corresponding
 * to the current screen's total width
 * @return The screen's width.
 */
int getScreenWidth() {
  int width = -1;
  FILE* fp = popen("tput cols", "r");
  if (fp == NULL) {
    return width;
  }
  if (fscanf(fp, "%d", &width) != 1) {
    pclose(fp);
    return -1;
  }
  pclose(fp);
  // Performing linear regression in order to scale the value to represent
  // actual full screen WIDTH
  width = FS_WIDTH_SLOPE * width + FS_WIDTH_Y_INTERCEPT;
  return width;
}

PerfMeasure::PerfMeasure(std::string name) {
  this->name = name;
}

std::string PerfMeasure::getName() {
  return name;
}

std::deque<int> PerfMeasure::getData() {
  return data;
}

/**
 * @brief Fetches performance measure data for a moving graph effect.
 * Uses a queue to create the moving effect, then copies its contents
 * into a vector for the given dimensions and value.
 * @param width Width of the display area.
 * @param height Height of the display area.
 * @param v Value to be plotted.
 * @return Vector containing the performance measure data.
 */
void PerfMeasure::setData(int width, int v) {
  size_t maxQueueSize = width;
  while (data.size() >= maxQueueSize) {
    data.pop_front();
  }
  data.push_back(v);
}

/**
 * @brief Initializes performance measure objects for graph representation.
 *
 * This function sets up various performance measure objects
 * that are essential for subsequent graph plotting and representation.
 */
void graph_init() {
  // Power
  pwrCard = std::make_shared<PerfMeasure>("CARD");
  pwrMinion = std::make_shared<PerfMeasure>("MINION");
  pwrSram = std::make_shared<PerfMeasure>("SRAM");
  pwrNoc = std::make_shared<PerfMeasure>("NOC");
  powerItems = {pwrCard, pwrMinion, pwrSram, pwrNoc};

  // DDR BW
  ddrR = std::make_shared<PerfMeasure>("DDR BW Read");
  ddrW = std::make_shared<PerfMeasure>("DDR BW Write");
  ddrItems = {ddrR, ddrW};

  // SC BW
  scbR = std::make_shared<PerfMeasure>("SC BW Read");
  scbW = std::make_shared<PerfMeasure>("SC BW Write");
  scItems = {scbR, scbW};

  // PCI BW
  pciR = std::make_shared<PerfMeasure>("PCI DMA BW Read");
  pciW = std::make_shared<PerfMeasure>("PCI DMA BW Write");
  pciItems = {pciR, pciW};

  // Throughput
  tpt = std::make_shared<PerfMeasure>("Throughput");
  throughputItems = {tpt};

  // Utilization
  uMinion = std::make_shared<PerfMeasure>("Minion");
  uDmaR = std::make_shared<PerfMeasure>("DMA Read");
  uDmaW = std::make_shared<PerfMeasure>("DMA Write");
  utilizationItems = {uMinion, uDmaR, uDmaW};

  // Temperature
  tempMinion = std::make_shared<PerfMeasure>("TEMP MINION");
  tempItems = {tempMinion};

  // Frequency
  freqMinion = std::make_shared<PerfMeasure>("MINION");
  freqNoc = std::make_shared<PerfMeasure>("NOC");
  freqDdr = std::make_shared<PerfMeasure>("DDR");
  freqIoShire = std::make_shared<PerfMeasure>("IO SHIRE");
  freqMemShire = std::make_shared<PerfMeasure>("MEM SHIRE");
  freqPcieShire = std::make_shared<PerfMeasure>("PCIE SHIRE");
  freqItems = {freqMinion, freqNoc, freqDdr, freqIoShire, freqMemShire, freqPcieShire};

  // Voltages
  voltMaxion = std::make_shared<PerfMeasure>("MAXION");
  voltMinion = std::make_shared<PerfMeasure>("MINION");
  voltNoc = std::make_shared<PerfMeasure>("NOC");
  voltL2Cache = std::make_shared<PerfMeasure>("L2 Cache");
  voltPShire = std::make_shared<PerfMeasure>("PShire");
  voltIoShire = std::make_shared<PerfMeasure>("IOShire");
  voltVddq = std::make_shared<PerfMeasure>("VDDQ");
  voltVddqlp = std::make_shared<PerfMeasure>("VDDQLP");
  voltDdr = std::make_shared<PerfMeasure>("DDR");
  voltItems = {voltMaxion, voltMinion, voltNoc, voltL2Cache, voltPShire, voltIoShire, voltVddq, voltVddqlp, voltDdr};
}

int cWIDTH = DEFAULT_DIM_SIZE;
int cHEIGHT = DEFAULT_DIM_SIZE;

/**
 * @brief Sets up the canvas and draws a graph.
 * Initializes the canvas with the given dimensions and
 * plots the data from the provided performance measure items.
 * @param items Vector of pointers to performance measures.
 * @param low Lower limit of the graph range.
 * @param high Upper limit of the graph range.
 * @param height Height of the drawing area.
 * @param width Width of the drawing area.
 * @return Initialized canvas with the plotted graph.
 */

Canvas setupCanvas(std::vector<std::shared_ptr<PerfMeasure>>& items, int low, int high, int height, int width) {
  auto c = Canvas(cWIDTH, cWIDTH);
  size_t i = 0;
  for (auto& perfMeasure : items) {
    const auto& data = perfMeasure->getData(); // Get the data from PerfMeasure

    // Check if data is not empty and colors[i] is a valid index
    if (!data.empty() && i < colors.size()) {
      for (size_t x = 0; x < data.size() - 1; x++) {
        c.DrawPointLine(x, adjustValue(data[x], low, high, height), x + 1, adjustValue(data[x + 1], low, high, height),
                        colors[i]);
      }
      i++;
    }
  }
  return c;
}

/**
 * @brief Draws the Y-axis box and its contents.
 * Sets up and populates the Y-axis box based on the provided
 * performance measure items.
 * @param items Vector of pointers to performance measure items.
 * @param unit Unit of the performance measurement.
 * @return The constructed Y-axis element.
 */
Element setupYAxis(std::vector<std::shared_ptr<PerfMeasure>>& items, std::string unit) {
  auto vboxElements = std::vector<ftxui::Element>{};
  size_t i = 0;
  vboxElements.push_back(hbox({
    text(unit),
  }));
  vboxElements.push_back(filler() | size(WIDTH, LESS_THAN, 1));
  for (const auto& item : items) {
    const auto& data = item->getData();

    // Check if data is not empty before accessing its size
    if (!data.empty() && i < colors.size()) {
      vboxElements.push_back(hbox({
        text("■") | ftxui::color(colors[i]),
        text(" " + item->getName() + " " + std::to_string(data[data.size() - 1])),
      }));
      vboxElements.push_back(filler() | size(WIDTH, LESS_THAN, 1));
      i++;
    }
  }
  auto yAxis = hbox({
    vbox(vboxElements) | border | flex,
  });
  return yAxis;
}

/**
 * @brief Draws the entire performance measure view.
 * Constructs and displays the full graph using the provided canvas,
 * title, and pre-constructed Y-axis element.
 * @param c Canvas on which the graph will be plotted.
 * @param title Title of the graph.
 * @param yAxis Pre-constructed Y-axis element.
 * @return The complete plotted graph as an element.
 */

Element plotGraph(Canvas& c, std::string title, Element& yAxis) {
  auto canvasModule = vbox({
                        canvas(std::move(c)) | border | flex,
                      }) |
                      flex;
  auto graphModule = vbox({text(title) | hcenter | bold, hbox({yAxis, canvasModule}) | flex});
  return hbox({graphModule | flex}) | flex;
}

/**
 * @brief Populates individual performance metric queues with the most recent values
 * Fills individual queue with performance metric values so that they can be plotted
 * on the graph
 * @param mmStats_ Master minion statistics
 * @param spStats_ Service processor statistics
 * @param freqStats_ frequency related statistics
 * @param moduleVoltStats_ voltage related statistics.
 * @param cWIDTH Canvas on which the graph will be plotted.
 */
void getAllData(const struct mm_stats_t& mmStats_, const struct sp_stats_t& spStats_,
                const device_mgmt_api::asic_frequencies_t& freqStats_,
                const device_mgmt_api::module_voltage_t& moduleVoltStats_, int cWIDTH) {
  struct op_stats_t op = spStats_.op;
  pwrCard->setData(cWIDTH, POWER_10MW_TO_W(op.system.power.avg));
  pwrMinion->setData(cWIDTH, POWER_MW_TO_W(op.minion.power.avg));
  pwrSram->setData(cWIDTH, POWER_MW_TO_W(op.sram.power.avg));
  pwrNoc->setData(cWIDTH, POWER_MW_TO_W(op.noc.power.avg));

  ddrR->setData(cWIDTH / 3, mmStats_.computeResources.ddr_read_bw.avg);
  ddrW->setData(cWIDTH / 3, mmStats_.computeResources.ddr_write_bw.avg);
  scbR->setData(cWIDTH / 3, mmStats_.computeResources.l2_l3_read_bw.avg);
  scbW->setData(cWIDTH / 3, mmStats_.computeResources.l2_l3_write_bw.avg);
  pciR->setData(cWIDTH / 3, mmStats_.computeResources.pcie_dma_read_bw.avg);
  pciW->setData(cWIDTH / 3, mmStats_.computeResources.pcie_dma_write_bw.avg);

  tpt->setData((cWIDTH / 2), mmStats_.computeResources.cm_bw.avg);

  uMinion->setData(cWIDTH / 2, mmStats_.computeResources.cm_utilization.avg);
  uDmaR->setData(cWIDTH / 2, mmStats_.computeResources.pcie_dma_read_utilization.avg);
  uDmaW->setData(cWIDTH / 2, mmStats_.computeResources.pcie_dma_write_utilization.avg);

  tempMinion->setData(cWIDTH, spStats_.op.minion.temperature.avg);

  freqMinion->setData(cWIDTH, freqStats_.minion_shire_mhz);
  freqNoc->setData(cWIDTH, freqStats_.noc_mhz);
  freqDdr->setData(cWIDTH, freqStats_.ddr_mhz);
  freqIoShire->setData(cWIDTH, freqStats_.io_shire_mhz);
  freqMemShire->setData(cWIDTH, freqStats_.mem_shire_mhz);
  freqPcieShire->setData(cWIDTH, freqStats_.pcie_shire_mhz);

  voltMaxion->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.maxion, 250, 5, 1));
  voltMinion->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.minion, 250, 5, 1));
  voltNoc->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.noc, 250, 5, 1));
  voltL2Cache->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.l2_cache, 250, 5, 1));
  voltPShire->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.pcie_logic, 250, 5, 1));
  voltIoShire->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.pcie_logic, 250, 5, 1));
  voltVddq->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.vddq, 250, 5, 1));
  voltVddqlp->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.vddqlp, 250, 5, 1));
  voltDdr->setData(cWIDTH, BIN2VOLTAGE(moduleVoltStats_.ddr, 250, 5, 1));
}

/**
 * @brief Plots the power measurements on a line graph.
 * Renders and displays the transition in power comsumption by various components
 * overtime
 * @return The rendered powerView graph as a component.
 */
Component powerViewRenderer() {
  auto powerView = Renderer([&] {
    cWIDTH = getScreenWidth();
    cHEIGHT = getHalfScreenHeight();
    if (cWIDTH == -1 || cHEIGHT == -1) {
      return hbox();
    }
    auto c = setupCanvas(powerItems, CANVAS_POWER_MIN, CANVAS_POWER_MAX, cHEIGHT, cWIDTH);
    auto yAxis = setupYAxis(powerItems, "Watts:");
    return plotGraph(c, "Power Measurements", yAxis);
  });
  return powerView;
}

/**
 * @brief Plots the ddr BW measurements on a line graph.
 * Renders and displays the transition in computation measurements
 * for various components overtime
 * @return The rendered ddrView graph as a component.
 */
Component ddrViewRenderer() {
  auto ddrView = Renderer([&] {
    auto c = setupCanvas(ddrItems, CANVAS_DDR_BW_MIN, CANVAS_DDR_BW_MAX, cHEIGHT, (cWIDTH / 3));
    auto yAxis = setupYAxis(ddrItems, "MB/sec:");
    return plotGraph(c, "DDR BW Measurements", yAxis);
  });
  return ddrView;
}

/**
 * @brief Plots the sc BW measurements on a line graph.
 * Renders and displays the transition in computation measurements
 * for various components overtime
 * @return The rendered scView graph as a component.
 */
Component scViewRenderer() {
  auto scView = Renderer([&] {
    auto c = setupCanvas(scItems, CANVAS_SC_BW_MIN, CANVAS_SC_BW_MAX, cHEIGHT, (cWIDTH / 3));
    auto yAxis = setupYAxis(scItems, "MB/sec:");
    return plotGraph(c, "SC BW Measurements", yAxis);
  });
  return scView;
}

/**
 * @brief Plots the pci BW measurements on a line graph.
 * Renders and displays the transition in computation measurements
 * for various components overtime
 * @return The rendered pciView graph as a component.
 */
Component pciViewRenderer() {
  auto pciView = Renderer([&] {
    auto c = setupCanvas(pciItems, CANVAS_PCI_BW_MIN, CANVAS_PCI_BW_MAX, cHEIGHT, (cWIDTH / 3));
    auto yAxis = setupYAxis(pciItems, "MB/sec:");
    return plotGraph(c, "PCI BW Measurements", yAxis);
  });
  return pciView;
}

/**
 * @brief Plots the throughput measurements on a line graph
 * Renders and displays the transition in throughput values
 * by various components overtime
 * @return The rendered throughputView graph as a component.
 */
Component throughputViewRenderer() {
  auto throughputView = Renderer([&] {
    auto c = setupCanvas(throughputItems, CANVAS_THROUGHPUT_MIN, CANVAS_THROUGHPUT_MAX, cHEIGHT, (cWIDTH / 2));
    auto yAxis = setupYAxis(throughputItems, "Kernel/sec:");
    return plotGraph(c, "Throughput Measurements", yAxis);
  });
  return throughputView;
}

/**
 * @brief Plots the utilization of various compute elements on a line graph
 * Renders and displays the transition in utlization percentage for different compute
 * elements overtime
 * @return The rendered utilizationView graph as a component.
 */
Component utilizationViewRenderer() {
  auto utilizationView = Renderer([&] {
    auto c = setupCanvas(utilizationItems, CANVAS_UTILIZATION_MIN, CANVAS_UTILIZATION_MAX, cHEIGHT, (cWIDTH / 2));
    auto yAxis = setupYAxis(utilizationItems, "Percent (%):");
    return plotGraph(c, "Utilization Measurements", yAxis);
  });
  return utilizationView;
}

/**
 * @brief Plots the temperature measurements on a line graph
 * Renders and displays the transition in temperature values overtime
 * @return The rendered tempView graph as a component.
 */
Component tempViewRenderer() {
  auto tempView = Renderer([&] {
    auto c = setupCanvas(tempItems, CANVAS_TEMPERATURE_MIN, CANVAS_TEMPERATURE_MAX, cHEIGHT, cWIDTH);
    auto yAxis = setupYAxis(tempItems, "Celsius:");
    return plotGraph(c, "Temperature Measurements", yAxis);
  });
  return tempView;
}

/**
 * @brief Plots the frequency measurements on a line graph
 * Renders and displays the transition in frequecy value for different
 * elements overtime
 * @return The rendered freqView graph as a component.
 */
Component freqViewRenderer() {
  auto freqView = Renderer([&] {
    cWIDTH = getScreenWidth();
    cHEIGHT = getHalfScreenHeight();
    if (cWIDTH == -1 || cHEIGHT == -1) {
      return hbox();
    }
    auto c = setupCanvas(freqItems, CANVAS_FREQUENCY_MIN, CANVAS_FREQUENCY_MAX, cHEIGHT, cWIDTH);
    auto yAxis = setupYAxis(freqItems, "MHz:");
    return plotGraph(c, "Frequency Measurements", yAxis);
  });
  return freqView;
}

/**
 * @brief Plots the voltage measurements on a line graph
 * Renders and displays the transition in voltage value for different
 * elements overtime
 * @return The rendered voltView graph as a component.
 */
Component voltViewRenderer() {
  auto voltView = Renderer([&] {
    auto c = setupCanvas(voltItems, CANVAS_VOLTAGE_MIN, CANVAS_VOLTAGE_MAX, cHEIGHT, cWIDTH);
    auto yAxis = setupYAxis(freqItems, "mV:");
    return plotGraph(c, "Voltage Measurements", yAxis);
  });
  return voltView;
}

Component exitComponent(ScreenInteractive& screen) {
  return Make<ExitComponent>(screen);
}

/**
 * @brief Signal handler for window resize events.
 * This function is invoked in response to the SIGWINCH signal,
 * which is sent to processes running in a terminal when the terminal's size changes.
 * It introduces a short delay after detecting the resize event.
 * This is done to pause the process while the window is getting resized
 */
void handleResize(int sig) {
  sleep(0.3);
}

/**
 * @brief Function to represent the various performance metric graph.
 * * This function fetches values for various performance metrics and plots them
 * as a line graph. It generates various views which are displayed according to the
 * tab selected
 * @param powerView Component object used to plot power related metrics
 * @param ddrView Component object used to plot DDR BW related metrics
 * @param scView Component object used to plot SC BW related metrics
 * @param pciView Component object used to plot PCI BW related metrics
 * @param tempView Component object used to plot temp related metrics
 * @param freqView Component object used to plot freq related metrics
 * @param voltView Component object used to plot volt related metrics
 * @param utilizationView Component object used to plot utlization related metrics
 * @param throughputView Component object used to plot throughput related metrics
 * @param etTop EtTop object to call EtTop member functions
 */
void renderMainDisplay(Component powerView, Component ddrView, Component scView, Component pciView, Component tempView,
                       Component freqView, Component voltView, Component utilizationView, Component throughputView,
                       EtTop* etTop) {
  auto screen = ScreenInteractive::Fullscreen();
  int shift = 0;

  int tabIndex = 0;
  std::vector<std::string> tabEntries = {"Power/Temp View", "Compute/Throughput/Utilization View", "Freq/Volt View",
                                         "Exit"};
  auto tabSelection = Menu(&tabEntries, &tabIndex, MenuOption::HorizontalAnimated());

  auto powerTempView = Container::Vertical({powerView, tempView});
  auto freqVoltView = Container::Vertical({freqView, voltView});

  auto throughputUtilizationView = Container::Horizontal({throughputView, utilizationView}) | flex;
  auto bandwithUtilizationView = Container::Horizontal({ddrView, scView, pciView}) | flex;
  auto throughputUtilizationComputeView = Container::Vertical({bandwithUtilizationView, throughputUtilizationView});
  auto exitView = exitComponent(screen);

  auto tabContent = Container::Tab(
    {
      powerTempView,
      throughputUtilizationComputeView,
      freqVoltView,
      exitView,
    },
    &tabIndex);

  // Main Screen
  auto mainContainer = Container::Vertical({
    tabSelection,
    tabContent,
  });

  auto mainRenderer = Renderer(mainContainer, [&] {
    signal(SIGWINCH, handleResize);

    return vbox({
      text("et-powertop - Graphical View") | bold | hcenter,
      tabSelection->Render(),
      tabContent->Render() | flex,
    });
  });

  // TODO:Spin new threads
  std::atomic<bool> refreshUiContinue = true;
  std::thread refreshUi([&] {
    while (refreshUiContinue) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(0.03s);
      etTop->collectStats();
      getAllData(etTop->getMmStats(), etTop->getSpStats(), etTop->getFreqStats(), etTop->getModuleVoltStats(), cWIDTH);
      // The |shift| variable belong to the main thread. `screen.Post(task)`
      // will execute the update on the thread where |screen| lives (e.g. the
      // main thread). Using `screen.Post(task)` is threadsafe.
      screen.Post([&] { shift++; });
      // After updating the state, request a new frame to be drawn. This is done
      // by simulating a new "custom" event to be handled.
      screen.Post(Event::Custom);
    }
  });
  screen.Loop(mainRenderer);
  refreshUiContinue = false;
  refreshUi.join();
}
