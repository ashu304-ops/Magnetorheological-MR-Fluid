
#!/usr/bin/env bash

set -u

# ============================================================
# VEHICLE SUSPENSION SYSTEM
# Full Stack Launcher - Arch Linux
#
# Starts:
#   1. Mosquitto MQTT broker
#   2. MQTT monitor
#   3. Spring Boot backend
#   4. QEMU STM32 + MQTT bridge
#   5. React dashboard
#
# No xterm required.
# ============================================================

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BACKEND_DIR="$PROJECT_DIR/backend"
FRONTEND_DIR="$PROJECT_DIR/suspension-dashboard"
BUILD_DIR="$PROJECT_DIR/build"
TOOLS_DIR="$PROJECT_DIR/tools"

LOG_DIR="$PROJECT_DIR/logs"

mkdir -p "$LOG_DIR"

# ============================================================
# COLORS
# ============================================================

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

# ============================================================
# HELPER
# ============================================================

log()
{
    echo -e "${BLUE}[SYSTEM]${NC} $1"
}

success()
{
    echo -e "${GREEN}[OK]${NC} $1"
}

warn()
{
    echo -e "${YELLOW}[WARN]${NC} $1"
}

error()
{
    echo -e "${RED}[ERROR]${NC} $1"
}

# ============================================================
# CLEANUP
# ============================================================

cleanup()
{
    echo
    echo "=============================================="
    echo "Stopping Vehicle Suspension System..."
    echo "=============================================="

    if [[ -n "${QEMU_PID:-}" ]]; then
        kill "$QEMU_PID" 2>/dev/null || true
    fi

    if [[ -n "${BRIDGE_PID:-}" ]]; then
        kill "$BRIDGE_PID" 2>/dev/null || true
    fi

    if [[ -n "${BACKEND_PID:-}" ]]; then
        kill "$BACKEND_PID" 2>/dev/null || true
    fi

    if [[ -n "${FRONTEND_PID:-}" ]]; then
        kill "$FRONTEND_PID" 2>/dev/null || true
    fi

    if [[ -n "${MONITOR_PID:-}" ]]; then
        kill "$MONITOR_PID" 2>/dev/null || true
    fi

    echo
    success "Application processes stopped."
    echo

    exit 0
}

trap cleanup INT TERM

# ============================================================
# HEADER
# ============================================================

clear

echo "=============================================="
echo "  VEHICLE SUSPENSION SYSTEM"
echo "  Full Stack Launcher"
echo "=============================================="
echo

cd "$PROJECT_DIR"

# ============================================================
# CHECK COMMANDS
# ============================================================

log "Checking required commands..."

COMMANDS=(
    mosquitto
    mosquitto_sub
    qemu-system-arm
    python3
    mvn
    npm
)

for CMD in "${COMMANDS[@]}"
do
    if ! command -v "$CMD" >/dev/null 2>&1
    then
        error "$CMD not found."
        exit 1
    fi
done

success "All required commands found."

# ============================================================
# CHECK PROJECT FILES
# ============================================================

echo

log "Checking project structure..."

if [[ ! -f "$BUILD_DIR/qemu_stm32.elf" ]]
then
    error "QEMU firmware not found:"
    echo "  $BUILD_DIR/qemu_stm32.elf"
    echo
    echo "Build it first:"
    echo "  make"
    exit 1
fi

if [[ ! -f "$TOOLS_DIR/qemu_mqtt_bridge.py" ]]
then
    error "MQTT bridge not found:"
    echo "  $TOOLS_DIR/qemu_mqtt_bridge.py"
    exit 1
fi

if [[ ! -d "$BACKEND_DIR" ]]
then
    error "Backend directory not found:"
    echo "  $BACKEND_DIR"
    exit 1
fi

if [[ ! -d "$FRONTEND_DIR" ]]
then
    error "Frontend directory not found:"
    echo "  $FRONTEND_DIR"
    exit 1
fi

success "Project structure OK."

# ============================================================
# 1. MOSQUITTO
# ============================================================

echo
echo "=============================================="
echo "[1/5] Starting Mosquitto MQTT broker..."
echo "=============================================="

if pgrep -x mosquitto >/dev/null 2>&1
then
    success "Mosquitto already running."
else
    mosquitto > "$LOG_DIR/mosquitto.log" 2>&1 &

    MQTT_PID=$!

    sleep 2

    if kill -0 "$MQTT_PID" 2>/dev/null
    then
        success "Mosquitto started."
    else
        error "Mosquitto failed to start."
        cat "$LOG_DIR/mosquitto.log"
        exit 1
    fi
fi

# ============================================================
# VERIFY MQTT PORT 1883
# ============================================================

if ss -lnt 2>/dev/null | grep -q ':1883'
then
    success "MQTT broker listening on port 1883."
else
    warn "Could not verify port 1883 with ss."
fi

# ============================================================
# 2. MQTT MONITOR
# ============================================================

echo
echo "=============================================="
echo "[2/5] Starting MQTT monitor..."
echo "=============================================="

