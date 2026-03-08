#include "tesla_fleet_api_server.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <esp_timer.h>

#include "esphome/core/log.h"

namespace esphome {
namespace tesla_fleet_api_server {

static const char *const TAG = "tesla_fleet_api_server";
static const char *const PROXY_NAME = "esphome-tesla-ble";
static const char *const PROXY_VERSION = "2025.9.1";

void TeslaFleetAPIServer::setup() {
  if (this->server_ != nullptr) {
    return;
  }

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = kServerPort;
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.max_uri_handlers = 32;
  config.max_open_sockets = 3;
  config.lru_purge_enable = true;

  esp_err_t err = httpd_start(&this->server_, &config);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start fleet API server: %s", esp_err_to_name(err));
    this->server_ = nullptr;
    return;
  }

  httpd_uri_t vehicles = {
      .uri = "/api/1/vehicles",
      .method = HTTP_GET,
      .handler = &TeslaFleetAPIServer::handle_get_vehicles,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &vehicles);

  httpd_uri_t vehicle_state = {
      .uri = "/api/1/vehicles/*/vehicle_state",
      .method = HTTP_GET,
      .handler = &TeslaFleetAPIServer::handle_get_vehicle_state,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &vehicle_state);

  httpd_uri_t charge_state = {
      .uri = "/api/1/vehicles/*/charge_state",
      .method = HTTP_GET,
      .handler = &TeslaFleetAPIServer::handle_get_charge_state,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &charge_state);

  httpd_uri_t vehicle_data = {
      .uri = "/api/1/vehicles/*/vehicle_data",
      .method = HTTP_GET,
      .handler = &TeslaFleetAPIServer::handle_get_vehicle_data,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &vehicle_data);

  httpd_uri_t body_controller_state = {
    .uri = "/api/1/vehicles/*/body_controller_state",
    .method = HTTP_GET,
    .handler = &TeslaFleetAPIServer::handle_get_body_controller_state,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &body_controller_state);

  httpd_uri_t charge_start = {
      .uri = "/api/1/vehicles/*/command/charge_start",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_charge_start,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &charge_start);

  httpd_uri_t charge_stop = {
      .uri = "/api/1/vehicles/*/command/charge_stop",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_charge_stop,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &charge_stop);

  httpd_uri_t set_amps = {
      .uri = "/api/1/vehicles/*/command/set_charging_amps",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_set_amps,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &set_amps);

  httpd_uri_t set_charge_limit = {
    .uri = "/api/1/vehicles/*/command/set_charge_limit",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_set_charge_limit,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &set_charge_limit);

  httpd_uri_t climate_start = {
      .uri = "/api/1/vehicles/*/command/auto_conditioning_start",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_climate_start,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &climate_start);

  httpd_uri_t climate_stop = {
      .uri = "/api/1/vehicles/*/command/auto_conditioning_stop",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_climate_stop,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &climate_stop);

  httpd_uri_t set_sentry_mode = {
    .uri = "/api/1/vehicles/*/command/set_sentry_mode",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_set_sentry_mode,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &set_sentry_mode);

  httpd_uri_t wake_up = {
      .uri = "/api/1/vehicles/*/wake_up",
      .method = HTTP_POST,
      .handler = &TeslaFleetAPIServer::handle_post_wake_up,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &wake_up);

  httpd_uri_t wake_up_command = {
    .uri = "/api/1/vehicles/*/command/wake_up",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_wake_up,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &wake_up_command);

  httpd_uri_t charge_port_open = {
    .uri = "/api/1/vehicles/*/command/charge_port_door_open",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_charge_port_open,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &charge_port_open);

  httpd_uri_t charge_port_close = {
    .uri = "/api/1/vehicles/*/command/charge_port_door_close",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_charge_port_close,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &charge_port_close);

  httpd_uri_t flash_lights = {
    .uri = "/api/1/vehicles/*/command/flash_lights",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_flash_lights,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &flash_lights);

  httpd_uri_t honk_horn = {
    .uri = "/api/1/vehicles/*/command/honk_horn",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_honk_horn,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &honk_horn);

  httpd_uri_t door_lock = {
    .uri = "/api/1/vehicles/*/command/door_lock",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_door_lock,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &door_lock);

  httpd_uri_t door_unlock = {
    .uri = "/api/1/vehicles/*/command/door_unlock",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_door_unlock,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &door_unlock);

