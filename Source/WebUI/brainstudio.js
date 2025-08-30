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
    setCookie('selected_background', "");
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
    data_package: {},   // for debugging

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
            //log.print("Request timed out.");
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

            let v = getCookie('selected_background');
            if(v && network.dict[v])
                selector.selectItems([], v);
            else
                selector.selectItems([], top);

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
                controller.updateImages(response.data);
        }

        if(response.log)
        {
            let logElement = document.querySelector('.log');
            //if(response.log.length > 0)    
            //    console.log(response.log);

            response.log.forEach((element) => 
            {
                let message_class = ["inherit", "quiet","exception","end_of_file","terminate","fatal_error","warning","print","debug","trace"][element[0]];
                let s = "<p class='"+message_class+"'>"+element[1];
                if(element[2])
                    s += "<a onclick='selector.selectError("+ '"'+element[2]+'"' +")'> &raquo; "+element[2]+"</a>";
                s += "</p>\n";
                logElement.innerHTML += s;
                if(element[0]<=6)
                {
                    log.showView();
                    logElement.scrollTop = logElement.scrollHeight; // FIXME: Only when needed
                }
            });
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
                if(w[i].children[1] && 'requestData' in w[i].children[1])
                    w[i].children[1].requestData(data_set);
            }
            catch(err)
            {
                console.log("requestData failed: "+err);
            }

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
    },

    debug_data()
    {
        var w = window.open("about:blank", "", "_blank");
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

    showComponent()
    {
        inspector.component.style.display = "block";
        inspector.system.style.display = "none";
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
                                    item[p.name] = evt.target.value; if(inspector.notify) inspector.notify.parameterChangeNotification(p);


