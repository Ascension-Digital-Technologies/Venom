import type { VenomCallOptions, VenomRuntimeError, VenomRuntimeStatus } from "@venom/runtime";

export interface UseVenomRuntimeOptions {
  autoInitialize?: boolean;
  timeout?: number;
}

export interface VenomReactRuntimeStatus extends VenomRuntimeStatus {
  readonly error: unknown;
  readonly initialize: typeof import("@venom/runtime").initializeVenom;
}

export interface ProtectedCallDefaults<Input, Output> extends VenomCallOptions {
  initialData?: Output;
  onSuccess?: (data: Output, input: Input) => void;
  onError?: (error: unknown, input: Input) => void;
}

export interface ProtectedCallState<Input, Output> {
  readonly execute: (input: Input, options?: VenomCallOptions) => Promise<Output>;
  readonly cancel: () => void;
  readonly reset: () => void;
  readonly pending: boolean;
  readonly data: Output | undefined;
  readonly error: unknown;
  readonly invocation: number;
}

export interface ProtectedPreloadState {
  readonly pending: boolean;
  readonly ready: readonly string[];
  readonly error: unknown;
}

export function useVenomRuntime(options?: UseVenomRuntimeOptions): VenomReactRuntimeStatus;
export function useProtectedCall<Input = unknown, Output = unknown>(name: string, defaults?: ProtectedCallDefaults<Input, Output>): ProtectedCallState<Input, Output>;
export function useProtectedPreload(names: string | readonly string[], options?: VenomCallOptions): ProtectedPreloadState;
export function createProtectedHooks<const Names extends readonly string[]>(names: Names): Readonly<{
  useCall<Input = unknown, Output = unknown>(name: Names[number], defaults?: ProtectedCallDefaults<Input, Output>): ProtectedCallState<Input, Output>;
  usePreload(options?: VenomCallOptions): ProtectedPreloadState;
  exports: readonly Names[number][];
}>;
export { isVenomRuntimeError } from "@venom/runtime";
export type { VenomRuntimeError };
