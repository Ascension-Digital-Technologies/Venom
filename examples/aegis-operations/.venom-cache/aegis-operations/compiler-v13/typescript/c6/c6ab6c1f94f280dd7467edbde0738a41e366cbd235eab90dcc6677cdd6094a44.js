// @venom: browser
import React from "../jsx-runtime";
export default function Icon({ name }) { const glyphs = { overview: "⌂", incidents: "!", assets: "◇", reports: "▤", settings: "⚙", search: "⌕", shield: "◆", pulse: "∿", command: "⌘", close: "×" }; return React.createElement("span",{"className":"icon","aria-hidden":"true"},(glyphs[name] || "•")); }
