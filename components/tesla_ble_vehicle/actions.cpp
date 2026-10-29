#include <car_server.pb.h>
#include <client.h>
#include <errors.h>

#include "tesla_ble_vehicle.h"

/*
*   CarServer message builders. These lived in the TeslaBLE library until it removed them in 2026.8.1, which only
*   keeps the generic buildCarServerActionPayload. Function names follow upstream's actions.cpp; only the actions
*   used in ACTION_SPECIFICS are built.
*/
namespace esphome
{
  namespace tesla_ble_vehicle
  {
    int TeslaBLEVehicle::actions_buildCarServerGetVehicleDataMessage(pb_byte_t *output_buffer, size_t *output_length, int which_get)
    {
      // Build generic part
      actions_action_message_                                                   = CarServer_Action_init_default;
      actions_action_message_.which_action_msg                                  = CarServer_Action_vehicleAction_tag;
      actions_action_message_.action_msg.vehicleAction                          = CarServer_VehicleAction_init_default;
      actions_action_message_.action_msg.vehicleAction.which_vehicle_action_msg = CarServer_VehicleAction_getVehicleData_tag;

      auto& vehicle_action_msg_          = actions_action_message_.action_msg.vehicleAction.vehicle_action_msg; // For legibility
      vehicle_action_msg_.getVehicleData = CarServer_GetVehicleData_init_default;
      // Now the get specific part
      switch (which_get)
      {
        case CarServer_GetVehicleData_getChargeState_tag:
          vehicle_action_msg_.getVehicleData.getChargeState     = CarServer_GetChargeState_init_default;
          vehicle_action_msg_.getVehicleData.has_getChargeState = true;
          break;
        case CarServer_GetVehicleData_getClimateState_tag:
          vehicle_action_msg_.getVehicleData.getClimateState     = CarServer_GetClimateState_init_default;
          vehicle_action_msg_.getVehicleData.has_getClimateState = true;
          break;
        case CarServer_GetVehicleData_getDriveState_tag:
          vehicle_action_msg_.getVehicleData.getDriveState     = CarServer_GetDriveState_init_default;
          vehicle_action_msg_.getVehicleData.has_getDriveState = true;
          break;
        case CarServer_GetVehicleData_getLocationState_tag:
          vehicle_action_msg_.getVehicleData.getLocationState     = CarServer_GetLocationState_init_default;
          vehicle_action_msg_.getVehicleData.has_getLocationState = true;
          break;
        case CarServer_GetVehicleData_getClosuresState_tag:
          vehicle_action_msg_.getVehicleData.getClosuresState     = CarServer_GetClosuresState_init_default;
          vehicle_action_msg_.getVehicleData.has_getClosuresState = true;
          break;
        case CarServer_GetVehicleData_getTirePressureState_tag:
          vehicle_action_msg_.getVehicleData.getTirePressureState     = CarServer_GetTirePressureState_init_default;
          vehicle_action_msg_.getVehicleData.has_getTirePressureState = true;
          break;
        default:
          ESP_LOGE(TAG, "Invalid which_get type %d, action message not built", which_get);
          return TeslaBLE::TeslaBLE_Status_E_ERROR_INTERNAL;
      }
      // Add it to the message
      int status = tesla_ble_client_->buildCarServerActionPayload(&actions_action_message_, output_buffer, output_length);
      if (status != TeslaBLE::TeslaBLE_Status_E_OK)
      {
        ESP_LOGE(TAG, "[buildCarServerGetVehicleDataMessage] Failed to build car action message");
        return status;
      }
      return TeslaBLE::TeslaBLE_Status_E_OK;
    }

