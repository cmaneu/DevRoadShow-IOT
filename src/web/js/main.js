function init() {
    var app = new Vue({
        el: '#devkit',
        data: {
            functionAppName: '',
            reportedTwin: {},
            deviceId:'AZ3166',
            lastUpdated: '',
            userLEDState: 0,
            rgbLEDState: 0,
            rgbColor: {
                r: 0,
                g: 0,
                b: 0
            },
            stateUpdateEndpoint: 'about:blank',
            loading: false,
            functionAppNameSet: false
        },
        computed: {
            ledStyle: function () {
                if (this.rgbLEDState === 1) {
                    let r = this.rgbColor.r;
                    let g = this.rgbColor.g;
                    let b = this.rgbColor.b;
                    return {
                        'background-color': `rgb(${r}, ${g}, ${b})`,
                        'box-shadow': `0px 0px 10px 2px rgb(${r}, ${g}, ${b})`
                    };
                }
                else
                    return {
                        'background-color': 'rgb(0, 0, 0)'
                    };
            }
        },
        methods: {
            getTwin: getTwin,
            getState: getState,
            updateUserLEDState: updateUserLEDState,
            updateRgbLEDState: updateRgbLEDState,
            updateRgbLEDColor: updateRgbLEDColor
        }
    });
}

function getTwin() {
    var scope = this;
    var timespan = new Date().getTime();
    var url = `https://${this.functionAppName}.azurewebsites.net/api/devkit-state?action=get&t=${timespan}`;
    this.$http.jsonp(url).then(function(data){
        return data.json();
    }).then(function (data){
        scope.loading = false;
        scope.reportedTwin = data.reported;
        scope.userLEDState = data.desired.userLEDState || 0;
        scope.rgbLEDState = data.desired.rgbLEDState || 0;
        scope.rgbColor.r = data.desired.rgbLEDR || 0;
// TODO : Add support for other colors
        scope.lastUpdated = new Date(scope.reportedTwin.$metadata.$lastUpdated).toLocaleString('en-GB', {hour12: false}).replace(',', '');
    });
}

function updateUserLEDState() {
    var timespan = new Date().getTime();
    var state = this.userLEDState === 1 ? 0 : 1;
    this.stateUpdateEndpoint = `https://${this.functionAppName}.azurewebsites.net/api/devkit-state?action=set&state=${state}&key=userLED&t=${timespan}`;
    setTimeout(this.getTwin, 1000);
}

function updateRgbLEDState() {
    var timespan = new Date().getTime();
    var state = this.rgbLEDState === 1 ? 0 : 1;
    this.stateUpdateEndpoint = `https://${this.functionAppName}.azurewebsites.net/api/devkit-state?action=set&state=${state}&key=rgbLED&t=${timespan}`;
    setTimeout(this.getTwin, 1000);
    if (state === 1 && this.rgbColor.r === 0 && this.rgbColor.g === 0 && this.rgbColor.b === 0) {
        this.rgbColor.r = 255;
        setTimeout(updateRgbLEDColor.bind(this), 500);
    }
}

function updateRgbLEDColor() {
    var timespan = new Date().getTime();
    var state = this.rgbColor;
    this.stateUpdateEndpoint = `https://${this.functionAppName}.azurewebsites.net/api/devkit-state?action=set&state=${state.r},${state.g},${state.b}&key=rgbLEDColor&t=${timespan}`;
    setTimeout(this.getTwin, 1000);
}

function getState() {
    if (!this.functionAppName)
    {
        alert('No function name found.');
        return;
    }
    this.functionAppNameSet = true;
    this.loading = true;
    setInterval(this.getTwin, 5000);
}

init();
