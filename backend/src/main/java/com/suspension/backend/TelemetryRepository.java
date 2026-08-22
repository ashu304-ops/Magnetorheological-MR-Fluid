package com.suspension.backend;

import com.suspension.backend.Telemetry;
import org.springframework.data.jpa.repository.JpaRepository;

public interface TelemetryRepository
        extends JpaRepository<Telemetry, Long> {
}