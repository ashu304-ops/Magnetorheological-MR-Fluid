package com.suspension.backend.websocket;

import com.suspension.backend.Telemetry;
import org.springframework.messaging.simp.SimpMessagingTemplate;
import org.springframework.stereotype.Service;

@Service
public class TelemetryWebSocketPublisher {

    private final SimpMessagingTemplate messagingTemplate;

    public TelemetryWebSocketPublisher(
            SimpMessagingTemplate messagingTemplate) {

        this.messagingTemplate = messagingTemplate;
    }

    public void publish(Telemetry telemetry) {

        messagingTemplate.convertAndSend(
                "/topic/telemetry",
                telemetry
        );
    }
}