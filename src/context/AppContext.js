import React, {
  createContext,
  useContext,
  useState,
  useEffect,
  useCallback,
} from "react";
import {
  subscribeSensors,
  subscribeActuators,
  updateSensorRTDB,
  updateActuatorRTDB,
  requestActuatorCommand,
  getUserProfile,
  createUserProfile,
  updateUserProfile,
} from "../firebase/firebaseService";
import { observeAuth, signOut as authSignOut } from "../firebase/authService";

const AppContext = createContext(null);

const SENSOR_DEFAULTS = {
  temperature: {
    unit: "°C",
    label: "Water Temperature",
    value: 25.7,
    status: "normal",
    enabled: true,
  },
  ph: {
    unit: "pH",
    label: "pH Level",
    value: 6.8,
    status: "normal",
    enabled: true,
  },
  tds: {
    unit: "ppm",
    label: "TDS Level",
    value: 628,
    status: "normal",
    enabled: true,
  },
  do: {
    unit: "mg/L",
    label: "Dissolved Oxygen",
    value: 7.1,
    status: "normal",
    enabled: true,
  },
  turbidity: {
    unit: "NTU",
    label: "Turbidity",
    value: 2.3,
    status: "normal",
    enabled: true,
  },
};

const ACTUATOR_DEFAULTS = {
  waterPump: {
    id: "waterPump",
    label: "Water Pump (Pond → Beds)",
    enabled: true,
    mode: "manual",
  },
  aerator: {
    id: "aerator",
    label: "Aerator (Pond Oxygen)",
    enabled: true,
    mode: "manual",
  },
  heater: {
    id: "heater",
    label: "Heater (Water Heater)",
    enabled: true,
    mode: "auto",
    automation: { sensor: "temperature", condition: "below", value: 25, hysteresis: 0 },
  },
  buzzer: {
    id: "buzzer",
    label: "Buzzer (Alarm)",
    enabled: true,
    mode: "auto",
    automation: { sensor: "tds", condition: "above", value: 500, hysteresis: 0 },
  },
  phPumpDown: {
    id: "phPumpDown",
    label: "pH Pump (-)",
    enabled: true,
    mode: "auto",
    automation: { sensor: "ph", condition: "above", value: 8, hysteresis: 0 },
  },
  phPumpUp: {
    id: "phPumpUp",
    label: "pH Pump (+)",
    enabled: true,
    mode: "auto",
    automation: { sensor: "ph", condition: "below", value: 6, hysteresis: 0 },
  },
  nutritionPump: {
    id: "nutritionPump",
    label: "Nutrition Pump",
    enabled: true,
    mode: "auto",
    automation: { sensor: "tds", condition: "below", value: 200, hysteresis: 0 },
  },
};

