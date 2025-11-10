/*
 * This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2025 Sam Groveman
 * 
 * External libraries needed:
 * ArduinoJSON: https://arduinojson.org/
 * RoboRuckusDevice: https://github.com/RoboRuckus/util-RoboRuckusDevice
 * 
 * Contributors: Sam Groveman
 */
#pragma once
#include <Actor.h>
#include <ActorManager.h>
#include <PeriodicTask.h>
#include <ArduinoJson.h>
#include <RuckusCommunicator.h>
#include <RoboRuckusDevice.h>
#include <vector>
#include <map>

/// @brief Class describing a RoboRuckus robot controller
class RoboRuckusBot : public Actor, public PeriodicTask {
	public:
		RoboRuckusBot(String Name, String server_url, String configFile = "RoboRuckusBot.json");
		bool begin();
		std::tuple<bool, String> receiveAction(int action, String payload = "");
		String getConfig();
		bool setConfig(String config, bool save);
		void runTask(ulong elapsed);

	protected:
		// @brief Holds button webhook configuration
		struct {
			/// @brief The game server URL
			String server_url;
		} bot_config;

		/// @brief Path to configuration file
		String config_path;

		/// @brief Queue to hold commands to be processed.
		QueueHandle_t commandQueue;

		/// @brief Object for communicating with the server
		RuckusCommunicator communicator;

		static void setupTaskWrapper(void* pvParameters);
		void setup();
		bool addCommandToQueue(RoboRuckusDevice::eventPayload* event);
		bool processCommandQueue();
};