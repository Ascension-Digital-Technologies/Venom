import React, { render } from "./jsx-runtime";
import { calculate } from "../protected/pricing";
function Card({ title, amount, tags = [] }) {
    return React.createElement("section",{"className":"card","data-kind":"quote"},React.createElement("h1",null,(title)),React.createElement("strong",null,"$",(amount.toFixed(2))),React.createElement(React.Fragment,null,(tags.map(tag => React.createElement("span",{"className":"tag"},(tag))))));
}
const root = document.querySelector("#app");
const cardProps = { title: "Protected total", amount: await calculate(24.5, 4) };
render(React.createElement(Card,{...(cardProps),"tags":(["TSX", "TurboJS"])}), root);
