export interface PricingRequest {
  basePrice: number;
  quantity: number;
  customerTier?: "standard" | "pro" | "enterprise";
  coupon?: string;
}

export interface PricingResult {
  subtotal: number;
  discount: number;
  tax: number;
  total: number;
  riskScore: number;
  tier: string;
  parityCheck: "even" | "odd";
  engine: string;
}

export type Tier = "standard" | "pro" | "enterprise";

export interface RuntimeIdentity {
  protected: boolean;
  language: string;
  features: string[];
}
