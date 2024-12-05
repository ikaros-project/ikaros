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
    "epi-head"
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
    var split = this.split(sep || /\s+/);
    return maxsplit ? [ split.slice(0, -maxsplit).join(sep) ].concat(split.slice(-maxsplit)) : split;
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
            return 1000000*d + " &#181;s";
        else if(d < 5)
            return 1000*d + " ms";
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
    var date = new Date();
    date.setTime(date.getTime()+(days?days:1)*86400000);
    var expires = "; expires="+date.toGMTString();
    document.cookie = name+"="+value+expires+"; path=/";
}

function getCookie(name)
{
    var nameEQ = name + "=";
    const ca = document.cookie.split(';');
    for(let i=0;i < ca.length;i++) {
        const c = ca[i];
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
    setCookie('current_view', "");
}



let dialog = 
{
    window: null,
    callback: null,

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
                alert("ould not get file list from server.");
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
            alert("ould not get file list from server.");
            console.log("Could not get file list from server.");
        })
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
            let sel = document.getElementById(`open_dialog_${type}_items`);
            sel.innerHTML = '';  // Clear previous options
            files.forEach(file => {
                let opt = document.createElement('option');
                opt.value = file;
                opt.text = file;
                sel.appendChild(opt);
            });
        }
    },

    displayMessage(message)
    {
        if (message) {
            document.getElementById('open_dialog_title').innerText = message;
        }
    },

    showSystemFileList() 
    {
        system_file_button

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
        this.rebuildDict();
        this.component_count = Object.keys(this.dict).length+1;
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
            delay: "1"
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

            source: old_widget.source || "",
            min: old_widget.min || "",
            max: old_widget.max || "",
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
        var w = window.open("about:blank", "", "_blank");
        w.document.write('<html><body><pre>'+JSON.stringify(network.network,null,2)+ '</pre></body></html>');
    },

    debug_dict()
    {
        var w = window.open("about:blank", "", "_blank");
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

    reconnect()
    {
        controller.get("update", controller.update);
        controller.setSystemInfo(null);
        let s = document.querySelector("#state");
        if(s.innerText == "waiting")
            s.innerHTML = "waiting &bull;";
        else
            s.innerHTML = "waiting";

    },

    defer_reconnect()
    {
        clearInterval(controller.reconnect_timer);
        //controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
        // AUTOMATIC RECONNECT TEMPORARILY TURNED OFF - should only happen in run mode **********
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
        var last_request = url;
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
            log.print("Request timed out.");
            controller.open_mode = false;
            controller.requestUpdate();


        }

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
        controller.reconnect_timer = setInterval(controller.reconnect, controller.reconnect_interval);
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
        controller.queueCommand('new');
        network.tainted = true;
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
        const filename = prompt("Save as:");
        if (filename !== null)
        {
            network.network.filename = filename;
            controller.save();
        } 
        else
        {

        }
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
    
    realtime()// ********************* HERE
    {
        if(network.tainted) // Should never happen
            return;
            
        main.setViewMode(); // FIXME: Probably not necessary
        controller.queueCommand('realtime');
    },

    clear_wait()
    {
        controller.load_count = 0;
        console.log("clear_wait - drawing or data load failed"); // FIXME: Remove
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
                if(w[i].children[1].loadData)
                {
                    controller.load_count += w[i].children[1].loadData(data);
                }
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
                document.querySelector("#debug_row").style.display="block";
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
            console.log("controller.setSystemInfo: incorrect package received form ikaros.")
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

        // NETWORK

        if(package_type == "network")
        {
            controller.open_mode = false;
            controller.clearQueue();
            controller.session_id = session_id;
            document.querySelector("header").style.display="block"; // Show page when network is loaded
            document.querySelector("#load").style.display="none";
   
            controller.tick = response.tick;
            network.init(response);
            nav.populate();
            let top = network.network.name;
            selector.selectItems([], top);
            let v = getCookie('current_view');

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
            controller.setSystemInfo(response);
            controller.tick_duration = response.tick_duration || 0;
            if(response.has_data)
                controller.updateImages(response.data);
        }

        if(response.log)
        {
            let logElement = document.querySelector('.log');
            if(response.log.length > 0)    
                console.log(response.log);
            response.log.forEach((element) => {
                if(element[0] > 5)
                logElement.innerHTML += "<p class='message'>"+element[1]+"</p>\n";
                else if(element[0] == 5)
                    logElement.innerHTML += "<p class='warning'>"+element[1]+"</p>\n";
                else
                {
                    logElement.innerHTML += "<p class='error'>"+element[1]+"</p>\n";
                    log.showView();
                }
        });
 
            logElement.scrollTop = logElement.scrollHeight;
        }
    },

    requestUpdate()
    {
        clearTimeout(controller.request_timer);
        controller.request_timer = setTimeout(controller.requestUpdate, controller.webui_req_int); // immediately schdeule next

        if(controller.open_mode)
            return;

        if(selector.selected_background == null)
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
                w[i].children[1].requestData(data_set);
            }
            catch(err)
            {}

        group_path = selector.selected_background;
        data_string = ""; // should be added to names to support multiple clients
        let sep = "";
        for(s of data_set)
        {
            data_string += (sep + s);
            sep = ","
         }

         while(controller.commandQueue.length>0)
        {
            const cmd_dict = controller.commandQueue.shift();
            let s = cmd_dict[0]; // FIXME: check empty string
            const path = cmd_dict[1];
            const dict = cmd_dict[2];
            dict.data = data_string;
            dict.root = group_path;
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
                w[i].children[1].receivedData = data;
                w[i].children[1].update(data); // include data for backward compatibility
            }
            catch(err)
            {
                console.log("updateWidgets failed: "+controller.client_id);
            }
    }
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
    system: null,

    init()
    {
        inspector.system = document.querySelector('#system_inspector');
        inspector.component = document.querySelector('#component_inspector');

        inspector.subview.nothing  = document.querySelector('#inspector_nothing'); 
        inspector.subview.multiple  = document.querySelector('#inspector_multiple');
        inspector.subview.table  = document.querySelector('#inspector_table');
        inspector.subview.group_background = document.querySelector('#inspector_group_background');
        inspector.subview.group = document.querySelector('#inspector_group');
        inspector.subview.module =  document.querySelector('#inspector_module');
        inspector.subview.widget =  document.querySelector('#inspector_widget');

        inspector.hideSubviews();
        inspector.subview.nothing.style.display='block';
    },
    toggleSystem()
    {
        if (window.getComputedStyle(inspector.system, null).display === 'none')
        {
            inspector.system.style.display = 'block';
            inspector.component.style.display = 'none';
        }
        else
        {
            inspector.system.style.display = 'none';
            inspector.component.style.display = 'none';
        }
    },

    toggleComponent()
    {
        if (window.getComputedStyle(inspector.component, null).display === 'none')        
            {
            inspector.component.style.display = "block";
            inspector.system.style.display = "none";
        }
        else
        {
            inspector.system.style.display = "none";
            inspector.component.style.display = "none";
        }
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
        var s = '<select name="'+name+'" oninput="this">';
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

        cell1.innerText = p.name;
        cell2.innerHTML = value != undefined ? value : "";
        cell2.setAttribute('class', p.type);
        cell2.addEventListener("paste", function(e) 
        {
            e.preventDefault();
            const text = e.clipboardData.getData("text/plain");
            document.execCommand("insertHTML", false, text); // FIXME: uses deprecated functions
        });

        cell2.contentEditable = true;
        cell2.className += ' textedit';
        cell2.addEventListener("keypress", function(evt) 
        {
            if(evt.keyCode == 13)
            {
                evt.target.blur();
                evt.preventDefault();
                if(inspector.notify)
                    inspector.notify.parameterChangeNotification(item);
                return;
            }
        });

        cell2.addEventListener("blur", function(evt) 
        {
            if(inspector.checkValueForType(evt.target.innerText, p.type))
            {
                if(p.type == 'number')
                    item[p.name] = parseFloat(evt.target.innerText);
                else
                    item[p.name] = evt.target.innerText.replace(String.fromCharCode(10), "").replace(String.fromCharCode(13), "");
                if(inspector.notify)
                    inspector.notify.parameterChangeNotification(p);
            }
        });
    },


    createMenuRow(item, p)
    {
        const row = current_t_body.insertRow(-1);
        const value = item[p.name];
        const cell1 = row.insertCell(0);
        const cell2 = row.insertCell(1);

        cell1.innerText = p.name;

        var opts = p.options.split(',').map(o=>o.trim());
                        
        var s = '<select name="'+p.name+'">';
        for(var j in opts)
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
                item[p.name] = evt.target.value.trim();
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
        cell2.addEventListener("change", function(evt) { item[p.name] = evt.target.checked; if(this.notify) this.notify.parameterChangeNotification(p);});

    },

    

    createSliderRow(item, p)
    {
        this.createTextEditRow(item, p); // FIXME: TEMPORARY
    },


    addDataRows(item, template, notify=null)
    {
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
                        var opts = p.values.split(',').map(o=>o.trim());
                        
                        var s = '<select name="'+p.name+'">';
                        for(var j in opts)
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
                        var opts = p.values.split(',').map(o=>o.trim());
                        
                        var s = '<select name="'+p.name+'">';
                        for(var j in opts)
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

        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'block';

        inspector.addHeader("Group");
        if(main.edit_mode)
        {
            inspector.addDataRows(item, [{'name':'name', 'control':'textedit', 'type':'source'}], inspector);
        }
        else
        {
            inspector.addAttributeValue("name", item.name);
        }

        inspector.addAttributeValue("subgroups", (item.groups || []).length);
        inspector.addAttributeValue("modules", (item.modules || []).length);
        inspector.addAttributeValue("connections", (item.connections || []).length);
        inspector.addAttributeValue("widgets", (item.widgets || []).length);
    },


    showSingleSelection(c) 
    {
        const item = network.dict[c];
        inspector.hideSubviews();
        inspector.setTable(inspector.subview.table);
        inspector.subview.table.style.display = 'block';
    
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
        const commonDataRow = [{'name': 'name', 'control': 'textedit', 'type': 'source'}];
    
        switch (item._tag) {
            case "module":
                inspector.addHeader("MODULE");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                    inspector.addMenu("class", item.class, network.classes).addEventListener('change', function () {
                        network.changeModuleClass(selector.selected_foreground[0], this.value);
                        selector.selectItems(selector.selected_foreground);
                    });

                    const template = item.parameters || [];
                    for (let key in template) {
                        //template[key].control = "textedit";
                    }
                    inspector.addDataRows(item, template, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                    inspector.addAttributeValue("class", item.class);
                }
                const alternatives = ["quiet","exception","end_of_file","terminate","fatal_error","warning","print","debug","trace"];
                inspector.addMenu("log_level", alternatives[item.log_level], alternatives).addEventListener('change', function () { item.log_level=alternatives.indexOf(this.value) });

                break;
            
            case "group":
                inspector.addHeader("GROUP");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "input":
                inspector.addHeader("INPUT");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "output":
                inspector.addHeader("OUTPUT");
                if (editMode) {
                    inspector.addDataRows(item, commonDataRow, inspector);
                } else {
                    inspector.addAttributeValue("name", item.name);
                }
                break;
            
            case "widget":
                const widgetContainer = document.getElementById(`${selector.selected_background}.${item.name}`);
                inspector.addHeader("WIDGET");
                if (editMode) {
                    inspector.addMenu("class", item.class, widget_classes).addEventListener('change', function () {
                        network.changeWidgetClass(selector.selected_foreground[0], this.value);
                        selector.selectItems(selector.selected_foreground);
                    });
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
        inspector.subview.table.style.display = 'block';

        inspector.addHeader("CONNECTION");
        inspector.addAttributeValue("source", item.source);
        inspector.addAttributeValue("target", item.target);

            if(main.edit_mode)
            {
                inspector.addDataRows(item, 
                [
                    {'name':'source_range', 'control':'textedit', 'type':'range'},
                    {'name':'target_range', 'control':'textedit', 'type':'range'},
                    {'name':'delay', 'control':'textedit', 'type':'delay'},
                    {'name':'alias', 'control':'textedit', 'type':'source'}     
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
        inspector.subview.table.style.display = 'block';
        inspector.addHeader("Multiple");
        inspector.addAttributeValue("selected", n); 
    },

    showInspectorForSelection()
    {
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
    init()
    {
        log.view = document.querySelector('footer');
    },

    toggleLog()
    {
        const s = window.getComputedStyle(log.view, null);
        if (s.display === 'none')
            log.view.style.display = 'block';
        else 
        log.view.style.display = 'none';
    },

    showView()
    {
        log.view.style.display = 'block';
    },

    print(message)
    {
        let logElement = document.querySelector('.log');
        logElement.innerHTML += "<p class='warning'>"+message+"</p>\n";
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

    selectItems(foreground=[], background=null, toggle=false, extend=false)
    {
        selector.selected_connection = null;

        // Select background

        if(background != null)
            selector.selected_background = background;

        // Toggle foreground

        if(toggle)
            toggleStrings(selector.selected_foreground, foreground);
        else if(!selector.selected_foreground.includes(foreground[0]))
            selector.selected_foreground = foreground;

         if(extend)
            selector.selected_foreground = [...new Set([...selector.selected_foreground , ...foreground])];

        if(selector.selected_background == null)
            return;

        if(selector.selected_foreground.length==0) // select background group
        {
            nav.selectItem(selector.selected_background);
            breadcrumbs.selectItem(selector.selected_background);    
            main.selectItem([], selector.selected_background);
            inspector.showInspectorForSelection();
        }

        else // select forground components
        {
            main.selectItem(selector.selected_foreground, selector.selected_background);
            inspector.showInspectorForSelection();
        }
    },

    selectConnection(connection)
    {


        selector.selected_foreground = [];
        selector.selected_connection = connection;
        main.selectItem(selector.selected_foreground, selector.selected_background);
        main.deselectConnection(selector.selected_connection);
        main.selectConnection(connection);
        inspector.showInspectorForSelection();
    },

    getLocalPath(s) // Remove outer path and range
    {
        return removeStringFromStart(getStringUpToBracket(s), selector.selected_background+'.');
    }
}

const main = 
{
    view: null,
    grid: null,
    connections: "",
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
        main.drawGrid();
    },

    drawGrid()
    {
        const ctx = main.grid_canvas.getContext("2d");
        ctx.fillStyle = "white";
        ctx.fillRect(0, 0, main.grid_canvas.width, main.grid_canvas.height);
        ctx.lineWidth = 0.2;
        ctx.strokeStyle = "gray";
        for(x=main.grid_spacing; x<3000; x+=main.grid_spacing)
        {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, 3000);
            ctx.moveTo(0,x);
            ctx.lineTo(3000,x);
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
    
    newModule()
    {
        const name = network.uniqueID("Untitled_");
        const m =
        {
            name:name,
            class:"Module",
            log_level: 5,
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
        main.new_position_x += 30;
        main.new_position_y += 30;

        if(main.new_position_y >600)
        {
            main.new_position_x -= 350;
            main.new_position_y = 100;   
        }
        nav.populate();
        selector.selectItems([full_name]);
    },

    newGroup() // FIXME: Move to network
    {
        const name = network.uniqueID("Group_");
        const m =
        {
            name:name,
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
    },

    newInput() // FIXME: Move to network
    {
        const name = network.uniqueID("Input_");
        const m =
        {
            name:name,
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
    },

    newOutput() // FIXME: Move to network
    {
        const name = network.uniqueID("Output_");
        const m =
        {
            name:name,
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
    },

    newWidget()
    {
        const name = network.uniqueID("Widget_");
        const w = {
            "_tag": "widget",
            "name": name,
            "title": name,
            "class": "bar-graph",
            _x:main.new_position_x,
            _y:main.new_position_y,
            width: 200,
            height: 200
        };
        const full_name = selector.selected_background+'.'+name;

        network.dict[selector.selected_background].widgets.push(w);
        network.dict[full_name]=w;
        selector.selectItems([full_name]);
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
            selector.selectItems([], null);
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
    },

    changeComponentPosition(c, dx,dy)
    {
        const e = document.getElementById(c);
        let new_x = main.map[c][0] + dx;
        let new_y = main.map[c][1] + dy;

        if(new_x < 0)
                new_x = 0;

            if(new_y < 0)
                new_y = 0;

        if(main.edit_mode)
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
        if(evt.detail == 2) // handle double clicks elsewhere
        {
                evt.stopPropagation();
                return;
        }

       // this.style.pointerEvents = "none";

        main.initialMouseX = evt.clientX;
        main.initialMouseY = evt.clientY;

        selector.selectItems([this.dataset.name], null, evt.shiftKey);

        if(!selector.selected_foreground.includes(this.dataset.name))
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
            source_element: e 
        };
        main.view.addEventListener('mousemove',main.moveTrackedConnection, true);
        main.view.addEventListener('mouseup',main.releaseTrackedConnection,true);
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

    releaseTrackedConnection(evt) 
    {
        evt.stopPropagation();
    
        if (!main.tracked_connection)
            return;
    
        const { target, source } = main.tracked_connection;
    
        if (!target) 
        {
            main.tracked_connection = null;
            return;
        }
    
        const isTargetWidget = target in network.dict && network.dict[target]._tag === 'widget';
    
        if (isTargetWidget) 
        {
            network.dict[target].source = removeStringFromStart(source.split(':')[0], selector.selected_background + ".");
            main.tracked_connection = null;
            selector.selectItems([target], null);
        } 
        else 
        {
            const cleanSource = removeStringFromStart(source.split(':')[0], selector.selected_background + ".");
            const cleanTarget = removeStringFromStart(target.split(':')[0], selector.selected_background + ".");
            network.newConnection(selector.selected_background, cleanSource, cleanTarget);
            main.tracked_connection = null;
            selector.selectConnection(`${selector.selected_background}.${cleanSource}*${selector.selected_background}.${cleanTarget}`);
        }
    },

    
    setConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;
        this.style.backgroundColor="orange";
        if(this.id != "")   // FIXME: Not sure why this is needed
            main.tracked_connection.target = this.id;
    },

    resetConnectectionTarget(evt)
    {
        if(!main.tracked_connection)
            return;
            main.tracked_connection.target = null;
        if(this.classList.contains('widget'))
            this.style.backgroundColor="rgb(0,0,0,0)";
        else
            this.style.backgroundColor="gray";
    },

    addGroup(g,path)
        {
        const fullName = `${path}.${g.name}`;
        let s = "";
        s += `<div class='gi module group' style='top:${g._y}px;left:${g._x}px;'  id='${fullName}' data-name='${fullName}'>`;
        s += `<table>`;
        s += `<tr><td class='title' colspan='3'>${g.name}</td></tr>`;

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
        main.view.innerHTML += `<div class='gi group_input' id='${path}.${i.name}' data-name='${path}.${i.name}' style='top:${i._y}px;left:${i._x}px;'>
        ${i.name}
        <div class='o_spot'  id='${path}.${i.name}:out'></div>
        </div>`;
    },

    addOutput(o,path)
    {
        main.view.innerHTML += `<div class='gi group_output'  id='${path}.${o.name}' data-name='${path}.${o.name}'  style='top:${o._y}px;left:${o._x}px;'><div class='i_spot' id='${path}.${o.name}:in'></div>${o.name}</div>`;
    },

    addModule(m,path)
    {
         let s = "";
         s += `<div class='gi module' style='top:${m._y}px;left:${m._x}px;'   id='${path}.${m.name}' data-name='${path}.${m.name}'>`;
         s += `<table>`;
         s += `<tr><td class='title' colspan='3'>${m.name}</td></tr>`;

        for(let i of m.inputs || [])
            s += `<tr><td class='input'><div class='i_spot' id='${path}.${m.name}.${i.name}:in' onclick='alert(this.id)'></div></td ><td>${i.name}</td><td class='output'></td></tr>`;
  
        for(let o of m.outputs || [])
            s += `<tr><td class='output'></td><td>${o.name}</td><td class='output'><div class='o_spot' id='${path}.${m.name}.${o.name}:out'></div></td></tr>`;

         s += `</table>`;
         s += `</div>`;
        main.view.innerHTML += s;
    },

    addWidget(w, path) {
        const newObject = document.createElement("div");
        newObject.setAttribute("class", "frame visible gi widget");
    
        const newTitle = document.createElement("div");
        newTitle.setAttribute("class", "title");
        newTitle.innerHTML = w.name;
        newObject.appendChild(newTitle);
    
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
        const fullName = `${path}.${w.name}`;
        newObject.setAttribute("id", fullName);
        newObject.setAttribute("data-name", fullName);
    
        try {
            newObject.widget.updateAll();
        } catch (err) {
            console.log(err);
        }
    },

    addConnection(c,path)
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
        const cc = `<line x1='${x1}' y1='${y1}' x2='${x2}' y2='${y2}' class='connection_line' data-source='${c.source}' id="${path}.${source}*${path}.${target}" data-target='${target}'onclick='selector.selectConnection("${path}.${source}*${path}.${target}")'/>`; 
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

    selectConnection(connection)
    {
        const c = document.getElementById(connection);
        if(c != undefined) // FIXME: Use exception above instead
        {
            c.classList.add("selected");
            const st = connection.split('*');
            const s = document.getElementById(st[0]+":out");
            const t = document.getElementById(st[1]+":in");
            s.style.backgroundColor = "orange";
            t.style.backgroundColor = "orange";
        }
    },

    deselectConnection(connection)
    {
        const c = document.getElementById(connection);
        if(c != undefined) // FIXME: Use exception above instead
            c.classList.remove("selected");
    },

    addConnections()
    {
        const path = selector.selected_background;
        const group = network.dict[path];
        const s = document.getElementById("connections");
        if(s)
            s.innerHTML = "";
        main.connections = "<svg xmlns='http://www.w3.org/2000/svg' id='connections'>";
        for(let c of group.connections || [])
            main.addConnection(c,path);
        main.addTrackedConnection();
        main.connections += "</svg>";
        s.innerHTML += main.connections;
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

        // Add object handlers for all visible elements in main view depending on mode and tyep

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
                    e.ondblclick = function (evt) { selector.selectItems([this.dataset.name]); inspector.toggleComponent(); } // Select item and toggle inspector
     
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
        }
        else // View mode
        {
            for (let e of main.view.querySelectorAll(".gi"))
                if(!e.classList.contains("widget"))
                {
                    // Main drag function
                    e.addEventListener('mousedown', main.startDragComponents, false);   // FIXME: Change to select only hete
                
                    // Double click behaviors
                    if (e.classList.contains("group")) 
                        e.ondblclick = function (evt) { selector.selectItems([], this.dataset.name); } // Jump into group
                    else 
                        e.ondblclick = function (evt) { selector.selectItems([this.dataset.name]); inspector.toggleComponent(); } // Select item and toggle inspector
        
                    // Set selection class if selected
                    if(selectionList.includes(e.dataset.name))
                        e.classList.add("selected")
                    else
                        e.classList.remove("selected"); // FIXME: is this ever necessary?
                }

            // Add handlerer to outputs - pssoibly some form of inspection function hete
                /*
            for(let o of main.view.querySelectorAll(".o_spot"))
                o.addEventListener('mousedown', main.startTrackConnection, false);
         */
        }
/*
         for(let e of main.view.querySelectorAll(".gi"))
        {
            e.addEventListener('mousedown', main.startDragComponents, false);
            if(e.classList.contains("group"))
                e.ondblclick = function(evt) {  
                selector.selectItems([], this.dataset.name); // Jump into group
            }
            else
                e.ondblclick = function(evt)
            {  
                selector.selectItems([this.dataset.name]);
                inspector.toggleComponent();
            }

            if(selectionList.includes(e.dataset.name))
                e.classList.add("selected")
            else
                e.classList.remove("selected");
        }
        for(let o of main.view.querySelectorAll(".o_spot"))
            o.addEventListener('mousedown', main.startTrackConnection, false);

        for(let i of main.view.querySelectorAll(".i_spot"))
        {
            i.addEventListener('mouseover', main.setConnectectionTarget, true);
            i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
        }

        for(let i of main.view.querySelectorAll(".widget"))
        {
            i.addEventListener('mouseover', main.setConnectectionTarget, true);
            i.addEventListener('mouseleave', main.resetConnectectionTarget, true);
        }
        */
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
        inspector.showInspectorForSelection();
    },

    setViewMode()
    {
        main.main.classList.add("view_mode");
        main.main.classList.remove("edit_mode");
        main.edit_mode = false;
        inspector.showInspectorForSelection();
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
    },

    keydown(evt)
    {
        if(evt.key== "Escape")
        {
            inspector.toggleSystem();
            evt.preventDefault();
            return;
        }

        if(evt.key== "Backspace")
        {

            return;
            alert("Delete selected items. (NOT IMPLEMENTED YET)");
        }

        if(!evt.metaKey)
            return;
        if(evt.keyIdentifier=="Meta")
            return;

        if(evt.key=="a")
        {
            let bg = selector.selected_background;
            let g = network.dict[bg];
            let comps = [...g.groups||[], ...g.modules||[], ...g.inputs||[], ...g.outputs||[], ...g.widgets||[]];
            selector.selectItems(comps.map((x) => bg+'.'+x.name), null, false, true);
        }
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
        else if (evt.key=="d")
        {
            evt.preventDefault();
            alert("Duplicate selected items and possibly connections. (NOT IMPLEMENTED YET)");
        }
        else if (evt.key=="i")
        { 
            evt.preventDefault();
            inspector.toggleComponent();
            return;
        }
    },

    showCommandInfo()
    {
        alert("No info yet");
    }
}

const brainstudio = 
{
    init()
    {
        log.init();
        inspector.init();
        controller.init();
        nav.init();
        breadcrumbs.init();
        main.init();

        document.onkeydown = function (evt){ main.keydown(evt) };

    }
}


