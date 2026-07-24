// @venom: browser
export default function formatMoney(value) {
    return new Intl.NumberFormat("en-US", { style: "currency", currency: "USD" }).format(value);
}
