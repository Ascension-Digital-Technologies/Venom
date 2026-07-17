// @venom: browser
import type { ScoredIncident } from "../data/types";
export function filterIncidents(items:ScoredIncident[], query:string, region:string):ScoredIncident[] { const needle=query.trim().toLowerCase(); return items.filter(item => (region==="All regions" || item.region===region) && (!needle || `${item.id} ${item.title} ${item.service} ${item.owner}`.toLowerCase().includes(needle))); }
export function sortByRisk(items:ScoredIncident[]):ScoredIncident[] { return [...items].sort((a,b)=>b.riskScore-a.riskScore); }
