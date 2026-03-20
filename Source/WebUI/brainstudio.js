/*
 * New version of WebUI interface with BrainStudio functions
 */

const widget_classes = 
[
    "bar-graph",
    "plot",
    "table",
    "marker",
    "path",
    "grid",
    "image",
    "text",
    "rectangle",
    "button",
    "slider-horizontal",
    "drop-down-menu",
    "control-grid",
    "canvas3d",
    "epi-head",
    "key-points"
]

const identifier = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.0123456789";


function isEmpty(obj) 
{
    if(obj)
        for (const prop in obj) 
        {
            if (Object.hasOwn(obj, prop)) 
                return false;
        }
    return true;
}

function deepCopy(source) 
{
    if (source === null || typeof source !== 'object') 
      return source;
  
    if (Array.isArray(source)) {
      return source.map(item => deepCopy(item));
    }
  
    const newObject = {};
    for (const key in source) {
      newObject[key] = deepCopy(source[key]);
    }
    return newObject;
}


function replaceProperties(target, source) 
{
    // Remove all properties from the target object
    for (const key in target) {
      if (Object.hasOwnProperty.call(target, key)) {
        delete target[key];
      }
    }
  
    // Copy properties from the source object to the target object
    for (const key in source)
        if (Object.hasOwnProperty.call(source, key)) 
            target[key] = deepCopy(source[key]);
}
  
  function toggleStrings(array, toggleItems)
    {
        toggleItems.forEach(item => 
        {
            const index = array.indexOf(item);
            if (index === -1)
                array.push(item);
            else 
                array.splice(index, 1);
        });
  }


  function removeStringFromStart(mainString, stringToRemove)
  {
    if (mainString.startsWith(stringToRemove))
        return mainString.slice(stringToRemove.length);
    return mainString;
  }


    String.prototype.rsplit = function(sep, maxsplit) {
    let split = this.split(sep || /\s+/);
    return maxsplit ? [ split.slice(0, -maxsplit).join(sep) ].concat(split.slice(-maxsplit)) : split;
}



function splitAtLastDot(str) {
    const lastDotIndex = str.lastIndexOf('.');
    if (lastDotIndex === -1) {
        return [str, ""]; // No dot found, return original string and empty string
    }
    
    const beforeDot = str.substring(0, lastDotIndex);
    const afterDot = str.substring(lastDotIndex + 1); // +1 to skip the dot
    return [beforeDot, afterDot];
}



function toURLParams(params) {
    return Object.keys(params).map((key) => {
        //console.log(key);
        return encodeURIComponent(key) + '=' + encodeURIComponent(params[key]);
    }).join('&');
}


function setType(x, t)
{

    if(t == 'int')
        return parseInt(x);
    
    if(t == 'float')
        return parseFloat(x);
    
    if(t == 'bool')
        return ['on','yes','true'].includes(x.toString().toLowerCase());
    
    return x;
};

function zeroPad(x)
{
    if(x <10)
        return "0"+x;
    else
        return String(x);
}

function secondsToHMS(d)
{
    try
    {
        if(d=="-")
            return "-";

        d = Number(d);
        if(isNaN(d) || d < 0)
            return "-";

        const h = Math.floor(d / 3600);
        const m = Math.floor(d % 3600 / 60);
        const s = Math.floor(d % 3600 % 60);
        return h+":"+zeroPad(m)+":"+zeroPad(s);
    }
    catch(err)
    {
        return "-";
    }
}

function formatTime(d)
{
    try
    {
        if(d == "-")
            return "-";
        if(d < 0)
            return "-";
        else if(d < 0.001)
            return Math.round(1000000*d) + " &#181;s";
        else if(d < 5)
            return Math.round(1000*d) + " ms";
        else
            return secondsToHMS(d);
    }
    catch(e)
    {
        return "-";
    }
}

function changeNameInPath(path, name)
{
    const ix = path.lastIndexOf('.');
    if (ix === -1)
        return name;
    return path.substring(0, ix + 1) + name;
}


function nameInPath(path)
{
    const ix = path.lastIndexOf('.');
    if (ix === -1)
        return "";
    return path.substring(ix + 1);
}

function parentPath(path)
{
    const p = path.split('.');
    p.pop();
    return p.join('.');
    
}

function getStringUpToBracket(str)
{
    const index = str.indexOf('[');
    if (index === -1) {
      return str;
    }
    return str.substring(0, index);
}


function getStringAfterBracket(str)
{
    const index = str.indexOf('[');
    if (index === -1) {
      return "";
    }
    return str.substring(index, str.length);
}

function setCookie(name,value,days=100)
{
    let date = new Date();
    date.setTime(date.getTime()+(days?days:1)*86400000);
    let expires = "; expires="+date.toGMTString();
    document.cookie = name+"="+value+expires+"; path=/";
}

function getCookie(name)
{
    let nameEQ = name + "=";
    const ca = document.cookie.split(';');
    for(let i=0;i < ca.length;i++) {
        let c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
    }
    return null;
}

function eraseCookie(name)
{
    createCookie(name,"",-1);
}

function resetCookies()
{
    setCookie('selected_background', "");
}



let dialog = 
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
            
            if (this.callback) {
                this.callback(text, dialogType);
            }
        } 
        catch (err) 
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
            if (!response.ok) {
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
        .catch(function () {
            alert("Could not get file list from server.");
            console.log("Could not get file list from server.");
        })
    },

    showSaveDialog(callback, message)
    {
        fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
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
        .catch(function () {
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
        if (files) 
        {
            let items = document.getElementById(`open_dialog_${type}_items`);
            items.innerHTML = '';
            files.forEach(file => {
                const opt = document.createElement('option');
                opt.value = file;
                opt.text = file;
                opt.ondblclick = function() {
                    dialog.confirmOpen();
                };
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
            opt.ondblclick = function() {
                dialog.confirmSave();
            };
            items.appendChild(opt);
        });
        items.selectedIndex = -1;
        items.oninput = function()
        {
            dialog.syncSaveFilenameFromSelection();
        };
        items.onchange = function()
        {
            dialog.syncSaveFilenameFromSelection();
        };
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
        if (message) {
            document.getElementById('open_dialog_title').innerText = message;
        }
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

    // OLD DIALOG CODE BELOW

    confirmListSelect()
    {
        let sel = document.getElementById("listSelectDialogItems");
        let text= sel.options[sel.selectedIndex].text;
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
            for(i of list.split(",")) // FIXME: Remove blank space
            {
                var opt = document.createElement('option');
                opt.value = i;
                opt.innerHTML = i;
                document.getElementById('listSelectDialogItems').appendChild(opt);
            }
            if(message)
            {
                document.getElementById('listSelectDialogTitle').innerText = message;
            }
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

const network = 
{
    network: null,
    classes: null,
    classinfo: {},
    dict: {},
    tainted: false,
    component_count: 0,

    init(n)
    {
        this.network = n;
        this.ensureGroupAutoRouting(this.network);
        this.ensureConnectionDefaults(this.network);
        this.rebuildDict();
        this.component_count = Object.keys(this.dict).length+1;
    },

    hasAnyComponentPosition(group=this.network)
    {
        if(!group || typeof group !== "object")
            return false;

        const hasPosition = function(component)
        {
            if(!component || typeof component !== "object")
                return false;
            const x = parseFloat(component._x);
            const y = parseFloat(component._y);
            return Number.isFinite(x) && Number.isFinite(y);
        };

        const components = [
            ...(group.groups || []),
            ...(group.modules || []),
            ...(group.inputs || []),
            ...(group.outputs || []),
            ...(group.widgets || [])
        ];

        for(const component of components)
        {
            if(hasPosition(component))
                return true;
            if(component && component._tag === "group" && this.hasAnyComponentPosition(component))
                return true;
        }

        return false;
    },

    ensureGroupAutoRouting(group)
    {
        if(!group || typeof group !== "object")
            return;
        if(group.color === undefined || group.color === null || group.color === "")
            group.color = "black";
        if(group.auto_routing === undefined)
            group.auto_routing = false;
        for(const child of group.groups || [])
            this.ensureGroupAutoRouting(child);
    },

    ensureConnectionDefaults(group)
    {
        if(!group || typeof group !== "object")
            return;

        for(const connection of group.connections || [])
        {
            if(!connection || typeof connection !== "object")
                continue;
            if(connection.color === undefined || connection.color === null || connection.color === "")
                connection.color = "black";
            if(connection.line_type === undefined || connection.line_type === null || connection.line_type === "")
                connection.line_type = "auto_route";
        }

        for(const child of group.groups || [])
            this.ensureConnectionDefaults(child);
    },

    isUnique(name)  // test if name can be changed to this ******************************
    {
        return true;
    },

    uniqueID(name)
    {
        let bg = selector.selected_background;
        let i = 1;
        test_name = bg+'.'+name+i;
        while(test_name in network.dict)
            test_name = bg+'.'+name+(++i);
        return name+i;
    },

    rebuildDict()
    {
        this.dict = {};
        this.buildDict(this.network, "");
    },

 

    addComponentsToDict(path, components)
    {
        for(let c of components || [])
            this.dict[path+'.'+c.name] = c;
    },


    buildDict(o, path)
    {
        let new_path = (path+'.'+o.name); //.substring(1);
        if(new_path[0]=='.')
            new_path = new_path.substring(1);

        network.dict[new_path] = o;

        for(let g of o.groups || [])
            this.buildDict(g, new_path)

        for(let c of o.connections || [])
            this.dict[new_path+'.'+getStringUpToBracket(c.source)+'*'+new_path+'.'+getStringUpToBracket(c.target)] = c;

        this.addComponentsToDict(new_path, o.inputs);
        this.addComponentsToDict(new_path, o.outputs);
        this.addComponentsToDict(new_path, o.modules);
        this.addComponentsToDict(new_path, o.widgets);
    },



    newConnection(path, source, target)
    {
        const connection = 
        {
            _tag: "connection",
            source: source,
            target: target,
            delay: "1",
            color: "black",
            line_type: "auto_route"
        };
        let group = network.dict[path];
        if(group.connections == null)
            group.connections = [];
        group.connections.push(connection);
        network.dict[path+"."+source+"*"+path+"."+target]=connection;
    },


    getParentGroup(fullComponentName)
    {

    },


    pruneConnections()
    {
        // Handled in kernel
    },


    changeModuleClass(module, new_class)
    {
        let old_module = deepCopy(network.dict[module]);
        replaceProperties(network.dict[module], network.classinfo[new_class]);
        let new_module = network.dict[module];
        new_module._tag = "module";
        new_module.class = new_class;
        new_module.name = old_module.name;
        new_module._x = old_module._x;
        new_module._y = old_module._y;
        new_module.color = old_module.color;
        new_module.log_level = old_module.log_level;
        // TODO: Check that all properties are in the class
        network.dict[module] = new_module;
        // FIXME: Update existing connections if possible
    },

    changeWidgetClass(widget, new_class)
    {
        const old_widget = deepCopy(network.dict[widget]);
        let new_widget  = 
        {
            _tag: "widget",
            name: old_widget.name,
            title: old_widget.title,
            class: new_class,
            _x:old_widget._x,
            _y:old_widget._y,
            width: old_widget.width,
            height: old_widget.height,
            show_title: old_widget.show_title,
            show_frame: old_widget.show_frame,

            source: old_widget.source || "",
            min: (old_widget.min !== undefined ?  old_widget.min : ""),
            max: (old_widget.max !== undefined ?  old_widget.max : ""),
            select: old_widget.select || "",
        };


        replaceProperties(network.dict[widget], new_widget);
    },


    renameGroupOrModule(group, old_name, new_name)
    {
        network.dict[new_name] = network.dict[old_name];
        delete network.dict[old_name];

        // Update connections to and from this group or module
        let parent_path = parentPath(old_name);
        let parent_group = network.dict[group];
        let connection_parent = "";
        for(let c of parent_group.connections || [])
        {
            // Update if source (without output) == old_name
            connection_parent = parentPath(c.source);
            if(connection_parent == nameInPath(old_name))
            {
                let p = c.source.split('.');
                c.source = nameInPath(new_name)+'.'+p[1];
            }
            // Update of target == this
            connection_parent = parentPath(c.target);
            if(connection_parent == nameInPath(old_name))
            {
                let p = c.target.split('.');
                c.target = nameInPath(new_name)+'.'+p[1];
            }
        }
    },  

    renameInput(group, old_name, new_name) // FIXME: NEW PATH not NAME
    {
        for(let c of network.dict[group].connections || [])
        {
            if(group+'.'+c.source == old_name)
            {
                c.source = nameInPath(new_name); // FIXME: USE END OF NEW_NAME - changeNameInPath(selector.selected_background, inspector.item.name);
            }
        }
        let parent_path = parentPath(group);
        let parent_group = network.dict[parent_path];

        if(parent_group)
            for(let c of parent_group.connections || [])
            if(parent_path+'.'+c.target == old_name)
            {
                c.target = changeNameInPath(c.target, inspector.item.name);
            }
            network.rebuildDict();
    },

    renameOutput(group, old_name, new_name)
    {
        for(let c of network.dict[group].connections || [])
        {
            if(group+'.'+c.target == old_name)
            {
                c.target = nameInPath(new_name);
            }
        }
        let parent_path = parentPath(group);
        let parent_group = network.dict[parent_path];

        if(parent_group)
            for(let c of parent_group.connections || [])
            if(parent_path+'.'+c.source == old_name)
            {
                c.source = changeNameInPath(c.source, inspector.item.name);
            }
        network.rebuildDict();
        nav.populate();
    },

    debug_json()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><pre>'+JSON.stringify(network.network,null,2)+ '</pre></body></html>');
    },

    debug_dict()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><h1>All components (in network.dict)</h1><pre>');
        w.document.write('<html><body><pre>');
        for(let k of Object.keys(network.dict))
            w.document.write(k+"\n");
        w.document.write('</pre></body></html>');
    }
}

const webui_widgets =
{
    constructors: {},
    add: function(element_name, class_object) {
        customElements.define(element_name, class_object);
        webui_widgets.constructors[element_name] = class_object;
    }
};

let controller = 
{
    run_mode: 'pause',
    commandQueue: [['update', "",{}]],
    tick: 0,
    session_id: 0,
    client_id: Date.now(),
    files: {},
    load_count: 0,
    load_count_timeout: null,
    g_data: null,
    send_stamp: 0,
    tick_duration: 0,
    webui_interval: 0,
    webui_req_int: 100,        
    timeout: 500,            
    reconnect_interval: 1200, 
    reconnect_timer: null,
    request_timer: null,
    open_mode: false,
    data_package: {},   // for debugging

    reconnect()
    {
        controller.get("update", controller.update);
        controller.setSystemInfo(null);
        controller.setWaitingState();
    },

    defer_reconnect()
    {
        clearInterval(controller.reconnect_timer);
        controller.reconnect_timer = null;
    },

    schedule_reconnect()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = null;
        if(controller.reconnect_timer != null)
            return;
        controller.setSystemInfo(null);
        controller.setWaitingState();
        controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
    },

    setWaitingState()
    {
        const stateElement = document.querySelector("#state");
        if(!stateElement)
            return;
        stateElement.innerHTML = 'waiting <span class="status-spinner" aria-hidden="true"></span>';
    },

        saveNetwork()
        {
            const jsonString = JSON.stringify(network.network,null,2);
    
            fetch("save", {
                method: 'PUT',
                headers: {
                    'Content-Type': 'application/json',
                    'Session-Id': controller.session_id,
                    'Client-Id': controller.client_id
                },
                body: jsonString
            })
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok ' + response.statusText);
                }
                return response.json(); // Assuming the server responds with JSON
            })
            .then(data => {
                // SUCCESS
                main.setViewMode(); // Pressing edit mode again will taint the network
                network.tainted = false;    // Only place to prevent saving again
                //alert(data);
                //console.log('Success:', data);
                // Now request network!
            })
            .catch(error => {
                console.error('Error:', error);
                alert("Save failed");
            });
    },
    
    get (url, callback)
    {
        controller.send_stamp = Date.now();
        let last_request = url;
        let xhr = new XMLHttpRequest();
        //console.log("<<< controller.get: \""+url+"\"");
        xhr.open("GET", url, true);
        xhr.setRequestHeader("Session-Id", controller.session_id);
        xhr.setRequestHeader("Client-Id", controller.client_id);
        xhr.onload = function(evt)
        {
              callback(xhr.response, xhr.getResponseHeader("Session-Id"), xhr.getResponseHeader("Package-Type"));
        }
        
        xhr.ontimeout = function() 
        {
            controller.open_mode = false;
            controller.schedule_reconnect();
        }

        xhr.onerror = function()
        {
            controller.open_mode = false;
            controller.schedule_reconnect();
        };

        xhr.responseType = 'json';
        xhr.timeout = 2000;
        try {
            xhr.send();
        }
        catch(error)
        {
            console.log("console.get: "+error);
        }
    },

    init()
    {
        controller.getClasses();
        controller.getClassInfo();
        controller.requestUpdate();
    },
    
    queueCommand(command, path="", dictionary={}) {
        controller.commandQueue.push([command, path, dictionary]);
    },

    clearQueue()
    {
        controller.commandQueue = [];
    },

    new() 
    {
        controller.clearQueue();
        controller.get("new", controller.update);
    },

    openCallback(filename, where)
    {
        controller.get("open?where="+where+"&file="+filename, controller.update);
    },

    open() 
    {

        controller.open_mode = true;
        dialog.showOpenDialog(controller.openCallback, "Select file to open");
    },

    save() 
    {
    if(network.network.filename=="")
        controller.saveas();
    else
        controller.saveNetwork();
    },

    saveas() 
    {
        dialog.showSaveDialog(function(filename) {
            network.network.filename = filename;
            controller.save();
        }, null);
    },

    quit() {
        controller.run_mode = 'quit';
        controller.get("quit", controller.update);
     },

    stop() {
        controller.run_mode = 'stop';
        controller.get("stop", controller.update);
     },
    
    pause() // Save and at set at step 0 if not running
    {
        if(network.tainted) // Should never happen
            return;

        main.setViewMode(); // FIXME: Probably not necessary
        controller.queueCommand('pause');
    },
    
    step()
    {
        if(network.tainted) // Should never happen
            return;
        
        document.querySelector("#state").innerText = "step";
        main.setViewMode(); // FIXME: Probably not necessary
        controller.queueCommand('step');
    },
    
    play()
    {
        if(network.tainted) // Should never happen
            return;
    
        main.setViewMode() // FIXME: Probably not necessary
        controller.queueCommand('play');
    },
    
    realtime()
    {
        if(network.tainted) // Should never happen
            return;
            
        main.setViewMode(); // FIXME: Probably not necessary
        controller.queueCommand('realtime');
    },

    clear_wait()
    {
        controller.load_count = 0;
    },

    wait_for_load(data)
    {
        if(controller.load_count > 0)
            setTimeout(controller.wait_for_load, 1);
        else
        {
            clearTimeout(controller.load_count_timeout);
            controller.updateWidgets(controller.g_data);
        }
    },
    
    updateImages(data)
    {
        controller.load_count = 0;
        controller.g_data = data;

        try
        {
            let w = document.getElementsByClassName('frame')
            for(let i=0; i<w.length; i++)
            {
                const widgetElement = w[i].widget;
                if(widgetElement && widgetElement.loadData)
                    controller.load_count += widgetElement.loadData(data);
            }
     
            controller.load_count_timeout = setTimeout(controller.clear_wait, 200); // give up after 1/5 s and continue
            setTimeout(controller.wait_for_load, 1);
        }
        catch(err)
        {
            console.log("updateImage: exception: "+err);
        }
    },

    setSystemInfo(response)
    {
        try
        {
            if(response == null)
            {
                document.querySelector("#file").innerText = "-";
                document.querySelector("#tick").innerText = "-";
                document.querySelector("#state").innerText = "-";
                document.querySelector("#uptime").innerText = "-";
                document.querySelector("#time").innerText = "-";
                document.querySelector("#ticks_per_s").innerText = "-";
                document.querySelector("#tick_duration").innerText = "-";
                document.querySelector("#actual_duration").innerText =  "-";
                document.querySelector("#lag").innerText = "-";
                document.querySelector("#cpu_cores").innerText = "-";
                document.querySelector("#time_usage").innerText = "-";
                document.querySelector("#usage").innerText = "-";
                document.querySelector("#webui_updates_per_s").innerText = "-";
                document.querySelector("#webui_interval").innerText = "-";
                document.querySelector("#webui_req_int").innerText = "-";
                document.querySelector("#webui_ping").innerText = "-";
                document.querySelector("#webui_lag").innerText = "-";
                let p = document.querySelector("#progress");
                p.style.display = "none";
                return;
            }

            if(response.debug)
                document.querySelector("#debug_row").style.display="table-row";
            else
                document.querySelector("#debug_row").style.display="none";

            document.querySelector("#file").innerText = response.file;
            document.querySelector("#tick").innerText = (Number.isInteger(response.tick) && response.tick >= 0 ?  response.tick : "-");
            document.querySelector("#state").innerHTML = controller.run_mode + (network.tainted ? " <span style='color:red'>&#9679;</span>" : "");

            document.querySelector("#uptime").innerText = secondsToHMS(response.uptime);
            document.querySelector("#time").innerText = secondsToHMS(response.time);
            document.querySelector("#ticks_per_s").innerText = response.ticks_per_s;
            document.querySelector("#tick_duration").innerHTML = formatTime(response.tick_duration);
            document.querySelector("#actual_duration").innerHTML = formatTime(response.actual_duration);
            document.querySelector("#lag").innerHTML = formatTime(response.lag);
            document.querySelector("#cpu_cores").innerText = response.cpu_cores;
            document.querySelector("#time_usage").value = response.time_usage;
            document.querySelector("#usage").value = response.cpu_usage;
            document.querySelector("#webui_updates_per_s").innerText = (1000/controller.webui_interval).toFixed(1) + (response.has_data ? "": " (no data)");
            document.querySelector("#webui_interval").innerText = controller.webui_interval+" ms";
            document.querySelector("#webui_req_int").innerText = controller.webui_req_int+" ms";
            document.querySelector("#webui_ping").innerText = controller.ping+" ms";
            document.querySelector("#webui_lag").innerText = (Date.now()-response.timestamp)+" ms";
            
            let p = document.querySelector("#progress");
            if(response.progress > 0) // FIXME: not working
            {
                p.value = response.progress;
                p.style.display = "table-row";
            }
            else
                p.style.display = "none";

            controller.tick = response.tick;
            controller.run_mode = ['quit', 'stop','pause','play','realtime','restart'][response.state];

            if(network.tainted)
            {
                document.querySelectorAll(".transport_button").forEach((b) => { b.disabled = true; });
            }
            else
            {
                document.querySelectorAll(".transport_button").forEach((b) => { b.disabled = false; });
            }
        }
        catch(err)
        {
            console.log("controller.setSystemInfo: incorrect package received from ikaros.")
        }
    },

    update(response, session_id, package_type)
    {
        if(isEmpty(response))
        {
            console.log("ERROR: empty or malformed response");
            return;
        }


        controller.ping = Date.now() - controller.send_stamp;
        controller.defer_reconnect(); // we are still connected
        if(controller.request_timer == null && !controller.open_mode)
            controller.requestUpdate();

        if(response.log)
        {
            let logElement = log.getMessagesElement();
            if(!logElement)
                return;

            response.log.forEach((element) => 
            {
                let message_class = ["inherit", "quiet","exception","end_of_file","terminate","fatal_error","warning","print","debug","trace"][element[0]];
                let p = document.createElement('p');
                p.className = message_class;
                p.appendChild(document.createTextNode(element[1]));
                if(element[2])
                {
                    let a = document.createElement('a');
                    a.href = '#';
                    a.innerText = " \u00bb " + element[2];
                    a.onclick = function (evt)
                    {
                        evt.preventDefault();
                        selector.selectError(element[2]);
                    };
                    p.appendChild(a);
                }
                logElement.appendChild(p);
                if(element[0]<=6)
                {
                    log.showView(true);
                    logElement.scrollTop = logElement.scrollHeight; // FIXME: Only when needed
                }
            });
        }

        // NETWORK

        if(package_type == "network")
        {
            //console.log("Loading network");
            controller.open_mode = false;
            controller.clearQueue();
            controller.session_id = session_id;
            document.querySelector("header").style.display="block"; // Show page when network is loaded
            document.querySelector("#load").style.display="none";
   
            controller.tick = response.tick;
            network.init(response);
            const shouldAutoArrange = !network.hasAnyComponentPosition();
            nav.populate();
            let top = network.network.name;

            let v = getCookie('selected_background');
            if(v && network.dict[v])
                selector.selectItems([], v);
            else
                selector.selectItems([], top);

            if(shouldAutoArrange)
                main.arrangeComponents();

            if(response.filename=="")
                network.tainted = true;
            else
                network.tainted = main.edit_mode;            
       }

        // NEW SESSION

        else if(controller.session_id != session_id) // new session
        {
            session_id = session_id;
            controller.get("network", controller.update);
            return;
        }

        // DATA

        else if(package_type == "data")
        {
            controller.data_package = response
            controller.setSystemInfo(response);
            controller.tick_duration = response.tick_duration || 0;
            if(response.has_data)
            {

                if(!response.data || Object.keys(response.data).length === 0)
                {
                    // Keep going so response.log can still be processed.
                }
                else
                {
                    controller.updateImages(response.data);
                }
            }
        }
    },

    requestUpdate()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = setTimeout(controller.requestUpdate, controller.webui_req_int); // immediately schedule next

        if(controller.open_mode)
            return;

        controller.webui_interval = Date.now() - controller.last_request_time;
        controller.last_request_time = Date.now();

        let group_path ="";
        let data_string = "";

        // Request new data
        let data_set = new Set();
        const w = document.getElementsByClassName('widget');
        for(let i=0; i<w.length; i++)
            try
            {
                const frame = w[i].closest('.frame');
                const widgetElement = frame ? frame.widget : null;
                if(widgetElement && 'requestData' in widgetElement)
                    widgetElement.requestData(data_set);
            }
            catch(err)
            {
                console.log("requestData failed: "+err);
            }

        group_path = selector.selected_background || "";
        data_string = ""; // should be added to names to support multiple clients
        let sep = "";
        for(s of data_set)
        {
            if(s!="")
            {
                data_string += (sep + s);
                sep = ","
            }
         }

         while(controller.commandQueue.length>0)
        {
            const cmd_dict = controller.commandQueue.shift();
            let s = cmd_dict[0]; // FIXME: check empty string
            const path = cmd_dict[1];
            const dict = cmd_dict[2];
            dict.data = data_string;
            //dict.root = group_path;
            const url_params = toURLParams(dict);

            if(path.length>0 && path[0]=='.') // top path
            {
                s += "/" + path;
            }
            else
            {
                if(group_path != "")
                    s += "/" + group_path;

                if(path != "")
                    s += "." + path;
            }

            if(url_params != "")
                s += "?" + url_params;

            controller.get(s, controller.update);
        }

        controller.queueCommand('update');
    },

    getClasses()
    {
        fetch('/classes', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            network.classes = json.classes.sort();
            if(inspector && typeof inspector.renderLibraryClassList === "function")
                inspector.renderLibraryClassList();
        })
        .catch(function () {
            console.log("Could not get class list from server.");
        })
    },

    getClassInfo()
    {
        fetch('/classinfo', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            network.classinfo = json;
        })
        .catch(function () {
            console.log("Could not get class list from server.");
        })
    },

    getFiles() // not called
    { 
        fetch('/files', {method: 'GET', headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}})
        .then(response => {
            if (!response.ok) {
                throw new Error("HTTP error " + response.status);
            }
            return response.json();
        })
        .then(json => {
            controller.filelist = json;
        })
        .catch(function () {
            console.log("Could not get file list from server.");
        })
    },

    updateWidgets(data)
    {
        // Update the views with data in response
        let w = document.getElementsByClassName('frame')

        for(let i=0; i<w.length; i++)
            try
            {
                const widgetElement = w[i].widget;
                if(!widgetElement)
                    continue;
                widgetElement.receivedData = data;
                widgetElement.update(data); // include data for backward compatibility
            }
            catch(err)
            {
                console.log("updateWidgets failed: "+controller.client_id);
            }
    },

    debug_data()
    {
        let w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><pre>'+JSON.stringify(controller.data_package,null,2)+ '</pre></body></html>');
    },
}

