import React from 'react';
import { NavLink } from 'react-router-dom';
import { useApp } from '../../context/AppContext';
import './Sidebar.css';

const navItems = [
  {
    path: '/dashboard', label: 'Dashboard',
    icon: <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><rect x="3" y="3" width="7" height="7" rx="1" /><rect x="14" y="3" width="7" height="7" rx="1" /><rect x="3" y="14" width="7" height="7" rx="1" /><rect x="14" y="14" width="7" height="7" rx="1" /></svg>,
  },
  {
    path: '/history', label: 'History',
    icon: <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12" /></svg>,
  },
  {
    path: '/actuator', label: 'Actuators',
    icon: <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><circle cx="12" cy="12" r="3" /><path d="M19.07 4.93a10 10 0 0 1 0 14.14" /><path d="M4.93 4.93a10 10 0 0 0 0 14.14" /></svg>,
  },
  {
    path: '/settings', label: 'Settings',
    icon: <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><circle cx="12" cy="12" r="3" /><path d="M12 2v2M12 20v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M2 12h2M20 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42" /></svg>,
  },
];

export default function Sidebar() {
  const { unreadCount, firebaseConnected } = useApp();

  return (
    <aside className="sidebar">
      {/* Logo */}
      <div className="sidebar-logo">
        <div className="logo-icon">
          <svg width="22" height="22" viewBox="0 0 24 24" fill="none">
            <path d="M12 2C8 2 4 6 4 10c0 5 8 12 8 12s8-7 8-12c0-4-4-8-8-8z" fill="#0d9488" opacity="0.15" />
            <path d="M12 2C8 2 4 6 4 10c0 5 8 12 8 12s8-7 8-12c0-4-4-8-8-8z" stroke="#0d9488" strokeWidth="1.5" fill="none" />
            <circle cx="12" cy="10" r="3" fill="#0d9488" />
            <path d="M8 16c1 1 2.5 2 4 2s3-1 4-2" stroke="#0d9488" strokeWidth="1.5" strokeLinecap="round" fill="none" />
          </svg>
        </div>
        <div className="logo-text">
          <span className="logo-name">AquaMonitor</span>
          <span className="logo-sub">Aquaponics IoT</span>
        </div>
      </div>

      {/* DB Status */}
      <div className="mqtt-status">
        <div className={`status-dot ${firebaseConnected ? 'connected' : 'disconnected'}`} />
        <span>{firebaseConnected ? 'Firebase Live' : 'Simulation Mode'}</span>
      </div>

      {/* Nav */}
      <nav className="sidebar-nav">
        <div className="nav-section-label">Main Menu</div>
        {navItems.map(item => (
          <NavLink
            key={item.path}
            to={item.path}
            className={({ isActive }) => `nav-item ${isActive ? 'active' : ''}`}
          >
            <span className="nav-icon">{item.icon}</span>
            <span className="nav-label">{item.label}</span>
            {item.path === '/dashboard' && unreadCount > 0 && (
              <span className="nav-badge">{unreadCount}</span>
            )}
          </NavLink>
        ))}
      </nav>

      {/* Firebase Info */}
      <div className="sidebar-footer">
        <div className="firebase-badge">
          <svg width="12" height="12" viewBox="0 0 24 24" fill="#f97316"><path d="M3.89 15.67L6.6 3.5l5.3 9.19-8.01 2.98zm16.22-.01l-2.71-12.17-5.3 9.19 8.01 2.98zM12 21.5l-8.11-5.83 8.11-3.01 8.11 3.01L12 21.5z" /></svg>
          <span>Firebase Firestore + RTDB</span>
        </div>
        <div className="system-info">
          <div className="info-row"><span className="info-label">Sensors</span><span className="info-val">5 Units</span></div>
          <div className="info-row"><span className="info-label">Actuators</span><span className="info-val">4 Units</span></div>
        </div>
        <div className="version-tag">v2.0.0 – Firebase Edition</div>
      </div>
    </aside>
  );
}
