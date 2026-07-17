import React, { render } from "./jsx-runtime";
import { calculate } from "../protected/pricing";
interface CardProps { title: string; amount: number; tags?: string[] }
function Card({ title, amount, tags = [] }: CardProps) {
  return <section className="card" data-kind="quote">
    <h1>{title}</h1>
    <strong>${amount.toFixed(2)}</strong>
    <>{tags.map(tag => <span className="tag">{tag}</span>)}</>
  </section>;
}
const root = document.querySelector<HTMLElement>("#app")!;
const cardProps: CardProps = { title: "Protected total", amount: await calculate(24.5, 4) };
render(<Card {...cardProps} tags={["TSX", "QuickJS"]} />, root);
