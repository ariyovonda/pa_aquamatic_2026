import React from "react";
import { useNavigate } from "react-router-dom";
import "./HomePage.css";

export default function HomePage() {
  const nav = useNavigate();
  return (
    <div className="home-page">
      <div className="home-card">
        <h1>AquaMonitor</h1>
        <p>Welcome — monitor your aquaponics system live.</p>
        <div className="home-actions">
          <button className="btn-primary" onClick={() => nav("/login")}>
            Sign in
          </button>
          <button
            className="btn-outline"
            onClick={() => nav("/login?mode=signup")}
          >
            Sign up
          </button>
        </div>
      </div>
    </div>
  );
}
