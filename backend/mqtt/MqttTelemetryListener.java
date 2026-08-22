package com.suspension.backend.mqtt;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.suspension.backend.entity.Telemetry;
import com.suspension.backend.service.TelemetryService;

import jakarta.annotation.PostConstruct;

import org.eclipse.paho.client.mqttv3.*;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Component;

@Component
public class MqttTelemetryListener {

    @Value("${mqtt.broker}")
    private String broker;

    @Value("${mqtt.topic}")
    private String topic;

    @Value("${mqtt.client-id}")
    private String clientId;

    private final TelemetryService telemetryService;

    private final ObjectMapper objectMapper =
            new ObjectMapper();

    private MqttClient mqttClient;

    public MqttTelemetryListener(
            TelemetryService telemetryService) {

        this.telemetryService = telemetryService;
    }

    @PostConstruct
    public void start() {

        try {

            System.out.println(
                    "[MQTT] Connecting to broker: "
                            + broker
            );

            mqttClient = new MqttClient(
                    broker,
                    clientId
            );

            MqttConnectOptions options =
                    new MqttConnectOptions();

            options.setAutomaticReconnect(true);
            options.setCleanSession(true);

            mqttClient.connect(options);

            System.out.println(
                    "[MQTT] Connected"
            );

            mqttClient.subscribe(
                    topic,
                    this::handleMessage
            );

            System.out.println(
                    "[MQTT] Subscribed to: "
                            + topic
            );

        } catch (Exception e) {

            System.err.println(
                    "[MQTT] Connection failed: "
                            + e.getMessage()
            );
        }
    }

    private void handleMessage(
            String receivedTopic,
            MqttMessage message) {

        try {

            String payload =
                    new String(message.getPayload());

            System.out.println(
                    "[MQTT] Received: "
                            + payload
            );

            JsonNode json =
                    objectMapper.readTree(payload);

            Telemetry telemetry =
                    new Telemetry();

            telemetry.setAcceleration(
                    (float) json
                            .path("acceleration")
                            .asDouble()
            );

            telemetry.setForce(
                    (float) json
                            .path("force")
                            .asDouble()
            );

            telemetry.setCurrent(
                    (float) json
                            .path("current")
                            .asDouble()
            );

            telemetry.setTemperature(
                    (float) json
                            .path("temperature")
                            .asDouble()
            );

            telemetry.setSafeMode(
                    json.path("safeMode")
                            .asBoolean()
            );

            telemetry.setSensorError(
                    json.path("sensorError")
                            .asInt()
            );

            telemetry.setCoilError(
                    json.path("coilError")
                            .asInt()
            );

            telemetry.setTimestamp(
                    json.path("timestamp")
                            .asLong()
            );

            telemetryService.save(telemetry);

            System.out.println(
                    "[DB] Telemetry saved"
            );

        } catch (Exception e) {

            System.err.println(
                    "[MQTT] Invalid telemetry: "
                            + e.getMessage()
            );
        }
    }
}