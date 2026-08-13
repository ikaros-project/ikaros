const dialog =
{
    requestGeneration: 0,
    openCallback: null,
    saveCallback: null,
    listSelectCallback: null,
    infoDialogBound: false,

    fetchFileList()
    {
        if(typeof controller.consumeWebUITestFailure === "function" &&
           controller.consumeWebUITestFailure("files"))
            return new Promise((resolve, reject) => {
                setTimeout(function()
                {
                    reject(new Error("injected WebUI /files test failure"));
                }, controller.webui_test_delay_ms);
            });

        return fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if(!response.ok)
                throw new Error("HTTP error " + response.status);
            return response.json();
        });
    },

    reportFileListError(error)
    {
        const message = error && error.message ? error.message : String(error);
        console.error("Could not get file list from server:", error);
        alert("Could not get file list from server: " + message);
    },

    setActiveFileSource(prefix, activeType)
    {
        const systemButton = document.getElementById(`${prefix}_system_file_button`) || (prefix === "open" ? document.getElementById("system_file_button") : null);
        const userButton = document.getElementById(`${prefix}_user_file_button`) || (prefix === "open" ? document.getElementById("user_file_button") : null);
        const examplesButton = document.getElementById(`${prefix}_examples_file_button`) || (prefix === "open" ? document.getElementById("examples_file_button") : null);
        const systemItems = document.getElementById(`${prefix}_dialog_system_items`);
        const userItems = document.getElementById(`${prefix}_dialog_user_items`);
        const examplesItems = document.getElementById(`${prefix}_dialog_examples_items`);

        if(systemButton)
            systemButton.classList.toggle("selected", activeType === "system");
        if(userButton)
            userButton.classList.toggle("selected", activeType === "user");
        if(examplesButton)
            examplesButton.classList.toggle("selected", activeType === "examples");
        if(systemItems)
            systemItems.style.display = activeType === "system" ? "block" : "none";
        if(userItems)
            userItems.style.display = activeType === "user" ? "block" : "none";
        if(examplesItems)
            examplesItems.style.display = activeType === "examples" ? "block" : "none";
    },

    populateDialogOptions(items, files, confirmHandler)
    {
        if(!items)
            return;
        items.innerHTML = '';
        files.forEach(file => {
            if(file == null || String(file).trim() === "")
                return;
            const opt = document.createElement('option');
            opt.value = file;
            opt.text = file;
            opt.addEventListener('dblclick', confirmHandler, false);
            items.appendChild(opt);
        });
    },

    confirmOpen()
    {
        let text = "";
        let dialogType = "";
        try
        {
            dialogType = this.getDialogType();
            const sel = document.getElementById(`open_dialog_${dialogType}_items`);
            if(!sel || sel.selectedIndex < 0)
                throw new Error("No file is selected.");
            text = sel.options[sel.selectedIndex].text;
            const openDialog = document.getElementById('open_dialog');
            if(!openDialog)
                throw new Error("Open dialog is unavailable.");
            this.requestGeneration++;
            openDialog.close(text);
        }
        catch(err)
        {
            const message = err && err.message ? err.message : String(err);
            console.error("Could not confirm Open dialog:", err);
            alert("Could not confirm Open dialog: " + message);
            return;
        }

        try
        {
            const callback = this.openCallback;
            this.openCallback = null;
            if(callback)
                callback(text, dialogType);
        }
        catch(err)
        {
            const message = err && err.message ? err.message : String(err);
            console.error("Could not start opening file:", err);
            alert("Could not start opening file: " + message);
        }
    },

    cancelOpen()
    {
        this.requestGeneration++;
        this.openCallback = null;
        controller.open_mode = false;
        const openDialog = document.getElementById('open_dialog');
        if(openDialog)
            openDialog.close(null);
    },

    showOpenDialog(callback, message)
    {
        const options = arguments.length > 2 && arguments[2] ? arguments[2] : {};
        const requestGeneration = ++this.requestGeneration;
        this.saveCallback = null;
        this.listSelectCallback = null;
        this.openCallback = callback;
        this.configureOpenDialogSources(options);
        this.setOpenDialogButtonText(options.confirmLabel || "Open");
        dialog.fetchFileList()
        .then(json => {
            if(requestGeneration !== this.requestGeneration)
                return;
            controller.filelist = json;
            this.populateFileList(json, options);
            if(options.userOnly)
                this.showUserFileList();
            else if(json.examples_files && json.examples_files.length > 0)
                this.showExamplesFileList();
            else
                this.showSystemFileList();
            this.selectDialogOption("open_dialog_user_items", options.defaultFilename || "");
            this.displayMessage(message);
            const openDialog = document.getElementById('open_dialog');
            if(openDialog && !openDialog.open)
                openDialog.showModal();
        })
        .catch(error => {
            if(requestGeneration !== this.requestGeneration)
                return;
            controller.open_mode = false;
            this.reportFileListError(error);
        });
    },

    showSaveDialog(callback, message)
    {
        const options = arguments.length > 2 && arguments[2] ? arguments[2] : {};
        const requestGeneration = ++this.requestGeneration;
        controller.open_mode = false;
        this.openCallback = null;
        this.listSelectCallback = null;
        this.saveCallback = callback;
        const saveDialog = document.getElementById('save_dialog');
        this.populateSaveFileList(controller.filelist || {}, options);
        this.setSaveDialogButtonText(options.confirmLabel || "Save");
        const filenameLabel = document.querySelector("label[for='save_dialog_filename']");
        if(filenameLabel)
            filenameLabel.innerText = options.filenameLabel || "Save as:";
        if(message)
        {
            const saveTitle = document.getElementById('save_dialog_title');
            if(saveTitle)
                saveTitle.innerText = message;
        }
        const filenameInput = document.getElementById("save_dialog_filename");
        if(filenameInput)
        {
            filenameInput.value = options.defaultFilename || "Untitled";
            filenameInput.onkeydown = function(evt)
            {
                if(evt.key == "Enter")
                {
                    evt.preventDefault();
                    dialog.confirmSave();
                }
            };
        }
        this.showUserSaveFileList();
        if(saveDialog && !saveDialog.open)
            saveDialog.showModal();
        if(filenameInput)
        {
            setTimeout(function()
            {
                filenameInput.focus();
                filenameInput.select();
            }, 0);
        }

        dialog.fetchFileList()
        .then(json => {
            if(requestGeneration !== this.requestGeneration)
                return;
            controller.filelist = json;
            this.populateSaveFileList(json, options);
            this.showUserSaveFileList();
            this.selectDialogOption("save_dialog_user_items", options.defaultFilename || "");
        })
        .catch(error => {
            if(requestGeneration !== this.requestGeneration)
                return;
            this.reportFileListError(error);
        });
    },

    configureOpenDialogSources(options)
    {
        const examplesButton = document.getElementById("examples_file_button");
        const systemButton = document.getElementById("system_file_button");
        const userButton = document.getElementById("user_file_button");
        const userOnly = options && options.userOnly;
        if(examplesButton)
            examplesButton.disabled = !!userOnly;
        if(systemButton)
            systemButton.disabled = !!userOnly;
        if(userButton)
            userButton.disabled = false;
    },

    setOpenDialogButtonText(text)
    {
        const button = document.querySelector("#open_dialog menu button:last-child");
        if(button)
            button.innerText = text;
    },

    setSaveDialogButtonText(text)
    {
        const button = document.querySelector("#save_dialog menu button:last-child");
        if(button)
            button.innerText = text;
    },

    selectDialogOption(selectId, value)
    {
        if(!value)
            return;
        const sel = document.getElementById(selectId);
        if(!sel)
            return;
        for(let i=0; i<sel.options.length; i++)
            if(sel.options[i].text === value || sel.options[i].value === value)
            {
                sel.selectedIndex = i;
                return;
            }
    },

    getDialogType()
    {
        const sys = document.getElementById("open_dialog_system_items");
        const examples = document.getElementById("open_dialog_examples_items");
        if(sys && sys.style.display === 'block')
            return 'system';
        if(examples && examples.style.display === 'block')
            return 'examples';
        return 'user';
    },

    populateFileList(file_list, options)
    {
        if(options && options.filesKey)
        {
            this.populateOptions('system', []);
            this.populateOptions('examples', []);
            this.populateOptions('user', file_list[options.filesKey] || []);
            return;
        }
        this.populateOptions('system', file_list.system_files);
        this.populateOptions('user', file_list.user_files);
        this.populateOptions('examples', file_list.examples_files);
    },

    populateOptions(type, files)
    {
        if(files)
            this.populateDialogOptions(
                document.getElementById(`open_dialog_${type}_items`),
                files,
                function() { dialog.confirmOpen(); }
            );
    },

    populateSaveFileList(file_list, options)
    {
        if(options && options.filesKey)
        {
            this.populateSaveOptions('system', []);
            this.populateSaveOptions('user', file_list[options.filesKey] || []);
            return;
        }
        this.populateSaveOptions('system', file_list.system_files || []);
        this.populateSaveOptions('user', file_list.user_files || []);
    },

    populateSaveOptions(type, files)
    {
        const items = document.getElementById(`save_dialog_${type}_items`);
        if(!items)
            return;
        this.populateDialogOptions(items, files, function() { dialog.confirmSave(); });
        items.selectedIndex = -1;
        items.oninput = dialog.syncSaveFilenameFromSelection;
        items.onchange = dialog.syncSaveFilenameFromSelection;
    },

    getActiveSaveSelect()
    {
        const dialogType = this.getSaveDialogType();
        return document.getElementById(`save_dialog_${dialogType}_items`);
    },

    syncSaveFilenameFromSelection()
    {
        const sel = this.getActiveSaveSelect();
        const filenameInput = document.getElementById("save_dialog_filename");
        if(!sel || !filenameInput)
            return;
        if(sel.selectedIndex >= 0)
            filenameInput.value = sel.options[sel.selectedIndex].text;
    },

    getSaveDialogType()
    {
        const sys = document.getElementById("save_dialog_system_items");
        return sys && sys.style.display === 'block' ? 'system' : 'user';
    },

    showSystemSaveFileList()
    {
        const systemButton = document.getElementById("save_system_file_button");
        if(systemButton && systemButton.disabled)
            return;
        this.setActiveFileSource("save", "system");
        this.syncSaveFilenameFromSelection();
    },

    showUserSaveFileList()
    {
        this.setActiveFileSource("save", "user");
        this.syncSaveFilenameFromSelection();
    },

    confirmSave()
    {
        let text = "";
        let dialogType = "";
        try
        {
            dialogType = this.getSaveDialogType();
            const sel = document.getElementById(`save_dialog_${dialogType}_items`);
            const filenameInput = document.getElementById("save_dialog_filename");
            text = filenameInput ? filenameInput.value.trim() : "";
            if(!text && sel)
            {
                if(sel.selectedIndex < 0 && sel.options.length > 0)
                    sel.selectedIndex = 0;
                text = sel.selectedIndex >= 0 ? sel.options[sel.selectedIndex].text : "";
            }
            if(!text)
                return;
            const saveDialog = document.getElementById('save_dialog');
            if(!saveDialog)
                throw new Error("Save As dialog is unavailable.");
            this.requestGeneration++;
            saveDialog.close(text);
        }
        catch(err)
        {
            const message = err && err.message ? err.message : String(err);
            console.error("Could not confirm Save As dialog:", err);
            alert("Could not confirm Save As dialog: " + message);
            return;
        }

        try
        {
            const callback = this.saveCallback;
            this.saveCallback = null;
            if(callback)
                callback(text, dialogType);
        }
        catch(err)
        {
            const message = err && err.message ? err.message : String(err);
            console.error("Could not start saving file:", err);
            alert("Could not start saving file: " + message);
        }
    },

    cancelSave()
    {
        this.requestGeneration++;
        this.saveCallback = null;
        const saveDialog = document.getElementById('save_dialog');
        if(saveDialog)
            saveDialog.close(null);
    },

    displayMessage(message)
    {
        if(message)
            document.getElementById('open_dialog_title').innerText = message;
    },

    showSystemFileList()
    {
        this.setActiveFileSource("open", "system");
    },

    showUserFileList()
    {
        this.setActiveFileSource("open", "user");
    },

    showExamplesFileList()
    {
        this.setActiveFileSource("open", "examples");
    },

    confirmListSelect()
    {
        let sel = document.getElementById("listSelectDialogItems");
        if(sel.selectedIndex < 0)
            return;

        let text = sel.options[sel.selectedIndex].text;
        const listSelectDialog = document.getElementById('list_select_dialog');
        dialog.requestGeneration++;
        if(listSelectDialog)
            listSelectDialog.close(text);
        const callback = dialog.listSelectCallback;
        dialog.listSelectCallback = null;
        if(callback)
            callback(text);
    },

    cancelListSelect()
    {
        dialog.requestGeneration++;
        dialog.listSelectCallback = null;
        const listSelectDialog = document.getElementById('list_select_dialog');
        if(listSelectDialog)
            listSelectDialog.close(null);
    },

    showListSelectDialog(list, callback, message)
    {
        dialog.requestGeneration++;
        controller.open_mode = false;
        dialog.openCallback = null;
        dialog.saveCallback = null;
        dialog.listSelectCallback = callback;
        const listSelectDialog = document.getElementById('list_select_dialog');
        let sel = document.getElementById('listSelectDialogItems');
        sel.innerHTML = '';
        if(list)
            for(const i of list.split(","))
            {
                if(i === "")
                    continue;

                const opt = document.createElement('option');
                opt.value = i;
                opt.textContent = i;
                document.getElementById('listSelectDialogItems').appendChild(opt);
            }
        if(message)
            document.getElementById('listSelectDialogTitle').innerText = message;
        sel.selectedIndex = sel.options.length > 0 ? 0 : -1;
        sel.ondblclick = function() { dialog.confirmListSelect(); };
        sel.onkeydown = function(event)
        {
            if(event.key === "Enter")
                dialog.confirmListSelect();
        };
        if(listSelectDialog && !listSelectDialog.open)
            listSelectDialog.showModal();
        setTimeout(function() { sel.focus(); }, 0);
    },

    showInfoDialog(message, title="Info", as_html=false)
    {
        const window = document.getElementById("info_dialog");
        if(!window)
            return;
        document.getElementById("info_dialog_title").innerText = title;
        const content = document.getElementById("info_dialog_content");
        if(as_html)
            content.innerHTML = message;
        else
            content.textContent = message;

        if(!dialog.infoDialogBound)
        {
            window.addEventListener("click", function(evt) {
                if(evt.target === window)
                    dialog.closeInfo();
            });
            window.addEventListener("cancel", function(evt) {
                evt.preventDefault();
                dialog.closeInfo();
            });
            dialog.infoDialogBound = true;
        }
        window.showModal();
    },

    closeInfo()
    {
        const window = document.getElementById("info_dialog");
        if(window)
            window.close();
    }
};
