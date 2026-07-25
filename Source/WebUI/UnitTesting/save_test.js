const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");


function loadController(context)
{
    const controllerPath = path.resolve(__dirname, "../core/controller.js");
    const source = fs.readFileSync(controllerPath, "utf8") +
        "\nglobalThis.controllerUnderTest = controller;\n";
    vm.runInContext(source, context, {filename: controllerPath});
    return context.controllerUnderTest;
}


function createHarness()
{
    const alerts = [];
    const errors = [];
    const requests = [];
    const timers = [];
    let setViewModeCount = 0;
    let networkRefreshCount = 0;
    let tainted = true;

    class FakeXMLHttpRequest
    {
        constructor()
        {
            this.headers = {};
            this.responseText = "";
            this.status = 0;
            this.statusText = "";
            requests.push(this);
        }

        open(method, url, asynchronous)
        {
            this.method = method;
            this.url = url;
            this.asynchronous = asynchronous;
        }

        setRequestHeader(name, value)
        {
            this.headers[name] = value;
        }

        send(body)
        {
            this.body = body;
        }

        respond(status, responseText, statusText = "")
        {
            this.status = status;
            this.statusText = statusText;
            this.responseText = responseText;
            this.onload();
        }

        failNetwork()
        {
            this.onerror();
        }

        timeOut()
        {
            this.ontimeout();
        }
    }

    const network = {
        network: {
            filename: "save_recovery",
            name: "SaveRecovery",
        },
        tainted: true,
    };
    const main = {
        savePaneLayout() {},
        setViewMode()
        {
            setViewModeCount++;
        },
    };
    const document = {
        activeElement: null,
        body: {},
        querySelector()
        {
            return null;
        },
    };
    const context = vm.createContext({
        alert(message)
        {
            alerts.push(message);
        },
        clearInterval() {},
        clearTimeout() {},
        console: {
            error(...args)
            {
                errors.push(args);
            },
            log() {},
        },
        Date,
        deepCopy(value)
        {
            return JSON.parse(JSON.stringify(value));
        },
        document,
        main,
        network,
        setInterval() {},
        setTimeout(callback, delay)
        {
            timers.push({callback, delay});
            return timers.length;
        },
        URLSearchParams,
        window: {
            location: {search: ""},
        },
        XMLHttpRequest: FakeXMLHttpRequest,
    });
    const controller = loadController(context);

    controller.get = function(url)
    {
        if(url === "network")
            networkRefreshCount++;
    };
    controller.save = function()
    {
        controller.saveNetwork();
    };
    controller.setTainted = function(value)
    {
        tainted = !!value;
        network.tainted = tainted;
    };

    return {
        alerts,
        controller,
        errors,
        main,
        network,
        requests,
        timers,
        getNetworkRefreshCount()
        {
            return networkRefreshCount;
        },
        getSetViewModeCount()
        {
            return setViewModeCount;
        },
        getTainted()
        {
            return tainted;
        },
    };
}


function runNextTimer(harness, expectedDelay)
{
    const timer = harness.timers.shift();
    assert.ok(timer, "expected a pending retry timer");
    assert.equal(timer.delay, expectedDelay);
    timer.callback();
}


function respondSuccessfully(request, filename = "save_recovery")
{
    request.respond(200, JSON.stringify({filename}));
}


function testPendingSaveRecoversAfterHttpFailure()
{
    const harness = createHarness();
    const {controller, requests} = harness;

    controller.saveNetwork();
    assert.equal(requests.length, 1);
    assert.equal(controller.save_in_flight, true);

    controller.saveNetwork();
    assert.equal(requests.length, 1);
    assert.equal(controller.save_pending, true);

    requests[0].respond(500, "injected failure", "Internal Server Error");

    assert.equal(harness.alerts.length, 1);
    assert.match(harness.alerts[0], /HTTP 500 Internal Server Error: injected failure/);
    assert.equal(requests.length, 2);
    assert.equal(controller.save_in_flight, true);
    assert.equal(controller.save_pending, false);

    respondSuccessfully(requests[1], "recovered_after_http_failure");

    assert.equal(controller.save_in_flight, false);
    assert.equal(controller.save_pending, false);
    assert.equal(harness.network.network.filename, "recovered_after_http_failure");
    assert.equal(harness.getSetViewModeCount(), 1);
    assert.equal(harness.getNetworkRefreshCount(), 1);
    assert.equal(harness.getTainted(), false);
}


function testTransportRetriesDoNotPoisonSaveState()
{
    const harness = createHarness();
    const {controller, requests} = harness;

    controller.saveNetwork();
    requests[0].failNetwork();
    assert.equal(controller.save_in_flight, true);
    assert.equal(harness.alerts.length, 0);
    runNextTimer(harness, 150);

    requests[1].timeOut();
    assert.equal(controller.save_in_flight, true);
    assert.equal(harness.alerts.length, 0);
    runNextTimer(harness, 350);

    respondSuccessfully(requests[2], "recovered_after_transport_failure");

    assert.equal(controller.save_in_flight, false);
    assert.equal(controller.save_pending, false);
    assert.equal(harness.alerts.length, 0);
    assert.equal(harness.network.network.filename, "recovered_after_transport_failure");
    assert.equal(harness.getSetViewModeCount(), 1);
    assert.equal(harness.getNetworkRefreshCount(), 1);
    assert.equal(harness.getTainted(), false);
    assert.deepEqual(
        requests.map(request => request.body),
        [requests[0].body, requests[0].body, requests[0].body]
    );
    assert.match(requests[0].url, /-0$/);
    assert.match(requests[1].url, /-1$/);
    assert.match(requests[2].url, /-2$/);
}


function testRepeatedFailuresAlwaysAllowTheNextSave()
{
    const harness = createHarness();
    const {controller, requests} = harness;
    const cycles = 50;

    for(let cycle = 0; cycle < cycles; cycle++)
    {
        controller.saveNetwork();
        const failedRequest = requests[cycle * 2];
        failedRequest.respond(503, `injected failure ${cycle}`, "Service Unavailable");

        assert.equal(controller.save_in_flight, false);
        assert.equal(controller.save_pending, false);

        controller.saveNetwork();
        const successfulRequest = requests[cycle * 2 + 1];
        respondSuccessfully(successfulRequest);

        assert.equal(controller.save_in_flight, false);
        assert.equal(controller.save_pending, false);
    }

    assert.equal(harness.alerts.length, cycles);
    assert.equal(harness.getSetViewModeCount(), cycles);
    assert.equal(harness.getNetworkRefreshCount(), cycles);
    assert.equal(harness.getTainted(), false);
}


testPendingSaveRecoversAfterHttpFailure();
testTransportRetriesDoNotPoisonSaveState();
testRepeatedFailuresAlwaysAllowTheNextSave();
console.log("save tests passed");