mosquitto_sub \
    -h localhost \
    -p 1883 \
    -t "vehicle/suspension/telemetry" \
    -v \
    > "$LOG_DIR/mqtt.log" 2>&1 &

MONITOR_PID=$!

success "MQTT monitor started."
echo "      Log: $LOG_DIR/mqtt.log"

# ============================================================
# 3. SPRING BOOT BACKEND
# ============================================================

echo
echo "=============================================="
echo "[3/5] Starting Spring Boot backend..."
echo "=============================================="

cd "$BACKEND_DIR"

mvn spring-boot:run \
    > "$LOG_DIR/backend.log" 2>&1 &

BACKEND_PID=$!

cd "$PROJECT_DIR"

success "Spring Boot starting..."
echo "      Log: $LOG_DIR/backend.log"
echo "      HTTP: http://localhost:8080"

# ============================================================
# WAIT FOR BACKEND
# ============================================================

log "Waiting for backend..."

BACKEND_READY=0

for i in {1..30}
do
    if curl -s http://localhost:8080 >/dev/null 2>&1
    then
        BACKEND_READY=1
        break
    fi

    sleep 1
done

if [[ "$BACKEND_READY" == "1" ]]
then
    success "Spring Boot backend is running."
else
    warn "Backend is still starting."
    echo "      Check: $LOG_DIR/backend.log"
fi

# ============================================================
# 4. QEMU + MQTT BRIDGE
# ============================================================

echo
echo "=============================================="
echo "[4/5] Starting QEMU + MQTT bridge..."
echo "=============================================="

(
    cd "$PROJECT_DIR"

    qemu-system-arm \
        -M olimex-stm32-h405 \
        -kernel "$BUILD_DIR/qemu_stm32.elf" \
        -nographic \
        -monitor none \
        -serial stdio \
    | python3 "$TOOLS_DIR/qemu_mqtt_bridge.py"
) > "$LOG_DIR/qemu_mqtt.log" 2>&1 &

QEMU_PID=$!

sleep 2

if kill -0 "$QEMU_PID" 2>/dev/null
then
    success "QEMU + MQTT bridge started."
else
    error "QEMU failed to start."
    echo "Check:"
    echo "  $LOG_DIR/qemu_mqtt.log"
fi

# ============================================================
# 5. REACT DASHBOARD
# ============================================================

echo
echo "=============================================="
echo "[5/5] Starting React dashboard..."
echo "=============================================="

cd "$FRONTEND_DIR"

npm run dev \
    > "$LOG_DIR/frontend.log" 2>&1 &

FRONTEND_PID=$!

cd "$PROJECT_DIR"

sleep 3

success "React dashboard started."
echo "      Log: $LOG_DIR/frontend.log"
echo "      URL: http://localhost:5173"

# ============================================================
# FINAL STATUS
# ============================================================

echo
echo "=============================================="
echo "  ALL COMPONENTS STARTED"
echo "=============================================="
echo

echo -e "${GREEN}MQTT${NC}"
echo "  Broker : localhost:1883"
echo "  Topic  : vehicle/suspension/telemetry"
echo

echo -e "${GREEN}BACKEND${NC}"
echo "  HTTP   : http://localhost:8080"
echo "  WS     : ws://localhost:8080/ws"
echo

echo -e "${GREEN}FRONTEND${NC}"
echo "  URL    : http://localhost:5173"
echo

echo -e "${GREEN}QEMU${NC}"
echo "  Firmware: build/qemu_stm32.elf"
echo

echo "=============================================="
echo "  LOG FILES"
echo "=============================================="
echo
echo "  MQTT    : logs/mqtt.log"
echo "  Backend : logs/backend.log"
echo "  QEMU    : logs/qemu_mqtt.log"
echo "  Frontend: logs/frontend.log"
echo

echo "=============================================="
echo "  LIVE MQTT TEST"
echo "=============================================="
echo
echo '  mosquitto_sub -h localhost -p 1883 -t "vehicle/suspension/telemetry" -v'
echo

echo "=============================================="
echo "  DASHBOARD"
echo "=============================================="
echo
echo "  http://localhost:5173"
echo

echo "Press CTRL+C to stop the application."
echo

# ============================================================
# KEEP SCRIPT ALIVE
# ============================================================

while true
do
    sleep 2

    # Detect unexpected QEMU exit
    if [[ -n "${QEMU_PID:-}" ]] &&
       ! kill -0 "$QEMU_PID" 2>/dev/null
    then
        warn "QEMU process stopped."
        echo "Check: logs/qemu_mqtt.log"
        QEMU_PID=""
    fi

    # Detect unexpected backend exit
    if [[ -n "${BACKEND_PID:-}" ]] &&
       ! kill -0 "$BACKEND_PID" 2>/dev/null
    then
        warn "Spring Boot process stopped."
        echo "Check: logs/backend.log"
        BACKEND_PID=""
    fi

    # Detect unexpected frontend exit
    if [[ -n "${FRONTEND_PID:-}" ]] &&
       ! kill -0 "$FRONTEND_PID" 2>/dev/null
    then
        warn "React process stopped."
        echo "Check: logs/frontend.log"
        FRONTEND_PID=""
    fi
done
