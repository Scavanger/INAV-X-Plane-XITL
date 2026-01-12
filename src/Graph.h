#pragma once

#include "platform.h"

static const char* SETTINGS_GRAPH_SECTION = "GraphSettings";
static const char* SETTINGS_GRAPH_TYPE = "settings_graph_type";
typedef enum
{
  GRAPH_UPDATES,
  GRAPH_ATTITUDE_OUTPUT,
  GRAPH_ATTITUDE_ESTIMATION,
  GRAPH_ACC,
  GRAPH_GYRO,
  GRAPH_DEBUG_ALTITUDE,
  GRAPH_DEBUG_CUSTOM
} TGraphType;

static constexpr int GRAPH_POINTS = 200;
static constexpr int GRAPH_COUNT_MAX = 10;


class TGraphSeries
{
  public:

    float points[GRAPH_POINTS];
    float max = 0.0f;
    float min = 0.0;

    bool autoRange = false;

    int color = 0;

    int head = 0;

    char name[128] = "";

    void clear();

    void setRange(float min, float max);
    void setName(const char* pName);

    void addPoint(float value);

    void drawOSD(float bx, float by, float width, float height);
};

class Graph
{
public:

  Graph();

private:
  TGraphSeries series[GRAPH_COUNT_MAX];

  void drawCallback();

  void setGraphType(TGraphType type);
  TGraphType getGraphType();

  void clear();

  void addOutputYPR(float yaw, float pitch, float roll);
  void addAttitudeYPR(float yaw, float pitch, float roll);
  void addACC(float x, float y, float z );
  void addGyro(float x, float y, float z);
  void addEstimatedAttitudeYPR(float yaw, float pitch, float roll);
  void addUpdatePeriodMS(uint32_t period);

  void addDebug(int index, float value);

  bool isActive = false;

  TGraphType graph_type = GRAPH_ACC;

  int activeCount;
  const char* pSeriesName;
  size_t lastLen;

  float debug[8];

  uint32_t lastUpdatesCountTime;
  int updatesCount;
  int updatesCountValue;

  void formatRangeNumber(char* dest, float value);
  void formatValueNumber(char* dest, float value);
};

