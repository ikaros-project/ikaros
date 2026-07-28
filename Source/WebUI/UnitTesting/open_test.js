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
    const timers = [];
    const clearedTimers = [];
    const overlays = [];
    const requests = [];

    class FakeXMLHttpRequest
    {
        constructor()
        {
            requests.push(this);
        }

        open(method, url)
        {
            this.method = method;
            this.url = url;
        }

        setRequestHeader() {}
        send() {}
        getResponseHeader() { return null; }
    }

    const main = {
        showOpeningOverlay(filename)
        {
            overlays.push({type: "opening", filename});
        },
        hideReconnectOverlay()
        {
            overlays.push({type: "hidden"});
        },
        showReconnectOverlay()
        {
            overlays.push({type: "reconnecting"});
        },
    };
    const context = vm.createContext({
        auth: {handleUnauthorized() {}},
        clearInterval() {},
        clearTimeout(id)
        {
            clearedTimers.push(id);
        },
        console,
        Date,
        document: {
            querySelector() { return null; },
        },
        main,
        network: {tainted: false},
        setInterval() { return 1; },
        setTimeout(callback, delay)
        {
            const timer = {id: timers.length + 1, callback, delay};
            timers.push(timer);
            return timer.id;
        },
        URLSearchParams,
        window: {location: {search: ""}},
        XMLHttpRequest: FakeXMLHttpRequest,
    });
    const controller = loadController(context);
    controller.session_id = 1;
    controller.client_id = 2;
    controller.setSystemInfo = function() {};
    return {clearedTimers, controller, overlays, requests, timers};
}


function testOpeningFeedbackIsDelayedAndCancelled()
{
    const harness = createHarness();
    harness.controller.open_mode = true;
    harness.controller.beginOpeningFeedback("QuickModel");

    assert.equal(harness.timers.length, 1);
    assert.equal(harness.timers[0].delay, 3000);
    assert.deepEqual(harness.overlays, [{type: "hidden"}]);

    harness.controller.endOpeningFeedback();
    assert.deepEqual(harness.clearedTimers, [1]);
    assert.deepEqual(harness.overlays, [{type: "hidden"}, {type: "hidden"}]);
}


function testSlowOpeningShowsNamedOverlay()
{
    const harness = createHarness();
    harness.controller.open_mode = true;
    harness.controller.beginOpeningFeedback("SlowModel");
    harness.timers[0].callback();

    assert.deepEqual(harness.overlays, [
        {type: "hidden"},
        {type: "opening", filename: "SlowModel"},
    ]);
}


function testOpenRequestUsesLongTimeoutAndReconnectsAfterFailure()
{
    const harness = createHarness();
    harness.controller.open_mode = true;
    harness.controller.get("open?where=user&file=SlowModel", function() {});
    const request = harness.requests[0];

    assert.equal(request.timeout, 30000);
    request.ontimeout();
    assert.equal(harness.controller.open_mode, false);
    assert.deepEqual(harness.overlays.slice(-2), [
        {type: "hidden"},
        {type: "reconnecting"},
    ]);
}


function testOrdinaryRequestKeepsShortTimeout()
{
    const harness = createHarness();
    harness.controller.get("update", function() {});
    assert.equal(harness.requests[0].timeout, 2000);
}


function testSupersededUpdateTimeoutDoesNotReconnect()
{
    const harness = createHarness();
    harness.controller.get("update", function() {});
    const request = harness.requests[0];

    harness.controller.update_generation++;
    request.ontimeout();

    assert.equal(harness.controller.update_in_flight, false);
    assert.deepEqual(harness.overlays, []);
    assert.equal(harness.controller.reconnect_timer, null);
}


testOpeningFeedbackIsDelayedAndCancelled();
testSlowOpeningShowsNamedOverlay();
testOpenRequestUsesLongTimeoutAndReconnectsAfterFailure();
testOrdinaryRequestKeepsShortTimeout();
testSupersededUpdateTimeoutDoesNotReconnect();
console.log("WebUI opening tests passed");
