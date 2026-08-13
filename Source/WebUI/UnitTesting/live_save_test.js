(function()
{
    const baseFilename = "webui_live_save_recovery";
    const copiedFilename = baseFilename + "_copy";
    const alerts = [];
    const originalAlert = window.alert;
    let resultElement = null;


    function delay(milliseconds)
    {
        return new Promise(resolve => setTimeout(resolve, milliseconds));
    }


    async function waitUntil(description, predicate, timeout = 10000)
    {
        const deadline = Date.now() + timeout;
        while(Date.now() < deadline)
        {
            if(predicate())
                return;
            await delay(20);
        }
        throw new Error("Timed out waiting for " + description);
    }


    function require(condition, message)
    {
        if(!condition)
            throw new Error(message);
    }


    function setResult(status, message)
    {
        if(!resultElement)
        {
            resultElement = document.createElement("output");
            resultElement.id = "webui_live_save_test_result";
            resultElement.style.position = "fixed";
            resultElement.style.left = "16px";
            resultElement.style.bottom = "16px";
            resultElement.style.zIndex = "100000";
            resultElement.style.padding = "12px";
            resultElement.style.background = status === "pass" ? "#174d2a" : "#641f1f";
            resultElement.style.color = "white";
            document.body.appendChild(resultElement);
        }
        resultElement.dataset.status = status;
        resultElement.textContent = message;
    }


    function selectUserFile(filename)
    {
        dialog.showUserFileList();
        const select = document.getElementById("open_dialog_user_items");
        require(select, "Open user-file list is unavailable.");
        for(let index = 0; index < select.options.length; index++)
            if(select.options[index].text === filename)
            {
                select.selectedIndex = index;
                return;
            }
        throw new Error(`Saved test model "${filename}" was not listed.`);
    }


    async function saveCurrentNetwork(filename)
    {
        network.network.filename = filename;
        controller.saveNetwork();
        await waitUntil("save completion", () => !controller.save_in_flight);
    }


    async function run()
    {
        window.alert = function(message)
        {
            alerts.push(String(message));
        };
        setResult("running", "WebUI live save recovery test running");

        await waitUntil("the initial network", () =>
            network && network.network &&
            controller && controller.session_id !== undefined &&
            !controller.update_in_flight
        );

        controller.webui_test_fail_save_remaining = 0;
        await saveCurrentNetwork(baseFilename);
        require(alerts.length === 0, "Initial save unexpectedly failed: " + alerts.join(" | "));

        controller.open();
        await waitUntil("injected /files failure", () => alerts.length === 1);
        require(
            alerts[0].includes("injected WebUI /files test failure"),
            "Unexpected /files failure message: " + alerts[0]
        );
        require(!document.getElementById("open_dialog").open, "Failed Open dialog remained visible.");
        require(dialog.openCallback !== null, "Open callback disappeared before replacement.");

        controller.webui_test_fail_files_remaining = 0;
        controller.open();
        await waitUntil("Open dialog", () => document.getElementById("open_dialog").open);
        selectUserFile(baseFilename);
        dialog.confirmOpen();
        await waitUntil("opened model", () =>
            !document.getElementById("open_dialog").open &&
            !controller.update_in_flight
        );

        controller.webui_test_fail_save_remaining = 1;
        controller.saveas();
        await waitUntil("Save As dialog", () => document.getElementById("save_dialog").open);
        const filenameInput = document.getElementById("save_dialog_filename");
        require(filenameInput, "Save As filename input is unavailable.");
        filenameInput.value = copiedFilename;
        dialog.confirmSave();
        await waitUntil("injected /save failure", () => alerts.length === 2);
        require(
            alerts[1].includes("HTTP 503 Service Unavailable"),
            "Unexpected /save failure message: " + alerts[1]
        );
        require(!controller.save_in_flight, "Failed Save As left save_in_flight set.");
        require(!controller.save_pending, "Failed Save As left save_pending set.");
        require(dialog.openCallback === null, "Save As retained the superseded Open callback.");

        await saveCurrentNetwork(copiedFilename);
        require(alerts.length === 2, "Retry produced an unexpected alert.");
        require(network.network.filename === copiedFilename, "Retry returned the wrong filename.");

        controller.open();
        await waitUntil("verification Open dialog", () => document.getElementById("open_dialog").open);
        selectUserFile(copiedFilename);
        dialog.cancelOpen();

        setResult("pass", "PASS: Open -> failed Save As -> retry recovered");
    }


    run()
    .catch(error => {
        console.error("WebUI live save recovery test failed:", error);
        setResult("fail", "FAIL: " + (error && error.message ? error.message : String(error)));
    })
    .finally(() => {
        window.alert = originalAlert;
    });
})();
