export const VENOM_RUNTIME_API_VERSION: 1;

export type VenomRuntimeState = "uninitialized" | "initializing" | "ready" | "error" | "disposed";

export interface VenomCallOptions {
  timeout?: number;
  signal?: AbortSignal;
}

export interface VenomInitializeOptions extends VenomCallOptions {}

export interface VenomBatchCall<Input = unknown> {
  name: string;
  input: Input;
  options?: VenomCallOptions;
}

export interface VenomRuntimeInfo {
  runtimeApiVersion?: number;
  protocol?: number;
  transport?: string;
  exports?: string[];
  [key: string]: unknown;
}

export interface VenomPublicRuntimeApi {
  readonly apiVersion?: number;
  ready(): Promise<VenomPublicRuntimeApi>;
  call<Input = unknown, Output = unknown>(name: string, input: Input, options?: VenomCallOptions): Promise<Output>;
  batch?<Output = unknown>(calls: VenomBatchCall[], options?: VenomCallOptions): Promise<Output[]>;
  preload?(names: string[], options?: VenomCallOptions): Promise<unknown>;
  status?(): { state: VenomRuntimeState; [key: string]: unknown };
  dispose?(reason?: string): Promise<unknown> | unknown;
  info(): VenomRuntimeInfo;
  readonly exports: Record<string, (input: unknown, options?: VenomCallOptions) => Promise<unknown>>;
}

export interface VenomRuntimeStatus {
  readonly state: VenomRuntimeState;
  readonly apiVersion: number;
  readonly ready: boolean;
  readonly disposed: boolean;
  readonly exports: readonly string[];
  readonly transport: string | null;
  readonly lastError: unknown;
}

export class VenomRuntimeError extends Error {
  readonly code: string;
  readonly phase: string;
  readonly recoverable: boolean;
  readonly details?: Readonly<Record<string, unknown>>;
  constructor(code: string, message: string, options?: { phase?: string; recoverable?: boolean; details?: Record<string, unknown>; cause?: unknown });
}

export function isVenomRuntimeError(value: unknown): value is VenomRuntimeError;
export function initializeVenom(options?: VenomInitializeOptions): Promise<VenomPublicRuntimeApi>;
export function callProtected<Input = unknown, Output = unknown>(name: string, input: Input, options?: VenomCallOptions): Promise<Output>;
export function batchProtected<Output = unknown>(calls: VenomBatchCall[], options?: VenomCallOptions): Promise<Output[]>;
export function preloadProtected(names: string | readonly string[], options?: VenomCallOptions): Promise<unknown>;
export function getRuntimeStatus(): VenomRuntimeStatus;
export function disposeRuntime(reason?: string): Promise<VenomRuntimeStatus>;
export function createProtectedClient<T extends Record<string, (...args: any[]) => any>>(names?: readonly (keyof T & string)[]): T;

export const venomRuntime: Readonly<{
  initialize: typeof initializeVenom;
  call: typeof callProtected;
  batch: typeof batchProtected;
  preload: typeof preloadProtected;
  status: typeof getRuntimeStatus;
  dispose: typeof disposeRuntime;
  createClient: typeof createProtectedClient;
}>;

export default venomRuntime;

declare global {
  var venom: VenomPublicRuntimeApi | undefined;
}
