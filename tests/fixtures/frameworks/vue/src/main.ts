// @venom: browser
import { protectedGreeting } from "./service";
export const app = { async mount(el: HTMLElement) { el.textContent = await protectedGreeting("Vue"); } };
