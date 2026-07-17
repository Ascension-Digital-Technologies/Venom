// @venom: browser
window.phase1Ready = true;

// @venom: protected
const secretAlgorithm = async (value) => {
  const marker = "PHASE1_SECRET_ALGORITHM_MARKER_9f8b7c6d";
  return value * 1337 + marker.length;
};
window.phase1Value = await secretAlgorithm(2);
