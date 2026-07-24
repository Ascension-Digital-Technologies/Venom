// @venom: browser
import React, { render } from "./jsx-runtime";
import Sidebar from "./components/Sidebar";
import Header from "./components/Header";
import KpiGrid from "./components/KpiGrid";
import TrendChart from "./components/TrendChart";
import RiskPanel from "./components/RiskPanel";
import CapacityPanel from "./components/CapacityPanel";
import IncidentTable from "./components/IncidentTable";
import AssetGrid from "./components/AssetGrid";
import ActivityFeed from "./components/ActivityFeed";
import ReportsPanel from "./components/ReportsPanel";
import SettingsPanel from "./components/SettingsPanel";
import CommandPalette from "./components/CommandPalette";
import IncidentModal from "./components/IncidentModal";
import Toast from "./components/Toast";
import { createSnapshot } from "./data/mock-data";
import { createState } from "./state/store";
import { filterIncidents, sortByRisk } from "./utils/filter";
import type { ViewName } from "./data/types";
import { computeOverview, scoreIncidents, buildTrend, forecastCapacity, createExecutiveReport } from "../protected/engine";

const root = document.querySelector<HTMLElement>("#app")!;
const state = createState(createSnapshot());
const titles = { overview: "Command Center", incidents: "Incident Intelligence", assets: "Asset Inventory", reports: "Executive Reports", settings: "Platform Settings" };

function currentIncident() { return state.selectedIncident ? state.scored.find(item => item.id === state.selectedIncident) || null : null; }
function visibleIncidents() { return sortByRisk(filterIncidents(state.scored, state.query, state.region)); }
function setToast(message:string){ state.toast=message; draw(); setTimeout(()=>{ if(state.toast===message){ state.toast=""; draw(); } },2400); }
function navigate(view:ViewName){ state.view=view; state.query=""; draw(); }
async function refreshIntelligence(){
  state.loading=true; draw();
  const [overview, scored, trend, forecast, report] = await Promise.all([
    computeOverview(state.snapshot),
    scoreIncidents(state.snapshot.incidents),
    buildTrend(state.snapshot.incidents),
    forecastCapacity(state.snapshot.assets),
    createExecutiveReport(state.snapshot)
  ]);
  state.overview=overview; state.scored=scored; state.trend=trend; state.forecast=forecast; state.report=report; state.loading=false;
  draw(); setToast("Protected intelligence refreshed");
}

function Overview(){ if(!state.overview||!state.forecast)return <div className="loading-panel">Protected models are analyzing the enterprise graph…</div>; const incidents=visibleIncidents(); return <><KpiGrid metrics={state.overview}/><section className="dashboard-grid"><TrendChart points={state.trend}/><RiskPanel incidents={state.scored}/><CapacityPanel forecast={state.forecast}/></section><section className="lower-grid"><IncidentTable incidents={incidents} limit={9} onSelect={id=>{state.selectedIncident=id;draw();}}/><ActivityFeed activity={state.snapshot.activity}/></section></>; }
function Incidents(){ const regions=["All regions",...Array.from(new Set(state.snapshot.incidents.map(item=>item.region)))]; const incidents=visibleIncidents(); return <><section className="toolbar panel"><div><strong>{incidents.length} prioritized incidents</strong><small>Scored by protected TurboJS/WASM analytics</small></div><div className="toolbar-controls"><select onChange={(event)=>{state.region=event.target.value;draw();}}>{regions.map(region=><option selected={region===state.region}>{region}</option>)}</select><button className="secondary-button" onClick={()=>setToast("Incident export prepared")}>Export table</button></div></section><IncidentTable incidents={incidents} limit={64} onSelect={id=>{state.selectedIncident=id;draw();}}/></>; }
function Assets(){ return <><section className="toolbar panel"><div><strong>{state.snapshot.assets.length} managed assets</strong><small>Live health, utilization, cost, and alert posture</small></div><button className="primary-button" onClick={()=>setToast("Asset inventory synchronized")}>Synchronize inventory</button></section><AssetGrid assets={state.snapshot.assets}/></>; }
function Content(){ if(state.view==="overview")return <Overview/>; if(state.view==="incidents")return <Incidents/>; if(state.view==="assets")return <Assets/>; if(state.view==="reports"&&state.report)return <ReportsPanel report={state.report} onRegenerate={refreshIntelligence}/>; if(state.view==="settings")return <SettingsPanel/>; return <div className="loading-panel">Preparing protected report…</div>; }
function App(){ return <div className="app-shell"><Sidebar view={state.view} onNavigate={navigate}/><main><Header title={titles[state.view]} query={state.query} onQuery={value=>{state.query=value;draw();}} onCommand={()=>{state.commandOpen=true;draw();}} onRefresh={refreshIntelligence} loading={state.loading}/><div className="content"><Content/></div></main><CommandPalette open={state.commandOpen} onClose={()=>{state.commandOpen=false;draw();}} onNavigate={navigate} onRefresh={refreshIntelligence}/><IncidentModal incident={currentIncident()} onClose={()=>{state.selectedIncident=null;draw();}}/><Toast message={state.toast}/></div>; }
function draw(){ render(<App/>,root); }
document.addEventListener("keydown",event=>{ if((event.ctrlKey||event.metaKey)&&event.key.toLowerCase()==="k"){event.preventDefault();state.commandOpen=true;draw();} if(event.key==="Escape"&&(state.commandOpen||state.selectedIncident)){state.commandOpen=false;state.selectedIncident=null;draw();} });
draw();
await refreshIntelligence();
