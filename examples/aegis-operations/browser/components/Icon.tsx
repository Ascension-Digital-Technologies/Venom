// @venom: browser
import React from "../jsx-runtime";
interface IconProps { name:string; }
export default function Icon({name}:IconProps){ const glyphs={ overview:"⌂", incidents:"!", assets:"◇", reports:"▤", settings:"⚙", search:"⌕", shield:"◆", pulse:"∿", command:"⌘", close:"×" }; return <span className="icon" aria-hidden="true">{glyphs[name]||"•"}</span>; }
