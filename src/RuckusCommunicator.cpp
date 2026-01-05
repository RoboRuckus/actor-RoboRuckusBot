#include "RuckusCommunicator.h"

// Initialize static variables
RuckusCommunicator::event_config RuckusCommunicator::Config;

/// @brief Creates a RuckusCommunicator object
RuckusCommunicator::RuckusCommunicator(String url) : webhook(url) {
    severAddress = url;
}

/// @brief Starts a communicator
/// @return True on success
bool RuckusCommunicator::begin() {
   Config.robotIP = WiFi.localIP().toString();
   return true;
}

/// @brief Gets the current base URL of the server
String RuckusCommunicator::getURL() {
    return webhook.webhook_config.url;
}

/// @brief Sets the current base URL of the server
void RuckusCommunicator::setURL(String url) {
    webhook.webhook_config.url = url;
    severAddress = url;
}

/// @brief Sends a signal to the server indicating the robot has finished moving
void RuckusCommunicator::moveComplete() {
	String content = "{\"bot\":" + String(Config.robotID) + "}";
	JsonDocument response;
	int tries = 0;
	do{
		String response_string = callHook(content, POST, "/bot/Done/");
		DeserializationError error = deserializeJson(response, response_string);
		// Test if parsing succeeds.
		if (error) {
			Logger.print(F("Deserialization failed: "));
			Logger.println(error.f_str());
			continue;
		}
		Logger.println("Sever response: " + response["code"].as<String>() + " " + response["response"].as<String>());
		tries++;
	} while(response["code"].as<int>() != HTTP_CODE_ACCEPTED || tries > 3);
}

/// @brief Calls a webhook to communicate with the game server
/// @param content The contest of the request
/// @param method The request method
/// @param path The path of the request on top of the base URL (the "/" and everything after)
/// @return The JSON string response from the Webhook utility
String RuckusCommunicator::callHook(String content, HTTP_Methods method, String path) {
	webhook.webhook_config.url = severAddress + path;
	String response = "{}";
	switch (method) {
		case GET:
			response = webhook.getRequest(content);
			break;		
		case PUT:
			response = webhook.putRequest(content);
			break;
		case POST:
			response = webhook.postRequest(content);
			break;
	}
	// Reset URL just in case
	webhook.webhook_config.url = severAddress;
	return response;
}