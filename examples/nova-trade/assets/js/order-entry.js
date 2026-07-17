// @venom: browser
// src/js/order-entry.js
// ===================== ORDER ENTRY =====================
function submitPaperOrder(side) {
  const result = validateOrderEntry();
  if (!result.valid) {
    showToast('error', 'Order Blocked', document.getElementById('oeValidation').textContent || 'Check order fields');
    return;
  }
  const base = getBaseAsset();
  const typeLabel = result.type === 'market' ? 'Market' : result.type === 'stop' ? 'Stop Limit' : 'Limit';
  const toastType = side === 'Buy' ? 'success' : 'error';
  showToast(toastType, `${typeLabel} Order Submitted`, `${side} ${formatAmount(result.amount)} ${base} at ${formatPrice(result.price)} USDT`);
}

document.getElementById('btnBuy').addEventListener('click', () => submitPaperOrder('Buy'));
document.getElementById('btnSell').addEventListener('click', () => submitPaperOrder('Sell'));

['oePrice', 'oeAmount', 'oeTotal'].forEach(id => {
  const input = document.getElementById(id);
  input.addEventListener('input', () => {
    if (id === 'oeTotal') {
      const price = getActiveOrderType() === 'market' ? state.currentPrice : parseNumber(document.getElementById('oePrice').value);
      const total = parseNumber(input.value);
      if (Number.isFinite(price) && price > 0 && Number.isFinite(total)) {
        document.getElementById('oeAmount').value = (total / price).toFixed(4);
      }
    }
    validateOrderEntry();
  });
  input.addEventListener('blur', () => {
    if (id === 'oePrice') input.value = formatPrice(parseNumber(input.value) || state.currentPrice);
    validateOrderEntry();
  });
});

document.getElementById('oeSlider').addEventListener('input', (e) => {
  const pct = Number(e.target.value);
  const price = getActiveOrderType() === 'market' ? state.currentPrice : parseNumber(document.getElementById('oePrice').value);
  const maxAmount = 1.505;
  const amount = (maxAmount * pct / 100).toFixed(4);
  document.getElementById('oeAmount').value = amount;
  document.getElementById('oeTotal').value = (price * amount).toLocaleString('en-US', {minimumFractionDigits: 2, maximumFractionDigits: 2});
  validateOrderEntry();
});

// TP/SL and execution option checkboxes
function initOrderOptionChecks() {
  document.querySelectorAll('.oe-check').forEach(check => {
    check.addEventListener('click', function() {
      const box = this.querySelector('.oe-check-box');
      box.classList.toggle('checked');
      safeStorage.write({ orderOptions: Array.from(document.querySelectorAll('.oe-check-box.checked')).map(el => el.parentElement.id) });
      validateOrderEntry();
    });
  });
}


// ===================== ORDER ENTRY RISK OVERLAY V4 =====================
const v3ValidateOrderEntry = validateOrderEntry;
validateOrderEntry = function validateOrderEntryV4() {
  const result = v3ValidateOrderEntry();
  const validation = document.getElementById('oeValidation');
  const price = result.type === 'market' ? state.currentPrice : result.price;
  const order = {
    symbol: state.selectedSymbol,
    side: 'Buy',
    type: result.type === 'market' ? 'Market' : result.type === 'stop' ? 'Stop Limit' : 'Limit',
    price,
    amount: result.amount,
    postOnly: Boolean(document.querySelector('#postOnlyCheck .oe-check-box.checked')),
    reduceOnly: Boolean(document.querySelector('#reduceOnlyCheck .oe-check-box.checked'))
  };
  if (window.NovaRisk && result.amount > 0 && price > 0) {
    const risk = evaluateRisk(order);
    result.risk = risk;
    result.valid = result.valid && risk.valid;
    const fee = document.getElementById('estFee');
    if (fee) fee.textContent = `${formatMoney(risk.fee)} USDT`;
    const riskPreview = document.getElementById('riskPreview');
    if (riskPreview) riskPreview.textContent = risk.health.level === 'healthy' ? 'Safe' : risk.health.level;
    const oeRisk = document.getElementById('oeRisk');
    if (oeRisk) oeRisk.textContent = riskLabelFromScore(risk.score);
    if (validation) {
      validation.classList.remove('good', 'warn', 'bad');
      if (!risk.valid) {
        validation.classList.add('bad');
        validation.textContent = risk.blockers[0];
      } else if (risk.warnings.length) {
        validation.classList.add('warn');
        validation.textContent = risk.warnings[0];
      } else if (result.valid) {
        validation.classList.add('good');
        validation.textContent = `Estimated margin ${formatMoney(risk.requiredMargin)} USDT · free ${formatMoney(risk.available)} USDT`;
      }
    }
  }
  document.getElementById('btnBuy').disabled = !result.valid;
  document.getElementById('btnSell').disabled = !result.valid;
  return result;
};
