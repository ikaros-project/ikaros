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


function loadDialog(context)
{
    const dialogPath = path.resolve(__dirname, "../ui/dialog.js");
    const source = fs.readFileSync(dialogPath, "utf8") +
        "\nglobalThis.dialogUnderTest = dialog;\n";
    vm.runInContext(source, context, {filename: dialogPath});
    return context.dialogUnderTest;
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
    const dialog = loadDialog(context);

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


async function testStaleSaveDoesNotReplaceOpen()
{
    const saveFiles = deferred();
    const openFiles = deferred();
    const requests = [saveFiles.promise, openFiles.promise];
    const filenameInput = {
        value: "save_pending",
        focus() {},
        select() {},
    };
    const openSelect = {
        options: [{text: "current.ikg"}],
        selectedIndex: 0,
    };
    let openShowCount = 0;
    let openCallbackCount = 0;
    let saveCallbackCount = 0;
    const alerts = [];

    const elements = {
        open_dialog: {
            open: false,
            close()
            {
                this.open = false;
            },
            showModal()
            {
                openShowCount++;
                this.open = true;
            },
        },
        open_dialog_user_items: openSelect,
        save_dialog: {
            open: false,
            close()
            {
                this.open = false;
            },
            showModal()
            {
                this.open = true;
            },
        },
        save_dialog_filename: filenameInput,
    };
    const controller = {
        session_id: 1,
        client_id: 2,
        filelist: {cached: true},
        open_mode: false,
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
    const dialog = loadDialog(context);

    dialog.configureOpenDialogSources = function() {};
    dialog.setOpenDialogButtonText = function() {};
    dialog.populateFileList = function() {};
    dialog.showUserFileList = function() {};
    dialog.showSystemFileList = function() {};
    dialog.showExamplesFileList = function() {};
    dialog.selectDialogOption = function() {};
    dialog.displayMessage = function() {};
    dialog.populateSaveFileList = function() {};
    dialog.setSaveDialogButtonText = function() {};
    dialog.showUserSaveFileList = function() {};

    dialog.showSaveDialog(function() {
        saveCallbackCount++;
    }, "Save");
    controller.open_mode = true;
    dialog.showOpenDialog(function() {
        openCallbackCount++;
    }, "Open");

    const staleFiles = {user_files: ["stale.ikg"]};
    saveFiles.resolve({
        ok: true,
        json()
        {
            return Promise.resolve(staleFiles);
        },
    });
    await flushPromises();

    assert.notEqual(controller.filelist, staleFiles);

    const currentFiles = {user_files: ["current.ikg"]};
    openFiles.resolve({
        ok: true,
        json()
        {
            return Promise.resolve(currentFiles);
        },
    });
    await flushPromises();

    assert.equal(controller.filelist, currentFiles);
    assert.equal(openShowCount, 1);

    dialog.confirmOpen();
    dialog.confirmOpen();

    assert.equal(openCallbackCount, 1);
    assert.equal(saveCallbackCount, 0);
    assert.deepEqual(alerts, []);
}


async function testListSelectionSupersedesPendingOpen()
{
    const openFiles = deferred();
    const listSelect = {
        options: [],
        selectedIndex: -1,
        appendChild(option)
        {
            this.options.push(option);
        },
        focus() {},
    };
    let openShowCount = 0;
    let openCallbackCount = 0;
    let listCallbackCount = 0;
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
        list_select_dialog: {
            open: false,
            close()
            {
                this.open = false;
            },
            showModal()
            {
                this.open = true;
            },
        },
        listSelectDialogItems: listSelect,
        listSelectDialogTitle: {innerText: ""},
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
            createElement()
            {
                return {};
            },
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
            return openFiles.promise;
        },
        setTimeout(callback)
        {
            callback();
        },
    });
    const dialog = loadDialog(context);

    dialog.configureOpenDialogSources = function() {};
    dialog.setOpenDialogButtonText = function() {};

    dialog.showOpenDialog(function() {
        openCallbackCount++;
    }, "Open");
    dialog.showListSelectDialog("one,two", function() {
        listCallbackCount++;
    }, "Select");

    assert.equal(controller.open_mode, false);
    assert.equal(listSelect.selectedIndex, 0);

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

    dialog.confirmListSelect();
    dialog.confirmListSelect();

    assert.equal(openCallbackCount, 0);
    assert.equal(listCallbackCount, 1);
    assert.deepEqual(alerts, []);
}


async function testFileListFailureDoesNotPoisonNextSave()
{
    const failedFiles = deferred();
    const recoveredFiles = deferred();
    const requests = [failedFiles.promise, recoveredFiles.promise];
    const filenameInput = {
        value: "recovered_after_file_list_failure",
        focus() {},
        select() {},
    };
    let openShowCount = 0;
    let saveShowCount = 0;
    let saveCallbackCount = 0;
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
                this.open = false;
            },
            showModal()
            {
                saveShowCount++;
                this.open = true;
            },
        },
        save_dialog_filename: filenameInput,
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
        console: {
            error() {},
        },
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
    const dialog = loadDialog(context);

    dialog.configureOpenDialogSources = function() {};
    dialog.setOpenDialogButtonText = function() {};
    dialog.populateFileList = function() {};
    dialog.showUserFileList = function() {};
    dialog.showSystemFileList = function() {};
    dialog.showExamplesFileList = function() {};
    dialog.selectDialogOption = function() {};
    dialog.displayMessage = function() {};
    dialog.populateSaveFileList = function() {};
    dialog.setSaveDialogButtonText = function() {};
    dialog.showUserSaveFileList = function() {};

    dialog.showOpenDialog(function() {}, "Open");
    failedFiles.reject(new Error("injected /files failure"));
    await flushPromises();

    assert.equal(openShowCount, 0);
    assert.equal(controller.open_mode, false);
    assert.deepEqual(alerts, [
        "Could not get file list from server: injected /files failure",
    ]);

    dialog.showSaveDialog(function() {
        saveCallbackCount++;
    }, "Save");
    recoveredFiles.resolve({
        ok: true,
        json()
        {
            return Promise.resolve({user_files: ["recovered.ikg"]});
        },
    });
    await flushPromises();

    assert.equal(saveShowCount, 1);
    dialog.confirmSave();
    assert.equal(saveCallbackCount, 1);
    assert.equal(dialog.openCallback, null);
    assert.equal(dialog.saveCallback, null);
}


testStaleOpenDoesNotReplaceSave()
.then(testStaleSaveDoesNotReplaceOpen)
.then(testListSelectionSupersedesPendingOpen)
.then(testFileListFailureDoesNotPoisonNextSave)
.then(() => {
    console.log("dialog tests passed");
})
.catch(error => {
    console.error(error);
    process.exitCode = 1;
});
