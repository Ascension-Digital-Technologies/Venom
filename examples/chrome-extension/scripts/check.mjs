import { execFileSync } from "node:child_process";
import { readFileSync } from "node:fs";

const files = [
  "background.js",
  "content-script.js",
  "visual-board.js",
  "humanize.js",
  "match-modes.js",
  "skill-levels.js",
  "page-bridge.js",
  "popup.js",
  "engine-host.js",
  "protected/velocity-engine.js"
];

JSON.parse(readFileSync("manifest.json", "utf8"));

for (const file of files) {
  execFileSync(process.execPath, ["--check", file], { stdio: "inherit" });
}

console.log(`Validated manifest.json and ${files.length} JavaScript files.`);
