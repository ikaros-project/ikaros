const dialog =
{
    window: null,
    callback: null,
    infoDialogBound: false,

    confirmOpen()
    {
        try {
            const dialogType = this.getDialogType();
            const sel = document.getElementById(`open_dialog_${dialogType}_items`);
            const text = sel.options[sel.selectedIndex].text;
            this.window.close(text);
            if(this.callback)
                this.callback(text, dialogType);
        }
        catch(err)
        {
            alert("Error opening file");
        }
    },

    cancelOpen()
    {
        controller.open_mode = false;
        this.window.close(null);
    },

    showOpenDialog(callback, message)
    {
        fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if(!response.ok) {
                alert("Could not get file list from server.");
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            controller.filelist = json;
            this.setupDialog(callback);
            this.populateFileList(controller.filelist);
            this.displayMessage(message);
            this.window.showModal();
        })
        .catch(function() {
            alert("Could not get file list from server.");
            console.log("Could not get file list from server.");
        });
    },

    showSaveDialog(callback, message)
    {
        fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if(!response.ok) {
                alert("Could not get file list from server.");
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            controller.filelist = json;
            this.callback = callback;
            this.window = document.getElementById('save_dialog');
            this.populateSaveFileList(controller.filelist);
            if(message)
            {
                const saveTitle = document.getElementById('save_dialog_title');
                if(saveTitle)
                    saveTitle.innerText = message;
            }
            const filenameInput = document.getElementById("save_dialog_filename");
            if(filenameInput)
            {
                filenameInput.value = "Untitled";
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
            this.window.showModal();
            if(filenameInput)
            {
                setTimeout(function()
                {
                    filenameInput.focus();
                    filenameInput.select();
                }, 0);
            }
        })
        .catch(function() {
            alert("Could not get file list from server.");
            console.log("Could not get file list from server.");
        });
    },

    getDialogType()
    {
        const sys = document.getElementById("open_dialog_system_items");
        return sys.style.display === 'block' ? 'system' : 'user';
    },

    setupDialog(callback)
    {
        this.callback = callback;
        this.window = document.getElementById('open_dialog');
    },

    populateFileList(file_list)
    {
        this.populateOptions('system', file_list.system_files);
        this.populateOptions('user', file_list.user_files);
    },

    populateOptions(type, files)
    {
        if(files)
        {
            let items = document.getElementById(`open_dialog_${type}_items`);
            items.innerHTML = '';
            files.forEach(file => {
                const opt = document.createElement('option');
                opt.value = file;
                opt.text = file;
                opt.addEventListener('dblclick', function() {
                    dialog.confirmOpen();
                }, false);
                items.appendChild(opt);
            });
        }
    },

    populateSaveFileList(file_list)
    {
        this.populateSaveOptions('system', file_list.system_files || []);
        this.populateSaveOptions('user', file_list.user_files || []);
    },

    populateSaveOptions(type, files)
    {
        const items = document.getElementById(`save_dialog_${type}_items`);
        if(!items)
            return;
        items.innerHTML = '';
        files.forEach(file => {
            const opt = document.createElement('option');
            opt.value = file;
            opt.text = file;
            opt.addEventListener('dblclick', function() {
                dialog.confirmSave();
            }, false);
            items.appendChild(opt);
        });
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
        document.getElementById('save_system_file_button').classList.add("selected");
        document.getElementById('save_user_file_button').classList.remove("selected");
        document.getElementById('save_dialog_system_items').style.display='block';
        document.getElementById('save_dialog_user_items').style.display='none';
        this.syncSaveFilenameFromSelection();
    },

    showUserSaveFileList()
    {
        document.getElementById('save_system_file_button').classList.remove("selected");
        document.getElementById('save_user_file_button').classList.add("selected");
        document.getElementById('save_dialog_system_items').style.display='none';
        document.getElementById('save_dialog_user_items').style.display='block';
        this.syncSaveFilenameFromSelection();
    },

    confirmSave()
    {
        try {
            const dialogType = this.getSaveDialogType();
            const sel = document.getElementById(`save_dialog_${dialogType}_items`);
            const filenameInput = document.getElementById("save_dialog_filename");
            let text = filenameInput ? filenameInput.value.trim() : "";
            if(!text && sel)
            {
                if(sel.selectedIndex < 0 && sel.options.length > 0)
                    sel.selectedIndex = 0;
                text = sel.selectedIndex >= 0 ? sel.options[sel.selectedIndex].text : "";
            }
            if(!text)
                return;
            this.window.close(text);
            if(this.callback)
                this.callback(text, dialogType);
        }
        catch(err)
        {
            alert("Error saving file");
        }
    },

    cancelSave()
    {
        if(this.window)
            this.window.close(null);
    },

    displayMessage(message)
    {
        if(message)
            document.getElementById('open_dialog_title').innerText = message;
    },

    showSystemFileList()
    {
        document.getElementById('system_file_button').classList.add("selected");
        document.getElementById('user_file_button').classList.remove("selected");
        document.getElementById('open_dialog_system_items').style.display='block';
        document.getElementById('open_dialog_user_items').style.display='none';
    },

    showUserFileList()
    {
        document.getElementById('system_file_button').classList.remove("selected");
        document.getElementById('user_file_button').classList.add("selected");
        document.getElementById('open_dialog_system_items').style.display='none';
        document.getElementById('open_dialog_user_items').style.display='block';
    },

    confirmListSelect()
    {
        let sel = document.getElementById("listSelectDialogItems");
        let text = sel.options[sel.selectedIndex].text;
        dialog.window.close(text);
        if(dialog.callback)
            dialog.callback(text);
    },

    cancelListSelect()
    {
        dialog.window.close(null);
    },

    showListSelectDialog(list, callback, message)
    {
        dialog.callback = callback;
        dialog.window = document.getElementById('list_select_dialog');
        let sel = document.getElementById('listSelectDialogItems');
        sel.innerHTML = '';
        if(list)
            for(const i of list.split(","))
            {
                const opt = document.createElement('option');
                opt.value = i;
                opt.innerHTML = i;
                document.getElementById('listSelectDialogItems').appendChild(opt);
            }
        if(message)
            document.getElementById('listSelectDialogTitle').innerText = message;
        dialog.window.showModal();
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
