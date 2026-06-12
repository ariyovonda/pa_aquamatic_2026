import { auth } from "./config";
import {
  createUserWithEmailAndPassword,
  signInWithEmailAndPassword,
  signOut as fbSignOut,
  onAuthStateChanged,
} from "firebase/auth";
import { createUserProfile } from "./firebaseService";

export async function signUp(email, password) {
  const credential = await createUserWithEmailAndPassword(
    auth,
    email,
    password,
  );
  try {
    await createUserProfile(
      credential.user.uid,
      credential.user.email || email,
    );
  } catch (err) {
    console.warn("[Auth] createUserProfile failed", err);
  }
  return credential;
}

export async function signIn(email, password) {
  return signInWithEmailAndPassword(auth, email, password);
}

export async function signOut() {
  return fbSignOut(auth);
}

export function observeAuth(cb) {
  return onAuthStateChanged(auth, cb);
}

export default { signUp, signIn, signOut, observeAuth };
