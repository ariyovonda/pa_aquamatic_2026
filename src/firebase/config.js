// Firebase Authentication + Realtime Database.
import { initializeApp } from "firebase/app";
import { getDatabase } from "firebase/database";
import { getAuth } from "firebase/auth";

const firebaseConfig = {
  apiKey: "AIzaSyDYmk_9OD1AnWP3c-1CjzVrojLEFG2sYnM",
  authDomain: "akuaponik-5bdfa.firebaseapp.com",
  databaseURL: "https://akuaponik-5bdfa-default-rtdb.firebaseio.com",
  projectId: "akuaponik-5bdfa",
  storageBucket: "akuaponik-5bdfa.appspot.com",
  messagingSenderId: "273363250481",
  appId: "1:273363250481:web:00d40c957d358a28a71410",
};

const app = initializeApp(firebaseConfig);
export const rtdb = getDatabase(app);
export const auth = getAuth(app);
export default app;
