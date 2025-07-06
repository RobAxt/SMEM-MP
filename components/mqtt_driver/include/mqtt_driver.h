#ifndef MQTT_DRIVER_H
#define MQTT_DRIVER_H

/**
 * @file mqtt_driver.h
 * @brief Header file for the MQTT driver component.
 * @details This file contains the declaration of the function used to start the MQTT client.
 * 
 * @author  Roberto Axt
 * @date    2025-06-28 
 * @version 0.0
 * 
 * @par License
 * This project is licensed under the MIT License.
 */ 

#include "esp_err.h"
#include "esp_netif_ip_addr.h"

// Define the maximum number of MQTT messages that can be published at the same time
#define MAX_PUBLISH_MSG 10
// Define the maximum number of MQTT topics that can be subscribed to
#define MAX_SUBSCRIBE_MSG 10
// Define the maximum size of the payload and topic strings
#define MAX_PAYLOAD_SIZE 256
#define MAX_TOPIC_SIZE 64

// Forward declaration of the MQTT message handler type
// This type of function will be called when a message is received on a subscribed topic 
typedef void (*mqtt_msg_handler_t)(const char *topic, const char *data);

/**
 * @brief Start the MQTT client module.
 * @details This function initializes and starts the MQTT client, allowing it to connect to an MQTT broker and
 *          handle MQTT messages.
 *
 * @param broker The IP address of the MQTT broker to connect to.
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_STATE if the MQTT client is already started
 *          - ESP_ERR_NO_MEM if there is insufficient memory to start the MQTT client
 *          - ESP_FAIL on any other failure
 **/
esp_err_t mqtt_client_start(esp_ip4_addr_t broker);

/**
 * @brief Publish a message to a specified MQTT topic.
 * @details This function publishes a message to the specified MQTT topic. The MQTT client must be
 *          started before calling this function.
 * 
 * @param topic The MQTT topic to which the message will be published.
 * @param payload The message payload to be published.
 * @param qos The Quality of Service level for the message (0, 1, or 2).
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_STATE if the MQTT client is not started
 *          - ESP_ERR_INVALID_ARG if the topic or payload is NULL
 *          - ESP_FAIL on any other failure
 **/
esp_err_t mqtt_client_publish(const char *topic, const char *payload, int qos);

/**
 * @brief Subscribe to an MQTT topic with a message handler.
 * @details This function subscribes to the specified MQTT topic and registers a handler function
 *          that will be called when messages are received on that topic.
 * 
 * @param topic The MQTT topic to subscribe to.
 * @param handler The function to handle incoming messages on the subscribed topic.
 * @param qos The Quality of Service level for the subscription (0, 1, or 2).
 * @return
 *          - ESP_OK on success
 *          - ESP_ERR_INVALID_STATE if the MQTT client is not started
 *          - ESP_ERR_INVALID_ARG if the topic or handler is NULL
 *          - ESP_FAIL on any other failure
 **/
esp_err_t mqtt_client_suscribe(const char *topic, mqtt_msg_handler_t handler, int qos);


#endif  // MQTT_DRIVER_H