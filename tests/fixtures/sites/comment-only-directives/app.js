// @venom: browser

const documentation = "// @venom: protected";
const template = `/* @venom: protected */`;
function mustStayBrowser(value) {
  return value + documentation.length + template.length;
}

/*
 * @venom: protected
 */
export async function actuallyProtected(value) {
  const marker = "A15_COMMENT_ONLY_DIRECTIVE";
  return value * 11 + marker.length;
}

const result = await actuallyProtected(mustStayBrowser(2));
document.body.dataset.result = String(result);
