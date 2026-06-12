import React, { useState, useRef, useEffect } from "react";
import { useApp } from "../context/AppContext";
import { useNavigate } from "react-router-dom";
import "./Topbar.css";

export default function Topbar() {
  const {
    notifications,
    unreadCount,
    markAllRead,
    user,
    userProfile,
    selectedFarm,
    selectFarm,
    signOut,
  } = useApp();
  const [showNotif, setShowNotif] = useState(false);
  const [showProfile, setShowProfile] = useState(false);
  const ref = useRef(null);
  const navigate = useNavigate();

  useEffect(() => {
    const handler = (e) => {
      if (ref.current && !ref.current.contains(e.target)) {
        setShowNotif(false);
        setShowProfile(false);
      }
    };
    document.addEventListener("mousedown", handler);
    return () => document.removeEventListener("mousedown", handler);
  }, []);

  const now = new Date();
  const timeStr = now.toLocaleTimeString("en-US", {
    hour: "2-digit",
    minute: "2-digit",
  });
  const dateStr = now.toLocaleDateString("en-US", {
    weekday: "long",
    day: "numeric",
    month: "long",
    year: "numeric",
  });

  return (
    <header className="topbar">
      <div className="topbar-left">
        <div className="topbar-date">
          <span className="topbar-time">{timeStr}</span>
          <span className="topbar-dateval">{dateStr}</span>
        </div>
      </div>
      <div className="topbar-right" ref={ref}>
        {/* Notification Bell */}
        <div className="notif-wrap">
          <button
            className={`notif-btn ${unreadCount > 0 ? "has-notif" : ""}`}
            onClick={() => {
              setShowNotif((v) => !v);
              if (!showNotif) markAllRead();
            }}
          >
            <svg
              width="18"
              height="18"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            >
              <path d="M18 8A6 6 0 0 0 6 8c0 7-3 9-3 9h18s-3-2-3-9" />
              <path d="M13.73 21a2 2 0 0 1-3.46 0" />
            </svg>
            {unreadCount > 0 && (
              <span className="notif-count">
                {unreadCount > 9 ? "9+" : unreadCount}
              </span>
            )}
          </button>

          {showNotif && (
            <div className="notif-panel">
              <div className="notif-header">
                <span className="notif-title">System Notifications</span>
                {unreadCount > 0 && (
                  <span className="notif-badge">{unreadCount}</span>
                )}
              </div>
              <div className="notif-list">
                {notifications.length === 0 ? (
                  <div className="notif-empty">No notifications</div>
                ) : (
                  notifications.slice(0, 20).map((n) => (
                    <div
                      key={n.id}
                      className={`notif-item ${n.type} ${!n.read ? "unread" : ""}`}
                    >
                      <div className="notif-icon-wrap">
                        {n.type === "success" ? (
                          <svg
                            width="14"
                            height="14"
                            viewBox="0 0 24 24"
                            fill="none"
                            stroke="currentColor"
                            strokeWidth="2.5"
                          >
                            <polyline points="20 6 9 17 4 12" />
                          </svg>
                        ) : n.type === "warning" ? (
                          <svg
                            width="14"
                            height="14"
                            viewBox="0 0 24 24"
                            fill="none"
                            stroke="currentColor"
                            strokeWidth="2.5"
                          >
                            <path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z" />
                            <line x1="12" y1="9" x2="12" y2="13" />
                            <line x1="12" y1="17" x2="12.01" y2="17" />
                          </svg>
                        ) : (
                          <svg
                            width="14"
                            height="14"
                            viewBox="0 0 24 24"
                            fill="none"
                            stroke="currentColor"
                            strokeWidth="2.5"
                          >
                            <circle cx="12" cy="12" r="10" />
                            <line x1="12" y1="8" x2="12" y2="12" />
                            <line x1="12" y1="16" x2="12.01" y2="16" />
                          </svg>
                        )}
                      </div>
                      <div className="notif-content">
                        <div className="notif-item-title">{n.title}</div>
                        <div className="notif-item-msg">{n.message}</div>
                        <div className="notif-item-time">{n.time}</div>
                      </div>
                    </div>
                  ))
                )}
              </div>
            </div>
          )}
        </div>

        {/* User Avatar / Profile */}
        <div className="user-avatar">
          {user ? (
            <>
              <button
                className="avatar-btn"
                onClick={() => setShowProfile((value) => !value)}
                title="Open profile"
              >
                <span>
                  {user.email ? user.email.charAt(0).toUpperCase() : "U"}
                </span>
              </button>
              {showProfile && (
                <div className="profile-panel">
                  <div className="profile-header">
                    <div className="profile-title">User Profile</div>
                    <div className="profile-subtitle">
                      {user.email || "No email"}
                    </div>
                  </div>
                  <div className="profile-section">
                    <div className="profile-label">Selected Ecosystem</div>
                    <div className="profile-value">
                      {selectedFarm?.name || "Belum memilih ecosystem"}
                    </div>
                  </div>
                  {userProfile?.farms?.length > 1 && (
                    <div className="profile-section">
                      <div className="profile-label">
                        Pilih Ecosystem Aquamatic
                      </div>
                      <div className="farm-list">
                        {userProfile.farms.map((farm) => (
                          <button
                            key={farm.id}
                            className={`farm-item ${
                              farm.id === selectedFarm?.id ? "active" : ""
                            }`}
                            onClick={() => selectFarm(farm.id)}
                            type="button"
                          >
                            {farm.name}
                          </button>
                        ))}
                      </div>
                    </div>
                  )}
                  <button
                    className="profile-logout"
                    type="button"
                    onClick={async () => {
                      const confirm = window.confirm(
                        "Apakah Anda yakin ingin logout dari akun ini?",
                      );
                      if (!confirm) return;
                      try {
                        await signOut();
                        navigate("/");
                      } catch (error) {
                        console.warn("Logout gagal", error);
                      }
                    }}
                  >
                    Logout
                  </button>
                </div>
              )}
            </>
          ) : (
            <span>A</span>
          )}
        </div>
      </div>
    </header>
  );
}
