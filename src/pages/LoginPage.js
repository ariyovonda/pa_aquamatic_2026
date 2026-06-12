import React, { useState, useEffect } from "react";
import { useNavigate, useLocation } from "react-router-dom";
import { signIn, signUp } from "../firebase/authService";
import "./LoginPage.css";
import { useApp } from "../context/AppContext";

export default function LoginPage() {
  const [mode, setMode] = useState("login"); // 'login' | 'signup'
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const navigate = useNavigate();
  const location = useLocation();
  const { user } = useApp();

  useEffect(() => {
    const params = new URLSearchParams(location.search);
    const m = params.get("mode");
    if (m === "signup") setMode("signup");
  }, [location.search]);

  useEffect(() => {
    if (user) navigate("/dashboard", { replace: true });
  }, [user, navigate]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    setError("");
    try {
      if (mode === "login") {
        await signIn(email, password);
      } else {
        await signUp(email, password);
      }
      navigate("/dashboard", { replace: true });
    } catch (err) {
      setError(err.message || "Authentication failed");
    } finally {
      setLoading(false);
    }
  };

  return (
    <div className="auth-page">
      <div className="auth-card">
        <h2>{mode === "login" ? "Sign in" : "Create account"}</h2>
        {error && <div className="auth-error">{error}</div>}
        <form onSubmit={handleSubmit} className="auth-form">
          <label>Email</label>
          <input
            type="email"
            value={email}
            onChange={(e) => setEmail(e.target.value)}
            required
          />
          <label>Password</label>
          <input
            type="password"
            value={password}
            onChange={(e) => setPassword(e.target.value)}
            required
            minLength={6}
          />
          <div className="auth-actions">
            <button type="submit" className="btn-primary" disabled={loading}>
              {loading
                ? "Please wait…"
                : mode === "login"
                  ? "Sign in"
                  : "Sign up"}
            </button>
            <button
              type="button"
              className="btn-link"
              onClick={() => setMode(mode === "login" ? "signup" : "login")}
            >
              {mode === "login"
                ? "Create an account"
                : "Have an account? Sign in"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
