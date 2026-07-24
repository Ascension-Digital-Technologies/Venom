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
import { computeOverview, scoreIncidents, buildTrend, forecastCapacity, createExecutiveReport } from "../protected/engine";
const root = document.querySelector("#app");
const state = createState(createSnapshot());
const titles = { overview: "Command Center", incidents: "Incident Intelligence", assets: "Asset Inventory", reports: "Executive Reports", settings: "Platform Settings" };
function currentIncident() { return state.selectedIncident ? state.scored.find(item => item.id === state.selectedIncident) || null : null; }
function visibleIncidents() { return sortByRisk(filterIncidents(state.scored, state.query, state.region)); }
function setToast(message) { state.toast = message; draw(); setTimeout(() => { if (state.toast === message) {
    state.toast = "";
    draw();
} }, 2400); }
function navigate(view) { state.view = view; state.query = ""; draw(); }
async function refreshIntelligence() {
    state.loading = true;
    draw();
    const [overview, scored, trend, forecast, report] = await Promise.all([
        computeOverview(state.snapshot),
        scoreIncidents(state.snapshot.incidents),
        buildTrend(state.snapshot.incidents),
        forecastCapacity(state.snapshot.assets),
        createExecutiveReport(state.snapshot)
    ]);
    state.overview = overview;
    state.scored = scored;
    state.trend = trend;
    state.forecast = forecast;
    state.report = report;
    state.loading = false;
    draw();
    setToast("Protected intelligence refreshed");
}
function Overview() { if (!state.overview || !state.forecast)
    return React.createElement("div",{"className":"loading-panel"},"Protected models are analyzing the enterprise graph…"); const incidents = visibleIncidents(); return React.createElement(React.Fragment,null,React.createElement(KpiGrid,{"metrics":(state.overview)}),React.createElement("section",{"className":"dashboard-grid"},React.createElement(TrendChart,{"points":(state.trend)}),React.createElement(RiskPanel,{"incidents":(state.scored)}),React.createElement(CapacityPanel,{"forecast":(state.forecast)})),React.createElement("section",{"className":"lower-grid"},React.createElement(IncidentTable,{"incidents":(incidents),"limit":(9),"onSelect":(id => { state.selectedIncident = id; draw(); })}),React.createElement(ActivityFeed,{"activity":(state.snapshot.activity)}))); }
function Incidents() { const regions = ["All regions", ...Array.from(new Set(state.snapshot.incidents.map(item => item.region)))]; const incidents = visibleIncidents(); return React.createElement(React.Fragment,null,React.createElement("section",{"className":"toolbar panel"},React.createElement("div",null,React.createElement("strong",null,(incidents.length),"prioritized incidents"),React.createElement("small",null,"Scored by protected TurboJS/WASM analytics")),React.createElement("div",{"className":"toolbar-controls"},React.createElement("select",{"onChange":((event) => { state.region = event.target.value; draw(); })},(regions.map(region => React.createElement("option",{"selected":(region === state.region)},(region))))),React.createElement("button",{"className":"secondary-button","onClick":(() => setToast("Incident export prepared"))},"Export table"))),React.createElement(IncidentTable,{"incidents":(incidents),"limit":(64),"onSelect":(id => { state.selectedIncident = id; draw(); })})); }
function Assets() { return React.createElement(React.Fragment,null,React.createElement("section",{"className":"toolbar panel"},React.createElement("div",null,React.createElement("strong",null,(state.snapshot.assets.length),"managed assets"),React.createElement("small",null,"Live health, utilization, cost, and alert posture")),React.createElement("button",{"className":"primary-button","onClick":(() => setToast("Asset inventory synchronized"))},"Synchronize inventory")),React.createElement(AssetGrid,{"assets":(state.snapshot.assets)})); }
function Content() { if (state.view === "overview")
    return React.createElement(Overview,null); if (state.view === "incidents")
    return React.createElement(Incidents,null); if (state.view === "assets")
    return React.createElement(Assets,null); if (state.view === "reports" && state.report)
    return React.createElement(ReportsPanel,{"report":(state.report),"onRegenerate":(refreshIntelligence)}); if (state.view === "settings")
    return React.createElement(SettingsPanel,null); return React.createElement("div",{"className":"loading-panel"},"Preparing protected report…"); }
function App() { return React.createElement("div",{"className":"app-shell"},React.createElement(Sidebar,{"view":(state.view),"onNavigate":(navigate)}),React.createElement("main",null,React.createElement(Header,{"title":(titles[state.view]),"query":(state.query),"onQuery":(value => { state.query = value; draw(); }),"onCommand":(() => { state.commandOpen = true; draw(); }),"onRefresh":(refreshIntelligence),"loading":(state.loading)}),React.createElement("div",{"className":"content"},React.createElement(Content,null))),React.createElement(CommandPalette,{"open":(state.commandOpen),"onClose":(() => { state.commandOpen = false; draw(); }),"onNavigate":(navigate),"onRefresh":(refreshIntelligence)}),React.createElement(IncidentModal,{"incident":(currentIncident()),"onClose":(() => { state.selectedIncident = null; draw(); })}),React.createElement(Toast,{"message":(state.toast)})); }
function draw() { render(React.createElement(App,null), root); }
document.addEventListener("keydown", event => { if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "k") {
    event.preventDefault();
    state.commandOpen = true;
    draw();
} if (event.key === "Escape" && (state.commandOpen || state.selectedIncident)) {
    state.commandOpen = false;
    state.selectedIncident = null;
    draw();
} });
draw();
await refreshIntelligence();
