#include "RoboRuckusBot.h"

extern bool POSTSuccess;

/// @brief Creates a RoboRuckus bot controller
/// @param Name Name of the device (must be known to game server)
/// @param server_url The URL of the game server
/// @param configFile Name of the config file to use
RoboRuckusBot::RoboRuckusBot(String Name, String server_url, String configFile) : Actor(Name), communicator(server_url) {
	config_path = "/settings/act/" + configFile;
	commandQueue = xQueueCreate(5, sizeof(RoboRuckusDevice::eventPayload*));
	RuckusCommunicator::Config.controller = Name;
}

/// @brief Starts a RoboRuckus bot controller
/// @return True on success
bool RoboRuckusBot::begin() { 
	// Set description
	Description.type = "robot";
	Description.actions = {{"move", 0}, {"assignPlayer", 1}, {"takeDamage", 2}, {"reset", 3}, {"stopMove", 4}, {"enterConfig", 5}, {"exitConfig", 6}};
	Description.version = "1.0.0";
	bool success = false;
	if (!checkConfig(config_path)) {
		// Set defaults
		task_config.taskPeriod = 100;
		success = setConfig(getConfig(), true);
	} else {
		// Load settings
		success = setConfig(Storage::readFile(config_path), false);
	}
	// Enable the task
	task_config.set_taskName(Description.name.c_str());
	success = enableTask(true);
	// Create setup task
	if (success) {
		success = xTaskCreate(setupTaskWrapper, "RoboRuckus Setup", 2048, this, 2, NULL) == pdPASS;
	}
	return success;
}

/// @brief Wraps the setup task for static access
/// @param arg The RoboRuckusBot object starting up
void RoboRuckusBot::setupTaskWrapper(void* pvParameters){
	static_cast<RoboRuckusBot*>(pvParameters)->setup();
}

/// @brief Runs necessary setup for robot
void RoboRuckusBot::setup() {
	while (!POSTSuccess) {
		delay(100);
	}
	
	addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::NOTREADY, .eventType = 0, .magnitude = 0});
	Logger.println("Starting robot...");
	Logger.println("Starting communicator");
	if (!communicator.begin()) {
		Logger.println("Could not start communicator");
		while(1);
	}

	Logger.print("RoboRuckus devices: ");
	Logger.println(RoboRuckusDevice::ruckusEventReceivers.size());
	Logger.println("Joining server...");
	JsonDocument botInfo;
	botInfo["name"] = Description.name;
	botInfo["ip"] = communicator.Config.robotIP;
	botInfo["code"] = 0;
	String output;
	serializeJson(botInfo, output);
	do {
		delay(1000);
		String response = communicator.callHook(output, RuckusCommunicator::PUT, "/bot");
		DeserializationError error = deserializeJson(botInfo, response);
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			continue;
		}
		Logger.println("Sever response: " + botInfo["code"].as<String>() + " " + botInfo["response"].as<String>());
	} while(botInfo["code"].as<int>() != HTTP_CODE_ACCEPTED);
	addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::READY, .eventType = 0, .magnitude = 0});
	Logger.println("Robot ready!");
	vTaskDelete(NULL);
}

/// @brief Receives an action
/// @param action The action to process
/// @param payload JSON formatted payload associated with an action
/// @return JSON response with OK
std::tuple<bool, String> RoboRuckusBot::receiveAction(int action, String payload) {
	JsonDocument doc;
	if (payload != "") {
		// Deserialize file contents
		DeserializationError error = deserializeJson(doc, payload);
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			return { true, R"({"success": false, "Response": "Bad payload"})" };
		}
	}
	switch (action) {
		case 0:
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::MOVESTART, .eventType = doc["move"].as<int>(), .magnitude = doc["magnitude"].as<int>()});
			break;
		case 1:
			RuckusCommunicator::Config.playerNumber = doc["player"].as<int>();
			RuckusCommunicator::Config.robotID = doc["botNumber"].as<int>();
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::ASSIGNPLAYER, .eventType = doc["botNumber"].as<int>(), .magnitude = doc["playerNumber"].as<int>()});
			break;
		case 2:
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::TAKEDAMAGE, .eventType = 0, .magnitude = doc["magnitude"].as<int>()});
			break;
		case 3:
			RuckusCommunicator::Config.playerNumber = 0;
			RuckusCommunicator::Config.robotID = -1;
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::RESET, .eventType = 0, .magnitude = 0});
			break;
		case 4:
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::MOVEFINISH, .eventType = 0, .magnitude = 0});
			break;
		case 5:
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::ENTERCONFIG, .eventType = 0, .magnitude = 0});
			break;
		case 6:
			addCommandToQueue(new RoboRuckusDevice::eventPayload {.event = RoboRuckusDevice::Events::EXITCONFIG, .eventType = 0, .magnitude = 0});
			break;
		default:
			return { true, R"({"success": false, "Response": "Bad command"})" };
			break;
	}
	return { true, R"({"success": true, "Response": "OK"})" };
}

/// @brief Gets the current config
/// @return A JSON string of the config
String RoboRuckusBot::getConfig() {
	// Allocate the JSON document
	JsonDocument doc;

	// Create string to hold output
	String output;
	doc["serverURL"] = communicator.getURL();
	doc["taskPeriod"] = task_config.taskPeriod;
	
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool RoboRuckusBot::setConfig(String config, bool save) {
	// Allocate the JSON document
	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, config);
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return false;
	}
	communicator.setURL(doc["serverURL"].as<String>());
	task_config.taskPeriod = doc["taskPeriod"].as<int>();

	if (save) {
		return saveConfig(config_path, config);
	}
	return true;
}

/// @brief Runs any commands in the robot action queue
/// @param elapsed The milliseconds since this function was last called
void RoboRuckusBot::runTask(ulong elapsed) {
	if (taskPeriodTriggered(elapsed)) {
		processCommandQueue();
	}
}

/// @brief Adds a command to the queue to be processed
/// @param event The event payload to add
/// @return True on success
bool RoboRuckusBot::addCommandToQueue(RoboRuckusDevice::eventPayload* event) {
	if (event != nullptr && commandQueue != nullptr) {
		return xQueueSend(commandQueue, &event, 100 / portTICK_PERIOD_MS) == pdTRUE;
	}
	Logger.println("Invalid event or queue");
	return false;
}

/// @brief Process next command in queue
bool RoboRuckusBot::processCommandQueue()
{
	RoboRuckusDevice::eventPayload* payload = nullptr;
	// Process all items in queue
	while (xQueueReceive(commandQueue, &payload, 10 / portTICK_PERIOD_MS) == pdTRUE) {
		for (const auto& r : RoboRuckusDevice::ruckusEventReceivers) {
			if (payload != nullptr) {
				if (r != NULL) {
					r->receiveEvent(payload);
				}
			} else {
				Logger.println("Pointer in event was null");
			}
		}
		delete payload;
	}
	return true;
}