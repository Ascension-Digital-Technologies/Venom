export function filterIncidents(items, query, region) { const needle = query.trim().toLowerCase(); return items.filter(item => (region === "All regions" || item.region === region) && (!needle || `${item.id} ${item.title} ${item.service} ${item.owner}`.toLowerCase().includes(needle))); }
export function sortByRisk(items) { return [...items].sort((a, b) => b.riskScore - a.riskScore); }
