#include "include/buried.h"

#include <iostream>

#include "buried_core.h"

extern "C" {

Buried* Buried_Create(const char* work_dir) {
  if (!work_dir) {
    return nullptr;
  }
  return new Buried(work_dir);
}

void Buried_Destroy(Buried* buried) {
  if (buried) {
    delete buried;
  }
}

int32_t Buried_Start(Buried* buried, BuriedConfig* config) {
  if (!buried || !config) {
    return BuriedResult::kBuriedInvalidParam;
  }
  Buried::Config buried_config;
  if (config->host) {
    buried_config.host = config->host;
  }
  if (config->port) {
    buried_config.port = config->port;
  }
  if (config->topic) {
    buried_config.topic = config->topic;
  }
  if (config->user_id) {
    buried_config.user_id = config->user_id;
  }
  if (config->app_version) {
    buried_config.app_version = config->app_version;
  }
  if (config->app_name) {
    buried_config.app_name = config->app_name;
  }
  if (config->custom_data) {
    buried_config.custom_data = config->custom_data;
  }
  return buried->Start(buried_config);
}

int32_t Buried_Report(Buried* buried, const char* title, const char* data,
                      uint32_t priority) {
  if (!buried || !title || !data) {
    return BuriedResult::kBuriedInvalidParam;
  }
  return buried->Report(title, data, priority);
}
}