  httpd_uri_t vehicle = {
      .uri = "/api/1/vehicles/*",
      .method = HTTP_GET,
      .handler = &TeslaFleetAPIServer::handle_get_vehicle,
      .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &vehicle);

  httpd_uri_t vehicle_post = {
    .uri = "/api/1/vehicles/*",
    .method = HTTP_POST,
    .handler = &TeslaFleetAPIServer::handle_post_vehicle,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &vehicle_post);

  httpd_uri_t proxy_version = {
    .uri = "/api/proxy/1/version",
    .method = HTTP_GET,
    .handler = &TeslaFleetAPIServer::handle_get_proxy_version,
    .user_ctx = this,
  };
  httpd_register_uri_handler(this->server_, &proxy_version);

  ESP_LOGCONFIG(TAG, "Fleet API server listening on port %u", kServerPort);
}

float TeslaFleetAPIServer::get_setup_priority() const {
  return setup_priority::AFTER_WIFI;
}

void TeslaFleetAPIServer::dump_config() {
  ESP_LOGCONFIG(TAG, "Tesla Fleet API server:");
  ESP_LOGCONFIG(TAG, "  Port: %u (fixed)", kServerPort);
  ESP_LOGCONFIG(TAG, "  VIN: %s", this->vin_.empty() ? "<not set>" : this->vin_.c_str());
}

uint64_t TeslaFleetAPIServer::fnv1a_64(const std::string &input) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  for (char ch : input) {
    hash ^= static_cast<uint8_t>(ch);
    hash *= 0x100000001b3ULL;
  }
  return hash;
}

uint64_t TeslaFleetAPIServer::get_effective_vehicle_id() {
  if (this->vehicle_id_ == 0) {
    if (!this->vin_.empty()) {
      std::string vin_upper = this->vin_;
      std::transform(vin_upper.begin(), vin_upper.end(), vin_upper.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
      });
      this->vehicle_id_ = fnv1a_64(vin_upper);
    } else {
      this->vehicle_id_ = 1;
    }
  }
  return this->vehicle_id_;
}

std::string TeslaFleetAPIServer::to_string_uint64(uint64_t value) const {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
  return std::string(buffer);
}

std::string TeslaFleetAPIServer::get_state_string() const {
  if (this->vehicle_ != nullptr) {
    auto asleep = this->vehicle_->get_is_asleep();
    if (asleep.has_value()) {
      return asleep.value() ? "asleep" : "online";
    }
  }
  return "offline";
}

std::string TeslaFleetAPIServer::escape_json(const std::string &value) {
  std::string out;
  out.reserve(value.size() + 4);
  for (char c : value) {
    switch (c) {
      case '\\':
        out += "\\\\";
        break;
      case '"':
        out += "\\\"";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (static_cast<uint8_t>(c) < 0x20) {
          char buf[7];
          std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<uint8_t>(c)));
          out += buf;
        } else {
          out += c;
        }
        break;
    }
  }
  return out;
}

std::string TeslaFleetAPIServer::format_float_optional(const optional<float> &value, int precision) {
  if (!value.has_value() || !std::isfinite(value.value())) {
    return "null";
  }
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value.value());
  return std::string(buffer);
}

std::string TeslaFleetAPIServer::format_bool_optional(const optional<bool> &value) {
  if (!value.has_value()) {
    return "null";
  }
  return value.value() ? "true" : "false";
}

