#pragma once

#include <esp_http_server.h>

#include <cstdint>
#include <string>
#include <vector>

#include "esphome/core/component.h"
#include "esphome/core/optional.h"

#include "esphome/components/tesla_ble_vehicle/tesla_ble_vehicle.h"

namespace esphome {
namespace tesla_fleet_api_server {

class TeslaFleetAPIServer : public Component {
 public:
  void set_vehicle(tesla_ble_vehicle::TeslaBLEVehicle *vehicle) { vehicle_ = vehicle; }
  void set_display_name(const std::string &display_name) { display_name_ = display_name; }
  void set_vin(const std::string &vin) { vin_ = vin; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  void loop() override {}
  ~TeslaFleetAPIServer();

 protected:
  tesla_ble_vehicle::TeslaBLEVehicle *vehicle_{nullptr};
  httpd_handle_t server_{nullptr};
  uint64_t vehicle_id_{0};
  std::string display_name_{"Tesla"};
  std::string vin_;

  static constexpr uint16_t kServerPort = 80;

  uint64_t get_effective_vehicle_id();
  std::string to_string_uint64(uint64_t value) const;
  std::string get_state_string() const;
  std::string build_vehicle_summary_json();
  std::string build_vehicle_details_json();
  std::string build_vehicle_state_object();
  std::string build_vehicle_state_response();
  std::string build_charge_state_object();
  std::string build_charge_state_response();
  std::string build_climate_state_object();
  std::string build_vehicle_data_response(const std::vector<std::string> &requested_endpoints = {});
  std::string build_body_controller_state_object();
  std::string build_body_controller_state_response();
  std::string build_proxy_version_response();
  std::string build_command_response_json(bool ok, const std::string &reason = std::string());
  static std::string escape_json(const std::string &value);
  static std::string format_float_optional(const optional<float> &value, int precision = 2);
  static std::string format_bool_optional(const optional<bool> &value);
  static uint64_t fnv1a_64(const std::string &input);

  bool extract_vehicle_suffix(httpd_req_t *req, std::string *suffix);
  bool ensure_vehicle_available(httpd_req_t *req);
  std::string read_request_body(httpd_req_t *req);
  esp_err_t send_json(httpd_req_t *req, const std::string &payload);
  esp_err_t send_not_found(httpd_req_t *req);
  esp_err_t send_method_not_allowed(httpd_req_t *req);
  esp_err_t handle_command_result(httpd_req_t *req, int result, const char *default_reason);
  static TeslaFleetAPIServer *from_request(httpd_req_t *req);

  bool parse_int_field(const std::string &body, const std::vector<std::string> &keys, int *value);
  bool parse_bool_field(const std::string &body, const std::vector<std::string> &keys, bool *value);
  std::vector<std::string> parse_requested_endpoints(httpd_req_t *req);
  bool id_matches_vehicle(const std::string &id_part) const;

  static esp_err_t handle_get_vehicles(httpd_req_t *req);
  static esp_err_t handle_get_vehicle(httpd_req_t *req);
  static esp_err_t handle_get_vehicle_state(httpd_req_t *req);
  static esp_err_t handle_get_charge_state(httpd_req_t *req);
  static esp_err_t handle_get_vehicle_data(httpd_req_t *req);
  static esp_err_t handle_get_body_controller_state(httpd_req_t *req);
  static esp_err_t handle_get_proxy_version(httpd_req_t *req);
  static esp_err_t handle_post_vehicle(httpd_req_t *req);
  static esp_err_t handle_post_charge_start(httpd_req_t *req);
  static esp_err_t handle_post_charge_stop(httpd_req_t *req);
  static esp_err_t handle_post_set_amps(httpd_req_t *req);
  static esp_err_t handle_post_climate_start(httpd_req_t *req);
  static esp_err_t handle_post_climate_stop(httpd_req_t *req);
  static esp_err_t handle_post_wake_up(httpd_req_t *req);
  static esp_err_t handle_post_charge_port_open(httpd_req_t *req);
  static esp_err_t handle_post_charge_port_close(httpd_req_t *req);
  static esp_err_t handle_post_flash_lights(httpd_req_t *req);
  static esp_err_t handle_post_honk_horn(httpd_req_t *req);
  static esp_err_t handle_post_door_lock(httpd_req_t *req);
  static esp_err_t handle_post_door_unlock(httpd_req_t *req);
  static esp_err_t handle_post_set_charge_limit(httpd_req_t *req);
  static esp_err_t handle_post_set_sentry_mode(httpd_req_t *req);
};

}  // namespace tesla_fleet_api_server
}  // namespace esphome
