// @venom: browser
export interface Incident { id:string; title:string; service:string; region:string; severity:number; confidence:number; ageMinutes:number; status:string; owner:string; signals:number; }
export interface Asset { id:string; name:string; category:string; region:string; utilization:number; health:number; cost:number; alerts:number; }
export interface Activity { id:string; time:string; actor:string; action:string; target:string; tone:string; }
export interface Snapshot { incidents:Incident[]; assets:Asset[]; activity:Activity[]; }
export interface OverviewMetrics { activeIncidents:number; criticalIncidents:number; protectedAssets:number; averageHealth:number; riskIndex:number; estimatedExposure:number; }
export interface ScoredIncident extends Incident { riskScore:number; riskBand:string; recommendedAction:string; }
export interface TrendPoint { label:string; risk:number; incidents:number; }
export interface CapacityForecast { current:number; projected:number; headroom:number; status:string; recommendations:string[]; }
export interface ExecutiveReport { generatedAt:string; headline:string; summary:string; priorities:string[]; checksum:string; }
export type ViewName = "overview" | "incidents" | "assets" | "reports" | "settings";