export function AppProvider({ children }) {
  const [sensorData, setSensorData] = useState(SENSOR_DEFAULTS);
  const [sensorHealth, setSensorHealth] = useState({
    temperature: false,
    ph: false,
    tds: false,
    do: false,
    turbidity: false,
  });
  const [dataReady, setDataReady] = useState(false);
  const [actuators, setActuators] = useState(ACTUATOR_DEFAULTS);
  const [notifications, setNotifications] = useState([
    {
      id: 1,
      type: "success",
      title: "All Systems Optimal",
      message: "All sensor readings are within optimal range",
      time: new Date().toLocaleTimeString("en-US"),
      read: false,
    },
  ]);
  const [firebaseConnected, setFirebaseConnected] = useState(false);
  const [mqttConnected, setMqttConnected] = useState(false);
  const [user, setUser] = useState(null);
  const [userProfile, setUserProfile] = useState(null);
  const [selectedFarmId, setSelectedFarmId] = useState(null);

  // 2. Subscribe Realtime Database → sensor live
  useEffect(() => {
    let unsub;
    try {
      unsub = subscribeSensors((firebaseData) => {
        const now = Date.now();
        setFirebaseConnected(true);
        setDataReady(true);
        setSensorData((prev) => {
          const next = {};
          Object.entries(SENSOR_DEFAULTS).forEach(([key, base]) => {
            const fbVal = firebaseData?.[key];
            const lastSeen = fbVal?.updatedAt ?? prev[key]?.lastSeen ?? null;
            const connected = !!lastSeen && now - lastSeen < 60000;
            next[key] = {
              ...base,
              value: fbVal?.value ?? null,
              unit: fbVal?.unit ?? base.unit,
              status: connected ? fbVal?.status || "normal" : "offline",
              enabled: fbVal?.enabled ?? prev[key]?.enabled ?? true,
              connected,
              lastSeen,
            };
          });
          return next;
        });
        setSensorHealth((prevHealth) => {
          const nextHealth = {};
          Object.keys(SENSOR_DEFAULTS).forEach((key) => {
            const fbVal = firebaseData?.[key];
            const lastSeen = fbVal?.updatedAt ?? null;
            nextHealth[key] = !!lastSeen && now - lastSeen < 60000;
          });
          return nextHealth;
        });
      }, selectedFarmId);
    } catch (e) {
      console.warn("[Firebase] Belum dikonfigurasi, menunggu data sensor live");
    }
    return () => {
      if (typeof unsub === "function") unsub();
    };
  }, [selectedFarmId]);

  // Periodic check: mark sensors offline if no data for 60s
  useEffect(() => {
    const interval = setInterval(() => {
      setSensorData((prev) => {
        const now = Date.now();
        const next = {};
        let changed = false;
        Object.entries(prev).forEach(([key, val]) => {
          const lastSeen = val.lastSeen ?? null;
          const wasConnected = val.connected ?? true;
          const nowConnected = !!lastSeen && now - lastSeen < 60000;
          if (wasConnected !== nowConnected) changed = true;
          next[key] = {
            ...val,
            status: nowConnected ? val.status || "normal" : "offline",
            connected: nowConnected,
          };
        });
        return changed ? next : prev;
      });
    }, 5000); // Check every 5 seconds
    return () => clearInterval(interval);
  }, []);

  // 3. Subscribe Realtime Database → aktuator
  useEffect(() => {
    let unsub;
    try {
      unsub = subscribeActuators((data) => {
        setActuators((prev) => {
          const next = { ...prev };
          Object.entries(data).forEach(([id, val]) => {
            next[id] = {
              id,
              label: prev[id]?.label || id,
              enabled: prev[id]?.enabled ?? false,
              ...prev[id],
              ...val,
            };
          });
          return next;
        });
      }, selectedFarmId);
    } catch (e) {}
    return () => {
      if (typeof unsub === "function") unsub();
    };
  }, [selectedFarmId]);

  // 5. Observe Firebase Auth state
  useEffect(() => {
    let unsub;
    try {
      unsub = observeAuth((u) => {
        setUser(u);
      });
    } catch (e) {}
    return () => {
      if (typeof unsub === "function") unsub();
    };
  }, []);

  useEffect(() => {
    if (!user) {
      setUserProfile(null);
      setSelectedFarmId(null);
      return;
    }

    let canceled = false;
    async function loadProfile() {
      try {
        let profile = await getUserProfile(user.uid);
        if (!profile) {
          profile = await createUserProfile(user.uid, user.email || "");
        }
        if (canceled) return;
        setUserProfile(profile);
        setSelectedFarmId(
          profile.selectedFarm || profile.farms?.[0]?.id || null,
        );
      } catch (error) {
        console.warn("[Auth] gagal memuat profil user", error);
      }
    }

    loadProfile();
    return () => {
      canceled = true;
    };
  }, [user]);

  const selectFarm = useCallback(
    async (farmId) => {
      setSelectedFarmId(farmId);
      setUserProfile((prev) =>
        prev ? { ...prev, selectedFarm: farmId } : prev,
      );
      if (!user) return;
      try {
        await updateUserProfile(user.uid, { selectedFarm: farmId });
      } catch (error) {
        console.warn("[User] gagal mengganti pond", error);
      }
    },
    [user],
  );

  const selectedFarm =
    userProfile?.farms?.find((f) => f.id === selectedFarmId) || null;

  const addNotification = useCallback((notif) => {
    setNotifications((prev) => {
      const exists = prev.find(
        (n) => n.title === notif.title && Date.now() - (n.ts || 0) < 30000,
      );
      if (exists) return prev;
      return [
        {
          id: Date.now(),
          ts: Date.now(),
          ...notif,
          time: new Date().toLocaleTimeString("id-ID"),
          read: false,
        },
        ...prev,
      ].slice(0, 50);
    });
  }, []);

  const markAllRead = useCallback(
    () => setNotifications((p) => p.map((n) => ({ ...n, read: true }))),
    [],
  );

  const toggleSensor = useCallback(
    async (key) => {
      const enabled = !(sensorData[key]?.enabled ?? true);
      setSensorData((prev) => ({
        ...prev,
        [key]: { ...prev[key], enabled },
      }));
      try {
        await updateSensorRTDB(key, { enabled }, selectedFarmId);
      } catch (error) {
        console.warn("[Sensor] gagal mengubah status sensor", error);
      }
    },
    [sensorData, selectedFarmId],
  );

  const toggleActuator = useCallback(
    async (id) => {
      const newEnabled = !actuators[id].enabled;
      setActuators((prev) => ({
        ...prev,
        [id]: {
          ...prev[id],
          enabled: newEnabled,
        },
      }));
      try {
        await requestActuatorCommand(
          id,
          newEnabled ? "on" : "off",
          selectedFarmId,
        );
      } catch (error) {
        setActuators((prev) => ({
          ...prev,
          [id]: { ...prev[id], enabled: !newEnabled },
        }));
        console.warn("[Actuator] RPC web gagal dikirim", error);
      }
    },
    [actuators, selectedFarmId],
  );

  const updateActuatorAutomation = useCallback(
    async (id, changes) => {
      setActuators((prev) => ({
        ...prev,
        [id]: { ...prev[id], ...changes },
      }));
      try {
        await updateActuatorRTDB(id, changes, selectedFarmId);
      } catch (error) {
        console.warn("[Actuator] gagal menyimpan aturan otomatis", error);
      }
    },
    [selectedFarmId],
  );

  const unreadCount = notifications.filter((n) => !n.read).length;

  return (
    <AppContext.Provider
      value={{
        sensorData,
        toggleSensor,
        actuators,
        toggleActuator,
        updateActuatorAutomation,
        notifications,
        addNotification,
        markAllRead,
        unreadCount,
        mqttConnected,
        setMqttConnected,
        firebaseConnected,
        user,
        userProfile,
        selectedFarm,
        selectedFarmId,
        selectFarm,
        signOut: authSignOut,
      }}
    >
      {children}
    </AppContext.Provider>
  );
}

export function useApp() {
  const ctx = useContext(AppContext);
  if (!ctx) throw new Error("useApp must be used within AppProvider");
  return ctx;
}
