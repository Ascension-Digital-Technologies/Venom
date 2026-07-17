// @venom: browser
export const Fragment = Symbol("Fragment");
export function createElement(type, props, ...children) { return { type, props: props ?? {}, children }; }
function append(value, parent) {
  if (value == null || value === false || value === true) return;
  if (typeof value === "string" || typeof value === "number") { parent.append(document.createTextNode(String(value))); return; }
  if (Array.isArray(value)) { value.forEach(item => append(item, parent)); return; }
  if (value.type === Fragment) { value.children.forEach(item => append(item, parent)); return; }
  if (typeof value.type === "function") { append(value.type({ ...value.props, children: value.children }), parent); return; }
  const node = document.createElement(value.type);
  for (const [key, item] of Object.entries(value.props)) {
    if (key === "className") node.setAttribute("class", String(item));
    else if (key === "htmlFor") node.setAttribute("for", String(item));
    else if (key.startsWith("on") && typeof item === "function") node.addEventListener(key.slice(2).toLowerCase(), item);
    else if (item !== false && item != null) node.setAttribute(key, item === true ? "" : String(item));
  }
  value.children.forEach(item => append(item, node));
  parent.append(node);
}
export function render(value, parent) { parent.replaceChildren(); append(value, parent); }
const React = { createElement, Fragment };
export default React;
