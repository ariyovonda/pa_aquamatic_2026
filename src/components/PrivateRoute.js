import React from "react";
import { Navigate } from "react-router-dom";
import { useApp } from "../context/AppContext";

export default function PrivateRoute({ children }) {
  const { user } = useApp();
  if (user) return children;
  return <Navigate to="/login" replace />;
}
