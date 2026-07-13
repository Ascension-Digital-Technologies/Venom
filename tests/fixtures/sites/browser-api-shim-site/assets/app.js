const directBeacon = navigator.sendBeacon();
const windowBeacon = window.navigator.sendBeacon();
const globalBeacon = globalThis.navigator.sendBeacon();
const selfBeacon = self.navigator.sendBeacon();
globalThis.__venomBeaconShim = [directBeacon, windowBeacon, globalBeacon, selfBeacon].join(',');

const directLegacyMedia = navigator.getUserMedia();
const windowLegacyMedia = window.navigator.webkitGetUserMedia();
const globalLegacyMedia = globalThis.navigator.mozGetUserMedia();
const selfLegacyMedia = self.navigator.msGetUserMedia();
globalThis.__venomMediaShim = [directLegacyMedia, windowLegacyMedia, globalLegacyMedia, selfLegacyMedia].join(',');

const modernMediaPromise = navigator.mediaDevices.getUserMedia();
globalThis.__venomModernMediaShim = modernMediaPromise && typeof modernMediaPromise.then === 'function' ? 'promise' : String(modernMediaPromise);
console.log('browser host api shim script loaded');

const directFp = fpCollect();
const windowFp = window.fpCollect();
const globalFp = globalThis.fpCollect();
const selfFp = self.fpCollect();
globalThis.__venomMissingGlobalShim = [directFp, windowFp, globalFp, selfFp].map((value) => value === null ? 'null' : String(value)).join(',');
