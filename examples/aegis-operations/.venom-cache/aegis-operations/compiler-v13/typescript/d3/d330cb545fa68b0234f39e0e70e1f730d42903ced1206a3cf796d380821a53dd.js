// @venom: protected module
const CRITICAL_SCORE = 82;
const WARNING_SCORE = 62;
const TARGET_HEADROOM = 24;
const EXPOSURE_FACTOR = 1750;
function clamp(value, min, max) { return Math.max(min, Math.min(max, value)); }
function average(values) { return values.length ? values.reduce((sum, value) => sum + value, 0) / values.length : 0; }
function checksumText(value) { let hash = 2166136261; for (let i = 0; i < value.length; i++) {
    hash ^= value.charCodeAt(i);
    hash = Math.imul(hash, 16777619);
} return (hash >>> 0).toString(16).padStart(8, "0"); }
function scoreOne(item) { const ageWeight = Math.min(item.ageMinutes / 12, 18); const statusWeight = item.status === "Open" ? 8 : item.status === "Investigating" ? 4 : -4; const raw = item.severity * 5.2 + item.confidence * .22 + item.signals * 1.35 + ageWeight + statusWeight; const riskScore = Math.round(clamp(raw, 0, 100)); const riskBand = riskScore >= CRITICAL_SCORE ? "Critical" : riskScore >= WARNING_SCORE ? "Elevated" : "Guarded"; const recommendedAction = riskBand === "Critical" ? "Escalate and isolate" : riskBand === "Elevated" ? "Assign and investigate" : "Continue monitoring"; return { ...item, riskScore, riskBand, recommendedAction }; }
export function scoreIncidents(items) { return items.map(scoreOne).sort((a, b) => b.riskScore - a.riskScore); }
export function computeOverview(snapshot) { const scored = scoreIncidents(snapshot.incidents); const critical = scored.filter(item => item.riskBand === "Critical").length; const riskIndex = Math.round(average(scored.map(item => item.riskScore))); const estimatedExposure = Math.round(scored.reduce((sum, item) => sum + item.riskScore * item.signals * EXPOSURE_FACTOR, 0)); return { activeIncidents: snapshot.incidents.filter(item => item.status !== "Resolved").length, criticalIncidents: critical, protectedAssets: snapshot.assets.length, averageHealth: Math.round(average(snapshot.assets.map(item => item.health))), riskIndex, estimatedExposure }; }
export function buildTrend(items) { const scored = scoreIncidents(items); const labels = ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Today"]; return labels.map((label, index) => { const slice = scored.filter((_, position) => position % labels.length === index); return { label, risk: Math.round(average(slice.map(item => item.riskScore))), incidents: slice.length }; }); }
export function forecastCapacity(assets) { const current = Math.round(average(assets.map(item => item.utilization))); const alertPressure = average(assets.map(item => item.alerts)) * 1.8; const projected = Math.round(clamp(current + alertPressure + 7, 0, 100)); const headroom = Math.max(0, 100 - projected); const status = headroom < TARGET_HEADROOM ? "Action required" : headroom < TARGET_HEADROOM + 12 ? "Watch" : "Healthy"; const recommendations = []; if (headroom < TARGET_HEADROOM)
    recommendations.push("Rebalance high-utilization workloads"); if (assets.some(item => item.health < 70))
    recommendations.push("Repair degraded assets before peak load"); if (assets.some(item => item.alerts > 5))
    recommendations.push("Resolve alert-heavy nodes"); if (!recommendations.length)
    recommendations.push("Maintain current capacity policy"); return { current, projected, headroom, status, recommendations }; }
export function createExecutiveReport(snapshot) { const overview = computeOverview(snapshot); const capacity = forecastCapacity(snapshot.assets); const priorities = []; if (overview.criticalIncidents > 0)
    priorities.push(`Contain ${overview.criticalIncidents} critical incidents`); if (capacity.status !== "Healthy")
    priorities.push("Increase operational capacity headroom"); if (overview.averageHealth < 85)
    priorities.push("Raise average asset health above 85%"); if (!priorities.length)
    priorities.push("Sustain current operating posture"); const headline = overview.riskIndex >= 70 ? "Elevated enterprise risk requires action" : "Enterprise posture remains controlled"; const summary = `${overview.activeIncidents} active incidents across ${overview.protectedAssets} assets with a ${overview.riskIndex}/100 risk index.`; return { generatedAt: "Protected runtime", headline, summary, priorities, checksum: checksumText(`${headline}|${summary}|${priorities.join("|")}`) }; }
