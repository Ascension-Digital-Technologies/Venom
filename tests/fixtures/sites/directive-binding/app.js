// @venom: browser

// explanatory comment between directive and declaration
/* another harmless comment */
function browserHelper(value) { return value + 1; }

// @venom: protected
// documentation for the protected function
export async function protectedValue(value) { return value * 17; }

document.body.dataset.result = String(await protectedValue(browserHelper(2)));
