package com.suspension.backend;

import jakarta.persistence.*;

@Entity
@Table(name = "telemetry")
public class Telemetry {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    private double acceleration;

    @Column(name = "force_value")
    private double force;

    @Column(name = "current_value")
    private double current;

    private double temperature;

    @Column(name = "safe_mode")
    private boolean safeMode;

    @Column(name = "sensor_error")
    private int sensorError;

    @Column(name = "coil_error")
    private int coilError;

    @Column(name = "timestamp_ms")
    private long timestamp;

    public Long getId() {
        return id;
    }

    public double getAcceleration() {
        return acceleration;
    }

    public void setAcceleration(double acceleration) {
        this.acceleration = acceleration;
    }

    public double getForce() {
        return force;
    }

    public void setForce(double force) {
        this.force = force;
    }

    public double getCurrent() {
        return current;
    }

    public void setCurrent(double current) {
        this.current = current;
    }

    public double getTemperature() {
        return temperature;
    }

    public void setTemperature(double temperature) {
        this.temperature = temperature;
    }

    public boolean isSafeMode() {
        return safeMode;
    }

    public void setSafeMode(boolean safeMode) {
        this.safeMode = safeMode;
    }

    public int getSensorError() {
        return sensorError;
    }

    public void setSensorError(int sensorError) {
        this.sensorError = sensorError;
    }

    public int getCoilError() {
        return coilError;
    }

    public void setCoilError(int coilError) {
        this.coilError = coilError;
    }

    public long getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(long timestamp) {
        this.timestamp = timestamp;
    }
}