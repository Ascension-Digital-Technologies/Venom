const services = ["Identity Cloud", "Payment Rail", "Telemetry Mesh", "Edge Gateway", "Data Lake", "Customer API", "Policy Engine"];
const regions = ["US East", "US West", "EU Central", "Asia Pacific"];
const owners = ["Maya Chen", "Noah Patel", "Avery Brooks", "Liam Rivera", "Sofia Kim", "Ethan Wright"];
function seeded(index, salt) { const value = Math.sin(index * 91.17 + salt * 17.31) * 10000; return value - Math.floor(value); }
export function createSnapshot() {
    const incidents = [];
    for (let index = 0; index < 64; index++) {
        const severity = 1 + Math.floor(seeded(index, 3) * 10);
        incidents.push({ id: `INC-${String(8400 + index)}`, title: ["Unusual authentication velocity", "Latency threshold exceeded", "Policy drift detected", "Elevated error concentration", "Unexpected data egress", "Certificate rotation delayed"][index % 6], service: services[index % services.length], region: regions[index % regions.length], severity, confidence: Math.round(55 + seeded(index, 7) * 44), ageMinutes: Math.round(2 + seeded(index, 11) * 420), status: index % 5 === 0 ? "Investigating" : index % 3 === 0 ? "Contained" : "Open", owner: owners[index % owners.length], signals: Math.round(2 + seeded(index, 13) * 18) });
    }
    const assets = [];
    for (let index = 0; index < 38; index++)
        assets.push({ id: `AST-${String(2100 + index)}`, name: `${services[index % services.length]} ${index + 1}`, category: ["Application", "Database", "Gateway", "Worker", "Storage"][index % 5], region: regions[index % regions.length], utilization: Math.round(28 + seeded(index, 17) * 69), health: Math.round(61 + seeded(index, 19) * 38), cost: Math.round(1400 + seeded(index, 23) * 28600), alerts: Math.round(seeded(index, 29) * 8) });
    const activity = [];
    for (let index = 0; index < 24; index++)
        activity.push({ id: `ACT-${index}`, time: `${String(9 + Math.floor(index / 6)).padStart(2, "0")}:${String((index * 11) % 60).padStart(2, "0")}`, actor: owners[index % owners.length], action: ["acknowledged", "assigned", "contained", "reviewed", "escalated", "resolved"][index % 6], target: incidents[index].id, tone: index % 6 === 5 ? "success" : index % 4 === 0 ? "warning" : "neutral" });
    return { incidents, assets, activity };
}
