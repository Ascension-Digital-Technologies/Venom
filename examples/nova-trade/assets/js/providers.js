// @venom: browser
// src/js/providers.js
// ===================== PROVIDER ADAPTERS =====================
// These lightweight adapters keep the UI decoupled from the current simulator.
// Later, MarketDataProvider can be swapped for a Rust/Go WebSocket backend without rewriting panels.
class MarketDataProvider {
  constructor(source = 'simulated') { this.source = source; }
  getSymbol(symbol) { return state.watchlist.find(w => w.sym === symbol); }
  getOrderBook() { return { bids: generateOrderBook('bid'), asks: generateOrderBook('ask') }; }
  getCandles() { return candles; }
}

class PaperTradingProvider {
  submit(order) { return submitPaperOrder(order.side, order); }
  cancel(orderId) { return cancelPaperOrder(orderId); }
  positions() { return state.positions; }
  openOrders() { return state.orders.filter(o => o.status === 'Open'); }
}

class PortfolioProvider {
  balances() { return state.balances || []; }
  equity() { return state.equity; }
}

class WebSocketProvider {
  constructor(url = null) { this.url = url; this.socket = null; this.connected = false; }
  connect() { this.connected = Boolean(this.url); return this.connected; }
  disconnect() { if (this.socket) this.socket.close(); this.connected = false; }
}

window.NovaProviders = {
  marketData: new MarketDataProvider(),
  paperTrading: new PaperTradingProvider(),
  portfolio: new PortfolioProvider(),
  websocket: new WebSocketProvider(),
  risk: { evaluate: evaluateRisk, health: computeAccountHealth },
  broker: window.NovaBroker || null,
  alerts: window.NovaAlerts || null,
  settings: window.NovaSettings || null
};
