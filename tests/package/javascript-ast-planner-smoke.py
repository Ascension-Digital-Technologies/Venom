#!/usr/bin/env python3
from __future__ import annotations
import json, subprocess, sys, tempfile
from pathlib import Path

venom = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='venom-ast-planner-') as tmp:
    root = Path(tmp)
    (root / 'app.js').write_text('''
export async function calculateRisk(input) {
  return input.values.reduce((sum, value) => sum + value, 0);
}
const renderDashboard =
  async (
    model
  ) => {
    document.querySelector('#app').textContent = String(model.score);
  };
const compact = value => ({ value });
const globalDocument = () => globalThis.document.title;
function receiverBound() { return this.value; }
let state = 0;
function readState() { return state; }
function mutateState() { state++; return state; }
const settings = { multiplier: 3 };
function readObjectCapture(value) { return value * settings.multiplier; }
const FACTOR = 4;
function helper(value) { return value + 1; }
function dependencySafe(value) { return helper(value) * FACTOR; }
const queue = [];
function mutateCapturedObject(value) { queue.push(value); return queue.length; }
function defaulted({ value = 1 } = {}) { return value * 2; }
const arrowHelper = value => value * 3;
function arrowDependency(value) { return arrowHelper(value); }
class PriceModel { constructor(rate) { this.rate = rate; } quote(value) { return value * this.rate; } }
function constructPrice(value) { return new PriceModel(2).quote(value); }
function browserHelper() { return document.title; }
function callsBrowserHelper() { return browserHelper(); }
function reviewHelper() { return state; }
function callsReviewHelper() { return reviewHelper(); }
function cycleA(value) { return value <= 0 ? 0 : cycleB(value - 1); }
function cycleB(value) { return value <= 0 ? 0 : cycleA(value - 1); }
function dynamicTarget(flag, a, b) { return (flag ? a : b)(); }
''', encoding='utf-8')
    run = subprocess.run([str(venom), 'plan', str(root), '--format', 'json'], text=True, capture_output=True)
    if run.returncode != 0: raise SystemExit(run.stdout + run.stderr)
    report = json.loads(run.stdout)
    functions = {item['name']: item for item in report['recommendations'][0]['functions']}
    if set(functions) != {'calculateRisk', 'renderDashboard', 'compact', 'globalDocument', 'receiverBound', 'readState', 'mutateState', 'readObjectCapture', 'helper', 'dependencySafe', 'mutateCapturedObject', 'defaulted', 'arrowHelper', 'arrowDependency', 'constructPrice', 'browserHelper', 'callsBrowserHelper', 'reviewHelper', 'callsReviewHelper', 'cycleA', 'cycleB', 'dynamicTarget'}: raise SystemExit(functions)
    if functions['renderDashboard']['runtime'] != 'browser': raise SystemExit(functions['renderDashboard'])
    if functions['calculateRisk']['runtime'] != 'protected': raise SystemExit(functions['calculateRisk'])
    if functions['globalDocument']['runtime'] != 'browser': raise SystemExit(functions['globalDocument'])
    if functions['receiverBound']['runtime'] != 'manual-review': raise SystemExit(functions['receiverBound'])
    if functions['readState']['runtime'] != 'manual-review': raise SystemExit(functions['readState'])
    if functions['mutateState']['runtime'] != 'manual-review': raise SystemExit(functions['mutateState'])
    if functions['readObjectCapture']['runtime'] != 'manual-review': raise SystemExit(functions['readObjectCapture'])
    if functions['dependencySafe']['runtime'] != 'protected': raise SystemExit(functions['dependencySafe'])
    if functions['mutateCapturedObject']['runtime'] != 'manual-review': raise SystemExit(functions['mutateCapturedObject'])
    if functions['defaulted']['runtime'] != 'protected': raise SystemExit(functions['defaulted'])
    if functions['arrowDependency']['runtime'] != 'protected': raise SystemExit(functions['arrowDependency'])
    if functions['constructPrice']['runtime'] != 'manual-review': raise SystemExit(functions['constructPrice'])
    if functions['callsBrowserHelper']['runtime'] != 'browser': raise SystemExit(functions['callsBrowserHelper'])
    if functions['callsReviewHelper']['runtime'] != 'manual-review': raise SystemExit(functions['callsReviewHelper'])
    if functions['cycleA']['runtime'] != 'protected' or functions['cycleB']['runtime'] != 'protected': raise SystemExit((functions['cycleA'], functions['cycleB']))
    if not any('dependency cycle validated' in reason for reason in functions['cycleA']['reasons']): raise SystemExit(functions['cycleA'])
    if functions['dynamicTarget']['runtime'] != 'manual-review': raise SystemExit(functions['dynamicTarget'])
print('JavaScript AST planner smoke: PASS')