std::string TeslaFleetAPIServer::build_vehicle_summary_json() {
  uint64_t id = this->get_effective_vehicle_id();
  std::string id_str = this->to_string_uint64(id);
  std::string state = this->get_state_string();

  std::stringstream ss;
  ss << "{\"response\":[{";
  ss << "\"id\":" << id_str << ",";
  ss << "\"vehicle_id\":" << id_str << ",";
  if (!this->vin_.empty()) {
    ss << "\"vin\":\"" << escape_json(this->vin_) << "\",";
  } else {
    ss << "\"vin\":null,";
  }
  ss << "\"display_name\":\"" << escape_json(this->display_name_) << "\",";
  ss << "\"state\":\"" << state << "\",";
  ss << "\"in_service\":false,";
  ss << "\"id_s\":\"" << id_str << "\",";
  ss << "\"calendar_enabled\":false,";
  ss << "\"api_version\":1";
  ss << "}],\"count\":1}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_vehicle_details_json() {
  uint64_t id = this->get_effective_vehicle_id();
  std::string id_str = this->to_string_uint64(id);
  std::string state = this->get_state_string();

  std::stringstream ss;
  ss << "{\"response\":{";
  ss << "\"id\":" << id_str << ",";
  ss << "\"vehicle_id\":" << id_str << ",";
  if (!this->vin_.empty()) {
    ss << "\"vin\":\"" << escape_json(this->vin_) << "\",";
  } else {
    ss << "\"vin\":null,";
  }
  ss << "\"display_name\":\"" << escape_json(this->display_name_) << "\",";
  ss << "\"state\":\"" << state << "\",";
  ss << "\"in_service\":false,";
  ss << "\"id_s\":\"" << id_str << "\",";
  ss << "\"calendar_enabled\":false,";
  ss << "\"api_version\":1";
  ss << "}}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_vehicle_state_object() {
  std::stringstream ss;
  ss << "{";
  bool first = true;
  auto append = [&](const std::string &key, const std::string &value) {
    if (!first) {
      ss << ",";
    }
    ss << "\"" << key << "\":" << value;
    first = false;
  };

  optional<bool> locked_opt;
  if (this->vehicle_ != nullptr) {
    auto unlocked = this->vehicle_->get_is_unlocked();
    if (unlocked.has_value()) {
      locked_opt = !unlocked.value();
    }
  }
  append("locked", format_bool_optional(locked_opt));

  optional<float> odometer;
  if (this->vehicle_ != nullptr) {
    odometer = this->vehicle_->get_odometer();
  }
  append("odometer", format_float_optional(odometer, 2));

  int64_t timestamp = esp_timer_get_time() / 1000;
  append("timestamp", std::to_string(timestamp));

  ss << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_vehicle_state_response() {
  std::stringstream ss;
  ss << "{\"response\":" << this->build_vehicle_state_object() << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_charge_state_object() {
  std::stringstream ss;
  ss << "{";
  bool first = true;
  auto append = [&](const std::string &key, const std::string &value) {
    if (!first) {
      ss << ",";
    }
    ss << "\"" << key << "\":" << value;
    first = false;
  };

  optional<std::string> charging_state;
  optional<float> battery_level;
  optional<float> energy_added;
  optional<float> charge_current;
  optional<float> charge_voltage;
  optional<float> charge_power;
  optional<float> charge_limit;
  optional<float> current_request_max;
  optional<float> minutes_to_full;
  optional<float> battery_range;
  optional<float> distance_added;
  optional<bool> charge_port_open;

  if (this->vehicle_ != nullptr) {
    charging_state = this->vehicle_->get_charging_state_text();
    battery_level = this->vehicle_->get_battery_level();
    energy_added = this->vehicle_->get_charge_energy_added();
    charge_current = this->vehicle_->get_charge_current();
    charge_voltage = this->vehicle_->get_charge_voltage();
    charge_power = this->vehicle_->get_charge_power();
    charge_limit = this->vehicle_->get_charge_limit_soc();
    current_request_max = this->vehicle_->get_charge_current_request_max();
    minutes_to_full = this->vehicle_->get_minutes_to_full_charge();
    battery_range = this->vehicle_->get_battery_range();
    distance_added = this->vehicle_->get_charge_distance_added();
    charge_port_open = this->vehicle_->get_is_charge_port_open();
  }

  append("charging_state", charging_state.has_value() ? "\"" + escape_json(charging_state.value()) + "\"" : "\"Unknown\"");
  append("battery_level", format_float_optional(battery_level, 0));
  append("usable_battery_level", format_float_optional(battery_level, 0));
  append("charge_energy_added", format_float_optional(energy_added, 2));
  append("charge_limit_soc", format_float_optional(charge_limit, 0));
  append("charge_limit_soc_std", format_float_optional(charge_limit, 0));
  append("charge_limit_soc_min", "50");
  append("charge_limit_soc_max", "100");
  append("battery_range", format_float_optional(battery_range, 2));
  append("est_battery_range", format_float_optional(battery_range, 2));
  append("ideal_battery_range", format_float_optional(battery_range, 2));
  append("charge_miles_added_rated", format_float_optional(distance_added, 2));
  append("charge_miles_added_ideal", format_float_optional(distance_added, 2));
  append("charger_voltage", format_float_optional(charge_voltage, 0));
  append("charger_actual_current", format_float_optional(charge_current, 0));
  append("charger_power", format_float_optional(charge_power, 0));
  append("charge_rate", "0");
  append("minutes_to_full_charge", format_float_optional(minutes_to_full, 0));
  append("charge_current_request", format_float_optional(current_request_max, 0));
  append("charge_current_request_max", format_float_optional(current_request_max, 0));
  append("charge_port_door_open", format_bool_optional(charge_port_open));
  append("fast_charger_present", "false");
  append("fast_charger_type", "null");
  append("fast_charger_brand", "null");
  append("managed_charging_active", "false");
  append("scheduled_charging_pending", "false");

  int64_t timestamp = esp_timer_get_time() / 1000;
  append("timestamp", std::to_string(timestamp));

  ss << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_charge_state_response() {
  std::stringstream ss;
  ss << "{\"response\":" << this->build_charge_state_object() << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_climate_state_object() {
  optional<bool> climate_on;
  if (this->vehicle_ != nullptr) {
    climate_on = this->vehicle_->get_is_climate_on();
  }

  std::stringstream ss;
  ss << "{";
  ss << "\"is_auto_conditioning_on\":" << format_bool_optional(climate_on);
  if (this->vehicle_ != nullptr) {
    auto inside = this->vehicle_->get_inside_temperature();
    auto outside = this->vehicle_->get_outside_temperature();
    ss << ",\"inside_temp\":" << format_float_optional(inside, 1);
    ss << ",\"outside_temp\":" << format_float_optional(outside, 1);
  }
  ss << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_vehicle_data_response(const std::vector<std::string> &requested_endpoints) {
  auto contains_endpoint = [&](const std::string &name) {
    if (requested_endpoints.empty()) {
      return true;
    }
    return std::any_of(requested_endpoints.begin(), requested_endpoints.end(), [&](const std::string &entry) {
      std::string entry_lower = entry;
      std::transform(entry_lower.begin(), entry_lower.end(), entry_lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
      std::string name_lower = name;
      std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
      });
      return entry_lower == name_lower;
    });
  };

  std::stringstream ss_sections;
  ss_sections << "{";
  bool first_section = true;
  auto append_section = [&](const std::string &key, const std::string &payload) {
    if (!first_section) {
      ss_sections << ",";
    }
    ss_sections << "\"" << key << "\":" << payload;
    first_section = false;
  };

  if (contains_endpoint("vehicle_state")) {
    append_section("vehicle_state", this->build_vehicle_state_object());
  }
  if (contains_endpoint("charge_state")) {
    append_section("charge_state", this->build_charge_state_object());
  }
  if (contains_endpoint("climate_state")) {
    append_section("climate_state", this->build_climate_state_object());
  }
  ss_sections << "}";

  std::stringstream ss;
  ss << "{\"response\":{";
  ss << "\"result\":true,";
  ss << "\"reason\":\"The request was successfully processed.\",";
  if (!this->vin_.empty()) {
    ss << "\"vin\":\"" << escape_json(this->vin_) << "\",";
  } else {
    ss << "\"vin\":null,";
  }
  ss << "\"command\":\"vehicle_data\",";
  ss << "\"response\":" << ss_sections.str();
  ss << "}}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_body_controller_state_object() {
  auto get_lock_state = [&]() {
    optional<bool> unlocked;
    if (this->vehicle_ != nullptr) {
      unlocked = this->vehicle_->get_is_unlocked();
    }
    if (!unlocked.has_value()) {
      return std::string("\"VEHICLELOCKSTATE_UNKNOWN\"");
    }
    return unlocked.value() ? std::string("\"VEHICLELOCKSTATE_UNLOCKED\"")
                            : std::string("\"VEHICLELOCKSTATE_LOCKED\"");
  };

  auto get_sleep_status = [&]() {
    optional<bool> asleep;
    if (this->vehicle_ != nullptr) {
      asleep = this->vehicle_->get_is_asleep();
    }
    if (!asleep.has_value()) {
      return std::string("\"VEHICLE_SLEEP_STATUS_UNKNOWN\"");
    }
    return asleep.value() ? std::string("\"VEHICLE_SLEEP_STATUS_ASLEEP\"")
                          : std::string("\"VEHICLE_SLEEP_STATUS_AWAKE\"");
  };

  auto get_user_presence = [&]() {
    optional<bool> present;
    if (this->vehicle_ != nullptr) {
      present = this->vehicle_->get_is_user_present();
    }
    if (!present.has_value()) {
      return std::string("\"VEHICLE_USER_PRESENCE_UNKNOWN\"");
    }
    return present.value() ? std::string("\"VEHICLE_USER_PRESENCE_PRESENT\"")
                           : std::string("\"VEHICLE_USER_PRESENCE_NOT_PRESENT\"");
  };

  std::stringstream ss;
  ss << "{";
  ss << "\"vehicleLockState\":" << get_lock_state() << ",";
  ss << "\"vehicleSleepStatus\":" << get_sleep_status() << ",";
  ss << "\"userPresence\":" << get_user_presence() << ",";
  ss << "\"timestamp\":" << std::to_string(esp_timer_get_time() / 1000);
  ss << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_body_controller_state_response() {
  std::stringstream ss;
  ss << "{\"response\":" << this->build_body_controller_state_object() << "}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_proxy_version_response() {
  std::stringstream ss;
  ss << "{\"response\":{";
  ss << "\"name\":\"" << std::string(PROXY_NAME) << "\",";
  ss << "\"version\":\"" << std::string(PROXY_VERSION) << "\"";
  ss << "}}";
  return ss.str();
}

std::string TeslaFleetAPIServer::build_command_response_json(bool ok, const std::string &reason) {
  std::stringstream ss;
  ss << "{\"response\":{\"result\":";
  ss << (ok ? "true" : "false");
  ss << ",\"reason\":\"";
  if (!reason.empty()) {
    ss << escape_json(reason);
  }
  ss << "\"}}";
  return ss.str();
}

bool TeslaFleetAPIServer::extract_vehicle_suffix(httpd_req_t *req, std::string *suffix) {
  const std::string uri(req->uri);
  const std::string prefix = "/api/1/vehicles/";
  if (uri.rfind(prefix, 0) != 0) {
    httpd_resp_set_status(req, "400 Bad Request");
    this->send_json(req, "{\"error\":\"invalid_vehicle_path\"}");
    return false;
  }

  std::string remainder = uri.substr(prefix.size());
  auto slash_pos = remainder.find('/');
  std::string id_part = slash_pos == std::string::npos ? remainder : remainder.substr(0, slash_pos);
  if (id_part.empty()) {
    httpd_resp_set_status(req, "400 Bad Request");
    this->send_json(req, "{\"error\":\"missing_vehicle_id\"}");
    return false;
  }

  if (!this->id_matches_vehicle(id_part)) {
    ESP_LOGW(TAG, "Vehicle identifier '%s' did not match configured VIN/hash; treating as single-vehicle proxy", id_part.c_str());
  }

  if (suffix != nullptr) {
    if (slash_pos == std::string::npos) {
      *suffix = "";
    } else {
      *suffix = remainder.substr(slash_pos);
    }
  }

  return true;
}

bool TeslaFleetAPIServer::ensure_vehicle_available(httpd_req_t *req) {
  if (this->vehicle_ == nullptr) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    this->send_json(req, "{\"error\":\"vehicle_unavailable\"}");
    return false;
  }
  return true;
}

std::string TeslaFleetAPIServer::read_request_body(httpd_req_t *req) {
  std::string body;
  size_t remaining = req->content_len;
  if (remaining > 1024) {
    ESP_LOGW(TAG, "Request body too large: %u bytes", remaining);
    return body;
  }
  body.reserve(remaining);
  char buffer[128];
  while (remaining > 0) {
    size_t to_read = std::min(remaining, sizeof(buffer));
    int read = httpd_req_recv(req, buffer, to_read);
    if (read <= 0) {
      break;
    }
    body.append(buffer, read);
    remaining -= read;
  }
  return body;
}

esp_err_t TeslaFleetAPIServer::send_json(httpd_req_t *req, const std::string &payload) {
  httpd_resp_set_type(req, "application/json");
  return httpd_resp_send(req, payload.c_str(), payload.size());
}

esp_err_t TeslaFleetAPIServer::send_not_found(httpd_req_t *req) {
  httpd_resp_set_status(req, "404 Not Found");
  return this->send_json(req, "{\"error\":\"not_found\"}");
}

esp_err_t TeslaFleetAPIServer::send_method_not_allowed(httpd_req_t *req) {
  httpd_resp_set_status(req, "405 Method Not Allowed");
  return this->send_json(req, "{\"error\":\"method_not_allowed\"}");
}

esp_err_t TeslaFleetAPIServer::handle_command_result(httpd_req_t *req, int result, const char *default_reason) {
  bool ok = (result == 0);
  if (!ok) {
    httpd_resp_set_status(req, "500 Internal Server Error");
  }
  std::string reason = ok ? "" : (default_reason != nullptr ? default_reason : "failed");
  return this->send_json(req, this->build_command_response_json(ok, reason));
}

TeslaFleetAPIServer *TeslaFleetAPIServer::from_request(httpd_req_t *req) {
  return static_cast<TeslaFleetAPIServer *>(req->user_ctx);
}

bool TeslaFleetAPIServer::id_matches_vehicle(const std::string &id_part) const {
  if (id_part.empty()) {
    return false;
  }

  auto *self = const_cast<TeslaFleetAPIServer *>(this);
  std::string expected_id = self->to_string_uint64(self->get_effective_vehicle_id());
  if (id_part == expected_id) {
    return true;
  }

  if (!this->vin_.empty()) {
    std::string vin_upper = this->vin_;
    std::transform(vin_upper.begin(), vin_upper.end(), vin_upper.begin(), [](unsigned char ch) {
      return static_cast<char>(std::toupper(ch));
    });

    std::string id_upper = id_part;
    std::transform(id_upper.begin(), id_upper.end(), id_upper.begin(), [](unsigned char ch) {
      return static_cast<char>(std::toupper(ch));
    });

    if (vin_upper == id_upper) {
      return true;
    }
  }

  return false;
}

std::vector<std::string> TeslaFleetAPIServer::parse_requested_endpoints(httpd_req_t *req) {
  std::vector<std::string> endpoints;
  size_t query_len = httpd_req_get_url_query_len(req);
  if (query_len == 0) {
    return endpoints;
  }

  std::vector<char> query_buffer(query_len + 1, 0);
  if (httpd_req_get_url_query_str(req, query_buffer.data(), query_buffer.size()) != ESP_OK) {
    return endpoints;
  }

  std::vector<char> value_buffer(query_len + 1, 0);
  if (httpd_query_key_value(query_buffer.data(), "endpoints", value_buffer.data(), value_buffer.size()) != ESP_OK) {
    return endpoints;
  }

  std::string endpoints_str(value_buffer.data());
  auto trim = [](std::string &s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
      s.erase(s.begin());
    }
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
      s.pop_back();
    }
  };

  size_t start = 0;
  while (start < endpoints_str.size()) {
    size_t end = endpoints_str.find(',', start);
    std::string token = endpoints_str.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
    trim(token);
    if (!token.empty()) {
      endpoints.push_back(token);
    }
    if (end == std::string::npos) {
      break;
    }
    start = end + 1;
  }

  return endpoints;
}

bool TeslaFleetAPIServer::parse_int_field(const std::string &body, const std::vector<std::string> &keys, int *value) {
  if (body.empty() || value == nullptr) {
    return false;
  }

  for (const auto &key : keys) {
    auto pos = body.find(key);
    if (pos == std::string::npos) {
      continue;
    }
    pos = body.find(':', pos);
    if (pos == std::string::npos) {
      continue;
    }
    size_t start = body.find_first_of("-0123456789", pos + 1);
    if (start == std::string::npos) {
      continue;
    }
    size_t end = (body[start] == '-') ? body.find_first_not_of("0123456789", start + 1)
                                      : body.find_first_not_of("0123456789", start);
    std::string number = body.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
    if (!number.empty()) {
      *value = std::atoi(number.c_str());
      return true;
    }
  }

  return false;
}

bool TeslaFleetAPIServer::parse_bool_field(const std::string &body, const std::vector<std::string> &keys, bool *value) {
  if (body.empty() || value == nullptr) {
    return false;
  }

  for (const auto &key : keys) {
    auto pos = body.find(key);
    if (pos == std::string::npos) {
      continue;
    }
    pos = body.find(':', pos);
    if (pos == std::string::npos) {
      continue;
    }
    size_t start = body.find_first_not_of(" \t\n\r\"'", pos + 1);
    if (start == std::string::npos) {
      continue;
    }
    size_t end = body.find_first_of(",} \t\n\r\"'", start);
    std::string token = body.substr(start, (end == std::string::npos) ? std::string::npos : end - start);
    if (token.empty()) {
      continue;
    }
    std::string lower = token;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
      return static_cast<char>(std::tolower(ch));
    });
    if (lower == "true" || lower == "1") {
      *value = true;
      return true;
    }
    if (lower == "false" || lower == "0") {
      *value = false;
      return true;
    }
  }

  return false;
}

