import React from "react";
import "./SettingsPage.css";

export default function SettingsPage() {
  return (
    <div className="settings-page">
      <div className="settings-header">
        <h1 className="page-title">System Settings</h1>
        <p className="page-subtitle">
          Konfigurasi sistem disimpan di Firebase Realtime Database.
        </p>
      </div>

      <div className="settings-grid">
        <section className="settings-card">
          <div className="settings-card-header">
            <div className="sc-icon-wrap teal">S</div>
            <div>
              <h2 className="sc-title">Sensor</h2>
              <p className="sc-sub">Sensor hanya memiliki kontrol ON/OFF.</p>
            </div>
          </div>
          <p className="settings-note">
            Nyalakan atau matikan sensor dari Dashboard. Statusnya tersimpan pada
            <code> /sensors/&lt;sensorId&gt;/enabled </code> di RTDB.
          </p>
        </section>

        <section className="settings-card">
          <div className="settings-card-header">
            <div className="sc-icon-wrap blue">A</div>
            <div>
              <h2 className="sc-title">Otomasi Aktuator</h2>
              <p className="sc-sub">Threshold dikelola per aktuator.</p>
            </div>
          </div>
          <p className="settings-note">
            Buka halaman Actuators untuk menentukan sensor sumber, kondisi,
            ambang, dan hysteresis. Node-RED akan membaca aturan itu saat
            menjalankan otomasi.
          </p>
        </section>

        <section className="settings-card wide">
          <div className="settings-card-header">
            <div className="sc-icon-wrap orange">DB</div>
            <div>
              <h2 className="sc-title">Struktur Realtime Database</h2>
              <p className="sc-sub">Satu database untuk data live, konfigurasi, dan riwayat.</p>
            </div>
          </div>
          <pre className="schema-code">{`/sensors/{sensorId}
  { value, unit, status, enabled, updatedAt }
/actuators/{actuatorId}
  { enabled, mode, automation: { sensor, condition, value, hysteresis }, updatedAt }
/history/sensors/{sensorId}/{pushId}
  { value, unit, status, timestamp }
/history/actuators/{pushId}
  { device, action, source, timestamp }`}</pre>
        </section>
      </div>
    </div>
  );
}
