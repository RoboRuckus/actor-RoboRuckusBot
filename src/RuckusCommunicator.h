/*
* This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2025 Sam Groveman
* 
* External libraries needed:
* ArduinoJSON: https://arduinojson.org/
* Webhook: https://github.com/FabricaIO/util-Webhook
* 
* Contributors: Sam Groveman
*/
#pragma once
#include <ArduinoJson.h>
#include <Webhook.h>
#include <esp_wifi.h>

/// @brief Utility class to facilitate communication to the RoboRuckus game server
class RuckusCommunicator {
	private:
		/// @brief Webhook to for sending signals back to server
		Webhook webhook;

		/// @brief Stores shared configurations data
		struct event_config {
			/// @brief The player number currently assigned to the robot (0 for unassigned)
			int playerNumber = 0;

			/// @brief The assigned ID of the robot (-1 for unassigned)
			int robotID = -1;

			/// @brief The robots IP address
			String robotIP;

			/// @brief Name of robot controller actor
			String controller = "";
		};

		/// @brief Stores the base game sever address
		String severAddress;

	public:
		/// @brief defines the HTTP methods available
		enum HTTP_Methods {
			GET,
			POST,
			PUT
		};

		/// @brief Types of movement that are allowed
		enum MoveTypes {
			FORWARD,
			BACKWARD,
			TURNLEFT,
			TURNRIGHT,
			SLIDELEFT,
			SLIDERIGHT
		};

		/// @brief Provides access to a shared configuration for the robot
		static event_config Config;

		RuckusCommunicator(String url);
		bool begin();
		String getURL();
		void setURL(String url);
		void moveComplete();
		String callHook(String content, HTTP_Methods method, String path);
};