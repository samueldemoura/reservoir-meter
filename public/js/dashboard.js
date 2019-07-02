// Constants
const UPDATE_DELAY = 5000;
const NUMBER_OF_DATA_POINTS = 10;

// Maps reservoir identifiers to their chart objects
const charts = [];

// List of reservoir identifiers
const ids = [];

/**
 * Creates the chart for the specified reservoir identifier in the page.
 * @param {*} id Reservoir identifier.
 */
function initializeChart(id) {
    const ctx = document.getElementById(`chart-${id}`).getContext('2d');

    // eslint-disable-next-line no-undef
    const chart = new Chart(ctx, {
        type: 'line',
        data: {
            labels: ['a', 'b', 'c', 'd', 'e'],
            datasets: [{
                label: 'Percentage full',
                data: [0, 33, 55, 67, 87],
                fill: 'start',
                backgroundColor: '#1e87f055',
                borderColor: '#1e87f0',
            }],
        },
        options: {
            scales: {
                xAxes: [{
                    type: 'time',
                    time: {
                        displayFormats: {
                            minute: 'h:mm a',
                        },
                    },
                    gridLines: {
                        color: 'rgba(0, 0, 0, 0.05)',
                    },
                }],

                yAxes: [{
                    ticks: {
                        beginAtZero: true,
                        max: 100,
                        callback: value => `${value}%`,
                    },
                    gridLines: {
                        color: 'rgba(0, 0, 0, 0.05)',
                    },
                }],
            },
        },
    });

    charts[id] = chart;
}

/**
 * Fetches reservoir history from API.
 * @param {*} id Reservoir identifier.
 * @param {*} quantity How many data points to fetch.
 */
function fetchData(id, quantity) {
    const url = `/api/data?id=${id}&quantity=${quantity}`;
    return fetch(url).then(
        resp => resp.text(),
    );
}

/**
 * Updates chart of the specified reservoir.
 * @param {*} id Reservoir identifier.
 */
function updateChart(id) {
    fetchData(id, NUMBER_OF_DATA_POINTS).then((resp) => {
        const max = document.getElementById(`max-${id}`).placeholder;

        // Update chart
        const respData = JSON.parse(resp).map(element => JSON.parse(element));
        const chart = charts[id];
        chart.data.labels = respData.map(x => x[0]);
        chart.data.datasets[0].data = respData.map(x => ((x[1] / max) * 100).toFixed(2));

        chart.update();

        // Update text and "progress" bar
        const percentage = respData[respData.length - 1][1] / max;
        document.getElementById(`progress-text-${id}`).innerHTML = `Latest reading: ${(percentage * 100).toFixed(1)}%`;
        document.getElementById(`progress-${id}`).value = percentage;
    });
}

/**
 * Updates all charts in the page.
 */
function updateAllCharts() {
    ids.forEach(id => updateChart(id));
}

window.addEventListener('DOMContentLoaded', () => {
    // Initialize all charts
    Array.from(document.getElementsByTagName('canvas')).forEach(async (element) => {
        const id = element.id.substring(6);
        ids.push(id);

        initializeChart(id);
        updateChart(id);
    });

    setInterval(() => {
        updateAllCharts();
    }, UPDATE_DELAY);
});