esp_err_t TeslaFleetAPIServer::handle_get_vehicles(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  return self->send_json(req, self->build_vehicle_summary_json());
}

esp_err_t TeslaFleetAPIServer::handle_get_vehicle(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (!suffix.empty()) {
    auto has_prefix = [&](const std::string &target) {
      return suffix.rfind(target, 0) == 0;
    };
    if (has_prefix("/vehicle_state")) {
      if (!self->ensure_vehicle_available(req)) {
        return ESP_OK;
      }
      return self->send_json(req, self->build_vehicle_state_response());
    }
    if (has_prefix("/charge_state")) {
      if (!self->ensure_vehicle_available(req)) {
        return ESP_OK;
      }
      return self->send_json(req, self->build_charge_state_response());
    }
    if (has_prefix("/vehicle_data")) {
      if (!self->ensure_vehicle_available(req)) {
        return ESP_OK;
      }
      auto endpoints = self->parse_requested_endpoints(req);
      return self->send_json(req, self->build_vehicle_data_response(endpoints));
    }
    if (has_prefix("/body_controller_state")) {
      if (!self->ensure_vehicle_available(req)) {
        return ESP_OK;
      }
      return self->send_json(req, self->build_body_controller_state_response());
    }
    return self->send_not_found(req);
  }
  return self->send_json(req, self->build_vehicle_details_json());
}