/*
 *
 * Breadcrums scripts
 *
 */

const breadcrumbs = 
{
    init()
    {
        breadcrumbs.breadcrumbs = document.querySelector("#breadcrumbs");
    },

    selectItem(item) // Item must be a group
    {
        const crum = breadcrumbs.breadcrumbs.querySelectorAll('.dynamic');
        crum.forEach(crum => { crum.remove(); });
        let path = "";
        let sep = "";
        let h = "";
        for(g of item.split('.'))
        {
            path += sep+g;
            sep=".";
            let styleStr = "";
            if(path==item)
            {
                styleStr = "style='--breadcrumb-element-color: var(--breadcrumb-active-color); border-radius: 5px;'";
                h += "<div class='dynamic' "+styleStr+" onclick='selector.selectItems([], \""+path+"\")'>"+g+"</div>";
            }
            else
            {
                h += "<div class='bread dynamic' "+styleStr+" onclick='selector.selectItems([], \""+path+"\")'>"+g+"</div>";
            }
        }
        h += "</div>";
        document.querySelector("#nav").insertAdjacentHTML('afterend', h);
    }
}

/*
 *
 * Navigator scripts
 *
 */

const nav =
{
    init() 
    {
        nav.navigator = document.getElementById('navigator');
  
    },
    toggle()
    {
        const s = window.getComputedStyle(nav.navigator , null);
        if (s.display === 'none')
            nav.navigator.style.display = 'block';
        else 
            nav.navigator.style.display = 'none';
    },

    toggleGroup(e)
    {
        if(e.target.classList.contains("group-open"))
            e.target.classList.replace("group-open", "group-closed");
        else if(e.target.classList.contains("group-closed"))
            e.target.classList.replace("group-closed", "group-open");

        e.stopPropagation();
    },

    openGroup(item)
    {
        const g = nav.navigator.querySelector("[data-name='"+item+"']");
        g = g.parentElement;
        while(g)
        {
            //g.setAttribute("class", "group-open");
            g.classList.remove("group-closed");
            g.classList.add("group-open");
            g = g.parentElement;
        }
    },

    selectItem(item)
    {
        nav.traverseAndSelect(nav.navigator, item);
        nav.traverseAndOpen(nav.navigator, item);
    },  

    selectModule(evt)
    {
        // FIXME: Check if ever called
    },
    navClick(e)
    {
        const bg = e.target.parentElement.dataset.name;
        selector.selectItems([], bg);
        e.stopPropagation();
    },

    buildList(group, name) 
    {
        if (isEmpty(group)) return "";
    
        let fullName = name ? `${name}.${group.name}` : group.name;

        if(group.groups.length == 0)
        {
            return `<li data-name='${fullName}' class='group-empty' ><span onclick='return nav.navClick(event)' >${group.name}</span></li>`;
        }
        else
        {
            let s = `<li data-name='${fullName}' class='group-closed' onclick='return nav.toggleGroup(event)'><span onclick='return nav.navClick(event)' >${group.name}</span>`;
            s += `<ul>${group.groups.map(subGroup => nav.buildList(subGroup, fullName)).join('')}</ul>`;
            s +=  "</li>";
            return s;
         }
    },

    traverseAndSelect(element, data_name)
    {
        if(!element) 
            return;
        if(element.dataset.name == data_name)
            element.classList.add("selected");
        else
            element.classList.remove("selected");
        if (element.children)
            Array.from(element.children).forEach((child) => { nav.traverseAndSelect(child, data_name); });
    },

    traverseAndOpen(element, data_name)
    {
        // Open tree to make selection visible
    },

    populate() 
    {
        nav.navigator.innerHTML = "<ul>"+nav.buildList(network.network, "")+"</ul>";
    }
}

const inspector = 
{
    subview: {},
    current_t_body: null,
    component: null,
    library: null,
    system: null,
    selected_library_class: null,
    library_readme_cache: {},
    pending_library_readme_class: null,
    component_width_cookie: 'component_inspector_width',
    library_width_cookie: 'library_inspector_width',
    min_width: 220,
    max_width_margin: 160,
    resize_active: false,
    default_width: 300,
    last_selected_signature: "",

    init()
    {
        inspector.system = document.querySelector('#system_inspector');
        inspector.component = document.querySelector('#component_inspector');
        inspector.library = document.querySelector('#library_inspector');
        inspector.libraryClassList = document.querySelector('#library_class_list');
        inspector.libraryClassDetails = document.querySelector('#library_class_details');

        inspector.subview.nothing  = document.querySelector('#inspector_nothing'); 
        inspector.subview.multiple  = document.querySelector('#inspector_multiple');
        inspector.subview.table  = document.querySelector('#inspector_table');
        inspector.subview.group_background = document.querySelector('#inspector_group_background');
        inspector.subview.group = document.querySelector('#inspector_group');
        inspector.subview.module =  document.querySelector('#inspector_module');
        inspector.subview.widget =  document.querySelector('#inspector_widget');

        inspector.hideSubviews();
        inspector.subview.nothing.style.display='table';
        inspector.updateLibraryButtonState();
        inspector.updateLibraryAddButtonState();
        inspector.renderLibraryClassList();

        inspector.setupResizeForPanel(inspector.component);
        inspector.setupResizeForPanel(inspector.library);
    },

    setupResizeForPanel(panel)
    {
        if(!panel)
            return;

        const computed = window.getComputedStyle(panel);
        const basis = parseInt(computed.flexBasis, 10);
        const width = parseInt(computed.width, 10);
        let defaultWidth = 300;
        if(Number.isFinite(basis) && basis > 0)
            defaultWidth = basis;
        else if(Number.isFinite(width) && width > 0)
            defaultWidth = width;
        panel.dataset.defaultWidth = String(defaultWidth);
        panel.dataset.widthCookie = panel === inspector.library ? inspector.library_width_cookie : inspector.component_width_cookie;

        let resizeHandle = panel.querySelector('.inspector-resize-handle');
        if(!resizeHandle)
        {
            resizeHandle = document.createElement('div');
            resizeHandle.className = 'inspector-resize-handle';
            panel.prepend(resizeHandle);
        }

        inspector.resetPanelWidth(panel);

        resizeHandle.addEventListener('mousedown', inspector.startResize, true);
    },

    resetPanelWidth(panel)
    {
        if(!panel)
            return;
        const cookieName = panel.dataset.widthCookie || inspector.component_width_cookie;
        const savedWidth = parseInt(getCookie(cookieName), 10);
        const maxWidth = Math.max(inspector.min_width, window.innerWidth - inspector.max_width_margin);
        const defaultWidth = parseInt(panel.dataset.defaultWidth, 10);
        const targetWidth = Number.isFinite(savedWidth) && savedWidth >= inspector.min_width
            ? savedWidth
            : (Number.isFinite(defaultWidth) && defaultWidth > 0 ? defaultWidth : 300);
        const width = Math.max(inspector.min_width, Math.min(maxWidth, targetWidth));
        panel.style.flex = `0 0 ${width}px`;
        panel.style.width = `${width}px`;
    },

    startResize(evt)
    {
        const panel = evt.currentTarget ? evt.currentTarget.parentElement : null;
        if(evt.button !== 0 || !panel)
            return;
        evt.preventDefault();
        inspector.resize_active = true;
        inspector.resize_target = panel;
        inspector.resize_start_x = evt.clientX;
        inspector.resize_start_width = panel.getBoundingClientRect().width;
        document.body.classList.add('inspector-resizing');
        document.addEventListener('mousemove', inspector.onResizeDrag, true);
        document.addEventListener('mouseup', inspector.stopResize, true);
    },

    onResizeDrag(evt)
    {
        if(!inspector.resize_active || !inspector.resize_target)
            return;
        const deltaX = inspector.resize_start_x - evt.clientX;
        const maxWidth = Math.max(inspector.min_width, window.innerWidth - inspector.max_width_margin);
        const newWidth = Math.max(inspector.min_width, Math.min(maxWidth, inspector.resize_start_width + deltaX));
        inspector.resize_target.style.flex = `0 0 ${newWidth}px`;
        inspector.resize_target.style.width = `${newWidth}px`;
        evt.preventDefault();
    },

    stopResize()
    {
        if(!inspector.resize_active)
            return;
        inspector.resize_active = false;
        document.removeEventListener('mousemove', inspector.onResizeDrag, true);
        document.removeEventListener('mouseup', inspector.stopResize, true);
        document.body.classList.remove('inspector-resizing');
        if(inspector.resize_target)
        {
            const w = Math.round(inspector.resize_target.getBoundingClientRect().width);
            const cookieName = inspector.resize_target.dataset.widthCookie || inspector.component_width_cookie;
            if(Number.isFinite(w) && w >= inspector.min_width)
                setCookie(cookieName, String(w));
        }
        inspector.resize_target = null;
    },

    toggleSystem()
    {
        if (window.getComputedStyle(inspector.system, null).display === 'none')
        {
            inspector.system.style.display = 'block';
            inspector.component.style.display = 'none';
            if(inspector.library)
                inspector.library.style.display = 'none';
        }
        else
        {
            inspector.system.style.display = 'none';
            inspector.component.style.display = 'none';
            if(inspector.library)
                inspector.library.style.display = 'none';
        }
        inspector.updateLibraryButtonState();
    },

    toggleComponent()
    {
        if (window.getComputedStyle(inspector.component, null).display === 'none')        
            {
            inspector.component.style.display = "block";
            inspector.system.style.display = "none";
            if(inspector.library)
                inspector.library.style.display = "none";
            inspector.resetPanelWidth(inspector.component);
        }
        else
        {
            inspector.system.style.display = "none";
            inspector.component.style.display = "none";
            if(inspector.library)
                inspector.library.style.display = "none";
        }
        inspector.updateLibraryButtonState();
    },

    toggleLibrary()
    {
        if(!inspector.library)
            return;
        if(window.getComputedStyle(inspector.library, null).display === 'none')
        {
            inspector.library.style.display = "flex";
            inspector.system.style.display = "none";
            inspector.component.style.display = "none";
            inspector.resetPanelWidth(inspector.library);
            inspector.renderLibraryClassList();
        }
        else
        {
            inspector.library.style.display = "none";
            inspector.system.style.display = "none";
            inspector.component.style.display = "none";
        }
        inspector.updateLibraryButtonState();
    },

    showComponent()
    {
        inspector.component.style.display = "block";
        inspector.system.style.display = "none";
        if(inspector.library)
            inspector.library.style.display = "none";
        inspector.resetPanelWidth(inspector.component);
        inspector.updateLibraryButtonState();
    },

    showLibrary()
    {
        if(!inspector.library)
            return;
        inspector.library.style.display = "flex";
        inspector.system.style.display = "none";
        inspector.component.style.display = "none";
        inspector.resetPanelWidth(inspector.library);
        inspector.renderLibraryClassList();
        inspector.updateLibraryButtonState();
        inspector.updateLibraryAddButtonState();
    },

    updateLibraryButtonState()
    {
        const button = document.getElementById("library_inspector_button");
        if(!button || !inspector.library)
            return;
        const open = window.getComputedStyle(inspector.library, null).display !== 'none';
        button.setAttribute("aria-pressed", open ? "true" : "false");
    },

    updateLibraryAddButtonState()
    {
        const button = document.getElementById("library_add_button");
        if(!button)
            return;
        const canAdd = !!(main && main.edit_mode && inspector.selected_library_class);
        button.disabled = !canAdd;
    },

    renderLibraryClassList()
    {
        if(!inspector.libraryClassList)
            return;

        inspector.libraryClassList.innerHTML = "";

        const classes = Array.isArray(network.classes) ? network.classes : [];
        if(classes.length === 0)
        {
            const emptyState = document.createElement("div");
            emptyState.className = "library-inspector-placeholder";
            emptyState.textContent = "No module classes available yet.";
            inspector.libraryClassList.appendChild(emptyState);
            if(inspector.libraryClassDetails && !inspector.selected_library_class)
                inspector.libraryClassDetails.textContent = "Select a module class to view details here later.";
            return;
        }

        if(!inspector.selected_library_class || !classes.includes(inspector.selected_library_class))
            inspector.selected_library_class = classes[0];
        inspector.updateLibraryAddButtonState();

        for(const className of classes)
        {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "library-class-item";
            if(className === inspector.selected_library_class)
                button.classList.add("is-selected");
            button.setAttribute("role", "option");
            button.setAttribute("aria-selected", className === inspector.selected_library_class ? "true" : "false");
            button.textContent = className;
            button.addEventListener("click", function()
            {
                inspector.selectLibraryClass(className);
            });
            inspector.libraryClassList.appendChild(button);
        }

        inspector.updateLibraryClassDetails();
    },

    selectLibraryClass(className)
    {
        inspector.selected_library_class = className;
        inspector.updateLibraryAddButtonState();
        inspector.renderLibraryClassList();
    },

    addSelectedLibraryClass()
    {
        if(!main.edit_mode || !inspector.selected_library_class)
            return;
        const fullName = main.newModule(inspector.selected_library_class);
        if(fullName)
            selector.selectItems([fullName], selector.selected_background, false, false, true);
    },

    escapeHtml(text)
    {
        return String(text)
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    },

    renderInlineMarkdown(text)
    {
        let html = inspector.escapeHtml(text);
        html = html.replace(/&lt;br\s*\/?&gt;/gi, "<br>");
        html = html.replace(/`([^`]+)`/g, "<code>$1</code>");
        html = html.replace(/\[([^\]]+)\]\(([^)\s]+)\)/g, '<a href="$2" target="_blank" rel="noopener noreferrer">$1</a>');
        html = html.replace(/\*\*([^*]+)\*\*/g, "<strong>$1</strong>");
        html = html.replace(/\*([^*]+)\*/g, "<em>$1</em>");
        return html;
    },

    renderMarkdown(text)
    {
        const lines = String(text || "").replace(/\r\n/g, "\n").split("\n");
        const blocks = [];
        let paragraph = [];
        let listItems = [];
        let codeLines = [];
        let inCodeBlock = false;
        let tableRows = [];

        const isTableLine = function(line)
        {
            return /\|/.test(line);
        };

        const isTableDivider = function(line)
        {
            const normalized = line.trim().replace(/^\||\|$/g, "").trim();
            if(normalized === "")
                return false;
            return normalized.split("|").every((cell) => /^:?-{3,}:?$/.test(cell.trim()));
        };

        const splitTableRow = function(line)
        {
            return line.trim().replace(/^\||\|$/g, "").split("|").map((cell) => cell.trim());
        };

        const flushParagraph = function()
        {
            if(paragraph.length === 0)
                return;
            blocks.push(`<p>${inspector.renderInlineMarkdown(paragraph.join(" "))}</p>`);
            paragraph = [];
        };

        const flushList = function()
        {
            if(listItems.length === 0)
                return;
            blocks.push(`<ul>${listItems.map((item) => `<li>${inspector.renderInlineMarkdown(item)}</li>`).join("")}</ul>`);
            listItems = [];
        };

        const flushTable = function()
        {
            if(tableRows.length < 2)
            {
                if(tableRows.length === 1)
                    paragraph.push(tableRows[0].join(" | "));
                tableRows = [];
                return;
            }

            const header = tableRows[0];
            const bodyRows = tableRows.slice(2);
            const thead = `<thead><tr>${header.map((cell) => `<th>${inspector.renderInlineMarkdown(cell)}</th>`).join("")}</tr></thead>`;
            const tbody = bodyRows.length > 0
                ? `<tbody>${bodyRows.map((row) => `<tr>${row.map((cell) => `<td>${inspector.renderInlineMarkdown(cell)}</td>`).join("")}</tr>`).join("")}</tbody>`
                : "";
            blocks.push(`<table>${thead}${tbody}</table>`);
            tableRows = [];
        };

        const flushCodeBlock = function()
        {
            if(codeLines.length === 0)
                return;
            blocks.push(`<pre><code>${inspector.escapeHtml(codeLines.join("\n"))}</code></pre>`);
            codeLines = [];
        };

        for(const line of lines)
        {
            if(line.trim().startsWith("```"))
            {
                flushParagraph();
                flushList();
                flushTable();
                if(inCodeBlock)
                {
                    flushCodeBlock();
                    inCodeBlock = false;
                }
                else
                {
                    inCodeBlock = true;
                }
                continue;
            }

            if(inCodeBlock)
            {
                codeLines.push(line);
                continue;
            }

            const trimmed = line.trim();
            if(trimmed === "")
            {
                flushParagraph();
                flushList();
                flushTable();
                continue;
            }

            if(isTableLine(line))
            {
                const row = splitTableRow(line);
                if(tableRows.length === 0)
                {
                    flushParagraph();
                    flushList();
                    tableRows.push(row);
                    continue;
                }
                if(tableRows.length === 1 && isTableDivider(line))
                {
                    tableRows.push(row);
                    continue;
                }
                if(tableRows.length >= 2)
                {
                    tableRows.push(row);
                    continue;
                }
            }
            else
            {
                flushTable();
            }

            const heading = trimmed.match(/^(#{1,6})\s+(.*)$/);
            if(heading)
            {
                flushParagraph();
                flushList();
                flushTable();
                const level = Math.min(6, heading[1].length);
                blocks.push(`<h${level}>${inspector.renderInlineMarkdown(heading[2])}</h${level}>`);
                continue;
            }

            const listItem = trimmed.match(/^[-*]\s+(.*)$/);
            if(listItem)
            {
                flushParagraph();
                listItems.push(listItem[1]);
                continue;
            }

            flushList();
            paragraph.push(trimmed);
        }

        flushParagraph();
        flushList();
        flushTable();
        flushCodeBlock();

        if(blocks.length === 0)
            return "<p></p>";
        return blocks.join("");
    },

    updateLibraryClassDetails()
    {
        if(!inspector.libraryClassDetails)
            return;
        if(!inspector.selected_library_class)
        {
            inspector.libraryClassDetails.innerHTML = "<p>Select a module class to view details here later.</p>";
            return;
        }
        const className = inspector.selected_library_class;
        if(Object.prototype.hasOwnProperty.call(inspector.library_readme_cache, className))
        {
            inspector.libraryClassDetails.innerHTML = inspector.renderMarkdown(inspector.library_readme_cache[className]);
            return;
        }

        inspector.pending_library_readme_class = className;
        inspector.libraryClassDetails.innerHTML = `<p>Loading ReadMe.md for ${inspector.escapeHtml(className)}...</p>`;
        fetch(`/classreadme?class=${encodeURIComponent(className)}`, {
            method: "GET",
            headers: {"Session-Id": controller.session_id, "Client-Id": controller.client_id}
        })
        .then(response => {
            if(!response.ok)
                throw new Error("HTTP error " + response.status);
            return response.text();
        })
        .then(text => {
            inspector.library_readme_cache[className] = text;
            if(inspector.pending_library_readme_class === className && inspector.selected_library_class === className)
                inspector.libraryClassDetails.innerHTML = inspector.renderMarkdown(text);
        })
        .catch(() => {
            const fallback = `Could not load ReadMe.md for ${className}.`;
            inspector.library_readme_cache[className] = fallback;
            if(inspector.pending_library_readme_class === className && inspector.selected_library_class === className)
                inspector.libraryClassDetails.innerHTML = `<p>${inspector.escapeHtml(fallback)}</p>`;
        });
    },

    hideSubviews()
    {
        Object.entries(inspector.subview).forEach(([key, value]) => { value.style.display = 'none'; });
    },

    setTable(table)
    {
        current_t_body = table.tBodies[0];
        current_t_body.innerHTML = "";
    },

    addTableRow(label, content="")
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        cell1.innerText = label;
        cell2.innerHTML = content;
        return cell2.firstElementChild;
    },

    focusEditableField(label)
    {
        if(!inspector.subview.table)
            return false;
        const rows = inspector.subview.table.querySelectorAll("tbody tr");
        for(const row of rows)
        {
            const cells = row.cells || [];
            if(cells.length < 2)
                continue;
            if((cells[0].innerText || "").trim() !== label)
                continue;
            const target = cells[1];
            if(!target || target.contentEditable !== "true")
                continue;
            target.focus();
            return true;
        }
        return false;
    },

    addHeader(header)
    {
        current_t_body.innerHTML += `<tr><td colspan='2' class='header'>${header}</td></tr>`;
    },

    addAttributeValue(attribute, value) // SET ALSO VARIABLE AND LINK GROUP for EDITABLE
    {
        this.addTableRow(attribute,value);
    },

    addMenu(name, value, opts, attribute) // SET ALSO VARIABLE AND LINK GROUP for EDITABLE
    {
        let s = '<select name="'+name+'" oninput="this">';
        for(let j in opts)
        {
            const val= opts[j];
            if(opts[j] == value)
                s += '<option value="'+val+'" selected >'+opts[j]+'</option>';
            else
                s += '<option value="'+val+'">'+opts[j]+'</option>';
        }
        s += '</select>';

        return inspector.addTableRow(name, s);
    },

    createTemplate(component)
    {
        let t = []; // {'name':'name','control':'textedit', 'type':'source'}] // FIXME: should be "identifier"
        for(let [a,v] of Object.entries(component))
        {
            if(!["name","class"].includes(a) && a[0]!='_')
                t.push({'name':a,'control':'textedit', 'type':'source'});
        }
        return t;
    },


    checkValueForType(value, type)
    {
        return true; // Accept everything for now

        if(type=="string")
            return true;

        if(value[0]=='@') // Do not check indirection
            return true;

        return false;
    },

    parseValueForType(value, type, fallbackValue)
    {
        if(type == "int")
        {
            const parsed = parseInt(value);
            return Number.isFinite(parsed) ? parsed : fallbackValue;
        }
        if(type == "float" || type == "number")
        {
            const parsed = parseFloat(value);
            return Number.isFinite(parsed) ? parsed : fallbackValue;
        }
        return value;
    },

    /* [{type, name, control}] */

    acreateHeaderRow(item, template)
    {
        const row = current_t_body.insertRow(-1);
        const cell1 = row.insertCell(0);
        //const cell2 = row.insertCell(1);
        cell1.innerText = template.name;
        //cell2.setAttribute('class', p.type);
    },



    createTextEditRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);
        const hasDefaultPlaceholder = !!(p && p.default !== undefined && p.default !== null && p.default !== "");
        const updateDefaultPlaceholderState = (targetCell, isEmpty) =>
        {
            if(!hasDefaultPlaceholder)
            {
                targetCell.removeAttribute("data-placeholder");
                targetCell.classList.remove("inspector-default-placeholder");
                return;
            }
            targetCell.setAttribute("data-placeholder", String(p.default));
            if(isEmpty)
                targetCell.classList.add("inspector-default-placeholder");
            else
                targetCell.classList.remove("inspector-default-placeholder");
        };

        cell1.innerText = p.name;
        const hasValue = !(value === undefined || value === null || value === "");
        cell2.textContent = hasValue ? String(value) : "";
        cell2.setAttribute('class', p.type + ' textedit');
        updateDefaultPlaceholderState(cell2, !hasValue);
        cell2.addEventListener("paste", function(e) 
        {
            e.preventDefault();
            const text = e.clipboardData.getData("text/plain");
            document.execCommand("insertHTML", false, text); // FIXME: uses deprecated functions
        });

        cell2.contentEditable = true;
        const commitOnInput = (p.name !== "name");
        const commitTextEditValue = function(evt)
        {
            let newValue = evt.target.innerText.replace(String.fromCharCode(10), "").replace(String.fromCharCode(13), "");
            newValue = newValue.replace(/\u200B/g, "");
            if(!inspector.checkValueForType(newValue, p.type))
                return;
            item[p.name] = inspector.parseValueForType(newValue, p.type, item[p.name]);
            if(inspector.notify && inspector.notify.parameters)
                inspector.notify.parameters[p.name] = item[p.name];
            if(inspector.notify)
                inspector.notify.parameterChangeNotification(p);
        };
        cell2.addEventListener("keypress", function(evt) 
        {
            if(evt.keyCode == 13)
            {
                evt.target.blur();
                evt.preventDefault();
                return;
            }
        });

        cell2.addEventListener("input", function(evt)
        {
            updateDefaultPlaceholderState(evt.target, evt.target.innerText.trim() === "");
            if(commitOnInput)
                commitTextEditValue(evt);
        });

        cell2.addEventListener("blur", function(evt) 
        {
            commitTextEditValue(evt);
            updateDefaultPlaceholderState(evt.target, evt.target.innerText.trim() === "");
        });
    },


    createMenuRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;

        let opts = p.options.split(',').map(o=>o.trim());
                        
        let s = '<select name="'+p.name+'">';
        for(let j in opts)
        {
            let optionValue = p.type == 'int' ? j : opts[j];
            let optionLabel = opts[j];
            let optionStyle = "";
            if(p.option_labels && Object.prototype.hasOwnProperty.call(p.option_labels, optionValue))
                optionLabel = p.option_labels[optionValue];
            if(p.option_styles && Object.prototype.hasOwnProperty.call(p.option_styles, optionValue))
                optionStyle = ` style="${p.option_styles[optionValue]}"`;
            if(opts[j] == item[p.name])
                s += '<option value="'+optionValue+'" selected'+optionStyle+'>'+optionLabel+'</option>';
            else
                s += '<option value="'+optionValue+'"'+optionStyle+'>'+optionLabel+'</option>';
        }
        s += '</select>';
        cell2.innerHTML= s;
        cell2.addEventListener("input", function(evt) { 
                item[p.name] = evt.target.value.trim();
                if(inspector.notify && inspector.notify.parameters)
                    inspector.notify.parameters[p.name] = item[p.name];
                if(inspector.notify)
                inspector.notify.parameterChangeNotification(p);
            });
    },

    

    createCheckBoxRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;
        if(value)
            cell2.innerHTML= '<input type="checkbox" checked />';
        else
            cell2.innerHTML= '<input type="checkbox" />';
        const commitCheckBoxValue = function(evt)
        {
            item[p.name] = evt.target.checked;
            if(inspector.notify && inspector.notify.parameters)
                inspector.notify.parameters[p.name] = item[p.name];
            if(inspector.notify)
                inspector.notify.parameterChangeNotification(p);
        };
        cell2.addEventListener("input", commitCheckBoxValue);
        cell2.addEventListener("change", commitCheckBoxValue);

    },

    

    createSliderRow(item, p)
    {
        this.createTextEditRow(item, p); // FIXME: TEMPORARY
    },


    addDataRows(item, template, notify=null)
    {
        try{
            inspector.item = item;
            inspector.template = template;
            inspector.notify = notify;      // object that will be notified on change
    
            for(let p of inspector.template)
            {
                // Set standard controls
                if(p.control == undefined)
                {
                    if(p.type=="bool")
                        p.control = "checkbox";
                    else if(p.options)
                        p.control = "menu";
                    else
                        p.control = "textedit";
                }
    
                switch(p.control)
                {
                    case 'header':
                        this.acreateHeaderRow(item, p); break;
                    case 'textedit':
                        this.createTextEditRow(item, p); break;
                    case 'menu':
                        this.createMenuRow(item, p); break;
                    case 'checkbox':
                        this.createCheckBoxRow(item, p); break;
                    case 'slider':
                        this.createSliderRow(item, p); break;
                }
    
    
                /*
                // Add row
                {
                    const row = current_t_body.insertRow(-1);
                    const value = item[p.name];
                    const cell1 = row.insertCell(0);
                    const cell2 = row.insertCell(1);
                    cell1.innerText = p.name;
                    cell2.innerHTML = value != undefined ? value : "";
                    cell2.setAttribute('class', p.type);
                    cell2.addEventListener("paste", function(e) 
                    {
                        e.preventDefault();
                        const text = e.clipboardData.getData("text/plain");
                        document.execCommand("insertHTML", false, text); // FIXME: uses deprecated functions
                    });
    
                    switch(p.control)
                    {
                        case 'header':
                            cell1.setAttribute("colspan", 2);
                            cell1.setAttribute("class", "header");
                            row.deleteCell(1);
                            break;
    
                        case 'textedit':
                            cell2.contentEditable = true;
                            cell2.className += ' textedit';
                            cell2.addEventListener("keypress", function(evt) {
                                if(evt.keyCode == 13)
                                {
                                    evt.target.blur();
                                    evt.preventDefault();
                                    if(inspector.notify)
                                        inspector.notify.parameterChangeNotification(p);
                                    return;
                                }
    
                            });
                            cell2.addEventListener("blur", function(evt) {
                                if(p.type == 'int')
                                item[p.name] = parseInt(evt.target.innerText);
                                else if(p.type == 'float')
                                item[p.name] = parseFloat(evt.target.innerText);
                                else
                                {
                                    item[p.name] = evt.target.innerText.replace(String.fromCharCode(10), "").replace(String.fromCharCode(13), "");
                                }
                                if(inspector.notify)
                                inspector.notify.parameterChangeNotification(p);
                            });
                            break;
    
                        case 'slider':
                            if(p.type == 'int' || p.type == 'float')
                            {
                                cell2.innerHTML= '<div>'+value+'</div><input type="range" value="'+value+'" min="'+p.min+'" max="'+p.max+'" step="'+(p.type == 'int' ?  1: 0.01)+'"/>';
                                cell2.addEventListener("input", function(evt) {
                                    evt.target.parentElement.querySelector('div').innerText = evt.target.value;
                                    item[p.name] = evt.target.value;
                                    if(inspector.notify) inspector.notify.parameterChangeNotification(p);
                                });
                            }
                            break;
                        
                        case 'menu':
                            let opts = p.values.split(',').map(o=>o.trim());
                            
                            let s = '<select name="'+p.name+'">';
                            for(let j in opts)
                            {
                                let value = p.type == 'int' ? j : opts[j];
                                if(opts[j] == item[p.name])
                                    s += '<option value="'+value+'" selected >'+opts[j]+'</option>';
                                else
                                    s += '<option value="'+value+'">'+opts[j]+'</option>';
                            }
                            s += '</select>';
                            cell2.innerHTML= s;
                            cell2.addEventListener("input", function(evt) { 
                                    component[p.name] = evt.target.value.trim();
                                    if(inspector.notify)
                                    inspector.notify.parameterChangeNotification(p);
                                });
                            break;
                        
                        case 'checkbox':
                            if(p.type == 'bool')
                            {
                                if(value)
                                    cell2.innerHTML= '<input type="checkbox" checked />';
                                else
                                    cell2.innerHTML= '<input type="checkbox" />';
                                cell2.addEventListener("change", function(evt) { item[p.name] = evt.target.checked; if(notify) notify.parameterChangeNotification(p);});
                            }
                            break;
                        
                        case 'number':
                            if(p.type == 'int')
                            {
                                cell2.innerHTML= '<input type="number" value="'+value+'" min="'+p.min+'" max="'+p.max+'"/>';
                                cell2.addEventListener("input", function(evt) { 
                                    item[p.name] = evt.target.value; if(inspector.notify) inspector.notify.parameterChangeNotification(p);});
                            }
                            break;
    
                        default:
                            break;
                    }
                }  */
            }
        }
        catch(err)
        {
            console.log("addDataRows: "+err);
        }

    
           
    },


