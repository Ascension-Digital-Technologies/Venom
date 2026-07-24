// @venom: browser
import React from "../jsx-runtime";
export default function Toast({ message }) {
    if (!message)
        return null;
    return React.createElement("div",{"className":"toast"},React.createElement("span",null,"✓"),(message));
}
