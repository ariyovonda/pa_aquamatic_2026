import React, {
  createContext,
  useContext,
  useRef,
  useState,
  useEffect,
} from "react";
import { useApp } from "./AppContext";

const MqttContext = createContext(null);

export function MqttProvider({ children }) {
  const clientRef = useRef(null);
  const [status, setStatus] = useState("disconnected");
  const { setMqttConnected } = useApp();

  // Aktifkan koneksi MQTT nyata — paket `mqtt` sudah ada di dependencies
  useEffect(() => {
    let client;
    try {
      const mqtt = require("mqtt");

      // Determine broker URL: prefer REACT_APP_MQTT_BROKER, fallback to MQTT_BROKER
      const envWs = process.env.REACT_APP_MQTT_BROKER;
      const envMq = process.env.MQTT_BROKER;
      let brokerUrl =
        envWs || (envMq ? `wss://${envMq}` : "ws://localhost:9001");

      const clientId =
        "react-aquamonitor-" + Math.random().toString(16).slice(2, 8);
      client = mqtt.connect(brokerUrl, { clientId, reconnectPeriod: 5000 });
      clientRef.current = client;

      client.on("connect", () => {
        setStatus("connected");
        if (typeof setMqttConnected === "function") setMqttConnected(true);
        const topicPrefix =
          process.env.REACT_APP_MQTT_TOPIC_PREFIX || "aquaponics";
        client.subscribe(`${topicPrefix}/sensors/#`, { qos: 0 });
      });

      // Node-RED adalah satu-satunya penulis sensor dan history RTDB.
      // Browser cukup berlangganan RTDB melalui AppContext agar tidak membuat
      // duplikasi history atau interval sampling yang tidak konsisten.

      client.on("error", (err) => {
        console.warn("[MQTT] error", err && err.message);
      });

      client.on("close", () => {
        setStatus("disconnected");
        if (typeof setMqttConnected === "function") setMqttConnected(false);
      });
    } catch (e) {
      console.warn(
        "[MQTT] mqtt package not available or failed to connect, running in simulate mode",
      );
    }

    return () => {
      if (clientRef.current) clientRef.current.end(true);
    };
  }, []);

  const publish = (subtopic, payload) => {
    if (clientRef.current?.connected) {
      clientRef.current.publish(
        `aquaponics/${subtopic}`,
        JSON.stringify(payload),
      );
    } else {
      console.log("[MQTT Simulate] Publish:", subtopic, payload);
    }
  };

  return (
    <MqttContext.Provider value={{ status, publish }}>
      {children}
    </MqttContext.Provider>
  );
}

export function useMqtt() {
  return useContext(MqttContext);
}