/*
    addDataRows_OLD(item, template, notify=null)
    {
        inspector.item = item;
        inspector.template = template;
        inspector.notify = notify;      // object that will be notified on change

        for(let p of inspector.template)
        {
            // Set standard controls

            if(p.control == undefined)
            {
                p.control = "textedit";

            }
        
            // Add row
            {
                const row = current_t_body.insertRow(-1);
                const value = item[p.name];
                const cell1 = row.insertCell(0);
                const cell2 = row.insertCell(1);
                cell1.innerText = p.name;
                cell2.innerHTML = value != undefined ? value : "";
                cell2.setAttribute('class', p.type);
                cell2.addEventListener("paste", function(e) 
                {
                    e.preventDefault();
                    const text = e.clipboardData.getData("text/plain");
                    document.execCommand("insertHTML", false, text); // FIXME: uses deprecated functions
                });

                switch(p.control)
                {
                    case 'header':
                        cell1.setAttribute("colspan", 2);
                        cell1.setAttribute("class", "header");
                        row.deleteCell(1);
                        break;

                    case 'textedit':
                        cell2.contentEditable = true;
                        cell2.className += ' textedit';
                        cell2.addEventListener("keypress", function(evt) {
                            if(evt.keyCode == 13)
                            {
                                evt.target.blur();
                                evt.preventDefault();
                                if(inspector.notify)
                                    inspector.notify.parameterChangeNotification(p);
                                return;
                            }
                            /* Allow all types of characters to allow indirection
                            if(p.type == 'int' && "-0123456789".indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(p.type == 'float' && "-0123456789.".indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(p.type == 'delay' && "0123456789:".indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(p.type == 'source' && "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ+-_.0123456789*".indexOf(evt.key) == -1)
                                evt.preventDefault();
                            

                            if((",:@+*-/,;"+identifier).indexOf(evt.key) == -1)
                                evt.preventDefault();
                
                            else if(p.type == 'source' && ("@"+identifier).indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(p.type == 'delay' && (",:@+*-/"+identifier).indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(p.type == 'matrix' && ("@,;"+identifier).indexOf(evt.key) == -1)
                                evt.preventDefault();
                            else if(("@+*-/"+identifier).indexOf(evt.key) == -1)
                                evt.preventDefault();
                

                        });
                        cell2.addEventListener("blur", function(evt) {
                            if(p.type == 'int')
                            item[p.name] = parseInt(evt.target.innerText);
                            else if(p.type == 'float')
                            item[p.name] = parseFloat(evt.target.innerText);
                            else
                            {
                                item[p.name] = evt.target.innerText.replace(String.fromCharCode(10), "").replace(String.fromCharCode(13), "");
                            }
                            if(inspector.notify)
                            inspector.notify.parameterChangeNotification(p);
                        });
                        break;

                    case 'slider':
                        if(p.type == 'int' || p.type == 'float')
                        {
                            cell2.innerHTML= '<div>'+value+'</div><input type="range" value="'+value+'" min="'+p.min+'" max="'+p.max+'" step="'+(p.type == 'int' ?  1: 0.01)+'"/>';
                            cell2.addEventListener("input", function(evt) {
                                evt.target.parentElement.querySelector('div').innerText = evt.target.value;
                                item[p.name] = evt.target.value;
                                if(inspector.notify) inspector.notify.parameterChangeNotification(p);
                            });
                        }
                        break;
                    
                    case 'menu':
                        let opts = p.values.split(',').map(o=>o.trim());
                        
                        let s = '<select name="'+p.name+'">';
                        for(let j in opts)
                        {
                            let value = p.type == 'int' ? j : opts[j];
                            if(opts[j] == item[p.name])
                                s += '<option value="'+value+'" selected >'+opts[j]+'</option>';
                            else
                                s += '<option value="'+value+'">'+opts[j]+'</option>';
                        }
                        s += '</select>';
                        cell2.innerHTML= s;
                        cell2.addEventListener("input", function(evt) { 
                                component[p.name] = evt.target.value.trim();
                                if(inspector.notify)
                                inspector.notify.parameterChangeNotification(p);
                            });
                        break;
                    
                    case 'checkbox':
                        if(p.type == 'bool')
                        {
                            if(value)
                                cell2.innerHTML= '<input type="checkbox" checked />';
                            else
                                cell2.innerHTML= '<input type="checkbox" />';
                            cell2.addEventListener("change", function(evt) { item[p.name] = evt.target.checked; if(notify) notify.parameterChangeNotification(p);});
                        }
                        break;
                    
                    case 'number':
                        if(p.type == 'int')
                        {
                            cell2.innerHTML= '<input type="number" value="'+value+'" min="'+p.min+'" max="'+p.max+'"/>';
                            cell2.addEventListener("input", function(evt) { 
                                item[p.name] = evt.target.value; if(inspector.notify) inspector.notify.parameterChangeNotification(p);});
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    },
*/

    checkRangeSyntax(s) // FIXME: Should check full syntax
    {
        if(!s)
            return "";
        if(s=="")
            return "";
        if(s[0]!="[")
        {
            alert("Range must start with [");
            return "";
        }
        if(s[s.length-1]!="]")
        {
            alert("Range must end with ]");
            return "";
        }
        return s;
    },

    parameterChangeNotification(p)
    {
        if(
            p &&
            p.name == "color" &&
            selector.selected_foreground &&
            selector.selected_foreground.length == 1 &&
            ["module", "group", "input", "output"].includes(inspector.item?._tag)
        )
        {
            const selectedId = selector.selected_foreground[0];
            const selectedElement = document.getElementById(selectedId);
            main.applyComponentColorToElement(selectedElement, inspector.item);
            network.tainted = true;
        }

        const isTopGroupBackground = (
            inspector.item &&
            inspector.item._tag == "group" &&
            selector.selected_foreground.length == 0 &&
            selector.selected_background == network.network.name
        );

        if(isTopGroupBackground && p && p.name == "tick_duration")
        {
            const parsed = parseFloat(inspector.item.tick_duration);
            const tickDuration = Number.isFinite(parsed) ? parsed : 0;
            inspector.item.tick_duration = tickDuration;
            controller.tick_duration = tickDuration;
            return;
        }

        if(
            inspector.item &&
            inspector.item._tag == "group" &&
            selector.selected_foreground.length == 0 &&
            p &&
            p.name == "auto_routing"
        )
        {
            main.updateAutoRoutingButtonState();
            main.addConnections();
        }

        if(inspector.item._tag == "connection")
        {
            this.item.source = getStringUpToBracket(this.item.source)+this.checkRangeSyntax(this.item.source_range);
            this.item.target = getStringUpToBracket(this.item.target)+this.checkRangeSyntax(this.item.target_range);
            selector.selectConnection(selector.selected_connection);
        }
        else if(inspector.item._tag == "group" && selector.selected_foreground.length == 0) // background is selected
            {
                let old_name = selector.selected_background;
                let new_name = changeNameInPath(selector.selected_background, inspector.item.name);
                if(new_name != old_name)
                {
                    network.dict[new_name] = inspector.item;
                    delete network.dict[old_name];
                    selector.selectItems([], new_name);
                }
        }
        else if (inspector.item._tag == "input")
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            network.renameInput(group, old_name, new_name);
            selector.selectItems([new_name], null);
         }
        else if (inspector.item._tag == "output")
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            network.renameOutput(group, old_name, new_name);
            selector.selectItems([new_name], null);
         }
        else // foreground group selected
        {
            let old_name = selector.selected_foreground[0];
            let new_name = selector.selected_background+'.'+inspector.item.name;
            let group = selector.selected_background;

            if(new_name != old_name)
            {
                network.renameGroupOrModule(group, old_name, new_name);

                // network.dict[new_name] = inspector.item;
                // delete network.dict[old_name];

                selector.selectItems([new_name], null);
            }
        }
        network.rebuildDict();
        nav.populate();
    },

    showGroupBackground(bg)
    {
        inspector.hideSubviews();
        let item = network.dict[bg];
        const colorOptions = "red,orange,yellow,green,blue,purple,pink,white,black";
        const colorOptionLabels = {
            red: "&#x1F7E5; red",
            orange: "&#x1F7E7; orange",
            yellow: "&#x1F7E8; yellow",
            green: "&#x1F7E9; green",
            blue: "&#x1F7E6; blue",
            purple: "&#x1F7EA; purple",
            pink: "&#x25A3; pink",
            white: "&#x2B1C; white",
            black: "&#x2B1B; black"
        };
        const isTopGroup = (bg == network.network.name);
        const rowTemplate = [];
        const readOnlyRows = [];
        const countRowKeys = ["groups", "modules", "connections", "widgets", "inputs", "outputs"];
        const countRows = [];
        const allAttributes = Object.keys(item || {});
        const hiddenTopGroupAttributes = new Set(["filename", "webui_port", "info", "stop"]);
        const priorityRowTemplates = new Map();
        for(const key of allAttributes)
        {
            if(key == "_tag" || key.startsWith("_"))
                continue;
            if(key == "parameters")
                continue;
            if(isTopGroup && hiddenTopGroupAttributes.has(key))
                continue;
            if(key == "name" || key == "tick_duration")
                continue;
            const value = item[key];
            if(countRowKeys.includes(key))
            {
                countRows.push({ key, value: (item[key] || []).length });
                continue;
            }
            if(Array.isArray(value))
                continue;
            if(key == "color")
            {
                priorityRowTemplates.set(key, {'name': key, 'control':'menu', 'type':'source', 'options': colorOptions, 'option_labels': colorOptionLabels});
                continue;
            }
            if(key == "auto_routing")
            {
                priorityRowTemplates.set(key, {'name': key, 'control':'checkbox', 'type':'bool'});
                continue;
            }
            if(typeof value == "number")
                rowTemplate.push({'name': key, 'control':'textedit', 'type': Number.isInteger(value) ? 'int' : 'float'});
            else if(typeof value == "boolean")
                rowTemplate.push({'name': key, 'control':'checkbox', 'type':'bool'});
            else if(typeof value == "string" || value === undefined || value === null)
                rowTemplate.push({'name': key, 'control':'textedit', 'type':'source'});
            else
                readOnlyRows.push({ key, value });
        }
        const orderedPriorityRows = [];
        if(priorityRowTemplates.has("color"))
            orderedPriorityRows.push(priorityRowTemplates.get("color"));
        if(priorityRowTemplates.has("auto_routing"))
            orderedPriorityRows.push(priorityRowTemplates.get("auto_routing"));
        rowTemplate.unshift(...orderedPriorityRows);
        for(const key of countRowKeys)
            if(!countRows.some((r) => r.key == key))
                countRows.push({ key, value: (item[key] || []).length });

        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';

        inspector.addHeader(isTopGroup ? "Top Group" : "Group");
        if(main.edit_mode)
        {
            const template = [{'name':'name', 'control':'textedit', 'type':'source'}];
            if(isTopGroup)
            {
                if(item.tick_duration === undefined)
                    item.tick_duration = controller.tick_duration || 0;
                template.push({'name':'tick_duration', 'control':'textedit', 'type':'float'});
            }
            inspector.addDataRows(item, template, inspector);
            if(rowTemplate.length > 0)
            {
                const firstCustomRowIndex = current_t_body.rows.length;
                inspector.addDataRows(item, rowTemplate, inspector);
                for(let i = 0; i < rowTemplate.length; i++)
                {
                    const attribute = rowTemplate[i];
                    const row = current_t_body.rows[firstCustomRowIndex + i];
                    if(!row || row.cells.length < 2)
                        continue;
                    if(attribute.name == "auto_routing" || attribute.name == "color")
                        continue;
                    const valueCell = row.cells[1];
                    valueCell.classList.add("inspector-removable-value-cell");
                    const removeButton = document.createElement("button");
                    removeButton.type = "button";
                    removeButton.className = "inspector-minus-button";
                    removeButton.setAttribute("aria-label", "Remove attribute");
                    removeButton.setAttribute("contenteditable", "false");
                    removeButton.innerHTML = `
                        <svg viewBox="0 0 24 24" width="16" height="16" aria-hidden="true" focusable="false">
                            <path d="M18 6L17.1991 18.0129C17.129 19.065 17.0939 19.5911 16.8667 19.99C16.6666 20.3412 16.3648 20.6235 16.0011 20.7998C15.588 21 15.0607 21 14.0062 21H9.99377C8.93927 21 8.41202 21 7.99889 20.7998C7.63517 20.6235 7.33339 20.3412 7.13332 19.99C6.90607 19.5911 6.871 19.065 6.80086 18.0129L6 6M4 6H20M16 6L15.7294 5.18807C15.4671 4.40125 15.3359 4.00784 15.0927 3.71698C14.8779 3.46013 14.6021 3.26132 14.2905 3.13878C13.9376 3 13.523 3 12.6936 3H11.3064C10.477 3 10.0624 3 9.70951 3.13878C9.39792 3.26132 9.12208 3.46013 8.90729 3.71698C8.66405 4.00784 8.53292 4.40125 8.27064 5.18807L8 6M14 10V17M10 10V17" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"></path>
                        </svg>
                    `;
                    removeButton.addEventListener("mousedown", function(evt)
                    {
                        evt.preventDefault();
                    });
                    removeButton.addEventListener("click", function(evt)
                    {
                        evt.preventDefault();
                        evt.stopPropagation();
                        delete item[attribute.name];
                        row.remove();
                    });
                    valueCell.appendChild(removeButton);
                    if(valueCell.childNodes.length == 1)
                        valueCell.insertBefore(document.createTextNode("\u200B"), removeButton);
                }
            }
        }
        else
        {
            inspector.addAttributeValue("name", item.name);
            if(isTopGroup)
                inspector.addAttributeValue("tick_duration", item.tick_duration !== undefined ? item.tick_duration : controller.tick_duration);
            for(const p of rowTemplate)
                inspector.addAttributeValue(p.name, item[p.name]);
        }
        for(const p of readOnlyRows)
            inspector.addAttributeValue(p.key, p.value);

        if(main.edit_mode)
        {
            const actionRow = current_t_body.insertRow(-1);
            const actionCellLeft = actionRow.insertCell(0);
            const actionCellRight = actionRow.insertCell(1);
            actionCellLeft.innerHTML = "";
            actionCellRight.style.textAlign = "right";
            actionCellRight.innerHTML = '<button type="button" class="inspector-plus-button" aria-label="Add">+</button>';

            const plusButton = actionCellRight.querySelector(".inspector-plus-button");
            if(plusButton)
            {
                const openNewAttributeRow = function()
                {
                    const row = document.createElement("tr");
                    const labelCell = document.createElement("td");
                    const valueCell = document.createElement("td");
                    labelCell.className = "textedit inspector-attr-cell inspector-attr-cell-label";
                    valueCell.className = "textedit inspector-attr-cell";
                    labelCell.contentEditable = true;
                    valueCell.contentEditable = true;
                    labelCell.setAttribute("data-placeholder", "label");
                    valueCell.setAttribute("data-placeholder", "value");
                    row.appendChild(labelCell);
                    row.appendChild(valueCell);
                    current_t_body.insertBefore(row, actionRow);

                    const commitAttribute = function()
                    {
                        const attrName = labelCell.innerText.trim();
                        if(attrName === "")
                            return;
                        const rawValue = valueCell.innerText.trim();
                        let parsedValue = rawValue;
                        if(rawValue === "true")
                            parsedValue = true;
                        else if(rawValue === "false")
                            parsedValue = false;
                        else if(rawValue !== "" && !isNaN(rawValue))
                            parsedValue = Number(rawValue);
                        item[attrName] = parsedValue;
                        inspector.showGroupBackground(selector.selected_background);
                    };

                    const blurOnEnter = function(evt)
                    {
                        if(evt.keyCode == 13)
                        {
                            evt.preventDefault();
                            evt.target.blur();
                        }
                    };

                    let movingFromLabelToValue = false;
                    const moveFocusToValue = function(evt)
                    {
                        if(evt.key == "Enter" || evt.key == "Tab")
                        {
                            evt.preventDefault();
                            movingFromLabelToValue = true;
                            setTimeout(function()
                            {
                                valueCell.focus();
                            }, 0);
                        }
                    };

                    labelCell.addEventListener("keydown", moveFocusToValue);
                    valueCell.addEventListener("keypress", blurOnEnter);
                    labelCell.addEventListener("blur", function()
                    {
                        labelCell.contentEditable = false;
                        labelCell.setAttribute("contenteditable", "false");
                        labelCell.classList.remove("textedit", "inspector-attr-cell");
                        labelCell.classList.add("inspector-attr-cell-locked");
                        labelCell.removeAttribute("data-placeholder");
                        labelCell.tabIndex = -1;
                        if(movingFromLabelToValue)
                        {
                            movingFromLabelToValue = false;
                            return;
                        }
                        commitAttribute();
                    });
                    labelCell.addEventListener("mousedown", function(evt)
                    {
                        if(labelCell.classList.contains("inspector-attr-cell-locked"))
                            evt.preventDefault();
                    });
                    valueCell.addEventListener("blur", commitAttribute);
                    labelCell.focus();
                };

                const requestNewAttributeRow = function()
                {
                    inspector.pendingAddAttributeRow = true;
                    const active = document.activeElement;
                    if(active && active.closest && active.closest("#component_inspector") && typeof active.blur == "function")
                        active.blur();
                    setTimeout(function()
                    {
                        if(inspector.pendingAddAttributeRow)
                            inspector.showGroupBackground(selector.selected_background);
                    }, 0);
                };

                plusButton.addEventListener("mousedown", function(evt)
                {
                    evt.preventDefault();
                    evt.stopPropagation();
                    requestNewAttributeRow();
                });

                plusButton.addEventListener("click", function(evt)
                {
                    evt.preventDefault();
                    evt.stopPropagation();
                });

                if(inspector.pendingAddAttributeRow)
                {
                    inspector.pendingAddAttributeRow = false;
                    openNewAttributeRow();
                }
            }
        }

        for(const p of countRows)
            inspector.addAttributeValue(p.key, p.value);

    },


    showSingleSelection(c) 
    {
        const item = network.dict[c];
        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';
    
        if (!item) 
        {
            inspector.addHeader("Internal Error");
            return;
        }
    
        if (!item._tag) {
            inspector.addHeader("Unknown Component");
            inspector.addAttributeValue("name", item.name);
            inspector.addDataRows(item, inspector.createTemplate(item));
            return;
        }
    
        const editMode = main.edit_mode;
        const colorOptions = "red,orange,yellow,green,blue,purple,pink,white,black";
        const colorOptionLabels = {
            red: "&#x1F7E5; red",
            orange: "&#x1F7E7; orange",
            yellow: "&#x1F7E8; yellow",
            green: "&#x1F7E9; green",
            blue: "&#x1F7E6; blue",
            purple: "&#x1F7EA; purple",
            pink: "&#x25A3; pink",
            white: "&#x2B1C; white",
            black: "&#x2B1B; black"
        };
        const commonDataRow = [
            {'name': 'name', 'control': 'textedit', 'type': 'source'},
            {'name': 'color', 'control': 'menu', 'type': 'source', 'options': colorOptions, 'option_labels': colorOptionLabels}
        ];
        const nameOnlyDataRow = [
            {'name': 'name', 'control': 'textedit', 'type': 'source'}
        ];
        const colorOnlyDataRow = [
            {'name': 'color', 'control': 'menu', 'type': 'source', 'options': colorOptions, 'option_labels': colorOptionLabels}
        ];
    
        switch (item._tag) {
            case "module":
                inspector.addHeader("MODULE");
                if (editMode) {
                    if(!item.color)
                        item.color = "black";
                    inspector.addDataRows(item, nameOnlyDataRow, inspector);
                    const moduleClassMenu = inspector.addMenu("class", item.class, network.classes);
                    const applyModuleClass = function () {
                        network.changeModuleClass(c, this.value);
                        selector.selectItems([c], null, false, false, true);
                    };
                    moduleClassMenu.addEventListener('input', applyModuleClass);
                    moduleClassMenu.addEventListener('change', applyModuleClass);
                    inspector.addDataRows(item, colorOnlyDataRow, inspector);

                    const template = item.parameters || [];
                    for (let key in template) {
                        //template[key].control = "textedit";
                    }
                    inspector.addDataRows(item, template, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                    inspector.addAttributeValue("class", item.class);
                }
                const alternatives = ["inherit", "quiet","exception","end_of_file","terminate","fatal_error","warning","print","debug","trace"];
                inspector.addMenu("log_level", alternatives[item.log_level], alternatives).addEventListener('change', function () { item.log_level=alternatives.indexOf(this.value); selector.setLogLevel(alternatives.indexOf(this.value)) });

                break;
            
            case "group":
                inspector.addHeader("GROUP");
                if (editMode) {
                    if(!item.color)
                        item.color = "black";
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "input":
                inspector.addHeader("INPUT");
                if (editMode) {
                    if(!item.color)
                        item.color = "black";
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "output":
                inspector.addHeader("OUTPUT");
                if (editMode) {
                    if(!item.color)
                        item.color = "black";
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "widget":
                const widgetContainer = document.getElementById(`${selector.selected_background}.${item.name}`);
                inspector.addHeader("WIDGET");
                if (editMode) {
                    const widgetClassMenu = inspector.addMenu("class", item.class, widget_classes);
                    const applyWidgetClass = function () {
                        network.changeWidgetClass(c, this.value);
                        selector.selectItems([c], null, false, false, true);
                    };
                    widgetClassMenu.addEventListener('input', applyWidgetClass);
                    widgetClassMenu.addEventListener('change', applyWidgetClass);
                    const template = widgetContainer.widget.parameter_template;
                    inspector.addDataRows(item, template, widgetContainer.widget);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
    
            default:
                inspector.addHeader("Unknown Tag");
                inspector.addAttributeValue("name", item.name);
                break;
        }
    },

    showConnection(connection)
    {
        const item = network.dict[connection];

        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';

        inspector.addHeader("CONNECTION");
        inspector.addAttributeValue("source", item.source);
        inspector.addAttributeValue("target", item.target);

            if(main.edit_mode)
            {
                if(!item.color)
                    item.color = "black";
                if(!item.line_type)
                    item.line_type = "auto_route";
                inspector.addDataRows(item, 
                [
                    {'name':'source_range', 'control':'textedit', 'type':'range'},
                    {'name':'target_range', 'control':'textedit', 'type':'range'},
                    {'name':'delay', 'control':'textedit', 'type':'delay'},
                    {'name':'alias', 'control':'textedit', 'type':'source'}     
                ], this);
                inspector.addDataRows(item,
                [
                    {
                        'name':'color',
                        'control':'menu',
                        'type':'source',
                        'options':'red,orange,yellow,green,blue,purple,pink,white,black',
                        'option_labels':{
                            red: "&#x1F7E5; red",
                            orange: "&#x1F7E7; orange",
                            yellow: "&#x1F7E8; yellow",
                            green: "&#x1F7E9; green",
                            blue: "&#x1F7E6; blue",
                            purple: "&#x1F7EA; purple",
                            pink: "&#x25A3; pink",
                            white: "&#x2B1C; white",
                            black: "&#x2B1B; black"
                        }
                    },
                    {
                        'name':'line_type',
                        'control':'menu',
                        'type':'source',
                        'options':'line,orthogonal,orthagonal rounded,spline,auto_route'
                    }
                ], this);
            }
            else
            {
                inspector.addAttributeValue("delay_range", item.delay_range);
                inspector.addAttributeValue("alias", item.alias);
            }
    },

    showMultipleSelection(n)
    {
        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'table';
        inspector.addHeader("Multiple");
        inspector.addAttributeValue("selected", n); 
    },

    showInspectorForSelection()
    {
        if(selector.selected_foreground.length > 0)
        {
            const signature = selector.selected_foreground.join("|");
            if(signature !== inspector.last_selected_signature)
            {
                inspector.resetComponentWidth();
                inspector.last_selected_signature = signature;
            }
        }
        else
        {
            inspector.last_selected_signature = "";
            inspector.resetComponentWidth();
        }

        if(selector.selected_connection)
            inspector.showConnection(selector.selected_connection)
        else if(selector.selected_foreground.length == 0)
            inspector.showGroupBackground(selector.selected_background);    
        else if(selector.selected_foreground.length > 1)
                inspector.showMultipleSelection(selector.selected_foreground.length);
            else
                inspector.showSingleSelection(selector.selected_foreground[0]);
    }               
}


const log = 
{
    height_cookie: 'log_height',
    resize_threshold: 12,
    min_height: 40,
    max_height_margin: 80,
    resize_active: false,

    isOnResizeBorder(evt)
    {
        if(!log.view)
            return false;
        const r = log.view.getBoundingClientRect();
        const y = evt.clientY - r.top;
        return y >= 0 && y <= log.resize_threshold;
    },

    updateResizeCursor(evt)
    {
        // Cursor is handled by CSS :hover on .log-resize-handle.
    },

    startResize(evt)
    {
        if(evt.button !== 0)
            return;
        const fromHandle = !!evt.target.closest('.log-resize-handle');
        if(!fromHandle && !log.isOnResizeBorder(evt))
            return;

        evt.preventDefault();
        log.resize_active = true;
        log.resize_start_y = evt.clientY;
        log.resize_start_height = log.view.getBoundingClientRect().height;
        document.body.classList.add('log-resizing');

        document.addEventListener('mousemove', log.onResizeDrag, true);
        document.addEventListener('mouseup', log.stopResize, true);
    },

    onResizeDrag(evt)
    {
        if(!log.resize_active || !log.view)
            return;

        const deltaY = evt.clientY - log.resize_start_y;
        const maxHeight = Math.max(log.min_height, window.innerHeight - log.max_height_margin);
        const newHeight = Math.max(log.min_height, Math.min(maxHeight, log.resize_start_height - deltaY));

        log.view.style.flex = `0 0 ${newHeight}px`;
        log.view.style.height = `${newHeight}px`;
        evt.preventDefault();
    },

    stopResize()
    {
        if(!log.resize_active)
            return;
        log.resize_active = false;
        document.removeEventListener('mousemove', log.onResizeDrag, true);
        document.removeEventListener('mouseup', log.stopResize, true);
        document.body.classList.remove('log-resizing');
        if(log.view)
        {
            const h = Math.round(log.view.getBoundingClientRect().height);
            if(Number.isFinite(h) && h >= log.min_height)
                setCookie(log.height_cookie, String(h));
        }
    },

    getMessagesElement()
    {
        if(!log.view)
            log.view = document.querySelector('footer');
        if(!log.view)
            return null;

        if(!log.messagesElement || !log.view.contains(log.messagesElement))
            log.messagesElement = log.view.querySelector('.log-messages');

        if(!log.messagesElement)
        {
            log.messagesElement = document.createElement('div');
            log.messagesElement.className = 'log-messages';
            log.view.appendChild(log.messagesElement);
        }

        return log.messagesElement;
    },

    init()
    {
        log.view = document.querySelector('footer');
        log.button = document.getElementById('log_toggle_button');
        log.messagesElement = log.getMessagesElement();
        if(log.view)
        {
            log.resizeHandle = log.view.querySelector('.log-resize-handle');
            if(!log.resizeHandle)
            {
                log.resizeHandle = document.createElement('div');
                log.resizeHandle.className = 'log-resize-handle';
                log.view.prepend(log.resizeHandle);
            }
            const savedHeight = parseInt(getCookie(log.height_cookie), 10);
            if(Number.isFinite(savedHeight) && savedHeight >= log.min_height)
            {
                const maxHeight = Math.max(log.min_height, window.innerHeight - log.max_height_margin);
                const clampedHeight = Math.min(savedHeight, maxHeight);
                log.view.style.flex = `0 0 ${clampedHeight}px`;
                log.view.style.height = `${clampedHeight}px`;
            }
            log.resizeHandle.addEventListener('mousedown', log.startResize, true);
        }
    },

    setAlert(state)
    {
        if(!log.button)
            return;
        log.button.classList.toggle('log-alert', !!state);
    },

    toggleLog()
    {
        log.setAlert(false);
        const s = window.getComputedStyle(log.view, null);
        if (s.display === 'none')
            log.view.style.display = 'block';
        else 
        log.view.style.display = 'none';
    },

    close()
    {
        log.setAlert(false);
        if(log.view)
            log.view.style.display = 'none';
    },

    showView(fromAlert=false)
    {
        log.view.style.display = 'block';
        if(fromAlert)
            log.setAlert(true);
    },

    clear()
    {
        const logElement = log.getMessagesElement();
        if(logElement)
            logElement.innerHTML = '';
        else if(log.view)
            log.view.querySelectorAll('p').forEach((e) => e.remove());
        log.setAlert(false);
    },

    print(message)
    {
        let logElement = log.getMessagesElement() || log.view || document.querySelector('.log');
        if(!logElement)
            return;
        let p = document.createElement('p');
        p.className = 'warning';
        p.appendChild(document.createTextNode(message));
        logElement.appendChild(p);
    }
}

/*
 *
 * Selector     -       select in 'navigator', 'breadcrums', 'inspector' and 'main'
 * 
 */

const selector =
{
    selected_foreground: [],
    selected_connection: null,
    selected_background: null,

    selectItems(foreground=[], background=null, toggle=false, extend=false, force_rebuild=false)
    {
        const previous_background = selector.selected_background;
        const previous_connection = selector.selected_connection;
        selector.selected_connection = null;

        // Select background

        if(background != null)
            selector.selected_background = background;

            setCookie("selected_background", selector.selected_background);

        const background_changed = (background != null && background !== previous_background);

        // Toggle foreground

        if(toggle)
            toggleStrings(selector.selected_foreground, foreground);
        else if(extend)
            selector.selected_foreground = [...new Set([...selector.selected_foreground , ...foreground])];
        else
            selector.selected_foreground = foreground;

        if(selector.selected_background == null)
            return;

        if(background_changed || force_rebuild)
        {
            if(selector.selected_foreground.length==0) // select background group
            {
                nav.selectItem(selector.selected_background);
                breadcrumbs.selectItem(selector.selected_background);    
                main.selectItem([], selector.selected_background);
                inspector.showInspectorForSelection();
            }
    
            else // select foreground components
            {
                main.selectItem(selector.selected_foreground, selector.selected_background);
                inspector.showInspectorForSelection();
            }
            return;
        }

        const needs_rebuild = selector.selected_foreground.some((name) => !document.getElementById(name));
        if(needs_rebuild)
        {
            main.selectItem(selector.selected_foreground, selector.selected_background);
            inspector.showInspectorForSelection();
            return;
        }

        // Background unchanged: update selection state without rebuilding the full view.
        if(previous_connection && previous_connection !== selector.selected_connection)
            main.deselectConnection(previous_connection);

        if(selector.selected_foreground.length==0)
        {
            nav.selectItem(selector.selected_background);
            breadcrumbs.selectItem(selector.selected_background);
        }

        main.updateComponentStates();
        inspector.showInspectorForSelection();
    },

    selectConnection(connection)
    {
        const previous_connection = selector.selected_connection;
        selector.selected_foreground = [];
        selector.selected_connection = connection;
        main.selectItem(selector.selected_foreground, selector.selected_background);
        if(previous_connection && previous_connection !== connection)
            main.deselectConnection(previous_connection);
        main.selectConnection(connection);
        inspector.showInspectorForSelection();
    },


    selectBackground()
    {
        if(selector.selected_foreground.length === 0 && selector.selected_connection == null)
            return;
        selector.selectItems([], selector.selected_background);
    },
   
   
    selectError(error)
    {
        let c = splitAtLastDot(error);
        let d = splitAtLastDot(c[0]);
        selector.selectItems([c[0]], d[0]);
        inspector.showSingleSelection(selector.selected_foreground[0]);
        inspector.showComponent();
        //breadcrums.selectItem(d[0]);    // FIXME: not working
        //nav.selectItem(d[0]);    // FIXME: not working
        main.setEditMode();
    },

    getLocalPath(s) // Remove outer path and range
    {
        return removeStringFromStart(getStringUpToBracket(s), selector.selected_background+'.');
    },

    setLogLevel(level)
    {
        let component = "";
        if(selector.selected_foreground.length == 1)
            component = selector.selected_foreground[0];

        controller.queueCommand("control", selector.getLocalPath(component)+".log_level", {"x":0, "y":0, "value":level});
    }
}

const main = 
{
    view: null,
    grid: null,
    connections: "",
    component_rectangles: [],
    output_debug_lines: [],
    input_debug_lines: [],
    horizontal_routing_lines: [],
    vertical_routing_lines: [],
    routing_grid_points: [],
    grid_spacing: 24,
    grid_active: false,
    edit_mode: false,
    map: {},

    new_position_x: 100,
    new_position_y: 100,

    init()
    {
        main.main = document.querySelector("#main");
        main.view = document.querySelector("#main_view");
        main.grid = document.querySelector("#main_grid");
        main.grid_canvas = document.querySelector("#main_grid_canvas");
        main.auto_routing_toggle_button = document.getElementById("auto_routing_toggle_button");
        main.createContextMenu();
        main.createComponentColorMenu();
        main.createWidgetMenu();
        main.drawGrid();
        window.addEventListener("resize", main.drawGrid, false);
        main.view.addEventListener("mousedown", main.startBackgroundSelection, false);
        main.view.addEventListener("contextmenu", main.showBackgroundContextMenu, false);
    },

    createContextMenu()
    {
        if(main.context_menu && main.context_menu.parentElement)
            return;

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <button type="button" class="main-context-menu-item" data-choice="Module">Module</button>
            <button type="button" class="main-context-menu-item" data-choice="Group">Group</button>
            <button type="button" class="main-context-menu-item" data-choice="Input">Input</button>
            <button type="button" class="main-context-menu-item" data-choice="Output">Output</button>
            <button type="button" class="main-context-menu-item" data-choice="Widget">Widget</button>
        `;
        document.body.appendChild(menu);
        main.context_menu = menu;

        menu.addEventListener("mousedown", function(evt)
        {
            evt.stopPropagation();
        }, true);

        menu.addEventListener("click", function(evt)
        {
            const button = evt.target.closest(".main-context-menu-item");
            if(!button)
                return;
            main.onContextMenuChoice(button.dataset.choice || "");
            main.hideContextMenu();
            evt.preventDefault();
            evt.stopPropagation();
        }, false);

        document.addEventListener("mousedown", function(evt)
        {
            if(main.context_menu_visible && main.context_menu && !main.context_menu.contains(evt.target))
                main.hideContextMenu();
            if(main.component_color_menu_visible && main.component_color_menu && !main.component_color_menu.contains(evt.target))
                main.hideComponentColorMenu();
            if(main.widget_menu_visible && main.widget_menu && !main.widget_menu.contains(evt.target))
                main.hideWidgetMenu();
        }, true);

        document.addEventListener("keydown", function(evt)
        {
            if(evt.key === "Escape")
            {
                main.hideContextMenu();
                main.hideComponentColorMenu();
                main.hideWidgetMenu();
            }
        }, true);

        document.addEventListener("scroll", function(evt)
        {
            const target = evt ? evt.target : null;
            if(
                target &&
                target.closest &&
                target.closest(".main-context-menu")
            )
                return;
            main.hideContextMenu();
            main.hideComponentColorMenu();
            main.hideWidgetMenu();
        }, true);

        window.addEventListener("blur", function()
        {
            main.hideContextMenu();
            main.hideComponentColorMenu();
            main.hideWidgetMenu();
        }, false);
    },

    showContextMenuAt(clientX, clientY)
    {
        if(!main.context_menu)
            return;

        main.hideComponentColorMenu();
        main.context_menu.style.display = "block";
        const rect = main.context_menu.getBoundingClientRect();
        const maxLeft = Math.max(0, window.innerWidth - rect.width - 4);
        const maxTop = Math.max(0, window.innerHeight - rect.height - 4);
        const left = Math.max(0, Math.min(clientX, maxLeft));
        const top = Math.max(0, Math.min(clientY, maxTop));
        main.context_menu.style.left = `${left}px`;
        main.context_menu.style.top = `${top}px`;
        main.context_menu_visible = true;
    },

    hideContextMenu()
    {
        if(!main.context_menu)
            return;
        main.context_menu.style.display = "none";
        main.context_menu_visible = false;
    },

    createComponentColorMenu()
    {
        if(main.component_color_menu && main.component_color_menu.parentElement)
            return;

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <div class="main-context-submenu component-class-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">class</button>
                <div class="main-context-submenu-panel main-context-class-submenu-panel" role="menu" aria-label="Class"></div>
            </div>
            <div class="main-context-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">color</button>
                <div class="main-context-submenu-panel" role="menu" aria-label="Color">
                    <button type="button" class="main-context-color-swatch" data-color="red" title="red" aria-label="red"><span class="main-context-color-chip" style="background:#d84a4a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="orange" title="orange" aria-label="orange"><span class="main-context-color-chip" style="background:#e58e3a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="yellow" title="yellow" aria-label="yellow"><span class="main-context-color-chip" style="background:#efe26a"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="green" title="green" aria-label="green"><span class="main-context-color-chip" style="background:#58a663"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="blue" title="blue" aria-label="blue"><span class="main-context-color-chip" style="background:#4f79c8"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="purple" title="purple" aria-label="purple"><span class="main-context-color-chip" style="background:#7f59b3"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="pink" title="pink" aria-label="pink"><span class="main-context-color-chip" style="background:#d77db2"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="white" title="white" aria-label="white"><span class="main-context-color-chip" style="background:#f5f5f5"></span></button>
                    <button type="button" class="main-context-color-swatch" data-color="black" title="black" aria-label="black"><span class="main-context-color-chip" style="background:#1a1a1a"></span></button>
                </div>
            </div>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="duplicate">Duplicate</button>
            <button type="button" class="main-context-menu-item" data-action="delete">Delete</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="rename">Rename</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="show-inspector">Show Inspector...</button>
        `;
        document.body.appendChild(menu);
        main.component_color_menu = menu;
        main.component_class_submenu = menu.querySelector(".component-class-submenu");
        main.component_class_submenu_panel = menu.querySelector(".main-context-class-submenu-panel");

        menu.addEventListener("mousedown", function(evt)
        {
            evt.stopPropagation();
        }, true);

        menu.addEventListener("click", function(evt)
        {
            const classButton = evt.target.closest(".main-context-class-item");
            if(classButton)
            {
                main.setComponentClass(classButton.dataset.className || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const button = evt.target.closest(".main-context-color-swatch");
            if(button)
            {
                main.setComponentColor(button.dataset.color || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const actionButton = evt.target.closest(".main-context-menu-item[data-action]");
            if(actionButton)
            {
                main.handleComponentMenuAction(actionButton.dataset.action || "");
                main.hideComponentColorMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            main.hideComponentColorMenu();
        }, false);
    },

    createWidgetMenu()
    {
        if(main.widget_menu && main.widget_menu.parentElement)
            main.widget_menu.remove();

        const menu = document.createElement("div");
        menu.className = "main-context-menu";
        menu.innerHTML = `
            <div class="main-context-submenu widget-class-submenu">
                <button type="button" class="main-context-menu-item main-context-submenu-trigger">class</button>
                <div class="main-context-submenu-panel main-context-class-submenu-panel" role="menu" aria-label="Widget Class"></div>
            </div>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="toggle-title">Hide Title</button>
            <button type="button" class="main-context-menu-item" data-action="toggle-frame">Hide Frame</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="bring-to-front">Bring to Front</button>
            <button type="button" class="main-context-menu-item" data-action="bring-to-back">Send to Back</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="duplicate">Duplicate</button>
            <button type="button" class="main-context-menu-item" data-action="delete">Delete</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="rename">Rename</button>
            <div class="main-context-menu-separator" role="separator" aria-hidden="true"></div>
            <button type="button" class="main-context-menu-item" data-action="show-inspector">Show Inspector...</button>
        `;
        document.body.appendChild(menu);
        main.widget_menu = menu;
        main.widget_class_submenu = menu.querySelector(".widget-class-submenu");
        main.widget_class_submenu_panel = menu.querySelector(".main-context-class-submenu-panel");

        menu.addEventListener("mousedown", function(evt)
        {
            evt.stopPropagation();
        }, true);

        menu.addEventListener("click", function(evt)
        {
            const classButton = evt.target.closest(".main-context-class-item");
            if(classButton)
            {
                main.setWidgetClass(classButton.dataset.className || "");
                main.hideWidgetMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            const actionButton = evt.target.closest(".main-context-menu-item[data-action]");
            if(actionButton)
            {
                main.handleWidgetMenuAction(actionButton.dataset.action || "");
                main.hideWidgetMenu();
                evt.preventDefault();
                evt.stopPropagation();
                return;
            }
            main.hideWidgetMenu();
        }, false);
    },

    handleComponentMenuAction(action)
    {
        if(!main.edit_mode)
            return;
        const fullName = main.component_color_target;
        if(!fullName || !network.dict[fullName])
            return;
        if(!selector.selected_foreground.includes(fullName))
            selector.selectItems([fullName], selector.selected_background);
        switch((action || "").toLowerCase())
        {
            case "duplicate":
                main.duplicateSelectedComponents(false);
                break;
            case "delete":
                main.deleteComponent();
                break;
            case "rename":
                main.startInlineNameEditForComponent(fullName);
                break;
            case "show-inspector":
                if(inspector && typeof inspector.showInspectorForSelection === "function")
                    inspector.showInspectorForSelection();
                if(inspector && typeof inspector.showComponent === "function")
                    inspector.showComponent();
                break;
        }
    },

    handleWidgetMenuAction(action)
    {
        if(!main.edit_mode)
            return;
        const fullName = main.widget_menu_target;
        const item = fullName ? network.dict[fullName] : null;
        if(!fullName || !item || item._tag !== "widget")
            return;
        if(!selector.selected_foreground.includes(fullName))
            selector.selectItems([fullName], selector.selected_background);
        switch((action || "").toLowerCase())
        {
            case "toggle-title":
                main.toggleWidgetParameter("show_title");
                break;
            case "toggle-frame":
                main.toggleWidgetParameter("show_frame");
                break;
            case "bring-to-front":
                main.reorderWidget(fullName, "front");
                break;
            case "bring-to-back":
                main.reorderWidget(fullName, "back");
                break;
            case "duplicate":
                main.duplicateSelectedComponents(false);
                break;
            case "delete":
                main.deleteComponent();
                break;
            case "rename":
                if(!item.show_title)
                {
                    if(inspector && typeof inspector.showComponent === "function")
                        inspector.showComponent();
                    if(inspector && typeof inspector.showInspectorForSelection === "function")
                        inspector.showInspectorForSelection();
                    setTimeout(function() { inspector.focusEditableField("title"); }, 0);
                }
                else
                    main.startInlineNameEditForComponent(fullName);
                break;
            case "show-inspector":
                if(inspector && typeof inspector.showInspectorForSelection === "function")
                    inspector.showInspectorForSelection();
                if(inspector && typeof inspector.showComponent === "function")
                    inspector.showComponent();
                break;
        }
    },

    populateWidgetClassSubmenu(widgetFullName)
    {
        if(!main.widget_class_submenu || !main.widget_class_submenu_panel)
            return;

        const item = network.dict[widgetFullName];
        if(!item || item._tag !== "widget")
        {
            main.widget_class_submenu.style.display = "none";
            main.widget_class_submenu_panel.innerHTML = "";
            return;
        }

        main.widget_class_submenu.style.display = "";
        main.widget_class_submenu_panel.innerHTML = "";
        for(const className of widget_classes)
        {
            const b = document.createElement("button");
            b.type = "button";
            b.className = "main-context-class-item";
            if(className === item.class)
                b.classList.add("selected");
            b.dataset.className = className;
            b.textContent = className;
            main.widget_class_submenu_panel.appendChild(b);
        }

        const titleToggle = main.widget_menu.querySelector('.main-context-menu-item[data-action="toggle-title"]');
        const frameToggle = main.widget_menu.querySelector('.main-context-menu-item[data-action="toggle-frame"]');
        if(titleToggle)
            titleToggle.textContent = item.show_title ? "Hide Title" : "Show Title";
        if(frameToggle)
            frameToggle.textContent = item.show_frame ? "Hide Frame" : "Show Frame";
    },

    getComponentColorTargetFromElement(element)
    {
        if(!element || !element.closest)
            return null;

        let gi = null;
        const titleCell = element.closest("td.title");
        if(titleCell)
            gi = titleCell.closest(".gi");
        else
            gi = element.closest(".group_input, .group_output");

        if(!gi || !gi.dataset || !gi.dataset.name)
            return null;

        const fullName = gi.dataset.name;
        const item = network.dict[fullName];
        if(!item)
            return null;
        if(!["module", "group", "input", "output"].includes(item._tag))
            return null;

        return fullName;
    },

    showComponentColorMenuAt(clientX, clientY, componentFullName, preferredSubmenu="color")
    {
        if(!main.component_color_menu || !componentFullName)
            return;

        main.hideContextMenu();
        main.populateComponentClassSubmenu(componentFullName);
        main.component_color_target = componentFullName;
        if(preferredSubmenu === "class")
        {
            main.component_color_menu.classList.add("open-class-submenu");
            main.component_color_menu.classList.add("open-class-only");
        }
        else
        {
            main.component_color_menu.classList.remove("open-class-submenu");
            main.component_color_menu.classList.remove("open-class-only");
        }
        main.component_color_menu.style.display = "block";

        const rect = main.component_color_menu.getBoundingClientRect();
        const maxLeft = Math.max(0, window.innerWidth - rect.width - 4);
        const maxTop = Math.max(0, window.innerHeight - rect.height - 4);
        const left = Math.max(0, Math.min(clientX, maxLeft));
        const top = Math.max(0, Math.min(clientY, maxTop));
        main.component_color_menu.style.left = `${left}px`;
        main.component_color_menu.style.top = `${top}px`;
        main.component_color_menu_visible = true;
    },

    hideComponentColorMenu()
    {
        if(!main.component_color_menu)
            return;
        main.component_color_menu.style.display = "none";
        main.component_color_menu.classList.remove("open-class-submenu");
        main.component_color_menu.classList.remove("open-class-only");
        main.component_color_menu_visible = false;
        main.component_color_target = null;
    },

    showWidgetMenuAt(clientX, clientY, widgetFullName)
    {
        if(!main.widget_menu || !widgetFullName)
            return;

        main.hideContextMenu();
        main.hideComponentColorMenu();
        main.populateWidgetClassSubmenu(widgetFullName);
        main.widget_menu_target = widgetFullName;
        main.widget_menu.style.display = "block";

        const rect = main.widget_menu.getBoundingClientRect();
        const maxLeft = Math.max(0, window.innerWidth - rect.width - 4);
        const maxTop = Math.max(0, window.innerHeight - rect.height - 4);
        const left = Math.max(0, Math.min(clientX, maxLeft));
        const top = Math.max(0, Math.min(clientY, maxTop));
        main.widget_menu.style.left = `${left}px`;
        main.widget_menu.style.top = `${top}px`;
        main.widget_menu_visible = true;
    },

    hideWidgetMenu()
    {
        if(!main.widget_menu)
            return;
        main.widget_menu.style.display = "none";
        main.widget_menu_visible = false;
        main.widget_menu_target = null;
    },

    setWidgetClass(className)
    {
        const fullName = main.widget_menu_target;
        if(!fullName || !className)
            return;
        const item = network.dict[fullName];
        if(!item || item._tag !== "widget")
            return;

        network.changeWidgetClass(fullName, className);
        selector.selectItems([fullName], null, false, false, true);
        network.tainted = true;
    },

    toggleWidgetParameter(parameterName)
    {
        const fullName = main.widget_menu_target;
        if(!fullName || !parameterName)
            return;
        const item = network.dict[fullName];
        const widgetFrame = document.getElementById(fullName);
        if(!item || item._tag !== "widget" || !widgetFrame || !widgetFrame.widget)
            return;

        item[parameterName] = !widgetFrame.widget.toBool(item[parameterName]);
        widgetFrame.widget.parameters[parameterName] = item[parameterName];
        try
        {
            widgetFrame.widget.updateAll();
        }
        catch(err)
        {
            console.log(err);
        }
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        network.tainted = true;
    },

    reorderWidget(fullName, direction)
    {
        if(!fullName)
            return;
        const group = network.dict[selector.selected_background];
        const item = network.dict[fullName];
        if(!group || !item || item._tag !== "widget" || !Array.isArray(group.widgets))
            return;

        const index = group.widgets.findIndex((widget) => widget && widget.name === item.name);
        if(index < 0)
            return;

        const [widget] = group.widgets.splice(index, 1);
        if(direction === "back")
            group.widgets.unshift(widget);
        else
            group.widgets.push(widget);

        network.rebuildDict();
        selector.selectItems([fullName], selector.selected_background, false, false, true);
        network.tainted = true;
    },

    setComponentColor(color)
    {
        const fullName = main.component_color_target;
        if(!fullName || !color)
            return;
        const item = network.dict[fullName];
        if(!item || !["module", "group", "input", "output"].includes(item._tag))
            return;
        item.color = color;
        main.applyComponentColorToElement(document.getElementById(fullName), item);
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        network.tainted = true;
    },

    populateComponentClassSubmenu(componentFullName)
    {
        if(!main.component_class_submenu || !main.component_class_submenu_panel)
            return;

        const item = network.dict[componentFullName];
        if(!item || item._tag !== "module" || !Array.isArray(network.classes) || network.classes.length === 0)
        {
            main.component_class_submenu.style.display = "none";
            main.component_class_submenu_panel.innerHTML = "";
            return;
        }

        main.component_class_submenu.style.display = "";
        main.component_class_submenu_panel.innerHTML = "";
        for(const className of network.classes)
        {
            const b = document.createElement("button");
            b.type = "button";
            b.className = "main-context-class-item";
            if(className === item.class)
                b.classList.add("selected");
            b.dataset.className = className;
            b.textContent = className;
            main.component_class_submenu_panel.appendChild(b);
        }
    },

    setComponentClass(className)
    {
        const fullName = main.component_color_target;
        if(!fullName || !className)
            return;
        const item = network.dict[fullName];
        if(!item || item._tag !== "module")
            return;

        network.changeModuleClass(fullName, className);
        selector.selectItems([fullName], null, false, false, true);
        network.tainted = true;
    },

    openComponentColorMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".module-title-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi") ? button.closest(".gi").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showComponentColorMenuAt(r.right + 2, r.top, fullName, "color");
    },

    openComponentClassMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".module-class-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi.module") ? button.closest(".gi.module").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showComponentColorMenuAt(r.right + 2, r.top, fullName, "class");
    },

    openWidgetMenuFromButton(evt)
    {
        if(!main.edit_mode)
            return;
        evt.preventDefault();
        evt.stopPropagation();
        const button = evt.currentTarget || evt.target.closest(".widget-title-menu-button");
        if(!button)
            return;
        const fullName = button.dataset.component || (button.closest(".gi.widget") ? button.closest(".gi.widget").dataset.name : null);
        if(!fullName)
            return;
        const r = button.getBoundingClientRect();
        main.showWidgetMenuAt(r.right + 2, r.top, fullName);
    },

    getSnappedBackgroundPositionFromEvent(evt)
    {
        const viewRect = main.view.getBoundingClientRect();
        let x = evt.clientX - viewRect.left;
        let y = evt.clientY - viewRect.top;
        if(x < 0) x = 0;
        if(y < 0) y = 0;
        const g = main.grid_spacing || 1;
        x = g * Math.round(x / g);
        y = g * Math.round(y / g);
        return {x, y};
    },

    showBackgroundContextMenu(evt)
    {
        if(!main.edit_mode)
            return;

        const isModuleTitleRow = !!(evt.target && evt.target.closest && evt.target.closest(".gi.module td.title"));
        if(isModuleTitleRow)
            return;

        const colorTarget = main.getComponentColorTargetFromElement(evt.target);
        if(colorTarget)
        {
            evt.preventDefault();
            evt.stopPropagation();
            main.showComponentColorMenuAt(evt.clientX, evt.clientY, colorTarget, "color");
            return;
        }

        if(evt.target !== main.view)
            return;

        evt.preventDefault();
        evt.stopPropagation();
        main.context_menu_position = main.getSnappedBackgroundPositionFromEvent(evt);
        main.showContextMenuAt(evt.clientX, evt.clientY);
    },

    onContextMenuChoice(choice)
    {
        if(main.context_menu_position)
        {
            main.new_position_x = main.context_menu_position.x;
            main.new_position_y = main.context_menu_position.y;
        }

        switch(choice)
        {
            case "Module":
                main.newModule();
                break;
            case "Group":
                main.newGroup();
                break;
            case "Input":
                main.newInput();
                break;
            case "Output":
                main.newOutput();
                break;
            case "Widget":
                main.newWidget();
                break;
        }

        main.context_menu_position = null;
    },

    ensureSelectionBox()
    {
        if(main.selection_box && main.selection_box.parentElement)
            return;
        main.selection_box = document.createElement("div");
        main.selection_box.className = "selection-rectangle";
        main.selection_box.style.display = "none";
        main.view.appendChild(main.selection_box);
    },

    startBackgroundSelection(evt)
    {
        main.hideContextMenu();
        main.hideComponentColorMenu();
        if(main.inline_name_edit)
            main.finishInlineNameEdit(false);
        if(evt.button !== 0)
            return;
        if(evt.target !== main.view)
            return;

        if(!main.edit_mode)
        {
            selector.selectBackground();
            return;
        }

        main.ensureSelectionBox();
        main.selection_drag_active = true;
        main.selection_drag_shift = evt.shiftKey;
        main.selection_base_foreground = [...selector.selected_foreground];
        main.selection_start_x = evt.clientX;
        main.selection_start_y = evt.clientY;
        main.selection_moved = false;

        main.selection_box.style.left = `${evt.offsetX}px`;
        main.selection_box.style.top = `${evt.offsetY}px`;
        main.selection_box.style.width = "0px";
        main.selection_box.style.height = "0px";
        main.selection_box.style.display = "block";

        main.view.addEventListener("mousemove", main.updateBackgroundSelection, true);
        main.view.addEventListener("mouseup", main.finishBackgroundSelection, true);
        evt.preventDefault();
        evt.stopPropagation();
    },

    collectBackgroundSelection(left, top, width, height)
    {
        const x1 = left;
        const y1 = top;
        const x2 = left + width;
        const y2 = top + height;
        const viewRect = main.view.getBoundingClientRect();
        const selected = [];

        for(const element of main.view.querySelectorAll(".gi"))
        {
            const r = element.getBoundingClientRect();
            const ex1 = r.left - viewRect.left;
            const ey1 = r.top - viewRect.top;
            const ex2 = ex1 + r.width;
            const ey2 = ey1 + r.height;

            const intersects = !(ex2 < x1 || ex1 > x2 || ey2 < y1 || ey1 > y2);
            if(intersects && element.dataset.name)
                selected.push(element.dataset.name);
        }

        return selected;
    },

    applyBackgroundSelection(selected, toggle_selection)
    {
        if(toggle_selection)
        {
            const base = new Set(main.selection_base_foreground || []);
            for(const name of selected)
            {
                if(base.has(name))
                    base.delete(name);
                else
                    base.add(name);
            }
            selector.selectItems(Array.from(base), null, false, false);
            return;
        }

        if(selected.length === 0)
        {
            selector.selectBackground();
            return;
        }

        selector.selectItems(selected, null, false, false);
    },

    updateBackgroundSelection(evt)
    {
        if(!main.selection_drag_active || !main.selection_box)
            return;

        const viewRect = main.view.getBoundingClientRect();
        const x1 = Math.max(0, Math.min(main.selection_start_x - viewRect.left, main.view.clientWidth));
        const y1 = Math.max(0, Math.min(main.selection_start_y - viewRect.top, main.view.clientHeight));
        const x2 = Math.max(0, Math.min(evt.clientX - viewRect.left, main.view.clientWidth));
        const y2 = Math.max(0, Math.min(evt.clientY - viewRect.top, main.view.clientHeight));

        const left = Math.min(x1, x2);
        const top = Math.min(y1, y2);
        const width = Math.abs(x2 - x1);
        const height = Math.abs(y2 - y1);

        if(width > 2 || height > 2)
            main.selection_moved = true;

        main.selection_box.style.left = `${left}px`;
        main.selection_box.style.top = `${top}px`;
        main.selection_box.style.width = `${width}px`;
        main.selection_box.style.height = `${height}px`;
    },

    finishBackgroundSelection(evt)
    {
        if(!main.selection_drag_active)
            return;

        main.view.removeEventListener("mousemove", main.updateBackgroundSelection, true);
        main.view.removeEventListener("mouseup", main.finishBackgroundSelection, true);

        if(main.selection_box)
            main.selection_box.style.display = "none";

        if(!main.selection_moved)
        {
            const toggle_selection = main.selection_drag_shift || evt.shiftKey;
            if(!toggle_selection)
                selector.selectBackground();
            main.selection_drag_active = false;
            main.selection_base_foreground = [];
            return;
        }

        const viewRect = main.view.getBoundingClientRect();
        const x1 = Math.max(0, Math.min(main.selection_start_x - viewRect.left, main.view.clientWidth));
        const y1 = Math.max(0, Math.min(main.selection_start_y - viewRect.top, main.view.clientHeight));
        const x2 = Math.max(0, Math.min(evt.clientX - viewRect.left, main.view.clientWidth));
        const y2 = Math.max(0, Math.min(evt.clientY - viewRect.top, main.view.clientHeight));
        const left = Math.min(x1, x2);
        const top = Math.min(y1, y2);
        const width = Math.abs(x2 - x1);
        const height = Math.abs(y2 - y1);
        const toggle_selection = main.selection_drag_shift || evt.shiftKey;
        const selected = main.collectBackgroundSelection(left, top, width, height);
        main.applyBackgroundSelection(selected, toggle_selection);

        main.selection_drag_active = false;
        main.selection_base_foreground = [];
        evt.preventDefault();
        evt.stopPropagation();
    },

    ensureGridCanvasSize()
    {
        if(!main.grid_canvas || !main.main || !main.view)
            return;
        const dpr = window.devicePixelRatio || 1;
        const minSize = 3000;
        const targetWidth = Math.max(minSize, main.main.clientWidth, main.view.clientWidth);
        const targetHeight = Math.max(minSize, main.main.clientHeight, main.view.clientHeight);
        const backingWidth = Math.floor(targetWidth * dpr);
        const backingHeight = Math.floor(targetHeight * dpr);
        if(main.grid_canvas.width !== backingWidth)
            main.grid_canvas.width = backingWidth;
        if(main.grid_canvas.height !== backingHeight)
            main.grid_canvas.height = backingHeight;
        main.grid_canvas.style.width = `${targetWidth}px`;
        main.grid_canvas.style.height = `${targetHeight}px`;
        main.grid_canvas_css_width = targetWidth;
        main.grid_canvas_css_height = targetHeight;
        main.grid_canvas_dpr = dpr;
    },

    drawGrid()
    {
        main.ensureGridCanvasSize();
        const ctx = main.grid_canvas.getContext("2d");
        const width = main.grid_canvas_css_width || main.grid_canvas.width;
        const height = main.grid_canvas_css_height || main.grid_canvas.height;
        const dpr = main.grid_canvas_dpr || 1;
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        ctx.fillStyle = "white";
        ctx.fillRect(0, 0, width, height);
        ctx.lineWidth = 1 / dpr;
        ctx.strokeStyle = "gray";
        const maxDim = Math.max(width, height);
        const pixelAlign = 0.5 / dpr;
        for(let x = main.grid_spacing; x < maxDim; x += main.grid_spacing)
        {
            const xx = x + pixelAlign;
            ctx.beginPath();
            ctx.moveTo(xx, 0);
            ctx.lineTo(xx, height);
            ctx.moveTo(0,xx);
            ctx.lineTo(width,xx);
            ctx.stroke();
        }
    },

    increaseGrid()
    {
        main.grid_spacing = main.grid_spacing*2;
        if(main.grid_spacing > 192)
        main.grid_spacing = 192;
        main.drawGrid();
    },

    decreaseGrid()
    {
        main.grid_spacing = main.grid_spacing/2;
        if(main.grid_spacing < 8)
        main.grid_spacing = 8;
        main.drawGrid();
    },

    centerComponents()
    {
        const g = network.dict[selector.selected_background];
        const sum_x = 0;
        const sum_y = 0;
        const n = 0;
        const comps = [...g.groups||[], ...g.modules||[], ...g.inputs||[], ...g.outputs||[], ...g.widgets||[]];
        for(let c of comps)
        {
            sum_x += parseInt(c._x);
            sum_y += parseInt(c._y);
            n += 1;
        }

        if(n==0)
            return;

        const old_center_x = sum_x/n;
        const old_center_y = sum_y/n;    
        const new_center_x = main.main.offsetWidth/2;
        const new_center_y = main.main.offsetHeight/2;
        const dx = new_center_x-old_center_x;
        const dy = new_center_y-old_center_y;

        for(let c of comps)
        {
            c._x = `${parseInt(c._x) + dx}`;
            c._y = `${parseInt(c._y) + dy}`;
        }
        selector.selectItems([], null);
    },

    getArrangeItems(group)
    {
        const items = [];
        const pushItem = function(localName, object)
        {
            if(!localName || !object)
                return;
            const fullName = `${selector.selected_background}.${localName}`;
            const element = document.getElementById(fullName);
            let width = element ? element.offsetWidth : 0;
            let height = element ? element.offsetHeight : 0;
            if(width <= 0 || height <= 0)
            {
                if(object._tag == "widget")
                {
                    width = parseInt(object.width) || 200;
                    height = parseInt(object.height) || 140;
                }
                else if(object._tag == "input" || object._tag == "output")
                {
                    width = 140;
                    height = 26;
                }
                else
                {
                    const inCount = (object.inputs || []).length;
                    const outCount = (object.outputs || []).length;
                    width = 220;
                    height = 34 + Math.max(inCount, outCount) * 20;
                }
            }
            items.push({
                localName,
                fullName,
                object,
                width: Math.max(60, width),
                height: Math.max(20, height),
                inDegree: 0,
                outDegree: 0,
                layer: 0
            });
        };

        for(const g of group.groups || [])
            pushItem(g.name, g);
        for(const m of group.modules || [])
            pushItem(m.name, m);
        for(const i of group.inputs || [])
            pushItem(i.name, i);
        for(const o of group.outputs || [])
            pushItem(o.name, o);
        // Widgets are intentionally excluded from auto-arrange.
        // They keep their current positions.
        return items;
    },

    getBackgroundAutoRoutingGroup()
    {
        const background = selector.selected_background;
        const group = background ? network.dict[background] : null;
        if(!group)
            return null;
        if(group.auto_routing === undefined)
            group.auto_routing = false;
        return group;
    },

    updateAutoRoutingButtonState()
    {
        if(!main.auto_routing_toggle_button)
            return;
        const group = main.getBackgroundAutoRoutingGroup();
        const enabled = !!(group && group.auto_routing);
        main.auto_routing_toggle_button.classList.toggle("selected", enabled);
        main.auto_routing_toggle_button.setAttribute("aria-pressed", enabled ? "true" : "false");
        main.auto_routing_toggle_button.title = enabled ? "Disable automatic routing" : "Enable automatic routing";
    },

    toggleBackgroundAutoRouting()
    {
        const group = main.getBackgroundAutoRoutingGroup();
        if(!group)
            return;
        group.auto_routing = !group.auto_routing;
        network.tainted = true;
        main.updateAutoRoutingButtonState();
        if(inspector && typeof inspector.showInspectorForSelection === "function" && selector.selected_foreground.length === 0)
            inspector.showInspectorForSelection();
        main.addConnections();
    },

    arrangeComponents()
    {
        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const nodes = main.getArrangeItems(group);
        if(nodes.length === 0)
            return;

        const nodeMap = new Map(nodes.map((n) => [n.localName, n]));
        const outgoing = new Map(nodes.map((n) => [n.localName, new Set()]));
        const incoming = new Map(nodes.map((n) => [n.localName, new Set()]));

        for(const c of group.connections || [])
        {
            const sourcePath = getStringUpToBracket(c.source || "");
            const targetPath = getStringUpToBracket(c.target || "");
            const sourceLocal = sourcePath.split(".")[0];
            const targetLocal = targetPath.split(".")[0];
            if(!nodeMap.has(sourceLocal) || !nodeMap.has(targetLocal) || sourceLocal == targetLocal)
                continue;
            outgoing.get(sourceLocal).add(targetLocal);
            incoming.get(targetLocal).add(sourceLocal);
        }

        for(const n of nodes)
        {
            n.outDegree = outgoing.get(n.localName).size;
            n.inDegree = incoming.get(n.localName).size;
        }

        const inputNodes = nodes.filter((n) => n.object && n.object._tag === "input");
        const outputNodes = nodes.filter((n) => n.object && n.object._tag === "output");
        const middleCandidates = nodes.filter((n) => !inputNodes.includes(n) && !outputNodes.includes(n));

        const sourceNodes = nodes.filter((n) => n.inDegree === 0 && n.outDegree > 0);
        const sinkNodes = nodes.filter((n) => n.outDegree === 0 && n.inDegree > 0);
        const isolatedNodes = nodes.filter((n) => n.inDegree === 0 && n.outDegree === 0);
        const middleNodes = nodes.filter((n) => n.inDegree > 0 && n.outDegree > 0);

        const queue = [];
        const visited = new Set();
        if(sourceNodes.length > 0)
        {
            for(const s of sourceNodes)
            {
                s.layer = 0;
                queue.push(s.localName);
                visited.add(s.localName);
            }
        }
        else
        {
            for(const n of middleNodes)
            {
                n.layer = 1;
                queue.push(n.localName);
                visited.add(n.localName);
            }
        }

        while(queue.length > 0)
        {
            const u = queue.shift();
            const nodeU = nodeMap.get(u);
            for(const v of outgoing.get(u))
            {
                const nodeV = nodeMap.get(v);
                const nextLayer = nodeU.layer + 1;
                if(nextLayer > nodeV.layer)
                    nodeV.layer = nextLayer;
                if(!visited.has(v))
                {
                    visited.add(v);
                    queue.push(v);
                }
            }
        }

        let maxLayer = 0;
        for(const n of nodes)
            maxLayer = Math.max(maxLayer, n.layer);

        for(const n of sinkNodes)
            n.layer = Math.max(n.layer, maxLayer + 1);
        maxLayer = Math.max(maxLayer + (sinkNodes.length ? 1 : 0), ...nodes.map((n) => n.layer));

        for(const n of isolatedNodes)
            n.layer = Math.max(1, Math.floor(maxLayer / 2));

        // Force all inputs to the first (leftmost) layer.
        for(const n of inputNodes)
            n.layer = 0;

        // Keep non-input/non-output nodes in the middle layers (start at layer 1).
        for(const n of middleCandidates)
            n.layer = Math.max(1, n.layer + 1);

        // Force all outputs to the last (rightmost) layer.
        let maxMiddleLayer = 1;
        for(const n of middleCandidates)
            maxMiddleLayer = Math.max(maxMiddleLayer, n.layer);
        for(const n of outputNodes)
            n.layer = maxMiddleLayer + 1;

        const layers = new Map();
        for(const n of nodes)
        {
            if(!layers.has(n.layer))
                layers.set(n.layer, []);
            layers.get(n.layer).push(n);
        }

        const layerIndices = Array.from(layers.keys()).sort((a,b)=>a-b);
        const getLayerPositionMap = function(layerNodes)
        {
            const m = new Map();
            for(let i = 0; i < layerNodes.length; i++)
                m.set(layerNodes[i].localName, i);
            return m;
        };

        const reorderByNeighborLayer = function(layerIndex, neighborLayerIndex, useIncoming)
        {
            const currentLayer = layers.get(layerIndex);
            const neighborLayer = layers.get(neighborLayerIndex);
            if(!currentLayer || !neighborLayer || currentLayer.length <= 1)
                return;

            const neighborPos = getLayerPositionMap(neighborLayer);
            const currentPos = getLayerPositionMap(currentLayer);
            currentLayer.sort((a, b) => {
                const neighA = useIncoming ? Array.from(incoming.get(a.localName)) : Array.from(outgoing.get(a.localName));
                const neighB = useIncoming ? Array.from(incoming.get(b.localName)) : Array.from(outgoing.get(b.localName));
                const avgA = neighA.length ? neighA.map((k) => neighborPos.get(k)).filter((v)=>v!==undefined).reduce((p,c)=>p+c,0) / neighA.length : currentPos.get(a.localName);
                const avgB = neighB.length ? neighB.map((k) => neighborPos.get(k)).filter((v)=>v!==undefined).reduce((p,c)=>p+c,0) / neighB.length : currentPos.get(b.localName);
                if(avgA !== avgB)
                    return avgA - avgB;
                return currentPos.get(a.localName) - currentPos.get(b.localName);
            });
        };

        // Crossing-minimization sweeps (second pass): top-down and bottom-up.
        for(let iter = 0; iter < 4; iter++)
        {
            for(let i = 1; i < layerIndices.length; i++)
                reorderByNeighborLayer(layerIndices[i], layerIndices[i - 1], true);
            for(let i = layerIndices.length - 2; i >= 0; i--)
                reorderByNeighborLayer(layerIndices[i], layerIndices[i + 1], false);
        }

        const yCenter = new Map();
        for(const layerIndex of layerIndices)
        {
            const layerNodes = layers.get(layerIndex);
            let y = 0;
            const rowGap = Math.max(main.grid_spacing || 24, 20);
            for(const n of layerNodes)
            {
                n.y = y;
                yCenter.set(n.localName, y + n.height / 2);
                y += n.height + rowGap;
            }
            const usedHeight = Math.max(0, y - rowGap);
            const offset = -usedHeight / 2;
            for(const n of layerNodes)
                n.y += offset;
        }

        const colGap = Math.max((main.grid_spacing || 24) * 5, 40);
        let cursorX = 0;
        for(const layerIndex of layerIndices)
        {
            const layerNodes = layers.get(layerIndex);
            const layerWidth = Math.max(...layerNodes.map((n) => n.width));
            for(const n of layerNodes)
                n.x = cursorX;
            cursorX += layerWidth + colGap;
        }

        const edges = [];
        for(const n of nodes)
            for(const t of outgoing.get(n.localName))
                if(nodeMap.has(t))
                    edges.push([n, nodeMap.get(t)]);

        const minGap = Math.max(main.grid_spacing || 24, 20);
        const pushMargin = Math.max(4, Math.round(minGap * 0.2));
        const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));

        // Third pass: vertical optimization to reduce edges crossing component boxes.
        for(let iter = 0; iter < 6; iter++)
        {
            const deltaY = new Map(nodes.map((n) => [n.localName, 0]));

            // 1) Pull toward average neighbor center to shorten edges.
            for(const n of nodes)
            {
                const neigh = [...incoming.get(n.localName), ...outgoing.get(n.localName)]
                    .map((k) => nodeMap.get(k))
                    .filter((x) => !!x);
                if(neigh.length === 0)
                    continue;
                const centerY = n.y + n.height / 2;
                const targetY = neigh.reduce((s, m) => s + (m.y + m.height / 2), 0) / neigh.length;
                deltaY.set(n.localName, deltaY.get(n.localName) + clamp((targetY - centerY) * 0.18, -minGap * 0.6, minGap * 0.6));
            }

            // 2) Repel nodes from connection segments that pass through their boxes.
            for(const [u, v] of edges)
            {
                const x1 = u.x + u.width / 2;
                const y1 = u.y + u.height / 2;
                const x2 = v.x + v.width / 2;
                const y2 = v.y + v.height / 2;
                if(Math.abs(x2 - x1) < 1e-6)
                    continue;

                const left = Math.min(x1, x2);
                const right = Math.max(x1, x2);
                for(const w of nodes)
                {
                    if(w === u || w === v)
                        continue;
                    const wx = w.x + w.width / 2;
                    if(wx <= left || wx >= right)
                        continue;
                    const t = (wx - x1) / (x2 - x1);
                    const yLine = y1 + t * (y2 - y1);
                    const top = w.y - pushMargin;
                    const bottom = w.y + w.height + pushMargin;
                    if(yLine < top || yLine > bottom)
                        continue;

                    const wCenter = w.y + w.height / 2;
                    const dist = Math.max(1, Math.abs(wCenter - yLine));
                    const dir = (wCenter >= yLine) ? 1 : -1;
                    const push = clamp((minGap * 1.1) / dist, 0.2, 1.2) * dir * (minGap * 0.55);
                    deltaY.set(w.localName, deltaY.get(w.localName) + push);
                }
            }

            // 3) Apply per-layer and enforce non-overlap with at least one-grid vertical spacing.
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length === 0)
                    continue;

                for(const n of layerNodes)
                    n.y += deltaY.get(n.localName);

                layerNodes.sort((a, b) => a.y - b.y);
                let currentY = layerNodes[0].y;
                layerNodes[0].y = currentY;
                for(let i = 1; i < layerNodes.length; i++)
                {
                    const prev = layerNodes[i - 1];
                    const n = layerNodes[i];
                    const minY = prev.y + prev.height + minGap;
                    if(n.y < minY)
                        n.y = minY;
                }

                const layerTop = Math.min(...layerNodes.map((n) => n.y));
                const layerBottom = Math.max(...layerNodes.map((n) => n.y + n.height));
                const offset = -((layerTop + layerBottom) / 2);
                for(const n of layerNodes)
                    n.y += offset;
            }
        }

        const edgeCrossesNode = function(u, v, w)
        {
            if(u === w || v === w)
                return false;
            const x1 = u.x + u.width / 2;
            const y1 = u.y + u.height / 2;
            const x2 = v.x + v.width / 2;
            const y2 = v.y + v.height / 2;

            const left = w.x - pushMargin;
            const right = w.x + w.width + pushMargin;
            const top = w.y - pushMargin;
            const bottom = w.y + w.height + pushMargin;

            const edgeMinX = Math.min(x1, x2);
            const edgeMaxX = Math.max(x1, x2);
            if(edgeMaxX < left || edgeMinX > right)
                return false;

            if(Math.abs(x2 - x1) < 1e-6)
            {
                if(x1 < left || x1 > right)
                    return false;
                const edgeMinY = Math.min(y1, y2);
                const edgeMaxY = Math.max(y1, y2);
                return !(edgeMaxY < top || edgeMinY > bottom);
            }

            const xa = Math.max(edgeMinX, left);
            const xb = Math.min(edgeMaxX, right);
            const ya = y1 + (xa - x1) * (y2 - y1) / (x2 - x1);
            const yb = y1 + (xb - x1) * (y2 - y1) / (x2 - x1);
            const segMinY = Math.min(ya, yb);
            const segMaxY = Math.max(ya, yb);
            return !(segMaxY < top || segMinY > bottom);
        };

        const snapY = function(v)
        {
            const g = main.grid_spacing || 1;
            return g * Math.round(v / g);
        };

        // Fourth pass: discrete local search minimizing "connection-over-component" events.
        for(let sweep = 0; sweep < 4; sweep++)
        {
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length <= 1)
                    continue;

                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 0; i < layerNodes.length; i++)
                {
                    const n = layerNodes[i];
                    const prev = i > 0 ? layerNodes[i - 1] : null;
                    const next = i < layerNodes.length - 1 ? layerNodes[i + 1] : null;
                    const minYAllowed = prev ? (prev.y + prev.height + minGap) : -Infinity;
                    const maxYAllowed = next ? (next.y - n.height - minGap) : Infinity;

                    const neighbors = [...incoming.get(n.localName), ...outgoing.get(n.localName)]
                        .map((k) => nodeMap.get(k))
                        .filter((x) => !!x);
                    const targetY = neighbors.length
                        ? neighbors.reduce((s, m) => s + (m.y + m.height / 2), 0) / neighbors.length - n.height / 2
                        : n.y;

                    const originalY = n.y;
                    let bestY = originalY;
                    let bestScore = Infinity;
                    for(let step = -8; step <= 8; step++)
                    {
                        let candidateY = snapY(originalY + step * minGap);
                        if(candidateY < minYAllowed)
                            candidateY = minYAllowed;
                        if(candidateY > maxYAllowed)
                            candidateY = maxYAllowed;
                        n.y = candidateY;

                        let blockCount = 0;
                        for(const [u, v] of edges)
                            if(edgeCrossesNode(u, v, n))
                                blockCount++;

                        const distancePenalty = Math.abs(candidateY - targetY) / (minGap || 1);
                        const score = blockCount * 100 + distancePenalty;
                        if(score < bestScore)
                        {
                            bestScore = score;
                            bestY = candidateY;
                        }
                    }
                    n.y = bestY;
                }

                // Enforce ordering/non-overlap after local search.
                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 1; i < layerNodes.length; i++)
                {
                    const prev = layerNodes[i - 1];
                    const n = layerNodes[i];
                    const minYAllowed = prev.y + prev.height + minGap;
                    if(n.y < minYAllowed)
                        n.y = minYAllowed;
                }

                // Keep each layer centered while allowing extra vertical expansion when needed.
                const layerTop = Math.min(...layerNodes.map((n) => n.y));
                const layerBottom = Math.max(...layerNodes.map((n) => n.y + n.height));
                const offset = -((layerTop + layerBottom) / 2);
                for(const n of layerNodes)
                    n.y += offset;
            }
        }

        const totalOverlapCount = function()
        {
            let count = 0;
            for(const [u, v] of edges)
                for(const w of nodes)
                    if(edgeCrossesNode(u, v, w))
                        count++;
            return count;
        };

        const totalConnectionLength = function()
        {
            let length = 0;
            for(const [u, v] of edges)
            {
                const x1 = u.x + u.width / 2;
                const y1 = u.y + u.height / 2;
                const x2 = v.x + v.width / 2;
                const y2 = v.y + v.height / 2;
                const dx = x2 - x1;
                const dy = y2 - y1;
                length += Math.sqrt(dx*dx + dy*dy);
            }
            return length;
        };

        // Final pass: greedy global minimization of edge-over-component overlaps.
        for(let pass = 0; pass < 3; pass++)
        {
            let improved = false;
            for(const layerIndex of layerIndices)
            {
                const layerNodes = layers.get(layerIndex);
                if(!layerNodes || layerNodes.length === 0)
                    continue;

                layerNodes.sort((a, b) => a.y - b.y);
                for(let i = 0; i < layerNodes.length; i++)
                {
                    const n = layerNodes[i];
                    const prev = i > 0 ? layerNodes[i - 1] : null;
                    const next = i < layerNodes.length - 1 ? layerNodes[i + 1] : null;
                    const minYAllowed = prev ? (prev.y + prev.height + minGap) : -Infinity;
                    const maxYAllowed = next ? (next.y - n.height - minGap) : Infinity;

                    const originalY = n.y;
                    let bestY = originalY;
                    let bestOverlap = totalOverlapCount();
                    let bestLength = totalConnectionLength();

                    const candidates = [];
                    for(let s = -6; s <= 6; s++)
                    {
                        if(s === 0)
                            continue;
                        candidates.push(snapY(originalY + s * minGap));
                    }

                    for(const candidateRaw of candidates)
                    {
                        const candidateY = clamp(candidateRaw, minYAllowed, maxYAllowed);
                        n.y = candidateY;
                        const overlap = totalOverlapCount();
                        if(overlap > bestOverlap)
                            continue;

                        const length = totalConnectionLength();
                        if(
                            overlap < bestOverlap ||
                            (overlap === bestOverlap && length < bestLength)
                        )
                        {
                            bestOverlap = overlap;
                            bestLength = length;
                            bestY = candidateY;
                        }
                    }

                    n.y = bestY;
                    if(bestY !== originalY)
                        improved = true;
                }
            }
            if(!improved)
                break;
        }

        const minX = Math.min(...nodes.map((n) => n.x));
        const minY = Math.min(...nodes.map((n) => n.y));
        const grid = main.grid_spacing || 24;
        const dx = (1 * grid) - minX;
        const dy = (2 * grid) - minY;
        const snap = function(v)
        {
            const g = main.grid_spacing || 1;
            return g * Math.round(v / g);
        };

        for(const n of nodes)
        {
            n.object._x = snap(n.x + dx);
            n.object._y = snap(n.y + dy);
        }

        const selected = nodes
            .filter((n) => n.object && n.object._tag !== "widget")
            .map((n) => n.fullName);
        network.rebuildDict();
        nav.populate();
        selector.selectItems(selected, background, false, false, true);
        main.addConnections();
    },
    
    newModule(moduleClass = "Module")
    {
        const name = network.uniqueID("Untitled_");
        const m =
        {
            name:name,
            class:moduleClass,
            color:"",
            log_level: 0,
            _tag:"module",
            _x:main.new_position_x,
            _y:main.new_position_y,
            inputs: [],
            outputs: [],
            parameters:[]
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].modules.push(m);
        network.dict[full_name]=m;
        if(moduleClass !== "Module" && network.classinfo && network.classinfo[moduleClass])
            network.changeModuleClass(full_name, moduleClass);
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        nav.populate();
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        return full_name;
    },

    newGroup() // FIXME: Move to network
    {
        const name = network.uniqueID("Group_");
        const m =
        {
            name:name,
            color:"",
            _tag:"group",
            _x:main.new_position_x,
            _y:main.new_position_y,
            inputs: [],
            outputs: [],
            parameters:[],
            modules: [],
            groups:[],
            widgets:[]
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].groups.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;
    
        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }

        nav.populate();
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
    },

    newInput() // FIXME: Move to network
    {
        const name = network.uniqueID("Input_");
        const m =
        {
            name:name,
            color:"",
            _tag:"input",
            _x:main.new_position_x,
            _y:main.new_position_y
        };
        let full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].inputs.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
    },

    newOutput() // FIXME: Move to network
    {
        const name = network.uniqueID("Output_");
        const m =
        {
            name:name,
            color:"",
            //size:"1",
            _tag:"output",
            _x:main.new_position_x,
            _y:main.new_position_y
        };
        const full_name = selector.selected_background+'.'+name;
        network.dict[selector.selected_background].outputs.push(m);
        network.dict[full_name]=m;
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
    },

    newWidget()
    {
        const name = network.uniqueID("Widget_");
        const defaultWidgetSize = main.grid_spacing * 8 + 1;
        const w = {
            "_tag": "widget",
            "name": name,
            "title": name,
            "class": "bar-graph",
            "show_frame": true,
            _x:main.new_position_x,
            _y:main.new_position_y,
            width: defaultWidgetSize,
            height: defaultWidgetSize
        };
        const full_name = selector.selected_background+'.'+name;

        network.dict[selector.selected_background].widgets.push(w);
        network.dict[full_name]=w;
        selector.selectItems([full_name]);
        main.beginRenameAfterSelection(full_name);
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
    },

    deleteGroup(g)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(g);
        group.groups = group.groups.filter(function(g) { return g.name !== name; });
        selector.selectItems([], null);
    },

    deleteModule(m)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(m);
        group.modules = group.modules.filter(function(m) { return m.name !== name; });
        selector.selectItems([], null);
    },

    deleteInput(i)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(i);
        group.inputs = group.inputs.filter(function(i) { return i.name !== name; });
        selector.selectItems([], null);
    },

    deleteOutput(o)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(o);
        group.outputs = group.outputs.filter(function(o) { return o.name !== name; });
        selector.selectItems([], null);
    },

    deleteWidget(w)
    {
        let group = network.dict[selector.selected_background];
        const name = nameInPath(w);
        group.widgets = group.widgets.filter(function(w) { return w.name !== name; });
        selector.selectItems([], null);
    },

    deleteConnection(c)
    {
        const connection = network.dict[c];
        const s_t = c.split('*');
        const source = selector.getLocalPath(s_t[0]);
        const target = selector.getLocalPath(s_t[1]);
        const group = network.dict[selector.selected_background];

        group.connections = group.connections.filter(con => !(selector.getLocalPath(con.source)==source && selector.getLocalPath(con.target)==target));
    },

    deleteComponent()
    {
        if(selector.selected_connection !=undefined)
        {
            this.deleteConnection(selector.selected_connection);
        }
        else
            for(let c of selector.selected_foreground)
            {
                switch(network.dict[c]._tag)
                {
                    case 'module':
                        this.deleteModule(c);
                        break;
                    case 'group':
                        this.deleteGroup(c);
                        break;
                    case 'input':
                        this.deleteInput(c);
                        break; 
                    case 'output':
                        this.deleteOutput(c);
                        break; 
                    case 'widget':
                        this.deleteWidget(c);
                        break; 
                }
            }
        network.rebuildDict();
        nav.populate();
        selector.selectItems([], selector.selected_background, false, false, true);
    },

    uniqueComponentName(baseName, reservedNames=new Set())
    {
        const background = selector.selected_background;
        let index = 1;
        let candidate = `${baseName}${index}`;
        while((`${background}.${candidate}` in network.dict) || reservedNames.has(candidate))
            candidate = `${baseName}${++index}`;
        return candidate;
    },

    remapConnectionEndpoint(endpoint, nameMap)
    {
        const beforeBracket = getStringUpToBracket(endpoint);
        const afterBracket = getStringAfterBracket(endpoint);
        const parts = beforeBracket.split('.');
        if(parts.length > 0 && parts[0] in nameMap)
            parts[0] = nameMap[parts[0]];
        return parts.join('.') + afterBracket;
    },

    duplicateSelectedComponents(include_external_connections=false)
    {
        if(!main.edit_mode)
            return;
        if(selector.selected_foreground.length === 0)
            return;

        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const selectedFullNames = [...selector.selected_foreground];
        const reservedNames = new Set();
        const nameMap = {};
        const duplicatedFullNames = [];

        for(const fullName of selectedFullNames)
        {
            const original = network.dict[fullName];
            if(!original || !original.name || !original._tag)
                continue;

            const oldName = original.name;
            const newName = main.uniqueComponentName(oldName, reservedNames);
            reservedNames.add(newName);
            nameMap[oldName] = newName;

            const clone = deepCopy(original);
            clone.name = newName;
            if(clone._x !== undefined)
                clone._x = parseInt(clone._x) + 30;
            if(clone._y !== undefined)
                clone._y = parseInt(clone._y) + 30;

            switch(clone._tag)
            {
                case 'group':
                    if(!group.groups) group.groups = [];
                    group.groups.push(clone);
                    break;
                case 'module':
                    if(!group.modules) group.modules = [];
                    group.modules.push(clone);
                    break;
                case 'input':
                    if(!group.inputs) group.inputs = [];
                    group.inputs.push(clone);
                    break;
                case 'output':
                    if(!group.outputs) group.outputs = [];
                    group.outputs.push(clone);
                    break;
                case 'widget':
                    if(!group.widgets) group.widgets = [];
                    group.widgets.push(clone);
                    break;
                default:
                    continue;
            }

            duplicatedFullNames.push(`${background}.${newName}`);
        }

        if(group.connections && Object.keys(nameMap).length > 0)
        {
            const duplicatedConnections = [];
            for(const c of group.connections)
            {
                const sourceComponent = getStringUpToBracket(c.source).split('.')[0];
                const targetComponent = getStringUpToBracket(c.target).split('.')[0];
                const sourceMapped = sourceComponent in nameMap;
                const targetMapped = targetComponent in nameMap;
                if(include_external_connections)
                {
                    if(!sourceMapped && !targetMapped)
                        continue;
                }
                else if(!sourceMapped || !targetMapped)
                    continue;

                const cc = deepCopy(c);
                cc.source = main.remapConnectionEndpoint(c.source, nameMap);
                cc.target = main.remapConnectionEndpoint(c.target, nameMap);
                duplicatedConnections.push(cc);
            }
            group.connections.push(...duplicatedConnections);
        }

        if(duplicatedFullNames.length === 0)
            return;

        network.rebuildDict();
        nav.populate();
        selector.selectItems(duplicatedFullNames, background, false, false, true);
    },

    changeComponentPosition(c, dx,dy, snap_to_grid=true)
    {
        const e = document.getElementById(c);
        if(!e)
            return;

        const base = main.map[c] || [e.offsetLeft, e.offsetTop];
        let new_x = base[0] + dx;
        let new_y = base[1] + dy;

        if(new_x < 0)
                new_x = 0;

            if(new_y < 0)
                new_y = 0;

        if(main.edit_mode && snap_to_grid)
        {
            const g = main.grid_spacing;
            new_x = g*Math.round(new_x/g);
            new_y = g*Math.round(new_y/g);
        }

        e.style.left = new_x+"px";
        e.style.top = new_y+"px";

        network.dict[c]._x = new_x;
        network.dict[c]._y = new_y;
    },

    changeComponentSize(dX, dY)
    {
        const w_id = selector.selected_foreground[0];
        const w = document.getElementById(w_id);
        const newWidth = main.grid_spacing*Math.round((main.startX + dX)/main.grid_spacing)+1;
        const newHeight = main.grid_spacing*Math.round((main.startY + dY)/main.grid_spacing)+1;

        w.style.width = newWidth + 'px';
        w.style.height = newHeight + 'px';
        network.dict[w_id].width = newWidth;
        network.dict[w_id].height = newHeight;

        w.widget.parameterChangeNotification(network.dict[w_id]);
    },

    moveComponents(evt)
    {
        if(Object.keys(main.map).length == 0) // FIME: SHOULD NOT BE NEEDED
            return;

        evt.stopPropagation()
        const dx = evt.clientX - main.initialMouseX;
        const dy = evt.clientY - main.initialMouseY;

        for(let c of selector.selected_foreground)
            main.changeComponentPosition(c, dx,dy);
        main.addConnections();
    },

    releaseComponents(evt)
    {
        const main_view = main.view;
        main_view.removeEventListener('mousemove',main.moveComponents, true);
        main_view.removeEventListener('mouseup',main.releaseComponents, true);
        main_view.removeEventListener('mousemove',main.moveComponents, false);
        main_view.removeEventListener('mouseup',main.releaseComponents, false);
        for(const c of selector.selected_foreground)
        {
            const e = document.getElementById(c);
            if(e)
                e.classList.remove("dragged");
        }
        main.map = {};
    },

    releaseResizeComponent(evt)
    {
        main_view.removeEventListener('mousemove',main.resizeComponent, true);
        main_view.removeEventListener('mouseup',main.releaseResizeComponent, true);
        main_view.removeEventListener('mousemove',main.resizeComponent, false);
        main.view.removeEventListener('mouseup',main.releaseResizeComponent, false);
        main.map = {}; // just in case

        const dX = evt.clientX - main.initialMouseX;
        const dY = evt.clientY - main.initialMouseY;
        main.changeComponentSize(dX,dY);

        const w_id = selector.selected_foreground[0];
        const w = document.getElementById(w_id);
        const newWidth = main.grid_spacing*Math.round((main.startX + dX)/main.grid_spacing)+1;
        const newHeight = main.grid_spacing*Math.round((main.startY + dY)/main.grid_spacing)+1;

        w.style.width = newWidth + 'px';
        w.style.height = newHeight + 'px';
        network.dict[w_id].width = newWidth;
        network.dict[w_id].height = newHeight;
        w.classList.remove("resized");
    },

    startResize(evt)
    {
        evt.stopPropagation();
        main.startX = this.offsetLeft;
        main.startY = this.offsetTop;
        selector.selectItems([this.parentElement.id], null);
        this.parentElement.classList.add("resized");

        main.initialMouseX = evt.clientX;
        main.initialMouseY = evt.clientY;
        main.view.addEventListener('mousemove',main.resizeComponent,true);
        main.view.addEventListener('mouseup',main.releaseResizeComponent,true);
        return false;
    },

    resizeComponent(evt) {
        const dX = evt.clientX - main.initialMouseX;
        const dY = evt.clientY - main.initialMouseY;
        main.changeComponentSize(dX,dY);
        return false;
    },

    startDragComponents(evt)
    {
        console.log("startDragComponents");
        evt.stopPropagation();
        if(evt.detail == 2) // handle double clicks elsewhere
        {
            const titleText = main.getEditableTitleTarget(evt.target);
            if(titleText)
            {
                const fullName = titleText.dataset.component || (this.dataset ? this.dataset.name : "");
                if(fullName)
                    setTimeout(function() { main.startInlineNameEditForComponent(fullName); }, 0);
            }
            evt.stopPropagation();
            return;
        }

       // this.style.pointerEvents = "none";

        main.initialMouseX = evt.clientX;
        main.initialMouseY = evt.clientY;

        const clickedName = this.dataset.name;
        const isAlreadySelected = selector.selected_foreground.includes(clickedName);
        if(evt.shiftKey || !isAlreadySelected)
            selector.selectItems([clickedName], null, evt.shiftKey);

        if(!selector.selected_foreground.includes(clickedName))
            return;

        if(!main.edit_mode)
            return;

        main.map = {};
        for(let c of selector.selected_foreground)
        {
            const e = document.getElementById(c);
            main.map[c] = [e.offsetLeft, e.offsetTop];
        }

        main.view.addEventListener('mousemove',main.moveComponents, true);
        main.view.addEventListener('mouseup',main.releaseComponents,true);

        return false;
    },

    startTrackConnection(evt)
    {
        if(!main.edit_mode)
            return;
        this.style.backgroundColor="orange";
        evt.stopPropagation();

        const id = this.id;
        const e = document.getElementById(id);
        const viewRect = main.view.getBoundingClientRect();
        const elementRect = e.getBoundingClientRect();
        const x = elementRect.left-viewRect.left+4.5;
        const y = elementRect.top-viewRect.top+4.5;

        main.tracked_connection =  
        { 
            x1: x, 
            y1: y, 
            x2: x, 
            y2: y, 
            source: id.split(':')[0], 
            target: null, 
            source_element: e,
            target_element: null
        };
        document.addEventListener('mousemove',main.moveTrackedConnection, true);
        document.addEventListener('mouseup',main.releaseTrackedConnection,true);
    },

    clearTrackedConnectionHighlights(tracked)
    {
        if(!tracked)
            return;

        if(tracked.source_element)
            tracked.source_element.style.backgroundColor = "";

        if(tracked.target_element)
            tracked.target_element.style.backgroundColor = "";

        if(tracked.target)
        {
            const targetElement = document.getElementById(tracked.target);
            if(targetElement)
                targetElement.style.backgroundColor = "";
        }
    },

    moveTrackedConnection(evt)
    {
        if(!main.tracked_connection)
            return;

        const viewRect = main.view.getBoundingClientRect();
        const ox = viewRect.left;
        const oy = viewRect.top;
        main.tracked_connection.x2 = evt.clientX - ox;
        main.tracked_connection.y2 = evt.clientY - oy;
        main.addConnections();
    },

    getConnectionTargetIdFromElement(element)
    {
        if(!element)
            return null;

        if(element.matches && element.matches(".i_spot") && element.id)
            return element.id;

        if(element.matches && element.matches(".widget"))
        {
            if(element.id)
                return element.id;
            const frame = element.closest(".gi.widget");
            if(frame && frame.id)
                return frame.id;
        }

        const socket = element.closest ? element.closest(".i_spot") : null;
        if(socket && socket.id)
            return socket.id;

        const widgetFrame = element.closest ? element.closest(".gi.widget") : null;
        if(widgetFrame && widgetFrame.id)
            return widgetFrame.id;

        return null;
    },

    getEditableTitleTarget(target)
    {
        let element = target || null;
        if(element && element.nodeType === Node.TEXT_NODE)
            element = element.parentElement;
        if(!element || !element.closest)
            return null;
        return element.closest(".component-title-text");
    },

    releaseTrackedConnection(evt) 
    {
        evt.stopPropagation();
    
        if (!main.tracked_connection)
            return;

        const tracked = main.tracked_connection;
        const { source } = tracked;
        let target = null;

        const connectionsLayer = document.getElementById("connections");
        let previousPointerEvents = null;
        if(connectionsLayer)
        {
            previousPointerEvents = connectionsLayer.style.pointerEvents;
            connectionsLayer.style.pointerEvents = "none";
        }

        if(document.elementsFromPoint)
        {
            const stack = document.elementsFromPoint(evt.clientX, evt.clientY) || [];
            for(const el of stack)
            {
                const resolved = main.getConnectionTargetIdFromElement(el);
                if(resolved)
                {
                    target = resolved;
                    break;
                }
            }
        }
        else
        {
            const hovered = document.elementFromPoint(evt.clientX, evt.clientY);
            target = main.getConnectionTargetIdFromElement(hovered);
        }

        if(connectionsLayer)
            connectionsLayer.style.pointerEvents = previousPointerEvents;

        if(!target)
            target = tracked.target || null;
    
        if (!target) 
        {
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            main.addConnections();
            document.removeEventListener('mousemove',main.moveTrackedConnection, true);
            document.removeEventListener('mouseup',main.releaseTrackedConnection,true);
            return;
        }
    
        const isTargetWidget = target in network.dict && network.dict[target]._tag === 'widget';
    
        if (isTargetWidget) 
        {
            const cleanSource = removeStringFromStart(source.split(':')[0], selector.selected_background + ".");
            const currentTitle = typeof network.dict[target].title === "string" ? network.dict[target].title : "";
            const currentSource = typeof network.dict[target].source === "string" ? network.dict[target].source : "";
            const shouldUpdateTitle = currentTitle.startsWith("Widget") || (currentTitle !== "" && currentTitle === currentSource);
            network.dict[target].source = cleanSource;
            if(shouldUpdateTitle)
                network.dict[target].title = cleanSource;

            const widgetFrame = document.getElementById(target);
            if(widgetFrame && widgetFrame.widget)
            {
                widgetFrame.widget.parameters.source = cleanSource;
                if(shouldUpdateTitle)
                    widgetFrame.widget.parameters.title = cleanSource;
                try
                {
                    widgetFrame.widget.updateAll();
                }
                catch(err)
                {
                    console.log(err);
                }
            }
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            selector.selectItems([target], null);
        } 
        else 
        {
            const cleanSource = removeStringFromStart(source.split(':')[0], selector.selected_background + ".");
            const cleanTarget = removeStringFromStart(target.split(':')[0], selector.selected_background + ".");
            network.newConnection(selector.selected_background, cleanSource, cleanTarget);
            main.clearTrackedConnectionHighlights(tracked);
            main.tracked_connection = null;
            selector.selectConnection(`${selector.selected_background}.${cleanSource}*${selector.selected_background}.${cleanTarget}`);
        }

        document.removeEventListener('mousemove',main.moveTrackedConnection, true);
        document.removeEventListener('mouseup',main.releaseTrackedConnection,true);
        main.addConnections();
    },

    
    setConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;

        if(main.tracked_connection.target_element && main.tracked_connection.target_element !== this)
            main.tracked_connection.target_element.style.backgroundColor = "";

        this.style.backgroundColor="orange";
        const targetId = main.getConnectionTargetIdFromElement(this);
        main.tracked_connection.target_element = this;
        if(targetId)
            main.tracked_connection.target = targetId;
    },

    resetConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;
        main.tracked_connection.target = null;
        if(main.tracked_connection.target_element === this)
            main.tracked_connection.target_element = null;
        this.style.backgroundColor="";
    },

    getComponentColorPalette(component)
    {
        if(!component || typeof component.color !== "string")
            return null;

        const colorName = component.color.trim().toLowerCase();
        if(colorName === "")
            return null;
        if(colorName === "black")
            return null;

        const palettes = {
            red:    { bg:"#5e1a1a", titleBg:"#7a2020", rowBg:"#9a2b2b", classBg:"#8a2525", separator:"#a74747", titleFg:"#ffecec", rowFg:"#ffdede" },
            orange: { bg:"#8b4a1f", titleBg:"#a85a24", rowBg:"#cf7d3f", classBg:"#b8672d", separator:"#c98958", titleFg:"#fff2e8", rowFg:"#ffe6d2" },
            yellow: { bg:"#aa9445", titleBg:"#9b7f32", rowBg:"#fff79a", classBg:"#b28c39", separator:"#d8d08b", border:"#333333", titleFg:"#fff7d6", rowFg:"#2b2b2b" },
            green:  { bg:"#234426", titleBg:"#2d5a31", rowBg:"#397241", classBg:"#2a522e", separator:"#4f8d58", titleFg:"#e6f5e7", rowFg:"#d5ecd7" },
            blue:   { bg:"#233a5b", titleBg:"#2c4b73", rowBg:"#375f8f", classBg:"#294864", separator:"#4a78a6", titleFg:"#e8f0ff", rowFg:"#dbe8ff" },
            purple: { bg:"#3f2a55", titleBg:"#51356d", rowBg:"#66458a", classBg:"#4a3164", separator:"#7d58a6", titleFg:"#f1e9ff", rowFg:"#e5d8fb" },
            pink:   { bg:"#8c4f71", titleBg:"#a65f86", rowBg:"#e6a8ca", classBg:"#cb7faa", separator:"#b27398", titleFg:"#fff4fa", rowFg:"#ffe8f3" },
            white:  { bg:"#d8d8d8", titleBg:"#ebebeb", rowBg:"#f4f4f4", classBg:"#dfdfdf", separator:"#c3c3c3", border:"#333333", titleFg:"#222222", rowFg:"#333333" }
        };
        return palettes[colorName] || null;
    },

    getComponentStyleVars(component)
    {
        const p = main.getComponentColorPalette(component);
        if(!p)
            return "";
        const borderVar = p.border ? `--component-border:${p.border};` : "";
        return `--component-bg:${p.bg};--component-title-bg:${p.titleBg};--component-row-bg:${p.rowBg};--component-class-bg:${p.classBg};--component-separator:${p.separator};--component-title-fg:${p.titleFg};--component-row-fg:${p.rowFg};${borderVar}`;
    },

    getPositionedComponentStyle(component)
    {
        const vars = main.getComponentStyleVars(component);
        return `top:${component._y}px;left:${component._x}px;${vars}`;
    },

    applyComponentColorToElement(element, component)
    {
        if(!element)
            return;
        const p = main.getComponentColorPalette(component);
        const keys = [
            "--component-bg",
            "--component-title-bg",
            "--component-row-bg",
            "--component-class-bg",
            "--component-separator",
            "--component-border",
            "--component-title-fg",
            "--component-row-fg"
        ];
        if(!p)
        {
            for(const k of keys)
                element.style.removeProperty(k);
            return;
        }
        element.style.setProperty("--component-bg", p.bg);
        element.style.setProperty("--component-title-bg", p.titleBg);
        element.style.setProperty("--component-row-bg", p.rowBg);
        element.style.setProperty("--component-class-bg", p.classBg);
        element.style.setProperty("--component-separator", p.separator);
        if(p.border)
            element.style.setProperty("--component-border", p.border);
        else
            element.style.removeProperty("--component-border");
        element.style.setProperty("--component-title-fg", p.titleFg);
        element.style.setProperty("--component-row-fg", p.rowFg);
    },

    getConnectionColorValue(connection)
    {
        if(!connection || typeof connection.color !== "string")
            return "";
        const c = connection.color.trim().toLowerCase();
        if(c === "" || c === "black")
            return "";
        const map = {
            red: "#c43a3a",
            orange: "#d57c2f",
            yellow: "#b7a22d",
            green: "#3f8c49",
            blue: "#3f69b7",
            purple: "#7551ad",
            pink: "#c06aa0",
            white: "#efefef"
        };
        return map[c] || connection.color;
    },

    buildRoundedOrthogonalPath(points, radius=10)
    {
        if(!points || points.length < 2)
            return "";
        if(points.length === 2)
            return `M ${points[0].x} ${points[0].y} L ${points[1].x} ${points[1].y}`;

        const rBase = Math.max(1, radius);
        let d = `M ${points[0].x} ${points[0].y}`;
        for(let i = 1; i < points.length - 1; i++)
        {
            const prev = points[i - 1];
            const curr = points[i];
            const next = points[i + 1];

            const v1x = curr.x - prev.x;
            const v1y = curr.y - prev.y;
            const v2x = next.x - curr.x;
            const v2y = next.y - curr.y;

            const len1 = Math.abs(v1x) + Math.abs(v1y);
            const len2 = Math.abs(v2x) + Math.abs(v2y);
            if(len1 < 1 || len2 < 1)
                continue;

            const dir1x = v1x === 0 ? 0 : (v1x > 0 ? 1 : -1);
            const dir1y = v1y === 0 ? 0 : (v1y > 0 ? 1 : -1);
            const dir2x = v2x === 0 ? 0 : (v2x > 0 ? 1 : -1);
            const dir2y = v2y === 0 ? 0 : (v2y > 0 ? 1 : -1);

            const r = Math.min(rBase, Math.floor(len1 * 0.5), Math.floor(len2 * 0.5));
            if(r < 1)
            {
                d += ` L ${curr.x} ${curr.y}`;
                continue;
            }

            const inX = curr.x - dir1x * r;
            const inY = curr.y - dir1y * r;
            const outX = curr.x + dir2x * r;
            const outY = curr.y + dir2y * r;

            d += ` L ${inX} ${inY}`;
            d += ` Q ${curr.x} ${curr.y}, ${outX} ${outY}`;
        }

        const last = points[points.length - 1];
        d += ` L ${last.x} ${last.y}`;
        return d;
    },

    shouldHighlightAutoRoutedConnection(group, connection)
    {
        return !!(group && group.auto_routing && connection && connection.line_type === "auto_route");
    },

    addGroup(g,path)
        {
        const fullName = `${path}.${g.name}`;
        let s = "";
        s += `<div class='gi module group' style='${main.getPositionedComponentStyle(g)}'  id='${fullName}' data-name='${fullName}'>`;
        s += `<table>`;
        s += `<tr><td class='title' colspan='3'><span class='component-title-text' data-component='${fullName}'>${g.name}</span></td></tr>`;

        for(let i of g.inputs || [])
            s += `<tr><td class='input'><div class='i_spot' id='${path}.${g.name}.${i.name}:in' onclick='alert(this.id)'></div></td><td>${i.name}</td><td></td></tr>`;

        for(let o of g.outputs || [])
            s += `<tr><td></td><td>${o.name}</td><td class='output'><div  class='o_spot' id='${path}.${g.name}.${o.name}:out'></div></td></tr>`;

        s += `</table>`;
        s += `</div>`;
        main.view.innerHTML += s;
   },

    addInput(i,path)
    {
        main.view.innerHTML += `<div class='gi group_input' id='${path}.${i.name}' data-name='${path}.${i.name}' style='${main.getPositionedComponentStyle(i)}'>
        <span class='component-title-text' data-component='${path}.${i.name}'>${i.name}</span>
        <div class='o_spot'  id='${path}.${i.name}:out'></div>
        </div>`;
    },

    addOutput(o,path)
    {
        main.view.innerHTML += `<div class='gi group_output'  id='${path}.${o.name}' data-name='${path}.${o.name}'  style='${main.getPositionedComponentStyle(o)}'><div class='i_spot' id='${path}.${o.name}:in'></div><span class='component-title-text' data-component='${path}.${o.name}'>${o.name}</span></div>`;
    },

    addModule(m,path)
    {
         let s = "";
         s += `<div class='gi module' style='${main.getPositionedComponentStyle(m)}'   id='${path}.${m.name}' data-name='${path}.${m.name}'>`;
         s += `<table>`;
         s += `<tr><td class='title' colspan='3'><span class='component-title-text' data-component='${path}.${m.name}'>${m.name}</span><button type='button' class='module-title-menu-button' aria-label='Module menu' title='Module menu' data-component='${path}.${m.name}'>&#9776;</button></td></tr>`;

             s += `<tr><td  colspan='3' class='class_line'>${m.class}<button type='button' class='module-class-menu-button' aria-label='Class menu' title='Class menu' data-component='${path}.${m.name}'>&#9776;</button></td></tr>`;
  
        for(let i of m.inputs || [])
            s += `<tr><td class='input'><div class='i_spot' id='${path}.${m.name}.${i.name}:in' onclick='alert(this.id)'></div></td ><td>${i.name}</td><td class='output'></td></tr>`;
  
        for(let o of m.outputs || [])
            s += `<tr><td class='output'></td><td>${o.name}</td><td class='output'><div class='o_spot' id='${path}.${m.name}.${o.name}:out'></div></td></tr>`;

         s += `</table>`;
         s += `</div>`;
        main.view.innerHTML += s;
    },

    addWidget(w, path) {
        const fullName = `${path}.${w.name}`;
        const newObject = document.createElement("div");
        newObject.setAttribute("class", "frame visible gi widget");
    
        const newTitle = document.createElement("div");
        newTitle.setAttribute("class", "title");
        newTitle.setAttribute("data-component", fullName);
        newTitle.innerHTML = `<span class="component-title-text" data-component="${fullName}" data-field="title">${w.title || w.name}</span>`;
        const titleText = newTitle.querySelector(".component-title-text");
        const startWidgetTitleEdit = function(evt)
        {
            evt.preventDefault();
            evt.stopPropagation();
            setTimeout(function() { main.startInlineNameEditForComponent(fullName); }, 0);
        };
        newTitle.addEventListener("dblclick", startWidgetTitleEdit, false);
        if(titleText)
            titleText.addEventListener("dblclick", startWidgetTitleEdit, false);
        newObject.appendChild(newTitle);

        const menuButton = document.createElement("button");
        menuButton.type = "button";
        menuButton.className = "widget-title-menu-button";
        menuButton.setAttribute("aria-label", "Widget menu");
        menuButton.setAttribute("title", "Widget menu");
        menuButton.setAttribute("data-component", fullName);
        menuButton.innerHTML = "&#9776;";
        newObject.appendChild(menuButton);
    
        const index = main.view.querySelectorAll(".widget").length;
        main.view.appendChild(newObject);
        //newObject.addEventListener('mousedown', main.startDragComponents, false);
    
        const widgetClass = `webui-widget-${w.class}`;
        let constr = webui_widgets.constructors[widgetClass];
    
        if (!constr) {
            console.log(`Internal Error: No constructor found for ${widgetClass}`);
            newObject.widget = new webui_widgets.constructors['webui-widget-text'];
            newObject.widget.element = newObject;
            newObject.widget.parameters['text'] = `"${widgetClass}" not found. Is it included in index.html?`;
            newObject.widget.parameters['_index_'] = index;
        } else {
            newObject.widget = new webui_widgets.constructors[widgetClass];
            for (let k in newObject.widget.parameters) {
                if (w[k] === undefined) {
                    w[k] = newObject.widget.parameters[k];
                } else {
                    let tp = newObject.widget.param_types[k];
                    w[k] = setType(w[k], tp);
                }
            }
            if(w.show_frame === undefined)
                w.show_frame = true;
            newObject.widget.parameters = w;
            newObject.widget.parameters['_index_'] = index;
        }
    
        newObject.widget.setAttribute('class', 'widget');
        newObject.appendChild(newObject.widget); 
    
        // Set style and position
        newObject.style.top = `${w._y}px`;
        newObject.style.left = `${w._x}px`;
        newObject.style.width = `${w.width || 200}px`;
        newObject.style.height = `${w.height || 200}px`;
    
        // Add handle for resizing
        const handle = document.createElement("div");
        handle.setAttribute("class", "handle");
        handle.onmousedown = main.startResize;
        newObject.appendChild(handle);
    
        // Set attributes for the new object
        newObject.setAttribute("id", fullName);
        newObject.setAttribute("data-name", fullName);
    
        try {
            newObject.widget.updateAll();
        } catch (err) {
            console.log(err);
        }
    },

    addConnection(c,path, routedPoints=null)
    {
        const source = getStringUpToBracket(c.source);
        const target = getStringUpToBracket(c.target);       
        const source_point = document.getElementById(`${path}.${source}:out`);
        const target_point = document.getElementById(`${path}.${target}:in`);

        if(source_point == undefined ||target_point == undefined)
            return;
        
        const ox = main.view.getBoundingClientRect().left;
        const oy = main.view.getBoundingClientRect().top;
        const x1 = source_point.getBoundingClientRect().left-ox+4.5;
        const y1 = source_point.getBoundingClientRect().top-oy+4.5;
        const x2 = target_point.getBoundingClientRect().left-ox+4.5;
        const y2 = target_point.getBoundingClientRect().top-oy+4.5;
        const connectionColor = main.getConnectionColorValue(c);
        const styleAttr = connectionColor ? ` style="--connection-color:${connectionColor};"` : "";
        const lineType = (c.line_type || "line").toLowerCase();
        let cc = "";
        if(Array.isArray(routedPoints) && routedPoints.length >= 2)
        {
            const d = main.buildRoundedOrthogonalPath(routedPoints, 10);
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}' onclick='selector.selectConnection("${path}.${source}*${path}.${target}")' ondblclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`;
        }
        else if(lineType === "orthogonal")
        {
            const mx = Math.round((x1 + x2) / 2);
            const points = `${x1},${y1} ${mx},${y1} ${mx},${y2} ${x2},${y2}`;
            cc = `<polyline points='${points}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}' onclick='selector.selectConnection("${path}.${source}*${path}.${target}")' ondblclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`;
        }
        else if(lineType === "orthagonal rounded" || lineType === "orthogonal rounded")
        {
            const mx = Math.round((x1 + x2) / 2);
            const points = [{x:x1,y:y1}, {x:mx,y:y1}, {x:mx,y:y2}, {x:x2,y:y2}];
            const d = main.buildRoundedOrthogonalPath(points, 10);
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}' onclick='selector.selectConnection("${path}.${source}*${path}.${target}")' ondblclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`;
        }
        else if(lineType === "spline")
        {
            const dx = Math.abs(x2 - x1);
            const bend = Math.max(24, Math.round(dx * 0.5));
            const c1x = x1 + bend;
            const c1y = y1;
            const c2x = x2 - bend;
            const c2y = y2;
            const d = `M ${x1} ${y1} C ${c1x} ${c1y}, ${c2x} ${c2y}, ${x2} ${y2}`;
            cc = `<path d='${d}' fill='none' class='connection_line'${styleAttr} data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}' onclick='selector.selectConnection("${path}.${source}*${path}.${target}")' ondblclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`;
        }
        else
            cc = `<line x1='${x1}' y1='${y1}' x2='${x2}' y2='${y2}' class='connection_line'${styleAttr} data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}' onclick='selector.selectConnection("${path}.${source}*${path}.${target}")' ondblclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`;
        main.connections += cc;
    },

    addTrackedConnection()
    {
        if(!main.tracked_connection)
            return;
        const con = main.tracked_connection;
        const x1 = con.x1;
        const y1 = con.y1;
        const x2 = con.x2;
        const y2 = con.y2;
        const cc = `<line x1='${x1}' y1='${y1}' x2='${x2}' y2='${y2}' class='connection_line tracked'/>`; 
        main.connections += cc;
    },

    collectComponentRectangles()
    {
        main.component_rectangles = [];
        if(!main.view)
            return main.component_rectangles;

        const viewRect = main.view.getBoundingClientRect();
        const margin = 15;
        for(const element of main.view.querySelectorAll(".gi"))
        {
            if(element.classList.contains("widget"))
                continue;
            const rect = element.getBoundingClientRect();
            const x = Math.round(rect.left - viewRect.left - margin);
            const y = Math.round(rect.top - viewRect.top - margin);
            const width = Math.round(rect.width + margin * 2);
            const height = Math.round(rect.height + margin * 2);
            main.component_rectangles.push({
                id: element.id || "",
                x,
                y,
                width,
                height
            });
        }

        return main.component_rectangles;
    },

    addDebugRectangles()
    {
        const rectangles = main.collectComponentRectangles();
        for(const rect of rectangles)
        {
            main.connections += `<rect class='component-debug-rectangle' x='${rect.x}' y='${rect.y}' width='${rect.width}' height='${rect.height}' data-component='${rect.id}'/>`;
        }
    },

    getOutputOwnerId(outputSpot)
    {
        if(!outputSpot || !outputSpot.id)
            return "";
        const outputId = outputSpot.id.replace(/:out$/, "");
        if(document.getElementById(outputId))
            return outputId;
        return parentPath(outputId);
    },

    collectOutputDebugLines()
    {
        main.output_debug_lines = [];
        if(!main.view)
            return main.output_debug_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const rectangles = main.component_rectangles || [];

        for(const outputSpot of main.view.querySelectorAll(".o_spot"))
        {
            const spotRect = outputSpot.getBoundingClientRect();
            const x1 = Math.round(spotRect.left - viewRect.left + spotRect.width * 0.5);
            const y = Math.round(spotRect.top - viewRect.top + spotRect.height * 0.5);
            const ownerId = main.getOutputOwnerId(outputSpot);
            let x2 = viewWidth;

            for(const rect of rectangles)
            {
                if(rect.id === ownerId)
                    continue;
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                if(rect.x <= x1)
                    continue;
                x2 = Math.min(x2, rect.x);
            }

            main.output_debug_lines.push({
                id: outputSpot.id,
                ownerId,
                x1,
                y1: y,
                x2,
                y2: y
            });
        }

        return main.output_debug_lines;
    },

    addOutputDebugLines()
    {
        const lines = main.collectOutputDebugLines();
        for(const line of lines)
        {
            main.connections += `<line class='output-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}' data-output='${line.id}' data-owner='${line.ownerId}'/>`;
        }
    },

    getInputOwnerId(inputSpot)
    {
        if(!inputSpot || !inputSpot.id)
            return "";
        const inputId = inputSpot.id.replace(/:in$/, "");
        if(document.getElementById(inputId))
            return inputId;
        return parentPath(inputId);
    },

    collectInputDebugLines()
    {
        main.input_debug_lines = [];
        if(!main.view)
            return main.input_debug_lines;

        const viewRect = main.view.getBoundingClientRect();
        const rectangles = main.component_rectangles || [];

        for(const inputSpot of main.view.querySelectorAll(".i_spot"))
        {
            const spotRect = inputSpot.getBoundingClientRect();
            const xEnd = Math.round(spotRect.left - viewRect.left + spotRect.width * 0.5);
            const y = Math.round(spotRect.top - viewRect.top + spotRect.height * 0.5);
            const ownerId = main.getInputOwnerId(inputSpot);
            let xStart = 0;

            for(const rect of rectangles)
            {
                if(rect.id === ownerId)
                    continue;
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                const rectRight = rect.x + rect.width;
                if(rectRight >= xEnd)
                    continue;
                xStart = Math.max(xStart, rectRight);
            }

            main.input_debug_lines.push({
                id: inputSpot.id,
                ownerId,
                x1: xStart,
                y1: y,
                x2: xEnd,
                y2: y
            });
        }

        return main.input_debug_lines;
    },

    addInputDebugLines()
    {
        const lines = main.collectInputDebugLines();
        for(const line of lines)
        {
            main.connections += `<line class='input-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}' data-input='${line.id}' data-owner='${line.ownerId}'/>`;
        }
    },

    getOutputRoutePitch()
    {
        return 22;
    },

    subtractBlockedIntervals(width, blocked)
    {
        let segments = [{x1: 0, x2: width}];
        for(const interval of blocked)
        {
            const nextSegments = [];
            for(const segment of segments)
            {
                if(interval.x2 <= segment.x1 || interval.x1 >= segment.x2)
                {
                    nextSegments.push(segment);
                    continue;
                }
                if(interval.x1 > segment.x1)
                    nextSegments.push({x1: segment.x1, x2: interval.x1});
                if(interval.x2 < segment.x2)
                    nextSegments.push({x1: interval.x2, x2: segment.x2});
            }
            segments = nextSegments;
            if(segments.length === 0)
                break;
        }
        return segments.filter((segment) => segment.x2 - segment.x1 > 1);
    },

    collectHorizontalRoutingLines()
    {
        main.horizontal_routing_lines = [];
        if(!main.view)
            return main.horizontal_routing_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const viewHeight = main.view.clientHeight || Math.round(viewRect.height) || 3000;
        const pitch = main.getOutputRoutePitch();
        const reservedLines = [
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || [])
        ].filter((line) =>
            line &&
            Number.isFinite(line.y1) &&
            Number.isFinite(line.x1) &&
            Number.isFinite(line.x2)
        );

        for(let y = Math.round(pitch); y < viewHeight; y += pitch)
        {
            const blocked = [];
            for(const rect of main.component_rectangles || [])
            {
                const top = rect.y;
                const bottom = rect.y + rect.height;
                if(y < top || y > bottom)
                    continue;
                blocked.push({x1: rect.x, x2: rect.x + rect.width});
            }
            for(const reservedLine of reservedLines)
            {
                if(Math.abs(y - reservedLine.y1) >= pitch)
                    continue;
                const x1 = Math.min(reservedLine.x1, reservedLine.x2);
                const x2 = Math.max(reservedLine.x1, reservedLine.x2);
                blocked.push({x1, x2});
            }
            blocked.sort((a, b) => a.x1 - b.x1);

            const segments = main.subtractBlockedIntervals(viewWidth, blocked);
            for(const segment of segments)
            {
                main.horizontal_routing_lines.push({
                    x1: Math.round(segment.x1),
                    y1: y,
                    x2: Math.round(segment.x2),
                    y2: y
                });
            }
        }

        return main.horizontal_routing_lines;
    },

    addHorizontalRoutingLines()
    {
        const lines = main.collectHorizontalRoutingLines();
        for(const line of lines)
        {
            main.connections += `<line class='routing-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}'/>`;
        }
    },

    collectVerticalRoutingLines()
    {
        main.vertical_routing_lines = [];
        if(!main.view)
            return main.vertical_routing_lines;

        const viewRect = main.view.getBoundingClientRect();
        const viewWidth = main.view.clientWidth || Math.round(viewRect.width) || 3000;
        const viewHeight = main.view.clientHeight || Math.round(viewRect.height) || 3000;
        const pitch = main.getOutputRoutePitch();
        const candidateXs = [];

        for(const rect of main.component_rectangles || [])
        {
            const left = rect.x;
            const right = rect.x + rect.width;
            for(let i = 0; i < 5; i++)
            {
                candidateXs.push(left - i * pitch, right + i * pitch);
            }
        }

        const uniqueXs = [];
        for(const rawX of candidateXs)
        {
            const x = Math.round(rawX);
            if(x < 0 || x > viewWidth)
                continue;
            if(uniqueXs.some((existingX) => Math.abs(existingX - x) < pitch))
                continue;
            uniqueXs.push(x);
        }

        for(const x of uniqueXs)
        {
            const blocked = [];
            for(const rect of main.component_rectangles || [])
            {
                const left = rect.x;
                const right = rect.x + rect.width;
                if(x < left || x > right)
                    continue;
                blocked.push({x1: rect.y, x2: rect.y + rect.height});
            }
            blocked.sort((a, b) => a.x1 - b.x1);

            const segments = main.subtractBlockedIntervals(viewHeight, blocked);
            for(const segment of segments)
            {
                main.vertical_routing_lines.push({
                    x1: x,
                    y1: Math.round(segment.x1),
                    x2: x,
                    y2: Math.round(segment.x2)
                });
            }
        }

        return main.vertical_routing_lines;
    },

    addVerticalRoutingLines()
    {
        const lines = main.collectVerticalRoutingLines();
        for(const line of lines)
        {
            main.connections += `<line class='vertical-routing-debug-line' x1='${line.x1}' y1='${line.y1}' x2='${line.x2}' y2='${line.y2}'/>`;
        }
    },

    collectRoutingGridPoints()
    {
        main.routing_grid_points = [];
        const horizontalLines = [
            ...(main.horizontal_routing_lines || []),
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || [])
        ];
        const verticalLines = main.vertical_routing_lines || [];
        const seen = new Set();

        for(const h of horizontalLines)
        {
            const hx1 = Math.min(h.x1, h.x2);
            const hx2 = Math.max(h.x1, h.x2);
            const hy = h.y1;
            for(const v of verticalLines)
            {
                const vy1 = Math.min(v.y1, v.y2);
                const vy2 = Math.max(v.y1, v.y2);
                const vx = v.x1;
                if(vx < hx1 || vx > hx2)
                    continue;
                if(hy < vy1 || hy > vy2)
                    continue;

                const key = `${Math.round(vx)}:${Math.round(hy)}`;
                if(seen.has(key))
                    continue;
                seen.add(key);
                main.routing_grid_points.push({
                    x: Math.round(vx),
                    y: Math.round(hy)
                });
            }
        }

        return main.routing_grid_points;
    },

    addRoutingGridPoints()
    {
        const points = main.collectRoutingGridPoints();
        for(const point of points)
        {
            main.connections += `<circle class='routing-grid-point' cx='${point.x}' cy='${point.y}' r='4'/>`;
        }
    },

    getRoutingNodeId(x, y)
    {
        return `${Math.round(x)},${Math.round(y)}`;
    },

    getConnectionSocketIds(path, connection)
    {
        const source = getStringUpToBracket(connection.source || "");
        const target = getStringUpToBracket(connection.target || "");
        return {
            sourceSocketId: `${path}.${source}:out`,
            targetSocketId: `${path}.${target}:in`
        };
    },

    getRoutingHorizontalLines()
    {
        return [
            ...(main.output_debug_lines || []),
            ...(main.input_debug_lines || []),
            ...(main.horizontal_routing_lines || [])
        ].map((line) => ({
            x1: Math.min(line.x1, line.x2),
            y1: line.y1,
            x2: Math.max(line.x1, line.x2),
            y2: line.y2,
            id: line.id || ""
        }));
    },

    getRoutingVerticalLines()
    {
        return (main.vertical_routing_lines || []).map((line) => ({
            x1: line.x1,
            y1: Math.min(line.y1, line.y2),
            x2: line.x2,
            y2: Math.max(line.y1, line.y2)
        }));
    },

    buildRoutingGraph()
    {
        const graph = {
            nodes: new Map(),
            edges: new Map(),
            adjacency: new Map()
        };
        const horizontals = main.getRoutingHorizontalLines();
        const verticals = main.getRoutingVerticalLines();

        const ensureNode = function(x, y)
        {
            const id = main.getRoutingNodeId(x, y);
            if(!graph.nodes.has(id))
                graph.nodes.set(id, {id, x: Math.round(x), y: Math.round(y)});
            if(!graph.adjacency.has(id))
                graph.adjacency.set(id, []);
            return id;
        };

        const addEdge = function(nodeA, nodeB, dir)
        {
            if(nodeA === nodeB)
                return;
            const key = nodeA < nodeB ? `${nodeA}|${nodeB}` : `${nodeB}|${nodeA}`;
            if(graph.edges.has(key))
                return;
            const a = graph.nodes.get(nodeA);
            const b = graph.nodes.get(nodeB);
            const length = dir === "H" ? Math.abs(a.x - b.x) : Math.abs(a.y - b.y);
            const edge = {key, a: nodeA, b: nodeB, dir, length};
            graph.edges.set(key, edge);
            graph.adjacency.get(nodeA).push(edge);
            graph.adjacency.get(nodeB).push(edge);
        };

        for(const h of horizontals)
        {
            const xs = [h.x1, h.x2];
            for(const other of horizontals)
            {
                if(other === h || Math.abs(other.y1 - h.y1) > 0.5)
                    continue;
                if(other.x1 > h.x2 || other.x2 < h.x1)
                    continue;
                xs.push(Math.max(h.x1, other.x1), Math.min(h.x2, other.x2), other.x1, other.x2);
            }
            for(const v of verticals)
            {
                if(v.x1 < h.x1 || v.x1 > h.x2)
                    continue;
                if(h.y1 < v.y1 || h.y1 > v.y2)
                    continue;
                xs.push(v.x1);
            }
            const uniqueXs = Array.from(new Set(xs.map((x) => Math.round(x)))).sort((a, b) => a - b);
            for(let i = 0; i < uniqueXs.length; i++)
                ensureNode(uniqueXs[i], h.y1);
            for(let i = 1; i < uniqueXs.length; i++)
                addEdge(main.getRoutingNodeId(uniqueXs[i - 1], h.y1), main.getRoutingNodeId(uniqueXs[i], h.y1), "H");
        }

        for(const v of verticals)
        {
            const ys = [v.y1, v.y2];
            for(const h of horizontals)
            {
                if(v.x1 < h.x1 || v.x1 > h.x2)
                    continue;
                if(h.y1 < v.y1 || h.y1 > v.y2)
                    continue;
                ys.push(h.y1);
            }
            const uniqueYs = Array.from(new Set(ys.map((y) => Math.round(y)))).sort((a, b) => a - b);
            for(let i = 0; i < uniqueYs.length; i++)
                ensureNode(v.x1, uniqueYs[i]);
            for(let i = 1; i < uniqueYs.length; i++)
                addEdge(main.getRoutingNodeId(v.x1, uniqueYs[i - 1]), main.getRoutingNodeId(v.x1, uniqueYs[i]), "V");
        }

        return graph;
    },

    compareRouteCost(a, b)
    {
        if(a.length !== b.length)
            return a.length - b.length;
        return a.segments - b.segments;
    },

    isRoutingEdgeAvailable(edgeKey, sourceSocketId, targetSocketId, occupiedEdges)
    {
        const occupants = occupiedEdges.get(edgeKey);
        if(!occupants || occupants.length === 0)
            return true;
        return occupants.every((occupant) => occupant.sourceSocketId === sourceSocketId || occupant.targetSocketId === targetSocketId);
    },

    findOrthogonalRoute(graph, sourceNodeId, targetNodeId, sourceSocketId, targetSocketId, occupiedEdges)
    {
        if(!graph.nodes.has(sourceNodeId) || !graph.nodes.has(targetNodeId))
            return null;

        const queue = [{
            nodeId: sourceNodeId,
            prevDir: null,
            length: 0,
            segments: 0,
            prevNodeId: null,
            prevState: null,
            viaEdgeKey: null
        }];
        const best = new Map();

        while(queue.length > 0)
        {
            let bestIndex = 0;
            for(let i = 1; i < queue.length; i++)
            {
                if(main.compareRouteCost(queue[i], queue[bestIndex]) < 0)
                    bestIndex = i;
            }
            const state = queue.splice(bestIndex, 1)[0];
            const bestKey = `${state.nodeId}|${state.prevDir || "-"}|${state.segments}`;
            const recorded = best.get(bestKey);
            if(recorded && main.compareRouteCost(recorded, state) < 0)
                continue;

            if(state.nodeId === targetNodeId && state.segments <= 5)
                return state;

            const currentNode = graph.nodes.get(state.nodeId);
            for(const edge of graph.adjacency.get(state.nodeId) || [])
            {
                if(!main.isRoutingEdgeAvailable(edge.key, sourceSocketId, targetSocketId, occupiedEdges))
                    continue;

                const nextNodeId = edge.a === state.nodeId ? edge.b : edge.a;
                if(state.prevNodeId && nextNodeId === state.prevNodeId)
                    continue;

                const nextNode = graph.nodes.get(nextNodeId);
                if(state.prevDir === null)
                {
                    if(edge.dir !== "H" || nextNode.x <= currentNode.x)
                        continue;
                }

                if(nextNodeId === targetNodeId)
                {
                    if(edge.dir !== "H" || currentNode.x >= nextNode.x)
                        continue;
                }

                const nextSegments = state.prevDir === null ? 1 : (state.prevDir === edge.dir ? state.segments : state.segments + 1);
                if(nextSegments > 5)
                    continue;

                const nextState = {
                    nodeId: nextNodeId,
                    prevDir: edge.dir,
                    length: state.length + edge.length,
                    segments: nextSegments,
                    prevNodeId: state.nodeId,
                    prevState: state,
                    viaEdgeKey: edge.key
                };
                const nextKey = `${nextState.nodeId}|${nextState.prevDir}|${nextState.segments}`;
                const bestNext = best.get(nextKey);
                if(bestNext && main.compareRouteCost(bestNext, nextState) <= 0)
                    continue;
                best.set(nextKey, nextState);
                queue.push(nextState);
            }
        }

        return null;
    },

    reconstructRoutePoints(graph, finalState)
    {
        const nodes = [];
        const edgeKeys = [];
        let state = finalState;
        while(state)
        {
            nodes.push(graph.nodes.get(state.nodeId));
            if(state.viaEdgeKey)
                edgeKeys.push(state.viaEdgeKey);
            state = state.prevState;
        }
        nodes.reverse();
        edgeKeys.reverse();

        const points = [];
        for(const node of nodes)
        {
            if(points.length < 2)
            {
                points.push({x: node.x, y: node.y});
                continue;
            }
            const prev = points[points.length - 1];
            const prevPrev = points[points.length - 2];
            const sameX = prevPrev.x === prev.x && prev.x === node.x;
            const sameY = prevPrev.y === prev.y && prev.y === node.y;
            if(sameX || sameY)
            {
                points[points.length - 1] = {x: node.x, y: node.y};
                continue;
            }
            points.push({x: node.x, y: node.y});
        }

        return {points, edgeKeys};
    },

    registerRouteOccupancy(edgeKeys, sourceSocketId, targetSocketId, occupiedEdges)
    {
        for(const edgeKey of edgeKeys)
        {
            if(!occupiedEdges.has(edgeKey))
                occupiedEdges.set(edgeKey, []);
            occupiedEdges.get(edgeKey).push({sourceSocketId, targetSocketId});
        }
    },

    buildAutoRoutesForGroup(group, path)
    {
        const routes = new Map();
        if(!group)
            return routes;

        const graph = main.buildRoutingGraph();
        const occupiedEdges = new Map();
        const sourceLineMap = new Map((main.output_debug_lines || []).map((line) => [line.id, line]));
        const targetLineMap = new Map((main.input_debug_lines || []).map((line) => [line.id, line]));

        const candidates = [];
        for(const connection of group.connections || [])
        {
            if(!main.shouldHighlightAutoRoutedConnection(group, connection))
                continue;
            const {sourceSocketId, targetSocketId} = main.getConnectionSocketIds(path, connection);
            const sourceLine = sourceLineMap.get(sourceSocketId);
            const targetLine = targetLineMap.get(targetSocketId);
            if(!sourceLine || !targetLine)
                continue;
            candidates.push({
                connection,
                sourceSocketId,
                targetSocketId,
                sourceLine,
                targetLine,
                estimatedLength: Math.abs(targetLine.x2 - sourceLine.x1) + Math.abs(targetLine.y1 - sourceLine.y1)
            });
        }

        candidates.sort((a, b) => a.estimatedLength - b.estimatedLength);

        for(const candidate of candidates)
        {
            const sourceNodeId = main.getRoutingNodeId(candidate.sourceLine.x1, candidate.sourceLine.y1);
            const targetNodeId = main.getRoutingNodeId(candidate.targetLine.x2, candidate.targetLine.y2);
            const finalState = main.findOrthogonalRoute(graph, sourceNodeId, targetNodeId, candidate.sourceSocketId, candidate.targetSocketId, occupiedEdges);
            if(!finalState)
                continue;

            const route = main.reconstructRoutePoints(graph, finalState);
            routes.set(main.getConnectionKey(path, candidate.connection), route.points);
            main.registerRouteOccupancy(route.edgeKeys, candidate.sourceSocketId, candidate.targetSocketId, occupiedEdges);
        }

        return routes;
    },

    getConnectionKey(path, connection)
    {
        const source = getStringUpToBracket(connection.source || "");
        const target = getStringUpToBracket(connection.target || "");
        return `${path}.${source}*${path}.${target}`;
    },

    commitInlineRename(fullName, newName)
    {
        const item = network.dict[fullName];
        if(!item || !newName)
            return;

        const trimmedName = newName.trim();
        if(!trimmedName || trimmedName === item.name)
            return;

        const group = selector.selected_background;
        const newFullName = `${group}.${trimmedName}`;
        if(newFullName !== fullName && network.dict[newFullName])
            return;

        item.name = trimmedName;
        if(item._tag == "input")
        {
            network.renameInput(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "output")
        {
            network.renameOutput(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "group" || item._tag == "module")
        {
            network.renameGroupOrModule(group, fullName, newFullName);
            selector.selectItems([newFullName], null);
        }
        else if(item._tag == "widget")
        {
            network.dict[newFullName] = item;
            delete network.dict[fullName];
            selector.selectItems([newFullName], null);
        }
        network.rebuildDict();
        nav.populate();
        network.tainted = true;
    },

    commitInlineTitleEdit(fullName, newTitle)
    {
        const item = network.dict[fullName];
        if(!item || item._tag !== "widget" || !newTitle)
            return;

        const trimmedTitle = newTitle.trim();
        if(!trimmedTitle || trimmedTitle === item.title)
            return;

        item.title = trimmedTitle;
        const widgetFrame = document.getElementById(fullName);
        if(widgetFrame && widgetFrame.widget)
        {
            widgetFrame.widget.parameters.title = trimmedTitle;
            try
            {
                widgetFrame.widget.updateAll();
            }
            catch(err)
            {
                console.log(err);
            }
        }
        if(inspector && typeof inspector.showInspectorForSelection === "function")
            inspector.showInspectorForSelection();
        network.tainted = true;
    },

    beginRenameAfterSelection(fullName)
    {
        if(!fullName)
            return;
        setTimeout(function()
        {
            main.startInlineNameEditForComponent(fullName);
        }, 0);
    },

    finishInlineNameEdit(commit)
    {
        const session = main.inline_name_edit;
        if(!session || session.finished)
            return;

        session.finished = true;
        const title = session.title;
        const componentElement = session.componentElement;
        title.contentEditable = "false";
        title.classList.remove("inline-name-edit");
        title.removeEventListener("keydown", session.onKeyDown, true);
        title.removeEventListener("blur", session.onBlur, true);

        const candidateName = title.textContent.replace(/\n/g, "").trim();
        if(commit && candidateName)
        {
            if(session.field === "title")
                main.commitInlineTitleEdit(session.fullName, candidateName);
            else
                main.commitInlineRename(session.fullName, candidateName);
        }
        else
            title.textContent = session.originalName;

        if(componentElement)
            componentElement.style.minWidth = "";
        title.removeAttribute("tabindex");

        main.inline_name_edit = null;
    },

    startInlineNameEdit(evt)
    {
        if(!main.edit_mode)
            return;

        const title = evt.currentTarget || evt.target.closest(".component-title-text");
        if(!title)
            return;
        const componentElement = title.closest(".gi");
        const fullName = title.dataset.component || (componentElement ? componentElement.dataset.name : "");
        const item = fullName ? network.dict[fullName] : null;
        const field = title.dataset.field || "name";
        if(!item || !["group", "module", "input", "output", "widget"].includes(item._tag))
            return;
        if(item._tag === "widget" && field !== "title")
            return;

        evt.preventDefault();
        evt.stopPropagation();

        if(main.inline_name_edit)
            main.finishInlineNameEdit(false);

        if(selector.selected_foreground[0] !== fullName || selector.selected_foreground.length !== 1)
            selector.selectItems([fullName], selector.selected_background);

        const originalName = field === "title" ? (item.title || title.textContent || "") : (item.name || title.textContent || "");
        if(componentElement)
            componentElement.style.minWidth = `${Math.ceil(componentElement.getBoundingClientRect().width)}px`;
        title.contentEditable = "true";
        title.setAttribute("contenteditable", "true");
        title.tabIndex = -1;
        title.spellcheck = false;
        title.classList.add("inline-name-edit");
        title.textContent = originalName;
        title.focus({preventScroll:true});
        document.getSelection()?.selectAllChildren(title);

        const onKeyDown = function(keyEvt)
        {
            if(keyEvt.key === "Enter")
            {
                keyEvt.preventDefault();
                main.finishInlineNameEdit(true);
                title.blur();
                return;
            }
            if(keyEvt.key === "Escape")
            {
                keyEvt.preventDefault();
                main.finishInlineNameEdit(false);
                title.blur();
            }
        };

        const onBlur = function()
        {
            main.finishInlineNameEdit(false);
        };

        main.inline_name_edit = {
            title,
            componentElement,
            fullName,
            field,
            originalName,
            onKeyDown,
            onBlur,
            finished: false
        };
        title.addEventListener("keydown", onKeyDown, true);
        title.addEventListener("blur", onBlur, true);
    },

    getInlineEditFieldForComponent(fullName)
    {
        const item = fullName ? network.dict[fullName] : null;
        return item && item._tag === "widget" ? "title" : "name";
    },

    getInlineEditTitleElement(fullName, field=null)
    {
        if(!fullName)
            return null;
        if(!main.view || typeof CSS === "undefined" || typeof CSS.escape !== "function")
            return null;
        const escapedName = CSS.escape(fullName);
        const resolvedField = field || main.getInlineEditFieldForComponent(fullName);
        if(resolvedField === "title")
            return main.view.querySelector(`.component-title-text[data-component="${escapedName}"][data-field="title"]`);
        return main.view.querySelector(`.component-title-text[data-component="${escapedName}"]:not([data-field]), .component-title-text[data-component="${escapedName}"][data-field="name"]`);
    },

    startInlineNameEditForComponent(fullName, field=null)
    {
        const title = main.getInlineEditTitleElement(fullName, field);
        if(!title)
            return;
        const syntheticEvent = {
            currentTarget: title,
            preventDefault() {},
            stopPropagation() {}
        };
        main.startInlineNameEdit(syntheticEvent);
    },

    selectConnection(connection)
    {
        const c = document.getElementById(connection);
        if(c != undefined) // FIXME: Use exception above instead
        {
            c.classList.add("selected");
            const st = connection.split('*');
            const s = document.getElementById(st[0]+":out");
            const t = document.getElementById(st[1]+":in");
            if(s)
                s.style.backgroundColor = "orange";
            if(t)
                t.style.backgroundColor = "orange";
        }
    },

    deselectConnection(connection)
    {
        const c = document.getElementById(connection);
        if(c != undefined) // FIXME: Use exception above instead
        {
            c.classList.remove("selected");

            const st = connection.split('*');
            const s = document.getElementById(st[0]+":out");
            const t = document.getElementById(st[1]+":in");
            if(s)
                s.style.backgroundColor = "rgb(177, 177, 177)";
            if(t)
                t.style.backgroundColor = "rgb(177, 177, 177)";
        }
    },

    addConnections()
    {
        const path = selector.selected_background;
        const group = network.dict[path];
        const s = document.getElementById("connections");
        if(!s || !group)
            return;

        main.connections = "<svg xmlns='http://www.w3.org/2000/svg' class='connections_svg'>";
        main.collectComponentRectangles();
        main.collectOutputDebugLines();
        main.collectInputDebugLines();
        main.collectHorizontalRoutingLines();
        main.collectVerticalRoutingLines();
        main.collectRoutingGridPoints();
        const autoRoutes = main.buildAutoRoutesForGroup(group, path);
        for(let c of group.connections || [])
            main.addConnection(c, path, autoRoutes.get(main.getConnectionKey(path, c)) || null);
        main.addTrackedConnection();
        main.connections += "</svg>";
        s.innerHTML = main.connections;
    },

    updateComponentStates()
    {
    // Add object handlers for all visible elements in main view depending on mode and tyep

    selectionList = selector.selected_foreground;

        if(main.edit_mode)
        {
            for (let e of main.view.querySelectorAll(".gi")) 
            {
                // Main drag function
                e.addEventListener('mousedown', main.startDragComponents, false);
            
                // Double click behaviors
                if (e.classList.contains("group")) 
                    e.ondblclick = function (evt) { selector.selectItems([], this.dataset.name); } // Jump into group
                else 
                    e.ondblclick = null;
     
                // Set selection class if selected
                if(selectionList.includes(e.dataset.name))
                    e.classList.add("selected")
                else
                    e.classList.remove("selected"); // FIXME: is this ever necessary?
            }

            // Add handlerer to outputs
            for(let o of main.view.querySelectorAll(".o_spot"))
                o.addEventListener('mousedown', main.startTrackConnection, false);
    
            // Add handlers to inputs

            for(let i of main.view.querySelectorAll(".i_spot"))
            {
                i.addEventListener('mouseover', main.setConnectectionTarget, true);
                i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
            }
    
            // Add handlers to widgets

            for(let i of main.view.querySelectorAll(".widget"))
            {
                i.addEventListener('mouseover', main.setConnectectionTarget, true);
                i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
            }

            for(const b of main.view.querySelectorAll(".module-title-menu-button"))
                b.addEventListener('mousedown', main.openComponentColorMenuFromButton, false);
            for(const b of main.view.querySelectorAll(".module-class-menu-button"))
                b.addEventListener('mousedown', main.openComponentClassMenuFromButton, false);
            for(const b of main.view.querySelectorAll(".widget-title-menu-button"))
                b.addEventListener('mousedown', main.openWidgetMenuFromButton, false);
            for(const t of main.view.querySelectorAll(".component-title-text"))
                t.addEventListener('dblclick', main.startInlineNameEdit, false);
        }
        else // View mode
        {
            for (let e of main.view.querySelectorAll(".gi"))
            {
                if(!e.classList.contains("widget"))
                {
                    // Main drag function
                    e.addEventListener('mousedown', main.startDragComponents, false);   // FIXME: Change to select only hete
                
                    // Double click behaviors
                    if (e.classList.contains("group")) 
                        e.ondblclick = function (evt) { selector.selectItems([], this.dataset.name); } // Jump into group
                    else 
                        e.ondblclick = null;

            // Add handlerer to outputs
            for(let o of main.view.querySelectorAll(".o_spot"))
                    o.addEventListener('dblclick', function (evt) 
                        {

                            window.open("http://localhost:8000/data/"+this.id.replace(/:out$/, ''), "_blank", "width=800,height=600");
                        }, false);

                    // Set selection class if selected
                }

                if(selectionList.includes(e.dataset.name))
                    e.classList.add("selected")
                else
                    e.classList.remove("selected"); // FIXME: is this ever necessary?
                }
            for(const t of main.view.querySelectorAll(".component-title-text"))
                t.addEventListener('dblclick', main.startInlineNameEdit, false);
        }
    },

    addComponents(group, selectionList, path)
    {
        if(group == undefined)
            return;

        main.view.innerHTML = "";

        for(let i of group.inputs || [])
            main.addInput(i,path);

        for(let o of group.outputs || [])
            main.addOutput(o,path);

        for(let g of group.groups || [])
            main.addGroup(g,path);

        for(let m of group.modules || [])
            main.addModule(m,path);

        for(let w of group.widgets || [])
            main.addWidget(w,path);

        main.addConnections();
        main.updateComponentStates();
    },

    cancelEditMode()
    {
        this.setViewMode();
    },

    setEditMode()
    {
        network.tainted = true;
        main.main.classList.add("edit_mode");
        main.main.classList.remove("view_mode");
        main.edit_mode = true;
        controller.run_mode = 'stop';
        controller.get("stop", controller.update);
        main.updateComponentStates();
        inspector.updateLibraryAddButtonState();
        inspector.showInspectorForSelection();
    },

    setViewMode()
    {
        main.main.classList.add("view_mode");
        main.main.classList.remove("edit_mode");
        main.edit_mode = false;
        inspector.updateLibraryAddButtonState();
        selector.selectItems([], selector.selected_background);
    },

    toggleEditMode()
    {
        if(main.main.classList.contains("edit_mode"))
            this.setViewMode();
        else
            this.setEditMode();
    },

    selectItem(foreground, background)
    {
        if(background != null)
        {
            let group = network.dict[background];
            main.addComponents(group, foreground, background);
        }
        main.updateAutoRoutingButtonState();
    },

    selectCurrentGroupComponents(options = {})
    {
        const background = selector.selected_background;
        const group = network.dict[background];
        if(!group)
            return;

        const includeWidgets = options.includeWidgets !== false;
        const widgetsOnly = options.widgetsOnly === true;
        let components = [];

        if(widgetsOnly)
            components = [...(group.widgets || [])];
        else
        {
            components = [
                ...(group.groups || []),
                ...(group.modules || []),
                ...(group.inputs || []),
                ...(group.outputs || [])
            ];
            if(includeWidgets)
                components.push(...(group.widgets || []));
        }

        selector.selectItems(components.map((component) => background + '.' + component.name), null, false, true);
    },

    keydown(evt)
    {
        const key = (evt.key || "").toLowerCase();
        const code = evt.code || "";
        const isModifier = evt.metaKey || evt.ctrlKey;
        const isSaveShortcut = isModifier && (key == "s" || code == "KeyS");

        const activeElement = document.activeElement;
        if(!isSaveShortcut && activeElement && (activeElement.tagName === "INPUT" || activeElement.tagName === "TEXTAREA" || activeElement.tagName === "SELECT" || activeElement.isContentEditable))
            return;

        if(main.edit_mode && selector.selected_foreground.length > 0)
        {
            const keyMoves = {
                ArrowLeft: [-1, 0],
                ArrowRight: [1, 0],
                ArrowUp: [0, -1],
                ArrowDown: [0, 1]
            };

            const move = keyMoves[evt.key];
            if(move)
            {
                evt.preventDefault();
                for(let c of selector.selected_foreground)
                    main.changeComponentPosition(c, move[0], move[1], false);
                main.addConnections();
                network.tainted = true;
                return;
            }
        }

        if(evt.key== "Escape")
        {
            inspector.toggleSystem();
            evt.preventDefault();
            return;
        }

        if(evt.key== "Backspace")
        {
            if(main.edit_mode && (selector.selected_connection != null || selector.selected_foreground.length > 0))
            {
                evt.preventDefault();
                main.deleteComponent();
                return;
            }
            return;
        }

        if(!isModifier)
            return;
        if(evt.keyIdentifier=="Meta")
            return;

        if(key=="a" || code=="KeyA")
        {
            evt.preventDefault();
            if(evt.altKey)
                main.selectCurrentGroupComponents({widgetsOnly: true});
            else if(evt.shiftKey)
                main.selectCurrentGroupComponents({includeWidgets: false});
            else
                main.selectCurrentGroupComponents({includeWidgets: true});
            return;
        }
  
  /*
  
        else if (evt.key=="c")
        {
            evt.preventDefault();
            alert("Copy selected items. (NOT IMPLEMENTED YET)");
        }
        else if (evt.key=="x")
        {
            evt.preventDefault();
            alert("Cut selected items. (NOT IMPLEMENTED YET)");
        }
        else if (evt.key=="v")
        {
            evt.preventDefault();
            alert("Paste copied items. (NOT IMPLEMENTED YET)");
        }
*/

        else if (key=="d" || code=="KeyD")
        {
            evt.preventDefault();
            if(main.edit_mode)
                main.duplicateSelectedComponents(evt.shiftKey);
            return;
        }
        else if (key=="e" || code=="KeyE")
        {
            evt.preventDefault();
            main.toggleEditMode();
            return;
        }
        else if (key=="i" || code=="KeyI")
        { 
            evt.preventDefault();
            inspector.toggleComponent();
            return;
        }
        else if (key=="s" || code=="KeyS")
        {
            evt.preventDefault();
            controller.save();
            return;
        }
        else if (evt.altKey && (key=="t" || code=="KeyT"))
        {
            evt.preventDefault();
            controller.realtime();
            return;
        }
        else if (evt.altKey && (key=="p" || code=="KeyP"))
        {
            evt.preventDefault();
            controller.play();
            return;
        }
        else if (evt.altKey && (key=="." || code=="Period"))
        {
            evt.preventDefault();
            controller.stop();
            return;
        }
        else if (evt.altKey && (key=="n" || code=="KeyN"))
        {
            evt.preventDefault();
            controller.new();
            return;
        }
        else if (evt.altKey && (key=="o" || code=="KeyO"))
        {
            evt.preventDefault();
            controller.open();
            return;
        }
    },

    showCommandInfo()
    {
        dialog.showInfoDialog(
`<table>
    <thead>
        <tr>
            <th>Command</th>
            <th>Effect</th>
        </tr>
    </thead>
    <tbody>
        <tr><td>Arrow keys</td><td>Move selected components by 1 px (edit mode).</td></tr>
        <tr><td>Backspace</td><td>Delete selected components/connections (edit mode).</td></tr>
        <tr><td>Escape</td><td>Toggle system inspector.</td></tr>
        <tr><td>&#8984; + A</td><td>Select all components/widgets in current group.</td></tr>
        <tr><td>&#8984; + &#8679; + A</td><td>Select all non-widget components in current group.</td></tr>
        <tr><td>&#8984; + &#8997; + A</td><td>Select widgets only in current group.</td></tr>
        <tr><td>&#8984; + D</td><td>Duplicate selected components (edit mode).</td></tr>
        <tr><td>&#8984; + &#8679; + D</td><td>Duplicate selected components (edit mode), preserving incoming/outgoing connections.</td></tr>
        <tr><td>&#8984; + E</td><td>Toggle edit mode.</td></tr>
        <tr><td>&#8984; + I</td><td>Toggle inspector.</td></tr>
        <tr><td>&#8984; + S</td><td>Save.</td></tr>
        <tr><td>&#8984; + &#8997; + T</td><td>Set runtime mode (same as Realtime button).</td></tr>
        <tr><td>&#8984; + &#8997; + P</td><td>Set play mode (same as Play button).</td></tr>
        <tr><td>&#8984; + &#8997; + .</td><td>Set stop mode (same as Stop button).</td></tr>
        <tr><td>&#8984; + &#8997; + N</td><td>New.</td></tr>
        <tr><td>&#8984; + &#8997; + O</td><td>Open.</td></tr>
    </tbody>
</table>`,
            "Keyboard Commands",
            true
        );
    }
}

const brainstudio = 
{
    keydownHandler(evt)
    {
        const key = (evt.key || "").toLowerCase();
        const code = evt.code || "";
        const isModifier = evt.metaKey || evt.ctrlKey;
        const infoDialog = document.getElementById("info_dialog");

        if(infoDialog && infoDialog.open && evt.key === "Escape")
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            dialog.closeInfo();
            return;
        }

        if(isModifier && evt.altKey && (key === "t" || code === "KeyT"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.realtime();
            return;
        }

        if(isModifier && evt.altKey && (key === "p" || code === "KeyP"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.play();
            return;
        }

        if(isModifier && evt.altKey && (key === "." || code === "Period"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.stop();
            return;
        }

        // Use Cmd/Ctrl+Option+O for Open to avoid browser-reserved Cmd+O.
        if(isModifier && evt.altKey && (key === "o" || code === "KeyO"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.open();
            return;
        }

        if(isModifier && evt.altKey && (key === "n" || code === "KeyN"))
        {
            if(evt.cancelable)
                evt.preventDefault();
            evt.stopPropagation();
            if(evt.stopImmediatePropagation)
                evt.stopImmediatePropagation();
            controller.new();
            return;
        }

        main.keydown(evt);
    },

    init()
    {
        log.init();
        inspector.init();
        controller.init();
        nav.init();
        breadcrumbs.init();
        main.init();

        window.addEventListener("keydown", brainstudio.keydownHandler, true);

    }
}
