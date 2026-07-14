// @venom: protected module

function finite(value, fallback = 0) {
  const n = Number(value);
  return Number.isFinite(n) ? n : fallback;
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

// @venom: input side:string, orderType:string, price:number, quantity:number, equity:number, currentPrice:number, openPositions:integer
// @venom: output approved:boolean, reason:string, riskScore:integer, notional:number, estimatedMargin:number
export function assessOrder(input) {
  const price = finite(input.price, finite(input.currentPrice));
  const quantity = finite(input.quantity);
  const equity = Math.max(0, finite(input.equity));
  const notional = Math.abs(price * quantity);
  const margin = notional * 0.10;
  const exposureRatio = equity > 0 ? notional / equity : 999;
  const concentration = clamp(finite(input.openPositions) * 6, 0, 30);
  const riskScore = Math.round(clamp(exposureRatio * 55 + concentration, 0, 100));
  let reason = "Approved by protected risk policy";
  let approved = true;
  if (!(price > 0) || !(quantity > 0)) { approved = false; reason = "Price and quantity must be positive"; }
  else if (equity <= 0) { approved = false; reason = "Account equity is unavailable"; }
  else if (margin > equity * 0.35) { approved = false; reason = "Estimated margin exceeds the 35% protected limit"; }
  else if (riskScore >= 85) { approved = false; reason = "Order exceeds the protected risk threshold"; }
  return { approved, reason, riskScore, notional, estimatedMargin: margin };
}

// @venom: input equity:number, markPrice:number, positions:array
// @venom: output riskScore:integer, riskLevel:string, grossExposure:number, netExposure:number, unrealizedPnl:number, marginUtilization:number
export function analyzePortfolio(input) {
  const equity = Math.max(0, finite(input.equity));
  let gross = 0, net = 0, pnl = 0;
  const positions = Array.isArray(input.positions) ? input.positions : [];
  for (const p of positions) {
    const size = Math.abs(finite(p.size));
    const entry = finite(p.entry, finite(input.markPrice));
    const mark = finite(p.mark, finite(input.markPrice));
    const direction = String(p.side).toLowerCase().startsWith("s") ? -1 : 1;
    const exposure = size * mark;
    gross += exposure;
    net += exposure * direction;
    pnl += (mark - entry) * size * direction;
  }
  const utilization = equity > 0 ? (gross * 0.10 / equity) * 100 : 100;
  const concentration = equity > 0 ? Math.abs(net) / equity * 35 : 100;
  const riskScore = Math.round(clamp(utilization * 0.7 + concentration + positions.length * 3, 0, 100));
  const riskLevel = riskScore >= 75 ? "high" : riskScore >= 45 ? "medium" : "low";
  return { riskScore, riskLevel, grossExposure: gross, netExposure: net, unrealizedPnl: pnl, marginUtilization: utilization };
}

// @venom: input symbol:string, candles:array
// @venom: output signal:string, confidence:integer, momentum:number, volatility:number
export function generateSignal(input) {
  const candles = Array.isArray(input.candles) ? input.candles : [];
  const closes = candles.map((c) => finite(c.close)).filter((v) => v > 0);
  if (closes.length < 10) return { signal: "NEUTRAL", confidence: 0, momentum: 0, volatility: 0 };
  const recent = closes.slice(-5).reduce((a,b)=>a+b,0)/5;
  const baseline = closes.slice(-20).reduce((a,b)=>a+b,0)/Math.min(20,closes.length);
  const returns = [];
  for (let i=1;i<closes.length;i++) returns.push((closes[i]-closes[i-1])/closes[i-1]);
  const momentum = baseline ? (recent-baseline)/baseline*100 : 0;
  const mean = returns.reduce((a,b)=>a+b,0)/Math.max(1,returns.length);
  const variance = returns.reduce((a,b)=>a+(b-mean)*(b-mean),0)/Math.max(1,returns.length);
  const volatility = Math.sqrt(variance)*100;
  const confidence = Math.round(clamp(Math.abs(momentum)*18 - volatility*2 + 45, 5, 96));
  const signal = momentum > 0.12 ? "BUY" : momentum < -0.12 ? "SELL" : "NEUTRAL";
  return { signal, confidence, momentum, volatility };
}
