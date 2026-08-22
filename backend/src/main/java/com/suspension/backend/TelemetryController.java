package com.suspension.backend;

import com.suspension.backend.service.TelemetryService;

import com.suspension.backend.Telemetry;


import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/telemetry")
@CrossOrigin(origins = "http://localhost:5173")
public class TelemetryController {

    private final TelemetryService telemetryService;

    public TelemetryController(
            TelemetryService telemetryService) {

        this.telemetryService = telemetryService;
    }

    @GetMapping
    public List<Telemetry> getAll() {

        return telemetryService.getAll();
    }

    @GetMapping("/latest")
    public Telemetry getLatest() {

        return telemetryService.getLatest();
    }
}