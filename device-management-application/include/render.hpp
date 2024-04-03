/*-------------------------------------------------------------------------
 * Copyright (C) 2023, Esperanto Technologies Inc.
 * The copyright to the computer program(s) herein is the
 * property of Esperanto Technologies, Inc. All Rights Reserved.
 * The program(s) may be used and/or copied only with
 * the written permission of Esperanto Technologies and
 * in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 *-------------------------------------------------------------------------*/
#pragma once
#include "et-top.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/flexbox_config.hpp"
#include "ftxui/screen/color.hpp"
#include "ftxui/screen/color_info.hpp"
#include "ftxui/screen/terminal.hpp"

using namespace ftxui;

/**
 * @brief Constants for screen dimension adjustments.
 *
 * When determining the screen's dimensions, values obtained by querying
 * the system for terminal dimensions might not match the actual screen size.
 * These constants provide scaled values to match the true dimensions.
 *
 * Linear regression is employed for the scaling, which involves calculating
 * the slope and y-intercept. The formulas used are:
 *
 * For the slope m:
 *
 * m = (n*sum(xi*yi) - sum(xi)*sum(yi)) / (n*sum(xi^2) - (sum(xi))^2)
 *
 * For the y-intercept b:
 *
 * b = (sum(yi) - m*sum(xi)) / n
 *
 *
 * Where:
 * - n is the number of data points.
 * - sum(xi) is the sum of all x values.
 * - sum(yi) is the sum of all y values.
 * - sum(xi*yi) is the sum of the products of each x value with its corresponding y value.
 * - sum(xi^2) is the sum of the squares of the x values.
 *
 * These constants represent the slope and y-intercept values for both
 * full and half screens as determined by this approach.
 */

#define HS_HEIGHT_SLOPE 2.032
#define HS_HEIGHT_Y_INTERCEPT -20.99

#define FS_HEIGHT_SLOPE 4.032
#define FS_HEIGHT_Y_INTERCEPT -27.31

#define HS_HEIGHT_SLOPE 2.032
#define HS_HEIGHT_Y_INTERCEPT -20.99

#define FS_WIDTH_SLOPE 2.033
#define FS_WIDTH_Y_INTERCEPT -28.99

#define DEFAULT_DIM_SIZE 300

#define QUEUE_Y_INTERCEPT 3
#define WIDTH_ERROR_CORRECTION_Y_INTERCEPT 10

class PerfMeasure {
private:
  std::string name;
  std::deque<int> data;

public:
  PerfMeasure(std::string name);
  std::string getName();
  void setData(int width, int v);
  std::deque<int> getData();
};

class ExitComponent : public ComponentBase {
public:
  ExitComponent(ScreenInteractive& screen)
    : screen_(screen) {
  }
  Element Render() override {
    screen_.ExitLoopClosure()();
    return text("");
  }

private:
  ScreenInteractive& screen_;
};

float adjustValue(float value, float l, float &h, float height);
Component powerViewRenderer(int maxPower);
Component ddrViewRenderer(int maxDDR);
Component scViewRenderer(int maxSc);
Component pciViewRenderer(int maxPci);
Component utilizationViewRenderer(int maxUtil);
Component throughputViewRenderer(int maxThroughput);
Component tempViewRenderer(int maxTemp);
Component freqViewRenderer(int maxFreq);
Component voltViewRenderer(int maxVolt);
Component exitComponent();
void renderMainDisplay(Component powerView, Component ddrView, Component scView, Component pciView, Component tempView,
                       Component freqView, Component voltView, Component utilizationView, Component throughputView,
                       EtTop* etTop);

void graph_init();
