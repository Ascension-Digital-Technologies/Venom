// @venom: browser
export function createState(snapshot) {
    return {
        view: "overview",
        query: "",
        region: "All regions",
        snapshot,
        overview: null,
        scored: [],
        trend: [],
        forecast: null,
        report: null,
        loading: true,
        commandOpen: false,
        selectedIncident: null,
        toast: ""
    };
}