esp_err_t TeslaFleetAPIServer::handle_get_vehicle_state(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  return self->send_json(req, self->build_vehicle_state_response());
}

esp_err_t TeslaFleetAPIServer::handle_get_charge_state(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  return self->send_json(req, self->build_charge_state_response());
}

esp_err_t TeslaFleetAPIServer::handle_get_vehicle_data(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  auto endpoints = self->parse_requested_endpoints(req);
  httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=30, must-revalidate");
  return self->send_json(req, self->build_vehicle_data_response(endpoints));
}

esp_err_t TeslaFleetAPIServer::handle_get_body_controller_state(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  return self->send_json(req, self->build_body_controller_state_response());
}

esp_err_t TeslaFleetAPIServer::handle_get_proxy_version(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  return self->send_json(req, self->build_proxy_version_response());
}

esp_err_t TeslaFleetAPIServer::handle_post_vehicle(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  std::string suffix;
  if (!self->extract_vehicle_suffix(req, &suffix)) {
    return ESP_OK;
  }
  if (suffix.empty()) {
    return self->send_method_not_allowed(req);
  }

  std::string normalized = suffix;
  auto query_pos = normalized.find('?');
  if (query_pos != std::string::npos) {
    normalized.erase(query_pos);
  }

  struct Route {
    const char *path;
    esp_err_t (*handler)(httpd_req_t *req);
  };
  static const Route routes[] = {
      {"/command/charge_start", &TeslaFleetAPIServer::handle_post_charge_start},
      {"/command/charge_stop", &TeslaFleetAPIServer::handle_post_charge_stop},
      {"/command/set_charging_amps", &TeslaFleetAPIServer::handle_post_set_amps},
      {"/command/set_charge_limit", &TeslaFleetAPIServer::handle_post_set_charge_limit},
      {"/command/auto_conditioning_start", &TeslaFleetAPIServer::handle_post_climate_start},
      {"/command/auto_conditioning_stop", &TeslaFleetAPIServer::handle_post_climate_stop},
      {"/command/set_sentry_mode", &TeslaFleetAPIServer::handle_post_set_sentry_mode},
      {"/command/wake_up", &TeslaFleetAPIServer::handle_post_wake_up},
      {"/command/charge_port_door_open", &TeslaFleetAPIServer::handle_post_charge_port_open},
      {"/command/charge_port_door_close", &TeslaFleetAPIServer::handle_post_charge_port_close},
      {"/command/flash_lights", &TeslaFleetAPIServer::handle_post_flash_lights},
      {"/command/honk_horn", &TeslaFleetAPIServer::handle_post_honk_horn},
      {"/command/door_lock", &TeslaFleetAPIServer::handle_post_door_lock},
      {"/command/door_unlock", &TeslaFleetAPIServer::handle_post_door_unlock},
      {"/wake_up", &TeslaFleetAPIServer::handle_post_wake_up},
  };

  for (const auto &route : routes) {
    if (normalized == route.path) {
      return route.handler(req);
    }
  }

  return self->send_not_found(req);
}

