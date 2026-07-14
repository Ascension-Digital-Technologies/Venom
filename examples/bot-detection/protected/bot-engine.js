// @venom: protected module


var COMMON_AUTOMATION_GLOBALS = [
  "_phantom", "callPhantom", "__nightmare", "domAutomation", "domAutomationController",
  "__webdriver_evaluate", "__selenium_evaluate", "__webdriver_script_function",
  "__webdriver_script_func", "__webdriver_script_fn", "__fxdriver_evaluate",
  "__driver_unwrapped", "__webdriver_unwrapped", "__driver_evaluate", "__selenium_unwrapped",
  "webdriver", "__playwright", "__pw_manual", "Cypress"
];

var TELEMETRY_SCHEMA_VERSION = 2;
var SESSION_TTL_MS = 120000;
var MAX_ACTIVE_SESSIONS = 64;
var protectedSessions = Object.create(null);
var protectedSessionOrder = [];

function telemetryError(code) { var error = new Error(code); error.code = code; return error; }
function makeOpaqueToken(prefix) { return prefix + "-" + fnv1a(String(Date.now()) + "|" + String(Math.random()) + "|" + String(protectedSessionOrder.length)); }
function pruneSessions(now) {
  while (protectedSessionOrder.length > 0) {
    var id = protectedSessionOrder[0]; var item = protectedSessions[id];
    if (item && item.expiresAt > now && protectedSessionOrder.length <= MAX_ACTIVE_SESSIONS) break;
    protectedSessionOrder.shift(); delete protectedSessions[id];
  }
}

export function beginAssessmentSession() {
  var now = Date.now(); pruneSessions(now);
  var sessionId = makeOpaqueToken("s") + makeOpaqueToken("i");
  var nonce = makeOpaqueToken("n") + makeOpaqueToken("c");
  var expiresAt = now + SESSION_TTL_MS;
  protectedSessions[sessionId] = { nonce: nonce, lastSequence: 0, lastCapturedAtMs: 0, expiresAt: expiresAt };
  protectedSessionOrder.push(sessionId); pruneSessions(now);
  return { schemaVersion: TELEMETRY_SCHEMA_VERSION, sessionId: sessionId, nonce: nonce, expiresAtMs: expiresAt, nextSequence: 1 };
}

function validateTelemetryEnvelope(envelope) {
  if (!envelope || typeof envelope !== "object" || Array.isArray(envelope)) throw telemetryError("invalid-envelope");
  var allowed = ["schemaVersion","sessionId","nonce","sequence","capturedAtMs","telemetry"];
  var keys = Object.keys(envelope);
  if (keys.length !== allowed.length || keys.some(function (key) { return allowed.indexOf(key) === -1; })) throw telemetryError("invalid-envelope-schema");
  if (finite(envelope.schemaVersion) !== TELEMETRY_SCHEMA_VERSION) throw telemetryError("unsupported-schema");
  var now = Date.now(); pruneSessions(now);
  var session = protectedSessions[text(envelope.sessionId)];
  if (!session) throw telemetryError("unknown-session");
  if (session.expiresAt <= now) throw telemetryError("expired-session");
  if (text(envelope.nonce) !== session.nonce) throw telemetryError("nonce-mismatch");
  var sequence = finite(envelope.sequence, -1);
  if (!Number.isInteger(sequence) || sequence !== session.lastSequence + 1) throw telemetryError("replayed-or-out-of-order");
  var capturedAt = finite(envelope.capturedAtMs, -1);
  if (!Number.isInteger(capturedAt) || capturedAt <= 0) throw telemetryError("invalid-capture-time");
  // Browser and protected QuickJS clocks are separate trust domains. Some WASM
  // hosts expose a non-advancing wall clock, so absolute skew checks can reject
  // fresh telemetry after the page has been open for 30 seconds. Sequence, nonce,
  // and session binding already prevent replay; require capture time to advance
  // monotonically within the authenticated session instead.
  if (session.lastCapturedAtMs > 0 && capturedAt < session.lastCapturedAtMs) throw telemetryError("stale-telemetry");
  if (!envelope.telemetry || typeof envelope.telemetry !== "object" || Array.isArray(envelope.telemetry)) throw telemetryError("invalid-telemetry");
  session.lastSequence = sequence;
  session.lastCapturedAtMs = capturedAt;
  return envelope.telemetry;
}