    int TeslaBLEVehicle::actions_buildCarServerVehicleActionMessage(int32_t set_value, pb_byte_t *output_buffer, size_t *output_length, int which_tag)
    {
      // Build generic part
      actions_action_message_                                                   = CarServer_Action_init_default;
      actions_action_message_.which_action_msg                                  = CarServer_Action_vehicleAction_tag;
      actions_action_message_.action_msg.vehicleAction                          = CarServer_VehicleAction_init_default;
      actions_action_message_.action_msg.vehicleAction.which_vehicle_action_msg = which_tag;

      auto& vehicle_action_msg_ = actions_action_message_.action_msg.vehicleAction.vehicle_action_msg; // For legibility
      // Now the specific part
      switch (which_tag)
      {
        case CarServer_VehicleAction_setChargingAmpsAction_tag:
          vehicle_action_msg_.setChargingAmpsAction               = CarServer_SetChargingAmpsAction_init_default;
          vehicle_action_msg_.setChargingAmpsAction.charging_amps = set_value;
          break;
        case CarServer_VehicleAction_chargingSetLimitAction_tag:
          vehicle_action_msg_.chargingSetLimitAction         = CarServer_ChargingSetLimitAction_init_default;
          vehicle_action_msg_.chargingSetLimitAction.percent = set_value;
          break;
        case CarServer_VehicleAction_chargingStartStopAction_tag:
          vehicle_action_msg_.chargingStartStopAction = CarServer_ChargingStartStopAction_init_default;
          if (set_value == 1)
          {
            vehicle_action_msg_.chargingStartStopAction.which_charging_action = CarServer_ChargingStartStopAction_start_tag;
            vehicle_action_msg_.chargingStartStopAction.charging_action.start = CarServer_Void_init_default;
          }
          else
          {
            vehicle_action_msg_.chargingStartStopAction.which_charging_action = CarServer_ChargingStartStopAction_stop_tag;
            vehicle_action_msg_.chargingStartStopAction.charging_action.stop  = CarServer_Void_init_default;
          }
          break;
        case CarServer_VehicleAction_vehicleControlSetSentryModeAction_tag:
          vehicle_action_msg_.vehicleControlSetSentryModeAction    = CarServer_VehicleControlSetSentryModeAction_init_default;
          vehicle_action_msg_.vehicleControlSetSentryModeAction.on = (set_value != 0);
          break;
        case CarServer_VehicleAction_hvacAutoAction_tag:
          vehicle_action_msg_.hvacAutoAction          = CarServer_HvacAutoAction_init_default;
          vehicle_action_msg_.hvacAutoAction.power_on = (set_value != 0);
          break;
        case CarServer_VehicleAction_hvacSteeringWheelHeaterAction_tag:
          vehicle_action_msg_.hvacSteeringWheelHeaterAction          = CarServer_HvacSteeringWheelHeaterAction_init_default;
          vehicle_action_msg_.hvacSteeringWheelHeaterAction.power_on = (set_value != 0);
          break;
        case CarServer_VehicleAction_chargePortDoorOpen_tag:
          vehicle_action_msg_.chargePortDoorOpen             = CarServer_ChargePortDoorOpen_init_default;
          vehicle_action_msg_.chargePortDoorOpen.dummy_field = 1;
          break;
        case CarServer_VehicleAction_chargePortDoorClose_tag:
          vehicle_action_msg_.chargePortDoorClose             = CarServer_ChargePortDoorClose_init_default;
          vehicle_action_msg_.chargePortDoorClose.dummy_field = 1;
          break;
        case CarServer_VehicleAction_vehicleControlFlashLightsAction_tag:
          vehicle_action_msg_.vehicleControlFlashLightsAction             = CarServer_VehicleControlFlashLightsAction_init_default;
          vehicle_action_msg_.vehicleControlFlashLightsAction.dummy_field = 1;
          break;
        case CarServer_VehicleAction_vehicleControlHonkHornAction_tag:
          vehicle_action_msg_.vehicleControlHonkHornAction             = CarServer_VehicleControlHonkHornAction_init_default;
          vehicle_action_msg_.vehicleControlHonkHornAction.dummy_field = 1;
          break;
        case CarServer_VehicleAction_vehicleControlWindowAction_tag:
          vehicle_action_msg_.vehicleControlWindowAction = CarServer_VehicleControlWindowAction_init_default;
          if (set_value == 1)
          {
            vehicle_action_msg_.vehicleControlWindowAction.which_action = CarServer_VehicleControlWindowAction_vent_tag;
            vehicle_action_msg_.vehicleControlWindowAction.action.vent  = CarServer_Void_init_default;
          }
          else
          {
            vehicle_action_msg_.vehicleControlWindowAction.which_action = CarServer_VehicleControlWindowAction_close_tag;
            vehicle_action_msg_.vehicleControlWindowAction.action.close = CarServer_Void_init_default;
          }
          break;
        case CarServer_VehicleAction_hvacSetPreconditioningMaxAction_tag:
          vehicle_action_msg_.hvacSetPreconditioningMaxAction    = CarServer_HvacSetPreconditioningMaxAction_init_default;
          vehicle_action_msg_.hvacSetPreconditioningMaxAction.on = (set_value != 0);
          break;
        default:
          ESP_LOGE(TAG, "Tag type %d, car server vehicle action message not built", which_tag);
          return TeslaBLE::TeslaBLE_Status_E_ERROR_INTERNAL;
      }
      // Add it to the message
      int status = tesla_ble_client_->buildCarServerActionPayload(&actions_action_message_, output_buffer, output_length);
      if (status != TeslaBLE::TeslaBLE_Status_E_OK)
      {
        ESP_LOGE(TAG, "Failed to build car server vehicle action message");
        return status;
      }
      return TeslaBLE::TeslaBLE_Status_E_OK;
    }
  } // namespace tesla_ble_vehicle
} // namespace esphome
