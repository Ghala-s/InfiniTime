#include "displayapp/screens/HeartRate.h"
#include <lvgl/lvgl.h>
#include <components/heartrate/HeartRateController.h>

#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"

using namespace Pinetime::Applications::Screens;

namespace {
  const char* ToString(Pinetime::Controllers::HeartRateController::States s) {
    switch (s) {
      case Pinetime::Controllers::HeartRateController::States::NotEnoughData:
        return "Not enough data,\nplease wait...";
      case Pinetime::Controllers::HeartRateController::States::NoTouch:
        return "No touch detected";
      case Pinetime::Controllers::HeartRateController::States::Running:
        return "Measuring...";
      case Pinetime::Controllers::HeartRateController::States::Stopped:
        return "Stopped";
    }
    return "";
  }

  void btnStartStopEventHandler(lv_obj_t* obj, lv_event_t event) {
    auto* screen = static_cast<HeartRate*>(obj->user_data);
    screen->OnStartStopEvent(event);
  }
}

HeartRate::HeartRate(Controllers::HeartRateController& heartRateController, Controllers::MotionController& motionController, System::SystemTask& systemTask)
  : heartRateController {heartRateController}, motionController {motionController}, wakeLock(systemTask) {

  bool isHrRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  label_hr = lv_label_create(lv_scr_act(), nullptr);

  lv_obj_set_style_local_text_font(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);

  if (isHrRunning) {
    lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::highlight);
  } else {
    lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
  }

  lv_label_set_text_static(label_hr, "---");
  lv_obj_align(label_hr, nullptr, LV_ALIGN_CENTER, 0, -40);

  label_bpm = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(label_bpm, "Heart rate BPM");
  lv_obj_align(label_bpm, label_hr, LV_ALIGN_OUT_TOP_MID, 0, -20);

  label_status = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_status, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
  lv_label_set_text_static(label_status, ToString(Pinetime::Controllers::HeartRateController::States::NotEnoughData));

  lv_obj_align(label_status, label_hr, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

  btn_startStop = lv_btn_create(lv_scr_act(), nullptr);
  btn_startStop->user_data = this;
  lv_obj_set_height(btn_startStop, 50);
  lv_obj_set_event_cb(btn_startStop, btnStartStopEventHandler);
  lv_obj_align(btn_startStop, nullptr, LV_ALIGN_IN_BOTTOM_MID, 0, 0);

  label_startStop = lv_label_create(btn_startStop, nullptr);
  UpdateStartStopButton(isHrRunning);
  if (isHrRunning) {
    wakeLock.Lock();
  }

  taskRefresh = lv_task_create(RefreshTaskCallback, 100, LV_TASK_PRIO_MID, this);
}

HeartRate::~HeartRate() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}
void HeartRate::Refresh() {
  float pitch = motionController.GetPitch();
  float roll = motionController.GetRoll();

  // Display orientation instead of heart rate
  lv_label_set_text_fmt(label_hr, "Pitch: %.1f°\nRoll: %.1f°", pitch, roll);
  lv_label_set_text_static(label_status, "Orientation");
  lv_obj_align(label_status, label_hr, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

// delete the health rate
/**
void HeartRate::Refresh() {

  auto state = heartRateController.State();
  switch (state) {
    case Controllers::HeartRateController::States::NoTouch:
    case Controllers::HeartRateController::States::NotEnoughData:
      // case Controllers::HeartRateController::States::Stopped:
      lv_label_set_text_static(label_hr, "---");
      break;
    default:
      if (heartRateController.HeartRate() == 0) {
        lv_label_set_text_static(label_hr, "---");
      } else {
        lv_label_set_text_fmt(label_hr, "%03d", heartRateController.HeartRate());
      }
  }

  lv_label_set_text_static(label_status, ToString(state));
  lv_obj_align(label_status, label_hr, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}
*/
void HeartRate::OnStartStopEvent(lv_event_t event) {
  if (event == LV_EVENT_CLICKED) {
    if (heartRateController.State() == Controllers::HeartRateController::States::Stopped) {
      heartRateController.Start();
      UpdateStartStopButton(heartRateController.State() != Controllers::HeartRateController::States::Stopped);
      wakeLock.Lock();
      lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::highlight);
    } else {
      heartRateController.Stop();
      UpdateStartStopButton(heartRateController.State() != Controllers::HeartRateController::States::Stopped);
      wakeLock.Release();
      lv_obj_set_style_local_text_color(label_hr, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    }
  }
}

void HeartRate::UpdateStartStopButton(bool isRunning) {
  if (isRunning) {
    lv_label_set_text_static(label_startStop, "Stop");
  } else {
    lv_label_set_text_static(label_startStop, "Start");
  }
}