function deriveTemporalBehavior(input) {
  var behavior = input.behavior && typeof input.behavior === "object" ? input.behavior : {};
  var samples = Array.isArray(behavior.samples) ? behavior.samples.slice(-128) : [];
  var intervals = [], velocities = [], previous = null;
  samples.forEach(function (sample) {
    if (!sample || typeof sample !== "object") return;
    var current = { type: text(sample.type), t: finite(sample.t, -1), x: finite(sample.x), y: finite(sample.y) };
    if (previous && current.t >= previous.t) {
      var dt = Math.max(1, current.t - previous.t); intervals.push(dt);
      if (current.type === "pointer" && previous.type === "pointer") {
        var dx = current.x - previous.x, dy = current.y - previous.y; velocities.push(Math.sqrt(dx*dx + dy*dy) / dt);
      }
    }
    previous = current;
  });
  function stats(values) {
    if (!values.length) return { count: 0, mean: 0, deviation: 0 };
    var mean = values.reduce(function(a,b){return a+b;},0)/values.length;
    var variance = values.reduce(function(a,b){var d=b-mean; return a+d*d;},0)/values.length;
    return { count: values.length, mean: mean, deviation: Math.sqrt(variance) };
  }
  behavior.temporal = { sampleCount: samples.length, interval: stats(intervals), velocity: stats(velocities) };
  delete behavior.samples; input.behavior = behavior; return input;
}

