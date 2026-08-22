import { Client } from "@stomp/stompjs";

const client = new Client({
    brokerURL: "ws://localhost:8080/ws",

    reconnectDelay: 5000,

    debug: (str) => {
        console.log("[STOMP]", str);
    },

    onConnect: () => {
        console.log("✅ WebSocket connected");

        client.subscribe("/topic/telemetry", (message) => {
            try {
                const telemetry = JSON.parse(message.body);

                console.log("📡 Telemetry received:", telemetry);

                window.dispatchEvent(
                    new CustomEvent("telemetry", {
                        detail: telemetry,
                    })
                );
            } catch (error) {
                console.error(
                    "❌ Invalid telemetry message:",
                    error
                );
            }
        });
    },

    onWebSocketError: (error) => {
        console.error("❌ WebSocket error:", error);
    },

    onStompError: (frame) => {
        console.error(
            "❌ STOMP error:",
            frame.headers["message"]
        );

        console.error(
            "Details:",
            frame.body
        );
    },

    onDisconnect: () => {
        console.log("🔌 WebSocket disconnected");
    },
});

export function connectWebSocket() {
    if (!client.active) {
        console.log("🔄 Connecting to WebSocket...");
        client.activate();
    }
}

export function disconnectWebSocket() {
    if (client.active) {
        client.deactivate();
    }
}