package com.suspension.backend.service;

import com.suspension.backend.Telemetry;
import com.suspension.backend.TelemetryRepository;
import com.suspension.backend.websocket.TelemetryWebSocketPublisher;

import org.springframework.stereotype.Service;

import java.util.List;

@Service
public class TelemetryService {

    private final TelemetryRepository repository;
    private final TelemetryWebSocketPublisher webSocketPublisher;

    public TelemetryService(
            TelemetryRepository repository,
            TelemetryWebSocketPublisher webSocketPublisher) {

        this.repository = repository;
        this.webSocketPublisher = webSocketPublisher;
    }

    public void save(Telemetry telemetry) {

        // Save telemetry to MySQL
        Telemetry saved = repository.save(telemetry);

        System.out.println("[DB] Telemetry saved");

        // Send telemetry immediately to React
        webSocketPublisher.publish(saved);

        System.out.println("[WS] Telemetry sent to dashboard");
    }

    public Telemetry getLatest() {

        return repository.findAll()
                .stream()
                .reduce((first, second) -> second)
                .orElse(null);
    }

    public List<Telemetry> getAll() {

        return repository.findAll();
    }
}