function browserNameFromUserAgent(userAgent) {
  var ua = text(userAgent);
  if (/Edg\//.test(ua)) return "Microsoft Edge";
  if (/OPR\//.test(ua)) return "Opera";
  if (/Firefox\//.test(ua)) return "Firefox";
  if (/Chrome\//.test(ua)) return "Chrome";
  if (/Safari\//.test(ua)) return "Safari";
  return "Unknown";
}

function deriveProtectedSignals(input) {
  var output = input && typeof input === "object" ? input : {};
  var browser = output.browser && typeof output.browser === "object" ? output.browser : {};
  var evidence = output.integrityEvidence && typeof output.integrityEvidence === "object" ? output.integrityEvidence : {};
  var ua = lower(browser.userAgent);
  var properties = Array.isArray(evidence.globalPropertyNames) ? evidence.globalPropertyNames : [];
  var notificationPermission = text(evidence.notificationPermission || "unsupported");
  var queriedNotification = text(evidence.queriedNotificationPermission || "unsupported");
  var normalizedNotification = notificationPermission === "default" ? "prompt" : notificationPermission;
  var functionSources = evidence.functionSources && typeof evidence.functionSources === "object" ? evidence.functionSources : {};
  var modifiedNativeSurfaces = Object.keys(functionSources).filter(function (name) {
    var source = text(functionSources[name]);
    return source.length > 0 && !/\{\s*\[native code\]\s*\}/.test(source);
  });
  browser.name = browserNameFromUserAgent(browser.userAgent);
  browser.mobile = bool(browser.userAgentDataMobile) || /android|iphone|ipad|mobile/.test(ua);
  output.browser = browser;
  output.automation = {
    webdriver: bool(evidence.webdriver),
    headlessUserAgent: /headlesschrome|phantomjs|slimerjs/.test(ua),
    automationGlobals: COMMON_AUTOMATION_GLOBALS.filter(function (name) { return properties.indexOf(name) !== -1; }),
    outerDimensionsMissing: !browser.mobile && (finite(evidence.outerWidth) === 0 || finite(evidence.outerHeight) === 0),
    permissionsMismatch: normalizedNotification !== "unsupported" && queriedNotification !== "unsupported" && normalizedNotification !== queriedNotification,
    notificationPermission: notificationPermission,
    queriedNotificationPermission: queriedNotification,
    nativeSurfaceMismatch: modifiedNativeSurfaces.length >= 2,
    modifiedNativeSurfaces: modifiedNativeSurfaces,
    chromeObjectPresent: bool(evidence.chromeObjectPresent),
    devtoolsSizeDelta: {
      width: Math.max(0, finite(evidence.outerWidth) - finite(evidence.innerWidth)),
      height: Math.max(0, finite(evidence.outerHeight) - finite(evidence.innerHeight))
    }
  };
  delete output.integrityEvidence;
  return output;
}

function scoreTone(score) {
  if (score >= 86) return "excellent";
  if (score >= 72) return "good";
  if (score >= 52) return "uncertain";
  if (score >= 30) return "suspicious";
  return "critical";
}

function countSignals(input) {
  return Object.keys(input).reduce(function (count, key) {
    var value = input[key];
    if (!value || typeof value !== "object") return count;
    return count + Object.keys(value).length;
  }, 0);
}

function finite(value, fallback) {
  var n = Number(value);
  return Number.isFinite(n) ? n : (fallback === undefined ? 0 : fallback);
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function bool(value) {
  return value === true;
}

function text(value) {
  return value == null ? "" : String(value);
}

function lower(value) {
  return text(value).toLowerCase();
}

function has(list, value) {
  return Array.isArray(list) && list.indexOf(value) !== -1;
}

function round(value) {
  return Math.round(finite(value));
}

function points(value, max) {
  return clamp(round(value), 0, max);
}

function fnv1a(value) {
  var hash = 2166136261;
  var source = text(value);
  for (var i = 0; i < source.length; i += 1) {
    hash ^= source.charCodeAt(i);
    hash = Math.imul(hash, 16777619);
  }
  return (hash >>> 0).toString(16).padStart(8, "0");
}

function addFinding(findings, severity, code, title, detail, impact) {
  findings.push({
    severity: severity,
    code: code,
    title: title,
    detail: detail,
    impact: round(impact)
  });
}

function category(id, label, score, max, signals) {
  var normalized = max > 0 ? score / max : 0;
  var level = normalized >= 0.78 ? "strong" : normalized >= 0.52 ? "mixed" : "weak";
  return {
    id: id,
    label: label,
    score: points(score, max),
    max: max,
    percentage: clamp(round(normalized * 100), 0, 100),
    level: level,
    signals: signals
  };
}

function scoreIntegrity(input, findings) {
  var automation = input.automation || {};
  var browser = input.browser || {};
  var score = 30;
  var signals = [];
  var ua = lower(browser.userAgent);

  if (bool(automation.webdriver)) {
    score -= 24;
    addFinding(findings, "critical", "webdriver", "WebDriver automation flag is active", "navigator.webdriver reported true.", -24);
  } else {
    signals.push("WebDriver flag is not active");
  }

  if (bool(automation.headlessUserAgent) || ua.indexOf("headlesschrome") !== -1 || ua.indexOf("phantomjs") !== -1) {
    score -= 22;
    addFinding(findings, "critical", "headless-ua", "Headless browser marker detected", "The user agent contains a known headless runtime token.", -22);
  }

  var globals = Array.isArray(automation.automationGlobals) ? automation.automationGlobals : [];
  if (globals.length > 0) {
    var globalPenalty = clamp(7 + globals.length * 3, 7, 18);
    score -= globalPenalty;
    addFinding(findings, "high", "automation-globals", "Automation framework globals detected", globals.slice(0, 5).join(", "), -globalPenalty);
  } else {
    signals.push("No common automation globals found");
  }

  if (bool(automation.outerDimensionsMissing)) {
    score -= 6;
    addFinding(findings, "medium", "window-dimensions", "Window chrome dimensions are missing", "outerWidth or outerHeight is zero, which is unusual for an interactive desktop browser.", -6);
  }

  if (bool(automation.permissionsMismatch)) {
    score -= 7;
    addFinding(findings, "high", "permission-mismatch", "Permission APIs report an inconsistent state", "Notification permission and Permissions API results disagree.", -7);
  }

  if (bool(automation.nativeSurfaceMismatch)) {
    score -= 5;
    addFinding(findings, "medium", "native-surface", "Browser API surface appears modified", "One or more native browser methods do not expose an expected native function representation.", -5);
  }

  if (!bool(browser.cookieEnabled)) {
    score -= 1;
    addFinding(findings, "info", "cookies-disabled", "Cookies are disabled", "This can be a privacy choice and is only a weak automation signal.", -1);
  }

  if (finite(browser.pluginsCount) === 0 && !bool(browser.mobile) && ua.indexOf("firefox") === -1) {
    score -= 2;
    addFinding(findings, "info", "plugin-surface", "Desktop plugin surface is empty", "Modern privacy settings can also produce this result.", -2);
  }

  if (score >= 26) signals.push("Browser surface is internally consistent");
  return category("integrity", "Browser integrity", score, 30, signals);
}

function scoreCoherence(input, findings) {
  var browser = input.browser || {};
  var hardware = input.hardware || {};
  var display = input.display || {};
  var graphics = input.graphics || {};
  var locale = input.locale || {};
  var score = 21;
  var signals = [];
  var ua = lower(browser.userAgent);
  var platform = lower(browser.platform);
  var cores = finite(hardware.hardwareConcurrency);
  var memory = finite(hardware.deviceMemory);
  var width = finite(display.screenWidth);
  var height = finite(display.screenHeight);
  var dpr = finite(display.devicePixelRatio);
  var touches = finite(hardware.maxTouchPoints);

  if (cores >= 1 && cores <= 256) {
    score += 1;
    signals.push("CPU concurrency is plausible");
  } else {
    score -= 5;
    addFinding(findings, "medium", "cpu-count", "Implausible logical processor count", "The reported hardwareConcurrency value is outside a normal browser range.", -5);
  }

  if (memory === 0 || (memory >= 0.25 && memory <= 64)) score += 1;
  else {
    score -= 3;
    addFinding(findings, "medium", "memory-value", "Implausible device memory value", "The reported deviceMemory value is outside the browser API range.", -3);
  }

  if (width >= 240 && height >= 240 && dpr >= 0.5 && dpr <= 8) {
    score += 1;
    signals.push("Display geometry is plausible");
  } else {
    score -= 6;
    addFinding(findings, "high", "display-geometry", "Display geometry is inconsistent", "Screen dimensions or pixel ratio are outside expected interactive-device ranges.", -6);
  }

  var claimsMobile = bool(browser.mobile) || /android|iphone|ipad|mobile/.test(ua);
  if (claimsMobile && touches <= 0) {
    score -= 4;
    addFinding(findings, "medium", "touch-mismatch", "Mobile identity has no touch capability", "The user agent looks mobile while maxTouchPoints is zero.", -4);
  } else if (!claimsMobile && touches > 20) {
    score -= 3;
    addFinding(findings, "medium", "touch-count", "Touch point count is unusual", "The browser reports an unusually high maxTouchPoints value.", -3);
  } else {
    score += 1;
  }

  if (ua.indexOf("windows") !== -1 && platform && platform.indexOf("win") === -1) {
    score -= 5;
    addFinding(findings, "high", "platform-mismatch", "User agent and platform disagree", "The user agent claims Windows while navigator.platform reports another platform.", -5);
  }
  if ((ua.indexOf("iphone") !== -1 || ua.indexOf("ipad") !== -1) && platform && platform.indexOf("mac") === -1 && platform.indexOf("iphone") === -1 && platform.indexOf("ipad") === -1) {
    score -= 5;
    addFinding(findings, "high", "ios-platform-mismatch", "iOS identity and platform disagree", "The reported platform is inconsistent with the mobile user agent.", -5);
  }

  if (text(locale.timeZone).length > 0 && Array.isArray(locale.languages) && locale.languages.length > 0) {
    score += 1;
    signals.push("Locale and timezone surfaces are populated");
  } else {
    score -= 2;
    addFinding(findings, "info", "locale-surface", "Locale surface is incomplete", "Timezone or language information is unavailable.", -2);
  }

  if (text(graphics.webglRenderer).length > 0 || !bool(graphics.webglSupported)) {
    score += 1;
  } else {
    score -= 2;
    addFinding(findings, "info", "webgl-surface", "WebGL renderer is unexpectedly blank", "WebGL is supported but renderer metadata is unavailable.", -2);
  }

  return category("coherence", "Environment coherence", score, 25, signals);
}

function scoreBehavior(input, findings) {
  var behavior = input.behavior || {};
  var session = input.session || {};
  var score = 4;
  var signals = [];
  var pointerMoves = finite(behavior.pointerMoves);
  var pointerDistance = finite(behavior.pointerDistance);
  var clicks = finite(behavior.clicks);
  var keys = finite(behavior.keyPresses);
  var scroll = finite(behavior.scrollDistance);
  var focusMs = finite(behavior.focusMs);
  var trusted = finite(behavior.trustedEvents);
  var synthetic = finite(behavior.syntheticEvents);
  var age = finite(session.ageMs);

  if (pointerMoves >= 4) {
    var pointerPoints = clamp(Math.log(pointerMoves + 1) * 1.8 + Math.log(pointerDistance + 1) * 0.55, 2, 8);
    score += pointerPoints;
    signals.push("Pointer movement observed");
  }
  if (clicks >= 1) {
    score += clamp(2 + clicks * 0.8, 2, 5);
    signals.push("Trusted click interaction observed");
  }
  if (keys >= 1) {
    score += clamp(1 + keys * 0.35, 1, 3);
    signals.push("Keyboard interaction observed");
  }
  if (scroll >= 80) {
    score += clamp(Math.log(scroll + 1) * 0.55, 1, 3);
    signals.push("Natural page exploration observed");
  }
  if (focusMs >= 1500) score += clamp(focusMs / 8000, 1, 3);

  if (synthetic > trusted && synthetic >= 3) {
    score -= 5;
    addFinding(findings, "high", "synthetic-events", "Synthetic events dominate the session", "More untrusted programmatic events were observed than trusted user events.", -5);
  }

  if (age > 15000 && trusted === 0) {
    score -= 3;
    addFinding(findings, "medium", "no-interaction", "No trusted interaction after an extended session", "A long-lived page session has not produced a trusted pointer, keyboard, click, or scroll event.", -3);
  }

  var temporal = behavior.temporal || {};
  if (finite(temporal.sampleCount) >= 8) {
    if (finite(temporal.interval && temporal.interval.deviation) >= 4) { score += 2; signals.push("Interaction timing contains natural variance"); }
    else { score -= 2; addFinding(findings, "medium", "uniform-event-cadence", "Interaction cadence is unusually uniform", "Protected temporal analysis found very low event timing variance.", -2); }
  }
  if (finite(temporal.velocity && temporal.velocity.count) >= 5) {
    if (finite(temporal.velocity.deviation) > 0.02) { score += 1; signals.push("Pointer velocity varies over time"); }
    else { score -= 2; addFinding(findings, "medium", "uniform-pointer-speed", "Pointer speed is unusually uniform", "Protected temporal analysis found near-constant pointer velocity.", -2); }
  }
  if (trusted >= 6 && synthetic === 0) score += 1;
  return category("behavior", "Behavioral evidence", score, 25, signals);
}

function scoreTiming(input, findings) {
  var timing = input.timing || {};
  var score = 5;
  var signals = [];
  var samples = finite(timing.sampleCount);
  var mean = finite(timing.frameMeanMs);
  var deviation = finite(timing.frameStdDevMs);
  var longFrames = finite(timing.longFrames);

  if (samples >= 12) {
    if (mean >= 4 && mean <= 80) {
      score += 2;
      signals.push("Animation timing is plausible");
    } else {
      score -= 2;
      addFinding(findings, "info", "frame-rate", "Frame timing is outside the usual interactive range", "This may reflect throttling, a background tab, remote desktop, or automation.", -2);
    }
    if (deviation >= 0.08) {
      score += 2;
      signals.push("Frame cadence contains natural variance");
    } else {
      score -= 1;
      addFinding(findings, "info", "perfect-cadence", "Frame cadence is unusually uniform", "Extremely uniform timing can occur in virtualized or scripted environments.", -1);
    }
    if (longFrames > 0) score += 1;
  }

  return category("timing", "Timing naturalness", score, 10, signals);
}

function scoreNetwork(input, findings) {
  var network = input.network || {};
  var score = 7;
  var signals = [];
  var rtt = finite(network.rtt, -1);
  var downlink = finite(network.downlink, -1);
  var candidateTypes = Array.isArray(network.iceCandidateTypes) ? network.iceCandidateTypes : [];

  if (bool(network.online)) {
    score += 1;
    signals.push("Browser reports an active network");
  } else {
    score -= 2;
    addFinding(findings, "info", "offline", "Browser reports offline status", "An offline browser cannot provide normal network telemetry.", -2);
  }

  if (rtt >= 0 && rtt <= 60000 && downlink >= 0 && downlink <= 10000) score += 1;
  if (bool(network.connectionApiAvailable)) signals.push("Network Information API is available");
  if (bool(network.webRtcSupported) && candidateTypes.length > 0) score += 1;
  if (bool(network.publicAddressAvailable)) signals.push("A server-observed network address is available");

  return category("network", "Network context", score, 10, signals);
}

function overallLabel(score) {
  if (score >= 86) return "Very likely human";
  if (score >= 72) return "Likely human";
  if (score >= 52) return "Uncertain";
  if (score >= 30) return "Likely automated";
  return "Very likely automated";
}

function recommendation(score, findings) {
  var critical = findings.some(function (item) { return item.severity === "critical"; });
  if (critical || score < 30) return "Challenge or block this session and require a trusted server-side verification step.";
  if (score < 52) return "Apply a low-friction challenge and rate limits before allowing sensitive actions.";
  if (score < 72) return "Allow low-risk browsing, but verify again before account, payment, or abuse-sensitive actions.";
  return "Allow normal interaction while retaining ordinary server-side rate limits and abuse monitoring.";
}

export function assessClient(envelope) {
  var input = validateTelemetryEnvelope(envelope);
  input = deriveTemporalBehavior(input);
  input = deriveProtectedSignals(input);
  var findings = [];
  var categories = [
    scoreIntegrity(input, findings),
    scoreCoherence(input, findings),
    scoreBehavior(input, findings),
    scoreTiming(input, findings),
    scoreNetwork(input, findings)
  ];

  var total = categories.reduce(function (sum, item) { return sum + item.score; }, 0);
  var humanScore = clamp(round(total), 1, 100);
  var severeCount = findings.filter(function (item) { return item.severity === "critical" || item.severity === "high"; }).length;
  var evidence = categories.reduce(function (sum, item) { return sum + item.signals.length; }, 0);
  var confidence = clamp(round(48 + evidence * 3 + severeCount * 6 + Math.abs(humanScore - 50) * 0.45), 35, 98);
  var identityMaterial = [
    text(input.browser && input.browser.userAgent),
    text(input.browser && input.browser.platform),
    text(input.display && input.display.screenWidth) + "x" + text(input.display && input.display.screenHeight),
    text(input.display && input.display.devicePixelRatio),
    text(input.locale && input.locale.timeZone),
    text(input.graphics && input.graphics.canvasHash),
    text(input.graphics && input.graphics.webglRenderer),
    text(input.graphics && input.graphics.audioHash)
  ].join("|");

  findings.sort(function (a, b) { return a.impact - b.impact; });
  return {
    humanScore: humanScore,
    scoreTone: scoreTone(humanScore),
    automationRisk: 100 - humanScore,
    classification: overallLabel(humanScore),
    confidence: confidence,
    fingerprintId: fnv1a(identityMaterial) + "-" + fnv1a(identityMaterial.split("").reverse().join("")),
    categories: categories,
    findings: findings.slice(0, 12),
    recommendation: recommendation(humanScore, findings),
    signalCount: countSignals(input),
    engine: "Venom protected heuristic engine v2",
    telemetrySchemaVersion: TELEMETRY_SCHEMA_VERSION
  };
}
