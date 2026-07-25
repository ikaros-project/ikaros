const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");


function deferred()
{
    let resolve;
    let reject;
    const promise = new Promise((resolvePromise, rejectPromise) => {
        resolve = resolvePromise;
        reject = rejectPromise;
    });
    return {promise, resolve, reject};
}


async function flushPromises()
{
    for(let i = 0; i < 4; i++)
        await Promise.resolve();
}


async function testStaleOpenDoesNotReplaceSave()
{
    const openFiles = deferred();
    const saveFiles = deferred();
    const requests = [openFiles.promise, saveFiles.promise];
    const filenameInput = {
        value: "race_fixed",
        focus() {},
        select() {},
    };
    const saveSelect = {
        options: [],
        selectedIndex: -1,
    };
    let openShowCount = 0;
    let saveShowCount = 0;
    let saveCloseCount = 0;
    let openCallbackCount = 0;
    let saveCallbackCount = 0;
    let savePopulateCount = 0;
    const alerts = [];

    const elements = {
        open_dialog: {
            open: false,
            close() {},
            showModal()
            {
                openShowCount++;
                this.open = true;
            },
        },
        save_dialog: {
            open: false,
            close()
            {
                saveCloseCount++;
                this.open = false;
            },
            showModal()
            {
                saveShowCount++;
                this.open = true;
            },
        },
        save_dialog_filename: filenameInput,
        save_dialog_user_items: saveSelect,
    };
    const controller = {
        session_id: 1,
        client_id: 2,
        filelist: {cached: true},
        open_mode: true,
    };
    const context = vm.createContext({
        alert(message)
        {
            alerts.push(message);
        },
        console,
        controller,
        document: {
            getElementById(id)
            {
                return elements[id] || null;
            },
            querySelector()
            {
                return null;
            },
        },
        fetch()
        {
            return requests.shift();
        },
        setTimeout(callback)
        {
            callback();
        },
    });
    const dialogPath = path.resolve(__dirname, "../ui/dialog.js");
    const source = fs.readFileSync(dialogPath, "utf8") +
        "\nglobalThis.dialogUnderTest = dialog;\n";
    vm.runInContext(source, context, {filename: dialogPath});
    const dialog = context.dialogUnderTest;

    dialog.configureOpenDialogSources = function() {};
    dialog.setOpenDialogButtonText = function() {};
    dialog.populateFileList = function() {};
    dialog.showUserFileList = function() {};
    dialog.showSystemFileList = function() {};
    dialog.showExamplesFileList = function() {};
    dialog.selectDialogOption = function() {};
    dialog.displayMessage = function() {};
    dialog.populateSaveFileList = function()
    {
        savePopulateCount++;
    };
    dialog.setSaveDialogButtonText = function() {};
    dialog.showUserSaveFileList = function() {};

    dialog.showOpenDialog(function() {
        openCallbackCount++;
    }, "Open");
    dialog.showSaveDialog(function() {
        saveCallbackCount++;
    }, "Save");

    assert.equal(controller.open_mode, false);
    assert.equal(saveShowCount, 1);

    const staleFiles = {user_files: ["stale.ikg"]};
    openFiles.resolve({
        ok: true,
        json()
        {
            return Promise.resolve(staleFiles);
        },
    });
    await flushPromises();

    assert.equal(openShowCount, 0);
    assert.notEqual(controller.filelist, staleFiles);

    const currentFiles = {user_files: ["current.ikg"]};
    saveFiles.resolve({
        ok: true,
        json()
        {
            return Promise.resolve(currentFiles);
        },
    });
    await flushPromises();

    assert.equal(controller.filelist, currentFiles);
    assert.equal(savePopulateCount, 2);

    dialog.confirmSave();
    dialog.confirmSave();

    assert.equal(saveCloseCount, 2);
    assert.equal(openCallbackCount, 0);
    assert.equal(saveCallbackCount, 1);
    assert.deepEqual(alerts, []);
}


testStaleOpenDoesNotReplaceSave()
.then(() => {
    console.log("dialog tests passed");
})
.catch(error => {
    console.error(error);
    process.exitCode = 1;
});
