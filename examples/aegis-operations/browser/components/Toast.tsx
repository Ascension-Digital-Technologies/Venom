// @venom: browser
import React from "../jsx-runtime";
interface ToastProps { message: string; }
export default function Toast({ message }: ToastProps) {
  if (!message) return null;
  return <div className="toast"><span>✓</span>{message}</div>;
}
