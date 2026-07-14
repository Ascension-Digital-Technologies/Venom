// @venom: browser

(() => {
  "use strict";

  const existingApi = globalThis.VenomFingerprint;
  if (existingApi &&
      typeof existingApi.collectFingerprint === "function" &&
      typeof existingApi.createBehaviorTracker === "function") {
    return;
  }


const COMMON_AUTOMATION_GLOBALS = [
  "_phantom", "callPhantom", "__nightmare", "domAutomation", "domAutomationController",
  "__webdriver_evaluate", "__selenium_evaluate", "__webdriver_script_function",
  "__webdriver_script_func", "__webdriver_script_fn", "__fxdriver_evaluate",
  "__driver_unwrapped", "__webdriver_unwrapped", "__driver_evaluate", "__selenium_unwrapped",
  "webdriver", "__playwright", "__pw_manual", "Cypress"
];

const FONT_CANDIDATES = [
  "Arial", "Calibri", "Cambria", "Consolas", "Courier New", "Georgia", "Helvetica",
  "Menlo", "Monaco", "Roboto", "Segoe UI", "Tahoma", "Times New Roman", "Trebuchet MS", "Verdana"
];

function safeValue(read, fallback = null) {
  try {
    const value = read();
    return value === undefined ? fallback : value;
  } catch {
    return fallback;
  }
}

function hashString(value) {
  let hash = 2166136261;
  const source = String(value ?? "");
  for (let index = 0; index < source.length; index += 1) {
    hash ^= source.charCodeAt(index);
    hash = Math.imul(hash, 16777619);
  }
  return (hash >>> 0).toString(16).padStart(8, "0");
}

function isNativeFunction(value) {
  if (typeof value !== "function") return false;
  try {
    return /\{\s*\[native code\]\s*\}/.test(Function.prototype.toString.call(value));
  } catch {
    return false;
  }
}

function browserNameFromUserAgent(userAgent) {
  const ua = String(userAgent || "");
  if (/Edg\//.test(ua)) return "Microsoft Edge";
  if (/OPR\//.test(ua)) return "Opera";
  if (/Firefox\//.test(ua)) return "Firefox";
  if (/Chrome\//.test(ua)) return "Chrome";
  if (/Safari\//.test(ua)) return "Safari";
  return "Unknown";
}

async function collectUserAgentData() {
  const data = navigator.userAgentData;
  if (!data) return null;
  const base = {
    brands: Array.isArray(data.brands) ? data.brands.map((item) => ({ brand: item.brand, version: item.version })) : [],
    mobile: Boolean(data.mobile),
    platform: String(data.platform || "")
  };
  if (typeof data.getHighEntropyValues !== "function") return base;
  try {
    const entropy = await data.getHighEntropyValues([
      "architecture", "bitness", "formFactors", "fullVersionList", "model", "platformVersion", "uaFullVersion", "wow64"
    ]);
    return { ...base, ...entropy };
  } catch {
    return base;
  }
}

function collectBrowser(userAgentData) {
  const userAgent = String(navigator.userAgent || "");
  const mobile = Boolean(userAgentData?.mobile || /Android|iPhone|iPad|Mobile/i.test(userAgent));
  return {
    name: browserNameFromUserAgent(userAgent),
    userAgent,
    appVersion: String(navigator.appVersion || ""),
    appName: String(navigator.appName || ""),
    product: String(navigator.product || ""),
    productSub: String(navigator.productSub || ""),
    vendor: String(navigator.vendor || ""),
    vendorSub: String(navigator.vendorSub || ""),
    platform: String(navigator.platform || ""),
    oscpu: String(navigator.oscpu || ""),
    buildId: String(navigator.buildID || ""),
    mobile,
    cookieEnabled: Boolean(navigator.cookieEnabled),
    doNotTrack: String(navigator.doNotTrack ?? window.doNotTrack ?? navigator.msDoNotTrack ?? "unspecified"),
    globalPrivacyControl: Boolean(navigator.globalPrivacyControl),
    pdfViewerEnabled: Boolean(navigator.pdfViewerEnabled),
    javaEnabled: safeValue(() => Boolean(navigator.javaEnabled?.()), false),
    pluginsCount: safeValue(() => navigator.plugins.length, 0),
    pluginNames: safeValue(() => Array.from(navigator.plugins, (plugin) => plugin.name).slice(0, 20), []),
    mimeTypesCount: safeValue(() => navigator.mimeTypes.length, 0),
    userAgentData
  };
}

function collectHardware() {
  return {
    hardwareConcurrency: Number(navigator.hardwareConcurrency || 0),
    deviceMemory: Number(navigator.deviceMemory || 0),
    maxTouchPoints: Number(navigator.maxTouchPoints || 0),
    pointerFine: safeValue(() => matchMedia("(pointer: fine)").matches, false),
    pointerCoarse: safeValue(() => matchMedia("(pointer: coarse)").matches, false),
    hoverSupported: safeValue(() => matchMedia("(hover: hover)").matches, false),
    anyHoverSupported: safeValue(() => matchMedia("(any-hover: hover)").matches, false),
    touchEventSupported: "ontouchstart" in window,
    bluetoothSupported: "bluetooth" in navigator,
    usbSupported: "usb" in navigator,
    serialSupported: "serial" in navigator,
    hidSupported: "hid" in navigator,
    gamepadSupported: typeof navigator.getGamepads === "function"
  };
}

function collectDisplay() {
  const orientation = screen.orientation || {};
  return {
    screenWidth: Number(screen.width || 0),
    screenHeight: Number(screen.height || 0),
    availableWidth: Number(screen.availWidth || 0),
    availableHeight: Number(screen.availHeight || 0),
    colorDepth: Number(screen.colorDepth || 0),
    pixelDepth: Number(screen.pixelDepth || 0),
    devicePixelRatio: Number(window.devicePixelRatio || 1),
    innerWidth: Number(window.innerWidth || 0),
    innerHeight: Number(window.innerHeight || 0),
    outerWidth: Number(window.outerWidth || 0),
    outerHeight: Number(window.outerHeight || 0),
    visualViewportWidth: Number(window.visualViewport?.width || 0),
    visualViewportHeight: Number(window.visualViewport?.height || 0),
    orientationType: String(orientation.type || "unknown"),
    orientationAngle: Number(orientation.angle || 0),
    prefersDark: safeValue(() => matchMedia("(prefers-color-scheme: dark)").matches, false),
    prefersReducedMotion: safeValue(() => matchMedia("(prefers-reduced-motion: reduce)").matches, false),
    forcedColors: safeValue(() => matchMedia("(forced-colors: active)").matches, false),
    colorGamut: safeValue(() => matchMedia("(color-gamut: rec2020)").matches ? "rec2020" : matchMedia("(color-gamut: p3)").matches ? "p3" : "srgb", "unknown")
  };
}

function collectLocale() {
  const resolved = safeValue(() => Intl.DateTimeFormat().resolvedOptions(), {});
  const number = safeValue(() => Intl.NumberFormat().resolvedOptions(), {});
  return {
    language: String(navigator.language || ""),
    languages: Array.isArray(navigator.languages) ? navigator.languages.map(String) : [],
    timeZone: String(resolved.timeZone || ""),
    timeZoneOffsetMinutes: new Date().getTimezoneOffset(),
    locale: String(resolved.locale || ""),
    calendar: String(resolved.calendar || ""),
    numberingSystem: String(resolved.numberingSystem || number.numberingSystem || ""),
    hourCycle: String(resolved.hourCycle || ""),
    currencyExample: safeValue(() => new Intl.NumberFormat(undefined, { style: "currency", currency: "USD" }).format(1234.56), ""),
    dateExample: safeValue(() => new Intl.DateTimeFormat(undefined, { dateStyle: "full", timeStyle: "long" }).format(new Date()), "")
  };
}

function canvasFingerprint() {
  try {
    const canvas = document.createElement("canvas");
    canvas.width = 320;
    canvas.height = 80;
    const context = canvas.getContext("2d");
    if (!context) return { supported: false, hash: "unavailable" };
    context.textBaseline = "alphabetic";
    context.fillStyle = "#f60";
    context.fillRect(18, 8, 140, 42);
    context.fillStyle = "#069";
    context.font = "18px Arial";
    context.fillText("Venom Sentinel 🛡️ 123", 12, 36);
    context.strokeStyle = "rgba(20, 220, 190, .82)";
    context.arc(218, 34, 23, 0, Math.PI * 2);
    context.stroke();
    const data = canvas.toDataURL();
    return { supported: true, hash: hashString(data), sampleLength: data.length };
  } catch (error) {
    return { supported: false, hash: "blocked", error: String(error?.message || error) };
  }
}

function webglFingerprint() {
  const canvas = document.createElement("canvas");
  let gl = null;
  try {
    gl = canvas.getContext("webgl2") || canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
  } catch {
    gl = null;
  }
  if (!gl) {
    return { webglSupported: false, webglVersion: "unavailable", webglVendor: "", webglRenderer: "", extensionsCount: 0 };
  }
  const debug = safeValue(() => gl.getExtension("WEBGL_debug_renderer_info"), null);
  const version = safeValue(() => gl.getParameter(gl.VERSION), "");
  const shading = safeValue(() => gl.getParameter(gl.SHADING_LANGUAGE_VERSION), "");
  const vendor = safeValue(() => debug ? gl.getParameter(debug.UNMASKED_VENDOR_WEBGL) : gl.getParameter(gl.VENDOR), "");
  const renderer = safeValue(() => debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : gl.getParameter(gl.RENDERER), "");
  const extensions = safeValue(() => gl.getSupportedExtensions() || [], []);
  return {
    webglSupported: true,
    webglVersion: String(version),
    shadingLanguageVersion: String(shading),
    webglVendor: String(vendor),
    webglRenderer: String(renderer),
    extensionsCount: extensions.length,
    maxTextureSize: Number(safeValue(() => gl.getParameter(gl.MAX_TEXTURE_SIZE), 0)),
    maxViewportDimensions: safeValue(() => Array.from(gl.getParameter(gl.MAX_VIEWPORT_DIMS)), []),
    antialias: Boolean(safeValue(() => gl.getContextAttributes()?.antialias, false))
  };
}

async function audioFingerprint() {
  const AudioContextClass = window.OfflineAudioContext || window.webkitOfflineAudioContext;
  if (!AudioContextClass) return { supported: false, hash: "unavailable" };
  try {
    const context = new AudioContextClass(1, 44100, 44100);
    const oscillator = context.createOscillator();
    const compressor = context.createDynamicsCompressor();
    oscillator.type = "triangle";
    oscillator.frequency.value = 10000;
    compressor.threshold.value = -50;
    compressor.knee.value = 40;
    compressor.ratio.value = 12;
    compressor.attack.value = 0;
    compressor.release.value = 0.25;
    oscillator.connect(compressor);
    compressor.connect(context.destination);
    oscillator.start(0);
    const buffer = await context.startRendering();
    const channel = buffer.getChannelData(0);
    let sample = "";
    let sum = 0;
    for (let index = 4500; index < 5000; index += 5) {
      const value = channel[index] || 0;
      sum += Math.abs(value);
      sample += value.toFixed(8) + ",";
    }
    return { supported: true, hash: hashString(sample), magnitude: Number(sum.toFixed(6)) };
  } catch (error) {
    return { supported: false, hash: "blocked", error: String(error?.message || error) };
  }
}

function detectFonts() {
  if (!document.fonts?.check) return [];
  return FONT_CANDIDATES.filter((font) => safeValue(() => document.fonts.check(`16px "${font}"`), false));
}

async function collectStorage() {
  const estimate = await safeValue(async () => navigator.storage?.estimate?.(), null);
  const persisted = await safeValue(async () => navigator.storage?.persisted?.(), null);
  const resolvedEstimate = estimate && typeof estimate.then === "function" ? await estimate.catch(() => null) : estimate;
  const resolvedPersisted = persisted && typeof persisted.then === "function" ? await persisted.catch(() => null) : persisted;
  return {
    localStorage: safeValue(() => { const key = "__venom_probe"; localStorage.setItem(key, "1"); localStorage.removeItem(key); return true; }, false),
    sessionStorage: safeValue(() => { const key = "__venom_probe"; sessionStorage.setItem(key, "1"); sessionStorage.removeItem(key); return true; }, false),
    indexedDb: "indexedDB" in window,
    cacheStorage: "caches" in window,
    storageManager: Boolean(navigator.storage),
    quotaBytes: Number(resolvedEstimate?.quota || 0),
    usageBytes: Number(resolvedEstimate?.usage || 0),
    persisted: resolvedPersisted === null ? "unknown" : Boolean(resolvedPersisted)
  };
}

async function permissionState(name) {
  if (!navigator.permissions?.query) return "unsupported";
  try {
    const result = await navigator.permissions.query({ name });
    return String(result.state || "unknown");
  } catch {
    return "unsupported";
  }
}

async function collectPermissions() {
  const names = ["notifications", "geolocation", "camera", "microphone", "clipboard-read"];
  const pairs = await Promise.all(names.map(async (name) => [name, await permissionState(name)]));
  return Object.fromEntries(pairs);
}

async function collectMediaDevices() {
  if (!navigator.mediaDevices?.enumerateDevices) {
    return { supported: false, audioInputs: 0, audioOutputs: 0, videoInputs: 0, labelsExposed: false };
  }
  try {
    const devices = await navigator.mediaDevices.enumerateDevices();
    return {
      supported: true,
      audioInputs: devices.filter((device) => device.kind === "audioinput").length,
      audioOutputs: devices.filter((device) => device.kind === "audiooutput").length,
      videoInputs: devices.filter((device) => device.kind === "videoinput").length,
      labelsExposed: devices.some((device) => Boolean(device.label))
    };
  } catch (error) {
    return { supported: true, audioInputs: 0, audioOutputs: 0, videoInputs: 0, labelsExposed: false, error: String(error?.message || error) };
  }
}

function parseIceCandidate(candidate) {
  const line = String(candidate?.candidate || "");
  if (!line) return null;
  const parts = line.trim().split(/\s+/);
  const typeIndex = parts.indexOf("typ");
  return {
    protocol: String(candidate.protocol || parts[2] || "unknown"),
    address: String(candidate.address || parts[4] || "unknown"),
    port: Number(candidate.port || parts[5] || 0),
    type: String(candidate.type || (typeIndex >= 0 ? parts[typeIndex + 1] : "unknown")),
    tcpType: String(candidate.tcpType || "")
  };
}

async function collectIceCandidates() {
  if (typeof RTCPeerConnection !== "function") return { supported: false, candidates: [] };
  const connection = new RTCPeerConnection({ iceServers: [] });
  const candidates = [];
  try {
    connection.createDataChannel("venom-network-probe");
    connection.addEventListener("icecandidate", (event) => {
      const parsed = parseIceCandidate(event.candidate);
      if (parsed && !candidates.some((item) => JSON.stringify(item) === JSON.stringify(parsed))) candidates.push(parsed);
    });
    const offer = await connection.createOffer();
    await connection.setLocalDescription(offer);
    await new Promise((resolve) => {
      const timer = setTimeout(resolve, 900);
      connection.addEventListener("icegatheringstatechange", () => {
        if (connection.iceGatheringState === "complete") {
          clearTimeout(timer);
          resolve();
        }
      });
    });
    return { supported: true, candidates: candidates.slice(0, 12) };
  } catch (error) {
    return { supported: true, candidates: [], error: String(error?.message || error) };
  } finally {
    connection.close();
  }
}

async function collectServerNetwork() {
  const endpoint = document.querySelector('meta[name="venom-network-endpoint"]')?.content?.trim();
  if (!endpoint) return { available: false, status: "No server-observed network endpoint configured" };
  try {
    const response = await fetch(endpoint, { credentials: "same-origin", cache: "no-store", headers: { Accept: "application/json" } });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    return {
      available: true,
      address: String(data.ip || data.address || ""),
      asn: String(data.asn || ""),
      organization: String(data.organization || data.org || ""),
      country: String(data.country || data.countryCode || ""),
      region: String(data.region || ""),
      city: String(data.city || "")
    };
  } catch (error) {
    return { available: false, status: String(error?.message || error) };
  }
}

async function collectNetwork() {
  const connection = navigator.connection || navigator.mozConnection || navigator.webkitConnection;
  const [ice, server] = await Promise.all([collectIceCandidates(), collectServerNetwork()]);
  const candidateTypes = [...new Set(ice.candidates.map((item) => item.type))];
  return {
    online: Boolean(navigator.onLine),
    origin: location.origin,
    protocol: location.protocol,
    host: location.host,
    secureContext: Boolean(window.isSecureContext),
    connectionApiAvailable: Boolean(connection),
    effectiveType: String(connection?.effectiveType || "unavailable"),
    type: String(connection?.type || "unavailable"),
    downlink: Number(connection?.downlink ?? -1),
    downlinkMax: Number(connection?.downlinkMax ?? -1),
    rtt: Number(connection?.rtt ?? -1),
    saveData: Boolean(connection?.saveData),
    webRtcSupported: Boolean(ice.supported),
    iceCandidates: ice.candidates,
    iceCandidateTypes: candidateTypes,
    publicAddressAvailable: Boolean(server.available && server.address),
    serverObserved: server
  };
}

function collectAutomation(browser, permissions) {
  const userAgent = String(browser.userAgent || "").toLowerCase();
  const automationGlobals = COMMON_AUTOMATION_GLOBALS.filter((name) => safeValue(() => name in window, false));
  const notificationPermission = typeof Notification === "function" ? String(Notification.permission) : "unsupported";
  const queriedNotification = String(permissions.notifications || "unsupported");
  const normalizedNotification = notificationPermission === "default" ? "prompt" : notificationPermission;
  const permissionsMismatch = normalizedNotification !== "unsupported" && queriedNotification !== "unsupported" && normalizedNotification !== queriedNotification;
  const nativeSurfaceChecks = [
    ["querySelector", document.querySelector],
    ["getElementById", document.getElementById],
    ["fetch", window.fetch],
    ["setTimeout", window.setTimeout]
  ];
  const modifiedNativeSurfaces = nativeSurfaceChecks.filter(([, value]) => typeof value === "function" && !isNativeFunction(value)).map(([name]) => name);
  return {
    webdriver: Boolean(navigator.webdriver),
    headlessUserAgent: /headlesschrome|phantomjs|slimerjs/.test(userAgent),
    automationGlobals,
    outerDimensionsMissing: !browser.mobile && (window.outerWidth === 0 || window.outerHeight === 0),
    permissionsMismatch,
    notificationPermission,
    queriedNotificationPermission: queriedNotification,
    nativeSurfaceMismatch: modifiedNativeSurfaces.length >= 2,
    modifiedNativeSurfaces,
    chromeObjectPresent: Boolean(window.chrome),
    devtoolsSizeDelta: {
      width: Math.max(0, Number(window.outerWidth || 0) - Number(window.innerWidth || 0)),
      height: Math.max(0, Number(window.outerHeight || 0) - Number(window.innerHeight || 0))
    }
  };
}

async function collectTiming(sampleTarget = 42) {
  if (typeof requestAnimationFrame !== "function") {
    return { sampleCount: 0, frameMeanMs: 0, frameStdDevMs: 0, frameMinMs: 0, frameMaxMs: 0, longFrames: 0 };
  }
  const samples = [];
  let previous = performance.now();
  await new Promise((resolve) => {
    function frame(now) {
      samples.push(now - previous);
      previous = now;
      if (samples.length >= sampleTarget || document.hidden) resolve();
      else requestAnimationFrame(frame);
    }
    requestAnimationFrame(frame);
  });
  if (!samples.length) return { sampleCount: 0, frameMeanMs: 0, frameStdDevMs: 0, frameMinMs: 0, frameMaxMs: 0, longFrames: 0 };
  const mean = samples.reduce((sum, value) => sum + value, 0) / samples.length;
  const variance = samples.reduce((sum, value) => sum + ((value - mean) ** 2), 0) / samples.length;
  return {
    sampleCount: samples.length,
    frameMeanMs: Number(mean.toFixed(3)),
    frameStdDevMs: Number(Math.sqrt(variance).toFixed(3)),
    frameMinMs: Number(Math.min(...samples).toFixed(3)),
    frameMaxMs: Number(Math.max(...samples).toFixed(3)),
    longFrames: samples.filter((value) => value > 45).length,
    navigationType: String(performance.getEntriesByType?.("navigation")?.[0]?.type || "unknown"),
    timeOrigin: Math.round(performance.timeOrigin || 0)
  };
}

function createBehaviorTracker() {
  const startedAt = performance.now();
  const state = {
    pointerMoves: 0,
    pointerDistance: 0,
    clicks: 0,
    keyPresses: 0,
    scrollDistance: 0,
    trustedEvents: 0,
    syntheticEvents: 0,
    visibilityChanges: 0,
    focusMs: 0,
    pointerTypes: [],
    lastPointer: null,
    lastScrollY: window.scrollY,
    focusedAt: document.hasFocus() ? performance.now() : null,
    lastInteractionAt: null
  };

  const countTrust = (event) => {
    if (event.isTrusted) state.trustedEvents += 1;
    else state.syntheticEvents += 1;
    state.lastInteractionAt = performance.now();
  };

  window.addEventListener("pointermove", (event) => {
    countTrust(event);
    state.pointerMoves += 1;
    const point = { x: event.clientX, y: event.clientY };
    if (state.lastPointer) state.pointerDistance += Math.hypot(point.x - state.lastPointer.x, point.y - state.lastPointer.y);
    state.lastPointer = point;
    if (event.pointerType && !state.pointerTypes.includes(event.pointerType)) state.pointerTypes.push(event.pointerType);
  }, { passive: true });

  window.addEventListener("pointerdown", (event) => countTrust(event), { passive: true });
  window.addEventListener("click", (event) => { countTrust(event); state.clicks += 1; }, { passive: true });
  window.addEventListener("keydown", (event) => { countTrust(event); state.keyPresses += 1; }, { passive: true });
  window.addEventListener("scroll", (event) => {
    countTrust(event);
    state.scrollDistance += Math.abs(window.scrollY - state.lastScrollY);
    state.lastScrollY = window.scrollY;
  }, { passive: true });
  document.addEventListener("visibilitychange", () => { state.visibilityChanges += 1; });
  window.addEventListener("focus", () => { if (state.focusedAt === null) state.focusedAt = performance.now(); });
  window.addEventListener("blur", () => {
    if (state.focusedAt !== null) state.focusMs += performance.now() - state.focusedAt;
    state.focusedAt = null;
  });

  return {
    snapshot() {
      const now = performance.now();
      const liveFocus = state.focusedAt === null ? 0 : now - state.focusedAt;
      return {
        pointerMoves: state.pointerMoves,
        pointerDistance: Math.round(state.pointerDistance),
        clicks: state.clicks,
        keyPresses: state.keyPresses,
        scrollDistance: Math.round(state.scrollDistance),
        trustedEvents: state.trustedEvents,
        syntheticEvents: state.syntheticEvents,
        visibilityChanges: state.visibilityChanges,
        focusMs: Math.round(state.focusMs + liveFocus),
        pointerTypes: [...state.pointerTypes],
        idleMs: state.lastInteractionAt === null ? Math.round(now - startedAt) : Math.round(now - state.lastInteractionAt)
      };
    },
    reset() {
      state.pointerMoves = 0;
      state.pointerDistance = 0;
      state.clicks = 0;
      state.keyPresses = 0;
      state.scrollDistance = 0;
      state.trustedEvents = 0;
      state.syntheticEvents = 0;
      state.visibilityChanges = 0;
      state.focusMs = 0;
      state.pointerTypes = [];
      state.lastPointer = null;
      state.lastScrollY = window.scrollY;
      state.focusedAt = document.hasFocus() ? performance.now() : null;
      state.lastInteractionAt = null;
    },
    startedAt
  };
}

async function collectFingerprint() {
  const collectedAt = new Date().toISOString();
  const userAgentData = await collectUserAgentData();
  const browser = collectBrowser(userAgentData);
  const [audio, storage, permissions, mediaDevices, network, timing] = await Promise.all([
    audioFingerprint(),
    collectStorage(),
    collectPermissions(),
    collectMediaDevices(),
    collectNetwork(),
    collectTiming()
  ]);
  const canvas = canvasFingerprint();
  const webgl = webglFingerprint();
  const fonts = detectFonts();
  const graphics = {
    ...webgl,
    canvasSupported: canvas.supported,
    canvasHash: canvas.hash,
    canvasSampleLength: canvas.sampleLength || 0,
    audioSupported: audio.supported,
    audioHash: audio.hash,
    audioMagnitude: audio.magnitude || 0,
    detectedFonts: fonts,
    detectedFontCount: fonts.length
  };
  return {
    collectedAt,
    browser,
    automation: collectAutomation(browser, permissions),
    hardware: collectHardware(),
    display: collectDisplay(),
    graphics,
    locale: collectLocale(),
    network,
    timing,
    storage,
    permissions,
    mediaDevices,
    features: {
      webAssembly: typeof WebAssembly === "object",
      webWorkers: typeof Worker === "function",
      serviceWorker: "serviceWorker" in navigator,
      sharedWorker: typeof SharedWorker === "function",
      sharedArrayBuffer: typeof SharedArrayBuffer === "function",
      webGpu: "gpu" in navigator,
      webCrypto: Boolean(globalThis.crypto?.subtle),
      notification: "Notification" in window,
      speechSynthesis: "speechSynthesis" in window,
      fileSystemAccess: "showOpenFilePicker" in window,
      paymentRequest: "PaymentRequest" in window,
      credentialsApi: "credentials" in navigator,
      userActivation: Boolean(navigator.userActivation)
    }
  };
}


Object.defineProperty(globalThis, "VenomFingerprint", {
  value: Object.freeze({ collectFingerprint, createBehaviorTracker }),
  configurable: false,
  enumerable: false,
  writable: false
});

})();