esp_err_t TeslaFleetAPIServer::handle_post_charge_start(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_CHARGING_SWITCH, 1);
  return self->handle_command_result(req, result, "charge_start_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_charge_stop(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_CHARGING_SWITCH, 0);
  return self->handle_command_result(req, result, "charge_stop_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_set_amps(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  std::string body = self->read_request_body(req);
  int amps = 0;
  bool found = self->parse_int_field(body, {"charging_amps", "amps"}, &amps);

  if (!found) {
    httpd_resp_set_status(req, "400 Bad Request");
    return self->send_json(req, "{\"error\":\"missing_charging_amps\"}");
  }

  if (amps < 0 || amps > 48) {
    httpd_resp_set_status(req, "400 Bad Request");
    return self->send_json(req, "{\"error\":\"invalid_charging_amps\"}");
  }

  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_CHARGING_AMPS, amps);
  return self->handle_command_result(req, result, "set_charging_amps_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_climate_start(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_HVAC_SWITCH, 1);
  return self->handle_command_result(req, result, "climate_start_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_climate_stop(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_HVAC_SWITCH, 0);
  return self->handle_command_result(req, result, "climate_stop_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_wake_up(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->wakeVehicle();
  return self->handle_command_result(req, result, "wake_up_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_charge_port_open(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_OPEN_CHARGE_PORT_DOOR, 1);
  return self->handle_command_result(req, result, "charge_port_door_open_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_charge_port_close(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_CLOSE_CHARGE_PORT_DOOR, 1);
  return self->handle_command_result(req, result, "charge_port_door_close_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_flash_lights(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(FLASH_LIGHT, 0);
  return self->handle_command_result(req, result, "flash_lights_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_honk_horn(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SOUND_HORN, 0);
  return self->handle_command_result(req, result, "honk_horn_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_door_lock(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->lockVehicle(VCSEC_RKEAction_E_RKE_ACTION_LOCK);
  return self->handle_command_result(req, result, "door_lock_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_door_unlock(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  int result = self->vehicle_->lockVehicle(VCSEC_RKEAction_E_RKE_ACTION_UNLOCK);
  return self->handle_command_result(req, result, "door_unlock_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_set_charge_limit(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  std::string body = self->read_request_body(req);
  int percent = 0;
  if (!self->parse_int_field(body, {"percent", "charge_limit", "charge_limit_soc"}, &percent)) {
    httpd_resp_set_status(req, "400 Bad Request");
    return self->send_json(req, "{\"error\":\"missing_charge_limit\"}");
  }
  if (percent < 0 || percent > 100) {
    httpd_resp_set_status(req, "400 Bad Request");
    return self->send_json(req, "{\"error\":\"invalid_charge_limit\"}");
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_CHARGING_LIMIT, percent);
  return self->handle_command_result(req, result, "set_charge_limit_failed");
}

esp_err_t TeslaFleetAPIServer::handle_post_set_sentry_mode(httpd_req_t *req) {
  auto *self = from_request(req);
  if (self == nullptr) {
    return ESP_FAIL;
  }
  if (!self->extract_vehicle_suffix(req, nullptr)) {
    return ESP_OK;
  }
  if (!self->ensure_vehicle_available(req)) {
    return ESP_OK;
  }
  std::string body = self->read_request_body(req);
  bool enable = false;
  if (!self->parse_bool_field(body, {"on", "enabled", "sentry_mode"}, &enable)) {
    httpd_resp_set_status(req, "400 Bad Request");
    return self->send_json(req, "{\"error\":\"missing_sentry_mode_state\"}");
  }
  int result = self->vehicle_->sendCarServerVehicleActionMessage(SET_SENTRY_SWITCH, enable ? 1 : 0);
  return self->handle_command_result(req, result, "set_sentry_mode_failed");
}

TeslaFleetAPIServer::~TeslaFleetAPIServer() {
  if (this->server_ != nullptr) {
    httpd_stop(this->server_);
    this->server_ = nullptr;
  }
}

}  // namespace tesla_fleet_api_server
}  // namespace esphome
